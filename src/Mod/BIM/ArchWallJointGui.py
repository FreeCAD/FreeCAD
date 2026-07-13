# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 FreeCAD Project Association
# SPDX-FileNotice: Part of the FreeCAD project.
################################################################################
#                                                                              #
#   FreeCAD is free software: you can redistribute it and/or modify            #
#   it under the terms of the GNU Lesser General Public License as             #
#   published by the Free Software Foundation, either version 2.1              #
#   of the License, or (at your option) any later version.                     #
#                                                                              #
#   FreeCAD is distributed in the hope that it will be useful,                 #
#   but WITHOUT ANY WARRANTY; without even the implied warranty of             #
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the               #
#   GNU Lesser General Public License for more details.                         #
#                                                                              #
#   You should have received a copy of the GNU Lesser General Public           #
#   License along with FreeCAD. If not, see https://www.gnu.org/licenses       #
#                                                                              #
################################################################################

"""GUI view provider and task panel for the two-wall relation object."""

import FreeCAD

import ArchWallRelation
import ArchWallRelationResolver

translate = FreeCAD.Qt.translate

if FreeCAD.GuiUp:
    import FreeCADGui
    from PySide import QtCore, QtGui
else:
    FreeCADGui = None


class _ViewProviderWallJoint:
    """View provider for the wall joint relation object."""

    def __init__(self, vobj):
        vobj.Proxy = self
        self.Object = vobj.Object

    def attach(self, vobj):
        self.Object = vobj.Object

    def dumps(self):
        return None

    def loads(self, _state):
        return None

    def updateData(self, _obj, _prop):
        return

    def onChanged(self, _vobj, _prop):
        return

    def getIcon(self):
        return f":/icons/BIM_Join_{self.Object.JointType}.svg"

    def setEdit(self, vobj, mode):
        if not FreeCAD.GuiUp or mode != 0:
            return None

        self.taskd = WallJointTaskPanel(vobj.Object)
        FreeCADGui.Control.showDialog(self.taskd)
        self.taskd.joint_type_combo.setFocus()
        return True

    def unsetEdit(self, _vobj, mode):
        if not FreeCAD.GuiUp or mode != 0:
            return None

        FreeCADGui.Control.closeDialog()
        return True

    def doubleClicked(self, _vobj):
        if not FreeCAD.GuiUp:
            return False
        FreeCADGui.ActiveDocument.setEdit(self.Object, 0)
        return True

    def setupContextMenu(self, _vobj, menu):
        if not FreeCAD.GuiUp or FreeCADGui.activeWorkbench().name() != "BIMWorkbench":
            return
        action_edit = QtGui.QAction(translate("BIM", "Edit Joint"), menu)
        action_edit.triggered.connect(self.edit)
        menu.addAction(action_edit)

    def edit(self):
        if FreeCAD.GuiUp:
            FreeCADGui.ActiveDocument.setEdit(self.Object, 0)


if FreeCAD.GuiUp:

    class WallJointTaskPanel:
        """Task panel for editing a wall joint relation."""

        def __init__(self, obj):
            self.obj = obj
            self._combo_values = {}
            self.form = QtGui.QWidget()
            self.form.setWindowTitle(translate("BIM", "Edit Wall Joint"))

            layout = QtGui.QVBoxLayout(self.form)

            summary = QtGui.QLabel(
                translate("BIM", "Adjust how the selected walls meet before applying."),
                self.form,
            )
            summary.setWordWrap(True)
            layout.addWidget(summary)

            wall_summary = QtGui.QLabel(
                self._get_wall_summary_text(),
                self.form,
            )
            wall_summary.setWordWrap(True)
            layout.addWidget(wall_summary)

            self.form_layout = QtGui.QFormLayout()
            layout.addLayout(self.form_layout)

            self.joint_type_combo = QtGui.QComboBox(self.form)
            self._add_combo_row(
                "JointType",
                translate("BIM", "Joint type"),
                self.joint_type_combo,
            )
            self._set_combo_items(
                self.joint_type_combo,
                "JointType",
                [(value, value) for value in ArchWallRelation.JOINT_TYPES],
            )

            self.end_a_combo = QtGui.QComboBox(self.form)
            self._add_combo_row(
                "EndA",
                self._get_end_label("A"),
                self.end_a_combo,
            )
            self._set_combo_items(
                self.end_a_combo,
                "EndA",
                [
                    ("Auto", "Auto"),
                    ("Start", "Start"),
                    ("End", "End"),
                    ("None", "None"),
                ],
            )

            self.end_b_combo = QtGui.QComboBox(self.form)
            self._add_combo_row(
                "EndB",
                self._get_end_label("B"),
                self.end_b_combo,
            )
            self._set_combo_items(
                self.end_b_combo,
                "EndB",
                [
                    ("Auto", "Auto"),
                    ("Start", "Start"),
                    ("End", "End"),
                    ("None", "None"),
                ],
            )

            self.butt_trimmed_combo = QtGui.QComboBox(self.form)
            self._add_combo_row(
                "ButtTrimmed",
                translate("BIM", "Trimmed wall"),
                self.butt_trimmed_combo,
            )
            self._set_combo_items(
                self.butt_trimmed_combo,
                "ButtTrimmed",
                [
                    ("Auto", translate("BIM", "Auto")),
                    ("WallA", self._get_wall_choice_label("A")),
                    ("WallB", self._get_wall_choice_label("B")),
                ],
            )

            self.tee_stem_combo = QtGui.QComboBox(self.form)
            self._add_combo_row(
                "TeeStem",
                translate("BIM", "Stem wall"),
                self.tee_stem_combo,
            )
            self._set_combo_items(
                self.tee_stem_combo,
                "TeeStem",
                [
                    ("Auto", translate("BIM", "Auto")),
                    ("WallA", self._get_wall_choice_label("A")),
                    ("WallB", self._get_wall_choice_label("B")),
                ],
            )

            self.preview_group = QtGui.QWidget(self.form)
            preview_layout = QtGui.QVBoxLayout(self.preview_group)
            preview_layout.setContentsMargins(0, 0, 0, 0)
            self.preview_title = QtGui.QLabel(self.preview_group)
            title_font = self.preview_title.font()
            title_font.setBold(True)
            self.preview_title.setFont(title_font)
            self.preview_title.setTextInteractionFlags(QtCore.Qt.TextSelectableByMouse)
            self.preview_message = QtGui.QLabel(self.preview_group)
            self.preview_message.setWordWrap(True)
            self.preview_message.setTextInteractionFlags(QtCore.Qt.TextSelectableByMouse)
            preview_layout.addWidget(self.preview_title)
            preview_layout.addWidget(self.preview_message)
            layout.addWidget(self.preview_group)

            self._connect_preview_updates()
            self._load_from_object()
            self._refresh_preview()

        def getStandardButtons(self):
            return QtGui.QDialogButtonBox.Ok | QtGui.QDialogButtonBox.Cancel

        def accept(self):
            values = self._current_values()
            if (
                self.obj.JointType == values["JointType"]
                and self.obj.EndA == values["EndA"]
                and self.obj.EndB == values["EndB"]
                and self.obj.ButtTrimmed == values["ButtTrimmed"]
                and self.obj.TeeStem == values["TeeStem"]
            ):
                self._reset_edit_mode()
                return True

            doc = self.obj.Document
            doc.openTransaction(translate("BIM", "Edit wall joint"))
            try:
                self.obj.JointType = values["JointType"]
                self.obj.EndA = values["EndA"]
                self.obj.EndB = values["EndB"]
                self.obj.ButtTrimmed = values["ButtTrimmed"]
                self.obj.TeeStem = values["TeeStem"]
                doc.commitTransaction()
            except Exception:
                doc.abortTransaction()
                raise
            doc.recompute()
            self._reset_edit_mode()
            return True

        def reject(self):
            self._reset_edit_mode()
            return True

        def _connect_preview_updates(self):
            for combo in (
                self.joint_type_combo,
                self.end_a_combo,
                self.end_b_combo,
                self.butt_trimmed_combo,
                self.tee_stem_combo,
            ):
                combo.currentIndexChanged.connect(self._refresh_preview)

        def _load_from_object(self):
            self._set_combo_value(self.joint_type_combo, "JointType", self.obj.JointType)
            self._set_combo_value(self.end_a_combo, "EndA", self.obj.EndA)
            self._set_combo_value(self.end_b_combo, "EndB", self.obj.EndB)
            self._set_combo_value(self.butt_trimmed_combo, "ButtTrimmed", self.obj.ButtTrimmed)
            self._set_combo_value(self.tee_stem_combo, "TeeStem", self.obj.TeeStem)
            self._update_editor_state()

        def _refresh_preview(self):
            self._update_editor_state()
            values = self._current_values()
            solution = ArchWallRelation.solve_wall_joint_settings(
                self.obj,
                values["JointType"],
                values["ButtTrimmed"],
                values["TeeStem"],
                values["EndA"],
                values["EndB"],
            )
            ArchWallRelationResolver.apply_relation_conflicts(self.obj, solution)
            title = self._get_preview_title(solution)
            message = self._get_preview_message(solution)
            self.preview_title.setText(title)
            has_message = bool(message)
            self.preview_group.setVisible(solution.status != "OK" or has_message)
            self.preview_message.setVisible(has_message)
            self.preview_message.setText(message)

        @staticmethod
        def _get_preview_title(solution):
            if solution.status == "RequiresExtension":
                return translate("BIM", "Cannot create this join")
            if solution.status == "MissingWall":
                return translate("BIM", "Missing wall")
            if solution.status == "UnsupportedBaseline":
                return translate("BIM", "Unsupported wall")
            if solution.status == "Conflict":
                return translate("BIM", "Conflict")
            return solution.status

        @staticmethod
        def _get_preview_message(solution):
            if solution.status == "RequiresExtension":
                return translate(
                    "BIM",
                    "The walls would need to be extended to meet. Wall extension is not supported yet.",
                )
            if solution.status_message == solution.status:
                return ""
            return solution.status_message

        def _update_editor_state(self):
            joint_type = self._get_combo_value(self.joint_type_combo, "JointType")
            self._set_row_visible("ButtTrimmed", joint_type == "Butt")
            self._set_row_visible("TeeStem", joint_type == "Tee")

        def _current_values(self):
            return {
                "JointType": self._get_combo_value(self.joint_type_combo, "JointType"),
                "EndA": self._get_combo_value(self.end_a_combo, "EndA"),
                "EndB": self._get_combo_value(self.end_b_combo, "EndB"),
                "ButtTrimmed": self._get_combo_value(self.butt_trimmed_combo, "ButtTrimmed"),
                "TeeStem": self._get_combo_value(self.tee_stem_combo, "TeeStem"),
            }

        def _add_combo_row(self, key, label_text, widget):
            label = QtGui.QLabel(label_text, self.form)
            self.form_layout.addRow(label, widget)
            self._combo_values[key] = {"label": label, "values": []}

        def _set_combo_items(self, combo, key, items):
            combo.clear()
            self._combo_values[key]["values"] = [value for value, _label in items]
            for _value, label in items:
                combo.addItem(label)

        def _set_combo_value(self, combo, key, value):
            values = self._combo_values[key]["values"]
            try:
                index = values.index(value)
            except ValueError:
                index = 0
            combo.setCurrentIndex(index)

        def _get_combo_value(self, combo, key):
            values = self._combo_values[key]["values"]
            index = combo.currentIndex()
            if index < 0 or index >= len(values):
                return values[0]
            return values[index]

        def _set_row_visible(self, key, visible):
            self._combo_values[key]["label"].setVisible(visible)
            combo = {
                "JointType": self.joint_type_combo,
                "EndA": self.end_a_combo,
                "EndB": self.end_b_combo,
                "ButtTrimmed": self.butt_trimmed_combo,
                "TeeStem": self.tee_stem_combo,
            }[key]
            combo.setVisible(visible)

        def _get_wall(self, wall_key):
            return self.obj.WallA if wall_key == "A" else self.obj.WallB

        def _get_wall_summary_text(self):
            wall_a = self.obj.WallA
            wall_b = self.obj.WallB
            label_a = wall_a.Label if wall_a is not None else translate("BIM", "Unassigned")
            label_b = wall_b.Label if wall_b is not None else translate("BIM", "Unassigned")
            return translate("BIM", "Wall 1: {0}\nWall 2: {1}").format(label_a, label_b)

        def _get_end_label(self, wall_key):
            wall = self._get_wall(wall_key)
            label = wall.Label if wall else translate("BIM", "Unassigned")
            return translate("BIM", "End on {0}").format(label)

        def _get_wall_choice_label(self, wall_key):
            wall = self._get_wall(wall_key)
            label = wall.Label if wall else translate("BIM", "Unassigned")
            wall_number = "1" if wall_key == "A" else "2"
            return translate("BIM", "Wall {0} ({1})").format(wall_number, label)

        @staticmethod
        def _reset_edit_mode():
            if FreeCADGui.ActiveDocument and FreeCADGui.ActiveDocument.getInEdit():
                FreeCADGui.ActiveDocument.resetEdit()
