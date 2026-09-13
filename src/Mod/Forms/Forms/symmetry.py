# SPDX-License-Identifier: LGPL-2.1-or-later
# /**************************************************************************
#                                                                           *
#    Copyright (c) 2026 AstoCAD     <hello@astocad.com>                     *
#                                                                           *
#    This file is part of FreeCAD.                                          *
#                                                                           *
#    FreeCAD is free software: you can redistribute it and/or modify it     *
#    under the terms of the GNU Lesser General Public License as            *
#    published by the Free Software Foundation, either version 2.1 of the   *
#    License, or (at your option) any later version.                        *
#                                                                           *
#    FreeCAD is distributed in the hope that it will be useful, but         *
#    WITHOUT ANY WARRANTY; without even the implied warranty of             *
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
#    Lesser General Public License for more details.                        *
#                                                                           *
#    You should have received a copy of the GNU Lesser General Public       *
#    License along with FreeCAD. If not, see                                *
#    <https://www.gnu.org/licenses/>.                                       *
#                                                                           *
# **************************************************************************/

"""Shared geometric symmetry mapping for Forms controls and topology.

Symmetry is deliberately derived from the editable control topology rather
than from the primitive that originally created it.  Consequently local
T-mesh vertices and faces participate in exactly the same mapping as the base
cage after Insert Edge or Subdivide.
"""

import math

_PLANE_AXES = {"XY": 2, "XZ": 1, "YZ": 0}


def object_axis(obj):
    """Return the reflected coordinate axis, or ``None`` when disabled."""
    if not bool(getattr(obj, "Symmetric", False)):
        return None
    return _PLANE_AXES.get(str(getattr(obj, "SymmetryPlane", "YZ")), 0)


def reflected(point, axis, center=0.0):
    result = list(point)
    result[int(axis)] = 2.0 * float(center) - result[int(axis)]
    return tuple(result)


def _tolerance(points):
    points = list(points)
    if not points:
        return 1.0e-7
    diagonal = math.sqrt(
        sum(
            (max(point[axis] for point in points) - min(point[axis] for point in points)) ** 2
            for axis in range(3)
        )
    )
    return max(diagonal * 1.0e-7, 1.0e-7)


def vertex_map(vertices, axis, center=0.0, strict=True):
    """Map every vertex ID to the closest reflected control vertex."""
    vertices = {int(index): tuple(point) for index, point in dict(vertices).items()}
    tolerance = _tolerance(vertices.values())
    result = {}
    for vertex_id, point in vertices.items():
        target = reflected(point, axis, center)
        match, distance = min(
            (
                (candidate_id, math.dist(target, candidate))
                for candidate_id, candidate in vertices.items()
            ),
            key=lambda item: item[1],
        )
        if distance <= tolerance or not strict:
            result[vertex_id] = match
        elif strict:
            raise ValueError(
                "Symmetric topology has no matching control point on the opposite side"
            )
    return result


def control_pairs(vertices, axis, center=0.0, strict=True):
    """Return positive/negative ID pairs and controls lying on the plane."""
    vertices = {int(index): tuple(point) for index, point in dict(vertices).items()}
    mapping = vertex_map(vertices, axis, center, strict=strict)
    tolerance = _tolerance(vertices.values())
    pairs = []
    plane = []
    for vertex_id, point in vertices.items():
        if abs(point[axis] - center) <= tolerance:
            plane.append(vertex_id)
        elif point[axis] > center and mapping[vertex_id] != vertex_id:
            pairs.append((vertex_id, mapping[vertex_id]))
    return pairs, plane


def enforce_vertices(vertices, axis, center=0.0):
    """Return a copy with negative controls reflected from positive controls."""
    result = {int(index): tuple(point) for index, point in dict(vertices).items()}
    pairs, plane = control_pairs(result, axis, center)
    for positive, negative in pairs:
        result[negative] = reflected(result[positive], axis, center)
    for vertex_id in plane:
        point = list(result[vertex_id])
        point[axis] = float(center)
        result[vertex_id] = tuple(point)
    return result


def mirror_faces(vertices, faces, face_ids, axis, center=0.0):
    """Expand selected face IDs with their reflected topology partners."""
    faces = {int(face_id): tuple(boundary) for face_id, boundary in dict(faces).items()}
    mapping = vertex_map(vertices, axis, center)
    by_vertices = {frozenset(boundary): face_id for face_id, boundary in faces.items()}
    result = set(int(face_id) for face_id in face_ids)
    for face_id in tuple(result):
        if face_id not in faces:
            raise ValueError("Symmetry references an unknown control face")
        target = frozenset(mapping[vertex] for vertex in faces[face_id])
        mirrored = by_vertices.get(target)
        if mirrored is None:
            raise ValueError("Symmetric topology has no matching face on the opposite side")
        result.add(mirrored)
    return tuple(sorted(result))


def mirror_edges(vertices, edges, axis, center=0.0):
    """Expand control edges with their reflected partners."""
    mapping = vertex_map(vertices, axis, center)
    result = {tuple(sorted((int(edge[0]), int(edge[1])))) for edge in edges}
    result.update(
        tuple(sorted((mapping[first], mapping[second]))) for first, second in tuple(result)
    )
    return result
