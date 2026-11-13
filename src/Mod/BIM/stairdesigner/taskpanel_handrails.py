# SPDX-License-Identifier: LGPL-2.1-or-later

"""Handrail task-panel controls and side selection."""

from functools import partial

import FreeCAD
from PySide import QtCore, QtGui

from .object_utils import get_flights

translate = FreeCAD.Qt.translate

from .taskpanel_widgets import (
    _load_task_form,
    _value,
)


class HandrailPanelMixin:
    """Task-panel methods grouped by responsibility."""

    def _make_handrail_panel(self):
        widget = _load_task_form(
            ":/ui/TaskStairDesignerHandrails.ui",
            self,
            (
                "handrail_tree",
                "handrail_height",
                "handrail_offset",
                "handrail_picket_spacing",
                "picket_group",
                "handrail_picket_shape",
                "handrail_picket_width",
                "handrail_picket_thickness",
                "handrail_picket_stringer_penetration",
                "handrail_picket_top_rail_penetration",
                "handrail_post_shape",
                "handrail_post_width",
                "handrail_post_thickness",
                "handrail_post_above",
                "handrail_post_below",
                "handrail_top_rail_shape",
                "handrail_top_rail_width",
                "handrail_top_rail_thickness",
                "handrail_top_rail_penetration",
            ),
        )
        widget.setWindowIcon(QtGui.QIcon(":/icons/Arch_Handrail.svg"))
        self.handrail_tree.setToolTip(
            translate(
                "BIM",
                "Enables a handrail independently on the left or right side of each flight",
            )
        )
        self.handrail_tree.header().setSectionResizeMode(0, QtGui.QHeaderView.ResizeToContents)
        self.handrail_tree.header().setSectionResizeMode(1, QtGui.QHeaderView.Stretch)
        self._populate_handrail_tree()

        self.handrail_height.setValue(_value(self.stair.HandrailHeightAboveNosing))
        self.handrail_offset.setValue(_value(self.stair.HandrailOffset))
        self.handrail_picket_spacing.setValue(_value(self.stair.HandrailPicketMaximumSpacing))
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
            label = widget.ui.generalForm.labelForField(editor)
            if label is not None:
                label.setToolTip(help_text)
        shape_editors = (
            (self.handrail_picket_shape, str(self.stair.HandrailPicketShape)),
            (self.handrail_post_shape, str(self.stair.HandrailPostShape)),
            (
                self.handrail_top_rail_shape,
                str(self.stair.HandrailTopRailShape),
            ),
        )
        for editor, value in shape_editors:
            editor.addItem(translate("BIM", "Square"), "Square")
            editor.addItem(translate("BIM", "Circular"), "Circular")
            self._select_data(editor, value)
        self.handrail_picket_width.setValue(_value(self.stair.HandrailPicketWidth))
        self.handrail_picket_thickness.setValue(_value(self.stair.HandrailPicketThickness))
        self.handrail_picket_stringer_penetration.setValue(
            _value(self.stair.HandrailPicketStringerPenetration)
        )
        self.handrail_picket_top_rail_penetration.setValue(
            _value(self.stair.HandrailPicketTopRailPenetration)
        )
        picket_help = {
            self.handrail_picket_shape: translate("BIM", "Square or circular picket cross-section"),
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
            label = widget.ui.picketForm.labelForField(editor)
            if label is not None:
                label.setToolTip(help_text)
        self.picket_group.setToolTip(
            translate(
                "BIM",
                "Pickets are distributed evenly using the fewest members that respect the maximum clear spacing",
            )
        )
        self.handrail_post_width.setValue(_value(self.stair.HandrailPostWidth))
        self.handrail_post_thickness.setValue(_value(self.stair.HandrailPostThickness))
        self.handrail_post_above.setValue(_value(self.stair.HandrailPostAboveTopRail))
        self.handrail_post_below.setValue(_value(self.stair.HandrailPostBelowStringer))
        post_help = {
            self.handrail_post_shape: translate("BIM", "Square or circular post cross-section."),
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
            label = widget.ui.postForm.labelForField(editor)
            if label is not None:
                label.setToolTip(help_text)
        self.handrail_top_rail_width.setValue(_value(self.stair.HandrailTopRailWidth))
        self.handrail_top_rail_thickness.setValue(_value(self.stair.HandrailTopRailThickness))
        self.handrail_top_rail_penetration.setValue(
            _value(self.stair.HandrailTopRailPostPenetration)
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
            label = widget.ui.topRailForm.labelForField(editor)
            if label is not None:
                label.setToolTip(help_text)

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
            editor.currentIndexChanged.connect(self._handrail_shape_changed)
        self._update_handrail_shape_editors()
        return widget

    def _populate_handrail_tree(self):
        self.handrail_tree.clear()
        self.handrail_flight_editors = []
        self.handrail_all_editors = {}
        flights = get_flights(self.stair)
        if len(flights) > 1:
            root = QtGui.QTreeWidgetItem(self.handrail_tree)
            root.setText(0, translate("BIM", "All"))
            root.setFirstColumnSpanned(True)
            for side, label in (
                ("Left", translate("BIM", "Left side")),
                ("Right", translate("BIM", "Right side")),
            ):
                states = {bool(getattr(flight, f"{side}HandrailEnabled")) for flight in flights}
                editor = QtGui.QCheckBox()
                editor.setTristate(True)
                editor.setCheckState(
                    QtCore.Qt.PartiallyChecked
                    if len(states) > 1
                    else (QtCore.Qt.Checked if states.pop() else QtCore.Qt.Unchecked)
                )
                child = QtGui.QTreeWidgetItem(root)
                child.setText(0, label)
                self.handrail_tree.setItemWidget(child, 1, editor)
                self.handrail_all_editors[side] = editor
                editor.stateChanged.connect(partial(self._all_handrail_changed, side, editor))
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
                editor.setChecked(bool(getattr(flight, f"{side}HandrailEnabled")))
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
                else (QtCore.Qt.Checked if states.pop() else QtCore.Qt.Unchecked)
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
            circular = str(shape_editor.itemData(shape_editor.currentIndex())) == "Circular"
            thickness_editor.setEnabled(not circular)
