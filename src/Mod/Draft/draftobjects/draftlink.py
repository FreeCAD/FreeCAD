# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2019 Zheng, Lei (realthunder)<realthunder.dev@gmail.com>*
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
"""Provides the base class for Link objects used by other objects.

This class was created by realthunder during the `LinkMerge`
to demonstrate how to use the `App::Link` objects to create
Link aware arrays.
It is used by `draftobject.array` (ortho, polar, circular)
and `draftobject.patharray` to create respective Link arrays.

NOTE: this class is a bit mysterious. We need more documentation
on how the properties are being set, and how the code interacts with
the arrays that use it.
"""
## @package draftlink
# \ingroup draftobjects
# \brief Provides the base class for Link objects used by other objects.

import lazy_loader.lazy_loader as lz
from PySide.QtCore import QT_TRANSLATE_NOOP

import FreeCAD as App
from draftutils import gui_utils
from draftutils.messages import _log

from draftobjects.base import DraftObject

# Delay import of module until first use because it is heavy
Part = lz.LazyLoader("Part", globals(), "Part")
DraftGeomUtils = lz.LazyLoader("DraftGeomUtils", globals(), "DraftGeomUtils")

## \addtogroup draftobjects
# @{


class DraftLink(DraftObject):
    """New class to use the App::Link objects in arrays.

    Introduced by realthunder.
    This is subclassed by `draftobjects.array.Array`
    and by `draftobjects.patharray.PathArray`.
    """

    def __init__(self, obj, tp):
        self.use_link = False if obj else True
        super().__init__(obj, tp)
        if obj:
            self.attach(obj)

    def dumps(self):
        """Return a tuple of all serializable objects or None."""
        return self.__dict__

    def loads(self, state):
        """Set some internal properties for all restored objects."""
        if isinstance(state, dict):
            self.__dict__ = state
        else:
            self.use_link = False
            super().loads(state)

    def attach(self, obj):
        """Set up the properties when the object is attached."""
        if self.use_link:
            obj.addExtension("App::LinkExtensionPython")
            self.linkSetup(obj)

    def canLinkProperties(self, _obj):
        """Link properties.

        TODO: add more explanation. Overrides a C++ method?
        """
        return False

    def linkSetup(self, obj):
        """Set up the link properties on attachment."""
        obj.configLinkProperty("Placement", LinkedObject="Base")

        if not hasattr(obj, "AlwaysSyncPlacement"):
            _tip = QT_TRANSLATE_NOOP(
                "App::Property",
                "Force sync pattern placements even when array elements are expanded",
            )
            obj.addProperty("App::PropertyBool", "AlwaysSyncPlacement", "Draft", _tip, locked=True)

        if hasattr(obj, "ShowElement"):
            # Rename 'ShowElement' property to 'ExpandArray' to avoid conflict
            # with native App::Link
            obj.configLinkProperty("ShowElement")
            showElement = obj.ShowElement

            _tip = QT_TRANSLATE_NOOP("App::Property", "Show the individual array elements")
            obj.addProperty("App::PropertyBool", "ExpandArray", "Draft", _tip, locked=True)

            obj.ExpandArray = showElement
            obj.configLinkProperty(ShowElement="ExpandArray")
            obj.removeProperty("ShowElement")
        else:
            obj.configLinkProperty(ShowElement="ExpandArray")

        if getattr(obj, "ExpandArray", False):
            obj.setPropertyStatus("PlacementList", "Immutable")
        else:
            obj.setPropertyStatus("PlacementList", "-Immutable")

        if not hasattr(obj, "LinkTransform"):
            obj.addProperty("App::PropertyBool", "LinkTransform", " Link", locked=True)

        if not hasattr(obj, "ColoredElements"):
            obj.addProperty("App::PropertyLinkSubHidden", "ColoredElements", " Link", locked=True)
            obj.setPropertyStatus("ColoredElements", "Hidden")

        if not hasattr(obj, "LinkCopyOnChange"):
            obj.addProperty("App::PropertyEnumeration", "LinkCopyOnChange", " Link", locked=True)

        obj.configLinkProperty("LinkCopyOnChange", "LinkTransform", "ColoredElements")

        if not getattr(obj, "Fuse", False):
            obj.setPropertyStatus("Shape", "Transient")

    def getViewProviderName(self, _obj):
        """Override the view provider name."""
        if self.use_link:
            return "Gui::ViewProviderLinkPython"
        return ""

    def migrate_attributes(self, obj):
        """Migrate old attribute names to new names if they exist.

        This is done to comply with Python guidelines or fix small issues
        in older code.
        """
        if hasattr(self, "useLink"):
            # This is only needed for some models created in 0.19
            # while it was in development. Afterwards,
            # all models should use 'use_link' by default
            # and this won't be run.
            self.use_link = bool(self.useLink)
            _log("v0.19, {}, 'useLink' will be migrated to 'use_link'".format(obj.Name))
            del self.useLink

    def onDocumentRestored(self, obj):
        """Execute code when the document in restored."""
        self.migrate_attributes(obj)

        if self.use_link:
            self.linkSetup(obj)
        else:
            obj.setPropertyStatus("Shape", "-Transient")

        if obj.Shape.isNull():
            if getattr(obj, "PlacementList", None):
                self.buildShape(obj, obj.Placement, obj.PlacementList)
            else:
                self.execute(obj)

        super().onDocumentRestored(obj)
        if hasattr(obj, "LinkTransform"):
            gui_utils.restore_view_object(
                obj, vp_module="view_draftlink", vp_class="ViewProviderDraftLink", format=False
            )
        else:
            gui_utils.restore_view_object(
                obj, vp_module="view_array", vp_class="ViewProviderDraftArray"
            )

    def buildShape(self, obj, pl, pls):
        """Build the shape of the link object."""
        if self.use_link:
            if not getattr(obj, "ExpandArray", False) or obj.Count != len(pls):
                obj.setPropertyStatus("PlacementList", "-Immutable")
                obj.PlacementList = pls
                obj.setPropertyStatus("PlacementList", "Immutable")
                obj.Count = len(pls)
            if getattr(obj, "ExpandArray", False) and getattr(obj, "AlwaysSyncPlacement", False):
                for pla, child in zip(pls, obj.ElementList):
                    child.Placement = pla
        else:
            obj.PlacementList = pls
            if obj.Count != len(pls):
                obj.Count = len(pls)

        if obj.Base:
            shape = getattr(obj.Base, "Shape", None)
            if not isinstance(shape, Part.Shape):
                obj.Shape = Part.Shape()
            elif shape.isNull():
                _err_msg = "'{}' cannot build shape " "from '{}'\n".format(
                    obj.Label, obj.Base.Label
                )
                raise RuntimeError(_err_msg)
            else:
                # Resetting the Placement of the copied shape does not work for
                # Part_Vertex and Draft_Point objects, we need to transform:
                place = shape.Placement
                shape = shape.copy()
                shape.transformShape(place.Matrix.inverse())
                base = []
                for i, pla in enumerate(pls):
                    vis = getattr(obj, "VisibilityList", [])
                    if len(vis) > i and not vis[i]:
                        continue

                    base.append(shape.transformed(pla.toMatrix()))

                if getattr(obj, "Fuse", False) and len(base) > 1:
                    obj.Shape = base[0].multiFuse(base[1:]).removeSplitter()
                else:
                    obj.Shape = Part.makeCompound(base)

                if not DraftGeomUtils.isNull(pl):
                    obj.Placement = pl

        if self.use_link:
            return False  # return False to call LinkExtension::execute()

    def onChanged(self, obj, prop):
        """Execute when a property changes."""
        self.props_changed_store(prop)

        if not getattr(self, "use_link", False):
            return

        if prop == "Fuse":
            if obj.Fuse:
                obj.setPropertyStatus("Shape", "-Transient")
            else:
                obj.setPropertyStatus("Shape", "Transient")
        elif prop == "ExpandArray":
            if hasattr(obj, "PlacementList"):
                if obj.ExpandArray:
                    obj.setPropertyStatus("PlacementList", "-Immutable")
                else:
                    obj.setPropertyStatus("PlacementList", "Immutable")

    def getPlacementOf(self, fp, subname, targetObj=None):
        """
        Return the placement of the sub-object relative to the link object.
        """

        # _getShowElementValue is mapped to ExpandArray.
        if getattr(fp, "ExpandArray", False):
            # Return None to fall back to the C++ implementation
            return None

        # We start with the object's own placement.
        plc = fp.Placement

        if not subname:
            return plc

        names = subname.split(".")

        linked_obj = getattr(fp, "Base", None)
        if not names or fp == targetObj or not linked_obj:
            return plc

        doc = linked_obj.Document

        try:
            i = int(names[0])
        except ValueError:
            return None

        # Check bounds in PlacementList
        plcs = fp.PlacementList
        if i < 0 or i >= len(plcs):
            return plc

        # Accumulate the element placement
        plc = plc * plcs[i]

        if len(names) < 2:
            return plc

        subObj = doc.getObject(names[1])
        if not subObj:
            return plc

        newSub = ".".join(names[2:])

        return plc * subObj.getPlacementOf(newSub, targetObj)


# Alias for compatibility with old versions of v0.19
_DraftLink = DraftLink

## @}
