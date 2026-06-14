# SPDX-License-Identifier: LGPL-2.1-or-later

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

import math
import lazy_loader.lazy_loader as lz

import FreeCAD as App

from draftgeoutils.general import geomType, vec, precision

# Delay import of module until first use because it is heavy
Part = lz.LazyLoader("Part", globals(), "Part")

## \addtogroup draftgeoutils
# @{


def isCubic(shape):
    """Return True if the shape is a parallelepiped (cuboid).

    A parallelepiped of cube-like shape has 8 vertices, 6 faces, 12 edges,
    and all angles are 90 degrees between its edges.
    """
    # first we try fast methods
    if len(shape.Vertexes) != 8 or len(shape.Faces) != 6 or len(shape.Edges) != 12:
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
                e2 = vec(f.Edges[i + 1])
            else:
                e2 = vec(f.Edges[0])
            rpi = [0.0, round(math.pi / 2, precision())]
            if round(e1.getAngle(e2), precision()) not in rpi:
                return False

    return True


def getCubicDimensions(shape):
    """Return a list containing the placement, and dimensions of the shape.

    The dimensions are length, width and height of a the parallelepiped, rounded to
    the value indicated by `precision`. With the length larger than or equal to width.
    The placement point is the center of the lowest face.

    If it is not a parallelepiped (cuboid), return None.
    """

    def _edge_vector(base_point, edge):
        """Get a unit-vector from the vertices of an edge. The base_point
        must match one of these points and determines the start point."""
        points = [v.Point for v in edge.Vertexes]
        if points[1].isEqual(base_point, 1e-6):
            points.reverse()
        return (points[1] - points[0]).normalize()

    if not isCubic(shape):
        return None

    # determine lowest face, which will be our base
    z = [10, 1000000000000]
    for i in range(len(shape.Faces)):
        if shape.Faces[i].CenterOfMass.z < z[1]:
            z = [i, shape.Faces[i].CenterOfMass.z]

    if z[0] > 5:
        return None

    base_face = shape.Faces[z[0]]
    base_norm = base_face.normalAt(0, 0)
    base_vertex = base_face.Vertexes[0]
    corner_edges = shape.ancestorsOfType(base_vertex, Part.Edge)
    edge_z = None
    for edge in corner_edges:
        edge_vector = vec(edge).normalize()
        if edge_vector.isEqual(base_norm, 1e-6) or edge_vector.isEqual(-base_norm, 1e-6):
            edge_z = edge
            break

    if edge_z is None:
        return None

    corner_edges.remove(edge_z)
    edge_x, edge_y = corner_edges
    if edge_x.Length < edge_y.Length:
        edge_x, edge_y = edge_y, edge_x

    base_point = base_vertex.Point
    rot = App.Rotation(
        _edge_vector(base_point, edge_x),
        _edge_vector(base_point, edge_y),
        _edge_vector(base_point, edge_z),
        "ZXY",
    )

    return [
        App.Placement(base_face.CenterOfMass, rot),
        round(edge_x.Length, precision()),
        round(edge_y.Length, precision()),
        round(edge_z.Length, precision()),
    ]


## @}
