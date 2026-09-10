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

"""Shared document-object lifecycle for wall joint and junction proxies.

The concrete proxies own their relation-specific properties and solving, while
this class centralizes the genuinely shared lifecycle: common settings,
document restoration, linked-wall invalidation, relation-priority updates, and
safe output publication.  The initialization guard keeps property callbacks
from observing a partially constructed or restored relation object.
"""

import ArchWallRelation
import FreeCAD

if FreeCAD.GuiUp:
    from PySide.QtCore import QT_TRANSLATE_NOOP
else:

    def QT_TRANSLATE_NOOP(_context, text):
        return text


class WallRelationProxy:
    """Base lifecycle for persistent wall relation proxies."""

    relation_type = "WallRelation"
    property_group = "Relation"
    link_properties = ()
    wall_change_properties = ()
    presentation_properties = ()
    _initializing = True
    _pre_change_walls = ()

    def __init__(self, obj):
        obj.Proxy = self
        self.Type = self.relation_type
        self._pre_change_walls = []
        self._initializing = True

    def setProperties(self, obj):
        """Add the settings shared by all persistent wall relations."""
        if "AutoLabel" not in obj.PropertiesList:
            obj.addProperty(
                "App::PropertyBool",
                "AutoLabel",
                self.property_group,
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Automatically updates the relation label from its linked walls.",
                ),
            )
            obj.AutoLabel = True
        if "Enabled" not in obj.PropertiesList:
            obj.addProperty(
                "App::PropertyBool",
                "Enabled",
                self.property_group,
                QT_TRANSLATE_NOOP("App::Property", "Enables or disables this wall relation."),
            )
            obj.Enabled = True
        if "Priority" not in obj.PropertiesList:
            obj.addProperty(
                "App::PropertyInteger",
                "Priority",
                self.property_group,
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Relation precedence for wall-end ownership. Lower values win.",
                ),
            )
            obj.Priority = ArchWallRelation.get_next_relation_priority(obj.Document, exclude=obj)

    def dumps(self):
        return self.Type

    def loads(self, _state):
        self.Type = self.relation_type
        self._pre_change_walls = []

    def onDocumentRestored(self, obj):
        self.Type = self.relation_type
        self._pre_change_walls = []
        self._initializing = True
        try:
            self.setProperties(obj)
        finally:
            self._initializing = False
        if obj.Priority < 0:
            obj.Priority = ArchWallRelation.get_next_relation_priority(obj.Document, exclude=obj)
        self.updatePresentation(obj, force_label=obj.AutoLabel)

    def onBeforeChange(self, obj, prop):
        if not self._initializing and prop in self.link_properties:
            self._pre_change_walls = self._linked_walls(obj)

    def onChanged(self, obj, prop):
        if self._initializing:
            return
        if prop == "Priority" and obj.Priority < 0:
            obj.Priority = ArchWallRelation.get_next_relation_priority(obj.Document, exclude=obj)
            return
        if prop in self.wall_change_properties:
            walls = self._linked_walls(obj)
            self._touch_walls(self._pre_change_walls + walls)
            self._pre_change_walls = []
            if prop == "Priority":
                self._touch_competing_relations(obj, walls)
        if prop in self.presentation_properties:
            self.updatePresentation(obj, force_label=(prop == "AutoLabel"))

    def onDelete(self, obj, _args):
        self._touch_walls(self._linked_walls(obj))
        return True

    @staticmethod
    def _touch_walls(walls):
        seen = set()
        for wall in walls:
            if wall is None or wall.Name in seen:
                continue
            seen.add(wall.Name)
            wall.touch()

    @staticmethod
    def _touch_competing_relations(obj, walls):
        competing_walls = []
        for wall in walls:
            for relation in ArchWallRelation.iter_wall_relations(wall):
                if relation != obj:
                    relation.touch()
                    competing_walls.extend(ArchWallRelation.get_relation_walls(relation))
        WallRelationProxy._touch_walls(competing_walls)
