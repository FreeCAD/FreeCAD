# ***************************************************************************
# *   Copyright (c) 2009, 2010 Yorik van Havre <yorik@uncreated.net>        *
# *   Copyright (c) 2009, 2010 Ken Cline <cline@frii.com>                   *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful,            *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with FreeCAD; if not, write to the Free Software        *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************
"""Provides various functions to work with edges."""
## @package edges
# \ingroup draftgeoutils
# \brief Provides various functions to work with edges.

import lazy_loader.lazy_loader as lz

import FreeCAD as App
import DraftVecUtils

from draftgeoutils.general import geomType, vec

# Delay import of module until first use because it is heavy
Part = lz.LazyLoader("Part", globals(), "Part")

## \addtogroup draftgeoutils
# @{


def findEdge(anEdge, aList):
    """Return True if edge is found in list of edges."""
    for e in range(len(aList)):
        if str(anEdge.Curve) == str(aList[e].Curve):
            if DraftVecUtils.equals(anEdge.Vertexes[0].Point,
                                    aList[e].Vertexes[0].Point):
                if DraftVecUtils.equals(anEdge.Vertexes[-1].Point,
                                        aList[e].Vertexes[-1].Point):
                    return e
    return None


def orientEdge(edge, normal=None, make_arc=False):
    """Re-orient the edge such that it is in the XY plane.

    Re-orients `edge` such that it is in the XY plane.
    If `normal` is passed, this is used as the basis for the rotation,
    otherwise the placement of `edge` is used.
    """
    # This 'normalizes' the placement to the xy plane
    edge = edge.copy()
    xyDir = App.Vector(0, 0, 1)
    base = App.Vector(0, 0, 0)

    if normal:
        angle = DraftVecUtils.angle(normal, xyDir) * App.Units.Radian
        axis = normal.cross(xyDir)
    else:
        axis = edge.Placement.Rotation.Axis
        angle = -1 * edge.Placement.Rotation.Angle * App.Units.Radian
    if axis == App.Vector(0.0, 0.0, 0.0):
        axis = App.Vector(0.0, 0.0, 1.0)
    if angle:
        edge.rotate(base, axis, angle)
    if isinstance(edge.Curve, Part.Line):
        return Part.LineSegment(edge.Curve,
                                edge.FirstParameter,
                                edge.LastParameter)
    elif make_arc and isinstance(edge.Curve, Part.Circle) and not edge.Closed:
        return Part.ArcOfCircle(edge.Curve,
                                edge.FirstParameter,
                                edge.LastParameter,
                                edge.Curve.Axis.z > 0)
    elif make_arc and isinstance(edge.Curve, Part.Ellipse) and not edge.Closed:
        return Part.ArcOfEllipse(edge.Curve,
                                 edge.FirstParameter,
                                 edge.LastParameter,
                                 edge.Curve.Axis.z > 0)
    return edge.Curve


def isSameLine(e1, e2):
    """Return True if the 2 edges are lines and have the same points."""
    if not isinstance(e1.Curve, Part.LineSegment):
        return False
    if not isinstance(e2.Curve, Part.LineSegment):
        return False

    if (DraftVecUtils.equals(e1.Vertexes[0].Point,
                             e2.Vertexes[0].Point)
        and DraftVecUtils.equals(e1.Vertexes[-1].Point,
                                 e2.Vertexes[-1].Point)):
        return True
    elif (DraftVecUtils.equals(e1.Vertexes[-1].Point,
                               e2.Vertexes[0].Point)
          and DraftVecUtils.equals(e1.Vertexes[0].Point,
                                   e2.Vertexes[-1].Point)):
        return True
    return False


def is_line(bspline):
    """Return True if the given BSpline curve is a straight line."""

# previous implementation may fail for a multipole straight spline due
# a second order error in tolerance, which introduce a difference of 1e-14
# in the values of the tangents. Also, may fail on a periodic spline.
#    step = bspline.LastParameter/10
#    b = bspline.tangent(0)
#
#    for i in range(10):
#        if bspline.tangent(i * step) != b:
#            return False

    start_point = bspline.StartPoint
    end_point = bspline.EndPoint
    dist_start_end = end_point.distanceToPoint(start_point)
    if abs(bspline.length() - dist_start_end) < 1e-7:
        return True

    return False



def invert(shape):
    """Return an inverted copy of the edge or wire contained in the shape."""
    if shape.ShapeType == "Wire":
        edges = [invert(edge) for edge in shape.OrderedEdges]
        edges.reverse()
        return Part.Wire(edges)
    elif shape.ShapeType == "Edge":
        if len(shape.Vertexes) == 1:
            return shape
        if geomType(shape) == "Line":
            return Part.LineSegment(shape.Vertexes[-1].Point,
                                    shape.Vertexes[0].Point).toShape()
        elif geomType(shape) == "Circle":
            mp = findMidpoint(shape)
            return Part.Arc(shape.Vertexes[-1].Point,
                            mp,
                            shape.Vertexes[0].Point).toShape()
        elif geomType(shape) in ["BSplineCurve", "BezierCurve"]:
            if isLine(shape.Curve):
                return Part.LineSegment(shape.Vertexes[-1].Point,
                                        shape.Vertexes[0].Point).toShape()

        print("DraftGeomUtils.invert: unable to invert", shape.Curve)
        return shape
    else:
        print("DraftGeomUtils.invert: unable to handle", shape.ShapeType)
        return shape


def findMidpoint(edge):
    """Return the midpoint of an edge."""
    if edge.Length == 0:
        return None
    else:
        return edge.valueAt(edge.Curve.parameterAtDistance(edge.Length/2, edge.FirstParameter))


def getTangent(edge, from_point=None):
    """Return the tangent to an edge, including BSpline and circular arcs.

    If from_point is given, it is used to calculate the tangent,
    only useful for a circular arc.
    """
    if geomType(edge) == "Line":
        return vec(edge)

    elif (geomType(edge) == "BSplineCurve"
          or geomType(edge) == "BezierCurve"):
        if not from_point:
            return None
        cp = edge.Curve.parameter(from_point)
        return edge.Curve.tangent(cp)[0]

    elif geomType(edge) == "Circle":
        if not from_point:
            v1 = edge.Vertexes[0].Point.sub(edge.Curve.Center)
        else:
            v1 = from_point.sub(edge.Curve.Center)
        return v1.cross(edge.Curve.Axis)

    return None


def get_referenced_edges(property_value):
    """Return the Edges referenced by the value of a App:PropertyLink, App::PropertyLinkList,
       App::PropertyLinkSub or App::PropertyLinkSubList property."""
    edges = []
    if not isinstance(property_value, list):
        property_value = [property_value]
    for element in property_value:
        if hasattr(element, "Shape") and element.Shape:
            edges += shape.Edges
        elif isinstance(element, tuple) and len(element) == 2:
            object, subelement_names = element
            if hasattr(object, "Shape") and object.Shape:
                if len(subelement_names) == 1 and subelement_names[0] == "":
                    edges += object.Shape.Edges
                else:
                    for subelement_name in subelement_names:
                        if subelement_name.startswith("Edge"):
                            edge_number = int(subelement_name.lstrip("Edge")) - 1
                            if edge_number < len(object.Shape.Edges):
                                edges.append(object.Shape.Edges[edge_number])
    return edges

# compatibility layer

isLine = is_line

## @}
