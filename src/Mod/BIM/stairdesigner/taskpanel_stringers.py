# SPDX-License-Identifier: LGPL-2.1-or-later

"""Stringer task-panel controls and per-part overrides."""

from functools import partial

import FreeCAD
import FreeCADGui
from PySide import QtGui

from .object_utils import get_flights


translate = FreeCAD.Qt.translate

from .taskpanel_widgets import (
    _length_spin,
    _load_task_form,
    _value,
)

class StringerPanelMixin:
    """Task-panel methods grouped by responsibility."""

    def _make_stringer_panel(self):
        widget = _load_task_form(
            ":/ui/TaskStairDesignerStringers.ui",
            self,
            (
                "stringer_tree",
                "stringer_thickness",
                "stringer_custom_width",
                "stringer_width",
                "stringer_step_overlap",
                "stringer_start_extension",
                "stringer_end_extension",
                "stringer_position_label",
                "stringer_position_editor",
                "stringer_nosing_direction",
                "stringer_nosing_offset",
            ),
        )
        widget.setWindowIcon(QtGui.QIcon(":/icons/Arch_Stringer.svg"))
        self.stringer_tree.setToolTip(
            translate(
                "BIM",
                "Sets the stringer type independently for each flight side",
            )
        )
        self.stringer_tree.header().setSectionResizeMode(
            0, QtGui.QHeaderView.ResizeToContents
        )
        self.stringer_tree.header().setSectionResizeMode(
            1, QtGui.QHeaderView.Stretch
        )
        self._populate_stringer_tree()

        self.stringer_thickness.setValue(_value(self.stair.StringerThickness))
        self.stringer_custom_width.setChecked(
            self.stair.StringerCustomWidth
        )
        self.stringer_width.setValue(_value(self.stair.StringerWidth))
        self.stringer_width.setReadOnly(
            not self.stringer_custom_width.isChecked()
        )
        self.stringer_step_overlap.setValue(
            _value(self.stair.StringerStepOverlap)
        )
        self.stringer_start_extension.setValue(
            _value(self.stair.StringerStartExtension)
        )
        self.stringer_end_extension.setValue(
            _value(self.stair.StringerEndExtension)
        )
        self.stringer_nosing_direction.addItem(
            translate("BIM", "Perpendicular"), "Perpendicular"
        )
        self.stringer_nosing_direction.addItem(
            translate("BIM", "Vertical"), "Vertical"
        )
        self._select_data(
            self.stringer_nosing_direction,
            str(self.stair.StringerNosingOffsetDirection),
        )
        self.stringer_nosing_offset.setValue(
            _value(self.stair.StringerNosingOffset)
        )
        stringer_help = {
            self.stringer_thickness: translate(
                "BIM", "Board thickness across the stair side"
            ),
            self.stringer_width: translate(
                "BIM",
                "Board width, calculated automatically from the stair geometry unless Custom width is enabled",
            ),
            self.stringer_step_overlap: translate(
                "BIM", "Distance that the tread enters or covers the stringer"
            ),
            self.stringer_start_extension: translate(
                "BIM",
                "Extends only the first flight's stringers beyond the first tread",
            ),
            self.stringer_end_extension: translate(
                "BIM",
                "Extends only the last flight's stringers beyond the last tread",
            ),
            self.stringer_position_editor: translate(
                "BIM",
                "Clear distance from the tread nosing line to the stringer top, measured vertically or perpendicular to the stair pitch",
            ),
        }
        for editor, help_text in stringer_help.items():
            editor.setToolTip(help_text)
            label = widget.ui.stringerForm.labelForField(editor)
            if label is not None:
                label.setToolTip(help_text)
        self.stringer_custom_width.setToolTip(
            translate(
                "BIM",
                "Uses a fixed board width instead of the automatic width",
            )
        )
        self.stringer_nosing_direction.setToolTip(
            self.stringer_position_editor.toolTip()
        )
        self.stringer_nosing_offset.setToolTip(
            self.stringer_position_editor.toolTip()
        )
        widget.ui.stringer_override_layout.addWidget(
            self._make_stringer_override_widget()
        )

        for editor in (
            self.stringer_thickness,
            self.stringer_step_overlap,
            self.stringer_start_extension,
            self.stringer_end_extension,
            self.stringer_nosing_offset,
        ):
            editor.valueChanged.connect(self._apply)
        self.stringer_width.valueChanged.connect(self._apply)
        self.stringer_custom_width.toggled.connect(
            self._stringer_custom_width_changed
        )
        self.stringer_nosing_direction.currentIndexChanged.connect(
            self._apply
        )
        return widget

    def _make_stringer_override_widget(self):
        group = QtGui.QGroupBox(
            translate("BIM", "Selected Stringer Overrides")
        )
        layout = QtGui.QVBoxLayout(group)
        self.stringer_override_name = QtGui.QLabel()
        layout.addWidget(self.stringer_override_name)
        form = QtGui.QFormLayout()

        self.override_thickness = QtGui.QCheckBox(
            translate("BIM", "Override thickness")
        )
        self.override_thickness_value = _length_spin(0.0, 0.01)
        form.addRow(
            self.override_thickness, self.override_thickness_value
        )

        self.override_width = QtGui.QCheckBox(
            translate("BIM", "Override width")
        )
        self.override_width_value = _length_spin(0.0, 0.01)
        form.addRow(self.override_width, self.override_width_value)

        self.override_step_overlap = QtGui.QCheckBox(
            translate("BIM", "Override step overlap")
        )
        self.override_step_overlap_value = _length_spin(
            0.0, -1000000.0
        )
        form.addRow(
            self.override_step_overlap,
            self.override_step_overlap_value,
        )

        self.override_nosing_position = QtGui.QCheckBox(
            translate("BIM", "Override position above nosing")
        )
        position = QtGui.QWidget()
        position_layout = QtGui.QHBoxLayout(position)
        position_layout.setContentsMargins(0, 0, 0, 0)
        self.override_nosing_direction = QtGui.QComboBox()
        self.override_nosing_direction.addItem(
            translate("BIM", "Perpendicular"), "Perpendicular"
        )
        self.override_nosing_direction.addItem(
            translate("BIM", "Vertical"), "Vertical"
        )
        self.override_nosing_offset = _length_spin(0.0)
        position_layout.addWidget(self.override_nosing_direction)
        position_layout.addWidget(self.override_nosing_offset)
        self.override_nosing_position_editor = position
        form.addRow(self.override_nosing_position, position)
        layout.addLayout(form)

        for checkbox in (
            self.override_thickness,
            self.override_width,
            self.override_step_overlap,
            self.override_nosing_position,
        ):
            checkbox.toggled.connect(self._apply_stringer_override)
        for editor in (
            self.override_thickness_value,
            self.override_width_value,
            self.override_step_overlap_value,
            self.override_nosing_offset,
        ):
            editor.valueChanged.connect(self._apply_stringer_override)
        self.override_nosing_direction.currentIndexChanged.connect(
            self._apply_stringer_override
        )
        self.stringer_override_widget = group
        group.hide()
        return group

    def _populate_stringer_tree(self):
        self.stringer_tree.clear()
        self.stringer_flight_editors = []
        self.stringer_all_editors = {}
        flights = get_flights(self.stair)
        if len(flights) > 1:
            root = QtGui.QTreeWidgetItem(self.stringer_tree)
            root.setText(0, translate("BIM", "All"))
            root.setFirstColumnSpanned(True)
            for side, label in (
                ("Left", translate("BIM", "Left side")),
                ("Right", translate("BIM", "Right side")),
            ):
                values = {
                    str(getattr(flight, f"{side}StringerType"))
                    for flight in flights
                }
                editor = self._make_stringer_type_editor(
                    values.pop() if len(values) == 1 else None
                )
                child = QtGui.QTreeWidgetItem(root)
                child.setText(0, label)
                self.stringer_tree.setItemWidget(child, 1, editor)
                self.stringer_all_editors[side] = editor
                editor.currentIndexChanged.connect(
                    partial(self._all_stringer_type_changed, side, editor)
                )
            root.setExpanded(True)

        for flight in flights:
            root = QtGui.QTreeWidgetItem(self.stringer_tree)
            root.setText(0, flight.Label)
            root.setFirstColumnSpanned(True)
            record = {"flight": flight, "item": root}
            for side, label in (
                ("Left", translate("BIM", "Left side")),
                ("Right", translate("BIM", "Right side")),
            ):
                editor = self._make_stringer_type_editor(
                    str(getattr(flight, f"{side}StringerType"))
                )
                child = QtGui.QTreeWidgetItem(root)
                child.setText(0, label)
                self.stringer_tree.setItemWidget(child, 1, editor)
                record[f"{side.lower()}_type"] = editor
                editor.currentIndexChanged.connect(
                    self._stringer_type_changed
                )
            root.setExpanded(True)
            self.stringer_flight_editors.append(record)

    def _make_stringer_type_editor(self, value=None):
        editor = QtGui.QComboBox()
        if value is None:
            editor.addItem(translate("BIM", "Mixed"), "__mixed__")
            mixed_item = editor.model().item(0)
            if mixed_item is not None:
                mixed_item.setEnabled(False)
        editor.addItem(translate("BIM", "None"), "None")
        editor.addItem(
            translate("BIM", "Housed stringer"),
            "Housed stringer",
        )
        editor.addItem(
            translate("BIM", "Notched stringer"),
            "Notched stringer",
        )
        if value is not None:
            self._select_data(editor, value)
        return editor

    def _all_stringer_type_changed(self, side, editor, *args):
        if self._loading:
            return
        value = str(editor.itemData(editor.currentIndex()))
        if value == "__mixed__":
            return
        for record in self.stringer_flight_editors:
            flight_editor = record[f"{side.lower()}_type"]
            blocked = flight_editor.blockSignals(True)
            self._select_data(flight_editor, value)
            flight_editor.blockSignals(blocked)
        self._stringer_type_changed()

    def _refresh_all_stringer_editors(self):
        for side, editor in self.stringer_all_editors.items():
            values = {
                str(
                    record[f"{side.lower()}_type"].itemData(
                        record[f"{side.lower()}_type"].currentIndex()
                    )
                )
                for record in self.stringer_flight_editors
            }
            blocked = editor.blockSignals(True)
            mixed_index = next(
                (
                    index
                    for index in range(editor.count())
                    if str(editor.itemData(index)) == "__mixed__"
                ),
                -1,
            )
            if len(values) == 1:
                if mixed_index >= 0:
                    editor.removeItem(mixed_index)
                self._select_data(editor, values.pop())
            else:
                if mixed_index < 0:
                    editor.insertItem(
                        0, translate("BIM", "Mixed"), "__mixed__"
                    )
                    mixed_item = editor.model().item(0)
                    if mixed_item is not None:
                        mixed_item.setEnabled(False)
                    mixed_index = 0
                editor.setCurrentIndex(mixed_index)
            editor.blockSignals(blocked)

    def _stringer_type_changed(self, *args):
        self._refresh_all_stringer_editors()
        self._update_stringer_editor_visibility()
        self._apply()

    def _update_stringer_editor_visibility(self):
        types = {
            str(editor.itemData(editor.currentIndex()))
            for record in self.stringer_flight_editors
            for editor in (
                record["left_type"],
                record["right_type"],
            )
        }
        visible = "Housed stringer" in types
        self.stringer_position_label.setVisible(visible)
        self.stringer_position_editor.setVisible(visible)

    def _stringer_custom_width_changed(self, checked):
        self.stringer_width.setReadOnly(not checked)
        self._apply()

    def _apply_stringer_override(self, *args):
        if (
            self._loading
            or self._loading_override
            or self.selected_stringer is None
        ):
            return
        part = self.selected_stringer
        try:
            part.OverrideThickness = self.override_thickness.isChecked()
            part.Thickness = self.override_thickness_value.value()
            part.OverrideWidth = self.override_width.isChecked()
            part.Width = self.override_width_value.value()
            part.OverrideStepOverlap = (
                self.override_step_overlap.isChecked()
            )
            part.StepOverlap = self.override_step_overlap_value.value()
            part.OverrideNosingPosition = (
                self.override_nosing_position.isChecked()
            )
            part.NosingOffsetDirection = str(
                self.override_nosing_direction.itemData(
                    self.override_nosing_direction.currentIndex()
                )
            )
            part.NosingOffset = self.override_nosing_offset.value()
        except ReferenceError:
            self._update_stringer_selection()
            return
        self.stair.Proxy.rebuild(
            self.stair, allow_structure_changes=True
        )
        self.stair.Document.recompute()
        self._update_stringer_selection()

    def _update_stringer_selection(self):
        if "stringers" not in self.sections:
            return
        candidates = []
        for candidate in FreeCADGui.Selection.getSelection():
            try:
                role = str(
                    getattr(candidate, "StairDesignerRole", "")
                )
                if (
                    getattr(candidate, "GeneratedBy", "")
                    == self.stair.Name
                    and role in {"LeftStringer", "RightStringer"}
                ):
                    candidates.append(candidate)
            except ReferenceError:
                continue
        selected = candidates[0] if len(candidates) == 1 else None
        self.selected_stringer = selected
        self.stringer_override_widget.setVisible(selected is not None)
        if selected is None:
            return

        self._loading_override = True
        try:
            self.stringer_override_name.setText(selected.Label)
            self.override_thickness.setChecked(
                selected.OverrideThickness
            )
            self.override_thickness_value.setValue(
                _value(selected.Thickness)
            )
            self.override_width.setChecked(selected.OverrideWidth)
            self.override_width_value.setValue(_value(selected.Width))
            self.override_step_overlap.setChecked(
                selected.OverrideStepOverlap
            )
            self.override_step_overlap_value.setValue(
                _value(selected.StepOverlap)
            )
            self.override_nosing_position.setChecked(
                selected.OverrideNosingPosition
            )
            self._select_data(
                self.override_nosing_direction,
                str(selected.NosingOffsetDirection),
            )
            self.override_nosing_offset.setValue(
                _value(selected.NosingOffset)
            )
            housed = str(selected.StringerType) == "Housed stringer"
            self.override_nosing_position.setVisible(housed)
            self.override_nosing_position_editor.setVisible(housed)
            self._update_override_editor_states()
        except ReferenceError:
            self.selected_stringer = None
            self.stringer_override_widget.hide()
        finally:
            self._loading_override = False

    def _update_override_editor_states(self):
        self.override_thickness_value.setEnabled(
            self.override_thickness.isChecked()
        )
        self.override_width_value.setEnabled(
            self.override_width.isChecked()
        )
        self.override_step_overlap_value.setEnabled(
            self.override_step_overlap.isChecked()
        )
        enabled = self.override_nosing_position.isChecked()
        self.override_nosing_direction.setEnabled(enabled)
        self.override_nosing_offset.setEnabled(enabled)
