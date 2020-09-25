# ***************************************************************************
# *   Copyright (c) 2014 Yorik van Havre <yorik@uncreated.net>              *
# *   Copyright (c) 2020 Eliud Cabrera Castillo <e.cabrera-castillo@tum.de> *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
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
"""Provides the object code for the Layer object."""
## @package layer
# \ingroup draftobjects
# \brief Provides the object code for the Layer object.

## \addtogroup draftobjects
# @{
from PySide.QtCore import QT_TRANSLATE_NOOP


class Layer:
    """The Layer object.

    This class is normally used to extend a base `App::FeaturePython` object.
    """

    def __init__(self, obj):
        self.Type = "Layer"
        self.Object = obj
        self.set_properties(obj)

        obj.Proxy = self

    def onDocumentRestored(self, obj):
        """Execute code when the document is restored.

        Add properties that don't exist.
        """
        self.set_properties(obj)

    def set_properties(self, obj):
        """Set properties only if they don't exist."""
        if "Group" not in obj.PropertiesList:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                                     "The objects that are part of this layer")
            obj.addProperty("App::PropertyLinkList",
                            "Group",
                            "Layer",
                            _tip)

    def __getstate__(self):
        """Return a tuple of objects to save or None."""
        return self.Type

    def __setstate__(self, state):
        """Set the internal properties from the restored state."""
        if state:
            self.Type = state

    def execute(self, obj):
        """Execute when the object is created or recomputed. Do nothing."""
        pass

    def addObject(self, obj, child):
        """Add an object to this object if not in the Group property."""
        group = obj.Group
        if child not in group:
            group.append(child)
        obj.Group = group


# Alias for compatibility with v0.18 and earlier
_VisGroup = Layer


class LayerContainer:
    """The container object for layers.

    This class is normally used to extend
    a base `App::DocumentObjectGroupPython` object.
    """

    def __init__(self, obj):
        self.Type = "LayerContainer"
        obj.Proxy = self

    def execute(self, obj):
        """Execute when the object is created or recomputed.

        Update the value of `Group` by sorting the contained layers
        by `Label`.
        """
        group = obj.Group
        group.sort(key=lambda layer: layer.Label)
        obj.Group = group

    def __getstate__(self):
        """Return a tuple of objects to save or None."""
        if hasattr(self, "Type"):
            return self.Type

    def __setstate__(self, state):
        """Set the internal properties from the restored state."""
        if state:
            self.Type = state

## @}
