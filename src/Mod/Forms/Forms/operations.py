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

"""Application-side topology operations for Forms control cages."""

import math

import FreeCAD as App

from .cage import ControlCage
from .capabilities import require_base_topology, validate_local_creases
from .limits import check_sampling
from .numerics import DenseLU
from .model import object_tmesh as _object_tmesh, form_preview_object as _form_preview_object
from .placement import global_placement
from .brep import (
    ConversionError,
    LocalEdgeInsert,
    dissolved_control_faces,
    seed_tmesh_vertices,
    validate_dissolved_edges,
)
from .tmesh import HierarchicalTMesh
from .symmetry import (
    enforce_vertices,
    mirror_edges,
    mirror_faces,
    object_axis,
    vertex_map,
)
from .topology import (
    flatten_points,
    straighten_points,
)


def _all_points(obj):
    return [
        (point.x, point.y, point.z)
        for point in list(obj.ControlPoints) + list(getattr(obj, "LocalControlPoints", ()))
    ]


def _write_all_points(obj, points):
    import FreeCAD as App

    base_count = len(obj.ControlPoints)
    obj.ControlPoints = [App.Vector(*point) for point in points[:base_count]]
    if "LocalControlPoints" in obj.PropertiesList:
        obj.LocalControlPoints = [App.Vector(*point) for point in points[base_count:]]
    obj.CageMode = "Editable"
    obj.touch()


def set_edge_crease(obj, edges, sharpness):
    """Set selected control edges to a semi-sharp subdivision value."""
    cage = ControlCage.from_object(obj)
    encoded = str(getattr(obj, "TMeshData", "") or "")
    valid = (
        set(HierarchicalTMesh.decode(encoded).atomic_edges())
        if encoded
        else set(cage.edge_counts())
    )
    selected = {tuple(sorted((int(edge[0]), int(edge[1])))) for edge in edges}
    axis = object_axis(obj)
    if axis is not None:
        selected = mirror_edges(enumerate(_all_points(obj)), selected, axis)
    if not selected or not selected.issubset(valid):
        raise ValueError("Crease requires valid control edges")
    value = max(0.0, min(float(sharpness), 10.0))
    validate_local_creases(obj, selected, value)
    edge_values = dict(cage.edge_sharpness)
    for edge in selected:
        if value:
            edge_values[edge] = value
        else:
            edge_values.pop(edge, None)
    obj.EdgeSharpness = [
        f"{edge[0]} {edge[1]} {amount:.12g}"
        for edge, amount in sorted(edge_values.items())
        if amount > 0.0
    ]
    obj.CageMode = "Editable"
    obj.touch()
    return obj


def straighten_control_points(obj, indices, line=None, surface_points=False):
    """Straighten selected control or surface points to a chosen line."""
    points = (
        _surface_straightened_controls(obj, indices, line)
        if surface_points
        else straighten_points(_all_points(obj), indices, line)
    )
    _write_all_points(obj, points)
    return obj


def flatten_control_points(obj, indices, plane=None):
    """Flatten selected base or hierarchical controls to a chosen plane."""
    _write_all_points(obj, flatten_points(_all_points(obj), indices, plane))
    return obj


def _preview_form_from_points(obj, points):
    """Build an unowned Form shape from replacement control points."""
    from .cage import update_object_shape

    preview = _form_preview_object(obj, points)
    update_object_shape(preview)
    if preview.Shape.isNull():
        raise ConversionError(preview.ConversionStatus or "Could not build Form preview")
    return preview


def preview_flatten_control_points(obj, indices, plane=None):
    """Return the flattened Form shape without changing the document object."""
    points = flatten_points(_all_points(obj), indices, plane)
    return _preview_form_from_points(obj, points).Shape


def preview_straighten_control_points(obj, indices, line=None):
    """Return the straightened Form shape without changing the document object."""
    points = straighten_points(_all_points(obj), indices, line)
    return _preview_form_from_points(obj, points).Shape


def _surface_straightened_controls(obj, indices, line=None):
    """Solve selected control positions whose mapped surface points are straight."""
    import FreeCAD as App

    from .cage import control_surface_points

    points = _all_points(obj)
    indices = sorted({int(index) for index in indices})
    preview = _form_preview_object(obj, points)
    surface = control_surface_points(preview)
    surface_tuples = [(point.x, point.y, point.z) for point in surface]
    target = straighten_points(surface_tuples, indices, line)
    response = [[0.0] * len(indices) for _index in indices]
    base_count = len(obj.ControlPoints)
    for column, control_index in enumerate(indices):
        perturbed = list(points)
        value = perturbed[control_index]
        perturbed[control_index] = (value[0] + 1.0, value[1], value[2])
        preview.ControlPoints = [App.Vector(*point) for point in perturbed[:base_count]]
        preview.LocalControlPoints = [App.Vector(*point) for point in perturbed[base_count:]]
        moved = control_surface_points(preview)
        for row, surface_index in enumerate(indices):
            response[row][column] = moved[surface_index].x - surface[surface_index].x
    factor = DenseLU(response)
    for axis in range(3):
        right_hand_side = [
            target[index][axis] - surface_tuples[index][axis] for index in indices
        ]
        solution = factor.solve(right_hand_side)
        for index, delta in zip(indices, solution):
            value = list(points[index])
            value[axis] += delta
            points[index] = tuple(value)
    return points


def _solve_linear_system(matrix, values):
    return DenseLU(matrix).solve(values)


def preview_straighten_surface_points(obj, indices, line=None):
    """Return a surface-point Straighten preview without changing the Form."""
    points = _surface_straightened_controls(obj, indices, line)
    return _preview_form_from_points(obj, points).Shape


def local_insert_target(cage, face_index, orientation=0, side="left"):
    """Resolve a hovered face/orientation into the existing local-insert model.

    ``left`` means the hovered face, ``right`` means its neighbor across the
    oriented edge, and ``both`` affects both.  The returned side token is the
    stable ordering expected by :func:`insert_edge`.
    """
    face_index = int(face_index)
    hierarchical = isinstance(cage, HierarchicalTMesh)
    if hierarchical:
        if face_index not in cage.faces:
            raise ValueError("No valid control face is under the cursor")
        logical_face = cage.faces[face_index]
        face = logical_face.corners
    else:
        if face_index < 0 or face_index >= len(cage.faces):
            raise ValueError("No valid control face is under the cursor")
        face = cage.faces[face_index]
        if len(face) != 4:
            raise ValueError("Insert Edge currently requires a quad cage segment")
    orientation = int(orientation) % 2
    if hierarchical:
        side_vertices = logical_face.sides[orientation]
        edge = tuple(sorted(side_vertices[:2]))
        adjacent = [
            candidate_id
            for candidate_id, candidate in cage.faces.items()
            if any(
                tuple(sorted((start, end))) == edge
                for candidate_side in candidate.sides
                for start, end in zip(candidate_side, candidate_side[1:])
            )
        ]
    else:
        edge = tuple(sorted((face[orientation], face[orientation + 1])))
        adjacent = [
            index
            for index, candidate in enumerate(cage.faces)
            if any(
                tuple(sorted((start, candidate[(position + 1) % len(candidate)]))) == edge
                for position, start in enumerate(candidate)
            )
        ]
    if face_index not in adjacent:
        raise ValueError("The hovered cage segment does not contain the insert edge")
    side = str(side).lower()
    if side == "both":
        return edge, tuple(adjacent), "both"
    if side == "left":
        target_index = face_index
    elif side == "right":
        others = [index for index in adjacent if index != face_index]
        if not others:
            raise ValueError("The oriented boundary edge has no opposite side")
        target_index = others[0]
    else:
        raise ValueError(f"Unsupported insert side: {side}")
    return edge, (target_index,), "left" if adjacent[0] == target_index else "right"


def insert_edge_on_face(obj, face_index, orientation=0, side="left"):
    """Insert an edge interactively from a hovered cage face."""
    cage = ControlCage.from_object(obj)
    topology = _object_tmesh(obj, cage)
    edge, _targets, resolved_side = local_insert_target(topology, face_index, orientation, side)
    return insert_edge(obj, edge, 0.5, resolved_side)


def insert_edge(obj, edge, position=0.5, side="left", mode="simple"):
    """Insert a local hierarchical edge with two editable endpoint controls."""
    if not getattr(obj, "FormType", "").startswith("Forms::"):
        raise TypeError("The object is not a Forms object")
    position = float(position)
    if abs(position - 0.5) > 1.0e-9:
        raise ValueError("Editable local insertion currently supports the 50% position")
    if mode != "simple":
        raise ValueError(f"Unsupported edge insertion mode: {mode}")
    side = str(side).lower()
    if side not in ("left", "right", "both"):
        raise ValueError(f"Unsupported insert side: {side}")
    cage = ControlCage.from_object(obj)
    mesh = _object_tmesh(obj, cage)
    edge = tuple(sorted((int(edge[0]), int(edge[1]))))
    adjacent_ids = [
        face_id
        for face_id, face in mesh.faces.items()
        if any(
            tuple(sorted((start, end))) == edge
            for face_side in face.sides
            for start, end in zip(face_side, face_side[1:])
        )
    ]
    if not adjacent_ids:
        raise ValueError("The selected edge is not part of the hierarchical cage")
    if side == "left":
        chosen_ids = adjacent_ids[:1]
    elif side == "right":
        if len(adjacent_ids) < 2:
            raise ValueError("The selected boundary edge has no second side")
        chosen_ids = adjacent_ids[1:2]
    else:
        chosen_ids = adjacent_ids
    axis = object_axis(obj)
    tasks = [(face_id, edge) for face_id in chosen_ids]
    if axis is not None:
        mapping = vertex_map(mesh.vertices, axis)
        mirrored_ids = mirror_faces(
            mesh.vertices,
            {face_id: face.boundary for face_id, face in mesh.faces.items()},
            chosen_ids,
            axis,
        )
        existing = {face_id for face_id, _edge_value in tasks}
        for face_id in mirrored_ids:
            if face_id not in existing:
                tasks.append((face_id, tuple(sorted((mapping[edge[0]], mapping[edge[1]])))))
    chosen_faces = tuple(mesh.faces[face_id].corners for face_id, _edge_value in tasks)
    for face_id, split_edge in tasks:
        old_mesh = mesh
        mesh, new_ids, _children = mesh.insert_edge(face_id, split_edge, position)
        _seed_tmesh_edit(cage, old_mesh, mesh, new_ids)

    if axis is not None:
        mesh.vertices = enforce_vertices(mesh.vertices, axis)

    _write_object_tmesh(obj, cage, mesh)
    return obj, chosen_faces


def _materialized_edit_cage(obj):
    """Return the currently visible logical topology as an ordinary cage.

    Insert Point is a free-position polygon edit, rather than a dyadic
    T-mesh refinement.  Materializing first makes every visible T-edge and
    dissolved polygon authoritative, so the operation never leaves a hidden
    evaluation seam behind.
    """
    cage = ControlCage.from_object(obj)
    encoded = str(getattr(obj, "TMeshData", "") or "")
    if encoded:
        mesh = _object_tmesh(obj, cage)
        vertex_ids = sorted(mesh.vertices)
        if vertex_ids != list(range(len(vertex_ids))):
            raise ValueError("Insert Point requires contiguous control vertex IDs")
        return ControlCage(
            [mesh.vertices[index] for index in vertex_ids],
            [face.boundary for _face_id, face in sorted(mesh.faces.items())],
            list(cage.vertex_sharpness) + [0.0] * (len(vertex_ids) - len(cage.vertices)),
            cage.edge_sharpness,
        )

    dissolved = []
    for value in getattr(obj, "DissolvedEdges", ()):
        try:
            first, second = str(value).split()
            dissolved.append(tuple(sorted((int(first), int(second)))))
        except (TypeError, ValueError):
            raise ValueError("A dissolved edge record is invalid")
    if dissolved:
        logical_faces, _groups = dissolved_control_faces(cage.faces, dissolved)
        return ControlCage(
            cage.vertices,
            logical_faces,
            cage.vertex_sharpness,
            cage.edge_sharpness,
        )
    return cage


def insert_point_face_target(
    topology,
    first_edge,
    second_edge,
    first_existing=None,
    second_existing=None,
):
    """Return the unique face and opposite sides joining two point targets.

    ``topology`` may be an ordinary :class:`ControlCage` or a hierarchical
    T-mesh.  Keeping this eligibility test shared by the live handler and the
    commit path prevents the preview from accepting a chain that commit must
    later reject.
    """
    first_edge = tuple(sorted((int(first_edge[0]), int(first_edge[1]))))
    second_edge = tuple(sorted((int(second_edge[0]), int(second_edge[1]))))

    def target_sides(face, edge, existing_vertex):
        sides = (
            face.sides
            if hasattr(face, "sides")
            else tuple(
                (start, face[(index + 1) % len(face)])
                for index, start in enumerate(face)
            )
        )
        matches = []
        for side_index, side in enumerate(sides):
            if existing_vertex is not None and existing_vertex in side[1:-1]:
                matches.append(side_index)
            elif existing_vertex is None and edge[0] in side and edge[1] in side:
                matches.append(side_index)
        return matches

    face_items = (
        topology.faces.items()
        if isinstance(topology.faces, dict)
        else enumerate(topology.faces)
    )
    candidates = []
    for face_id, face in face_items:
        first_sides = target_sides(face, first_edge, first_existing)
        second_sides = target_sides(face, second_edge, second_existing)
        candidates.extend(
            (face_id, first_side, second_side)
            for first_side in first_sides
            for second_side in second_sides
            if len(getattr(face, "sides", face)) == 4
            and (first_side - second_side) % 4 == 2
        )
    if len(candidates) != 1:
        raise ValueError(
            "Consecutive Insert Points must lie on opposite sides of one control face"
        )
    return candidates[0]


def insert_point_edges(obj, points):
    """Join arbitrary edge positions with local hierarchical seams.

    ``points`` is an ordered sequence of ``((vertex_a, vertex_b), fraction)``
    records.  Fractions are measured from the sorted edge's first vertex.
    Every consecutive pair must lie on opposite sides of one common logical
    face. The T-mesh knot remains dyadic, while the two new editable controls
    retain the independently requested geometric positions. Consequently the
    operation exposes only the requested seam instead of evaluator refinement
    edges across the complete Form.
    """
    if not getattr(obj, "FormType", "").startswith("Forms::"):
        raise TypeError("The object is not a Forms object")
    if str(obj.FormType) == "Forms::Surface":
        raise ValueError("Part Design Form Surface does not support Insert Point")
    records = [
        (tuple(sorted((int(edge[0]), int(edge[1])))), float(fraction))
        for edge, fraction in points
    ]
    if len(records) < 2:
        raise ValueError("Insert Point requires at least two points")
    if any(not 1.0e-6 < fraction < 1.0 - 1.0e-6 for _edge_value, fraction in records):
        raise ValueError("Insert Point positions must lie inside an edge")

    cage = ControlCage.from_object(obj)
    mesh = _object_tmesh(obj, cage)
    valid_edges = set(mesh.atomic_edges())
    if any(edge not in valid_edges for edge, _fraction in records):
        raise ValueError("An Insert Point target is not a visible control edge")
    occurrence_ids = [None] * len(records)

    for segment_index in range(len(records) - 1):
        first_record = records[segment_index]
        second_record = records[segment_index + 1]
        first_existing = occurrence_ids[segment_index]
        second_existing = occurrence_ids[segment_index + 1]
        face_id, first_side, _second_side = insert_point_face_target(
            mesh,
            first_record[0],
            second_record[0],
            first_existing,
            second_existing,
        )
        face = mesh.faces[face_id]
        selected_side = face.sides[(first_side - 1) % 4]
        selected_atomic = (selected_side[0], selected_side[1])
        old_mesh = mesh
        mesh, created, children = mesh.insert_edge(face_id, selected_atomic, 0.5)
        _seed_tmesh_edit(cage, old_mesh, mesh, created)
        seam = mesh.faces[children[0]].sides[2]
        seam_ids = (seam[0], seam[-1])
        for occurrence, vertex_id, existing in (
            (segment_index, seam_ids[0], first_existing),
            (segment_index + 1, seam_ids[1], second_existing),
        ):
            if existing is not None and vertex_id != existing:
                raise ValueError("The Insert Point chain cannot branch across this T-edge")
            occurrence_ids[occurrence] = vertex_id
            edge, fraction = records[occurrence]
            point = tuple(
                old_mesh.vertices[edge[0]][axis] * (1.0 - fraction)
                + old_mesh.vertices[edge[1]][axis] * fraction
                for axis in range(3)
            )
            mesh.set_vertex(vertex_id, point)

    _write_object_tmesh(obj, cage, mesh)
    return obj, tuple(occurrence_ids)




def _seed_tmesh_edit(cage, old_mesh, new_mesh, vertex_ids):
    seeds = seed_tmesh_vertices(
        cage.vertices,
        cage.faces,
        old_mesh,
        new_mesh,
        vertex_ids,
        cage.edge_sharpness,
        cage.vertex_sharpness,
    )
    for vertex_id, point in seeds.items():
        new_mesh.set_vertex(vertex_id, point)


def _write_object_tmesh(obj, cage, mesh):
    """Persist the authoritative mesh and its editable FreeCAD vector view."""
    depth = max(mesh.vertex_levels.values(), default=0)
    check_sampling(len(cage.faces), max(2, depth + 1) + 1, root_grid=True)
    obj.TMeshData = mesh.encode()
    base_count = len(cage.vertices)
    obj.LocalControlPoints = [
        mesh.vertices[vertex_id] for vertex_id in range(base_count, mesh.next_vertex_id)
    ]
    obj.LocalEdgeInserts = []
    obj.CageMode = "Editable"
    obj.touch()


def subdivide_faces(obj, face_ids, u_divisions=2, v_divisions=2):
    """Subdivide selected logical leaves by dyadic U and V counts."""
    if not getattr(obj, "FormType", "").startswith("Forms::"):
        raise TypeError("The object is not a Forms object")
    cage = ControlCage.from_object(obj)
    old_mesh = _object_tmesh(obj, cage)
    counts = (int(u_divisions), int(v_divisions))
    if any(count < 1 or count & (count - 1) for count in counts):
        raise ValueError("Subdivision counts must be powers of two")
    levels = tuple(int(math.log2(count)) for count in counts)
    if not any(levels):
        raise ValueError("At least one subdivision count must be greater than one")
    axis = object_axis(obj)
    if axis is not None:
        face_ids = mirror_faces(
            old_mesh.vertices,
            {face_id: face.boundary for face_id, face in old_mesh.faces.items()},
            face_ids,
            axis,
        )
    mesh, descendants = old_mesh.subdivide_grid(face_ids, *levels)
    new_ids = sorted(set(mesh.vertices).difference(old_mesh.vertices))
    _seed_tmesh_edit(cage, old_mesh, mesh, new_ids)
    if axis is not None:
        mesh.vertices = enforce_vertices(mesh.vertices, axis)
    _write_object_tmesh(obj, cage, mesh)
    return obj, descendants


def delete_faces(obj, face_indices):
    """Delete indexed control faces from *obj* and compact its cage.

    The caller owns the document transaction.  Keeping transaction policy out
    of this module makes the same operation usable from GUI commands, Python,
    macros, and future task-panel tools.
    """
    if not getattr(obj, "FormType", "").startswith("Forms::"):
        raise TypeError("The object is not a Forms object")
    cage = ControlCage.from_object(obj)
    axis = object_axis(obj)
    encoded = str(getattr(obj, "TMeshData", "") or "")
    if encoded:
        mesh = _object_tmesh(obj, cage)
        if axis is not None:
            face_indices = mirror_faces(
                mesh.vertices,
                {face_id: face.boundary for face_id, face in mesh.faces.items()},
                face_indices,
                axis,
            )
        mesh = mesh.delete_faces(face_indices)
        _write_object_tmesh(obj, cage, mesh)
        return obj
    if axis is not None:
        face_indices = mirror_faces(
            enumerate(cage.vertices), enumerate(cage.faces), face_indices, axis
        )
    cage = cage.delete_faces(face_indices)
    obj.CageMode = "Editable"
    cage.write(obj)
    obj.touch()
    return obj


def dissolve_edges(obj, edges):
    """Merge logical control faces across selected internal edges.

    The editable topology exposes each dissolved region as one polygonal face.
    Its original quads remain only as a hidden evaluation decomposition, so
    removing an edge preserves the limit surface and cannot open a hole.
    """
    if not getattr(obj, "FormType", "").startswith("Forms::"):
        raise TypeError("The object is not a Forms object")
    if str(obj.FormType) == "Forms::Surface":
        raise ValueError("Part Design Form Surface does not support edge dissolve")
    cage = ControlCage.from_object(obj)
    selected = {tuple(sorted((int(edge[0]), int(edge[1])))) for edge in edges}
    if not selected:
        raise ValueError("No control edges were selected")
    axis = object_axis(obj)
    if axis is not None:
        selected = set(mirror_edges(enumerate(_all_points(obj)), selected, axis))
    if str(getattr(obj, "TMeshData", "") or "") or getattr(obj, "LocalEdgeInserts", ()):
        mesh = _object_tmesh(obj, cage)
        mesh = mesh.dissolve_edges(selected)
        _write_object_tmesh(obj, cage, mesh)
        return obj
    counts = cage.edge_counts()
    if any(counts.get(edge) != 2 for edge in selected):
        raise ValueError("Only internal control edges can be dissolved")
    dissolved = set()
    for encoded in getattr(obj, "DissolvedEdges", ()):
        try:
            first, second = str(encoded).split()
            dissolved.add(tuple(sorted((int(first), int(second)))))
        except (TypeError, ValueError):
            raise ValueError("A dissolved edge record is invalid")
    dissolved.update(selected)
    validate_dissolved_edges(cage.faces, dissolved)
    obj.CageMode = "Editable"
    obj.DissolvedEdges = [f"{edge[0]} {edge[1]}" for edge in sorted(dissolved)]
    obj.EdgeSharpness = [
        encoded
        for encoded in obj.EdgeSharpness
        if tuple(sorted(int(value) for value in str(encoded).split()[:2])) not in dissolved
    ]
    obj.touch()
    return obj


def erase_and_fill(obj, face_indices):
    """Erase selected control faces and minimally rebuild their boundary."""
    require_base_topology(obj, "Erase and fill")
    if not getattr(obj, "FormType", "").startswith("Forms::"):
        raise TypeError("The object is not a Forms object")
    cage = ControlCage.from_object(obj)
    axis = object_axis(obj)
    if axis is not None:
        face_indices = mirror_faces(
            enumerate(cage.vertices), enumerate(cage.faces), face_indices, axis
        )
    cage = cage.erase_and_fill(face_indices)
    obj.CageMode = "Editable"
    cage.write(obj)
    obj.touch()
    return obj


def fill_holes(obj, boundary_edges, mode="automatic"):
    """Fill the boundary loops containing *boundary_edges* on a Forms object."""
    require_base_topology(obj, "Fill holes")
    if not getattr(obj, "FormType", "").startswith("Forms::"):
        raise TypeError("The object is not a Forms object")
    cage = ControlCage.from_object(obj)
    axis = object_axis(obj)
    if axis is not None:
        boundary_edges = mirror_edges(enumerate(cage.vertices), boundary_edges, axis)
    cage = cage.fill_boundaries(boundary_edges, mode)
    obj.CageMode = "Editable"
    cage.write(obj)
    obj.touch()
    return obj


def bridge_boundaries(obj, boundary_edges):
    """Bridge two equal-sized control-cage boundary loops."""
    require_base_topology(obj, "Bridge boundaries")
    if not getattr(obj, "FormType", "").startswith("Forms::"):
        raise TypeError("The object is not a Forms object")
    cage = ControlCage.from_object(obj).bridge_boundaries(boundary_edges)
    obj.CageMode = "Editable"
    cage.write(obj)
    obj.touch()
    return obj


def _clear_match_constraint(obj):
    """Remove associative boundary data invalidated by a topology split/join."""
    if "MatchSupport" in obj.PropertiesList:
        obj.MatchSupport = None
    for name in ("MatchBoundary", "MatchParameters", "MatchCornerVertices"):
        if name in obj.PropertiesList:
            setattr(obj, name, [])
    if "MatchCornerEdges" in obj.PropertiesList:
        obj.MatchCornerEdges = []


def unweld_segment(obj, segment_edges, separate_forms=True):
    """Split *obj* into two surfaces along one separating control segment.

    Both surfaces receive independent controls along the opening.  With
    ``separate_forms`` enabled, the original object becomes the first half and
    a document copy becomes the second; otherwise both remain in *obj*.
    """
    if not getattr(obj, "FormType", "").startswith("Forms::"):
        raise TypeError("The object is not a Forms object")
    if str(obj.FormType) == "Forms::Surface":
        raise ValueError("Part Design Form Surface cannot be unwelded")
    if str(getattr(obj, "TypeId", "")) != "Part::FeaturePython":
        raise ValueError("Unweld currently requires a standalone Form")
    if (
        getattr(obj, "LocalEdgeInserts", ())
        or str(getattr(obj, "TMeshData", "") or "")
        or getattr(obj, "DissolvedEdges", ())
    ):
        raise ValueError("Unweld currently requires an all-quad base control cage")
    cage = _materialized_edit_cage(obj)
    if not cage.is_closed:
        raise ValueError("Unweld currently requires a closed Form")
    first, second = cage.split_along_edges(segment_edges)
    if not bool(separate_forms):
        first.disjoint_union(second).write(obj)
        obj.CageMode = "Editable"
        _clear_match_constraint(obj)
        obj.touch()
        return (obj,)

    document = obj.Document
    second_obj = document.copyObject(obj, False)
    if second_obj is None:
        raise ConversionError("Could not create the second unwelded Form")
    second_obj.Label = App.Qt.translate("Forms_Unweld", "%1 (Unwelded)").replace(
        "%1", obj.Label
    )
    for target, result in ((obj, first), (second_obj, second)):
        result.write(target)
        target.CageMode = "Editable"
        _clear_match_constraint(target)
        target.touch()
    return obj, second_obj


def weld_boundaries(obj, first_edge, other, second_edge):
    """Join two Forms by identifying the selected free boundary loops."""
    if obj is other:
        raise ValueError("Weld requires two different Forms")
    if obj.Document is not other.Document:
        raise ValueError("Weld Forms must belong to the same document")
    if not all(
        getattr(candidate, "FormType", "").startswith("Forms::")
        for candidate in (obj, other)
    ):
        raise TypeError("Weld requires two Forms objects")
    if any(
        str(getattr(candidate, "TypeId", "")) != "Part::FeaturePython"
        for candidate in (obj, other)
    ):
        raise ValueError("Weld currently requires two standalone Forms")
    if any(
        str(candidate.FormType) == "Forms::Surface"
        or getattr(candidate, "LocalEdgeInserts", ())
        or str(getattr(candidate, "TMeshData", "") or "")
        for candidate in (obj, other)
    ):
        raise ValueError("Weld currently requires standalone all-quad Forms")

    first = _materialized_edit_cage(obj)
    second = _materialized_edit_cage(other)
    first_placement = global_placement(obj)
    second_placement = global_placement(other)
    to_first = first_placement.inverse()
    second = ControlCage(
        [
            (
                transformed.x,
                transformed.y,
                transformed.z,
            )
            for point in second.vertices
            for transformed in (
                to_first.multVec(second_placement.multVec(App.Vector(*point))),
            )
        ],
        second.faces,
        second.vertex_sharpness,
        second.edge_sharpness,
    )
    result = first.weld_boundary(second, first_edge, second_edge)
    result.write(obj)
    obj.CageMode = "Editable"
    _clear_match_constraint(obj)
    obj.touch()
    obj.Document.removeObject(other.Name)
    return obj


def thicken_surface(obj, distance, sharp=True):
    """Turn an open Form surface into a closed, editable thickened cage."""
    require_base_topology(obj, "Thicken surface")
    if not getattr(obj, "FormType", "").startswith("Forms::"):
        raise TypeError("The object is not a Forms object")
    if getattr(obj, "LocalEdgeInserts", ()) or str(getattr(obj, "TMeshData", "") or ""):
        raise ValueError("Thicken does not yet support Forms with local edge inserts")
    cage = ControlCage.from_object(obj).thickened(distance, sharp)
    cage.write(obj)
    obj.CageMode = "Editable"
    obj.touch()
    return obj


def insert_edge_loop(obj, edge, position=0.5, mode="simple"):
    """Insert an edge loop through the quad ring containing *edge*."""
    require_base_topology(obj, "Insert edge loop")
    if not getattr(obj, "FormType", "").startswith("Forms::"):
        raise TypeError("The object is not a Forms object")
    cage, inserted_edges = ControlCage.from_object(obj).insert_edge_ring(edge, position, mode)
    obj.CageMode = "Editable"
    cage.write(obj)
    obj.touch()
    return obj, inserted_edges
