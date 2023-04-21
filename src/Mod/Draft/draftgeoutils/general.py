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
"""Provides general functions to work with topological shapes."""
## @package general
# \ingroup draftgeoutils
# \brief Provides general functions to work with topological shapes.

import math
import lazy_loader.lazy_loader as lz

import FreeCAD as App
import DraftVecUtils

# Delay import of module until first use because it is heavy
Part = lz.LazyLoader("Part", globals(), "Part")

## \addtogroup draftgeoutils
# @{
PARAMGRP = App.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft")

# Default normal direction for all geometry operations
NORM = App.Vector(0, 0, 1)


def precision():
    """Return the Draft precision setting."""
    # Set precision level with a cap to avoid overspecification that:
    #  1 - whilst it is precise enough (e.g. that OCC would consider
    #      2 points are coincident)
    #      (not sure what it should be 10 or otherwise);
    #  2 - but FreeCAD/OCC can handle 'internally'
    #      (e.g. otherwise user may set something like
    #      15 that the code would never consider 2 points are coincident
    #      as internal float is not that precise)
    precisionMax = 10
    precisionInt = PARAMGRP.GetInt("precision", 6)
    precisionInt = (precisionInt if precisionInt <= 10 else precisionMax)
    return precisionInt  # return PARAMGRP.GetInt("precision", 6)


def vec(edge, use_orientation = False):
    """Return a vector from an edge or a Part.LineSegment.

    If use_orientation is True, it takes into account the edges orientation.
    If edge is not straight, you'll get strange results!
    """
    if isinstance(edge, Part.Shape):
        if use_orientation and isinstance(edge, Part.Edge) and edge.Orientation == "Reversed":
            return edge.Vertexes[0].Point.sub(edge.Vertexes[-1].Point)
        else:
            return edge.Vertexes[-1].Point.sub(edge.Vertexes[0].Point)
    elif isinstance(edge, Part.LineSegment):
        return edge.EndPoint.sub(edge.StartPoint)
    else:
        return None


def edg(p1, p2):
    """Return an edge from 2 vectors."""
    if isinstance(p1, App.Vector) and isinstance(p2, App.Vector):
        if DraftVecUtils.equals(p1, p2):
            return None

        return Part.LineSegment(p1, p2).toShape()


def getVerts(shape):
    """Return a list containing vectors of each vertex of the shape."""
    if not hasattr(shape, "Vertexes"):
        return []

    p = []
    for v in shape.Vertexes:
        p.append(v.Point)
    return p


def v1(edge):
    """Return the first point of an edge."""
    return edge.Vertexes[0].Point


def isNull(something):
    """Return True if the given shape, vector, or placement is Null.

    If the vector is (0, 0, 0), it will return True.
    """
    if isinstance(something, Part.Shape):
        return something.isNull()
    elif isinstance(something, App.Vector):
        if something == App.Vector(0, 0, 0):
            return True
        else:
            return False
    elif isinstance(something, App.Placement):
        if (something.Base == App.Vector(0, 0, 0)
                and something.Rotation.Q == (0, 0, 0, 1)):
            return True
        else:
            return False


def isPtOnEdge(pt, edge):
    """Test if a point lies on an edge."""
    v = Part.Vertex(pt)
    try:
        d = v.distToShape(edge)
    except Part.OCCError:
        return False
    else:
        if d:
            if round(d[0], precision()) == 0:
                return True
    return False


def hasCurves(shape):
    """Check if the given shape has curves."""
    for e in shape.Edges:
        if not isinstance(e.Curve, (Part.LineSegment, Part.Line)):
            return True
    return False


def isAligned(edge, axis="x"):
    """Check if the given edge or line is aligned to the given axis.

    The axis can be 'x', 'y' or 'z'.
    """
    def is_same(a, b):
        return round(a, precision()) == round(b, precision())

    if axis == "x":
        if isinstance(edge, Part.Edge):
            if len(edge.Vertexes) == 2:
                return is_same(edge.Vertexes[0].X, edge.Vertexes[-1].X)
        elif isinstance(edge, Part.LineSegment):
            return is_same(edge.StartPoint.x, edge.EndPoint.x)

    elif axis == "y":
        if isinstance(edge, Part.Edge):
            if len(edge.Vertexes) == 2:
                return is_same(edge.Vertexes[0].Y, edge.Vertexes[-1].Y)
        elif isinstance(edge, Part.LineSegment):
            return is_same(edge.StartPoint.y, edge.EndPoint.y)

    elif axis == "z":
        if isinstance(edge, Part.Edge):
            if len(edge.Vertexes) == 2:
                return is_same(edge.Vertexes[0].Z, edge.Vertexes[-1].Z)
        elif isinstance(edge, Part.LineSegment):
            return is_same(edge.StartPoint.z, edge.EndPoint.z)

    return False


def getQuad(face):
    """Return a list of 3 vectors if the face is a quad, ortherwise None.

    Returns
    -------
    basepoint, Xdir, Ydir
        If the face is a quad.

    None
        If the face is not a quad.
    """
    if len(face.Edges) != 4:
        return None

    v1 = vec(face.Edges[0])  # Warning redefinition of function v1
    v2 = vec(face.Edges[1])
    v3 = vec(face.Edges[2])
    v4 = vec(face.Edges[3])
    angles90 = [round(math.pi*0.5, precision()),
                round(math.pi*1.5, precision())]

    angles180 = [0,
                 round(math.pi, precision()),
                 round(math.pi*2, precision())]
    for ov in [v2, v3, v4]:
        if not (round(v1.getAngle(ov), precision()) in angles90 + angles180):
            return None

    for ov in [v2, v3, v4]:
        if round(v1.getAngle(ov), precision()) in angles90:
            v1.normalize()
            ov.normalize()
            return [face.Edges[0].Vertexes[0].Point, v1, ov]


def areColinear(e1, e2):
    """Return True if both edges are colinear."""
    if not isinstance(e1.Curve, (Part.LineSegment, Part.Line)):
        return False
    if not isinstance(e2.Curve, (Part.LineSegment, Part.Line)):
        return False

    v1 = vec(e1)
    v2 = vec(e2)
    a = round(v1.getAngle(v2), precision())
    if (a == 0) or (a == round(math.pi, precision())):
        v3 = e2.Vertexes[0].Point.sub(e1.Vertexes[0].Point)
        if DraftVecUtils.isNull(v3):
            return True
        else:
            a2 = round(v1.getAngle(v3), precision())
            if (a2 == 0) or (a2 == round(math.pi, precision())):
                return True
    return False


def hasOnlyWires(shape):
    """Return True if all edges are inside a wire."""
    ne = 0
    for w in shape.Wires:
        ne += len(w.Edges)
    if ne == len(shape.Edges):
        return True
    return False


def geomType(edge):
    """Return the type of geometry this edge is based on."""
    try:
        if isinstance(edge.Curve, (Part.LineSegment, Part.Line)):
            return "Line"
        elif isinstance(edge.Curve, Part.Circle):
            return "Circle"
        elif isinstance(edge.Curve, Part.BSplineCurve):
            return "BSplineCurve"
        elif isinstance(edge.Curve, Part.BezierCurve):
            return "BezierCurve"
        elif isinstance(edge.Curve, Part.Ellipse):
            return "Ellipse"
        else:
            return "Unknown"
    except Exception: # catch all errors, no only TypeError
        return "Unknown"


def isValidPath(shape):
    """Return True if the shape can be used as an extrusion path."""
    if shape.isNull():
        return False
    if shape.Faces:
        return False
    if len(shape.Wires) > 1:
        return False
    if shape.Wires:
        if shape.Wires[0].isClosed():
            return False
    if shape.isClosed():
        return False
    return True


def findClosest(base_point, point_list):
    """Find closest point in a list of points to the base point.

    Returns
    -------
    int
        An index from the list of points is returned.

    None
        If point_list is empty.
    """
    npoint = None
    if not point_list:
        return None

    smallest = 1000000
    for n in range(len(point_list)):
        new = base_point.sub(point_list[n]).Length
        if new < smallest:
            smallest = new
            npoint = n

    return npoint


def getBoundaryAngles(angle, alist):
    """Return the 2 closest angles that encompass the given angle."""
    negs = True
    while negs:
        negs = False
        for i in range(len(alist)):
            if alist[i] < 0:
                alist[i] = 2*math.pi + alist[i]
                negs = True
        if angle < 0:
            angle = 2*math.pi + angle
            negs = True

    lower = None
    for a in alist:
        if a < angle:
            if lower is None:
                lower = a
            else:
                if a > lower:
                    lower = a

    if lower is None:
        lower = 0
        for a in alist:
            if a > lower:
                lower = a

    higher = None
    for a in alist:
        if a > angle:
            if higher is None:
                higher = a
            else:
                if a < higher:
                    higher = a

    if higher is None:
        higher = 2*math.pi
        for a in alist:
            if a < higher:
                higher = a

    return lower, higher

## @}
