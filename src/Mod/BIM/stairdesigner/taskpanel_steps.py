# SPDX-License-Identifier: LGPL-2.1-or-later

"""Stair, step, riser, and selected-component task-panel sections."""

import FreeCAD
import FreeCADGui
from PySide import QtGui

from .geometry_core import BLONDEL_MAXIMUM, BLONDEL_MINIMUM
from .object_components import _set_tread_properties

translate = FreeCAD.Qt.translate

from .taskpanel_widgets import (
    _length_spin,
    _load_task_form,
    _value,
)


class StepPanelMixin:
    """Task-panel methods grouped by responsibility."""

    def _make_stair_panel(self):
        widget = _load_task_form(
            ":/ui/TaskStairDesignerStair.ui",
            self,
            (
                "stair_type",
                "floor_height",
                "end_with_riser",
                "step_count",
                "concrete_thickness",
                "concrete_thickness_label",
                "cut_distance_label",
                "bottom_cut_distance",
                "bottom_cut_distance_label",
                "top_cut_distance",
                "top_cut_distance_label",
                "riser_height",
                "tread_width",
                "blondel_label",
                "blondel_value",
                "snap_position",
            ),
        )
        widget.setWindowIcon(QtGui.QIcon(":/icons/Arch_Stairs.svg"))
        self.stair_type.addItem(translate("BIM", "Wood"), "Wood")
        self.stair_type.addItem(translate("BIM", "Concrete"), "Concrete")
        self._select_data(self.stair_type, str(self.stair.StairType))
        self.floor_height.setValue(_value(self.stair.FloorHeight))
        self.end_with_riser.setChecked(self.stair.EndWithRiser)
        self.step_count.setValue(self.stair.NumberOfSteps)
        self.concrete_thickness.setValue(_value(self.stair.ConcreteThickness))
        self.bottom_cut_distance.setValue(_value(self.stair.BottomCutDistance))
        self.top_cut_distance.setValue(_value(self.stair.TopCutDistance))
        widget.ui.flight_tree_layout.addWidget(self._make_multiflight_panel())

        self._connect_stair_controls()
        self.snap_position.clicked.connect(self._snap_position)
        return widget

    def _make_step_panel(self):
        widget = _load_task_form(
            ":/ui/TaskStairDesignerSteps.ui",
            self,
            (
                "steps_enabled",
                "step_thickness",
                "nosing",
                "structure_width_offset",
                "structure_width_offset_label",
                "risers_group",
                "riser_thickness",
                "priority_to_riser",
                "step_riser_overlap",
                "step_riser_overlap_label",
                "riser_upper_offset",
                "riser_lower_offset",
            ),
        )
        widget.setWindowIcon(QtGui.QIcon(":/icons/Arch_Stairs.svg"))
        self.steps_enabled.setChecked(self.stair.StepsEnabled)
        self.step_thickness.setValue(_value(self.stair.StepThickness))
        self.nosing.setValue(_value(self.stair.Nosing))
        self.structure_width_offset.setValue(_value(self.stair.StructureWidthOffset))
        widget.ui.selected_step_layout.addWidget(self._make_selected_step_widget())
        self.risers_group.setChecked(self.stair.RisersEnabled)
        self.riser_thickness.setValue(_value(self.stair.RiserThickness))
        self.priority_to_riser.setChecked(self.stair.PriorityToRiser)
        self.step_riser_overlap.setValue(_value(self.stair.StepRiserOverlap))
        self._update_overlap_label()
        self.riser_upper_offset.setValue(_value(self.stair.RiserUpperOffset))
        self.riser_lower_offset.setValue(_value(self.stair.RiserLowerOffset))
        self._connect_step_controls()
        return widget

    def _make_selected_step_widget(self):
        group = QtGui.QGroupBox(translate("BIM", "Selected Step / Riser"))
        layout = QtGui.QVBoxLayout(group)
        self.selected_step_name = QtGui.QLabel()
        layout.addWidget(self.selected_step_name)
        form = QtGui.QFormLayout()
        self.selected_step_extra_width = _length_spin(0.0, -1000000.0, 1000000.0)
        self.selected_step_extra_width.setToolTip(
            translate(
                "BIM",
                "Adjusts this tread's usable going; positive values widen it "
                "and negative values shorten it while the general going is "
                "redistributed to keep the same total stair length",
            )
        )
        form.addRow(
            translate("BIM", "Extra width"),
            self.selected_step_extra_width,
        )
        self.selected_step_extra_height = _length_spin(0.0, -1000000.0, 1000000.0)
        self.selected_step_extra_height.setToolTip(
            translate(
                "BIM",
                "Adjusts the rise below this tread; positive values raise it "
                "and negative values lower it while the general riser height "
                "is redistributed to keep the same floor height",
            )
        )
        form.addRow(
            translate("BIM", "Extra height"),
            self.selected_step_extra_height,
        )
        layout.addLayout(form)
        self.selected_step_extra_width.valueChanged.connect(self._apply_selected_step)
        self.selected_step_extra_height.valueChanged.connect(self._apply_selected_step)
        self.selected_step_widget = group
        group.hide()
        return group

    def _connect_stair_controls(self):
        for spin in (
            self.floor_height,
            self.step_count,
            self.concrete_thickness,
            self.bottom_cut_distance,
            self.top_cut_distance,
        ):
            spin.valueChanged.connect(self._apply)
        self.stair_type.currentIndexChanged.connect(self._apply)
        self.end_with_riser.toggled.connect(self._apply)

    def _connect_step_controls(self):
        for spin in (
            self.step_thickness,
            self.nosing,
            self.structure_width_offset,
            self.riser_thickness,
            self.step_riser_overlap,
            self.riser_upper_offset,
            self.riser_lower_offset,
        ):
            spin.valueChanged.connect(self._apply)
        self.risers_group.toggled.connect(self._apply)
        self.steps_enabled.toggled.connect(self._apply)
        self.priority_to_riser.toggled.connect(self._priority_changed)

    def _apply_selected_step(self, *args):
        if self._loading or self._loading_override or self.selected_step is None:
            return
        try:
            self.selected_step.ExtraWidth = self.selected_step_extra_width.value()
            self.selected_step.ExtraHeight = self.selected_step_extra_height.value()
        except ReferenceError:
            self._update_step_selection()
            return
        self.stair.Proxy.rebuild(self.stair, allow_structure_changes=True)
        self.stair.Document.recompute()
        self._update_step_selection()

    def _update_step_selection(self):
        if "steps" not in self.sections:
            return
        if str(self.stair.StairType) != "Wood":
            self.selected_step_component = None
            self.selected_step = None
            self.selected_step_widget.hide()
            return
        candidates = []
        for candidate in FreeCADGui.Selection.getSelection():
            try:
                if getattr(candidate, "GeneratedBy", "") == self.stair.Name and str(
                    getattr(candidate, "StairDesignerRole", "")
                ) in ("Tread", "Riser"):
                    candidates.append(candidate)
            except ReferenceError:
                continue
        component = candidates[0] if len(candidates) == 1 else None
        selected = None
        if component is not None:
            component_index = int(getattr(component, "Index", 0))
            selected = next(
                (
                    child
                    for child in self.stair.StepsGroup.Group
                    if getattr(child, "GeneratedBy", "") == self.stair.Name
                    and str(getattr(child, "StairDesignerRole", "")) == "Tread"
                    and int(getattr(child, "Index", 0)) == component_index
                ),
                None,
            )
        self.selected_step_component = component
        self.selected_step = selected
        self.selected_step_widget.setVisible(selected is not None)
        if selected is None:
            return

        self._loading_override = True
        try:
            if any(name not in selected.PropertiesList for name in ("ExtraWidth", "ExtraHeight")):
                _set_tread_properties(selected)
            self.selected_step_name.setText(
                translate("BIM", "Step {0} / Riser {0}").format(selected.Index)
            )
            self.selected_step_extra_width.setValue(_value(selected.ExtraWidth))
            self.selected_step_extra_height.setValue(_value(selected.ExtraHeight))
        except ReferenceError:
            self.selected_step_component = None
            self.selected_step = None
            self.selected_step_widget.hide()
        finally:
            self._loading_override = False

    def _priority_changed(self, *args):
        self._update_overlap_label()
        self._apply(*args)

    def _update_overlap_label(self):
        if self.priority_to_riser.isChecked():
            text = translate("BIM", "Step penetration")
        else:
            text = translate("BIM", "Step rear overlap")
        self.step_riser_overlap_label.setText(text)

    def _refresh_diagnostics(self):
        self.riser_height.setValue(_value(self.stair.RiserHeight))
        self.tread_width.setValue(_value(self.stair.TreadWidth))
        self.blondel_value.setValue(_value(self.stair.BlondelValue))
        compliant = bool(self.stair.BlondelCompliant)
        warning = translate(
            "BIM",
            "Blondel law is outside " f"{BLONDEL_MINIMUM:.0f}-{BLONDEL_MAXIMUM:.0f} mm.",
        )
        if compliant:
            self.blondel_label.setText(translate("BIM", "Stair rule"))
            self.blondel_label.setStyleSheet("")
            self.blondel_label.setToolTip("")
            self.blondel_value.setToolTip("")
        else:
            self.blondel_label.setText(translate("BIM", "\u26a0 Stair rule"))
            self.blondel_label.setStyleSheet("color: #b71c1c; font-weight: bold;")
            self.blondel_label.setToolTip(warning)
            self.blondel_value.setToolTip(warning)
