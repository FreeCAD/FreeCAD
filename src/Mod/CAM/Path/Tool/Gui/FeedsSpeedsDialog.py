# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2024 sliptonic <shopinthewoods@gmail.com>
# SPDX-FileNotice: Part of the FreeCAD project.

################################################################################
#                                                                              #
#   FreeCAD is free software: you can redistribute it and/or modify            #
#   it under the terms of the GNU Lesser General Public License as             #
#   published by the Free Software Foundation, either version 2.1              #
#   of the License, or (at your option) any later version.                     #
#                                                                              #
#   FreeCAD is distributed in the hope that it will be useful,                 #
#   but WITHOUT ANY WARRANTY; without even the implied warranty                #
#   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                    #
#   See the GNU Lesser General Public License for more details.                #
#                                                                              #
#   You should have received a copy of the GNU Lesser General Public           #
#   License along with FreeCAD. If not, see https://www.gnu.org/licenses       #
#                                                                              #
################################################################################

"""
Suggest Feeds & Speeds dialog and supporting adapters.

Adapters convert FreeCAD doc objects into the resolver's frozen dataclass
contexts. The dialog shows current vs. suggested values side by side, lets
the user override the op-type combo for this resolution, and lets the user
pick a specific named preset from the tool to apply directly.

Presets are read-only in this dialog by design: creating and editing
presets is library curation, done in the ToolBit editor's Presets tab in
the library browser. Job-side flows only consume presets.
"""

from typing import Optional, Tuple

import FreeCAD
from PySide import QtGui, QtWidgets
from PySide.QtCore import QT_TRANSLATE_NOOP

import Path
from Path.Tool.FeedsSpeeds import (
    FeedSpeedResult,
    MachineContext,
    MaterialContext,
    OP_TYPES,
    OpContext,
    ToolContext,
    derive_preset_label,
    get_presets,
    resolve,
)
from Path.Tool.FeedsSpeeds.providers import _derive_from_engineering

translate = FreeCAD.Qt.translate


# Lazy-added TC properties (added on first dialog open).
TC_OP_TYPE_HINT = "OpTypeHint"
TC_PROVENANCE = "FeedSpeedProvenance"
TC_PROPERTY_GROUP = "Feeds & Speeds"


def ensure_tc_properties(tc) -> None:
    """Lazy-add OpTypeHint and FeedSpeedProvenance to the TC if absent."""
    if not hasattr(tc, TC_OP_TYPE_HINT):
        tc.addProperty(
            "App::PropertyEnumeration",
            TC_OP_TYPE_HINT,
            TC_PROPERTY_GROUP,
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Hint indicating which op category this TC is intended for",
            ),
        )
        tc.OpTypeHint = [""] + list(OP_TYPES)
        tc.OpTypeHint = ""
    if not hasattr(tc, TC_PROVENANCE):
        tc.addProperty(
            "App::PropertyMap",
            TC_PROVENANCE,
            TC_PROPERTY_GROUP,
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Per-field provenance: which source set HorizFeed/VertFeed/SpindleSpeed",
            ),
        )
        tc.FeedSpeedProvenance = {}


def _toolbit_from_tc(tc):
    """
    Returns the underlying ToolBit doc object for this TC, or None.
    """
    tool = getattr(tc, "Tool", None)
    return tool


def adapt_toolbit(tc) -> ToolContext:
    """Build a ToolContext from a TC's ToolBit doc object."""
    tb = _toolbit_from_tc(tc)
    diameter = 0.0
    flutes: Optional[int] = None
    presets: Tuple[dict, ...] = ()
    shape_id = ""
    tool_material: Optional[str] = None
    chipload_default: Optional[float] = None

    if tb is not None:
        try:
            diameter_q = getattr(tb, "Diameter", None)
            if diameter_q is not None:
                diameter = (
                    float(diameter_q.Value) if hasattr(diameter_q, "Value") else float(diameter_q)
                )
        except (AttributeError, TypeError, ValueError):
            diameter = 0.0
        try:
            f = getattr(tb, "Flutes", None)
            flutes = int(f) if f is not None else None
        except (TypeError, ValueError):
            flutes = None
        presets = tuple(get_presets(tb))
        shape_id = getattr(tb, "ShapeID", "") or ""
        tool_material = getattr(tb, "Material", None) or None
        try:
            chipload_q = getattr(tb, "Chipload", None)
            if chipload_q is not None:
                v = float(chipload_q.Value) if hasattr(chipload_q, "Value") else float(chipload_q)
                chipload_default = v if v > 0 else None
        except (AttributeError, TypeError, ValueError):
            chipload_default = None

    return ToolContext(
        diameter=diameter,
        flutes=flutes,
        presets=presets,
        shape_id=shape_id,
        tool_material=tool_material,
        chipload_default=chipload_default,
    )


def _surface_speed_m_per_min(props, key: str) -> Optional[float]:
    """Read a velocity property and return it in m/min, or None."""
    raw = props.get(key)
    if not raw:
        return None
    try:
        return float(FreeCAD.Units.Quantity(raw).getValueAs("m/min"))
    except (TypeError, ValueError, AttributeError):
        return None


def adapt_material_from_job(job) -> MaterialContext:
    """
    Pull the material context from a Job's Stock.ShapeMaterial.

    Returns an empty MaterialContext (uuid=None, name=None) when the job has
    no stock material assigned, which is treated as "any material" by the
    resolver. When the material carries the Machinability model, its
    ``SurfaceSpeedHSS`` and ``SurfaceSpeedCarbide`` properties are
    extracted (in m/min) and attached so ``MachinabilityProvider`` can
    pick a branch by tool material without re-reading the material.
    """
    if job is None:
        return MaterialContext()
    stock = getattr(job, "Stock", None)
    if stock is None:
        return MaterialContext()
    shape_material = getattr(stock, "ShapeMaterial", None)
    if shape_material is None:
        return MaterialContext()
    uuid = getattr(shape_material, "UUID", None) or None
    name = getattr(shape_material, "Name", None) or None
    props = getattr(shape_material, "PhysicalProperties", {}) or {}
    return MaterialContext(
        uuid=uuid,
        name=name,
        surface_speed_hss=_surface_speed_m_per_min(props, "SurfaceSpeedHSS"),
        surface_speed_carbide=_surface_speed_m_per_min(props, "SurfaceSpeedCarbide"),
    )


def adapt_machine_from_job(job) -> Optional[MachineContext]:
    """
    Build a MachineContext from the Job's Machine definition's first
    spindle. Returns None when the job has no Machine configured or the
    machine has no spindles. The resolver treats None as "no clamping."
    """
    if job is None or not hasattr(job, "Proxy"):
        return None
    try:
        machine = job.Proxy.getMachine()
    except (AttributeError, TypeError):
        return None
    if machine is None:
        return None
    spindles = getattr(machine, "spindles", None) or []
    if not spindles:
        return None
    s = spindles[0]
    return MachineContext(
        min_rpm=float(getattr(s, "min_rpm", 0.0) or 0.0),
        max_rpm=float(getattr(s, "max_rpm", 0.0) or 0.0),
    )


def find_job_for_tc(tc):
    """Walk InListRecursive to find the Job that owns this TC, if any."""
    if tc is None:
        return None
    inlist = getattr(tc, "InListRecursive", []) or []
    for parent in inlist:
        if hasattr(parent, "Proxy") and parent.Proxy.__class__.__name__ == "ObjectJob":
            return parent
        if getattr(parent, "TypeId", "") == "Path::FeatureCompoundPython" and hasattr(
            parent, "Stock"
        ):
            return parent
    return None


def write_result_to_tc(tc, result: FeedSpeedResult) -> None:
    """
    Apply a resolver result to the TC. Resolver feeds are mm/min and are
    converted to mm/s when written to ``HorizFeed``/``VertFeed`` (the TC
    stores ``App::PropertyVelocity`` in mm/s). ``SpindleSpeed`` is rpm
    and passes through unchanged. Stamps ``FeedSpeedProvenance`` for
    each field actually written.
    """
    if result.source == "":
        return
    ensure_tc_properties(tc)
    provenance = dict(getattr(tc, TC_PROVENANCE, {}) or {})

    try:
        if result.horiz_feed is not None:
            tc.HorizFeed = result.horiz_feed / 60.0  # mm/min -> mm/s
            provenance["HorizFeed"] = result.source
        if result.vert_feed is not None:
            tc.VertFeed = result.vert_feed / 60.0  # mm/min -> mm/s
            provenance["VertFeed"] = result.source
        if result.spindle_speed is not None:
            tc.SpindleSpeed = float(result.spindle_speed)
            provenance["SpindleSpeed"] = result.source
    finally:
        setattr(tc, TC_PROVENANCE, provenance)


def _format_speed(value: Optional[float], unit: str) -> str:
    if value is None:
        return "—"
    return f"{value:.1f} {unit}"


def _delta_label(current: float, suggested: Optional[float]) -> str:
    if suggested is None:
        return ""
    delta = suggested - current
    sign = "+" if delta >= 0 else ""
    return f"({sign}{delta:.1f})"


def doc_unit_schema(obj) -> int:
    """
    The unit-schema index of ``obj``'s document, so feeds display in the
    document's unit system (e.g. in/min for an imperial project) rather
    than a hardcoded unit. Falls back to the active/global schema when the
    document has no explicit ``UnitSystem`` set.
    """
    try:
        doc = getattr(obj, "Document", None)
        if doc is not None:
            # The document's ``UnitSystem`` enumeration lists the schemas in
            # schema-index order, so the position of the current value is the
            # schema number ``schemaTranslate`` expects.
            names = doc.getEnumerationsOfProperty("UnitSystem")
            if names and doc.UnitSystem in names:
                return names.index(doc.UnitSystem)
    except Exception:
        pass
    try:
        return FreeCAD.Units.getSchema()
    except Exception:
        return 0


def _format_feed(value: Optional[float], schema: int) -> str:
    """Format a feed (given in mm/min) in the given document unit schema."""
    if value is None:
        return "—"
    try:
        quantity = FreeCAD.Units.Quantity(float(value), "mm/min")
        return FreeCAD.Units.schemaTranslate(quantity, schema)[0]
    except Exception:
        return f"{value:.1f} mm/min"


def _delta_feed(current: float, suggested: Optional[float], schema: int) -> str:
    """Signed feed delta (mm/min) formatted in the document unit schema."""
    if suggested is None:
        return ""
    delta = suggested - current
    sign = "+" if delta >= 0 else "-"
    return f"({sign}{_format_feed(abs(delta), schema)})"


class FeedsSpeedsDialog(QtWidgets.QDialog):
    """
    Suggest dialog for a single Tool Controller. Shows the resolver's
    suggestion against current TC values; "Apply" writes the suggestion
    back to the TC.
    """

    def __init__(self, tc, parent=None):
        super().__init__(parent)
        self._centered = False
        self.tc = tc
        ensure_tc_properties(tc)
        self.toolbit = _toolbit_from_tc(tc)
        self.job = find_job_for_tc(tc)
        self.material_ctx = adapt_material_from_job(self.job)
        self.machine_ctx = adapt_machine_from_job(self.job)
        self.tool_ctx = adapt_toolbit(tc)
        self._result: Optional[FeedSpeedResult] = None
        # Feeds are shown in the document's unit schema, not a hardcoded unit.
        self._schema = doc_unit_schema(tc)

        self.setWindowTitle(translate("CAM_FeedsSpeeds", "Suggest Feeds & Speeds"))
        self._build_ui()
        self._refresh()

    def showEvent(self, event) -> None:
        super().showEvent(event)
        if not self._centered:
            self._centered = True
            screen = QtWidgets.QApplication.primaryScreen()
            if screen is not None:
                geo = screen.availableGeometry()
                frame = self.frameGeometry()
                frame.moveCenter(geo.center())
                self.move(frame.topLeft())

    def _build_ui(self) -> None:
        outer = QtWidgets.QVBoxLayout(self)

        # Context summary
        ctx_form = QtWidgets.QFormLayout()
        tool_label = (
            self.toolbit.Label if self.toolbit else translate("CAM_FeedsSpeeds", "(no tool)")
        )
        ctx_form.addRow(translate("CAM_FeedsSpeeds", "Tool:"), QtWidgets.QLabel(tool_label))
        material_label = self.material_ctx.name or translate(
            "CAM_FeedsSpeeds", "(none — generic resolution)"
        )
        ctx_form.addRow(translate("CAM_FeedsSpeeds", "Material:"), QtWidgets.QLabel(material_label))

        self.op_type_combo = QtWidgets.QComboBox()
        self.op_type_combo.addItem(translate("CAM_FeedsSpeeds", "(any)"), None)
        for op in OP_TYPES:
            self.op_type_combo.addItem(op, op)
        current_hint = getattr(self.tc, TC_OP_TYPE_HINT, "") or None
        if current_hint:
            idx = self.op_type_combo.findData(current_hint)
            if idx >= 0:
                self.op_type_combo.setCurrentIndex(idx)
        self.op_type_combo.currentIndexChanged.connect(self._refresh)
        ctx_form.addRow(translate("CAM_FeedsSpeeds", "Op type:"), self.op_type_combo)

        # Preset picker — sentinel index 0 = "Auto (use resolver)".
        # Other entries pick a specific named preset directly, bypassing
        # the resolver's specificity ranking.
        self.preset_combo = QtWidgets.QComboBox()
        self._populate_preset_combo()
        self.preset_combo.currentIndexChanged.connect(self._on_preset_picked)
        ctx_form.addRow(translate("CAM_FeedsSpeeds", "Apply preset:"), self.preset_combo)
        outer.addLayout(ctx_form)

        # Suggestion frame
        self.suggestion_box = QtWidgets.QGroupBox(translate("CAM_FeedsSpeeds", "Suggestion"))
        sb_layout = QtWidgets.QFormLayout(self.suggestion_box)

        self.source_label = QtWidgets.QLabel("—")
        sb_layout.addRow(translate("CAM_FeedsSpeeds", "Source:"), self.source_label)

        self.confidence_bar = QtWidgets.QProgressBar()
        self.confidence_bar.setRange(0, 100)
        self.confidence_bar.setValue(0)
        self.confidence_bar.setTextVisible(True)
        sb_layout.addRow(translate("CAM_FeedsSpeeds", "Confidence:"), self.confidence_bar)

        # Comparison grid
        self.compare_grid = QtWidgets.QGridLayout()
        self.compare_grid.addWidget(QtWidgets.QLabel(""), 0, 0)
        self.compare_grid.addWidget(QtWidgets.QLabel(translate("CAM_FeedsSpeeds", "Current")), 0, 1)
        self.compare_grid.addWidget(
            QtWidgets.QLabel(translate("CAM_FeedsSpeeds", "Suggested")), 0, 2
        )
        self.compare_grid.addWidget(QtWidgets.QLabel(translate("CAM_FeedsSpeeds", "Δ")), 0, 3)
        self._row_labels: list[Tuple[QtWidgets.QLabel, QtWidgets.QLabel, QtWidgets.QLabel]] = []
        for i, name in enumerate(("Spindle", "Horiz feed", "Vert feed"), start=1):
            self.compare_grid.addWidget(QtWidgets.QLabel(translate("CAM_FeedsSpeeds", name)), i, 0)
            cur = QtWidgets.QLabel("—")
            sug = QtWidgets.QLabel("—")
            dlt = QtWidgets.QLabel("")
            self.compare_grid.addWidget(cur, i, 1)
            self.compare_grid.addWidget(sug, i, 2)
            self.compare_grid.addWidget(dlt, i, 3)
            self._row_labels.append((cur, sug, dlt))
        sb_layout.addRow(self.compare_grid)

        self.warnings_label = QtWidgets.QLabel("")
        self.warnings_label.setWordWrap(True)
        palette = self.warnings_label.palette()
        palette.setColor(QtGui.QPalette.WindowText, QtGui.QColor("#b07000"))
        self.warnings_label.setPalette(palette)
        sb_layout.addRow(self.warnings_label)

        outer.addWidget(self.suggestion_box)

        # Buttons
        self.button_box = QtWidgets.QDialogButtonBox(
            QtWidgets.QDialogButtonBox.Cancel | QtWidgets.QDialogButtonBox.Apply
        )
        self.apply_button = self.button_box.button(QtWidgets.QDialogButtonBox.Apply)
        self.apply_button.clicked.connect(self._on_apply)
        self.button_box.rejected.connect(self.reject)
        outer.addWidget(self.button_box)

    def _populate_preset_combo(self) -> None:
        """Fill the preset picker combo with the tool's saved presets."""
        self.preset_combo.blockSignals(True)
        try:
            self.preset_combo.clear()
            self.preset_combo.addItem(translate("CAM_FeedsSpeeds", "Auto (use resolver)"), None)
            presets = list(self.tool_ctx.presets) if self.tool_ctx else []
            for i, preset in enumerate(presets):
                label = derive_preset_label(preset)
                self.preset_combo.addItem(label, i)  # data = index into presets list
        finally:
            self.preset_combo.blockSignals(False)

    def _on_preset_picked(self, _idx: int) -> None:
        """Triggered when the user selects a specific preset to apply."""
        self._refresh()

    def _build_result_from_preset(self, preset: dict) -> Optional[FeedSpeedResult]:
        """
        Derive a FeedSpeedResult from a single preset, ignoring the resolver's
        match logic. Called when the user explicitly picks a preset by name.
        """
        surface_speed = preset.get("surface_speed")
        chipload = preset.get("chipload")
        vert_ratio = preset.get("vert_feed_ratio", 0.33)
        warnings = []

        spindle = horiz = vert = None
        if surface_speed is not None or chipload is not None:
            spindle, horiz, vert, derive_warnings = _derive_from_engineering(
                surface_speed, chipload, vert_ratio, self.tool_ctx.diameter, self.tool_ctx.flutes
            )
            warnings.extend(derive_warnings)

        if spindle is None and horiz is None and vert is None:
            return None

        label = derive_preset_label(preset)
        return FeedSpeedResult(
            horiz_feed=horiz,
            vert_feed=vert,
            spindle_speed=spindle,
            chipload=chipload,
            surface_speed=surface_speed,
            source=f"preset:tool/manual/{label}",
            confidence=1.0,
            warnings=tuple(warnings),
        )

    def _current_op_ctx(self) -> OpContext:
        return OpContext(op_type=self.op_type_combo.currentData())

    def _current_horiz(self) -> float:
        try:
            v = self.tc.HorizFeed
            mm_s = float(v.Value) if hasattr(v, "Value") else float(v)
            return mm_s * 60.0  # mm/s -> mm/min
        except (AttributeError, TypeError, ValueError):
            return 0.0

    def _current_vert(self) -> float:
        try:
            v = self.tc.VertFeed
            mm_s = float(v.Value) if hasattr(v, "Value") else float(v)
            return mm_s * 60.0  # mm/s -> mm/min
        except (AttributeError, TypeError, ValueError):
            return 0.0

    def _current_spindle(self) -> float:
        try:
            return float(getattr(self.tc, "SpindleSpeed", 0.0) or 0.0)
        except (TypeError, ValueError):
            return 0.0

    def _refresh(self) -> None:
        # Re-fetch tool ctx in case presets changed (after Save).
        self.tool_ctx = adapt_toolbit(self.tc)

        # If a specific preset was picked from the combo, build the result
        # directly from it. Otherwise run the resolver chain as usual.
        chosen_idx = self.preset_combo.currentData()
        if chosen_idx is not None and 0 <= chosen_idx < len(self.tool_ctx.presets):
            picked = self.tool_ctx.presets[chosen_idx]
            built = self._build_result_from_preset(picked)
            result = built if built is not None else FeedSpeedResult()
        else:
            op_ctx = self._current_op_ctx()
            result = resolve(self.tool_ctx, self.material_ctx, op_ctx, machine=self.machine_ctx)
        self._result = result

        cur_spindle = self._current_spindle()
        cur_horiz = self._current_horiz()
        cur_vert = self._current_vert()

        self._row_labels[0][0].setText(_format_speed(cur_spindle, "rpm"))
        self._row_labels[1][0].setText(_format_feed(cur_horiz, self._schema))
        self._row_labels[2][0].setText(_format_feed(cur_vert, self._schema))

        if not result.source:
            self.source_label.setText(translate("CAM_FeedsSpeeds", "No suggestion available"))
            self.confidence_bar.setValue(0)
            for _, sug, dlt in self._row_labels:
                sug.setText("—")
                dlt.setText("")
            self.warnings_label.setText(
                translate(
                    "CAM_FeedsSpeeds",
                    "No matching preset on this tool. Open the tool from the library "
                    "to add presets.",
                )
            )
            self.apply_button.setEnabled(False)
            return

        self.source_label.setText(result.source)
        self.confidence_bar.setValue(int(round(result.confidence * 100)))

        self._row_labels[0][1].setText(_format_speed(result.spindle_speed, "rpm"))
        self._row_labels[0][2].setText(
            _delta_label(cur_spindle, result.spindle_speed) if result.spindle_speed else ""
        )
        self._row_labels[1][1].setText(_format_feed(result.horiz_feed, self._schema))
        self._row_labels[1][2].setText(
            _delta_feed(cur_horiz, result.horiz_feed, self._schema) if result.horiz_feed else ""
        )
        self._row_labels[2][1].setText(_format_feed(result.vert_feed, self._schema))
        self._row_labels[2][2].setText(
            _delta_feed(cur_vert, result.vert_feed, self._schema) if result.vert_feed else ""
        )

        self.warnings_label.setText("\n".join(result.warnings))
        self.apply_button.setEnabled(True)

    def _on_apply(self) -> None:
        if self._result is None or not self._result.source:
            return
        write_result_to_tc(self.tc, self._result)
        # Persist the op-type choice if user changed it.
        chosen = self.op_type_combo.currentData() or ""
        if getattr(self.tc, TC_OP_TYPE_HINT, None) != chosen:
            self.tc.OpTypeHint = chosen
        try:
            self.tc.Document.recompute()
        except Exception:
            Path.Log.debug("recompute after F&S apply failed; ignoring")
        self.accept()


def open_for(tc, parent=None) -> None:
    """Convenience entry: open the dialog modally for the given TC."""
    dlg = FeedsSpeedsDialog(tc, parent=parent)
    dlg.exec_()
