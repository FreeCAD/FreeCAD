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
#   but WITHOUT ANY WARRANTY; without even the implied warranty                #
#   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                    #
#   See the GNU Lesser General Public License for more details.                #
#                                                                              #
#   You should have received a copy of the GNU Lesser General Public           #
#   License along with FreeCAD. If not, see https://www.gnu.org/licenses       #
#                                                                              #
################################################################################

"""Persistent two-wall relation object for BIM walls.

A ``WallJoint`` stores join settings and links to two walls, solves the active
intersection and cutting planes during recompute, and exposes the resolved
status, wall ends, and global trim planes for editing and inspection.  The
proxy owns document properties and invalidation; the pure geometry lives in
``ArchWallRelation`` and precedence lives in
``ArchWallRelationResolver``.  Keep the proxy module name and persisted
property names stable because FreeCAD restores scripted objects from them.
"""

import FreeCAD

import ArchWallRelation
import ArchWallRelationBase
import ArchWallRelationResolver

translate = FreeCAD.Qt.translate

if FreeCAD.GuiUp:
    from PySide.QtCore import QT_TRANSLATE_NOOP
else:

    def QT_TRANSLATE_NOOP(_context, text):
        return text


class _WallJoint(ArchWallRelationBase.WallRelationProxy):
    """Relation object that solves trims between two walls."""

    relation_type = "WallJoint"
    property_group = "Joint"
    link_properties = ("WallA", "WallB")
    wall_change_properties = (
        "JointType",
        "Enabled",
        "WallA",
        "WallB",
        "ButtTrimmed",
        "TeeStem",
        "EndA",
        "EndB",
        "Priority",
    )
    presentation_properties = ("AutoLabel", "JointType", "WallA", "WallB")

    def __init__(self, obj):
        super().__init__(obj)
        try:
            _WallJoint.setProperties(self, obj)
        finally:
            self._initializing = False

    def setProperties(self, obj):
        super().setProperties(obj)
        if "JointType" not in obj.PropertiesList:
            obj.addProperty(
                "App::PropertyEnumeration",
                "JointType",
                "Joint",
                QT_TRANSLATE_NOOP("App::Property", "The type of wall joint."),
            )
            obj.JointType = list(ArchWallRelation.JOINT_TYPES)
            obj.JointType = "Miter"
        if "WallA" not in obj.PropertiesList:
            obj.addProperty(
                "App::PropertyLink",
                "WallA",
                "Joint",
                QT_TRANSLATE_NOOP("App::Property", "The first wall referenced by this joint."),
            )
        if "WallB" not in obj.PropertiesList:
            obj.addProperty(
                "App::PropertyLink",
                "WallB",
                "Joint",
                QT_TRANSLATE_NOOP("App::Property", "The second wall referenced by this joint."),
            )
        if "ButtTrimmed" not in obj.PropertiesList:
            obj.addProperty(
                "App::PropertyEnumeration",
                "ButtTrimmed",
                "Joint",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Which wall is flush-trimmed when the joint type is Butt.",
                ),
            )
            obj.ButtTrimmed = ["Auto", "WallA", "WallB"]
            obj.ButtTrimmed = "Auto"
        if "TeeStem" not in obj.PropertiesList:
            obj.addProperty(
                "App::PropertyEnumeration",
                "TeeStem",
                "Joint",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Which wall acts as the stem when the joint type is Tee.",
                ),
            )
            obj.TeeStem = ["Auto", "WallA", "WallB"]
            obj.TeeStem = "Auto"
        if "EndA" not in obj.PropertiesList:
            obj.addProperty(
                "App::PropertyEnumeration",
                "EndA",
                "Joint",
                QT_TRANSLATE_NOOP("App::Property", "Which end of WallA is trimmed by this joint."),
            )
            obj.EndA = ["Auto", "Start", "End", "None"]
            obj.EndA = "Auto"
        if "EndB" not in obj.PropertiesList:
            obj.addProperty(
                "App::PropertyEnumeration",
                "EndB",
                "Joint",
                QT_TRANSLATE_NOOP("App::Property", "Which end of WallB is trimmed by this joint."),
            )
            obj.EndB = ["Auto", "Start", "End", "None"]
            obj.EndB = "Auto"
        if "Status" not in obj.PropertiesList:
            obj.addProperty(
                "App::PropertyEnumeration",
                "Status",
                "Joint",
                QT_TRANSLATE_NOOP("App::Property", "The current solve status of this wall joint."),
            )
            obj.Status = list(ArchWallRelation.RELATION_STATUSES)
            obj.Status = "MissingWall"
            obj.setEditorMode("Status", 1)
        if "StatusMessage" not in obj.PropertiesList:
            obj.addProperty(
                "App::PropertyString",
                "StatusMessage",
                "Joint",
                QT_TRANSLATE_NOOP("App::Property", "A detailed message about the joint status."),
            )
            obj.setEditorMode("StatusMessage", 1)
        if "ConflictJointA" not in obj.PropertiesList:
            obj.addProperty(
                "App::PropertyLink",
                "ConflictJointA",
                "Joint",
                QT_TRANSLATE_NOOP("App::Property", "Deprecated blocker link for WallA conflicts."),
            )
            obj.setEditorMode("ConflictJointA", 2)
        if "ConflictJointB" not in obj.PropertiesList:
            obj.addProperty(
                "App::PropertyLink",
                "ConflictJointB",
                "Joint",
                QT_TRANSLATE_NOOP("App::Property", "Deprecated blocker link for WallB conflicts."),
            )
            obj.setEditorMode("ConflictJointB", 2)
        if "ConflictJointLabelA" not in obj.PropertiesList:
            obj.addProperty(
                "App::PropertyString",
                "ConflictJointLabelA",
                "Joint",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "The label of the blocking wall joint that conflicts with WallA.",
                ),
            )
            obj.setEditorMode("ConflictJointLabelA", 1)
        if "ConflictJointLabelB" not in obj.PropertiesList:
            obj.addProperty(
                "App::PropertyString",
                "ConflictJointLabelB",
                "Joint",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "The label of the blocking wall joint that conflicts with WallB.",
                ),
            )
            obj.setEditorMode("ConflictJointLabelB", 1)
        if "ConflictMessageA" not in obj.PropertiesList:
            obj.addProperty(
                "App::PropertyString",
                "ConflictMessageA",
                "Joint",
                QT_TRANSLATE_NOOP(
                    "App::Property", "Details about the wall-joint conflict on WallA."
                ),
            )
            obj.setEditorMode("ConflictMessageA", 1)
        if "ConflictMessageB" not in obj.PropertiesList:
            obj.addProperty(
                "App::PropertyString",
                "ConflictMessageB",
                "Joint",
                QT_TRANSLATE_NOOP(
                    "App::Property", "Details about the wall-joint conflict on WallB."
                ),
            )
            obj.setEditorMode("ConflictMessageB", 1)
        if "Intersection" not in obj.PropertiesList:
            obj.addProperty(
                "App::PropertyVector",
                "Intersection",
                "Joint",
                QT_TRANSLATE_NOOP("App::Property", "The resolved baseline intersection point."),
            )
            obj.setEditorMode("Intersection", 1)
        if "ResolvedEndA" not in obj.PropertiesList:
            obj.addProperty(
                "App::PropertyEnumeration",
                "ResolvedEndA",
                "Joint",
                QT_TRANSLATE_NOOP("App::Property", "The resolved wall end used on WallA."),
            )
            obj.ResolvedEndA = ["None", "Start", "End"]
            obj.ResolvedEndA = "None"
            obj.setEditorMode("ResolvedEndA", 1)
        if "ResolvedEndB" not in obj.PropertiesList:
            obj.addProperty(
                "App::PropertyEnumeration",
                "ResolvedEndB",
                "Joint",
                QT_TRANSLATE_NOOP("App::Property", "The resolved wall end used on WallB."),
            )
            obj.ResolvedEndB = ["None", "Start", "End"]
            obj.ResolvedEndB = "None"
            obj.setEditorMode("ResolvedEndB", 1)
        if "ResolvedPlaneA" not in obj.PropertiesList:
            obj.addProperty(
                "App::PropertyPlacement",
                "ResolvedPlaneA",
                "Joint",
                QT_TRANSLATE_NOOP("App::Property", "The resolved global cutting plane for WallA."),
            )
            obj.setEditorMode("ResolvedPlaneA", 1)
        if "ResolvedPlaneB" not in obj.PropertiesList:
            obj.addProperty(
                "App::PropertyPlacement",
                "ResolvedPlaneB",
                "Joint",
                QT_TRANSLATE_NOOP("App::Property", "The resolved global cutting plane for WallB."),
            )
            obj.setEditorMode("ResolvedPlaneB", 1)
        for prop in (
            "Status",
            "StatusMessage",
            "ConflictJointA",
            "ConflictJointB",
            "ConflictJointLabelA",
            "ConflictJointLabelB",
            "ConflictMessageA",
            "ConflictMessageB",
            "Intersection",
            "ResolvedEndA",
            "ResolvedEndB",
            "ResolvedPlaneA",
            "ResolvedPlaneB",
        ):
            if prop in obj.PropertiesList:
                obj.setPropertyStatus(prop, "Output")
                obj.setPropertyStatus(prop, "NoRecompute")
        self._update_editor_modes(obj)

    def execute(self, obj):
        """Solve the relation and publish derived output properties.

        This method does not directly modify either wall shape.  The linked
        walls collect the winning relation trim during their own recompute.
        """
        solution = ArchWallRelationResolver.solve_wall_relation(obj)
        obj.Status = solution.status
        obj.StatusMessage = solution.status_message
        obj.ConflictJointA = None
        obj.ConflictJointB = None
        obj.ConflictJointLabelA = solution.conflict_joint_label_a
        obj.ConflictJointLabelB = solution.conflict_joint_label_b
        obj.ConflictMessageA = solution.conflict_message_a
        obj.ConflictMessageB = solution.conflict_message_b
        obj.Intersection = solution.intersection
        obj.ResolvedEndA = solution.resolved_end_a if solution.resolved_end_a else "None"
        obj.ResolvedEndB = solution.resolved_end_b if solution.resolved_end_b else "None"
        obj.ResolvedPlaneA = solution.plane_a if solution.plane_a else FreeCAD.Placement()
        obj.ResolvedPlaneB = solution.plane_b if solution.plane_b else FreeCAD.Placement()
        self.updatePresentation(obj)

    def updatePresentation(self, obj, force_label=False):
        self._update_editor_modes(obj)
        self._update_label(obj, force=force_label)

    @staticmethod
    def _update_editor_modes(obj):
        joint_type = obj.JointType
        obj.setEditorMode("ButtTrimmed", 0 if joint_type == "Butt" else 2)
        obj.setEditorMode("TeeStem", 0 if joint_type == "Tee" else 2)

    def _update_label(self, obj, force=False):
        if not force and not obj.AutoLabel:
            return
        label = self._get_auto_label(obj)
        if obj.Label != label:
            obj.Label = label

    @staticmethod
    def _get_auto_label(obj):
        joint_type = obj.JointType
        wall_a, wall_b = _WallJoint._linked_walls(obj)
        if wall_a is not None and wall_b is not None:
            return f"{joint_type}: {wall_a.Label} <-> {wall_b.Label}"
        return f"{joint_type} {translate('Arch', 'Wall Joint')}"

    @staticmethod
    def _linked_walls(obj):
        return [obj.WallA, obj.WallB]


class _ViewProviderWallJoint:
    """Minimal view provider for the wall joint relation object."""

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
        joint_type = getattr(self.Object, "JointType", "Miter")
        return f":/icons/BIM_Join_{joint_type}.svg"

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
                [("Miter", "Miter"), ("Butt", "Butt"), ("Tee", "Tee")],
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
            changed = [prop for prop, value in values.items() if getattr(self.obj, prop) != value]
            if not changed:
                self._reset_edit_mode()
                return True

            doc = self.obj.Document
            doc.openTransaction(translate("BIM", "Edit wall joint"))
            try:
                for prop, value in values.items():
                    setattr(self.obj, prop, value)
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
            self._set_combo_value(
                self.butt_trimmed_combo,
                "ButtTrimmed",
                getattr(self.obj, "ButtTrimmed", "Auto"),
            )
            self._set_combo_value(
                self.tee_stem_combo, "TeeStem", getattr(self.obj, "TeeStem", "Auto")
            )
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
            if solution.status == "UnsupportedWall":
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
            butt_enabled = joint_type == "Butt"
            tee_enabled = joint_type == "Tee"
            self._set_row_visible("ButtTrimmed", butt_enabled)
            self._set_row_visible("TeeStem", tee_enabled)

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
            combo = getattr(self, self._combo_attr_name(key))
            combo.setVisible(visible)

        @staticmethod
        def _combo_attr_name(key):
            return {
                "JointType": "joint_type_combo",
                "EndA": "end_a_combo",
                "EndB": "end_b_combo",
                "ButtTrimmed": "butt_trimmed_combo",
                "TeeStem": "tee_stem_combo",
            }[key]

        def _get_wall_summary_text(self):
            wall_a = getattr(self.obj, "WallA", None)
            wall_b = getattr(self.obj, "WallB", None)
            label_a = wall_a.Label if wall_a is not None else translate("BIM", "Unassigned")
            label_b = wall_b.Label if wall_b is not None else translate("BIM", "Unassigned")
            return translate("BIM", "Wall 1: {0}\nWall 2: {1}").format(label_a, label_b)

        def _get_end_label(self, wall_key):
            wall = getattr(self.obj, "Wall" + wall_key, None)
            label = wall.Label if wall else translate("BIM", "Unassigned")
            return translate("BIM", "End on {0}").format(label)

        def _get_wall_choice_label(self, wall_key):
            wall = getattr(self.obj, "Wall" + wall_key, None)
            label = wall.Label if wall else translate("BIM", "Unassigned")
            wall_number = "1" if wall_key == "A" else "2"
            return translate("BIM", "Wall {0} ({1})").format(wall_number, label)

        @staticmethod
        def _reset_edit_mode():
            if FreeCADGui.ActiveDocument and FreeCADGui.ActiveDocument.getInEdit():
                FreeCADGui.ActiveDocument.resetEdit()
