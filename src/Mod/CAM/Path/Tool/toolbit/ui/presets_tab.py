# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 sliptonic <shopinthewoods@gmail.com>
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
Presets tab for the ToolBit editor. Lists the feeds & speeds presets stored
on a ToolBit and supports add/edit/delete; each operation is written back to
the ``Presets`` property immediately. Used as the last tab in the existing
ToolBit editor's QTabWidget.
"""

import math
from typing import List, Optional

import FreeCAD
import FreeCADGui
from PySide import QtCore, QtGui

from ...FeedsSpeeds import (
    OP_TYPES,
    get_presets,
    make_preset,
    set_presets,
)

translate = FreeCAD.Qt.translate

# ``Gui::QuantitySpinBox`` stores its ``rawValue`` in the quantity's base
# unit: mm/s for a velocity, mm for a length. The engineering math below
# works in m/min (surface speed), mm/min (feed) and mm (chipload), so we
# convert at the widget boundary. Chipload is already in mm = base unit.
_MM_S_PER_M_MIN = 1000.0 / 60.0  # 1 m/min expressed in mm/s
_MM_S_PER_MM_MIN = 1.0 / 60.0  # 1 mm/min expressed in mm/s


def _fmt(value: Optional[float], unit: str = "") -> str:
    if value is None:
        return ""
    if unit:
        return f"{value:g} {unit}"
    return f"{value:g}"


def _preferred_unit(unit_expr: str) -> str:
    """
    The user's schema-preferred unit string for the dimension of
    ``unit_expr`` (e.g. "mm/s" -> "mm/min" or "in/min", "mm" -> "mm" or
    "in"). Falls back to ``unit_expr`` if the preference can't be resolved.
    """
    try:
        pref = FreeCAD.Units.Quantity(1.0, unit_expr).getUserPreferred()
        if pref and len(pref) > 2 and pref[2]:
            return pref[2]
    except Exception:
        pass
    return unit_expr


def _fmt_quantity(value: Optional[float], unit_expr: str) -> str:
    """
    Format a magnitude (given in ``unit_expr`` units) as a schema-aware
    display string, so table columns honor the user's unit preference.
    """
    if value is None:
        return ""
    try:
        return FreeCAD.Units.Quantity(float(value), unit_expr).UserString
    except Exception:
        return _fmt(value)


def _spin_raw(widget) -> float:
    """Read a ``Gui::QuantitySpinBox`` value in its base unit (mm/s, mm)."""
    try:
        return float(widget.property("rawValue"))
    except (TypeError, ValueError):
        return 0.0


def _set_spin_raw(widget, base_value: float) -> None:
    """Set a ``Gui::QuantitySpinBox`` from a base-unit value, muted."""
    widget.blockSignals(True)
    try:
        widget.setProperty("rawValue", float(base_value))
    finally:
        widget.blockSignals(False)


def _surface_speed_from_rpm(rpm: float, diameter_mm: float) -> Optional[float]:
    """surface_speed (m/min) = rpm * pi * d / 1000."""
    if rpm <= 0 or diameter_mm <= 0:
        return None
    return (rpm * math.pi * diameter_mm) / 1000.0


def _rpm_from_surface_speed(surface_speed: float, diameter_mm: float) -> Optional[float]:
    """rpm = surface_speed (m/min) * 1000 / (pi * d_mm)."""
    if surface_speed <= 0 or diameter_mm <= 0:
        return None
    return (surface_speed * 1000.0) / (math.pi * diameter_mm)


def _chipload_from_feed(feed_mm_min: float, rpm: float, flutes: int) -> Optional[float]:
    """chipload (mm/tooth) = feed / (rpm * flutes)."""
    if feed_mm_min <= 0 or rpm <= 0 or flutes <= 0:
        return None
    return feed_mm_min / (rpm * flutes)


def _feed_from_chipload(chipload: float, rpm: float, flutes: int) -> Optional[float]:
    """feed (mm/min) = chipload * rpm * flutes."""
    if chipload <= 0 or rpm <= 0 or flutes <= 0:
        return None
    return chipload * rpm * flutes


class _EditPresetDialog:
    """Sub-dialog for adding or editing a single preset row.

    Wraps the ``FeedsSpeedsPresetEdit.ui`` panel. The dialog accepts the
    tool's current ``diameter_mm`` and ``flutes`` so it can keep the
    engineering inputs (surface speed, chipload) and the direct inputs
    (spindle rpm, horiz feed) in sync as the user types. Storage is
    engineering-only: only surface_speed, chipload and vert_feed_ratio are
    persisted; the rpm/feed spinboxes are display affordances derived from
    those plus the tool's geometry.

    All feed/speed spinboxes are ``Gui::QuantitySpinBox`` widgets so their
    display honors the user's unit schema (metric, imperial, …). Values are
    read/written in the base unit via ``rawValue`` and converted to the
    engineering units the math works in (m/min, mm/min, mm).
    """

    def __init__(
        self,
        preset: Optional[dict],
        diameter_mm: float = 0.0,
        flutes: Optional[int] = None,
        tool_chipload_mm: Optional[float] = None,
        parent=None,
    ):
        self.form = FreeCADGui.PySideUic.loadUi(":/panels/FeedsSpeedsPresetEdit.ui")
        if parent is not None:
            self.form.setParent(parent, QtCore.Qt.Dialog)

        # A new preset (preset is None) seeds its chipload from the tool's
        # own ``Chipload`` property; an existing preset keeps its saved value.
        self._is_new = preset is None
        self._initial = preset or make_preset()
        self._material_uuid: Optional[str] = None
        self._material_name: Optional[str] = None
        self._diameter_mm = float(diameter_mm) if diameter_mm else 0.0
        self._flutes = int(flutes) if flutes else 0
        self._tool_chipload_mm = float(tool_chipload_mm) if tool_chipload_mm else 0.0
        self._syncing = False  # guards against feedback loops

        f = self.form
        self.name_edit = f.nameEdit
        self.material_label = f.materialLabel
        self.browse_button = f.browseButton
        self.generic_check = f.genericCheck
        self.op_type_combo = f.opTypeCombo
        self.surface_speed_spin = f.surfaceSpeedSpin
        self.chipload_spin = f.chiploadSpin
        self.vert_ratio_spin = f.vertRatioSpin
        self.raw_feed_spin = f.rawFeedSpin
        self.raw_speed_spin = f.rawSpeedSpin

        self.name_edit.setPlaceholderText(
            translate("CAM_FeedsSpeeds", "Optional, e.g. 'Aluminum aggressive'")
        )

        # Display units follow the user's unit schema. Surface speed and
        # horiz feed are velocities; chipload is a length (per tooth).
        vel_unit = _preferred_unit("mm/s")
        self.surface_speed_spin.setProperty("unit", vel_unit)
        self.raw_feed_spin.setProperty("unit", vel_unit)
        self.chipload_spin.setProperty("unit", _preferred_unit("mm"))
        # Chipload needs finer resolution than the default spinbox decimals.
        # Gui::QuantitySpinBox comes through PySide as a bare QAbstractSpinBox
        # (no typed setters), so drive it through the Qt property system —
        # the same way its ``unit`` and ``rawValue`` are set.
        self.chipload_spin.setProperty("decimals", 4)

        # Op type
        self.op_type_combo.addItem(translate("CAM_FeedsSpeeds", "(any)"), None)
        for op in OP_TYPES:
            self.op_type_combo.addItem(op, op)

        # Wire up bidirectional sync. Each handler re-reads the widgets it
        # needs (via unit-aware accessors) and ignores the signal payload,
        # so it doesn't matter whether the signal carries a str or a float.
        self.browse_button.clicked.connect(self._on_browse)
        self.generic_check.toggled.connect(self._on_generic_toggled)
        self.surface_speed_spin.textChanged.connect(self._on_surface_speed_changed)
        self.chipload_spin.textChanged.connect(self._on_chipload_changed)
        self.raw_feed_spin.textChanged.connect(self._on_raw_feed_changed)
        self.raw_speed_spin.valueChanged.connect(self._on_raw_speed_changed)

        # The hint is only relevant when geometry is missing and sync can't
        # run; hide it otherwise.
        self.form.geometryHint.setVisible(self._diameter_mm <= 0 or self._flutes <= 0)

        self._populate_from_preset(self._initial)

    def exec_(self) -> int:
        return self.form.exec_()

    # ------------------------------------------------------------------
    # Unit-aware accessors. Getters return the engineering unit the math
    # uses (surface speed m/min, feed mm/min, chipload mm, spindle rpm);
    # setters take the same and push the base-unit value into the widget.
    # ------------------------------------------------------------------

    def _get_surface_speed(self) -> float:  # m/min
        return _spin_raw(self.surface_speed_spin) / _MM_S_PER_M_MIN

    def _set_surface_speed(self, ss_m_min: float) -> None:
        _set_spin_raw(self.surface_speed_spin, ss_m_min * _MM_S_PER_M_MIN)

    def _get_feed(self) -> float:  # mm/min
        return _spin_raw(self.raw_feed_spin) / _MM_S_PER_MM_MIN

    def _set_feed(self, feed_mm_min: float) -> None:
        _set_spin_raw(self.raw_feed_spin, feed_mm_min * _MM_S_PER_MM_MIN)

    def _get_chipload(self) -> float:  # mm (== base unit)
        return _spin_raw(self.chipload_spin)

    def _set_chipload(self, cl_mm: float) -> None:
        _set_spin_raw(self.chipload_spin, cl_mm)

    def _get_rpm(self) -> float:
        return self.raw_speed_spin.value()

    def _set_rpm(self, rpm: float) -> None:
        self.raw_speed_spin.blockSignals(True)
        try:
            self.raw_speed_spin.setValue(rpm)
        finally:
            self.raw_speed_spin.blockSignals(False)

    def _populate_from_preset(self, preset: dict) -> None:
        if preset.get("name"):
            self.name_edit.setText(preset["name"])
        hint = preset.get("material_hint")
        if hint is None:
            self.generic_check.setChecked(True)
        else:
            self._material_uuid = hint.get("uuid") or None
            self._material_name = hint.get("name") or None
            self._refresh_material_label()
        op = preset.get("op_type_hint")
        if op is not None:
            idx = self.op_type_combo.findData(op)
            if idx >= 0:
                self.op_type_combo.setCurrentIndex(idx)
        # Storage carries only the engineering values. Load those, then
        # derive the raw display values from them and the tool's geometry.
        # Suppress the sync handlers during this initial population so the
        # derivation happens once, controlled, instead of cascading.
        self._syncing = True
        try:
            ss = preset.get("surface_speed")
            cl = preset.get("chipload")
            # Seed a brand-new preset's chipload from the tool's own
            # Chipload property. This is a one-time convenience at creation:
            # it never overwrites an existing preset's value, and the saved
            # preset stays independent of the tool afterward.
            if cl is None and self._is_new and self._tool_chipload_mm > 0:
                cl = self._tool_chipload_mm
            if ss is not None:
                self._set_surface_speed(ss)
            if cl is not None:
                self._set_chipload(cl)
            self.vert_ratio_spin.setValue(preset.get("vert_feed_ratio", 0.33))

            # Compute raw display values from engineering + geometry.
            rpm = _rpm_from_surface_speed(ss, self._diameter_mm) if ss is not None else None
            if rpm is not None:
                self._set_rpm(rpm)
                if cl is not None and self._flutes > 0:
                    rf = _feed_from_chipload(cl, rpm, self._flutes)
                    if rf is not None:
                        self._set_feed(rf)
        finally:
            self._syncing = False

    # ------------------------------------------------------------------
    # Bidirectional sync between engineering values (surface_speed, chipload)
    # and direct values (raw_speed, raw_feed). Each handler returns early
    # when self._syncing is set, breaking feedback loops.
    # ------------------------------------------------------------------

    def _on_surface_speed_changed(self, *_) -> None:
        if self._syncing:
            return
        self._syncing = True
        try:
            rpm = _rpm_from_surface_speed(self._get_surface_speed(), self._diameter_mm)
            if rpm is not None:
                self._set_rpm(rpm)
                cl = self._get_chipload()
                if cl > 0 and self._flutes > 0:
                    rf = _feed_from_chipload(cl, rpm, self._flutes)
                    if rf is not None:
                        self._set_feed(rf)
        finally:
            self._syncing = False

    def _on_chipload_changed(self, *_) -> None:
        if self._syncing:
            return
        self._syncing = True
        try:
            rpm = self._get_rpm()
            rf = _feed_from_chipload(self._get_chipload(), rpm, self._flutes)
            if rf is not None:
                self._set_feed(rf)
        finally:
            self._syncing = False

    def _on_raw_speed_changed(self, *_) -> None:
        if self._syncing:
            return
        self._syncing = True
        try:
            rpm = self._get_rpm()
            ss = _surface_speed_from_rpm(rpm, self._diameter_mm)
            if ss is not None:
                self._set_surface_speed(ss)
            cl = _chipload_from_feed(self._get_feed(), rpm, self._flutes)
            if cl is not None:
                self._set_chipload(cl)
        finally:
            self._syncing = False

    def _on_raw_feed_changed(self, *_) -> None:
        if self._syncing:
            return
        self._syncing = True
        try:
            cl = _chipload_from_feed(self._get_feed(), self._get_rpm(), self._flutes)
            if cl is not None:
                self._set_chipload(cl)
        finally:
            self._syncing = False

    def _on_generic_toggled(self, generic: bool) -> None:
        self.material_label.setEnabled(not generic)
        self.browse_button.setEnabled(not generic)

    def _on_browse(self) -> None:
        from Path.Tool.Gui.MaterialPicker import pick_machinability_material

        result = pick_machinability_material(self.form)
        if result is None:
            return
        self._material_uuid, self._material_name = result
        self._refresh_material_label()
        self.generic_check.setChecked(False)

    def _refresh_material_label(self) -> None:
        if self._material_name:
            self.material_label.setText(self._material_name)
        elif self._material_uuid:
            self.material_label.setText(self._material_uuid)
        else:
            self.material_label.setText(translate("CAM_FeedsSpeeds", "(none)"))

    def build_preset(self) -> dict:
        if self.generic_check.isChecked():
            uuid = None
            name = None
        else:
            uuid = self._material_uuid
            name = self._material_name
        # Storage is engineering-only. The raw_speed/raw_feed spinboxes
        # are display-and-entry affordances; whatever the user typed
        # there is already reflected in surface_speed/chipload via the
        # bidirectional sync, so we only persist the engineering side.
        return make_preset(
            name=self.name_edit.text().strip() or None,
            material_uuid=uuid,
            material_name=name,
            op_type=self.op_type_combo.currentData(),
            surface_speed=self._get_surface_speed() or None,
            chipload=self._get_chipload() or None,
            vert_feed_ratio=self.vert_ratio_spin.value(),
        )


class PresetsTab(QtGui.QWidget):
    """
    Tab widget showing the ToolBit's saved presets in a table, with
    add/edit/delete buttons.
    """

    def __init__(self, toolbit_obj, parent=None):
        super().__init__(parent)
        self._obj = toolbit_obj

        layout = QtGui.QVBoxLayout(self)

        self.table = QtGui.QTableWidget()
        self.table.setColumnCount(6)
        self.table.setHorizontalHeaderLabels(
            [
                translate("CAM_FeedsSpeeds", "Name"),
                translate("CAM_FeedsSpeeds", "Material"),
                translate("CAM_FeedsSpeeds", "Op type"),
                translate("CAM_FeedsSpeeds", "Surface speed"),
                translate("CAM_FeedsSpeeds", "Chipload"),
                translate("CAM_FeedsSpeeds", "Notes"),
            ]
        )
        self.table.horizontalHeader().setStretchLastSection(True)
        self.table.setSelectionBehavior(QtGui.QAbstractItemView.SelectRows)
        self.table.setEditTriggers(QtGui.QAbstractItemView.NoEditTriggers)
        layout.addWidget(self.table)

        button_row = QtGui.QHBoxLayout()
        button_row.addStretch()
        self.add_button = QtGui.QPushButton(translate("CAM_FeedsSpeeds", "Add preset"))
        self.add_button.clicked.connect(self._on_add)
        button_row.addWidget(self.add_button)

        self.edit_button = QtGui.QPushButton(translate("CAM_FeedsSpeeds", "Edit"))
        self.edit_button.clicked.connect(self._on_edit)
        button_row.addWidget(self.edit_button)

        self.delete_button = QtGui.QPushButton(translate("CAM_FeedsSpeeds", "Delete"))
        self.delete_button.clicked.connect(self._on_delete)
        button_row.addWidget(self.delete_button)

        layout.addLayout(button_row)
        self.refresh()

    def _presets(self) -> List[dict]:
        return list(get_presets(self._obj))

    def refresh(self) -> None:
        presets = self._presets()
        self.table.setRowCount(len(presets))
        for i, p in enumerate(presets):
            hint = p.get("material_hint")
            if hint is None:
                mat_text = translate("CAM_FeedsSpeeds", "(any material)")
            else:
                mat_text = hint.get("name") or hint.get("uuid") or "?"
            op_text = p.get("op_type_hint") or translate("CAM_FeedsSpeeds", "(any)")
            name_text = p.get("name") or ""
            self.table.setItem(i, 0, QtGui.QTableWidgetItem(name_text))
            self.table.setItem(i, 1, QtGui.QTableWidgetItem(mat_text))
            self.table.setItem(i, 2, QtGui.QTableWidgetItem(op_text))
            self.table.setItem(
                i, 3, QtGui.QTableWidgetItem(_fmt_quantity(p.get("surface_speed"), "m/min"))
            )
            self.table.setItem(i, 4, QtGui.QTableWidgetItem(_fmt_quantity(p.get("chipload"), "mm")))
            notes = []
            if p.get("raw_feed") is not None or p.get("raw_speed") is not None:
                notes.append(translate("CAM_FeedsSpeeds", "raw fallback"))
            self.table.setItem(i, 5, QtGui.QTableWidgetItem(", ".join(notes)))

    def _selected_index(self) -> Optional[int]:
        rows = self.table.selectionModel().selectedRows()
        if not rows:
            return None
        return rows[0].row()

    def _tool_geometry(self) -> tuple:
        """Read (diameter_mm, flutes) from the ToolBit doc object."""
        diameter_mm = 0.0
        flutes = 0
        try:
            d = getattr(self._obj, "Diameter", None)
            if d is not None:
                diameter_mm = float(d.Value) if hasattr(d, "Value") else float(d)
        except (AttributeError, TypeError, ValueError):
            diameter_mm = 0.0
        try:
            f = getattr(self._obj, "Flutes", None)
            flutes = int(f) if f is not None else 0
        except (TypeError, ValueError):
            flutes = 0
        return diameter_mm, flutes

    def _tool_chipload(self) -> Optional[float]:
        """Read the ToolBit's ``Chipload`` (mm/tooth), or None if unset or zero."""
        try:
            c = getattr(self._obj, "Chipload", None)
            if c is None:
                return None
            value = float(c.Value) if hasattr(c, "Value") else float(c)
            return value if value > 0 else None
        except (AttributeError, TypeError, ValueError):
            return None

    def _on_add(self) -> None:
        d, f = self._tool_geometry()
        dlg = _EditPresetDialog(
            None,
            diameter_mm=d,
            flutes=f,
            tool_chipload_mm=self._tool_chipload(),
            parent=self,
        )
        if dlg.exec_() != QtGui.QDialog.Accepted:
            return
        presets = self._presets()
        presets.append(dlg.build_preset())
        set_presets(self._obj, presets)
        self.refresh()

    def _on_edit(self) -> None:
        idx = self._selected_index()
        if idx is None:
            return
        presets = self._presets()
        d, f = self._tool_geometry()
        dlg = _EditPresetDialog(presets[idx], diameter_mm=d, flutes=f, parent=self)
        if dlg.exec_() != QtGui.QDialog.Accepted:
            return
        presets[idx] = dlg.build_preset()
        set_presets(self._obj, presets)
        self.refresh()

    def _on_delete(self) -> None:
        idx = self._selected_index()
        if idx is None:
            return
        presets = self._presets()
        del presets[idx]
        set_presets(self._obj, presets)
        self.refresh()
