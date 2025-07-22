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
"""Provides various functions to work with cubic shapes (parallelepipeds)."""
## @package cuboids
# \ingroup draftgeoutils
# \brief Provides various functions for cubic shapes (parallelepipeds).

## \addtogroup draftgeoutils
# @{
import math

import FreeCAD as App
import DraftVecUtils

from draftgeoutils.general import geomType, vec, precision


def isCubic(shape):
    """Return True if the shape is a parallelepiped (cuboid).

    A parallelepiped of cube-like shape has 8 vertices, 6 faces, 12 edges,
    and all angles are 90 degrees between its edges.
    """
    # first we try fast methods
    if (len(shape.Vertexes) != 8
            or len(shape.Faces) != 6
            or len(shape.Edges) != 12):
        return False

    for e in shape.Edges:
        if geomType(e) != "Line":
            return False

    # if ok until now, let's do more advanced testing
    for f in shape.Faces:
        if len(f.Edges) != 4:
            return False

        for i in range(4):
            e1 = vec(f.Edges[i])
            if i < 3:
                e2 = vec(f.Edges[i+1])
            else:
                e2 = vec(f.Edges[0])
            rpi = [0.0, round(math.pi/2, precision())]
            if round(e1.getAngle(e2), precision()) not in rpi:
                return False

    return True


def getCubicDimensions(shape):
    """Return a list containing the placement, and dimensions of the shape.

    The dimensios are length, width and height of a the parallelepiped,
    rounded to the value indicated by `precision`.
    The placement point is the lowest corner of the shape.

    If it is not a parallelepiped (cuboid), return None.
    """
    if not isCubic(shape):
        return None

    # determine lowest face, which will be our base
    z = [10, 1000000000000]
    for i in range(len(shape.Faces)):
        if shape.Faces[i].CenterOfMass.z < z[1]:
            z = [i, shape.Faces[i].CenterOfMass.z]

    if z[0] > 5:
        return None

    base = shape.Faces[z[0]]
    basepoint = base.Edges[0].Vertexes[0].Point
    plpoint = base.CenterOfMass
    # basenorm = base.normalAt(0.5, 0.5)

    # getting length and width
    vx = vec(base.Edges[0])
    vy = vec(base.Edges[1])
    if round(vx.Length) == round(vy.Length):
        vy = vec(base.Edges[2])

    # getting rotations
    rotZ = DraftVecUtils.angle(vx)
    rotY = DraftVecUtils.angle(vx, App.Vector(vx.x, vx.y, 0))
    rotX = DraftVecUtils.angle(vy, App.Vector(vy.x, vy.y, 0))

    # getting height
    vz = None
    rpi = round(math.pi/2, precision())
    for i in range(1, 6):
        for e in shape.Faces[i].Edges:
            if basepoint in [e.Vertexes[0].Point, e.Vertexes[1].Point]:
                vtemp = vec(e)
                # print(vtemp)
                if round(vtemp.getAngle(vx), precision()) == rpi:
                    if round(vtemp.getAngle(vy), precision()) == rpi:
                        vz = vtemp

    if not vz:
        return None

    mat = App.Matrix()
    mat.move(plpoint)
    mat.rotateX(rotX)
    mat.rotateY(rotY)
    mat.rotateZ(rotZ)

    return [App.Placement(mat),
            round(vx.Length, precision()),
            round(vy.Length, precision()),
            round(vz.Length, precision())]

## @}
