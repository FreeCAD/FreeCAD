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
"""Provides various functions for inversive operations using a circle.

http://en.wikipedia.org/wiki/Inversive_geometry
"""
## @package circle_inversion
# \ingroup draftgeoutils
# \brief Provides various functions for inversive geometry operations.

import lazy_loader.lazy_loader as lz

import FreeCAD as App
import DraftVecUtils

from draftgeoutils.general import NORM, geomType
from draftgeoutils.geometry import findDistance

# Delay import of module until first use because it is heavy
Part = lz.LazyLoader("Part", globals(), "Part")

## \addtogroup draftgeoutils
# @{


def pointInversion(circle, point):
    """Return the circle inversion of a point.

    It will return `None` if the given point is equal to the center
    of the circle.
    """
    if geomType(circle) != "Circle" or isinstance(point, App.Vector):
        print("debug: pointInversion bad parameters!")
        return None

    cen = circle.Curve.Center
    rad = circle.Curve.Radius

    if DraftVecUtils.equals(cen, point):
        return None

    # Inverse the distance of the point
    # dist(cen -> P) = r^2 / dist(cen -> invP)

    dist = DraftVecUtils.dist(point, cen)
    invDist = rad**2 / dist

    invPoint = App.Vector(0, 0, point.z)
    invPoint.x = cen.x + (point.x - cen.x) * invDist / dist
    invPoint.y = cen.y + (point.y - cen.y) * invDist / dist

    return invPoint


def polarInversion(circle, edge):
    """Return the inversion pole of a line. The edge is the polar.

    The nearest point on the line is inversed.

    http://mathworld.wolfram.com/InversionPole.html
    """
    if geomType(circle) != "Circle" or geomType(edge) != "Line":
        print("debug: circleInversionPole bad parameters! Must be a circle.")
        return None

    nearest = circle.Curve.Center.add(findDistance(circle.Curve.Center,
                                                   edge,
                                                   False))
    if nearest:
        inversionPole = pointInversion(circle, nearest)
        if inversionPole:
            return inversionPole

    return None


def circleInversion(circle, circle2):
    """Circle inversion of a circle, inverting the center point.

    Returns the new circle created from the inverted center of circle2.
    """
    if geomType(circle) != "Circle" or geomType(circle2) != "Circle":
        print("debug: circleInversion bad parameters! Must be circles.")
        return None

    cen1 = circle.Curve.Center

    cen2 = circle2.Curve.Center
    rad2 = circle2.Curve.Radius

    if DraftVecUtils.equals(cen1, cen2):
        return None

    invCen2 = pointInversion(circle, cen2)

    pointOnCircle2 = App.Vector.add(cen2, App.Vector(rad2, 0, 0))
    invPointOnCircle2 = pointInversion(circle, pointOnCircle2)

    return Part.Circle(invCen2,
                       NORM,
                       DraftVecUtils.dist(invCen2, invPointOnCircle2))

## @}
