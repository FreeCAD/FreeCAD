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

"""Convert Catmull-Clark control cages into OCCT BRep solids."""

import math
import json
from dataclasses import dataclass

import FreeCAD as App
import Part

from .topology import (
    catmull_clark_limit_points,
    catmull_clark_patch_grids,
    catmull_clark_step_details,
)
from .tmesh import HierarchicalTMesh


class ConversionError(RuntimeError):
    """Raised when a control cage cannot produce a valid BRep."""


@dataclass(frozen=True)
class LocalEdgeInsert:
    """One exact split of a logical quad patch, persisted on a Forms object."""

    face: tuple
    edge: tuple
    position: float
    points: tuple = ()

    def encode(self):
        return json.dumps(
            {
                "face": list(self.face),
                "edge": list(self.edge),
                "position": float(self.position),
                "points": list(self.points),
            },
            separators=(",", ":"),
            sort_keys=True,
        )

    @classmethod
    def decode(cls, value):
        data = json.loads(str(value))
        face = tuple(int(index) for index in data["face"])
        edge = tuple(sorted(int(index) for index in data["edge"]))
        position = float(data["position"])
        points = tuple(int(index) for index in data.get("points", ()))
        if (
            len(face) != 4
            or len(edge) != 2
            or len(points) not in (0, 2)
            or any(index < 0 for index in points)
            or not 0.0 < position < 1.0
        ):
            raise ValueError("Invalid local edge-insert record")
        return cls(face, edge, position, points)


def decode_local_edge_inserts(values):
    """Decode persisted local split records, rejecting malformed data."""
    return [LocalEdgeInsert.decode(value) for value in (values or ())]


def _validate_closed_quad_cage(vertices, faces):
    if not vertices or not faces:
        raise ConversionError("The control cage is empty")
    edge_counts = {}
    for face in faces:
        if len(face) != 4:
            raise ConversionError("BRep conversion currently requires quad faces")
        for position, start in enumerate(face):
            end = face[(position + 1) % 4]
            if start == end or start < 0 or end < 0:
                raise ConversionError("The control cage contains an invalid edge")
            if start >= len(vertices) or end >= len(vertices):
                raise ConversionError("A control-cage face index is out of range")
            edge = tuple(sorted((start, end)))
            edge_counts[edge] = edge_counts.get(edge, 0) + 1
    if any(count != 2 for count in edge_counts.values()):
        raise ConversionError("A solid requires a closed manifold control cage")


def _validate_open_quad_cage(vertices, faces):
    if not vertices or not faces:
        raise ConversionError("The control cage is empty")
    edge_counts = {}
    for face in faces:
        if len(face) != 4:
            raise ConversionError("BRep conversion currently requires quad faces")
        for position, start in enumerate(face):
            end = face[(position + 1) % 4]
            if start == end or start < 0 or end < 0:
                raise ConversionError("The control cage contains an invalid edge")
            if start >= len(vertices) or end >= len(vertices):
                raise ConversionError("A control-cage face index is out of range")
            edge = tuple(sorted((start, end)))
            edge_counts[edge] = edge_counts.get(edge, 0) + 1
    if any(count > 2 for count in edge_counts.values()):
        raise ConversionError("A surface requires a manifold control cage")
    if not any(count == 1 for count in edge_counts.values()):
        raise ConversionError("The surface control cage has no boundary")


def _prepare_polygon_cage(
    vertices, faces, edge_sharpness, vertex_sharpness, dissolved_edges, closed
):
    """Refine an n-gon cage once into equivalent Catmull-Clark quads."""
    if all(len(face) == 4 for face in faces):
        return vertices, faces, edge_sharpness, vertex_sharpness, dissolved_edges, 0
    if dissolved_edges:
        raise ConversionError("Dissolved patches cannot be combined with polygon control faces")
    try:
        (
            vertices,
            faces,
            _old_map,
            _edge_map,
            _face_map,
            edge_sharpness,
            vertex_sharpness,
        ) = catmull_clark_step_details(
            vertices,
            faces,
            edge_sharpness,
            vertex_sharpness,
        )
    except ValueError as error:
        raise ConversionError(str(error)) from error
    validator = _validate_closed_quad_cage if closed else _validate_open_quad_cage
    validator(vertices, faces)
    return vertices, faces, edge_sharpness, vertex_sharpness, None, 1


def _clamped_interpolation_knots(count, degree=3):
    parameters = [index / float(count - 1) for index in range(count)]
    interior = [
        sum(parameters[index : index + degree]) / degree for index in range(1, count - degree)
    ]
    full_knots = [0.0] * (degree + 1) + interior + [1.0] * (degree + 1)
    return parameters, full_knots


def _basis(index, degree, parameter, knots, control_count):
    if degree == 0:
        if parameter == 1.0:
            return 1.0 if index == control_count - 1 else 0.0
        return 1.0 if knots[index] <= parameter < knots[index + 1] else 0.0
    value = 0.0
    left_denominator = knots[index + degree] - knots[index]
    if left_denominator:
        value += (
            (parameter - knots[index])
            / left_denominator
            * _basis(index, degree - 1, parameter, knots, control_count)
        )
    right_denominator = knots[index + degree + 1] - knots[index + 1]
    if right_denominator:
        value += (
            (knots[index + degree + 1] - parameter)
            / right_denominator
            * _basis(index + 1, degree - 1, parameter, knots, control_count)
        )
    return value


def _solve(matrix, values):
    """Solve a small dense system with three-coordinate right-hand sides."""
    count = len(matrix)
    augmented = [
        list(matrix[row]) + [float(component) for component in values[row]] for row in range(count)
    ]
    for column in range(count):
        pivot = max(range(column, count), key=lambda row: abs(augmented[row][column]))
        if abs(augmented[pivot][column]) < 1.0e-14:
            raise ConversionError("The B-spline interpolation system is singular")
        augmented[column], augmented[pivot] = augmented[pivot], augmented[column]
        divisor = augmented[column][column]
        augmented[column] = [value / divisor for value in augmented[column]]
        for row in range(count):
            if row == column:
                continue
            factor = augmented[row][column]
            if factor:
                augmented[row] = [
                    augmented[row][item] - factor * augmented[column][item]
                    for item in range(count + 3)
                ]
    return [tuple(row[count : count + 3]) for row in augmented]


def _make_surface(grid):
    u_count = len(grid)
    v_count = len(grid[0]) if grid else 0
    if u_count < 2 or v_count < 2 or any(len(row) != v_count for row in grid):
        raise ConversionError("B-spline patch samples must form at least a 2x2 grid")
    u_degree = min(3, u_count - 1)
    v_degree = min(3, v_count - 1)
    u_parameters, u_full_knots = _clamped_interpolation_knots(u_count, u_degree)
    v_parameters, v_full_knots = _clamped_interpolation_knots(v_count, v_degree)
    u_matrix = [
        [_basis(column, u_degree, parameter, u_full_knots, u_count) for column in range(u_count)]
        for parameter in u_parameters
    ]
    v_matrix = [
        [_basis(column, v_degree, parameter, v_full_knots, v_count) for column in range(v_count)]
        for parameter in v_parameters
    ]

    temporary = [[None] * v_count for _index in range(u_count)]
    for v_index in range(v_count):
        solved = _solve(u_matrix, [grid[u_index][v_index] for u_index in range(u_count)])
        for u_index, point in enumerate(solved):
            temporary[u_index][v_index] = point
    poles = []
    for u_index in range(u_count):
        poles.append(_solve(v_matrix, temporary[u_index]))

    def compressed_knots(full_knots):
        unique = []
        multiplicities = []
        for knot in full_knots:
            if not unique or knot != unique[-1]:
                unique.append(knot)
                multiplicities.append(1)
            else:
                multiplicities[-1] += 1
        return unique, multiplicities

    u_knots, u_multiplicities = compressed_knots(u_full_knots)
    v_knots, v_multiplicities = compressed_knots(v_full_knots)

    surface = Part.BSplineSurface()
    surface.buildFromPolesMultsKnots(
        [[App.Vector(*point) for point in row] for row in poles],
        u_multiplicities,
        v_multiplicities,
        u_knots,
        v_knots,
        False,
        False,
        u_degree,
        v_degree,
    )
    return surface


def _refined_parameter_cells(cells):
    """Propagate root-face parameter rectangles through one uniform step."""
    result = {}
    for root, root_cells in cells.items():
        result[root] = {}
        for face_index, parameters in root_cells.items():
            center = tuple(sum(point[axis] for point in parameters) * 0.25 for axis in range(2))
            midpoints = tuple(
                tuple(
                    (parameters[index][axis] + parameters[(index + 1) % 4][axis]) * 0.5
                    for axis in range(2)
                )
                for index in range(4)
            )
            for corner in range(4):
                result[root][face_index * 4 + corner] = (
                    parameters[corner],
                    midpoints[corner],
                    center,
                    midpoints[corner - 1],
                )
    return result


def _parameter_vertex_indices(cells, faces):
    result = {}
    for root, root_cells in cells.items():
        for face_index, parameters in root_cells.items():
            for vertex_index, point in zip(faces[face_index], parameters):
                result[(root, round(point[0], 12), round(point[1], 12))] = vertex_index
    return result


def _tmesh_refinement(
    vertices,
    faces,
    mesh,
    edge_sharpness=None,
    vertex_sharpness=None,
    controlled_ids=None,
):
    """Evaluate all hierarchical controls on one nested uniform CC cage."""
    if not isinstance(mesh, HierarchicalTMesh):
        raise ConversionError("Invalid hierarchical T-mesh")
    if any(root < 0 or root >= len(faces) for root in {face.root for face in mesh.faces.values()}):
        raise ConversionError("A T-mesh root face no longer exists")
    current_vertices = [tuple(point) for point in vertices]
    current_faces = [tuple(face) for face in faces]
    current_edges = edge_sharpness
    current_corners = vertex_sharpness
    cells = {
        face_index: {face_index: ((0.0, 0.0), (1.0, 0.0), (1.0, 1.0), (0.0, 1.0))}
        for face_index in range(len(faces))
    }
    controlled = set(mesh.vertices) if controlled_ids is None else set(controlled_ids)
    locations = mesh.parameter_locations()
    maximum_level = max(
        max(mesh.vertex_levels.values(), default=0),
        max((face.level for face in mesh.faces.values()), default=0),
    )
    for level in range(1, maximum_level + 1):
        details = catmull_clark_step_details(
            current_vertices, current_faces, current_edges, current_corners
        )
        (
            current_vertices,
            current_faces,
            _old_map,
            _edge_map,
            _face_map,
            current_edges,
            current_corners,
        ) = details
        current_vertices = list(current_vertices)
        cells = _refined_parameter_cells(cells)
        parameter_indices = _parameter_vertex_indices(cells, current_faces)
        for control_id, control_level in mesh.vertex_levels.items():
            if control_id not in controlled or control_level != level:
                continue
            matched = False
            for root, u_value, v_value in locations[control_id]:
                index = parameter_indices.get((root, round(u_value, 12), round(v_value, 12)))
                if index is not None:
                    current_vertices[index] = mesh.vertices[control_id]
                    matched = True
            if not matched:
                raise ConversionError("A hierarchical control is off the dyadic grid")
    parameter_indices = _parameter_vertex_indices(cells, current_faces)
    control_indices = {}
    for control_id, candidates in locations.items():
        for root, u_value, v_value in candidates:
            index = parameter_indices.get((root, round(u_value, 12), round(v_value, 12)))
            if index is not None:
                control_indices[control_id] = index
                break
    if len(control_indices) != len(mesh.vertices):
        raise ConversionError("Could not map every T-mesh control to the evaluator")
    return (
        current_vertices,
        current_faces,
        cells,
        control_indices,
        current_edges,
        current_corners,
    )


def seed_tmesh_vertices(
    vertices, faces, old_mesh, new_mesh, vertex_ids, edge_sharpness=None, vertex_sharpness=None
):
    """Return surface-preserving seeds for controls introduced by an edit."""
    refined = _tmesh_refinement(
        vertices,
        faces,
        new_mesh,
        edge_sharpness,
        vertex_sharpness,
        controlled_ids=old_mesh.vertices,
    )
    fine_vertices, _faces, _cells, indices = refined[:4]
    return {vertex_id: fine_vertices[indices[vertex_id]] for vertex_id in vertex_ids}


def _assemble_parameter_grid(fine_grids, root_cells, region):
    u_values = [point[0] for point in region]
    v_values = [point[1] for point in region]
    u_min, u_max = min(u_values), max(u_values)
    v_min, v_max = min(v_values), max(v_values)
    samples = {}
    for face_index, parameters in root_cells.items():
        cell_u = [point[0] for point in parameters]
        cell_v = [point[1] for point in parameters]
        if (
            min(cell_u) < u_min - 1.0e-12
            or max(cell_u) > u_max + 1.0e-12
            or min(cell_v) < v_min - 1.0e-12
            or max(cell_v) > v_max + 1.0e-12
        ):
            continue
        grid = fine_grids[face_index]
        size = len(grid) - 1
        if size < 1 or any(len(row) != size + 1 for row in grid):
            raise ConversionError("Hierarchical child samples are inconsistent")
        p0, p1, _p2, p3 = parameters
        for u_index, row in enumerate(grid):
            for v_index, point in enumerate(row):
                u_value = (
                    p0[0] + (p1[0] - p0[0]) * u_index / size + (p3[0] - p0[0]) * v_index / size
                )
                v_value = (
                    p0[1] + (p1[1] - p0[1]) * u_index / size + (p3[1] - p0[1]) * v_index / size
                )
                if (
                    u_min - 1.0e-12 <= u_value <= u_max + 1.0e-12
                    and v_min - 1.0e-12 <= v_value <= v_max + 1.0e-12
                ):
                    samples[(round(u_value, 12), round(v_value, 12))] = point
    us = sorted({key[0] for key in samples})
    vs = sorted({key[1] for key in samples})
    if len(us) < 2 or len(vs) < 2 or len(samples) != len(us) * len(vs):
        raise ConversionError("Could not assemble a logical T-mesh patch")
    return [[samples[(u_value, v_value)] for v_value in vs] for u_value in us]


def _assemble_tmesh_root_grids(fine_grids, cells):
    root_region = ((0.0, 0.0), (1.0, 0.0), (1.0, 1.0), (0.0, 1.0))
    return [
        _assemble_parameter_grid(fine_grids, cells[root], root_region) for root in sorted(cells)
    ]


def _shape_from_tmesh_surfaces(surfaces, mesh, closed, tolerance):
    """Create logical leaf faces as exact parameter trims of root surfaces."""
    faces = []
    for face_id in sorted(mesh.faces):
        leaf = mesh.faces[face_id]
        u_values = [point[0] for point in leaf.parameters]
        v_values = [point[1] for point in leaf.parameters]
        faces.append(
            surfaces[leaf.root].toShape(min(u_values), max(u_values), min(v_values), max(v_values))
        )
    if not closed and len(faces) == 1:
        if faces[0].isNull() or not faces[0].isValid():
            raise ConversionError("OCCT rejected the dissolved T-mesh face")
        return faces[0]
    compound = Part.makeCompound(faces)
    compound.sewShape(max(float(tolerance), 1.0e-7))
    if len(compound.Shells) != 1:
        raise ConversionError("T-mesh trims did not produce one shell")
    shell = compound.Shells[0]
    if closed:
        if not shell.isClosed():
            raise ConversionError("T-mesh trims opened the solid")
        solid = Part.makeSolid(shell)
        if solid.isNull() or not solid.isValid():
            raise ConversionError("OCCT rejected the trimmed T-mesh solid")
        return solid
    if shell.isNull() or not shell.isValid() or shell.isClosed():
        raise ConversionError("OCCT rejected the trimmed T-mesh surface")
    return shell.Faces[0] if len(shell.Faces) == 1 else shell


def tmesh_cage_to_shape(
    vertices,
    faces,
    mesh,
    closed,
    tolerance=0.05,
    max_refinement=3,
    edge_sharpness=None,
    vertex_sharpness=None,
):
    """Evaluate a hierarchical T-mesh into minimal selectable OCC faces."""
    edge_counts = {}
    for face in faces:
        for index, first in enumerate(face):
            edge = tuple(sorted((first, face[(index + 1) % len(face)])))
            edge_counts[edge] = edge_counts.get(edge, 0) + 1
    if edge_counts and all(count == 2 for count in edge_counts.values()):
        _validate_closed_quad_cage(vertices, faces)
    else:
        _validate_open_quad_cage(vertices, faces)
    refined = _tmesh_refinement(vertices, faces, mesh, edge_sharpness, vertex_sharpness)
    fine_vertices, fine_faces, cells, _indices, fine_edges, fine_corners = refined
    tolerance = float(tolerance)
    maximum = int(max_refinement)
    base_level = max(mesh.vertex_levels.values(), default=0)
    surfaces = []
    deviation = math.inf
    level = max(2, base_level + 1)
    for level in range(max(2, base_level + 1), maximum + base_level + 1):
        fit = catmull_clark_patch_grids(
            fine_vertices, fine_faces, level - base_level, fine_edges, fine_corners
        )
        fit_grids = _assemble_tmesh_root_grids(fit, cells)
        surfaces = [_make_surface(grid) for grid in fit_grids]
        validation = catmull_clark_patch_grids(
            fine_vertices, fine_faces, level - base_level + 1, fine_edges, fine_corners
        )
        validation_grids = _assemble_tmesh_root_grids(validation, cells)
        deviation = _maximum_sample_deviation(surfaces, validation_grids, tolerance)
        if deviation <= tolerance:
            break
    shape = _shape_from_tmesh_surfaces(surfaces, mesh, closed, tolerance)
    return shape, deviation, level


def tmesh_control_surface_points(vertices, faces, mesh, edge_sharpness=None, vertex_sharpness=None):
    """Return limit positions in stable T-mesh control-ID order."""
    refined = _tmesh_refinement(vertices, faces, mesh, edge_sharpness, vertex_sharpness)
    fine_vertices, fine_faces, _cells, indices, fine_edges, fine_corners = refined
    limits = catmull_clark_limit_points(fine_vertices, fine_faces, fine_edges, fine_corners)
    return [limits[indices[vertex_id]] for vertex_id in sorted(mesh.vertices)]


def _hierarchical_refinement(
    vertices,
    faces,
    inserts,
    local_points,
    edge_sharpness=None,
    vertex_sharpness=None,
):
    """Build the hidden evaluation level and apply persistent local controls."""
    details = catmull_clark_step_details(vertices, faces, edge_sharpness, vertex_sharpness)
    (
        fine_vertices,
        fine_faces,
        old_map,
        edge_map,
        _face_map,
        fine_edge_sharpness,
        fine_vertex_sharpness,
    ) = details
    fine_vertices = list(fine_vertices)
    face_lookup = {tuple(face): index for index, face in enumerate(faces)}
    split_sides = {}
    local_fine_indices = {}
    for insert in inserts:
        face_index = face_lookup.get(tuple(insert.face))
        if face_index is None:
            raise ConversionError("An Insert Edge patch no longer exists")
        if face_index in split_sides:
            raise ConversionError("Only one local edge may currently be inserted per face")
        if len(insert.points) != 2:
            raise ConversionError("The local insertion has no editable endpoint controls")
        face = faces[face_index]
        side = next(
            (
                index
                for index, start in enumerate(face)
                if tuple(sorted((start, face[(index + 1) % 4]))) == tuple(insert.edge)
            ),
            None,
        )
        if side is None:
            raise ConversionError("The Insert Edge boundary no longer exists")
        # The inserted seam is parallel to the selected side, so its editable
        # endpoints lie halfway along the two adjacent sides.
        first_cross_edge = tuple(sorted((face[(side + 1) % 4], face[(side + 2) % 4])))
        second_cross_edge = tuple(sorted((face[(side + 3) % 4], face[side])))
        fine_indices = (edge_map[first_cross_edge], edge_map[second_cross_edge])
        for local_index, fine_index in zip(insert.points, fine_indices):
            if local_index >= len(local_points):
                raise ConversionError("An Insert Edge endpoint is missing")
            fine_vertices[fine_index] = tuple(local_points[local_index])
            local_fine_indices[local_index] = fine_index
        split_sides[face_index] = side
    return (
        fine_vertices,
        fine_faces,
        old_map,
        local_fine_indices,
        fine_edge_sharpness,
        fine_vertex_sharpness,
        split_sides,
    )


def _composite_patch_grids(fine_grids, face_count):
    """Recompose four evaluation cells into each minimal logical BRep face."""
    result = []
    for face_index in range(face_count):
        children = fine_grids[face_index * 4 : face_index * 4 + 4]
        if len(children) != 4:
            raise ConversionError("Hierarchical patch refinement is incomplete")
        size = len(children[0]) - 1
        combined = [[None] * (2 * size + 1) for _ in range(2 * size + 1)]
        corners = (
            ((0, 0), (size, 0), (size, size), (0, size)),
            ((2 * size, 0), (2 * size, size), (size, size), (size, 0)),
            ((2 * size, 2 * size), (size, 2 * size), (size, size), (2 * size, size)),
            ((0, 2 * size), (0, size), (size, size), (size, 2 * size)),
        )
        for child, target_corners in zip(children, corners):
            if len(child) != size + 1 or any(len(row) != size + 1 for row in child):
                raise ConversionError("Hierarchical child samples have inconsistent sizes")
            p0, p1, _p2, p3 = target_corners
            for u_index, row in enumerate(child):
                for v_index, point in enumerate(row):
                    u = (
                        p0[0]
                        + (p1[0] - p0[0]) * u_index // size
                        + (p3[0] - p0[0]) * v_index // size
                    )
                    v = (
                        p0[1]
                        + (p1[1] - p0[1]) * u_index // size
                        + (p3[1] - p0[1]) * v_index // size
                    )
                    combined[u][v] = point
        if any(point is None for row in combined for point in row):
            raise ConversionError("Could not recompose a hierarchical patch")
        result.append(combined)
    return result


def hierarchical_cage_to_shape(
    vertices,
    faces,
    inserts,
    local_points,
    closed,
    tolerance=0.05,
    max_refinement=3,
    edge_sharpness=None,
    vertex_sharpness=None,
):
    """Evaluate editable local insertions into a minimal selectable BRep."""
    if closed:
        _validate_closed_quad_cage(vertices, faces)
    else:
        _validate_open_quad_cage(vertices, faces)
    (
        fine_vertices,
        fine_faces,
        _old_map,
        _local_map,
        fine_edges,
        fine_corners,
        _split_sides,
    ) = _hierarchical_refinement(
        vertices,
        faces,
        inserts,
        local_points,
        edge_sharpness,
        vertex_sharpness,
    )
    tolerance = float(tolerance)
    surfaces = []
    deviation = math.inf
    level = 2
    for level in range(2, int(max_refinement) + 1):
        fine_fit = catmull_clark_patch_grids(
            fine_vertices, fine_faces, level - 1, fine_edges, fine_corners
        )
        # Fit every logical patch once.  Its local children are evaluation
        # cells, not BRep faces.  OCCT trims that single fitted surface below,
        # which makes the two new faces share the exact same geometry.
        fit_grids = _composite_patch_grids(fine_fit, len(faces))
        surfaces = [_make_surface(grid) for grid in fit_grids]
        fine_validation = catmull_clark_patch_grids(
            fine_vertices, fine_faces, level, fine_edges, fine_corners
        )
        validation_grids = _composite_patch_grids(fine_validation, len(faces))
        deviation = _maximum_sample_deviation(surfaces, validation_grids, tolerance)
        if deviation <= tolerance:
            break
    builder = _solid_from_surfaces if closed else _open_shape_from_surfaces
    shape = builder(surfaces, tolerance)
    surface_points = [None] * len(vertices)
    for face, grid in zip(faces, fit_grids):
        corners = (grid[0][0], grid[-1][0], grid[-1][-1], grid[0][-1])
        for index, point in zip(face, corners):
            surface_points[index] = point
    shape = apply_local_edge_inserts(
        shape,
        vertices,
        faces,
        level,
        inserts,
        tolerance,
        edge_sharpness,
        vertex_sharpness,
        surface_points_override=surface_points,
    )
    return shape, deviation, level


def hierarchical_control_surface_points(
    vertices,
    faces,
    inserts,
    local_points,
    edge_sharpness=None,
    vertex_sharpness=None,
):
    """Return limit positions for base controls followed by local controls."""
    (
        fine_vertices,
        fine_faces,
        old_map,
        local_map,
        fine_edges,
        fine_corners,
        _split_sides,
    ) = _hierarchical_refinement(
        vertices,
        faces,
        inserts,
        local_points,
        edge_sharpness,
        vertex_sharpness,
    )
    limits = catmull_clark_limit_points(fine_vertices, fine_faces, fine_edges, fine_corners)
    result = [limits[old_map[index]] for index in range(len(vertices))]
    result.extend(limits[local_map[index]] for index in range(len(local_points)))
    return result


def _point_surface_distance(surface, point):
    vector = App.Vector(*point)
    try:
        u_parameter, v_parameter = surface.parameter(vector)
        projected = surface.value(u_parameter, v_parameter)
        return (projected - vector).Length
    except (Part.OCCError, RuntimeError):
        return math.inf


def _maximum_sample_deviation(surfaces, validation_grids, tolerance):
    """Measure fit error at the known parameter of every validation sample.

    ``catmull_clark_patch_grids`` samples each original face on a uniform UV
    grid and ``_make_surface`` preserves that parameterization.  The distance
    at the corresponding UV is an inexpensive upper bound on geometric
    distance.  Only samples whose bound exceeds the requested tolerance need
    the much more expensive inverse-surface projection.
    """
    deviation = 0.0
    for surface, grid in zip(surfaces, validation_grids):
        u_count = len(grid)
        v_count = len(grid[0]) if grid else 0
        if u_count < 2 or v_count < 2 or any(len(row) != v_count for row in grid):
            return math.inf
        for u_index, row in enumerate(grid):
            u_parameter = u_index / float(u_count - 1)
            for v_index, point in enumerate(row):
                v_parameter = v_index / float(v_count - 1)
                try:
                    evaluated = surface.value(u_parameter, v_parameter)
                except (Part.OCCError, RuntimeError):
                    return math.inf
                parameter_deviation = (evaluated - App.Vector(*point)).Length
                if parameter_deviation <= tolerance:
                    deviation = max(deviation, parameter_deviation)
                else:
                    deviation = max(
                        deviation,
                        _point_surface_distance(surface, point),
                    )
    return deviation


def _dissolved_patch_layouts(faces, dissolved_edges):
    """Return rectangular face groups joined across selected internal edges."""
    faces = [tuple(int(vertex) for vertex in face) for face in faces]
    dissolved = {
        tuple(sorted((int(edge[0]), int(edge[1])))) for edge in (dissolved_edges or ())
    }
    if not dissolved:
        return [((index,), {index: ((0, 0), (1, 0), (1, 1), (0, 1))}) for index in range(len(faces))]

    edge_faces = {}
    for face_index, face in enumerate(faces):
        if len(face) != 4:
            raise ConversionError("Edge dissolve requires an all-quad control cage")
        for position, first in enumerate(face):
            edge = tuple(sorted((first, face[(position + 1) % 4])))
            edge_faces.setdefault(edge, []).append(face_index)
    if any(edge not in edge_faces or len(edge_faces[edge]) != 2 for edge in dissolved):
        raise ConversionError("Only internal control edges can be dissolved")

    adjacency = {index: [] for index in range(len(faces))}
    for edge in dissolved:
        first, second = edge_faces[edge]
        adjacency[first].append((second, edge))
        adjacency[second].append((first, edge))

    layouts = []
    unused = set(range(len(faces)))
    while unused:
        first = min(unused)
        group = {first}
        pending = [first]
        while pending:
            current = pending.pop()
            for neighbor, _edge_value in adjacency[current]:
                if neighbor not in group:
                    group.add(neighbor)
                    pending.append(neighbor)
        unused.difference_update(group)
        coordinates = {first: ((0, 0), (1, 0), (1, 1), (0, 1))}
        pending = [first]
        while pending:
            current = pending.pop()
            current_face = faces[current]
            current_coordinates = coordinates[current]
            for neighbor, shared in adjacency[current]:
                position = next(
                    index
                    for index, vertex in enumerate(current_face)
                    if tuple(sorted((vertex, current_face[(index + 1) % 4]))) == shared
                )
                start = current_coordinates[position]
                end = current_coordinates[(position + 1) % 4]
                center = (
                    sum(point[0] for point in current_coordinates) / 4.0,
                    sum(point[1] for point in current_coordinates) / 4.0,
                )
                midpoint = ((start[0] + end[0]) * 0.5, (start[1] + end[1]) * 0.5)
                outward = (
                    int(round(2.0 * (midpoint[0] - center[0]))),
                    int(round(2.0 * (midpoint[1] - center[1]))),
                )
                neighbor_face = faces[neighbor]
                neighbor_position = next(
                    index
                    for index, vertex in enumerate(neighbor_face)
                    if tuple(sorted((vertex, neighbor_face[(index + 1) % 4]))) == shared
                )
                by_vertex = {
                    current_face[position]: start,
                    current_face[(position + 1) % 4]: end,
                }
                neighbor_coordinates = [None] * 4
                neighbor_coordinates[neighbor_position] = by_vertex[
                    neighbor_face[neighbor_position]
                ]
                neighbor_coordinates[(neighbor_position + 1) % 4] = by_vertex[
                    neighbor_face[(neighbor_position + 1) % 4]
                ]
                neighbor_coordinates[(neighbor_position + 2) % 4] = (
                    neighbor_coordinates[(neighbor_position + 1) % 4][0] + outward[0],
                    neighbor_coordinates[(neighbor_position + 1) % 4][1] + outward[1],
                )
                neighbor_coordinates[(neighbor_position + 3) % 4] = (
                    neighbor_coordinates[neighbor_position][0] + outward[0],
                    neighbor_coordinates[neighbor_position][1] + outward[1],
                )
                neighbor_coordinates = tuple(neighbor_coordinates)
                if neighbor in coordinates:
                    if coordinates[neighbor] != neighbor_coordinates:
                        raise ConversionError("Dissolved edges do not form a rectangular patch")
                    continue
                coordinates[neighbor] = neighbor_coordinates
                pending.append(neighbor)

        occupied = set()
        all_points = []
        for face_index in group:
            points = coordinates[face_index]
            all_points.extend(points)
            minimum_u = min(point[0] for point in points)
            minimum_v = min(point[1] for point in points)
            if set(points) != {
                (minimum_u, minimum_v),
                (minimum_u + 1, minimum_v),
                (minimum_u + 1, minimum_v + 1),
                (minimum_u, minimum_v + 1),
            }:
                raise ConversionError("Dissolved faces must retain a rectangular parameter grid")
            occupied.add((minimum_u, minimum_v))
        minimum_u = min(point[0] for point in all_points)
        maximum_u = max(point[0] for point in all_points)
        minimum_v = min(point[1] for point in all_points)
        maximum_v = max(point[1] for point in all_points)
        expected = {
            (u_value, v_value)
            for u_value in range(minimum_u, maximum_u)
            for v_value in range(minimum_v, maximum_v)
        }
        if occupied != expected:
            raise ConversionError("Dissolved edges would create a non-rectangular face")
        layouts.append((tuple(sorted(group)), coordinates))
    return layouts


def validate_dissolved_edges(faces, dissolved_edges):
    """Validate that dissolved edges can be recomposed into rectangular faces."""
    _dissolved_patch_layouts(faces, dissolved_edges)


def dissolved_control_faces(faces, dissolved_edges):
    """Return user-facing polygon faces and their evaluation-patch members.

    The all-quad input remains the subdivision evaluation decomposition. A
    dissolved rectangular group is exposed as one polygon whose boundary may
    contain T-points. The removed seams therefore disappear from the editable
    topology while the hidden quads retain a stable BRep parameterization.
    """
    faces = [tuple(int(vertex) for vertex in face) for face in faces]
    layouts = _dissolved_patch_layouts(faces, dissolved_edges)
    logical_faces = []
    groups = []
    for group, _coordinates in layouts:
        groups.append(tuple(group))
        if len(group) == 1:
            logical_faces.append(faces[group[0]])
            continue
        occurrences = {}
        directions = {}
        for face_index in group:
            face = faces[face_index]
            for position, first in enumerate(face):
                second = face[(position + 1) % len(face)]
                edge = tuple(sorted((first, second)))
                occurrences[edge] = occurrences.get(edge, 0) + 1
                directions[edge] = (first, second)
        boundary = [directions[edge] for edge, count in occurrences.items() if count == 1]
        following = {first: second for first, second in boundary}
        if len(following) != len(boundary):
            raise ConversionError("A dissolved control face has a branched boundary")
        start = min(following)
        polygon = [start]
        current = start
        while True:
            current = following.get(current)
            if current is None:
                raise ConversionError("A dissolved control face has an open boundary")
            if current == start:
                break
            if current in polygon:
                raise ConversionError("A dissolved control face boundary is inconsistent")
            polygon.append(current)
        if len(polygon) < 4 or len(polygon) != len(boundary):
            raise ConversionError("A dissolved control face boundary is incomplete")
        logical_faces.append(tuple(polygon))
    return logical_faces, groups


def _compose_dissolved_grids(grids, layouts):
    """Recompose per-control-face sample grids into one grid per logical face."""
    result = []
    for group, coordinates in layouts:
        if len(group) == 1:
            result.append(grids[group[0]])
            continue
        scale = len(grids[group[0]]) - 1
        points = [point for face_index in group for point in coordinates[face_index]]
        minimum_u = min(point[0] for point in points)
        maximum_u = max(point[0] for point in points)
        minimum_v = min(point[1] for point in points)
        maximum_v = max(point[1] for point in points)
        combined = [
            [None] * ((maximum_v - minimum_v) * scale + 1)
            for _index in range((maximum_u - minimum_u) * scale + 1)
        ]
        for face_index in group:
            corners = coordinates[face_index]
            grid = grids[face_index]
            if len(grid) - 1 != scale or any(len(row) - 1 != scale for row in grid):
                raise ConversionError("Dissolved patch grids require a common refinement")
            for u_index in range(scale + 1):
                for v_index in range(scale + 1):
                    fraction_u = u_index / float(scale)
                    fraction_v = v_index / float(scale)
                    u_value = corners[0][0] + fraction_u * (
                        corners[1][0] - corners[0][0]
                    ) + fraction_v * (corners[3][0] - corners[0][0])
                    v_value = corners[0][1] + fraction_u * (
                        corners[1][1] - corners[0][1]
                    ) + fraction_v * (corners[3][1] - corners[0][1])
                    target_u = int(round((u_value - minimum_u) * scale))
                    target_v = int(round((v_value - minimum_v) * scale))
                    combined[target_u][target_v] = grid[u_index][v_index]
        if any(point is None for row in combined for point in row):
            raise ConversionError("Dissolved patch grid is incomplete")
        result.append(combined)
    return result


def _oriented_patch_grid(grid, corners):
    """Rotate or reflect one square grid into its integer-layout orientation."""
    scale = len(grid) - 1
    oriented = [[None] * (scale + 1) for _index in range(scale + 1)]
    minimum_u = min(point[0] for point in corners)
    minimum_v = min(point[1] for point in corners)
    for u_index in range(scale + 1):
        for v_index in range(scale + 1):
            fraction_u = u_index / float(scale)
            fraction_v = v_index / float(scale)
            u_value = corners[0][0] + fraction_u * (
                corners[1][0] - corners[0][0]
            ) + fraction_v * (corners[3][0] - corners[0][0])
            v_value = corners[0][1] + fraction_u * (
                corners[1][1] - corners[0][1]
            ) + fraction_v * (corners[3][1] - corners[0][1])
            target_u = int(round((u_value - minimum_u) * scale))
            target_v = int(round((v_value - minimum_v) * scale))
            oriented[target_u][target_v] = grid[u_index][v_index]
    return oriented


def _join_bspline_surfaces(first, second, axis):
    """Join two clamped compatible tensor-product surfaces exactly at a seam."""
    first_poles = first.getPoles()
    second_poles = second.getPoles()
    if axis == 0:
        if len(first_poles[0]) != len(second_poles[0]):
            raise ConversionError("Dissolved surfaces have incompatible V poles")
        poles = first_poles + second_poles[1:]
        degree = first.UDegree
        u_knots = list(first.getUKnots()[:-1]) + [first.getUKnots()[-1]] + [
            value + first.getUKnots()[-1] - second.getUKnots()[0]
            for value in second.getUKnots()[1:]
        ]
        u_multiplicities = (
            list(first.getUMultiplicities()[:-1])
            + [degree]
            + list(second.getUMultiplicities()[1:])
        )
        v_knots = list(first.getVKnots())
        v_multiplicities = list(first.getVMultiplicities())
    else:
        if len(first_poles) != len(second_poles):
            raise ConversionError("Dissolved surfaces have incompatible U poles")
        poles = [
            list(first_row) + list(second_row[1:])
            for first_row, second_row in zip(first_poles, second_poles)
        ]
        degree = first.VDegree
        v_knots = list(first.getVKnots()[:-1]) + [first.getVKnots()[-1]] + [
            value + first.getVKnots()[-1] - second.getVKnots()[0]
            for value in second.getVKnots()[1:]
        ]
        v_multiplicities = (
            list(first.getVMultiplicities()[:-1])
            + [degree]
            + list(second.getVMultiplicities()[1:])
        )
        u_knots = list(first.getUKnots())
        u_multiplicities = list(first.getUMultiplicities())
    surface = Part.BSplineSurface()
    surface.buildFromPolesMultsKnots(
        poles,
        u_multiplicities,
        v_multiplicities,
        u_knots,
        v_knots,
        False,
        False,
        first.UDegree,
        first.VDegree,
    )
    return surface


def _make_dissolved_surfaces(grids, layouts):
    """Fit base patches separately, then concatenate them without moving seams."""
    surfaces = []
    for group, coordinates in layouts:
        if len(group) == 1:
            surfaces.append(_make_surface(grids[group[0]]))
            continue
        points = [point for face_index in group for point in coordinates[face_index]]
        minimum_u = min(point[0] for point in points)
        maximum_u = max(point[0] for point in points)
        minimum_v = min(point[1] for point in points)
        maximum_v = max(point[1] for point in points)
        tiles = {}
        for face_index in group:
            corners = coordinates[face_index]
            cell = (
                min(point[0] for point in corners),
                min(point[1] for point in corners),
            )
            tiles[cell] = _make_surface(_oriented_patch_grid(grids[face_index], corners))
        rows = []
        for v_value in range(minimum_v, maximum_v):
            row = tiles[minimum_u, v_value]
            for u_value in range(minimum_u + 1, maximum_u):
                row = _join_bspline_surfaces(row, tiles[u_value, v_value], 0)
            rows.append(row)
        surface = rows[0]
        for row in rows[1:]:
            surface = _join_bspline_surfaces(surface, row, 1)
        surface.scaleKnotsToBounds(0.0, 1.0, 0.0, 1.0)
        surfaces.append(surface)
    return surfaces


def _solid_from_surfaces(surfaces, sewing_tolerance):
    faces = [surface.toShape() for surface in surfaces]
    sewed = Part.makeCompound(faces)
    sewed.sewShape(max(float(sewing_tolerance), 1.0e-7))
    if len(sewed.Shells) != 1:
        raise ConversionError(f"Surface sewing produced {len(sewed.Shells)} shells instead of one")
    shell = sewed.Shells[0]
    if not shell.isClosed():
        raise ConversionError("The converted surface shell is not closed")
    solid = Part.makeSolid(shell)
    if solid.isNull() or not solid.isValid():
        raise ConversionError("OCCT rejected the converted solid")
    if not solid.Solids:
        raise ConversionError("The converted BRep does not contain a solid")
    return solid


def _open_shape_from_surfaces(surfaces, sewing_tolerance):
    faces = [surface.toShape() for surface in surfaces]
    if len(faces) == 1:
        if faces[0].isNull() or not faces[0].isValid():
            raise ConversionError("OCCT rejected the converted open surface")
        return faces[0]
    sewed = Part.makeCompound(faces)
    sewed.sewShape(max(float(sewing_tolerance), 1.0e-7))
    if len(sewed.Shells) != 1:
        raise ConversionError(f"Surface sewing produced {len(sewed.Shells)} shells instead of one")
    shell = sewed.Shells[0]
    if shell.isNull() or not shell.isValid() or shell.isClosed():
        raise ConversionError("OCCT rejected the converted open surface")
    return shell.Faces[0] if len(shell.Faces) == 1 else shell


def _matches_points(shape_vertices, expected_points, tolerance):
    if len(shape_vertices) < len(expected_points):
        return False
    return all(
        min((vertex.Point - App.Vector(*point)).Length for vertex in shape_vertices) <= tolerance
        for point in expected_points
    )


def _face_and_edge_for_insert(shape, face, edge, surface_points, tolerance):
    expected_face = [surface_points[index] for index in face]
    matching_faces = [
        candidate
        for candidate in shape.Faces
        if _matches_points(candidate.Vertexes, expected_face, tolerance)
    ]
    if len(matching_faces) != 1:
        raise ConversionError("Could not resolve the patch selected for Insert Edge")
    target_face = matching_faces[0]
    expected_edge = [surface_points[index] for index in edge]
    matching_edges = [
        candidate
        for candidate in target_face.Edges
        if len(candidate.Vertexes) == 2
        and _matches_points(candidate.Vertexes, expected_edge, tolerance)
    ]
    if len(matching_edges) != 1:
        raise ConversionError("Could not resolve the boundary selected for Insert Edge")
    return target_face, matching_edges[0]


def apply_local_edge_inserts(
    shape,
    vertices,
    faces,
    level,
    inserts,
    tolerance,
    edge_sharpness=None,
    vertex_sharpness=None,
    surface_points_override=None,
):
    """Split logical patches exactly while preserving the fitted surface.

    Each split adds one BRep face and one interior edge. OCCT also divides the
    two neighboring boundary edges at the T endpoints, but it does not divide
    those neighboring faces or create evaluation-only surface patches.
    """
    inserts = list(inserts or ())
    if not inserts:
        return shape
    surface_points = surface_points_override
    if surface_points is None:
        grids = catmull_clark_patch_grids(
            vertices,
            faces,
            int(level),
            edge_sharpness,
            vertex_sharpness,
        )
        surface_points = [None] * len(vertices)
        for face, grid in zip(faces, grids):
            corners = (grid[0][0], grid[-1][0], grid[-1][-1], grid[0][-1])
            for index, point in zip(face, corners):
                surface_points[index] = point
    if any(point is None for point in surface_points):
        raise ConversionError("Could not map the control cage to its fitted patches")

    lookup = {tuple(face): index for index, face in enumerate(faces)}
    diagonal = shape.BoundBox.DiagonalLength
    matching_tolerance = max(diagonal * 1.0e-5, float(tolerance), 1.0e-7)
    split_pairs = []
    for insert in inserts:
        if tuple(insert.face) not in lookup:
            raise ConversionError("An Insert Edge patch no longer exists")
        target_face, target_edge = _face_and_edge_for_insert(
            shape,
            insert.face,
            insert.edge,
            surface_points,
            matching_tolerance,
        )
        surface = target_face.Surface
        u_min, u_max, v_min, v_max = target_face.ParameterRange
        edge_parameters = [surface.parameter(vertex.Point) for vertex in target_edge.Vertexes]
        u_span = abs(edge_parameters[0][0] - edge_parameters[1][0])
        v_span = abs(edge_parameters[0][1] - edge_parameters[1][1])
        if u_span <= v_span:
            boundary = sum(value[0] for value in edge_parameters) * 0.5
            opposite = u_max if abs(boundary - u_min) <= abs(boundary - u_max) else u_min
            parameter = boundary + (opposite - boundary) * insert.position
            split_edge = surface.uIso(parameter).toShape(v_min, v_max)
        else:
            boundary = sum(value[1] for value in edge_parameters) * 0.5
            opposite = v_max if abs(boundary - v_min) <= abs(boundary - v_max) else v_min
            parameter = boundary + (opposite - boundary) * insert.position
            split_edge = surface.vIso(parameter).toShape(u_min, u_max)
        split_pairs.append((split_edge, target_face))

    split_result = Part.makeSplitShape(shape, split_pairs)
    if len(split_result) < 2 or not split_result[1]:
        raise ConversionError("OCCT did not produce a locally split shape")
    if not shape.Solids and shape.ShapeType == "Face":
        split_faces = [face for result_shape in split_result[1] for face in result_shape.Faces]
        shell = Part.makeShell(split_faces)
        if shell.isNull() or not shell.isValid():
            raise ConversionError("OCCT rejected the locally split surface")
        return shell.Faces[0] if len(shell.Faces) == 1 else shell
    split_shape = Part.makeCompound(split_result[1])
    split_shape.sewShape(max(float(tolerance), 1.0e-7))
    if len(split_shape.Shells) != 1:
        raise ConversionError("Local edge insertion did not produce one shell")
    shell = split_shape.Shells[0]
    if shape.Solids:
        if not shell.isClosed():
            raise ConversionError("Local edge insertion opened the solid")
        result = Part.makeSolid(shell)
        if result.isNull() or not result.isValid():
            raise ConversionError("OCCT rejected the locally split solid")
        return result
    if shell.isNull() or not shell.isValid():
        raise ConversionError("OCCT rejected the locally split surface")
    return shell.Faces[0] if len(shell.Faces) == 1 else shell


def cage_to_solid(
    vertices,
    faces,
    tolerance=0.05,
    max_refinement=3,
    edge_sharpness=None,
    vertex_sharpness=None,
    dissolved_edges=None,
):
    """Return ``(solid, deviation, level)`` for a closed quad cage.

    One B-spline patch is fitted to each original cage face. The fit is checked
    against Catmull-Clark limit points sampled at the next refinement level.
    Refinement stops when the requested tolerance is reached or the configured
    cap is exhausted. A result above tolerance is reported instead of silently
    presenting an uncertified solid.
    """
    vertices = [tuple(float(component) for component in point) for point in vertices]
    faces = [tuple(int(index) for index in face) for face in faces]
    (
        vertices,
        faces,
        edge_sharpness,
        vertex_sharpness,
        dissolved_edges,
        level_offset,
    ) = _prepare_polygon_cage(
        vertices,
        faces,
        edge_sharpness,
        vertex_sharpness,
        dissolved_edges,
        True,
    )
    _validate_closed_quad_cage(vertices, faces)
    tolerance = float(tolerance)
    max_refinement = int(max_refinement)
    if tolerance <= 0.0:
        raise ConversionError("BRep tolerance must be positive")
    if max_refinement < 2:
        raise ConversionError("Maximum refinement must be at least two")
    if len(faces) * (4 ** (max_refinement + 1)) > 250_000:
        raise ConversionError(
            "The cage/refinement combination exceeds the 250000-face sampling limit"
        )

    surfaces = []
    deviation = math.inf
    level = 2
    layouts = _dissolved_patch_layouts(faces, dissolved_edges)
    for level in range(2, max_refinement + 1):
        fit_grids = catmull_clark_patch_grids(
            vertices, faces, level, edge_sharpness, vertex_sharpness
        )
        surfaces = _make_dissolved_surfaces(fit_grids, layouts)
        validation_grids = catmull_clark_patch_grids(
            vertices, faces, level + 1, edge_sharpness, vertex_sharpness
        )
        validation_grids = _compose_dissolved_grids(validation_grids, layouts)
        deviation = _maximum_sample_deviation(surfaces, validation_grids, tolerance)
        if deviation <= tolerance:
            break

    solid = _solid_from_surfaces(surfaces, tolerance)
    return solid, deviation, level + level_offset


def cage_to_surface(
    vertices,
    faces,
    tolerance=0.05,
    max_refinement=3,
    edge_sharpness=None,
    vertex_sharpness=None,
    dissolved_edges=None,
):
    """Return ``(shape, deviation, level)`` for an open manifold quad cage."""
    vertices = [tuple(float(component) for component in point) for point in vertices]
    faces = [tuple(int(index) for index in face) for face in faces]
    (
        vertices,
        faces,
        edge_sharpness,
        vertex_sharpness,
        dissolved_edges,
        level_offset,
    ) = _prepare_polygon_cage(
        vertices,
        faces,
        edge_sharpness,
        vertex_sharpness,
        dissolved_edges,
        False,
    )
    _validate_open_quad_cage(vertices, faces)
    tolerance = float(tolerance)
    max_refinement = int(max_refinement)
    if tolerance <= 0.0:
        raise ConversionError("BRep tolerance must be positive")
    if max_refinement < 2:
        raise ConversionError("Maximum refinement must be at least two")
    if len(faces) * (4 ** (max_refinement + 1)) > 250_000:
        raise ConversionError(
            "The cage/refinement combination exceeds the 250000-face sampling limit"
        )

    surfaces = []
    deviation = math.inf
    level = 2
    layouts = _dissolved_patch_layouts(faces, dissolved_edges)
    for level in range(2, max_refinement + 1):
        fit_grids = catmull_clark_patch_grids(
            vertices, faces, level, edge_sharpness, vertex_sharpness
        )
        surfaces = _make_dissolved_surfaces(fit_grids, layouts)
        validation_grids = catmull_clark_patch_grids(
            vertices, faces, level + 1, edge_sharpness, vertex_sharpness
        )
        validation_grids = _compose_dissolved_grids(validation_grids, layouts)
        deviation = _maximum_sample_deviation(surfaces, validation_grids, tolerance)
        if deviation <= tolerance:
            break

    shape = _open_shape_from_surfaces(surfaces, tolerance)
    return shape, deviation, level + level_offset
