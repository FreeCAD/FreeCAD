# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2020 sliptonic <shopinthewoods@gmail.com>               *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************

from PySide.QtCore import QT_TRANSLATE_NOOP
import FreeCAD
import Path
import re

__title__ = "Generic property container to store some values."
__author__ = "sliptonic (Brad Collette)"
__url__ = "https://www.freecad.org"
__doc__ = "A generic container for typed properties in arbitrary categories."

if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


translate = FreeCAD.Qt.translate


SupportedPropertyType = {
    "Angle": "App::PropertyAngle",
    "Bool": "App::PropertyBool",
    "Distance": "App::PropertyDistance",
    "Enumeration": "App::PropertyEnumeration",
    "File": "App::PropertyFile",
    "Float": "App::PropertyFloat",
    "Integer": "App::PropertyInteger",
    "Length": "App::PropertyLength",
    "Percent": "App::PropertyPercent",
    "String": "App::PropertyString",
}


def getPropertyTypeName(o):
    for typ in SupportedPropertyType:
        if SupportedPropertyType[typ] == o:
            return typ
    raise IndexError()


class PropertyBag(object):
    """Property container object."""

    CustomPropertyGroups = "CustomPropertyGroups"
    CustomPropertyGroupDefault = "User"

    def __init__(self, obj):
        obj.addProperty(
            "App::PropertyStringList",
            self.CustomPropertyGroups,
            "Base",
            QT_TRANSLATE_NOOP("App::Property", "List of custom property groups"),
        )
        self.onDocumentRestored(obj)

    def dumps(self):
        return None

    def loads(self, state):
        return None

    def __sanitizePropertyName(self, name):
        if len(name) == 0:
            return
        clean = name[0]
        for i in range(1, len(name)):
            if name[i] == " ":
                clean += name[i + 1].upper()
                i += 1
            elif name[i - 1] != " ":
                clean += name[i]
        return clean

    def onDocumentRestored(self, obj):
        self.obj = obj
        obj.setEditorMode(self.CustomPropertyGroups, 2)  # hide

    def getCustomProperties(self):
        """getCustomProperties() ... Return a list of all custom properties created in this container."""
        return [
            p
            for p in self.obj.PropertiesList
            if self.obj.getGroupOfProperty(p) in self.obj.CustomPropertyGroups
        ]

    def addCustomProperty(self, propertyType, name, group=None, desc=None):
        """addCustomProperty(propertyType, name, group=None, desc=None) ... adds a custom property and tracks its group."""
        if desc is None:
            desc = ""
        if group is None:
            group = self.CustomPropertyGroupDefault
        groups = self.obj.CustomPropertyGroups

        name = self.__sanitizePropertyName(name)
        if not re.match("^[A-Za-z0-9_]*$", name):
            raise ValueError("Property Name can only contain letters and numbers")

        if not group in groups:
            groups.append(group)
            self.obj.CustomPropertyGroups = groups
        self.obj.addProperty(propertyType, name, group, desc)
        return name

    def refreshCustomPropertyGroups(self):
        """refreshCustomPropertyGroups() ... removes empty property groups, should be called after deleting properties."""
        customGroups = []
        for p in self.obj.PropertiesList:
            group = self.obj.getGroupOfProperty(p)
            if group in self.obj.CustomPropertyGroups and not group in customGroups:
                customGroups.append(group)
        self.obj.CustomPropertyGroups = customGroups


def Create(name="PropertyBag"):
    obj = FreeCAD.ActiveDocument.addObject("App::FeaturePython", name)
    obj.Proxy = PropertyBag(obj)
    return obj


def IsPropertyBag(obj):
    """Returns True if the supplied object is a property container (or its Proxy)."""

    if type(obj) == PropertyBag:
        return True
    if hasattr(obj, "Proxy"):
        return IsPropertyBag(obj.Proxy)
    return False
