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
"""Provides various functions for Apollonius and Soddy circle operations.

The 'problem of Appollonius' consists of finding a circle that is
simultaneously tangent to three circles on a plane.

- http://en.wikipedia.org/wiki/Problem_of_Apollonius#Inversive_methods
- http://mathworld.wolfram.com/ApolloniusCircle.html
- http://mathworld.wolfram.com/ApolloniusProblem.html

If each circle only has one intersection with each other, the problem is that
of finding a 'Soddy circle', which has two solutions, and inner circle
and an outer circle.

- http://en.wikipedia.org/wiki/Problem_of_Apollonius#Mutually_tangent_given_circles:_Soddy.27s_circles_and_Descartes.27_theorem
- http://mathworld.wolfram.com/SoddyCircles.html
- http://mathworld.wolfram.com/InnerSoddyCenter.html
- http://mathworld.wolfram.com/OuterSoddyCenter.html
"""
## @package circles_apollonius
# \ingroup draftgeoutils
# \brief Provides various functions for Apollonius and Soddy circles.

import cmath
import math
import lazy_loader.lazy_loader as lz

import FreeCAD as App

from draftgeoutils.general import geomType, NORM
from draftgeoutils.intersections import findIntersection

# Delay import of module until first use because it is heavy
Part = lz.LazyLoader("Part", globals(), "Part")

## \addtogroup draftgeoutils
# @{


def outerSoddyCircle(circle1, circle2, circle3):
    """Compute the outer soddy circle for three tightly packed circles.

    Original Java code Copyright (rc) 2008 Werner Randelshofer
    Converted to python by Martin Buerbaum 2009
    http://www.randelshofer.ch/treeviz/
    Either Creative Commons Attribution 3.0, the MIT license,
    or the GNU Lesser General License LGPL.
    """
    if (geomType(circle1) != "Circle"
            or geomType(circle2) != "Circle"
            or geomType(circle3) != "Circle"):
        print("debug: outerSoddyCircle bad parameters! Must be circles.")
        return None

    A = circle1.Curve.Center
    B = circle2.Curve.Center
    C = circle3.Curve.Center

    ra = circle1.Curve.Radius
    rb = circle2.Curve.Radius
    rc = circle3.Curve.Radius

    # Solution using Descartes' theorem, as described here:
    # http://en.wikipedia.org/wiki/Descartes%27_theorem
    k1 = 1 / ra
    k2 = 1 / rb
    k3 = 1 / rc
    k4 = abs(k1 + k2 + k3 - 2 * math.sqrt(k1 * k2 + k2 * k3 + k3 * k1))

    q1 = (k1 + 0j) * (A.x + A.y * 1j)
    q2 = (k2 + 0j) * (B.x + B.y * 1j)
    q3 = (k3 + 0j) * (C.x + C.y * 1j)

    temp = ((q1 * q2) + (q2 * q3) + (q3 * q1))
    q4 = q1 + q2 + q3 - ((2 + 0j) * cmath.sqrt(temp))

    z = q4 / (k4 + 0j)

    # If the formula is not solvable, we return no circle.
    if not z or not (1 / k4):
        return None

    X = -z.real
    Y = -z.imag
    print("Outer Soddy circle: " + str(X) + " " + str(Y) + "\n")  # Debug

    # The Radius of the outer soddy circle can also be calculated
    # with the following formula:
    # radiusOuter = abs(r1*r2*r3 / (r1*r2 + r1*r3 + r2*r3 - 2 * math.sqrt(r1*r2*r3 * (r1+r2+r3))))
    circ = Part.Circle(App.Vector(X, Y, A.z), NORM, 1 / k4)

    return circ


def innerSoddyCircle(circle1, circle2, circle3):
    """Compute the inner soddy circle for three tightly packed circles.

    Original Java code Copyright (rc) 2008 Werner Randelshofer
    Converted to python by Martin Buerbaum 2009
    http://www.randelshofer.ch/treeviz/
    """
    if (geomType(circle1) != "Circle"
            and geomType(circle2) != "Circle"
            and geomType(circle3) != "Circle"):
        print("debug: innerSoddyCircle bad parameters! Must be circles.")
        return None

    A = circle1.Curve.Center
    B = circle2.Curve.Center
    C = circle3.Curve.Center

    ra = circle1.Curve.Radius
    rb = circle2.Curve.Radius
    rc = circle3.Curve.Radius

    # Solution using Descartes' theorem, as described here:
    # http://en.wikipedia.org/wiki/Descartes%27_theorem
    k1 = 1 / ra
    k2 = 1 / rb
    k3 = 1 / rc
    k4 = abs(k1 + k2 + k3 + 2 * math.sqrt(k1 * k2 + k2 * k3 + k3 * k1))

    q1 = (k1 + 0j) * (A.x + A.y * 1j)
    q2 = (k2 + 0j) * (B.x + B.y * 1j)
    q3 = (k3 + 0j) * (C.x + C.y * 1j)

    temp = ((q1 * q2) + (q2 * q3) + (q3 * q1))
    q4 = q1 + q2 + q3 + ((2 + 0j) * cmath.sqrt(temp))

    z = q4 / (k4 + 0j)

    # If the formula is not solvable, we return no circle.
    if (not z or not (1 / k4)):
        return None

    X = z.real
    Y = z.imag
    print("Outer Soddy circle: " + str(X) + " " + str(Y) + "\n")  # Debug

    # The Radius of the inner soddy circle can also be calculated
    # with the following formula:
    # radiusInner = abs(r1*r2*r3 / (r1*r2 + r1*r3 + r2*r3 + 2 * math.sqrt(r1*r2*r3 * (r1+r2+r3))))
    circ = Part.Circle(App.Vector(X, Y, A.z), NORM, 1 / k4)

    return circ


def circleFrom3CircleTangents(circle1, circle2, circle3):
    """Return the circle that is tangent to three other circles.

    This problem is called the 'Problem of Appollonius'.

    A special case is that of 'Soddy circles'.

    To Do
    -----
    Currently not all possible solutions are found, only the Soddy circles.

    * Calc all 6 homothetic centers.
    * Create 3 lines from the inner and 4 from the outer h. center.
    * Calc. the 4 inversion poles of these lines for each circle.
    * Calc. the radical center of the 3 circles.
    * Calc. the intersection points (max. 8) of 4 lines (through each
      inversion pole and the radical center) with the circle.
    * This gives us all the tangent points.
    """
    if (geomType(circle1) != "Circle"
            and geomType(circle2) != "Circle"
            and geomType(circle3) == "Circle"):
        print("debug: circleFrom3CircleTangents bad input! Must be circles")
        return None

    int12 = findIntersection(circle1, circle2, True, True)
    int23 = findIntersection(circle2, circle3, True, True)
    int31 = findIntersection(circle3, circle1, True, True)

    if int12 and int23 and int31:
        if len(int12) == 1 and len(int23) == 1 and len(int31) == 1:
            # If only one intersection with each circle, Soddy circle

            # r1 = circle1.Curve.Radius
            # r2 = circle2.Curve.Radius
            # r3 = circle3.Curve.Radius
            outerSoddy = outerSoddyCircle(circle1, circle2, circle3)
            # print(str(outerSoddy))  # Debug

            innerSoddy = innerSoddyCircle(circle1, circle2, circle3)
            # print(str(innerSoddy))  # Debug

            circles = []
            if outerSoddy:
                circles.append(outerSoddy)
            if innerSoddy:
                circles.append(innerSoddy)
            return circles

        # Here the rest of the circles should be calculated
        # ...
    else:
        # Some circles are inside each other or an error has occurred.
        return None

## @}
