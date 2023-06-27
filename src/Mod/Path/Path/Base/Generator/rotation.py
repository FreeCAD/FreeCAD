# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2021 sliptonic <shopinthewoods@gmail.com>               *
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


# Technical Debt.  This generator currently assumes 3+2 axis rotation of CA.
# The main generator function should be extended to include other flavors of 3+2


import math
import Path
import FreeCAD
from enum import Enum

__title__ = "Rotation Path Generator"
__author__ = "sliptonic (Brad Collette)"
__url__ = "https://www.freecad.org"
__doc__ = "Generates the rotation toolpath"


if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


class refAxis(Enum):
    x = FreeCAD.Vector(1, 0, 0)
    y = FreeCAD.Vector(0, 1, 0)
    z = FreeCAD.Vector(0, 0, 1)


def relAngle(vec, ref):
    """
    Takes a vector and a reference axis (refAxis) vector. Calculates the
    relative angle.  The result is returned in degrees (plus or minus)
    """

    Path.Log.debug("vec: {}  ref: {}".format(vec, ref))
    norm = vec * 1  # copy vec so we don't alter original

    if ref == refAxis.x:
        plane = refAxis.y.value
    elif ref == refAxis.y:
        plane = refAxis.z.value
    else:
        plane = refAxis.x.value

    norm.projectToPlane(FreeCAD.Vector(0, 0, 0), plane)

    ref = ref.value
    rot = FreeCAD.Rotation(norm, ref)
    ang = math.degrees(rot.Angle)
    angle = ang * plane.dot(rot.Axis)
    Path.Log.debug("relative ang: {}".format(angle))

    return angle


def __getCRotation(normalVector, cMin=-360, cMax=360):
    """
    Calculate the valid C axis rotations component to align the normalVector
    with either the +y or -y axis.
    multiple poses may be possible.  Returns a list of all valid poses
    """
    Path.Log.debug(
        "normalVector: {} cMin: {} cMax: {}".format(normalVector, cMin, cMax)
    )

    angle = relAngle(normalVector, refAxis.y)

    # Given an angle, there are four possibilities; rotating +- to each of the
    # two axes +y and -y
    candidates = [angle]
    if angle == 0:
        candidates.append(180)
    elif angle == 180:
        candidates.append(0)
    elif angle >= 0:
        candidates.append(angle - 180)
        candidates.append(180 + angle)
        candidates.append(angle - 360)
    else:
        candidates.append(angle + 180)
        candidates.append(-180 + angle)
        candidates.append(angle + 360)

    # final results are candidates that don't violate rotation limits
    results = [c for c in candidates if c >= cMin and c <= cMax]

    return results


def __getARotation(normalVector, aMin=-360, aMax=360):
    """
    Calculate the A axis rotation component.
    Final rotation is always assumed to be around +X. The sign of the returned
    value indicates direction of rotation.

    Returns None if rotation violates min/max constraints
    """

    angle = relAngle(normalVector, refAxis.z)

    # only return a result if it doesn't violate rotation constraints
    if angle > aMin and angle <= aMax:
        return angle
    else:
        return None


def generate(normalVector, aMin=-360, aMax=360, cMin=-360, cMax=360, compound=False):
    """
    Generates Gcode rotation to align a vector (alignVector) with the positive Z axis.

    It first rotates around the Z axis (C rotation)
    to align the vector the positive Y axis. Then around the X axis
    (A rotation).

    The min and max arguments dictate the range of motion allowed rotation in
    the respective axis.
    Default assumes continuous rotation.

    Returns a list of path commands for the shortest valid solution

    If compound is False, axis moves will be broken out to individual commands

    The normalVector input from a typical face (f) can be obtained like this:

        u, v = f.ParameterRange[:2]
        n = f.normalAt(u,v)
        plm = obj.getGlobalPlacement()
        rot = plm.Rotation
        normalVector = rot.multVec(n
    """

    Path.Log.track(
        "\n=============\n normalVector: {}\n aMin: {}\n aMax: {}\n cMin: {}\n cMax: {}".format(
            normalVector, aMin, aMax, cMin, cMax
        )
    )

    # Calculate C rotation
    cResults = __getCRotation(normalVector, cMin, cMax)
    Path.Log.debug("C Rotation results {}".format(cResults))

    solutions = []
    for result in cResults:

        # calculate a new vector based on the result
        rot = FreeCAD.Rotation(FreeCAD.Vector(0, 0, 1), result)
        newvec = rot.multVec(normalVector)

        # Get the candidate A rotation for the new vector
        aResult = __getARotation(newvec, aMin, aMax)

        Path.Log.debug(
            "\n=====\nFor C Rotation: {}\n Calculated A {}\n".format(result, aResult)
        )

        if aResult is not None:
            solutions.append({"A": aResult, "C": result})

    if len(solutions) == 0:  # No valid solution found
        raise ValueError("No valid rotation solution found")

    # find pose with the shortest transit length
    best = solutions[0]
    curlen = math.fabs(best["A"]) + math.fabs(best["C"])
    for solution in solutions[1:]:
        testlen = math.fabs(solution["A"]) + math.fabs(solution["C"])
        if testlen < curlen:
            best = solution
            curlen = testlen

    Path.Log.debug("best result: {}".format(best))

    # format and return rotation commands
    commands = []
    if compound:
        commands.append(Path.Command("G0", best))
    else:
        for key, val in best.items():
            print(key, val)
            commands.append(Path.Command("G0", {key: val}))

    return commands
