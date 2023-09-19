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

        self.setProperties(obj)
        super(Shape2DView, self).__init__(obj, "Shape2DView")

    def setProperties(self,obj):

        pl = obj.PropertiesList

        if not "Base" in pl:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                    "The base object this 2D view must represent")
            obj.addProperty("App::PropertyLink", "Base",
                            "Draft", _tip)
        if not "Projection" in pl:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                    "The projection vector of this object")
            obj.addProperty("App::PropertyVector", "Projection",
                            "Draft", _tip)
            obj.Projection = App.Vector(0,0,1)
        if not "ProjectionMode" in pl:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                    "The way the viewed object must be projected")
            obj.addProperty("App::PropertyEnumeration", "ProjectionMode",
                            "Draft", _tip)
            obj.ProjectionMode = ["Solid", "Individual Faces",
                                  "Cutlines", "Cutfaces","Solid faces"]
        if not "FaceNumbers" in pl:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                    "The indices of the faces to be projected in Individual Faces mode")
            obj.addProperty("App::PropertyIntegerList", "FaceNumbers",
                            "Draft", _tip)
        if not "HiddenLines" in pl:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                    "Show hidden lines")
            obj.addProperty("App::PropertyBool", "HiddenLines",
                            "Draft", _tip)
            obj.HiddenLines = False
        if not "FuseArch" in pl:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                    "Fuse wall and structure objects of same type and material")
            obj.addProperty("App::PropertyBool", "FuseArch",
                            "Draft", _tip)
        if not "Tessellation" in pl:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                    "Tessellate Ellipses and B-splines into line segments")
            obj.addProperty("App::PropertyBool", "Tessellation",
                            "Draft", _tip)
            obj.Tessellation = False
        if not "InPlace" in pl:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                    "For Cutlines and Cutfaces modes, \
                    this leaves the faces at the cut location")
            obj.addProperty("App::PropertyBool", "InPlace",
                            "Draft", _tip)
            obj.InPlace = True
        if not "SegmentLength" in pl:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                    "Length of line segments if tessellating Ellipses or B-splines \
                    into line segments")
            obj.addProperty("App::PropertyFloat", "SegmentLength",
                            "Draft", _tip)
            obj.SegmentLength = .05
        if not "VisibleOnly" in pl:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                    "If this is True, this object will include only visible objects")
            obj.addProperty("App::PropertyBool", "VisibleOnly",
                            "Draft", _tip)
            obj.VisibleOnly = False
        if not "ExclusionPoints" in pl:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                    "A list of exclusion points. Any edge touching any of those points will not be drawn.")
            obj.addProperty("App::PropertyVectorList", "ExclusionPoints",
                            "Draft", _tip)
        if not "ExclusionNames" in pl:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                    "A list of exclusion object names. Any object viewed that matches a name from the list will not be drawn.")
            obj.addProperty("App::PropertyStringList", "ExclusionNames",
                            "Draft", _tip)
        if not "OnlySolids" in pl:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                    "If this is True, only solid geometry is handled. This overrides the base object's Only Solids property")
            obj.addProperty("App::PropertyBool", "OnlySolids",
                            "Draft", _tip)
        if not "Clip" in pl:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                    "If this is True, the contents are clipped to the borders of the section plane, if applicable. This overrides the base object's Clip property")
            obj.addProperty("App::PropertyBool", "Clip",
                            "Draft", _tip)
        if not "AutoUpdate" in pl:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                    "This object will be recomputed only if this is True.")
            obj.addProperty("App::PropertyBool", "AutoUpdate",
                            "Draft", _tip)
            obj.AutoUpdate = True

    def onDocumentRestored(self, obj):

        self.setProperties(obj)

    def getProjected(self,obj,shape,direction):

        "returns projected edges from a shape and a direction"
        import Part
        import TechDraw
        import DraftGeomUtils
        edges = []
        _groups = TechDraw.projectEx(shape, direction)
        for g in _groups[0:5]:
            if g:
                edges.append(g)
        if hasattr(obj,"HiddenLines"):
            if obj.HiddenLines:
                for g in _groups[5:]:
                    edges.append(g)
        edges = self.cleanExcluded(obj,edges)
        #return Part.makeCompound(edges)
        if hasattr(obj,"Tessellation") and obj.Tessellation:
            return DraftGeomUtils.cleanProjection(Part.makeCompound(edges),
                                                  obj.Tessellation,
                                                  obj.SegmentLength)
        else:
            return Part.makeCompound(edges)
            #return DraftGeomUtils.cleanProjection(Part.makeCompound(edges))

    def cleanExcluded(self,obj,shapes):

        """removes any edge touching exclusion points"""
        import Part
        MAXDIST = 0.0001
        if (not hasattr(obj,"ExclusionPoints")) or (not obj.ExclusionPoints):
            return shapes
        #verts = [Part.Vertex(obj.Placement.multVec(p)) for p in obj.ExclusionPoints]
        verts = [Part.Vertex(p) for p in obj.ExclusionPoints]
        nedges = []
        for s in shapes:
            for e in s.Edges:
                for v in verts:
                    try:
                        d = e.distToShape(v)
                        if d and (d[0] <= MAXDIST):
                            break
                    except RuntimeError:
                        print("FIXME: shape2dview: distance unavailable for edge",e,"in",obj.Label)
                else:
                    nedges.append(e)
        return nedges

    def excludeNames(self,obj,objs):
        if hasattr(obj,"ExclusionNames"):
            objs = [o for o in objs if not(o.Name in obj.ExclusionNames)]
            return objs

    def execute(self, obj):
        if self.props_changed_placement_only(obj) \
                or not getattr(obj, "AutoUpdate", True):
            obj.positionBySupport()
            self.props_changed_clear()
            return

        import Part
        import DraftGeomUtils
        pl = obj.Placement
        if obj.Base:
            if utils.get_type(obj.Base) in ["BuildingPart","SectionPlane"]:
                objs = []
                if utils.get_type(obj.Base) == "SectionPlane":
                    objs = self.excludeNames(obj,obj.Base.Objects)
                    cutplane = obj.Base.Shape
                else:
                    objs = self.excludeNames(obj,obj.Base.Group)
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
                    if hasattr(obj,"OnlySolids"): # override base object
                        onlysolids = obj.OnlySolids
                    import Arch
                    objs = groups.get_group_contents(objs, walls=True)
                    if getattr(obj,"VisibleOnly",True):
                        objs = gui_utils.remove_hidden(objs)
                    shapes = []
                    if getattr(obj,"FuseArch", False):
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
                                                       else "None",[]).extend(o.Shape.SubShapes)
                            elif hasattr(o,'Shape'):
                                if onlysolids:
                                    shapes.extend(o.Shape.Solids)
                                else:
                                    shapes.extend(o.Shape.SubShapes)
                        for k, v in shtypes.items():
                            v1 = v.pop()
                            if v:
                                try:
                                    v1 = v1.multiFuse(v)
                                except (RuntimeError, Part.OCCError):
                                    # multifuse can fail
                                    for v2 in v:
                                        v1 = v1.fuse(v2)
                                try:
                                    v1 = v1.removeSplitter()
                                except (RuntimeError, Part.OCCError):
                                    pass
                            if v1.Solids:
                                shapes.extend(v1.Solids)
                            else:
                                print("Shape2DView: Fusing Arch objects produced non-solid results")
                                shapes.extend(v1.SubShapes)
                    else:
                        for o in objs:
                            if hasattr(o,'Shape'):
                                if onlysolids:
                                    shapes.extend(o.Shape.Solids)
                                else:
                                    shapes.extend(o.Shape.SubShapes)
                    clip = False
                    if hasattr(obj.Base,"Clip"):
                        clip = obj.Base.Clip
                    if hasattr(obj,"Clip"): #override base object
                        clip = obj.Clip
                    depth = None
                    if hasattr(obj.Base,"Depth"):
                        depth = obj.Base.Depth.Value
                    cutp, cutv, iv = Arch.getCutVolume(cutplane, shapes, clip, depth)
                    cuts = []
                    opl = App.Placement(obj.Base.Placement)
                    proj = opl.Rotation.multVec(App.Vector(0, 0, 1))
                    if obj.ProjectionMode in ["Solid","Solid faces"]:
                        shapes_to_cut = shapes
                        if obj.ProjectionMode == "Solid faces":
                            shapes_to_cut = []
                            for s in shapes:
                                shapes_to_cut.extend(s.Faces)
                        for sh in shapes_to_cut:
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
                                    cuts.extend(c.SubShapes)
                            else:
                                if onlysolids:
                                    cuts.extend(sh.Solids)
                                else:
                                    cuts.extend(sh.SubShapes)
                        comp = Part.makeCompound(cuts)
                        obj.Shape = self.getProjected(obj,comp,proj)
                    elif obj.ProjectionMode in ["Cutlines", "Cutfaces"]:
                        for sh in shapes:
                            if sh.Volume < 0:
                                sh.reverse()
                            c = sh.section(cutp)
                            if hasattr(obj,"InPlace"):
                                if not obj.InPlace:
                                    c = self.getProjected(obj, c, proj)
                            faces = []
                            if (obj.ProjectionMode == "Cutfaces") and (sh.ShapeType == "Solid"):
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
                objs = self.excludeNames(obj,groups.get_group_contents(obj.Base))
                for o in objs:
                    if hasattr(o,'Shape'):
                        shapes.extend(o.Shape.SubShapes)
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

        obj.Placement = pl
        obj.positionBySupport()
        self.props_changed_clear()

    def onChanged(self, obj, prop):
        self.props_changed_store(prop)


# Alias for compatibility with v0.18 and earlier
_Shape2DView = Shape2DView

## @}
