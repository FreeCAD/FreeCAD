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
