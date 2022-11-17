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
"""Provides various functions to work with arcs."""
## @package arcs
# \ingroup draftgeoutils
# \brief Provides various functions to work with arcs.

import math
import lazy_loader.lazy_loader as lz

import FreeCAD as App
import DraftVecUtils

from draftgeoutils.general import geomType
from draftgeoutils.edges import findMidpoint

# Delay import of module until first use because it is heavy
Part = lz.LazyLoader("Part", globals(), "Part")

## \addtogroup draftgeoutils
# @{


def isClockwise(edge, ref=None):
    """Return True if a circle-based edge has a clockwise direction."""
    if not geomType(edge) == "Circle":
        return True

    v1 = edge.Curve.tangent(edge.ParameterRange[0])[0]
    if DraftVecUtils.isNull(v1):
        return True

    # we take an arbitrary other point on the edge that has little chances
    # to be aligned with the first one
    v2 = edge.Curve.tangent(edge.ParameterRange[0] + 0.01)[0]
    n = edge.Curve.Axis
    # if that axis points "the wrong way" from the reference, we invert it
    if not ref:
        ref = App.Vector(0, 0, 1)
    if n.getAngle(ref) > math.pi/2:
        n = n.negative()

    if DraftVecUtils.angle(v1, v2, n) < 0:
        return False

    if n.z < 0:
        return False

    return True


def isWideAngle(edge):
    """Return True if the given edge is an arc with angle > 180 degrees."""
    if geomType(edge) != "Circle":
        return False

    r = edge.Curve.Radius
    total = 2*r*math.pi

    if edge.Length > total/2:
        return True

    return False


def arcFrom2Pts(firstPt, lastPt, center, axis=None):
    """Build an arc with center and 2 points, can be oriented with axis."""
    radius1 = firstPt.sub(center).Length
    radius2 = lastPt.sub(center).Length

    # (PREC = 4 = same as Part Module),  Is it possible?
    if round(radius1-radius2, 4) != 0:
        return None

    thirdPt = App.Vector(firstPt.sub(center).add(lastPt).sub(center))
    thirdPt.normalize()
    thirdPt.scale(radius1, radius1, radius1)
    thirdPt = thirdPt.add(center)
    newArc = Part.Edge(Part.Arc(firstPt, thirdPt, lastPt))

    if axis and newArc.Curve.Axis.dot(axis) < 0:
        thirdPt = thirdPt.sub(center)
        thirdPt.scale(-1, -1, -1)
        thirdPt = thirdPt.add(center)
        newArc = Part.Edge(Part.Arc(firstPt, thirdPt, lastPt))

    return newArc


def arcFromSpline(edge):
    """Turn given edge into a circular arc from three points.

    Takes its first point, midpoint and endpoint. It works best with bspline
    segments such as those from imported svg files. Use this only
    if you are sure your edge is really an arc.

    It returns None if there is a problem, including passing straight edges.
    """
    if geomType(edge) == "Line":
        print("This edge is straight, cannot build an arc on it")
        return None

    if len(edge.Vertexes) > 1:
        # 2-point arc
        p1 = edge.Vertexes[0].Point
        p2 = edge.Vertexes[-1].Point
        ml = edge.Length/2
        p3 = edge.valueAt(ml)
        try:
            return Part.Arc(p1, p3, p2).toShape()
        except Part.OCCError:
            print("Couldn't make an arc out of this edge")
            return None
    else:
        # circle
        p1 = edge.Vertexes[0].Point
        p2 = findMidpoint(edge)
        ray = p2.sub(p1)
        ray.scale(0.5, 0.5, 0.5)
        center = p1.add(ray)
        radius = ray.Length
        try:
            return Part.makeCircle(radius, center)
        except Part.OCCError:
            print("couldn't make a circle out of this edge")
            return None

## @}
