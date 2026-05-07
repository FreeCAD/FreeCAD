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
from PySide import QtCore, QtGui

from ...FeedsSpeeds import (
    OP_TYPES,
    derive_preset_label,
    get_presets,
    make_preset,
    set_presets,
)

translate = FreeCAD.Qt.translate


def _fmt(value: Optional[float], unit: str = "") -> str:
    if value is None:
        return ""
    if unit:
        return f"{value:g} {unit}"
    return f"{value:g}"


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


class _EditPresetDialog(QtGui.QDialog):
    """Sub-dialog for adding or editing a single preset row.

    The dialog accepts the tool's current ``diameter_mm`` and ``flutes`` so
    it can keep the engineering inputs (surface speed, chipload) and the
    direct inputs (spindle rpm, horiz feed mm/min) in sync as the user
    types. Storage is engineering-only: only surface_speed, chipload and
    vert_feed_ratio are persisted; the rpm/feed spinboxes are display
    affordances derived from those plus the tool's geometry.
    """

    def __init__(
        self,
        preset: Optional[dict],
        diameter_mm: float = 0.0,
        flutes: Optional[int] = None,
        parent=None,
    ):
        super().__init__(parent)
        self.setWindowTitle(translate("CAM_FeedsSpeeds", "Edit preset"))
        layout = QtGui.QFormLayout(self)

        self._initial = preset or make_preset()
        self._material_uuid: Optional[str] = None
        self._material_name: Optional[str] = None
        self._diameter_mm = float(diameter_mm) if diameter_mm else 0.0
        self._flutes = int(flutes) if flutes else 0
        self._syncing = False  # guards against feedback loops

        # Preset name — user-given label, optional but encouraged.
        self.name_edit = QtGui.QLineEdit()
        self.name_edit.setPlaceholderText(
            translate("CAM_FeedsSpeeds", "Optional, e.g. 'Aluminum aggressive'")
        )
        layout.addRow(translate("CAM_FeedsSpeeds", "Name:"), self.name_edit)

        # Material row: read-only label, Browse button opens the
        # Machinability-filtered MaterialDialog. UUID is the authoritative
        # identifier; name is for display + as a fallback when the UUID
        # can't be resolved on a different machine.
        mat_row = QtGui.QHBoxLayout()
        self.material_label = QtGui.QLabel(translate("CAM_FeedsSpeeds", "(none)"))
        mat_row.addWidget(self.material_label, 1)
        self.browse_button = QtGui.QPushButton(translate("CAM_FeedsSpeeds", "Browse…"))
        self.browse_button.clicked.connect(self._on_browse)
        mat_row.addWidget(self.browse_button)
        self.generic_check = QtGui.QCheckBox(translate("CAM_FeedsSpeeds", "Generic (any material)"))
        self.generic_check.toggled.connect(self._on_generic_toggled)
        mat_row.addWidget(self.generic_check)
        layout.addRow(translate("CAM_FeedsSpeeds", "Material:"), mat_row)

        # Op type
        self.op_type_combo = QtGui.QComboBox()
        self.op_type_combo.addItem(translate("CAM_FeedsSpeeds", "(any)"), None)
        for op in OP_TYPES:
            self.op_type_combo.addItem(op, op)
        layout.addRow(translate("CAM_FeedsSpeeds", "Op type:"), self.op_type_combo)

        # Engineering values
        self.surface_speed_spin = QtGui.QDoubleSpinBox()
        self.surface_speed_spin.setRange(0, 100000)
        self.surface_speed_spin.setDecimals(2)
        self.surface_speed_spin.setSuffix(" m/min")
        layout.addRow(translate("CAM_FeedsSpeeds", "Surface speed:"), self.surface_speed_spin)

        self.chipload_spin = QtGui.QDoubleSpinBox()
        self.chipload_spin.setRange(0, 10)
        self.chipload_spin.setDecimals(4)
        self.chipload_spin.setSuffix(" mm/tooth")
        layout.addRow(translate("CAM_FeedsSpeeds", "Chipload:"), self.chipload_spin)

        self.vert_ratio_spin = QtGui.QDoubleSpinBox()
        self.vert_ratio_spin.setRange(0, 1)
        self.vert_ratio_spin.setDecimals(3)
        self.vert_ratio_spin.setValue(0.33)
        layout.addRow(translate("CAM_FeedsSpeeds", "Vert feed ratio:"), self.vert_ratio_spin)

        # Raw values: entered directly as feed/speed. When tool geometry
        # is known, these stay in sync with surface_speed/chipload above
        # (editing either side updates the other).
        raw_box = QtGui.QGroupBox(translate("CAM_FeedsSpeeds", "Direct feed and speed"))
        raw_form = QtGui.QFormLayout(raw_box)
        self.raw_feed_spin = QtGui.QDoubleSpinBox()
        self.raw_feed_spin.setRange(0, 100000)
        self.raw_feed_spin.setSuffix(" mm/min")
        raw_form.addRow(translate("CAM_FeedsSpeeds", "Horiz feed:"), self.raw_feed_spin)
        self.raw_speed_spin = QtGui.QDoubleSpinBox()
        self.raw_speed_spin.setRange(0, 200000)
        self.raw_speed_spin.setSuffix(" rpm")
        raw_form.addRow(translate("CAM_FeedsSpeeds", "Spindle speed:"), self.raw_speed_spin)
        layout.addRow(raw_box)

        # Wire up bidirectional sync. Each handler reads geometry; when
        # geometry is missing, that direction silently no-ops.
        self.surface_speed_spin.valueChanged.connect(self._on_surface_speed_changed)
        self.chipload_spin.valueChanged.connect(self._on_chipload_changed)
        self.raw_speed_spin.valueChanged.connect(self._on_raw_speed_changed)
        self.raw_feed_spin.valueChanged.connect(self._on_raw_feed_changed)

        # Inform the user when sync is unavailable.
        if self._diameter_mm <= 0 or self._flutes <= 0:
            hint = QtGui.QLabel(
                translate(
                    "CAM_FeedsSpeeds",
                    "Tool diameter and/or flute count missing — surface speed "
                    "and chipload won't auto-sync with direct feed/speed.",
                )
            )
            hint.setWordWrap(True)
            hint.setStyleSheet("color: #888;")
            layout.addRow(hint)

        # Buttons
        button_box = QtGui.QDialogButtonBox(
            QtGui.QDialogButtonBox.Ok | QtGui.QDialogButtonBox.Cancel
        )
        button_box.accepted.connect(self.accept)
        button_box.rejected.connect(self.reject)
        layout.addRow(button_box)

        self._populate_from_preset(self._initial)

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
            if ss is not None:
                self.surface_speed_spin.setValue(ss)
            if cl is not None:
                self.chipload_spin.setValue(cl)
            self.vert_ratio_spin.setValue(preset.get("vert_feed_ratio", 0.33))

            # Compute raw display values from engineering + geometry.
            rpm = _rpm_from_surface_speed(ss, self._diameter_mm) if ss is not None else None
            if rpm is not None:
                self.raw_speed_spin.setValue(rpm)
                if cl is not None and self._flutes > 0:
                    rf = _feed_from_chipload(cl, rpm, self._flutes)
                    if rf is not None:
                        self.raw_feed_spin.setValue(rf)
        finally:
            self._syncing = False

    # ------------------------------------------------------------------
    # Bidirectional sync between engineering values (surface_speed, chipload)
    # and direct values (raw_speed, raw_feed). Each handler returns early
    # when self._syncing is set, breaking feedback loops.
    # ------------------------------------------------------------------

    def _set_silently(self, spinbox, value: float) -> None:
        spinbox.blockSignals(True)
        try:
            spinbox.setValue(value)
        finally:
            spinbox.blockSignals(False)

    def _on_surface_speed_changed(self, ss: float) -> None:
        if self._syncing:
            return
        self._syncing = True
        try:
            rpm = _rpm_from_surface_speed(ss, self._diameter_mm)
            if rpm is not None:
                self._set_silently(self.raw_speed_spin, rpm)
                cl = self.chipload_spin.value()
                if cl > 0 and self._flutes > 0:
                    rf = _feed_from_chipload(cl, rpm, self._flutes)
                    if rf is not None:
                        self._set_silently(self.raw_feed_spin, rf)
        finally:
            self._syncing = False

    def _on_chipload_changed(self, cl: float) -> None:
        if self._syncing:
            return
        self._syncing = True
        try:
            rpm = self.raw_speed_spin.value()
            rf = _feed_from_chipload(cl, rpm, self._flutes)
            if rf is not None:
                self._set_silently(self.raw_feed_spin, rf)
        finally:
            self._syncing = False

    def _on_raw_speed_changed(self, rpm: float) -> None:
        if self._syncing:
            return
        self._syncing = True
        try:
            ss = _surface_speed_from_rpm(rpm, self._diameter_mm)
            if ss is not None:
                self._set_silently(self.surface_speed_spin, ss)
            rf = self.raw_feed_spin.value()
            cl = _chipload_from_feed(rf, rpm, self._flutes)
            if cl is not None:
                self._set_silently(self.chipload_spin, cl)
        finally:
            self._syncing = False

    def _on_raw_feed_changed(self, rf: float) -> None:
        if self._syncing:
            return
        self._syncing = True
        try:
            rpm = self.raw_speed_spin.value()
            cl = _chipload_from_feed(rf, rpm, self._flutes)
            if cl is not None:
                self._set_silently(self.chipload_spin, cl)
        finally:
            self._syncing = False

    def _on_generic_toggled(self, generic: bool) -> None:
        self.material_label.setEnabled(not generic)
        self.browse_button.setEnabled(not generic)

    def _on_browse(self) -> None:
        from Path.Tool.Gui.MaterialPicker import pick_machinability_material

        result = pick_machinability_material(self)
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
            surface_speed=self.surface_speed_spin.value() or None,
            chipload=self.chipload_spin.value() or None,
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
            self.table.setItem(i, 3, QtGui.QTableWidgetItem(_fmt(p.get("surface_speed"))))
            self.table.setItem(i, 4, QtGui.QTableWidgetItem(_fmt(p.get("chipload"))))
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

    def _on_add(self) -> None:
        d, f = self._tool_geometry()
        dlg = _EditPresetDialog(None, diameter_mm=d, flutes=f, parent=self)
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
