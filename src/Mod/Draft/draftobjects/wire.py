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
"""Provides the object code for the Wire (Polyline) object."""
## @package wire
# \ingroup draftobjects
# \brief Provides the object code for the Wire (Polyline) object.

## \addtogroup draftobjects
# @{
import math
from PySide.QtCore import QT_TRANSLATE_NOOP

import FreeCAD as App
import DraftGeomUtils
import DraftVecUtils

from draftutils.utils import get_param
from draftobjects.base import DraftObject


class Wire(DraftObject):
    """The Wire object"""

    def __init__(self, obj):
        super(Wire, self).__init__(obj, "Wire")

        _tip = QT_TRANSLATE_NOOP("App::Property",
                "The vertices of the wire")
        obj.addProperty("App::PropertyVectorList","Points", "Draft",_tip)

        _tip = QT_TRANSLATE_NOOP("App::Property",
                "If the wire is closed or not")
        obj.addProperty("App::PropertyBool","Closed", "Draft",_tip)

        _tip = QT_TRANSLATE_NOOP("App::Property",
                "The base object is the wire, it's formed from 2 objects")
        obj.addProperty("App::PropertyLink","Base", "Draft",_tip)

        _tip = QT_TRANSLATE_NOOP("App::Property",
                "The tool object is the wire, it's formed from 2 objects")
        obj.addProperty("App::PropertyLink","Tool", "Draft",_tip)

        _tip = QT_TRANSLATE_NOOP("App::Property",
                "The start point of this line")
        obj.addProperty("App::PropertyVectorDistance","Start", "Draft",_tip)

        _tip = QT_TRANSLATE_NOOP("App::Property",
                "The end point of this line")
        obj.addProperty("App::PropertyVectorDistance","End", "Draft",_tip)

        _tip = QT_TRANSLATE_NOOP("App::Property",
                "The length of this line")
        obj.addProperty("App::PropertyLength","Length", "Draft",_tip)

        _tip = QT_TRANSLATE_NOOP("App::Property",
                "Radius to use to fillet the corners")
        obj.addProperty("App::PropertyLength","FilletRadius", "Draft",_tip)

        _tip = QT_TRANSLATE_NOOP("App::Property",
                "Size of the chamfer to give to the corners")
        obj.addProperty("App::PropertyLength","ChamferSize", "Draft",_tip)

        _tip = QT_TRANSLATE_NOOP("App::Property",
                "Create a face if this object is closed")
        obj.addProperty("App::PropertyBool","MakeFace", "Draft",_tip)

        _tip = QT_TRANSLATE_NOOP("App::Property",
                "The number of subdivisions of each edge")
        obj.addProperty("App::PropertyInteger","Subdivisions", "Draft",_tip)

        _tip = QT_TRANSLATE_NOOP("App::Property",
                "The area of this object")
        obj.addProperty("App::PropertyArea","Area", "Draft",_tip)

        obj.MakeFace = get_param("fillmode",True)
        obj.Closed = False

    def execute(self, obj):
        if self.props_changed_placement_only(obj): # Supplying obj is required because of `Base` and `Tool`.
            obj.positionBySupport()
            self.update_start_end(obj)
            self.props_changed_clear()
            return

        import Part
        plm = obj.Placement
        if obj.Base and (not obj.Tool):
            if obj.Base.isDerivedFrom("Sketcher::SketchObject"):
                shape = obj.Base.Shape.copy()
                if obj.Base.Shape.isClosed():
                    if getattr(obj,"MakeFace",True):
                        shape = Part.Face(shape)
                obj.Shape = shape
        elif obj.Base and obj.Tool:
            if hasattr(obj.Base,'Shape') and hasattr(obj.Tool,'Shape'):
                if (not obj.Base.Shape.isNull()) and (not obj.Tool.Shape.isNull()):
                    sh1 = obj.Base.Shape.copy()
                    sh2 = obj.Tool.Shape.copy()
                    shape = sh1.fuse(sh2)
                    if DraftGeomUtils.isCoplanar(shape.Faces):
                        shape = DraftGeomUtils.concatenate(shape)
                        obj.Shape = shape
                        p = []
                        for v in shape.Vertexes: p.append(v.Point)
                        if obj.Points != p: obj.Points = p
        elif obj.Points:
            if obj.Points[0] == obj.Points[-1]:
                if not obj.Closed: obj.Closed = True
                obj.Points.pop()
            if obj.Closed and (len(obj.Points) > 2):
                pts = obj.Points
                if getattr(obj,"Subdivisions",0) > 0:
                    npts = []
                    for i in range(len(pts)):
                        p1 = pts[i]
                        npts.append(pts[i])
                        if i == len(pts)-1:
                            p2 = pts[0]
                        else:
                            p2 = pts[i+1]
                        v = p2.sub(p1)
                        v = DraftVecUtils.scaleTo(v,v.Length/(obj.Subdivisions+1))
                        for j in range(obj.Subdivisions):
                            npts.append(p1.add(App.Vector(v).multiply(j+1)))
                    pts = npts
                shape = Part.makePolygon(pts+[pts[0]])
                if "ChamferSize" in obj.PropertiesList:
                    if obj.ChamferSize.Value != 0:
                        w = DraftGeomUtils.filletWire(shape,obj.ChamferSize.Value,chamfer=True)
                        if w:
                            shape = w
                if "FilletRadius" in obj.PropertiesList:
                    if obj.FilletRadius.Value != 0:
                        w = DraftGeomUtils.filletWire(shape,obj.FilletRadius.Value)
                        if w:
                            shape = w
                try:
                    if getattr(obj,"MakeFace",True):
                        shape = Part.Face(shape)
                except Part.OCCError:
                    pass
            else:
                edges = []
                pts = obj.Points[1:]
                lp = obj.Points[0]
                for p in pts:
                    if not DraftVecUtils.equals(lp,p):
                        if getattr(obj,"Subdivisions",0) > 0:
                            npts = []
                            v = p.sub(lp)
                            v = DraftVecUtils.scaleTo(v,v.Length/(obj.Subdivisions+1))
                            edges.append(Part.LineSegment(lp,lp.add(v)).toShape())
                            lv = lp.add(v)
                            for j in range(obj.Subdivisions):
                                edges.append(Part.LineSegment(lv,lv.add(v)).toShape())
                                lv = lv.add(v)
                        else:
                            edges.append(Part.LineSegment(lp,p).toShape())
                        lp = p
                try:
                    shape = Part.Wire(edges)
                except Part.OCCError:
                    print("Error wiring edges")
                    shape = None
                if "ChamferSize" in obj.PropertiesList:
                    if obj.ChamferSize.Value != 0:
                        w = DraftGeomUtils.filletWire(shape,obj.ChamferSize.Value,chamfer=True)
                        if w:
                            shape = w
                if "FilletRadius" in obj.PropertiesList:
                    if obj.FilletRadius.Value != 0:
                        w = DraftGeomUtils.filletWire(shape,obj.FilletRadius.Value)
                        if w:
                            shape = w
            if shape:
                obj.Shape = shape
                if hasattr(obj,"Area") and hasattr(shape,"Area"):
                    obj.Area = shape.Area
                if hasattr(obj,"Length"):
                    obj.Length = shape.Length

        obj.Placement = plm
        obj.positionBySupport()
        self.update_start_end(obj)
        self.props_changed_clear()

    def onChanged(self, obj, prop):
        self.props_changed_store(prop)
        tol = 1e-7

        if prop == "Start":
            pts = obj.Points
            invpl = App.Placement(obj.Placement).inverse()
            realfpstart = invpl.multVec(obj.Start)
            if pts:
                if not pts[0].isEqual(realfpstart, tol):
                    pts[0] = realfpstart
                    obj.Points = pts

        elif prop == "End":
            pts = obj.Points
            invpl = App.Placement(obj.Placement).inverse()
            realfpend = invpl.multVec(obj.End)
            if len(pts) > 1:
                if not pts[-1].isEqual(realfpend, tol):
                    pts[-1] = realfpend
                    obj.Points = pts

        elif prop == "Length":
            if (len(obj.Points) == 2
                    and obj.Length.Value > tol
                    and obj.Shape
                    and (not obj.Shape.isNull())
                    and abs(obj.Length.Value - obj.Shape.Length) > tol):
                v = obj.Points[-1].sub(obj.Points[0])
                v = DraftVecUtils.scaleTo(v, obj.Length.Value)
                obj.Points = [obj.Points[0], obj.Points[0].add(v)]

    def update_start_end(self, obj):
        tol = 1e-7

        pl = App.Placement(obj.Placement)
        if len(obj.Points) > 1:
            displayfpstart = pl.multVec(obj.Points[0])
            displayfpend = pl.multVec(obj.Points[-1])
            if not obj.Start.isEqual(displayfpstart, tol):
                obj.Start = displayfpstart
            if not obj.End.isEqual(displayfpend, tol):
                obj.End = displayfpend


# Alias for compatibility with v0.18 and earlier
_Wire = Wire

## @}
