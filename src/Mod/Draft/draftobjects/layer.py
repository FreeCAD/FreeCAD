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

import FreeCAD as App
from draftutils import gui_utils
from draftutils import utils
from draftutils.messages import _wrn
from draftutils.translate import translate


class Layer:
    """The Layer object.

    This class is normally used to extend a base `App::FeaturePython` object.
    """

    def __init__(self, obj):
        self.Type = "Layer"
        self.Object = obj
        self.set_properties(obj)

        obj.Proxy = self

    def set_properties(self, obj):
        """Set properties only if they don't exist."""
        if "Group" not in obj.PropertiesList:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                                     "The objects that are part of this layer")
            # "App::PropertyLinkListHidden" instead of "App::PropertyLinkList" has 2 advantages:
            # 1. No 'might break' warning when deleting an object nested in a layer.
            # 2. No 'out of scope' warning for objects also nested in f.e. a Std_Part.
            obj.addProperty("App::PropertyLinkListHidden",
                            "Group",
                            "Layer",
                            _tip,
                            locked=True)

    def onDocumentRestored(self, obj):
        """Execute code when the document is restored."""
        # Group property type changed in v1.0:
        if obj.getTypeIdOfProperty("Group") != "App::PropertyLinkListHidden":
            grp = obj.Group  # Type: "App::PropertyLinkList".
            group_removed = obj.removeProperty("Group")  # Not possible for VisGroups (< v0.19)
            self.set_properties(obj)
            if group_removed:
                obj.Group = grp
                _wrn("v1.0, " + obj.Label + ", " + translate("draft", "changed 'Group' property type"))

        gui_utils.restore_view_object(
            obj, vp_module="view_layer", vp_class="ViewProviderLayer", format=False
        )

        if not getattr(obj, "ViewObject", None):
            return
        vobj = obj.ViewObject

        if self.Type == "VisGroup":  # Type prior to v0.19.
            self.Type = "Layer"
            # It is not possible to change the property group of obj.Group.
            for prop in ("DrawStyle", "LineColor", "LineWidth", "ShapeColor", "Transparency"):
                vobj.setGroupOfProperty(prop, "Layer")

        if not hasattr(vobj, "ShapeAppearance"):
            vobj.Proxy.set_properties(vobj)
            material = App.Material()  #  Material with default v0.21 properties.
            material.DiffuseColor = vobj.ShapeColor
            material.Transparency = vobj.Transparency / 100
            vobj.ShapeAppearance = (material, )
            vobj.setPropertyStatus("ShapeColor", "Hidden")
            if hasattr(vobj, "OverrideShapeColorChildren"):  # v0.19 - v0.21
                vobj.OverrideShapeAppearanceChildren = vobj.OverrideShapeColorChildren
                vobj.removeProperty("OverrideShapeColorChildren")
            _wrn("v1.0, " + obj.Label + ", " + translate("draft", "updated view properties"))

    def dumps(self):
        """Return a tuple of objects to save or None."""
        return self.Type

    def loads(self, state):
        """Set the internal properties from the restored state."""
        if state:
            self.Type = state

    def execute(self, obj):
        """Execute when the object is created or recomputed. Do nothing."""
        pass

    def _get_other_layers(self, obj, child):
        other_lyrs = []
        for find in child.Document.findObjects(Type="App::FeaturePython"):
            if utils.get_type(find) == "Layer" and find != obj and child in find.Group:
                other_lyrs.append(find)
        return other_lyrs

    def onBeforeChange(self, obj, prop):
        if prop == "Group":
            self.oldGroup = obj.Group

    def onChanged(self, obj, prop):
        if prop != "Group":
            return
        vobj = getattr(obj, "ViewObject", None)
        old_grp = getattr(self, "oldGroup", [])
        for child in obj.Group:
            if child in old_grp:
                continue
            for other_lyr in self._get_other_layers(obj, child):
                other_grp = other_lyr.Group
                other_grp.remove(child)
                other_lyr.Group = other_grp
            if vobj is None:
                continue
            for prop in ("LineColor", "ShapeAppearance", "LineWidth", "DrawStyle", "Visibility"):
                vobj.Proxy.change_view_properties(vobj, prop, old_prop=None, targets=[child])

    def addObject(self, obj, child):
        if utils.get_type(child) in ("Layer", "LayerContainer"):
            return
        grp = obj.Group
        if child in grp:
            return
        grp.append(child)
        obj.Group = grp

    def removeObject(self, obj, child):
        grp = obj.Group
        if not child in grp:
            return
        grp.remove(child)
        obj.Group = grp


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

    def onDocumentRestored(self, obj):
        gui_utils.restore_view_object(
            obj, vp_module="view_layer", vp_class="ViewProviderLayerContainer", format=False
        )

    def execute(self, obj):
        """Execute when the object is created or recomputed.

        Update the value of `Group` by sorting the contained layers
        by `Label`.
        """
        grp = obj.Group
        grp.sort(key=lambda layer: layer.Label)
        obj.Group = grp

    def dumps(self):
        """Return a tuple of objects to save or None."""
        if hasattr(self, "Type"):
            return self.Type

    def loads(self, state):
        """Set the internal properties from the restored state."""
        if state:
            self.Type = state


def get_layer(obj):
    """Get the layer the object belongs to."""
    finds = obj.Document.findObjects(Name="LayerContainer")
    if not finds:
        return None
    # First look in the LayerContainer:
    for layer in finds[0].Group:
        if utils.get_type(layer) == "Layer" and obj in layer.Group:
            return layer
    # If not found, look through all App::FeaturePython objects (not just layers):
    for find in obj.Document.findObjects(Type="App::FeaturePython"):
        if utils.get_type(find) == "Layer" and obj in find.Group:
            return find
    return None

## @}
