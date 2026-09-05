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

"""Apply stored matching constraints without depending on document commands."""

import FreeCAD as App

from .brep import ConversionError
from .controlcage import ControlCage
from .match_support import (
    _support_shape,
    _linked_support,
    _face_match_parameters,
    _face_point,
    _wire_point,
    _wire_match_parameters,
    _apply_match_corner_sharpness,
    _apply_match_corner_creases,
    _surface_tangent_plane,
    _project_to_planes,
    _boundary_tangent_planes,
)


def apply_match_constraints(obj):
    """Keep the stored cage boundary associated with its support face."""
    boundary = tuple(int(index) for index in getattr(obj, "MatchBoundary", ()))
    reference = getattr(obj, "MatchSupport", None)
    support_shape = _support_shape(reference, obj)
    support, tangent_face, support_kind = _linked_support(reference, obj, support_shape)
    if not boundary:
        return
    if support is None:
        raise ValueError("The matched Form support is no longer valid")
    cage = ControlCage.from_object(obj)
    if any(index < 0 or index >= len(cage.vertices) for index in boundary):
        raise ValueError("The matched Form boundary is no longer valid")

    vertices = [App.Vector(*point) for point in cage.vertices]
    boundary_points = [vertices[index] for index in boundary]
    parameters = list(getattr(obj, "MatchParameters", ()))
    expected = len(boundary) * 2 if support_kind == "Face" else len(boundary)
    if len(parameters) != expected:
        parameters = (
            _face_match_parameters(tangent_face, boundary_points)
            if support_kind == "Face"
            else _wire_match_parameters(support, boundary_points)
        )
        obj.MatchParameters = parameters
    projected = {}
    for position, index in enumerate(boundary):
        if support_kind == "Face":
            projected[index] = _face_point(
                tangent_face,
                parameters[position * 2],
                parameters[position * 2 + 1],
            )
        else:
            projected[index] = _wire_point(support, parameters[position])
        vertices[index] = projected[index]

    match_corners = _apply_match_corner_sharpness(obj, cage, boundary, projected, support)
    _apply_match_corner_creases(
        obj,
        cage,
        boundary,
        match_corners,
        str(obj.MatchContinuity) in ("Tangent", "Curvature") and str(obj.MatchTangentMode) == "AdjacentFaces",
    )

    if str(obj.MatchContinuity) == "Curvature":
        from .curvature import solve_curvature_match
        solve_curvature_match(obj)
        return
    obj.MatchStatus = ""

    if str(obj.MatchContinuity) == "Tangent":
        if tangent_face is None:
            raise ValueError("Tangent Match requires a face or planar closed wire")
        boundary_set = set(boundary)
        tangent_planes = (
            _boundary_tangent_planes(reference, tangent_face, support, boundary, projected, obj, support_shape)
            if support_kind == "Face" and str(obj.MatchTangentMode) == "AdjacentFaces"
            else {index: [] for index in boundary}
        )
        boundary_center = sum(projected.values(), App.Vector()) / len(projected)
        form_center = sum(vertices, App.Vector()) / len(vertices)
        boundary_spacing = sum(
            (projected[boundary[(index + 1) % len(boundary)]] - projected[vertex]).Length
            for index, vertex in enumerate(boundary)
        ) / len(boundary)
        boundary_edges = {
            tuple(sorted((boundary[index], boundary[(index + 1) % len(boundary)])))
            for index in range(len(boundary))
        }
        candidates = {}
        for edge in cage.edge_counts():
            if edge in boundary_edges:
                continue
            first, second = edge
            if (first in boundary_set) == (second in boundary_set):
                continue
            boundary_index = first if first in boundary_set else second
            interior_index = second if first in boundary_set else first
            point = projected[boundary_index]
            planes = tangent_planes.get(boundary_index) or [
                _surface_tangent_plane(tangent_face, point)
            ]
            target = _project_to_planes(vertices[interior_index], planes)
            if target.sub(point).Length <= 1.0e-9:
                inward = boundary_center - point
                handle = min(
                    vertices[interior_index].sub(point).Length,
                    boundary_spacing / 3.0,
                )
                if inward.Length > 1.0e-9:
                    inward = inward * (handle / inward.Length)
                    target = _project_to_planes(point + inward, planes)
                if target.sub(point).Length <= 1.0e-9:
                    selected_normal = _surface_tangent_plane(tangent_face, point)[0]
                    direction = (
                        1.0
                        if form_center.sub(boundary_center).dot(selected_normal) >= 0.0
                        else -1.0
                    )
                    target = _project_to_planes(
                        point + selected_normal * (direction * handle), planes
                    )
                if target.sub(point).Length <= 1.0e-9:
                    raise ConversionError("Could not determine a tangent Match direction")
            candidates.setdefault(interior_index, []).append(target)
        for index, points in candidates.items():
            vertices[index] = sum(points, App.Vector()) / len(points)

    obj.ControlPoints = vertices
