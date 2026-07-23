# SPDX-License-Identifier: LGPL-2.1-or-later

"""Task panels for the BIM Stair Designer."""

import math
from functools import partial

import FreeCAD
import FreeCADGui
from PySide import QtCore, QtGui

from stairdesigner import objects as stair_objects
from stairdesigner.geometry import BLONDEL_MAXIMUM, BLONDEL_MINIMUM


translate = FreeCAD.Qt.translate


def _value(quantity):
    return float(quantity.Value) if hasattr(quantity, "Value") else float(quantity)


def _length_spin(value=0.0, minimum=0.0, maximum=1000000.0):
    spin = QtGui.QDoubleSpinBox()
    spin.setDecimals(2)
    spin.setRange(minimum, maximum)
    spin.setSuffix(" mm")
    spin.setValue(value)
    return spin


def _float_spin(value=0.0, minimum=0.0, maximum=1000.0, decimals=3):
    spin = QtGui.QDoubleSpinBox()
    spin.setDecimals(decimals)
    spin.setRange(minimum, maximum)
    spin.setValue(value)
    return spin


def _percent_spin(value=50):
    spin = QtGui.QSpinBox()
    spin.setRange(0, 100)
    spin.setSuffix(" %")
    spin.setValue(int(round(_value(value))))
    return spin


class _FlightTreeWidget(QtGui.QTreeWidget):
    """Flight tree that owns and consumes its Delete-key action."""

    def __init__(self, delete_callback, parent=None):
        super().__init__(parent)
        self._delete_callback = delete_callback

    def keyPressEvent(self, event):
        if event.key() == QtCore.Qt.Key_Delete:
            self._delete_callback()
            event.accept()
            return
        super().keyPressEvent(event)


class StairDesignerTaskPanel:
    """Edit one or more Stair Designer parameter sections."""

    def __init__(self, stair, sections=None, edit_object=None, is_creating=False):
        self.stair = stair
        self.edit_object = edit_object or stair
        self.is_creating = is_creating
        requested_sections = sections or (
            "stairs",
            "steps",
            "stringers",
            "handrails",
        )
        self.sections = tuple(
            section
            for section in requested_sections
            if section in {"stairs", "steps", "stringers", "handrails"}
        )
        self._loading = True
        self.form = []
        self.section_widgets = {}
        self._component_sections_hidden = False
        self._collapsed_sections = set()
        self._loading_override = False
        self._selection_observer_registered = False
        self.selected_stringer = None
        self.flight = self._first_flight()

        for section in self.sections:
            if section == "stairs":
                panel = self._make_stair_panel()
            elif section == "steps":
                panel = self._make_step_panel()
            elif section == "stringers":
                panel = self._make_stringer_panel()
            elif section == "handrails":
                panel = self._make_handrail_panel()
            else:
                continue
            self.section_widgets[section] = panel
            self.form.append(panel)

        self._loading = False
        if {"stairs", "steps", "stringers", "handrails"}.intersection(
            self.sections
        ):
            self._update_type_visibility()
            QtCore.QTimer.singleShot(0, self._configure_task_boxes)
        if "stairs" in self.sections:
            self._refresh_diagnostics()
        if "stringers" in self.sections and FreeCAD.GuiUp:
            FreeCADGui.Selection.addObserver(self)
            self._selection_observer_registered = True
            QtCore.QTimer.singleShot(0, self._update_stringer_selection)

    def _make_stair_panel(self):
        widget = QtGui.QWidget()
        widget.setWindowTitle(translate("BIM", "Stair Parameters"))
        widget.setWindowIcon(QtGui.QIcon(":/icons/Arch_Stairs.svg"))
        layout = QtGui.QVBoxLayout(widget)

        dimension_form = QtGui.QFormLayout()
        self.stair_type = QtGui.QComboBox()
        self.stair_type.addItem(translate("BIM", "Wood"), "Wood")
        self.stair_type.addItem(translate("BIM", "Concrete"), "Concrete")
        self._select_data(self.stair_type, str(self.stair.StairType))
        dimension_form.addRow(translate("BIM", "Stair type"), self.stair_type)

        self.floor_height = _length_spin(_value(self.stair.FloorHeight), 1.0)
        dimension_form.addRow(translate("BIM", "Floor height"), self.floor_height)

        self.step_count = QtGui.QSpinBox()
        self.step_count.setRange(2, 1000)
        self.step_count.setValue(self.stair.NumberOfSteps)
        dimension_form.addRow(translate("BIM", "Number of steps"), self.step_count)

        self.concrete_thickness = _length_spin(_value(self.stair.ConcreteThickness), 0.0)
        self.concrete_thickness_label = QtGui.QLabel(translate("BIM", "Stair thickness"))
        dimension_form.addRow(self.concrete_thickness_label, self.concrete_thickness)
        layout.addLayout(dimension_form)

        layout.addWidget(self._make_multiflight_panel())

        design_form = QtGui.QFormLayout()
        self.riser_height = self._readonly_length_spin()
        self.tread_width = self._readonly_length_spin()
        self.blondel_value = self._readonly_length_spin()
        design_form.addRow(translate("BIM", "Riser height"), self.riser_height)
        design_form.addRow(translate("BIM", "Tread width"), self.tread_width)
        self.blondel_label = QtGui.QLabel(translate("BIM", "Stair rule"))
        design_form.addRow(self.blondel_label, self.blondel_value)
        layout.addLayout(design_form)
        layout.addStretch()

        self._connect_stair_controls()
        return widget

    def _make_step_panel(self):
        widget = QtGui.QWidget()
        widget.setWindowTitle(translate("BIM", "Step Parameters"))
        widget.setWindowIcon(QtGui.QIcon(":/icons/Arch_Stairs.svg"))
        layout = QtGui.QVBoxLayout(widget)

        step_form = QtGui.QFormLayout()
        self.step_thickness = _length_spin(_value(self.stair.StepThickness), 0.01)
        self.nosing = _length_spin(_value(self.stair.Nosing))
        step_form.addRow(translate("BIM", "Thickness"), self.step_thickness)
        step_form.addRow(translate("BIM", "Nosing"), self.nosing)
        layout.addLayout(step_form)

        risers = QtGui.QGroupBox(translate("BIM", "Risers"))
        risers.setCheckable(True)
        risers.setChecked(self.stair.RisersEnabled)
        riser_form = QtGui.QFormLayout(risers)
        self.risers_group = risers
        self.riser_thickness = _length_spin(_value(self.stair.RiserThickness), 0.01)
        self.priority_to_riser = QtGui.QCheckBox()
        self.priority_to_riser.setChecked(self.stair.PriorityToRiser)
        self.step_riser_overlap = _length_spin(_value(self.stair.StepRiserOverlap))
        self.step_riser_overlap_label = QtGui.QLabel()
        self._update_overlap_label()
        self.riser_upper_offset = _length_spin(
            _value(self.stair.RiserUpperOffset), -1000000.0
        )
        self.riser_lower_offset = _length_spin(
            _value(self.stair.RiserLowerOffset), -1000000.0
        )
        riser_form.addRow(translate("BIM", "Thickness"), self.riser_thickness)
        riser_form.addRow(translate("BIM", "Priority to riser"), self.priority_to_riser)
        riser_form.addRow(self.step_riser_overlap_label, self.step_riser_overlap)
        riser_form.addRow(translate("BIM", "Upper offset"), self.riser_upper_offset)
        riser_form.addRow(translate("BIM", "Lower offset"), self.riser_lower_offset)
        layout.addWidget(risers)
        layout.addStretch()

        self._connect_step_controls()
        return widget

    def _make_stringer_panel(self):
        widget = QtGui.QWidget()
        widget.setWindowTitle(translate("BIM", "Stringer Parameters"))
        widget.setWindowIcon(QtGui.QIcon(":/icons/Arch_Stringer.svg"))
        layout = QtGui.QVBoxLayout(widget)

        self.stringer_tree = QtGui.QTreeWidget()
        self.stringer_tree.setColumnCount(2)
        self.stringer_tree.setHeaderHidden(True)
        self.stringer_tree.setRootIsDecorated(True)
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
        layout.addWidget(self.stringer_tree)
        self._populate_stringer_tree()

        form = QtGui.QFormLayout()

        self.stringer_thickness = _length_spin(
            _value(self.stair.StringerThickness), 0.01
        )
        form.addRow(
            translate("BIM", "Thickness"), self.stringer_thickness
        )

        self.stringer_custom_width = QtGui.QCheckBox(
            translate("BIM", "Custom width")
        )
        self.stringer_custom_width.setChecked(
            self.stair.StringerCustomWidth
        )
        self.stringer_width = _length_spin(
            _value(self.stair.StringerWidth), 0.01
        )
        self.stringer_width.setReadOnly(
            not self.stringer_custom_width.isChecked()
        )
        form.addRow(self.stringer_custom_width, self.stringer_width)

        self.stringer_step_overlap = _length_spin(
            _value(self.stair.StringerStepOverlap), -1000000.0
        )
        form.addRow(
            translate("BIM", "Step overlap"),
            self.stringer_step_overlap,
        )
        self.stringer_start_extension = _length_spin(
            _value(self.stair.StringerStartExtension)
        )
        self.stringer_end_extension = _length_spin(
            _value(self.stair.StringerEndExtension)
        )
        form.addRow(
            translate("BIM", "Length beyond first step"),
            self.stringer_start_extension,
        )
        form.addRow(
            translate("BIM", "Length beyond last step"),
            self.stringer_end_extension,
        )
        self.stringer_position_label = QtGui.QLabel(
            translate("BIM", "Position above nosing")
        )
        self.stringer_position_editor = QtGui.QWidget()
        position_layout = QtGui.QHBoxLayout(
            self.stringer_position_editor
        )
        position_layout.setContentsMargins(0, 0, 0, 0)
        self.stringer_nosing_direction = QtGui.QComboBox()
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
        self.stringer_nosing_offset = _length_spin(
            _value(self.stair.StringerNosingOffset)
        )
        position_layout.addWidget(self.stringer_nosing_direction)
        position_layout.addWidget(self.stringer_nosing_offset)
        form.addRow(
            self.stringer_position_label,
            self.stringer_position_editor,
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
            label = form.labelForField(editor)
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
        layout.addLayout(form)

        layout.addWidget(self._make_stringer_override_widget())

        layout.addStretch()

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
        self._update_stringer_editor_visibility()
        return widget

    def _make_handrail_panel(self):
        widget = QtGui.QWidget()
        widget.setWindowTitle(translate("BIM", "Handrail Parameters"))
        widget.setWindowIcon(QtGui.QIcon(":/icons/Arch_Handrail.svg"))
        layout = QtGui.QVBoxLayout(widget)

        self.handrail_tree = QtGui.QTreeWidget()
        self.handrail_tree.setColumnCount(2)
        self.handrail_tree.setHeaderHidden(True)
        self.handrail_tree.setRootIsDecorated(True)
        self.handrail_tree.setMinimumHeight(245)
        self.handrail_tree.setToolTip(
            translate(
                "BIM",
                "Enables a handrail independently on the left or right side of each flight",
            )
        )
        self.handrail_tree.header().setSectionResizeMode(
            0, QtGui.QHeaderView.ResizeToContents
        )
        self.handrail_tree.header().setSectionResizeMode(
            1, QtGui.QHeaderView.Stretch
        )
        layout.addWidget(self.handrail_tree)
        self._populate_handrail_tree()

        form = QtGui.QFormLayout()
        self.handrail_height = _length_spin(
            _value(self.stair.HandrailHeightAboveNosing), 0.01
        )
        self.handrail_offset = _length_spin(
            _value(self.stair.HandrailOffset), -1000000.0
        )
        self.handrail_picket_spacing = _length_spin(
            _value(self.stair.HandrailPicketMaximumSpacing), 0.01
        )
        form.addRow(
            translate("BIM", "Height above nosing"),
            self.handrail_height,
        )
        form.addRow(translate("BIM", "Offset"), self.handrail_offset)
        form.addRow(
            translate("BIM", "Picket maximum spacing"),
            self.handrail_picket_spacing,
        )
        general_help = {
            self.handrail_height: translate(
                "BIM",
                "Vertical distance from the tread nosing line to the top of the top rail. Height requirements vary by the applicable building code.",
            ),
            self.handrail_offset: translate(
                "BIM",
                "Offsets the handrail from the stringer center. Positive values move it toward the stair interior; negative values move it outward.",
            ),
            self.handrail_picket_spacing: translate(
                "BIM",
                "Maximum clear opening between pickets. Common targets are 100 mm in UK/European practice and 4 in (102 mm) under the US IRC; requirements vary by local code.",
            ),
        }
        for editor, help_text in general_help.items():
            editor.setToolTip(help_text)
            label = form.labelForField(editor)
            if label is not None:
                label.setToolTip(help_text)
        layout.addLayout(form)

        pickets = QtGui.QGroupBox(translate("BIM", "Picket Parameters"))
        picket_form = QtGui.QFormLayout(pickets)
        self.handrail_picket_shape = self._make_handrail_shape_editor(
            str(self.stair.HandrailPicketShape)
        )
        self.handrail_picket_width = _length_spin(
            _value(self.stair.HandrailPicketWidth), 0.01
        )
        self.handrail_picket_thickness = _length_spin(
            _value(self.stair.HandrailPicketThickness), 0.01
        )
        self.handrail_picket_stringer_penetration = _length_spin(
            _value(self.stair.HandrailPicketStringerPenetration)
        )
        self.handrail_picket_top_rail_penetration = _length_spin(
            _value(self.stair.HandrailPicketTopRailPenetration)
        )
        picket_form.addRow(
            translate("BIM", "Shape"), self.handrail_picket_shape
        )
        picket_form.addRow(
            translate("BIM", "Width / diameter"),
            self.handrail_picket_width,
        )
        picket_form.addRow(
            translate("BIM", "Thickness"),
            self.handrail_picket_thickness,
        )
        picket_form.addRow(
            translate("BIM", "Penetration in stringer"),
            self.handrail_picket_stringer_penetration,
        )
        picket_form.addRow(
            translate("BIM", "Penetration in top rail"),
            self.handrail_picket_top_rail_penetration,
        )
        picket_help = {
            self.handrail_picket_shape: translate(
                "BIM", "Square or circular picket cross-section"
            ),
            self.handrail_picket_width: translate(
                "BIM",
                "Picket width; for a circular picket this is its diameter",
            ),
            self.handrail_picket_thickness: translate(
                "BIM",
                "Picket depth along the walking direction. Circular pickets use the diameter instead.",
            ),
            self.handrail_picket_stringer_penetration: translate(
                "BIM",
                "Depth that a wooden-stair picket enters the stringer",
            ),
            self.handrail_picket_top_rail_penetration: translate(
                "BIM", "Depth that the picket enters the top rail"
            ),
        }
        for editor, help_text in picket_help.items():
            editor.setToolTip(help_text)
            label = picket_form.labelForField(editor)
            if label is not None:
                label.setToolTip(help_text)
        pickets.setToolTip(
            translate(
                "BIM",
                "Pickets are distributed evenly using the fewest members that respect the maximum clear spacing",
            )
        )
        layout.addWidget(pickets)

        posts = QtGui.QGroupBox(translate("BIM", "Post Parameters"))
        post_form = QtGui.QFormLayout(posts)
        self.handrail_post_shape = self._make_handrail_shape_editor(
            str(self.stair.HandrailPostShape)
        )
        self.handrail_post_width = _length_spin(
            _value(self.stair.HandrailPostWidth), 0.01
        )
        self.handrail_post_thickness = _length_spin(
            _value(self.stair.HandrailPostThickness), 0.01
        )
        self.handrail_post_above = _length_spin(
            _value(self.stair.HandrailPostAboveTopRail)
        )
        self.handrail_post_below = _length_spin(
            _value(self.stair.HandrailPostBelowStringer)
        )
        post_form.addRow(
            translate("BIM", "Shape"), self.handrail_post_shape
        )
        post_form.addRow(
            translate("BIM", "Width / diameter"),
            self.handrail_post_width,
        )
        post_form.addRow(
            translate("BIM", "Thickness"), self.handrail_post_thickness
        )
        post_form.addRow(
            translate("BIM", "Length above top rail"),
            self.handrail_post_above,
        )
        post_form.addRow(
            translate("BIM", "Length below stringer"),
            self.handrail_post_below,
        )
        post_help = {
            self.handrail_post_shape: translate(
                "BIM", "Square or circular post cross-section."
            ),
            self.handrail_post_width: translate(
                "BIM", "Post width; for a circular post this is its diameter"
            ),
            self.handrail_post_thickness: translate(
                "BIM",
                "Post depth along the walking direction. Circular posts use the diameter instead.",
            ),
            self.handrail_post_above: translate(
                "BIM", "Post length extending vertically above the top rail"
            ),
            self.handrail_post_below: translate(
                "BIM",
                "Post length below a wooden stringer. The first post stops at the floor.",
            ),
        }
        for editor, help_text in post_help.items():
            editor.setToolTip(help_text)
            label = post_form.labelForField(editor)
            if label is not None:
                label.setToolTip(help_text)
        layout.addWidget(posts)

        top_rail = QtGui.QGroupBox(
            translate("BIM", "Top rail Parameters")
        )
        top_rail_form = QtGui.QFormLayout(top_rail)
        self.handrail_top_rail_shape = self._make_handrail_shape_editor(
            str(self.stair.HandrailTopRailShape)
        )
        self.handrail_top_rail_width = _length_spin(
            _value(self.stair.HandrailTopRailWidth), 0.01
        )
        self.handrail_top_rail_thickness = _length_spin(
            _value(self.stair.HandrailTopRailThickness), 0.01
        )
        self.handrail_top_rail_penetration = _length_spin(
            _value(self.stair.HandrailTopRailPostPenetration)
        )
        top_rail_form.addRow(
            translate("BIM", "Shape"), self.handrail_top_rail_shape
        )
        top_rail_form.addRow(
            translate("BIM", "Width / diameter"),
            self.handrail_top_rail_width,
        )
        top_rail_form.addRow(
            translate("BIM", "Thickness"),
            self.handrail_top_rail_thickness,
        )
        top_rail_form.addRow(
            translate("BIM", "Penetration in posts"),
            self.handrail_top_rail_penetration,
        )
        top_rail_help = {
            self.handrail_top_rail_shape: translate(
                "BIM", "Square or circular top-rail cross-section"
            ),
            self.handrail_top_rail_width: translate(
                "BIM",
                "Top-rail width; for a circular rail this is its diameter",
            ),
            self.handrail_top_rail_thickness: translate(
                "BIM",
                "Vertical top-rail thickness. Circular rails use the diameter instead.",
            ),
            self.handrail_top_rail_penetration: translate(
                "BIM",
                "Distance the top rail enters each end post, measured from the post's inner face",
            ),
        }
        for editor, help_text in top_rail_help.items():
            editor.setToolTip(help_text)
            label = top_rail_form.labelForField(editor)
            if label is not None:
                label.setToolTip(help_text)
        layout.addWidget(top_rail)
        layout.addStretch()

        for editor in (
            self.handrail_height,
            self.handrail_offset,
            self.handrail_picket_spacing,
            self.handrail_picket_width,
            self.handrail_picket_thickness,
            self.handrail_picket_stringer_penetration,
            self.handrail_picket_top_rail_penetration,
            self.handrail_post_width,
            self.handrail_post_thickness,
            self.handrail_post_above,
            self.handrail_post_below,
            self.handrail_top_rail_width,
            self.handrail_top_rail_thickness,
            self.handrail_top_rail_penetration,
        ):
            editor.valueChanged.connect(self._apply)
        for editor in (
            self.handrail_picket_shape,
            self.handrail_post_shape,
            self.handrail_top_rail_shape,
        ):
            editor.currentIndexChanged.connect(
                self._handrail_shape_changed
            )
        self._update_handrail_shape_editors()
        return widget

    def _make_handrail_shape_editor(self, value):
        editor = QtGui.QComboBox()
        editor.addItem(translate("BIM", "Square"), "Square")
        editor.addItem(translate("BIM", "Circular"), "Circular")
        self._select_data(editor, value)
        return editor

    def _populate_stringer_tree(self):
        self.stringer_tree.clear()
        self.stringer_flight_editors = []
        self.stringer_all_editors = {}
        flights = stair_objects.get_flights(self.stair)
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

    def _populate_handrail_tree(self):
        self.handrail_tree.clear()
        self.handrail_flight_editors = []
        self.handrail_all_editors = {}
        flights = stair_objects.get_flights(self.stair)
        if len(flights) > 1:
            root = QtGui.QTreeWidgetItem(self.handrail_tree)
            root.setText(0, translate("BIM", "All"))
            root.setFirstColumnSpanned(True)
            for side, label in (
                ("Left", translate("BIM", "Left side")),
                ("Right", translate("BIM", "Right side")),
            ):
                states = {
                    bool(getattr(flight, f"{side}HandrailEnabled"))
                    for flight in flights
                }
                editor = QtGui.QCheckBox()
                editor.setTristate(True)
                editor.setCheckState(
                    QtCore.Qt.PartiallyChecked
                    if len(states) > 1
                    else (
                        QtCore.Qt.Checked
                        if states.pop()
                        else QtCore.Qt.Unchecked
                    )
                )
                child = QtGui.QTreeWidgetItem(root)
                child.setText(0, label)
                self.handrail_tree.setItemWidget(child, 1, editor)
                self.handrail_all_editors[side] = editor
                editor.stateChanged.connect(
                    partial(self._all_handrail_changed, side, editor)
                )
            root.setExpanded(True)

        for flight in flights:
            root = QtGui.QTreeWidgetItem(self.handrail_tree)
            root.setText(0, flight.Label)
            root.setFirstColumnSpanned(True)
            record = {"flight": flight, "item": root}
            for side, label in (
                ("Left", translate("BIM", "Left side")),
                ("Right", translate("BIM", "Right side")),
            ):
                editor = QtGui.QCheckBox()
                editor.setChecked(
                    bool(getattr(flight, f"{side}HandrailEnabled"))
                )
                child = QtGui.QTreeWidgetItem(root)
                child.setText(0, label)
                self.handrail_tree.setItemWidget(child, 1, editor)
                record[f"{side.lower()}_enabled"] = editor
                editor.toggled.connect(self._handrail_enabled_changed)
            root.setExpanded(True)
            self.handrail_flight_editors.append(record)

    def _all_handrail_changed(self, side, editor, _state):
        if self._loading:
            return
        check_state = editor.checkState()
        if check_state == QtCore.Qt.PartiallyChecked:
            # A tristate checkbox normally cycles Unchecked -> Partially
            # checked -> Checked.  The partial state is only useful as a
            # programmatic mixed-state display, not as a user action.
            blocked = editor.blockSignals(True)
            editor.setCheckState(QtCore.Qt.Checked)
            editor.blockSignals(blocked)
            check_state = QtCore.Qt.Checked
        checked = check_state == QtCore.Qt.Checked
        for record in self.handrail_flight_editors:
            flight_editor = record[f"{side.lower()}_enabled"]
            blocked = flight_editor.blockSignals(True)
            flight_editor.setChecked(checked)
            flight_editor.blockSignals(blocked)
        self._handrail_enabled_changed()

    def _handrail_enabled_changed(self, *args):
        self._refresh_all_handrail_editors()
        self._apply()

    def _refresh_all_handrail_editors(self):
        for side, editor in self.handrail_all_editors.items():
            states = {
                record[f"{side.lower()}_enabled"].isChecked()
                for record in self.handrail_flight_editors
            }
            blocked = editor.blockSignals(True)
            editor.setCheckState(
                QtCore.Qt.PartiallyChecked
                if len(states) > 1
                else (
                    QtCore.Qt.Checked
                    if states.pop()
                    else QtCore.Qt.Unchecked
                )
            )
            editor.blockSignals(blocked)

    def _handrail_shape_changed(self, *args):
        self._update_handrail_shape_editors()
        self._apply()

    def _update_handrail_shape_editors(self):
        for shape_editor, thickness_editor in (
            (
                self.handrail_picket_shape,
                self.handrail_picket_thickness,
            ),
            (
                self.handrail_post_shape,
                self.handrail_post_thickness,
            ),
            (
                self.handrail_top_rail_shape,
                self.handrail_top_rail_thickness,
            ),
        ):
            circular = (
                str(
                    shape_editor.itemData(shape_editor.currentIndex())
                )
                == "Circular"
            )
            thickness_editor.setEnabled(not circular)

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

    def _make_multiflight_panel(self):
        self.flight_tree = _FlightTreeWidget(self._remove_flight)
        self.flight_tree.setColumnCount(2)
        self.flight_tree.setHeaderHidden(True)
        self.flight_tree.setRootIsDecorated(True)
        self.flight_tree.setMinimumHeight(245)
        self.flight_tree.header().setSectionResizeMode(0, QtGui.QHeaderView.ResizeToContents)
        self.flight_tree.header().setSectionResizeMode(1, QtGui.QHeaderView.Stretch)
        self.flight_tree.setContextMenuPolicy(QtCore.Qt.CustomContextMenu)
        self.flight_tree.customContextMenuRequested.connect(
            self._show_flight_context_menu
        )

        self._populate_flight_tree()
        return self.flight_tree

    def _populate_flight_tree(self):
        self.flight_tree.clear()
        self.flight_editors = []
        flights = stair_objects.get_flights(self.stair)
        single_flight = len(flights) == 1
        all_straight = all(
            str(flight.FlightType) == "Straight" for flight in flights
        )
        for index, flight in enumerate(flights):
            root = QtGui.QTreeWidgetItem(self.flight_tree)
            root.setText(0, flight.Label)
            root.setFirstColumnSpanned(True)
            record = {"flight": flight, "item": root}

            flight_type = QtGui.QComboBox()
            flight_type.addItem(translate("BIM", "Straight flight"), "Straight")
            flight_type.addItem(translate("BIM", "Circular flight"), "Circular")
            flight_type.addItem(
                translate("BIM", "Straight landing"), "Straight landing"
            )
            flight_type.addItem(
                translate("BIM", "Circular landing"), "Circular landing"
            )
            self._select_data(flight_type, str(flight.FlightType))
            record["flight_type"] = flight_type
            record["left_length"] = _length_spin(_value(flight.LeftLength), 1.0)
            record["right_length"] = _length_spin(_value(flight.RightLength), 1.0)
            record["inner_radius"] = _length_spin(
                _value(flight.InnerRadius), 1.0
            )
            record["outer_radius"] = _length_spin(
                _value(flight.OuterRadius), 1.0
            )
            record["width"] = _length_spin(_value(flight.Width), 1.0)
            record["angle"] = self._angle_spin(_value(flight.Angle))
            rotation = QtGui.QComboBox()
            rotation.addItem(translate("BIM", "Left"), "Left")
            rotation.addItem(translate("BIM", "Right"), "Right")
            self._select_data(rotation, str(flight.Rotation))
            record["rotation"] = rotation
            is_circular = str(flight.FlightType).startswith("Circular")
            is_landing = str(flight.FlightType).endswith("landing")
            previous_is_circular = index > 0 and str(
                flights[index - 1].FlightType
            ).startswith("Circular")
            previous_is_landing = index > 0 and str(
                flights[index - 1].FlightType
            ).endswith("landing")
            parameters = [(translate("BIM", "Type"), flight_type)]
            if index:
                parameters.append((translate("BIM", "Rotation"), rotation))
            winding_rows = []
            if is_circular:
                record["angle"].setRange(0.01, 359.99)
                parameters.extend(
                    (
                        (translate("BIM", "Inner radius"), record["inner_radius"]),
                        (translate("BIM", "Outer radius"), record["outer_radius"]),
                        (translate("BIM", "Width"), record["width"]),
                        (translate("BIM", "Angle"), record["angle"]),
                    )
                )
            else:
                if single_flight:
                    parameters.extend(
                        (
                            (translate("BIM", "Length"), record["left_length"]),
                            (translate("BIM", "Width"), record["width"]),
                        )
                    )
                else:
                    parameters.extend(
                        (
                            (
                                translate("BIM", "Left length"),
                                record["left_length"],
                            ),
                            (
                                translate("BIM", "Right length"),
                                record["right_length"],
                            ),
                            (translate("BIM", "Width"), record["width"]),
                        )
                    )
            if index:
                turn_type = QtGui.QComboBox()
                turn_type.addItem(
                    translate("BIM", "Herse balancing"), "Herse balancing"
                )
                turn_type.addItem(translate("BIM", "Landing"), "Landing")
                self._select_data(turn_type, str(flight.TurnType))
                record["turn_type"] = turn_type
                if not is_circular:
                    parameters.append((translate("BIM", "Angle"), record["angle"]))
                    supports_winding = not (
                        previous_is_circular
                        or is_landing
                        or previous_is_landing
                    )
                    if supports_winding:
                        local_winding = _percent_spin(flight.WindingLocal)
                        distant_winding = _percent_spin(flight.WindingDistant)
                        local_winding.setToolTip(
                            translate(
                                "BIM",
                                "Adjusts winders nearest the inner corner",
                            )
                        )
                        distant_winding.setToolTip(
                            translate(
                                "BIM",
                                "Adjusts how far winding extends from the inner corner",
                            )
                        )
                        record["winding_local"] = local_winding
                        record["winding_distant"] = distant_winding
                        parameters.append(
                            (translate("BIM", "Turn type"), turn_type)
                        )
                        winding_rows.append(
                            (translate("BIM", "Local winding"), local_winding)
                        )
                        winding_rows.append(
                            (
                                translate("BIM", "Distant winding"),
                                distant_winding,
                            )
                        )
            if is_landing and not is_circular:
                entry_direction = QtGui.QComboBox()
                entry_direction.addItem(
                    translate("BIM", "Straight"), "Straight"
                )
                entry_direction.addItem(
                    translate("BIM", "From left"), "From left"
                )
                entry_direction.addItem(
                    translate("BIM", "From right"), "From right"
                )
                self._select_data(
                    entry_direction, str(flight.EntryDirection)
                )
                record["entry_direction"] = entry_direction
                exit_direction = QtGui.QComboBox()
                exit_direction.addItem(
                    translate("BIM", "Straight"), "Straight"
                )
                exit_direction.addItem(
                    translate("BIM", "To left"), "To left"
                )
                exit_direction.addItem(
                    translate("BIM", "To right"), "To right"
                )
                self._select_data(exit_direction, str(flight.ExitDirection))
                record["exit_direction"] = exit_direction
                parameters.append(
                    (translate("BIM", "Entry direction"), entry_direction)
                )
                parameters.append(
                    (translate("BIM", "Exit direction"), exit_direction)
                )
            if index == 0 and all_straight:
                record["start_angle"] = self._angle_spin(_value(flight.StartAngle))
                record["start_angle"].setRange(-89.0, 89.0)
                entry_direction = QtGui.QComboBox()
                entry_direction.addItem(translate("BIM", "Straight"), "Straight")
                entry_direction.addItem(
                    translate("BIM", "From left"), "From left"
                )
                entry_direction.addItem(
                    translate("BIM", "From right"), "From right"
                )
                self._select_data(entry_direction, str(flight.EntryDirection))
                record["entry_direction"] = entry_direction
                parameters.append(
                    (translate("BIM", "Start angle"), record["start_angle"])
                )
                parameters.append(
                    (translate("BIM", "Entry direction"), entry_direction)
                )
            if index == len(flights) - 1 and all_straight:
                record["end_angle"] = self._angle_spin(_value(flight.EndAngle))
                record["end_angle"].setRange(-89.0, 89.0)
                exit_direction = QtGui.QComboBox()
                exit_direction.addItem(translate("BIM", "Straight"), "Straight")
                exit_direction.addItem(translate("BIM", "To left"), "To left")
                exit_direction.addItem(translate("BIM", "To right"), "To right")
                self._select_data(exit_direction, str(flight.ExitDirection))
                record["exit_direction"] = exit_direction
                parameters.append(
                    (translate("BIM", "End angle"), record["end_angle"])
                )
                parameters.append(
                    (translate("BIM", "Exit direction"), exit_direction)
                )
            turn_type_item = None
            for label, editor in parameters:
                child = QtGui.QTreeWidgetItem(root)
                child.setText(0, label)
                self.flight_tree.setItemWidget(child, 1, editor)
                if editor is record.get("turn_type"):
                    turn_type_item = child
            if turn_type_item is not None:
                record["winding_items"] = []
                hide_winding = str(flight.TurnType) == "Landing"
                for label, editor in winding_rows:
                    child = QtGui.QTreeWidgetItem(turn_type_item)
                    child.setText(0, label)
                    child.setHidden(hide_winding)
                    self.flight_tree.setItemWidget(child, 1, editor)
                    record["winding_items"].append(child)
                turn_type_item.setExpanded(True)
            root.setExpanded(True)
            self.flight_editors.append(record)
            for key in ("left_length", "right_length"):
                editor = record.get(key)
                if editor:
                    editor.valueChanged.connect(
                        lambda _value, current=record, side=key: (
                            self._flight_length_changed(current, side)
                        )
                    )
            for key in ("inner_radius", "outer_radius"):
                editor = record[key]
                editor.valueChanged.connect(
                    lambda _value, current=record, radius=key: (
                        self._flight_radius_changed(current, radius)
                    )
                )
            for key in ("width", "angle"):
                editor = record.get(key)
                if editor:
                    editor.valueChanged.connect(
                        lambda _value, current=record: self._turn_geometry_changed(
                            current
                        )
                    )
            for key in ("start_angle", "end_angle"):
                editor = record.get(key)
                if editor:
                    editor.valueChanged.connect(
                        lambda _value, current=record: (
                            self._endpoint_angle_changed(current)
                        )
                    )
            for key in ("entry_direction", "exit_direction"):
                editor = record.get(key)
                if editor:
                    editor.currentIndexChanged.connect(self._apply)
            flight_type.currentIndexChanged.connect(
                lambda _index, current=record: self._flight_type_changed(
                    current
                )
            )
            record["rotation"].currentIndexChanged.connect(
                lambda _index, current=record: self._turn_geometry_changed(
                    current
                )
            )
            if record.get("turn_type") is not None:
                record["turn_type"].currentIndexChanged.connect(
                    lambda _index, current=record: self._turn_type_changed(
                        current
                    )
                )
            for key in ("winding_local", "winding_distant"):
                editor = record.get(key)
                if editor:
                    editor.valueChanged.connect(self._apply)
    def _flight_length_changed(self, record, side):
        if self._loading:
            return
        self._sync_flight_length_editors(
            self.flight_editors.index(record),
            "LeftLength" if side == "left_length" else "RightLength",
        )
        self._apply()

    def _flight_radius_changed(self, record, radius):
        if self._loading:
            return
        self._sync_flight_radius_editors(
            self.flight_editors.index(record),
            "InnerRadius" if radius == "inner_radius" else "OuterRadius",
        )
        self._apply()

    def _turn_geometry_changed(self, record):
        if self._loading:
            return
        index = self.flight_editors.index(record)
        if self._editor_flight_type(record).startswith("Circular"):
            self._sync_flight_radius_editors(index)
        else:
            self._sync_flight_length_editors(index)
        if index:
            self._sync_flight_length_editors(index - 1)
        if index + 1 < len(self.flight_editors):
            self._sync_flight_length_editors(index + 1)
        self._apply()

    def _turn_type_changed(self, record):
        if self._loading:
            return
        editor = record["turn_type"]
        is_landing = (
            str(editor.itemData(editor.currentIndex())) == "Landing"
        )
        for item in record.get("winding_items", ()):
            item.setHidden(is_landing)
        self._apply()

    def _endpoint_angle_changed(self, record):
        if self._loading:
            return
        self._sync_flight_length_editors(
            self.flight_editors.index(record), "LeftLength"
        )
        self._apply()

    def _flight_type_changed(self, record):
        if self._loading:
            return
        index = self.flight_editors.index(record)
        flight_type = self._editor_flight_type(record)
        if flight_type.startswith("Circular"):
            record["angle"].setRange(0.01, 359.99)
            blocked = record["angle"].blockSignals(True)
            record["angle"].setValue(90.0)
            record["angle"].blockSignals(blocked)
            self._sync_flight_radius_editors(index)
        else:
            record["angle"].setRange(-360.0, 360.0)
            if flight_type.endswith("landing"):
                blocked = record["angle"].blockSignals(True)
                record["angle"].setValue(90.0)
                record["angle"].blockSignals(blocked)
            self._sync_flight_length_editors(index)
        if index:
            self._sync_flight_length_editors(index - 1)
        if index + 1 < len(self.flight_editors):
            self._sync_flight_length_editors(index + 1)
        self._apply()
        self._loading = True
        try:
            self._populate_flight_tree()
            if hasattr(self, "stringer_tree"):
                self._populate_stringer_tree()
            if hasattr(self, "handrail_tree"):
                self._populate_handrail_tree()
        finally:
            self._loading = False

    @staticmethod
    def _editor_flight_type(record):
        editor = record["flight_type"]
        return str(editor.itemData(editor.currentIndex()))

    def _sync_flight_length_editors(self, index, driver=None):
        record = self.flight_editors[index]
        if self._editor_flight_type(record).startswith("Circular"):
            self._sync_flight_radius_editors(index)
            return
        next_record = (
            self.flight_editors[index + 1]
            if index + 1 < len(self.flight_editors)
            else None
        )
        previous_record = (
            self.flight_editors[index - 1] if index > 0 else None
        )
        incoming_straight_turn = (
            previous_record
            and not self._editor_flight_type(previous_record).startswith(
                "Circular"
            )
            and abs(record["angle"].value()) > 1e-7
        )
        straight_turn = (
            next_record
            and not self._editor_flight_type(next_record).startswith("Circular")
            and abs(next_record["angle"].value()) > 1e-7
        )
        signed_length_difference = 0.0
        if incoming_straight_turn:
            turn_difference = stair_objects.straight_turn_side_difference(
                record["width"].value(),
                previous_record["width"].value(),
                record["angle"].value(),
            )
            rotation = record["rotation"]
            if str(rotation.itemData(rotation.currentIndex())) == "Right":
                turn_difference = -turn_difference
            signed_length_difference += turn_difference
        if straight_turn:
            turn_difference = stair_objects.straight_turn_side_difference(
                record["width"].value(),
                next_record["width"].value(),
                next_record["angle"].value(),
            )
            rotation = next_record["rotation"]
            if str(rotation.itemData(rotation.currentIndex())) == "Right":
                turn_difference = -turn_difference
            signed_length_difference += turn_difference
        all_straight = all(
            self._editor_flight_type(item) == "Straight"
            for item in self.flight_editors
        )
        if all_straight and index == 0 and record.get("start_angle") is not None:
            signed_length_difference += record["width"].value() * math.tan(
                math.radians(record["start_angle"].value())
            )
        if (
            all_straight
            and index == len(self.flight_editors) - 1
            and record.get("end_angle") is not None
        ):
            signed_length_difference -= record["width"].value() * math.tan(
                math.radians(record["end_angle"].value())
            )
        left, right = stair_objects.linked_flight_side_lengths_for_difference(
            record["left_length"].value(),
            record["right_length"].value(),
            signed_length_difference,
            driver,
        )
        for editor, value in (
            (record["left_length"], left),
            (record["right_length"], right),
        ):
            blocked = editor.blockSignals(True)
            editor.setValue(value)
            editor.blockSignals(blocked)

    def _sync_flight_radius_editors(self, index, driver=None):
        record = self.flight_editors[index]
        inner, outer = stair_objects.linked_circular_radii(
            record["inner_radius"].value(),
            record["outer_radius"].value(),
            record["width"].value(),
            driver,
        )
        for editor, value in (
            (record["inner_radius"], inner),
            (record["outer_radius"], outer),
        ):
            blocked = editor.blockSignals(True)
            editor.setValue(value)
            editor.blockSignals(blocked)

    def _add_flight(self):
        if self._loading:
            return
        self._loading = True
        try:
            flights = stair_objects.get_flights(self.stair)
            stair_objects.resize_flights(self.stair, len(flights) + 1)
            self.flight = self._first_flight()
            self._populate_flight_tree()
            if hasattr(self, "stringer_tree"):
                self._populate_stringer_tree()
            if hasattr(self, "handrail_tree"):
                self._populate_handrail_tree()
        finally:
            self._loading = False
        self.stair.Proxy.rebuild(self.stair, allow_structure_changes=True)
        self.stair.Document.recompute()
        self._refresh_diagnostics()

    def _show_flight_context_menu(self, position):
        clicked = self.flight_tree.itemAt(position)
        if clicked is not None:
            self.flight_tree.setCurrentItem(clicked)
        selected = self.flight_tree.currentItem()
        menu = QtGui.QMenu(self.flight_tree)
        add_action = menu.addAction(
            QtGui.QIcon(":/icons/Arch_Add.svg"),
            translate("BIM", "Add Flight"),
        )
        add_action.triggered.connect(self._add_flight)
        if selected is not None:
            menu.addSeparator()
            delete_action = menu.addAction(
                QtGui.QIcon(":/icons/Arch_Remove.svg"),
                translate("BIM", "Delete Flight"),
            )
            delete_action.setEnabled(
                len(stair_objects.get_flights(self.stair)) > 1
            )
            delete_action.triggered.connect(self._remove_flight)
        menu.exec_(self.flight_tree.viewport().mapToGlobal(position))

    def _remove_flight(self):
        flights = stair_objects.get_flights(self.stair)
        if self._loading or len(flights) <= 1:
            return
        selected = self.flight_tree.currentItem()
        selected_flight = None
        while selected and selected.parent():
            selected = selected.parent()
        for record in self.flight_editors:
            if record["item"] is selected:
                selected_flight = record["flight"]
                break
        if selected_flight is None:
            return
        self._loading = True
        proxy = self.stair.Proxy
        proxy._updating = True
        try:
            self.stair.FlightsGroup.removeObject(selected_flight)
            self.stair.Document.removeObject(selected_flight.Name)
            remaining = stair_objects.get_flights(self.stair)
            if str(remaining[0].FlightType) == "Straight":
                remaining[0].Angle = 0.0
            for index, flight in enumerate(remaining):
                flight.Label = f"{translate('BIM', 'Flight')} {index + 1}"
            stair_objects.sync_all_flight_side_lengths(self.stair)
            self.flight = self._first_flight()
            self._populate_flight_tree()
            if hasattr(self, "stringer_tree"):
                self._populate_stringer_tree()
            if hasattr(self, "handrail_tree"):
                self._populate_handrail_tree()
        finally:
            proxy._updating = False
            self._loading = False
        self.stair.Proxy.rebuild(self.stair, allow_structure_changes=True)
        self.stair.Document.recompute()
        self._refresh_diagnostics()

    def _connect_stair_controls(self):
        for spin in (
            self.floor_height,
            self.step_count,
            self.concrete_thickness,
        ):
            spin.valueChanged.connect(self._apply)
        self.stair_type.currentIndexChanged.connect(self._apply)

    def _connect_step_controls(self):
        for spin in (
            self.step_thickness,
            self.nosing,
            self.riser_thickness,
            self.step_riser_overlap,
            self.riser_upper_offset,
            self.riser_lower_offset,
        ):
            spin.valueChanged.connect(self._apply)
        self.risers_group.toggled.connect(self._apply)
        self.priority_to_riser.toggled.connect(self._priority_changed)

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

    def addSelection(self, *args):
        self._update_stringer_selection()

    def removeSelection(self, *args):
        self._update_stringer_selection()

    def setSelection(self, *args):
        self._update_stringer_selection()

    def clearSelection(self, *args):
        self._update_stringer_selection()

    def _remove_selection_observer(self):
        if self._selection_observer_registered:
            FreeCADGui.Selection.removeObserver(self)
            self._selection_observer_registered = False

    def _priority_changed(self, *args):
        self._update_overlap_label()
        self._apply(*args)

    def _apply(self, *args):
        if self._loading or not self.flight:
            return
        proxy = self.stair.Proxy
        proxy._updating = True
        try:
            if "stairs" in self.sections:
                self.stair.StairType = str(
                    self.stair_type.itemData(self.stair_type.currentIndex())
                )
                self.stair.FloorHeight = self.floor_height.value()
                self.stair.NumberOfSteps = self.step_count.value()
                self.stair.ConcreteThickness = self.concrete_thickness.value()
            if "steps" in self.sections:
                self.stair.StepThickness = self.step_thickness.value()
                self.stair.Nosing = self.nosing.value()
                self.stair.RisersEnabled = self.risers_group.isChecked()
                self.stair.RiserThickness = self.riser_thickness.value()
                self.stair.PriorityToRiser = self.priority_to_riser.isChecked()
                self.stair.StepRiserOverlap = self.step_riser_overlap.value()
                self.stair.RiserUpperOffset = self.riser_upper_offset.value()
                self.stair.RiserLowerOffset = self.riser_lower_offset.value()
            if "stringers" in self.sections:
                for record in self.stringer_flight_editors:
                    flight = record["flight"]
                    flight_proxy = flight.Proxy
                    was_updating = getattr(
                        flight_proxy, "_updating", False
                    )
                    flight_proxy._updating = True
                    try:
                        for side in ("Left", "Right"):
                            editor = record[f"{side.lower()}_type"]
                            setattr(
                                flight,
                                f"{side}StringerType",
                                str(
                                    editor.itemData(
                                        editor.currentIndex()
                                    )
                                ),
                            )
                    finally:
                        flight_proxy._updating = was_updating
                self.stair.StringerThickness = (
                    self.stringer_thickness.value()
                )
                self.stair.StringerCustomWidth = (
                    self.stringer_custom_width.isChecked()
                )
                if self.stair.StringerCustomWidth:
                    self.stair.StringerWidth = (
                        self.stringer_width.value()
                    )
                self.stair.StringerStepOverlap = (
                    self.stringer_step_overlap.value()
                )
                self.stair.StringerStartExtension = (
                    self.stringer_start_extension.value()
                )
                self.stair.StringerEndExtension = (
                    self.stringer_end_extension.value()
                )
                self.stair.StringerNosingOffsetDirection = str(
                    self.stringer_nosing_direction.itemData(
                        self.stringer_nosing_direction.currentIndex()
                    )
                )
                self.stair.StringerNosingOffset = (
                    self.stringer_nosing_offset.value()
                )
            if "handrails" in self.sections:
                for record in self.handrail_flight_editors:
                    flight = record["flight"]
                    flight_proxy = flight.Proxy
                    was_updating = getattr(
                        flight_proxy, "_updating", False
                    )
                    flight_proxy._updating = True
                    try:
                        for side in ("Left", "Right"):
                            setattr(
                                flight,
                                f"{side}HandrailEnabled",
                                record[
                                    f"{side.lower()}_enabled"
                                ].isChecked(),
                            )
                    finally:
                        flight_proxy._updating = was_updating
                self.stair.HandrailHeightAboveNosing = (
                    self.handrail_height.value()
                )
                self.stair.HandrailOffset = self.handrail_offset.value()
                self.stair.HandrailPicketMaximumSpacing = (
                    self.handrail_picket_spacing.value()
                )
                self.stair.HandrailPicketShape = str(
                    self.handrail_picket_shape.itemData(
                        self.handrail_picket_shape.currentIndex()
                    )
                )
                self.stair.HandrailPicketWidth = (
                    self.handrail_picket_width.value()
                )
                self.stair.HandrailPicketThickness = (
                    self.handrail_picket_thickness.value()
                )
                self.stair.HandrailPicketStringerPenetration = (
                    self.handrail_picket_stringer_penetration.value()
                )
                self.stair.HandrailPicketTopRailPenetration = (
                    self.handrail_picket_top_rail_penetration.value()
                )
                self.stair.HandrailPostShape = str(
                    self.handrail_post_shape.itemData(
                        self.handrail_post_shape.currentIndex()
                    )
                )
                self.stair.HandrailPostWidth = (
                    self.handrail_post_width.value()
                )
                self.stair.HandrailPostThickness = (
                    self.handrail_post_thickness.value()
                )
                self.stair.HandrailPostAboveTopRail = (
                    self.handrail_post_above.value()
                )
                self.stair.HandrailPostBelowStringer = (
                    self.handrail_post_below.value()
                )
                self.stair.HandrailTopRailShape = str(
                    self.handrail_top_rail_shape.itemData(
                        self.handrail_top_rail_shape.currentIndex()
                    )
                )
                self.stair.HandrailTopRailWidth = (
                    self.handrail_top_rail_width.value()
                )
                self.stair.HandrailTopRailThickness = (
                    self.handrail_top_rail_thickness.value()
                )
                self.stair.HandrailTopRailPostPenetration = (
                    self.handrail_top_rail_penetration.value()
                )
            for record in getattr(self, "flight_editors", ()):
                flight = record["flight"]
                flight_proxy = flight.Proxy
                was_updating = getattr(flight_proxy, "_updating", False)
                flight_proxy._updating = True
                try:
                    flight_type = str(
                        record["flight_type"].itemData(
                            record["flight_type"].currentIndex()
                        )
                    )
                    flight.FlightType = flight_type
                    flight_proxy._update_dimension_visibility(flight)
                    flight.LeftLength = record["left_length"].value()
                    flight.RightLength = record["right_length"].value()
                    flight.Width = record["width"].value()
                    flight.InnerRadius = record["inner_radius"].value()
                    flight.OuterRadius = record["outer_radius"].value()
                    flight.Angle = record["angle"].value()
                    if record.get("turn_type") is not None:
                        flight.TurnType = str(
                            record["turn_type"].itemData(
                                record["turn_type"].currentIndex()
                            )
                        )
                    if record.get("winding_local") is not None:
                        flight.WindingLocal = record["winding_local"].value()
                        flight.WindingDistant = record[
                            "winding_distant"
                        ].value()
                    if record.get("rotation") is not None:
                        flight.Rotation = str(
                            record["rotation"].itemData(
                                record["rotation"].currentIndex()
                            )
                        )
                    if record.get("start_angle") is not None:
                        flight.StartAngle = record["start_angle"].value()
                    if record.get("end_angle") is not None:
                        flight.EndAngle = record["end_angle"].value()
                    if flight_type.startswith("Circular"):
                        flight.EntryDirection = "Straight"
                        flight.ExitDirection = "Straight"
                    elif record.get("entry_direction") is not None:
                        flight.EntryDirection = str(
                            record["entry_direction"].itemData(
                                record["entry_direction"].currentIndex()
                            )
                        )
                    if (
                        not flight_type.startswith("Circular")
                        and record.get("exit_direction") is not None
                    ):
                        flight.ExitDirection = str(
                            record["exit_direction"].itemData(
                                record["exit_direction"].currentIndex()
                            )
                        )
                finally:
                    flight_proxy._updating = was_updating
        finally:
            proxy._updating = False
        proxy.rebuild(self.stair, allow_structure_changes=True)
        self.stair.Document.recompute()
        if "steps" in self.sections:
            self._update_overlap_label()
        if "stringers" in self.sections:
            blocked = self.stringer_width.blockSignals(True)
            self.stringer_width.setValue(
                _value(self.stair.StringerWidth)
            )
            self.stringer_width.blockSignals(blocked)
            self._update_stringer_selection()
        self._update_type_visibility()
        if "stairs" in self.sections:
            self._refresh_diagnostics()

    def _update_overlap_label(self):
        if self.priority_to_riser.isChecked():
            text = translate("BIM", "Step penetration")
        else:
            text = translate("BIM", "Step rear overlap")
        self.step_riser_overlap_label.setText(text)

    def _update_type_visibility(self):
        if hasattr(self, "stair_type"):
            stair_type = str(
                self.stair_type.itemData(self.stair_type.currentIndex())
            )
        else:
            stair_type = str(self.stair.StairType)
        wood = stair_type == "Wood"
        if hasattr(self, "concrete_thickness"):
            self.concrete_thickness_label.setVisible(not wood)
            self.concrete_thickness.setVisible(not wood)
        steps_panel = self.section_widgets.get("steps")
        if steps_panel:
            self._set_section_visible(steps_panel, wood)
        if hasattr(self, "handrail_picket_stringer_penetration"):
            self.handrail_picket_stringer_penetration.setEnabled(wood)
            self.handrail_post_below.setEnabled(wood)
        if not wood:
            for section in ("stringers",):
                panel = self.section_widgets.get(section)
                if panel:
                    self._set_section_visible(panel, False)
            self._component_sections_hidden = True
        elif self._component_sections_hidden:
            for section in ("stringers",):
                panel = self.section_widgets.get(section)
                if panel:
                    self._set_section_visible(panel, True)
                    self._collapse_section(section, panel)
            self._component_sections_hidden = False

    def _configure_task_boxes(self):
        self._update_type_visibility()
        for section in self.sections[1:]:
            panel = self.section_widgets.get(section)
            if panel and not panel.isHidden():
                self._collapse_section(section, panel)

    def _collapse_section(self, section, panel):
        if section in self._collapsed_sections:
            return
        task_box = self._task_box(panel)
        if not task_box:
            return
        for child in task_box.findChildren(QtCore.QObject):
            if str(child.metaObject().className()).endswith("TaskHeader"):
                try:
                    invoked = QtCore.QMetaObject.invokeMethod(child, "fold")
                except (RuntimeError, TypeError):
                    invoked = False
                if invoked:
                    self._collapsed_sections.add(section)
                return

    @staticmethod
    def _set_section_visible(panel, visible):
        panel.setVisible(visible)
        task_box = StairDesignerTaskPanel._task_box(panel)
        if task_box:
            task_box.setVisible(visible)

    @staticmethod
    def _task_box(panel):
        parent = panel.parentWidget()
        while parent:
            if str(parent.metaObject().className()).endswith("TaskBox"):
                return parent
            parent = parent.parentWidget()
        return None

    def _refresh_diagnostics(self):
        self.riser_height.setValue(_value(self.stair.RiserHeight))
        self.tread_width.setValue(_value(self.stair.TreadWidth))
        self.blondel_value.setValue(_value(self.stair.BlondelValue))
        compliant = bool(self.stair.BlondelCompliant)
        warning = translate(
            "BIM",
            "Blondel law is outside "
            f"{BLONDEL_MINIMUM:.0f}-{BLONDEL_MAXIMUM:.0f} mm.",
        )
        if compliant:
            self.blondel_label.setText(translate("BIM", "Stair rule"))
            self.blondel_label.setStyleSheet("")
            self.blondel_label.setToolTip("")
            self.blondel_value.setToolTip("")
        else:
            self.blondel_label.setText(
                translate("BIM", "\u26a0 Stair rule")
            )
            self.blondel_label.setStyleSheet(
                "color: #b71c1c; font-weight: bold;"
            )
            self.blondel_label.setToolTip(warning)
            self.blondel_value.setToolTip(warning)

    def _first_flight(self):
        group = self.stair.FlightsGroup
        if not group:
            return None
        return next(
            (flight for flight in group.Group if getattr(flight.Proxy, "Type", "") == "Flight"),
            None,
        )

    def _readonly_length_spin(self):
        spin = _length_spin()
        spin.setReadOnly(True)
        spin.setButtonSymbols(QtGui.QAbstractSpinBox.NoButtons)
        return spin

    @staticmethod
    def _angle_spin(value):
        spin = _float_spin(value, -360.0, 360.0, 2)
        spin.setSuffix(" deg")
        return spin

    @staticmethod
    def _select_data(combo, value):
        for index in range(combo.count()):
            if str(combo.itemData(index)) == value:
                combo.setCurrentIndex(index)
                return

    def accept(self):
        self._remove_selection_observer()
        doc = self.stair.Document
        doc.recompute()
        gui_doc = FreeCADGui.ActiveDocument
        if self.is_creating:
            gui_doc.commitCommand()
        elif doc.getBookedTransactionID():
            gui_doc.commitCommand()
        if not self.is_creating:
            gui_doc.resetEdit()
        return True

    def reject(self):
        self._remove_selection_observer()
        doc = self.stair.Document
        gui_doc = FreeCADGui.ActiveDocument
        if self.is_creating:
            gui_doc.abortCommand()
        elif doc.getBookedTransactionID():
            gui_doc.abortCommand()
        doc.recompute()
        if not self.is_creating:
            gui_doc.resetEdit()
        return True

    def __del__(self):
        try:
            self._remove_selection_observer()
        except (AttributeError, RuntimeError):
            pass

    def open(self):
        gui_doc = FreeCADGui.ActiveDocument
        if not self.stair.Document.getBookedTransactionID():
            gui_doc.openCommand(translate("BIM", "Edit Stair"))

    def getStandardButtons(self):
        return QtGui.QDialogButtonBox.Ok | QtGui.QDialogButtonBox.Cancel

    def isAllowedAlterSelection(self):
        return True

    def isAllowedAlterView(self):
        return True
