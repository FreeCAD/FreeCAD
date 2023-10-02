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
"""Provides functions to create offsets of different shapes."""
## @package offset
# \ingroup draftfunctions
# \brief Provides functions to create offsets of different shapes.

## \addtogroup draftfunctions
# @{
import math

import FreeCAD as App
import DraftVecUtils
import draftutils.gui_utils as gui_utils
import draftutils.utils as utils

from draftmake.make_rectangle import make_rectangle
from draftmake.make_wire import make_wire
from draftmake.make_polygon import make_polygon
from draftmake.make_circle import make_circle
from draftmake.make_bspline import make_bspline


def offset(obj, delta, copy=False, bind=False, sym=False, occ=False):
    """offset(object,delta,[copymode],[bind])

    Offset the given wire by applying the given delta Vector to its first
    vertex.

    Parameters
    ----------
    obj :

    delta : Base.Vector or list of Base.Vector
        If offsetting a BSpline, the delta must not be a Vector but a list
        of Vectors, one for each node of the spline.

    copy : bool
        If copymode is True, another object is created, otherwise the same
        object gets offsetted.

    copy : bool
        If bind is True, and provided the wire is open, the original
        and the offset wires will be bound by their endpoints, forming a face.

    sym : bool
        if sym is True, bind must be true too, and the offset is made on both
        sides, the total width being the given delta length.
    """
    import Part
    import DraftGeomUtils
    newwire = None
    delete = None

    if (copy is False
            and (utils.get_type(obj).startswith("Sketcher::")
                or utils.get_type(obj).startswith("Part::")
                or utils.get_type(obj).startswith("PartDesign::"))): # For PartDesign_SubShapeBinders which can reference sketches.
        print("the offset tool is currently unable to offset a non-Draft object directly - Creating a copy")
        copy = True

    def getRect(p,obj):
        """returns length,height,placement"""
        pl = obj.Placement.copy()
        pl.Base = p[0]
        diag = p[2].sub(p[0])
        bb = p[1].sub(p[0])
        bh = p[3].sub(p[0])
        nb = DraftVecUtils.project(diag,bb)
        nh = DraftVecUtils.project(diag,bh)
        if obj.Length.Value < 0: l = -nb.Length
        else: l = nb.Length
        if obj.Height.Value < 0: h = -nh.Length
        else: h = nh.Length
        return l,h,pl

    def getRadius(obj,delta):
        """returns a new radius for a regular polygon"""
        an = math.pi/obj.FacesNumber
        nr = DraftVecUtils.rotate(delta,-an)
        nr.multiply(1/math.cos(an))
        nr = obj.Shape.Vertexes[0].Point.add(nr)
        nr = nr.sub(obj.Placement.Base)
        nr = nr.Length
        if obj.DrawMode == "inscribed":
            return nr
        else:
            return nr * math.cos(math.pi/obj.FacesNumber)

    newwire = None
    if utils.get_type(obj) == "Circle":
        pass
    elif utils.get_type(obj) == "BSpline":
        pass
    else:
        if sym:
            d1 = App.Vector(delta).multiply(0.5)
            d2 = d1.negative()
            n1 = DraftGeomUtils.offsetWire(obj.Shape,d1)
            n2 = DraftGeomUtils.offsetWire(obj.Shape,d2)
        else:
            if isinstance(delta,float) and (len(obj.Shape.Edges) == 1):
                # circle
                c = obj.Shape.Edges[0].Curve
                nc = Part.Circle(c.Center,c.Axis,delta)
                if len(obj.Shape.Vertexes) > 1:
                    nc = Part.ArcOfCircle(nc,obj.Shape.Edges[0].FirstParameter,obj.Shape.Edges[0].LastParameter)
                newwire = Part.Wire(nc.toShape())
                p = []
            else:
                newwire = DraftGeomUtils.offsetWire(obj.Shape,delta)
                if DraftGeomUtils.hasCurves(newwire) and copy:
                    p = []
                else:
                    p = DraftGeomUtils.getVerts(newwire)
    if occ:
        newobj = App.ActiveDocument.addObject("Part::Feature","Offset")
        newobj.Shape = DraftGeomUtils.offsetWire(obj.Shape,delta,occ=True)
        gui_utils.formatObject(newobj,obj)
        if not copy:
            delete = obj.Name
    elif bind:
        if not DraftGeomUtils.isReallyClosed(obj.Shape):
            if sym:
                s1 = n1
                s2 = n2
            else:
                s1 = obj.Shape
                s2 = newwire
            if s1 and s2:
                w1 = s1.Edges
                w2 = s2.Edges
                w3 = Part.LineSegment(s1.Vertexes[0].Point,s2.Vertexes[0].Point).toShape()
                w4 = Part.LineSegment(s1.Vertexes[-1].Point,s2.Vertexes[-1].Point).toShape()
                newobj = App.ActiveDocument.addObject("Part::Feature","Offset")
                newobj.Shape = Part.Face(Part.Wire(w1+[w3]+w2+[w4]))
            else:
                print("Draft.offset: Unable to bind wires")
        else:
            newobj = App.ActiveDocument.addObject("Part::Feature","Offset")
            newobj.Shape = Part.Face(obj.Shape.Wires[0])
        if not copy:
            delete = obj.Name
    elif copy:
        newobj = None
        if sym: return None
        if utils.get_type(obj) == "Wire":
            if p:
                newobj = make_wire(p)
                newobj.Closed = obj.Closed
            elif newwire:
                newobj = App.ActiveDocument.addObject("Part::Feature","Offset")
                newobj.Shape = newwire
            else:
                print("Draft.offset: Unable to duplicate this object")
        elif utils.get_type(obj) == "Rectangle":
            if p:
                length,height,plac = getRect(p,obj)
                newobj = make_rectangle(length,height,plac)
            elif newwire:
                newobj = App.ActiveDocument.addObject("Part::Feature","Offset")
                newobj.Shape = newwire
            else:
                print("Draft.offset: Unable to duplicate this object")
        elif utils.get_type(obj) == "Circle":
            pl = obj.Placement
            newobj = make_circle(delta)
            newobj.FirstAngle = obj.FirstAngle
            newobj.LastAngle = obj.LastAngle
            newobj.Placement = pl
        elif utils.get_type(obj) == "Polygon":
            pl = obj.Placement
            newobj = make_polygon(obj.FacesNumber)
            newobj.Radius = getRadius(obj,delta)
            newobj.DrawMode = obj.DrawMode
            newobj.Placement = pl
        elif utils.get_type(obj) == "BSpline":
            newobj = make_bspline(delta)
            newobj.Closed = obj.Closed
        else:
            # try to offset anyway
            try:
                if p:
                    newobj = make_wire(p)
                    newobj.Closed = DraftGeomUtils.isReallyClosed(obj.Shape)
            except Part.OCCError:
                pass
            if (not newobj) and newwire:
                newobj = App.ActiveDocument.addObject("Part::Feature","Offset")
                newobj.Shape = newwire
            if not newobj:
                print("Draft.offset: Unable to create an offset")
        if newobj:
            gui_utils.formatObject(newobj,obj)
    else:
        newobj = None
        if sym: return None
        if utils.get_type(obj) == "Wire":
            if obj.Base or obj.Tool:
                App.Console.PrintWarning("Warning: object history removed\n")
                obj.Base = None
                obj.Tool = None
            obj.Placement = App.Placement() # p points are in the global coordinate system
            obj.Points = p
        elif utils.get_type(obj) == "BSpline":
            #print(delta)
            obj.Points = delta
            #print("done")
        elif utils.get_type(obj) == "Rectangle":
            length,height,plac = getRect(p,obj)
            obj.Placement = plac
            obj.Length = length
            obj.Height = height
        elif utils.get_type(obj) == "Circle":
            obj.Radius = delta
        elif utils.get_type(obj) == "Polygon":
            obj.Radius = getRadius(obj,delta)
        elif utils.get_type(obj) == 'Part':
            print("unsupported object") # TODO
        newobj = obj
    if copy and utils.get_param("selectBaseObjects",False):
        gui_utils.select(newobj)
    else:
        gui_utils.select(obj)
    if delete:
        App.ActiveDocument.removeObject(delete)
    return newobj

## @}
