# ***************************************************************************
# *   Copyright (c) 2009, 2010 Yorik van Havre <yorik@uncreated.net>        *
# *   Copyright (c) 2009, 2010 Ken Cline <cline@frii.com>                   *
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
"""Define geometry functions for manipulating shapes in the Draft Workbench.

These functions are used by different object creation functions
of the Draft Workbench, both in `Draft.py` and `DraftTools.py`.
They operate on the internal shapes (`Part::TopoShape`) of different objects
and on their subelements, that is, vertices, edges, and faces.
"""
## \defgroup DRAFTGEOMUTILS DraftGeomUtils
#  \ingroup UTILITIES
#  \brief Shape manipulation utilities for the Draft workbench
#
# Shapes manipulation utilities

## \addtogroup DRAFTGEOMUTILS
#  @{
import cmath
import math

import FreeCAD
import Part
import DraftVecUtils
from FreeCAD import Vector

__title__ = "FreeCAD Draft Workbench - Geometry library"
__author__ = "Yorik van Havre, Jacques-Antoine Gaudin, Ken Cline"
__url__ = ["https://www.freecadweb.org"]

from draftgeoutils.general import PARAMGRP as params

from draftgeoutils.general import NORM

# Generic functions *********************************************************


from draftgeoutils.general import precision


from draftgeoutils.general import vec


from draftgeoutils.general import edg


from draftgeoutils.general import getVerts


from draftgeoutils.general import v1


from draftgeoutils.general import isNull


from draftgeoutils.general import isPtOnEdge


from draftgeoutils.general import hasCurves


from draftgeoutils.general import isAligned


from draftgeoutils.general import getQuad


from draftgeoutils.general import areColinear


from draftgeoutils.general import hasOnlyWires


from draftgeoutils.general import geomType


from draftgeoutils.general import isValidPath


# edge functions *************************************************************


from draftgeoutils.edges import findEdge


from draftgeoutils.intersections import findIntersection


from draftgeoutils.intersections import wiresIntersect


from draftgeoutils.offsets import pocket2d


from draftgeoutils.edges import orientEdge


from draftgeoutils.geometry import mirror


from draftgeoutils.arcs import isClockwise


from draftgeoutils.edges import isSameLine


from draftgeoutils.arcs import isWideAngle


from draftgeoutils.general import findClosest


from draftgeoutils.faces import concatenate


from draftgeoutils.faces import getBoundary


from draftgeoutils.edges import isLine


from draftgeoutils.sort_edges import sortEdges


from draftgeoutils.sort_edges import sortEdgesOld


from draftgeoutils.edges import invert


from draftgeoutils.wires import flattenWire


from draftgeoutils.wires import findWires


from draftgeoutils.wires import findWiresOld2


from draftgeoutils.wires import superWire


from draftgeoutils.edges import findMidpoint


from draftgeoutils.geometry import findPerpendicular


from draftgeoutils.offsets import offset


from draftgeoutils.wires import isReallyClosed


from draftgeoutils.geometry import getSplineNormal


from draftgeoutils.geometry import getNormal


from draftgeoutils.geometry import getRotation


from draftgeoutils.geometry import calculatePlacement


from draftgeoutils.offsets import offsetWire


from draftgeoutils.intersections import connect


from draftgeoutils.geometry import findDistance


from draftgeoutils.intersections import angleBisection


from draftgeoutils.circles import findClosestCircle


from draftgeoutils.faces import isCoplanar


from draftgeoutils.geometry import isPlanar


from draftgeoutils.wires import findWiresOld


from draftgeoutils.edges import getTangent


from draftgeoutils.faces import bind


from draftgeoutils.faces import cleanFaces


from draftgeoutils.cuboids import isCubic


from draftgeoutils.cuboids import getCubicDimensions


from draftgeoutils.wires import removeInterVertices


from draftgeoutils.arcs import arcFromSpline


from draftgeoutils.fillets import fillet


from draftgeoutils.fillets import filletWire


from draftgeoutils.circles import getCircleFromSpline


from draftgeoutils.wires import curvetowire


from draftgeoutils.wires import cleanProjection


from draftgeoutils.wires import curvetosegment


from draftgeoutils.wires import tessellateProjection


from draftgeoutils.wires import rebaseWire


from draftgeoutils.faces import removeSplitter


# circle functions *********************************************************


def getBoundaryAngles(angle, alist):
        """returns the 2 closest angles from the list that
        encompass the given angle"""
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
        return (lower,higher)


def circleFrom2tan1pt(tan1, tan2, point):
    """circleFrom2tan1pt(edge, edge, Vector)"""
    if (geomType(tan1) == "Line") and (geomType(tan2) == "Line") and isinstance(point, FreeCAD.Vector):
        return circlefrom2Lines1Point(tan1, tan2, point)
    elif (geomType(tan1) == "Circle") and (geomType(tan2) == "Line") and isinstance(point, FreeCAD.Vector):
        return circlefromCircleLinePoint(tan1, tan2, point)
    elif (geomType(tan2) == "Circle") and (geomType(tan1) == "Line") and isinstance(point, FreeCAD.Vector):
        return circlefromCircleLinePoint(tan2, tan1, point)
    elif (geomType(tan2) == "Circle") and (geomType(tan1) == "Circle") and isinstance(point, FreeCAD.Vector):
        return circlefrom2Circles1Point(tan2, tan1, point)


def circleFrom2tan1rad(tan1, tan2, rad):
    """circleFrom2tan1rad(edge, edge, float)"""
    if (geomType(tan1) == "Line") and (geomType(tan2) == "Line"):
        return circleFrom2LinesRadius(tan1, tan2, rad)
    elif (geomType(tan1) == "Circle") and (geomType(tan2) == "Line"):
        return circleFromCircleLineRadius(tan1, tan2, rad)
    elif (geomType(tan1) == "Line") and (geomType(tan2) == "Circle"):
        return circleFromCircleLineRadius(tan2, tan1, rad)
    elif (geomType(tan1) == "Circle") and (geomType(tan2) == "Circle"):
        return circleFrom2CirclesRadius(tan1, tan2, rad)


def circleFrom1tan2pt(tan1, p1, p2):
    if (geomType(tan1) == "Line") and isinstance(p1, FreeCAD.Vector) and isinstance(p2, FreeCAD.Vector):
        return circlefrom1Line2Points(tan1, p1, p2)
    if (geomType(tan1) == "Line") and isinstance(p1, FreeCAD.Vector) and isinstance(p2, FreeCAD.Vector):
        return circlefrom1Circle2Points(tan1, p1, p2)


def circleFrom1tan1pt1rad(tan1, p1, rad):
    if (geomType(tan1) == "Line") and isinstance(p1, FreeCAD.Vector):
        return circleFromPointLineRadius(p1, tan1, rad)
    if (geomType(tan1) == "Circle") and isinstance(p1, FreeCAD.Vector):
        return circleFromPointCircleRadius(p1, tan1, rad)


def circleFrom3tan(tan1, tan2, tan3):
    tan1IsLine = (geomType(tan1) == "Line")
    tan2IsLine = (geomType(tan2) == "Line")
    tan3IsLine = (geomType(tan3) == "Line")
    tan1IsCircle = (geomType(tan1) == "Circle")
    tan2IsCircle = (geomType(tan2) == "Circle")
    tan3IsCircle = (geomType(tan3) == "Circle")
    if tan1IsLine and tan2IsLine and tan3IsLine:
        return circleFrom3LineTangents(tan1, tan2, tan3)
    elif tan1IsCircle and tan2IsCircle and tan3IsCircle:
        return circleFrom3CircleTangents(tan1, tan2, tan3)
    elif (tan1IsCircle and tan2IsLine and tan3IsLine):
        return circleFrom1Circle2Lines(tan1, tan2, tan3)
    elif (tan1IsLine and tan2IsCircle and tan3IsLine):
        return circleFrom1Circle2Lines(tan2, tan1, tan3)
    elif (tan1IsLine and tan2IsLine and tan3IsCircle):
        return circleFrom1Circle2Lines(tan3, tan1, tan2)
    elif (tan1IsLine and tan2IsCircle and tan3IsCircle):
        return circleFrom2Circle1Lines(tan2, tan3, tan1)
    elif (tan1IsCircle and tan2IsLine and tan3IsCircle):
        return circleFrom2Circle1Lines(tan1, tan3, tan2)
    elif (tan1IsCircle and tan2IsCircle and tan3IsLine):
        return circleFrom2Circle1Lines(tan1, tan2, tan3)


from draftgeoutils.circles import circlefrom2Lines1Point


from draftgeoutils.circles import circlefrom1Line2Points


from draftgeoutils.circles import circleFrom2LinesRadius


from draftgeoutils.circles import circleFrom3LineTangents


from draftgeoutils.circles import circleFromPointLineRadius


from draftgeoutils.circles import circleFrom2PointsRadius


from draftgeoutils.arcs import arcFrom2Pts


#############################33 to include

from draftgeoutils.circles_apollonius import outerSoddyCircle


from draftgeoutils.circles_apollonius import innerSoddyCircle


from draftgeoutils.circles_apollonius import circleFrom3CircleTangents


from draftgeoutils.linear_algebra import linearFromPoints


from draftgeoutils.linear_algebra import determinant


from draftgeoutils.circles import findHomotheticCenterOfCircles


from draftgeoutils.circles import findRadicalAxis


from draftgeoutils.circles import findRadicalCenter


def pointInversion(circle, point):
    """Circle inversion of a point.

    pointInversion(Circle, Vector)

    Will calculate the inversed point an return it.
    If the given point is equal to the center of the circle "None" will be returned.

    See also:
    http://en.wikipedia.org/wiki/Inversive_geometry
    """
    if (geomType(circle) == "Circle") and isinstance(point, FreeCAD.Vector):
        cen = circle.Curve.Center
        rad = circle.Curve.Radius

        if DraftVecUtils.equals(cen, point):
            return None

        # Inverse the distance of the point
        # dist(cen -> P) = r^2 / dist(cen -> invP)

        dist = DraftVecUtils.dist(point, cen)
        invDist = rad**2 / d

        invPoint = Vector(0, 0, point.z)
        invPoint.x = cen.x + (point.x - cen.x) * invDist / dist;
        invPoint.y = cen.y + (point.y - cen.y) * invDist / dist;

        return invPoint

    else:
        FreeCAD.Console.PrintMessage("debug: pointInversion bad parameters!\n")
        return None


def polarInversion(circle, edge):
    """Return the inversion pole of a line.

    polarInversion(circle, edge):

    edge ... The polar.
    i.e. The nearest point on the line is inversed.

    http://mathworld.wolfram.com/InversionPole.html
    """

    if (geomType(circle) == "Circle") and (geomType(edge) == "Line"):
        nearest = circle.Curve.Center.add(findDistance(circle.Curve.Center, edge, False))
        if nearest:
            inversionPole = pointInversion(circle, nearest)
            if inversionPole:
                return inversionPole
    else:
        FreeCAD.Console.PrintMessage("debug: circleInversionPole bad parameters!\n")
        return None


def circleInversion(circle, circle2):
    """
    pointInversion(Circle, Circle)

    Circle inversion of a circle.
    """
    if (geomType(circle) == "Circle") and (geomType(circle2) == "Circle"):
        cen1 = circle.Curve.Center
        rad1 = circle.Curve.Radius

        if DraftVecUtils.equals(cen1, point):
            return None

        invCen2 = Inversion(circle, circle2.Curve.Center)

        pointOnCircle2 = Vector.add(circle2.Curve.Center, Vector(circle2.Curve.Radius, 0, 0))
        invPointOnCircle2 = Inversion(circle, pointOnCircle2)

        return Part.Circle(invCen2, norm, DraftVecUtils.dist(invCen2, invPointOnCircle2))

    else:
        FreeCAD.Console.PrintMessage("debug: circleInversion bad parameters!\n")
        return None

##  @}
