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

"""Persistent multi-wall relation object for BIM walls.

A ``WallJunction`` stores a cluster of three or more walls and either chooses
or links one carrier wall.  It solves direct global trim planes for branch
walls that terminate at one common junction point.  The proxy publishes
derived output properties but does not own wall geometry; linked walls consume
winning claims during recompute.  Keep this proxy module name and its
persisted property names stable for document restoration.
"""

import FreeCAD

import ArchWallRelation
import ArchWallRelationBase
import ArchWallRelationResolver

translate = FreeCAD.Qt.translate

if FreeCAD.GuiUp:
    from PySide.QtCore import QT_TRANSLATE_NOOP
else:

    def QT_TRANSLATE_NOOP(ctxt, txt):
        return txt


class _WallJunction(ArchWallRelationBase.WallRelationProxy):
    """Relation object that manages direct trims across 3+ wall intersections."""

    relation_type = "WallJunction"
    property_group = "Junction"
    link_properties = ("Walls", "CarrierWall")
    wall_change_properties = ("Enabled", "Walls", "CarrierMode", "CarrierWall", "Priority")
    presentation_properties = ("AutoLabel", "Walls", "CarrierMode", "CarrierWall")

    def __init__(self, obj):
        super().__init__(obj)
        try:
            _WallJunction.setProperties(self, obj)
        finally:
            self._initializing = False

    def setProperties(self, obj):
        super().setProperties(obj)
        if "Walls" not in obj.PropertiesList:
            obj.addProperty(
                "App::PropertyLinkList",
                "Walls",
                "Junction",
                QT_TRANSLATE_NOOP("App::Property", "The walls referenced by this junction."),
            )
        if "CarrierMode" not in obj.PropertiesList:
            obj.addProperty(
                "App::PropertyEnumeration",
                "CarrierMode",
                "Junction",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Selects whether the junction carrier wall is detected automatically or chosen explicitly.",
                ),
            )
            obj.CarrierMode = ["Auto", "Explicit"]
            obj.CarrierMode = "Auto"
        if "CarrierWall" not in obj.PropertiesList:
            obj.addProperty(
                "App::PropertyLink",
                "CarrierWall",
                "Junction",
                QT_TRANSLATE_NOOP("App::Property", "The explicit carrier wall for this junction."),
            )
        if "Status" not in obj.PropertiesList:
            obj.addProperty(
                "App::PropertyEnumeration",
                "Status",
                "Junction",
                QT_TRANSLATE_NOOP(
                    "App::Property", "The current solve status of this wall junction."
                ),
            )
            obj.Status = list(ArchWallRelation.RELATION_STATUSES)
            obj.Status = "MissingWall"
            obj.setEditorMode("Status", 1)
        if "StatusMessage" not in obj.PropertiesList:
            obj.addProperty(
                "App::PropertyString",
                "StatusMessage",
                "Junction",
                QT_TRANSLATE_NOOP("App::Property", "A detailed message about the junction status."),
            )
            obj.setEditorMode("StatusMessage", 1)
        if "Intersection" not in obj.PropertiesList:
            obj.addProperty(
                "App::PropertyVector",
                "Intersection",
                "Junction",
                QT_TRANSLATE_NOOP("App::Property", "The resolved junction intersection point."),
            )
            obj.setEditorMode("Intersection", 1)
        if "ResolvedCarrierWall" not in obj.PropertiesList:
            obj.addProperty(
                "App::PropertyLink",
                "ResolvedCarrierWall",
                "Junction",
                QT_TRANSLATE_NOOP("App::Property", "The resolved carrier wall of this junction."),
            )
            obj.setEditorMode("ResolvedCarrierWall", 1)
        if "ResolvedBranchWalls" not in obj.PropertiesList:
            obj.addProperty(
                "App::PropertyLinkList",
                "ResolvedBranchWalls",
                "Junction",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "The resolved branch walls trimmed by this wall junction.",
                ),
            )
            obj.setEditorMode("ResolvedBranchWalls", 1)
        if "ConflictWalls" not in obj.PropertiesList:
            obj.addProperty(
                "App::PropertyLinkList",
                "ConflictWalls",
                "Junction",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "The walls whose trimmed ends are blocked by older relations.",
                ),
            )
            obj.setEditorMode("ConflictWalls", 1)
        if "ConflictRelationLabels" not in obj.PropertiesList:
            obj.addProperty(
                "App::PropertyStringList",
                "ConflictRelationLabels",
                "Junction",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "The labels of the blocking relations for this wall junction.",
                ),
            )
            obj.setEditorMode("ConflictRelationLabels", 1)
        if "ConflictMessages" not in obj.PropertiesList:
            obj.addProperty(
                "App::PropertyStringList",
                "ConflictMessages",
                "Junction",
                QT_TRANSLATE_NOOP(
                    "App::Property", "Details about the blocking relation conflicts."
                ),
            )
            obj.setEditorMode("ConflictMessages", 1)
        for prop in (
            "Status",
            "StatusMessage",
            "Intersection",
            "ResolvedCarrierWall",
            "ResolvedBranchWalls",
            "ConflictWalls",
            "ConflictRelationLabels",
            "ConflictMessages",
        ):
            if prop in obj.PropertiesList:
                obj.setPropertyStatus(prop, "Output")
                obj.setPropertyStatus(prop, "NoRecompute")
        self._update_editor_modes(obj)

    def execute(self, obj):
        """Solve the junction and publish derived output properties."""
        solution = ArchWallRelationResolver.solve_wall_relation(obj)
        obj.Status = solution.status
        obj.StatusMessage = solution.status_message
        obj.Intersection = solution.intersection
        obj.ResolvedCarrierWall = solution.carrier_wall
        obj.ResolvedBranchWalls = (
            solution.branch_walls if solution.status in ("OK", "Conflict") else []
        )
        obj.ConflictWalls = list(solution.conflict_walls)
        obj.ConflictRelationLabels = list(solution.conflict_relation_labels)
        obj.ConflictMessages = list(solution.conflict_messages)
        self.updatePresentation(obj)

    @staticmethod
    def _update_editor_modes(obj):
        obj.setEditorMode("CarrierWall", 0 if obj.CarrierMode == "Explicit" else 2)

    def updatePresentation(self, obj, force_label=False):
        self._update_editor_modes(obj)
        self._update_label(obj, force=force_label)

    def _update_label(self, obj, force=False):
        if not force and not obj.AutoLabel:
            return
        label = self._get_auto_label(obj)
        if obj.Label != label:
            obj.Label = label

    @staticmethod
    def _get_auto_label(obj):
        walls = [wall.Label for wall in _WallJunction._linked_walls(obj) if wall]
        if walls:
            return "Junction: " + ", ".join(walls)
        return translate("Arch", "Wall Junction")

    @staticmethod
    def _linked_walls(obj):
        return list(obj.Walls)


class _ViewProviderWallJunction:
    """Minimal view provider for the wall junction relation object."""

    def __init__(self, vobj):
        vobj.Proxy = self

    def attach(self, _vobj):
        return

    def dumps(self):
        return None

    def loads(self, _state):
        return None

    def updateData(self, _obj, _prop):
        return

    def onChanged(self, _vobj, _prop):
        return

    def getIcon(self):
        return ":/icons/BIM_Join_Tee.svg"

    def doubleClicked(self, _vobj):
        return False

    def getDisplayModes(self, _obj):
        return []

    def getDefaultDisplayMode(self):
        return "Flat Lines"

    def setDisplayMode(self, mode):
        return mode
