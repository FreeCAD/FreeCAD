# ***************************************************************************
# *   Copyright (c) 2009, 2010 Yorik van Havre <yorik@uncreated.net>        *
# *   Copyright (c) 2009, 2010 Ken Cline <cline@frii.com>                   *
# *   Copyright (c) 2020 FreeCAD Developers                                 *
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
"""Provides the object code for the Clone object."""
## @package clone
# \ingroup draftobjects
# \brief Provides the object code for the Clone object.

## \addtogroup draftobjects
# @{
from PySide.QtCore import QT_TRANSLATE_NOOP

import FreeCAD as App
import DraftVecUtils
from draftobjects.base import DraftObject
from draftutils import gui_utils
from draftutils.messages import _wrn
from draftutils.translate import translate


class Clone(DraftObject):
    """The Clone object"""

    def __init__(self, obj):
        self.set_properties(obj)
        super().__init__(obj, "Clone")


    def set_properties(self, obj):
        pl = obj.PropertiesList
        if not "Objects" in pl:
            _tip = QT_TRANSLATE_NOOP("App::Property", "The objects included in this clone")
            obj.addProperty("App::PropertyLinkListGlobal", "Objects", "Draft", _tip, locked=True)
        if not "Scale" in pl:
            _tip = QT_TRANSLATE_NOOP("App::Property", "The scale factor of this clone")
            obj.addProperty("App::PropertyVector", "Scale", "Draft", _tip, locked=True)
            obj.Scale = App.Vector(1, 1, 1)
        if not "Fuse" in pl:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                                     "If Clones includes several objects,\n"
                                     "set True for fusion or False for compound")
            obj.addProperty("App::PropertyBool", "Fuse", "Draft", _tip, locked=True)
        if not "ForceCompound" in pl:
            _tip = QT_TRANSLATE_NOOP("App::Property", "Always create a compound")
            obj.addProperty("App::PropertyBool", "ForceCompound", "Draft", _tip, locked=True)

    def onDocumentRestored(self, obj):
        super().onDocumentRestored(obj)
        ref = obj.Objects[0] if obj.Objects else None
        gui_utils.restore_view_object(
            obj, vp_module="view_clone", vp_class="ViewProviderClone", format_ref=ref
        )
        if hasattr(obj, "ForceCompound"):
            return
        self.set_properties(obj)
        _wrn("v1.1, " + obj.Label + ", " + translate("draft", "added 'ForceCompound' property"))

    def join(self, obj, shapes):
        fuse = getattr(obj, "Fuse", False)
        force_compound = getattr(obj, "ForceCompound", False)
        if fuse:
            tmps = []
            for s in shapes:
                tmps += s.Solids
            if not tmps:
                for s in shapes:
                    tmps += s.Faces
                if not tmps:
                    for s in shapes:
                        tmps += s.Edges
            shapes = tmps
        import Part
        if len(shapes) == 1:
            if force_compound:
                return Part.makeCompound([shapes[0]])
            else:
                return shapes[0]
        if fuse:
            try:
                sh = shapes[0].multiFuse(shapes[1:])
                sh = sh.removeSplitter()
            except Exception:
                pass
            else:
                if force_compound:
                    return Part.makeCompound([sh])
                else:
                    return sh
        return Part.makeCompound(shapes)

    def execute(self,obj):
        if self.props_changed_placement_only(obj):
            if hasattr(obj,"positionBySupport"):
                obj.positionBySupport()
            self.props_changed_clear()
            return

        import Part
        pl = obj.Placement
        shapes = []
        if obj.isDerivedFrom("Part::Part2DObject"):
            # if our clone is 2D, make sure all its linked geometry is 2D too
            for o in obj.Objects:
                if not o.getLinkedObject(True).isDerivedFrom("Part::Part2DObject"):
                    App.Console.PrintWarning("Warning 2D Clone "+obj.Name+" contains 3D geometry")
                    return
        for o in obj.Objects:
            sh = Part.getShape(o)
            if not sh.isNull():
                shapes.append(sh)
        if shapes:
            sh = self.join(obj, shapes)
            m = App.Matrix()
            if hasattr(obj,"Scale") and not sh.isNull():
                if not DraftVecUtils.equals(obj.Scale, App.Vector(1, 1, 1)):
                    op = sh.Placement
                    sh.Placement = App.Placement()
                    m.scale(obj.Scale)
                    sh.transformShape(m, False, True)
                    sh.Placement = op
            obj.Shape = sh

        obj.Placement = pl
        if hasattr(obj,"positionBySupport"):
            obj.positionBySupport()
        self.props_changed_clear()

    def onChanged(self, obj, prop):
        self.props_changed_store(prop)

    def getSubVolume(self,obj,placement=None):
        # this allows clones of arch windows to return a subvolume too
        if obj.Objects:
            if hasattr(obj.Objects[0],"Proxy"):
                if hasattr(obj.Objects[0].Proxy, "getSubVolume"):
                    if not placement:
                        # clones must displace the original subvolume too
                        placement = obj.Placement
                    return obj.Objects[0].Proxy.getSubVolume(obj.Objects[0], placement)
        return None


# Alias for compatibility with v0.18 and earlier
_Clone = Clone

## @}
