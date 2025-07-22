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
"""Provides various incomplete functions for creating circles.

These functions cannot be used because they require that certain functions
to calculate shapes exist first.

These functions are meant to create circles starting from three values,
either a tangent edge, a point, or a radius. Various combinations
are possible.

- Two tangents and a point.
- Two tangents and a radius.
- A tangent and two points.
- A tangent, a point, and a radius.
- Three tangents.

In this case, the word 'tangent' refers to either a straight line
or to a circular edge, so even more combinations are possible.

- Two tangents and a point.
  * Line, line, and point.
  * Circle, line, and point.
  * Line, circle, and point.
  * Circle, circle, and point.

And so on, with the other combinations.
"""
## @package circles_incomplete
# \ingroup draftgeoutils
# \brief Provides various incomplete functions for creating circles.

## \addtogroup draftgeoutils
# @{
import FreeCAD

from draftutils.messages import _wrn
from draftgeoutils.general import geomType
from draftgeoutils.circles import (circlefrom2Lines1Point,
                                   circleFrom2LinesRadius,
                                   circlefrom1Line2Points,
                                   circleFromPointLineRadius,
                                   circleFrom3LineTangents)
from draftgeoutils.circles_apollonius import circleFrom3CircleTangents


def circlefromCircleLinePoint(circle, line, point):
    """Do nothing. Placeholder function. Needs to be implemented."""
    _wrn("Placeholder function, does nothing.")


def circlefrom2Circles1Point(circle1, circle2, point):
    """Do nothing. Placeholder function. Needs to be implemented."""
    _wrn("Placeholder function, does nothing.")


def circleFrom2tan1pt(tan1, tan2, point):
    """Circle from two tangents and one point.

    The tangents should be edges, and they may be either straight line edges
    or circular edges, so four combinations are possible.
    """
    if (geomType(tan1) == "Line"
            and geomType(tan2) == "Line"
            and isinstance(point, FreeCAD.Vector)):
        return circlefrom2Lines1Point(tan1, tan2, point)

    elif (geomType(tan1) == "Circle"
          and geomType(tan2) == "Line"
          and isinstance(point, FreeCAD.Vector)):
        return circlefromCircleLinePoint(tan1, tan2, point)

    elif (geomType(tan2) == "Circle"
          and geomType(tan1) == "Line"
          and isinstance(point, FreeCAD.Vector)):
        return circlefromCircleLinePoint(tan2, tan1, point)

    elif (geomType(tan2) == "Circle"
          and geomType(tan1) == "Circle"
          and isinstance(point, FreeCAD.Vector)):
        return circlefrom2Circles1Point(tan2, tan1, point)


def circleFromCircleLineRadius(circle, line, radius):
    """Do nothing. Placeholder function. Needs to be implemented."""
    _wrn("Placeholder function, does nothing.")


def circleFrom2CirclesRadius(circle1, circle2, radius):
    """Do nothing. Placeholder function. Needs to be implemented."""
    _wrn("Placeholder function, does nothing.")


def circleFrom2tan1rad(tan1, tan2, rad):
    """Circle from two tangents and one radius.

    The tangents should be edges, and they may be either straight line edges
    or circular edges, so four combinations are possible.
    """
    if geomType(tan1) == "Line" and geomType(tan2) == "Line":
        return circleFrom2LinesRadius(tan1, tan2, rad)

    elif geomType(tan1) == "Circle" and geomType(tan2) == "Line":
        return circleFromCircleLineRadius(tan1, tan2, rad)

    elif geomType(tan1) == "Line" and geomType(tan2) == "Circle":
        return circleFromCircleLineRadius(tan2, tan1, rad)

    elif geomType(tan1) == "Circle" and geomType(tan2) == "Circle":
        return circleFrom2CirclesRadius(tan1, tan2, rad)


def circlefrom1Circle2Points(circle, point1, point2):
    """Do nothing. Placeholder function. Needs to be implemented."""
    _wrn("Placeholder function, does nothing.")


def circleFrom1tan2pt(tan1, p1, p2):
    """Circle from one tangent and two points.

    The tangents should be edges, and they may be either straight line edges
    or circular edges, so two combinations are possible.
    """
    if (geomType(tan1) == "Line"
            and isinstance(p1, FreeCAD.Vector)
            and isinstance(p2, FreeCAD.Vector)):
        return circlefrom1Line2Points(tan1, p1, p2)

    elif (geomType(tan1) == "Circle"
            and isinstance(p1, FreeCAD.Vector)
            and isinstance(p2, FreeCAD.Vector)):
        return circlefrom1Circle2Points(tan1, p1, p2)


def circleFromPointCircleRadius(point, circle, radius):
    """Do nothing. Placeholder function. Needs to be implemented."""
    _wrn("Placeholder function, does nothing.")


def circleFrom1tan1pt1rad(tan1, p1, rad):
    """Circle from one tangent, one point, and radius.

    The tangents should be edges, and they may be either straight line edges
    or circular edges, so two combinations are possible.
    """
    if geomType(tan1) == "Line" and isinstance(p1, FreeCAD.Vector):
        return circleFromPointLineRadius(p1, tan1, rad)

    elif geomType(tan1) == "Circle" and isinstance(p1, FreeCAD.Vector):
        return circleFromPointCircleRadius(p1, tan1, rad)


def circleFrom1Circle2Lines(circle, line1, line2):
    """Do nothing. Placeholder function. Needs to be implemented."""
    _wrn("Placeholder function, does nothing.")


def circleFrom2Circle1Lines(circle1, circle2, line):
    """Do nothing. Placeholder function. Needs to be implemented."""
    _wrn("Placeholder function, does nothing.")


def circleFrom3tan(tan1, tan2, tan3):
    """Circle from three tangents.

    The tangents should be edges, and they may be either straight line edges
    or circular edges, so eight combinations are possible.
    """
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

    elif tan1IsCircle and tan2IsLine and tan3IsLine:
        return circleFrom1Circle2Lines(tan1, tan2, tan3)

    elif tan1IsLine and tan2IsCircle and tan3IsLine:
        return circleFrom1Circle2Lines(tan2, tan1, tan3)

    elif tan1IsLine and tan2IsLine and tan3IsCircle:
        return circleFrom1Circle2Lines(tan3, tan1, tan2)

    elif tan1IsLine and tan2IsCircle and tan3IsCircle:
        return circleFrom2Circle1Lines(tan2, tan3, tan1)

    elif tan1IsCircle and tan2IsLine and tan3IsCircle:
        return circleFrom2Circle1Lines(tan1, tan3, tan2)

    elif tan1IsCircle and tan2IsCircle and tan3IsLine:
        return circleFrom2Circle1Lines(tan1, tan2, tan3)

## @}
