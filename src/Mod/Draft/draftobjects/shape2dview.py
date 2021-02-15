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
"""Provides the object code for the Shape2dView object."""
## @package shape2dview
# \ingroup draftobjects
# \brief Provides the object code for the Shape2dView object.

## \addtogroup draftobjects
# @{
from PySide.QtCore import QT_TRANSLATE_NOOP

import FreeCAD as App
import DraftVecUtils
import draftutils.utils as utils
import draftutils.gui_utils as gui_utils
import draftutils.groups as groups

from draftutils.translate import translate
from draftobjects.base import DraftObject


class Shape2DView(DraftObject):
    """The Shape2DView object"""

    def __init__(self,obj):

        _tip = QT_TRANSLATE_NOOP("App::Property",
                "The base object this 2D view must represent")
        obj.addProperty("App::PropertyLink", "Base",
                        "Draft", _tip)

        _tip = QT_TRANSLATE_NOOP("App::Property",
                "The projection vector of this object")
        obj.addProperty("App::PropertyVector", "Projection",
                        "Draft", _tip)

        _tip = QT_TRANSLATE_NOOP("App::Property",
                "The way the viewed object must be projected")
        obj.addProperty("App::PropertyEnumeration", "ProjectionMode",
                        "Draft", _tip)

        _tip = QT_TRANSLATE_NOOP("App::Property",
                "The indices of the faces to be projected in Individual Faces mode")
        obj.addProperty("App::PropertyIntegerList", "FaceNumbers",
                        "Draft", _tip)

        _tip = QT_TRANSLATE_NOOP("App::Property",
                "Show hidden lines")
        obj.addProperty("App::PropertyBool", "HiddenLines",
                        "Draft", _tip)

        _tip = QT_TRANSLATE_NOOP("App::Property",
                "Fuse wall and structure objects of same type and material")
        obj.addProperty("App::PropertyBool", "FuseArch",
                        "Draft", _tip)

        _tip = QT_TRANSLATE_NOOP("App::Property",
                "Tessellate Ellipses and B-splines into line segments")
        obj.addProperty("App::PropertyBool", "Tessellation",
                        "Draft", _tip)

        _tip = QT_TRANSLATE_NOOP("App::Property",
                "For Cutlines and Cutfaces modes, \
                this leaves the faces at the cut location")
        obj.addProperty("App::PropertyBool", "InPlace",
                        "Draft", _tip)

        _tip = QT_TRANSLATE_NOOP("App::Property",
                "Length of line segments if tessellating Ellipses or B-splines \
                into line segments")
        obj.addProperty("App::PropertyFloat", "SegmentLength",
                        "Draft", _tip)

        _tip = QT_TRANSLATE_NOOP("App::Property", 
                "If this is True, this object will be recomputed only if it is \
                visible")
        obj.addProperty("App::PropertyBool", "VisibleOnly",
                        "Draft", _tip)
        
        obj.Projection = App.Vector(0,0,1)
        obj.ProjectionMode = ["Solid", "Individual Faces",
                              "Cutlines", "Cutfaces"]
        obj.HiddenLines = False
        obj.Tessellation = False
        obj.VisibleOnly = False
        obj.InPlace = True
        obj.SegmentLength = .05
        super(Shape2DView, self).__init__(obj, "Shape2DView")

    def getProjected(self,obj,shape,direction):
        "returns projected edges from a shape and a direction"
        import Part, Drawing, DraftGeomUtils
        edges = []
        _groups = Drawing.projectEx(shape, direction)
        for g in _groups[0:5]:
            if g:
                edges.append(g)
        if hasattr(obj,"HiddenLines"):
            if obj.HiddenLines:
                for g in _groups[5:]:
                    edges.append(g)
        #return Part.makeCompound(edges)
        if hasattr(obj,"Tessellation") and obj.Tessellation:
            return DraftGeomUtils.cleanProjection(Part.makeCompound(edges),
                                                  obj.Tessellation,
                                                  obj.SegmentLength)
        else:
            return Part.makeCompound(edges)
            #return DraftGeomUtils.cleanProjection(Part.makeCompound(edges))

    def execute(self,obj):
        if hasattr(obj,"VisibleOnly"):
            if obj.VisibleOnly:
                if obj.ViewObject:
                    if obj.ViewObject.Visibility == False:
                        return False
        import Part, DraftGeomUtils
        obj.positionBySupport()
        pl = obj.Placement
        if obj.Base:
            if utils.get_type(obj.Base) in ["BuildingPart","SectionPlane"]:
                objs = []
                if utils.get_type(obj.Base) == "SectionPlane":
                    objs = obj.Base.Objects
                    cutplane = obj.Base.Shape
                else:
                    objs = obj.Base.Group
                    cutplane = Part.makePlane(1000, 1000, App.Vector(-500, -500, 0))
                    m = 1
                    if obj.Base.ViewObject and hasattr(obj.Base.ViewObject,"CutMargin"):
                        m = obj.Base.ViewObject.CutMargin.Value
                    cutplane.translate(App.Vector(0,0,m))
                    cutplane.Placement = cutplane.Placement.multiply(obj.Base.Placement)
                if objs:
                    onlysolids = True
                    if hasattr(obj.Base,"OnlySolids"):
                        onlysolids = obj.Base.OnlySolids
                    import Arch, Part, Drawing
                    objs = groups.get_group_contents(objs, walls=True)
                    objs = gui_utils.remove_hidden(objs)
                    shapes = []
                    if hasattr(obj,"FuseArch") and obj.FuseArch:
                        shtypes = {}
                        for o in objs:
                            if utils.get_type(o) in ["Wall","Structure"]:
                                if onlysolids:
                                    shtypes.setdefault(o.Material.Name
                                                      if (hasattr(o,"Material") and o.Material) 
                                                      else "None",[]).extend(o.Shape.Solids)
                                else:
                                    shtypes.setdefault(o.Material.Name 
                                                       if (hasattr(o,"Material") and o.Material) 
                                                       else "None",[]).append(o.Shape.copy())
                            elif hasattr(o,'Shape'):
                                if onlysolids:
                                    shapes.extend(o.Shape.Solids)
                                else:
                                    shapes.append(o.Shape.copy())
                        for k, v in shtypes.items():
                            v1 = v.pop()
                            if v:
                                v1 = v1.multiFuse(v)
                                v1 = v1.removeSplitter()
                            if v1.Solids:
                                shapes.extend(v1.Solids)
                            else:
                                print("Shape2DView: Fusing Arch objects produced non-solid results")
                                shapes.append(v1)
                    else:
                        for o in objs:
                            if hasattr(o,'Shape'):
                                if onlysolids:
                                    shapes.extend(o.Shape.Solids)
                                else:
                                    shapes.append(o.Shape.copy())
                    clip = False
                    if hasattr(obj.Base,"Clip"):
                        clip = obj.Base.Clip
                    cutp, cutv, iv = Arch.getCutVolume(cutplane, shapes, clip)
                    cuts = []
                    opl = App.Placement(obj.Base.Placement)
                    proj = opl.Rotation.multVec(App.Vector(0, 0, 1))
                    if obj.ProjectionMode == "Solid":
                        for sh in shapes:
                            if cutv:
                                if sh.Volume < 0:
                                    sh.reverse()
                                #if cutv.BoundBox.intersect(sh.BoundBox):
                                #    c = sh.cut(cutv)
                                #else:
                                #    c = sh.copy()
                                c = sh.cut(cutv)
                                if onlysolids:
                                    cuts.extend(c.Solids)
                                else:
                                    cuts.append(c)
                            else:
                                if onlysolids:
                                    cuts.extend(sh.Solids)
                                else:
                                    cuts.append(sh.copy())
                        comp = Part.makeCompound(cuts)
                        obj.Shape = self.getProjected(obj,comp,proj)
                    elif obj.ProjectionMode in ["Cutlines", "Cutfaces"]:
                        for sh in shapes:
                            if sh.Volume < 0:
                                sh.reverse()
                            c = sh.section(cutp)
                            faces = []
                            if (obj.ProjectionMode == "Cutfaces") and (sh.ShapeType == "Solid"):
                                if hasattr(obj,"InPlace"):
                                    if not obj.InPlace:
                                        c = self.getProjected(obj, c, proj)
                                wires = DraftGeomUtils.findWires(c.Edges)
                                for w in wires:
                                    if w.isClosed():
                                        faces.append(Part.Face(w))
                            if faces:
                                cuts.extend(faces)
                            else:
                                cuts.append(c)
                        comp = Part.makeCompound(cuts)
                        opl = App.Placement(obj.Base.Placement)
                        comp.Placement = opl.inverse()
                        if comp:
                            obj.Shape = comp

            elif obj.Base.isDerivedFrom("App::DocumentObjectGroup"):
                shapes = []
                objs = groups.get_group_contents(obj.Base)
                for o in objs:
                    if hasattr(o,'Shape'):
                        if o.Shape:
                            if not o.Shape.isNull():
                                shapes.append(o.Shape)
                if shapes:
                    import Part
                    comp = Part.makeCompound(shapes)
                    obj.Shape = self.getProjected(obj,comp,obj.Projection)

            elif hasattr(obj.Base,'Shape'):
                if not DraftVecUtils.isNull(obj.Projection):
                    if obj.ProjectionMode == "Solid":
                        obj.Shape = self.getProjected(obj,obj.Base.Shape,obj.Projection)
                    elif obj.ProjectionMode == "Individual Faces":
                        import Part
                        if obj.FaceNumbers:
                            faces = []
                            for i in obj.FaceNumbers:
                                if len(obj.Base.Shape.Faces) > i:
                                    faces.append(obj.Base.Shape.Faces[i])
                            views = []
                            for f in faces:
                                views.append(self.getProjected(obj,f,obj.Projection))
                            if views:
                                obj.Shape = Part.makeCompound(views)
                    else:
                        App.Console.PrintWarning(obj.ProjectionMode+" mode not implemented\n")
        if not DraftGeomUtils.isNull(pl):
            obj.Placement = pl


# Alias for compatibility with v0.18 and earlier
_Shape2DView = Shape2DView

## @}
