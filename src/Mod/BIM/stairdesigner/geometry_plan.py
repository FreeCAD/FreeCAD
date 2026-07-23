# SPDX-License-Identifier: LGPL-2.1-or-later

"""Stair footprints, plan partitions, and plan geometry."""

import bisect
import math

import FreeCAD
import Part

from .geometry_core import (
    BalancedSection,
    _cross,
    _shifted,
)

from .geometry_tangent import (
    _primitive_point,
    _primitive_tangent,
    _tangent_junction_modes,
    _tangent_path_primitives,
)

from .geometry_winders import (
    _safe_angle_tangent,
)

def make_tangent_stair_footprint(
    flight_specs,
    turn_types=None,
    start_angle=0.0,
    end_angle=0.0,
):
    """Return the exact strip footprint of a mixed straight/circular path."""

    if not flight_specs:
        return Part.Shape()
    specs, primitives = _tangent_path_primitives(flight_specs)
    corner_types = list(turn_types or [])
    corner_types.extend(
        ["Herse balancing"] * (len(specs) - 1 - len(corner_types))
    )
    modes = _tangent_junction_modes(
        primitives, corner_types[: len(specs) - 1]
    )
    start_extensions = [0.0] * len(primitives)
    end_extensions = [0.0] * len(primitives)
    for index, mode in enumerate(modes):
        if mode == "Tangent":
            continue
        incoming = primitives[index]
        outgoing = primitives[index + 1]
        turn_sine = abs(_cross(incoming["end_tangent"], outgoing["tangent"]))
        end_extensions[index] = outgoing["width"] / (2.0 * turn_sine)
        start_extensions[index + 1] = incoming["width"] / (2.0 * turn_sine)

    faces = []
    for index, primitive in enumerate(primitives):
        if primitive["type"] == "Circular":
            faces.append(_circular_primitive_face(primitive))
        else:
            direction = primitive["tangent"]
            normal = (-direction[1], direction[0])
            primitive_start = primitive.get(
                "face_start", primitive["start"]
            )
            primitive_end = primitive.get("face_end", primitive["end"])
            start = (
                primitive_start[0] - direction[0] * start_extensions[index],
                primitive_start[1] - direction[1] * start_extensions[index],
            )
            end = (
                primitive_end[0] + direction[0] * end_extensions[index],
                primitive_end[1] + direction[1] * end_extensions[index],
            )
            half_width = primitive["width"] / 2.0
            start_offset = 0.0
            end_offset = 0.0
            if index == 0:
                start_offset = half_width * _safe_angle_tangent(start_angle)
            if index == len(primitives) - 1:
                end_offset = half_width * _safe_angle_tangent(end_angle)
            faces.append(
                _horizontal_face(
                    (
                        (
                            start[0]
                            + normal[0] * half_width
                            + direction[0] * start_offset,
                            start[1]
                            + normal[1] * half_width
                            + direction[1] * start_offset,
                        ),
                        (
                            end[0]
                            + normal[0] * half_width
                            + direction[0] * end_offset,
                            end[1]
                            + normal[1] * half_width
                            + direction[1] * end_offset,
                        ),
                        (
                            end[0]
                            - normal[0] * half_width
                            - direction[0] * end_offset,
                            end[1]
                            - normal[1] * half_width
                            - direction[1] * end_offset,
                        ),
                        (
                            start[0]
                            - normal[0] * half_width
                            - direction[0] * start_offset,
                            start[1]
                            - normal[1] * half_width
                            - direction[1] * start_offset,
                        ),
                    ),
                    0.0,
                )
            )
    return _fuse_plan_faces(faces)


def tangent_tread_faces(sections, flight_specs):
    """Return tread cells clipped to their own tangent-path primitive.

    Explicit landing flights introduce internal plan seams, so their cells
    cannot be recovered by walking only the outer wire of the stair union.
    """

    _specs, primitives = _tangent_path_primitives(flight_specs)
    primitive_faces = [
        _circular_primitive_face(primitive)
        if primitive["type"] == "Circular"
        else _straight_primitive_face(primitive)
        for primitive in primitives
    ]
    result = []
    for front, rear in zip(sections, sections[1:]):
        if front.level_to_next:
            result.append(primitive_faces[front.flight_index])
            continue
        primitive_index = (
            rear.flight_index
            if rear.flight_index != front.flight_index
            and not rear.level_to_next
            else front.flight_index
        )
        primitive_face = primitive_faces[primitive_index]
        faces = _balanced_step_faces(front, rear, primitive_face)
        if not faces:
            return []
        result.append(
            faces[0] if len(faces) == 1 else _fuse_plan_faces(faces)
        )
    return result


def _straight_primitive_face(primitive):
    direction = primitive["tangent"]
    normal = (-direction[1], direction[0])
    half_width = primitive["width"] / 2.0
    start = primitive.get("face_start", primitive["start"])
    end = primitive.get("face_end", primitive["end"])
    return _horizontal_face(
        (
            (
                start[0] + normal[0] * half_width,
                start[1] + normal[1] * half_width,
            ),
            (
                end[0] + normal[0] * half_width,
                end[1] + normal[1] * half_width,
            ),
            (
                end[0] - normal[0] * half_width,
                end[1] - normal[1] * half_width,
            ),
            (
                start[0] - normal[0] * half_width,
                start[1] - normal[1] * half_width,
            ),
        ),
        0.0,
    )


def _circular_primitive_face(primitive):
    def vector(point):
        return FreeCAD.Vector(point[0], point[1], 0.0)

    def side_point(distance, side):
        center = _primitive_point(primitive, distance)
        tangent = _primitive_tangent(primitive, distance)
        normal = (-tangent[1], tangent[0])
        offset = side * primitive["width"] / 2.0
        return center[0] + normal[0] * offset, center[1] + normal[1] * offset

    half = primitive["length"] / 2.0
    left_start = side_point(0.0, 1.0)
    left_middle = side_point(half, 1.0)
    left_end = side_point(primitive["length"], 1.0)
    right_start = side_point(0.0, -1.0)
    right_middle = side_point(half, -1.0)
    right_end = side_point(primitive["length"], -1.0)
    edges = (
        Part.Arc(
            vector(left_start), vector(left_middle), vector(left_end)
        ).toShape(),
        Part.makeLine(vector(left_end), vector(right_end)),
        Part.Arc(
            vector(right_end), vector(right_middle), vector(right_start)
        ).toShape(),
        Part.makeLine(vector(right_start), vector(left_start)),
    )
    return Part.Face(Part.Wire(edges))


def _fuse_plan_faces(faces):
    result = faces[0]
    for face in faces[1:]:
        result = result.fuse(face)
        if len(result.Faces) == 1:
            result = result.Faces[0]
    result = result.removeSplitter()
    if len(result.Faces) == 1 and result.Faces[0].isValid():
        return result.Faces[0]
    solids = [face.extrude(FreeCAD.Vector(0.0, 0.0, 1.0)) for face in result.Faces]
    solid = solids[0]
    for addition in solids[1:]:
        solid = solid.fuse(addition)
    solid = solid.removeSplitter()
    horizontal = [face for face in solid.Faces if face.BoundBox.ZLength < 1e-7]
    if horizontal:
        return min(horizontal, key=lambda face: face.BoundBox.ZMin)
    return result


def fit_tangent_sections_to_footprint(sections, footprint):
    """Extend radial/tangent sections to the exact mixed-flight boundary."""

    if len(sections) < 2 or footprint.isNull():
        return sections
    extent = max(footprint.BoundBox.DiagonalLength * 2.0, 1000.0)
    fitted = []
    for section_index, section in enumerate(sections):
        center_vertex = Part.Vertex(
            FreeCAD.Vector(section.center[0], section.center[1], 0.0)
        )
        if (
            section_index in {0, len(sections) - 1}
            and footprint.OuterWire.distToShape(center_vertex)[0] < 1e-7
        ):
            # Entry/exit sections can intentionally occupy only part of an
            # outer side. Extending their coincident line would turn them
            # into the complete rail and destroy the endpoint winding.
            fitted.append(section)
            continue
        normal = (-section.tangent[1], section.tangent[0])
        line = Part.makeLine(
            FreeCAD.Vector(
                section.center[0] - normal[0] * extent,
                section.center[1] - normal[1] * extent,
                0.0,
            ),
            FreeCAD.Vector(
                section.center[0] + normal[0] * extent,
                section.center[1] + normal[1] * extent,
                0.0,
            ),
        )
        intervals = []
        for edge in footprint.common(line).Edges:
            parameters = [
                (vertex.Point.x - section.center[0]) * normal[0]
                + (vertex.Point.y - section.center[1]) * normal[1]
                for vertex in edge.Vertexes
            ]
            if parameters:
                intervals.append((min(parameters), max(parameters)))
        containing = [
            interval
            for interval in intervals
            if interval[0] - 1e-7 <= 0.0 <= interval[1] + 1e-7
        ]
        if containing:
            lower, upper = max(
                containing, key=lambda interval: interval[1] - interval[0]
            )
            left = (
                section.center[0] + normal[0] * upper,
                section.center[1] + normal[1] * upper,
            )
            right = (
                section.center[0] + normal[0] * lower,
                section.center[1] + normal[1] * lower,
            )
            width = upper - lower
        else:
            left, right, width = section.left, section.right, section.width
        fitted.append(
            BalancedSection(
                center=section.center,
                tangent=section.tangent,
                left=left,
                right=right,
                station=section.station,
                width=width,
                flight_index=section.flight_index,
                landing_to_next=section.landing_to_next,
                locked_to_flight=section.locked_to_flight,
                level_to_next=section.level_to_next,
                riser_index=section.riser_index,
            )
        )
    return fitted


def make_stair_footprint(flight_specs, start_angle=0.0, end_angle=0.0):
    """Return the mitered strip footprint of connected straight flights."""

    if not flight_specs:
        return Part.Shape()
    specs = [
        (max(float(length), 0.01), max(float(width), 0.01), float(heading))
        for length, width, heading in flight_specs
    ]
    directions = []
    for _length, _width, heading in specs:
        radians = math.radians(heading)
        directions.append((math.cos(radians), math.sin(radians)))
    center = (0.0, specs[0][1] / 2.0)
    vertices = [center]
    for (length, _width, _heading), direction in zip(specs, directions):
        center = (
            center[0] + direction[0] * length,
            center[1] + direction[1] * length,
        )
        vertices.append(center)

    left_starts = []
    right_starts = []
    left_ends = []
    right_ends = []
    for index, (_length, width, _heading) in enumerate(specs):
        direction = directions[index]
        normal = (-direction[1], direction[0])
        half_width = width / 2.0
        start = vertices[index]
        end = vertices[index + 1]
        left_starts.append(
            (start[0] + normal[0] * half_width, start[1] + normal[1] * half_width)
        )
        right_starts.append(
            (start[0] - normal[0] * half_width, start[1] - normal[1] * half_width)
        )
        left_ends.append(
            (end[0] + normal[0] * half_width, end[1] + normal[1] * half_width)
        )
        right_ends.append(
            (end[0] - normal[0] * half_width, end[1] - normal[1] * half_width)
        )

    for index in range(len(specs) - 1):
        incoming = directions[index]
        outgoing = directions[index + 1]
        left_corner = _line_intersection(
            left_ends[index], incoming, left_starts[index + 1], outgoing
        )
        right_corner = _line_intersection(
            right_ends[index], incoming, right_starts[index + 1], outgoing
        )
        if left_corner is None or right_corner is None:
            continue
        left_ends[index] = left_corner
        left_starts[index + 1] = left_corner
        right_ends[index] = right_corner
        right_starts[index + 1] = right_corner

    start_offset = specs[0][1] / 2.0 * _safe_angle_tangent(start_angle)
    start_direction = directions[0]
    left_starts[0] = (
        left_starts[0][0] + start_direction[0] * start_offset,
        left_starts[0][1] + start_direction[1] * start_offset,
    )
    right_starts[0] = (
        right_starts[0][0] - start_direction[0] * start_offset,
        right_starts[0][1] - start_direction[1] * start_offset,
    )
    end_offset = specs[-1][1] / 2.0 * _safe_angle_tangent(end_angle)
    end_direction = directions[-1]
    left_ends[-1] = (
        left_ends[-1][0] + end_direction[0] * end_offset,
        left_ends[-1][1] + end_direction[1] * end_offset,
    )
    right_ends[-1] = (
        right_ends[-1][0] - end_direction[0] * end_offset,
        right_ends[-1][1] - end_direction[1] * end_offset,
    )

    faces = []
    for index in range(len(specs)):
        try:
            face = _horizontal_face(
                (
                    left_starts[index],
                    left_ends[index],
                    right_ends[index],
                    right_starts[index],
                ),
                0.0,
            )
        except Part.OCCError:
            return Part.Shape()
        if not face.isValid() or face.Area < 1e-7:
            return Part.Shape()
        faces.append(face)
    planar_result = faces[0]
    for face in faces[1:]:
        planar_result = planar_result.fuse(face)
        if len(planar_result.Faces) == 1:
            planar_result = planar_result.Faces[0]
    planar_result = planar_result.removeSplitter()
    if len(planar_result.Faces) == 1 and planar_result.Faces[0].isValid():
        return planar_result.Faces[0]

    # Coplanar fusion sometimes leaves a shell with internal seams (notably
    # on a U footprint).  Fusing a thin extrusion is more reliable in OCCT;
    # its lower face is the same two-dimensional outline with one clean wire.
    solids = [
        face.extrude(FreeCAD.Vector(0.0, 0.0, 1.0))
        for face in planar_result.Faces
    ]
    result = solids[0]
    for solid in solids[1:]:
        result = result.fuse(solid)
    result = result.removeSplitter()
    horizontal_faces = [
        face for face in result.Faces if face.BoundBox.ZLength < 1e-7
    ]
    if horizontal_faces:
        return min(horizontal_faces, key=lambda face: face.BoundBox.ZMin)
    return planar_result


def _line_intersection(first_point, first_direction, second_point, second_direction):
    denominator = _cross(first_direction, second_direction)
    if abs(denominator) < 1e-9:
        return None
    relative = (
        second_point[0] - first_point[0],
        second_point[1] - first_point[1],
    )
    parameter = _cross(relative, second_direction) / denominator
    return (
        first_point[0] + first_direction[0] * parameter,
        first_point[1] + first_direction[1] * parameter,
    )


def fit_balanced_sections_to_footprint(sections, footprint):
    """Make tread endpoints advance monotonically along both stair rails."""

    if len(sections) < 2 or footprint.isNull():
        return sections
    sections = fit_tangent_sections_to_footprint(sections, footprint)
    boundary = _boundary_data(footprint)
    orientation = 1.0 if boundary["signed_area"] > 0.0 else -1.0
    left_parameters = _monotone_boundary_parameters(
        [section.left for section in sections],
        sections,
        boundary,
        -orientation,
    )
    right_parameters = _monotone_boundary_parameters(
        [section.right for section in sections],
        sections,
        boundary,
        orientation,
    )

    fitted = []
    for index, (section, left_parameter, right_parameter) in enumerate(
        zip(sections, left_parameters, right_parameters)
    ):
        center_on_boundary = footprint.OuterWire.distToShape(
            Part.Vertex(
                FreeCAD.Vector(section.center[0], section.center[1], 0.0)
            )
        )[0] < 1e-7
        if (
            section.locked_to_flight
            or center_on_boundary
            or section.landing_to_next
            or (index and sections[index - 1].landing_to_next)
        ):
            fitted.append(section)
            continue
        left = _boundary_point(left_parameter, boundary)
        right = _boundary_point(right_parameter, boundary)
        left, right = _clip_chord_to_boundary(left, right, boundary)
        chord = (left[0] - right[0], left[1] - right[1])
        chord_length = math.hypot(chord[0], chord[1])
        if chord_length > 1e-9:
            tangent = (chord[1] / chord_length, -chord[0] / chord_length)
            if (
                tangent[0] * section.tangent[0]
                + tangent[1] * section.tangent[1]
                < 0.0
            ):
                tangent = (-tangent[0], -tangent[1])
        else:
            tangent = section.tangent
        center = (
            (left[0] + right[0]) / 2.0,
            (left[1] + right[1]) / 2.0,
        )
        fitted.append(
            BalancedSection(
                center=center,
                tangent=tangent,
                left=left,
                right=right,
                station=section.station,
                width=chord_length,
                flight_index=section.flight_index,
                landing_to_next=section.landing_to_next,
                locked_to_flight=section.locked_to_flight,
                level_to_next=section.level_to_next,
                riser_index=section.riser_index,
            )
        )
    return fitted


def _clip_chord_to_boundary(left, right, boundary):
    """Keep only the connected part of a nosing that lies in the footprint."""

    midpoint = ((left[0] + right[0]) / 2.0, (left[1] + right[1]) / 2.0)
    chord = (left[0] - right[0], left[1] - right[1])
    chord_length = math.hypot(chord[0], chord[1])
    if chord_length < 1e-9:
        return left, right
    direction = (chord[0] / chord_length, chord[1] / chord_length)
    parameters = []
    vertices = boundary["vertices"]
    for first, second in zip(vertices, vertices[1:] + vertices[:1]):
        edge = (second[0] - first[0], second[1] - first[1])
        relative = (first[0] - midpoint[0], first[1] - midpoint[1])
        denominator = direction[0] * edge[1] - direction[1] * edge[0]
        if abs(denominator) < 1e-12:
            if abs(relative[0] * direction[1] - relative[1] * direction[0]) < 1e-7:
                parameters.extend(
                    (
                        relative[0] * direction[0] + relative[1] * direction[1],
                        (second[0] - midpoint[0]) * direction[0]
                        + (second[1] - midpoint[1]) * direction[1],
                    )
                )
            continue
        edge_ratio = (
            relative[0] * direction[1] - relative[1] * direction[0]
        ) / denominator
        if -1e-9 <= edge_ratio <= 1.0 + 1e-9:
            parameters.append(
                (relative[0] * edge[1] - relative[1] * edge[0]) / denominator
            )
    parameters.sort()
    unique = []
    for parameter in parameters:
        if not unique or abs(parameter - unique[-1]) > 1e-7:
            unique.append(parameter)
    intervals = []
    for lower, upper in zip(unique, unique[1:]):
        probe = (lower + upper) / 2.0
        point = (
            midpoint[0] + direction[0] * probe,
            midpoint[1] + direction[1] * probe,
        )
        if _point_in_boundary(point, boundary):
            intervals.append((lower, upper))
    if not intervals:
        return left, right
    lower, upper = min(
        intervals,
        key=lambda interval: (
            0.0
            if interval[0] - 1e-7 <= 0.0 <= interval[1] + 1e-7
            else min(abs(interval[0]), abs(interval[1])),
            -(interval[1] - interval[0]),
        ),
    )
    return (
        (midpoint[0] + direction[0] * upper, midpoint[1] + direction[1] * upper),
        (midpoint[0] + direction[0] * lower, midpoint[1] + direction[1] * lower),
    )


def _point_in_boundary(point, boundary):
    inside = False
    previous = boundary["vertices"][-1]
    for current in boundary["vertices"]:
        if (current[1] > point[1]) != (previous[1] > point[1]):
            crossing = (
                (previous[0] - current[0])
                * (point[1] - current[1])
                / (previous[1] - current[1])
                + current[0]
            )
            if point[0] < crossing:
                inside = not inside
        previous = current
    return inside


def balanced_tread_faces(sections, footprint):
    """Partition a stair footprint into consecutive, non-overlapping faces."""

    if len(sections) < 2 or footprint.isNull():
        return []
    if any(
        abs(
            edge.curvatureAt(
                (edge.FirstParameter + edge.LastParameter) / 2.0
            )
        )
        > 1e-9
        for edge in footprint.Edges
    ):
        return _tangent_tread_faces(sections, footprint)
    boundary = _boundary_data(footprint)
    orientation = 1.0 if boundary["signed_area"] > 0.0 else -1.0
    left_parameters = _unwrap_boundary_parameters(
        [section.left for section in sections], boundary
    )
    right_parameters = _unwrap_boundary_parameters(
        [section.right for section in sections], boundary
    )
    left_direction = -orientation
    right_direction = orientation

    faces = []
    for index, (front, rear) in enumerate(zip(sections, sections[1:])):
        points = [front.left, front.right]
        points.extend(
            _boundary_vertices_between(
                right_parameters[index],
                right_parameters[index + 1],
                right_direction,
                boundary,
            )
        )
        points.extend((rear.right, rear.left))
        points.extend(
            _boundary_vertices_between(
                left_parameters[index + 1],
                left_parameters[index],
                -left_direction,
                boundary,
            )
        )
        faces.append(_horizontal_face(_without_duplicate_points(points), 0.0))
    return faces


def _tangent_tread_faces(sections, footprint):
    """Partition a curved footprint with consecutive tangent half-planes."""

    result = []
    for front, rear in zip(sections, sections[1:]):
        candidates = _balanced_step_faces(front, rear, footprint)
        if not candidates:
            result.append(Part.Shape())
            continue
        tread = candidates[0]
        for candidate in candidates[1:]:
            tread = tread.fuse(candidate)
        result.append(tread.removeSplitter())
    return result


def balanced_partition_is_valid(faces, footprint, expected_count):
    """Return whether a footprint partition is safe to turn into solids."""

    if len(faces) != expected_count or footprint.isNull():
        return False
    if any(face.isNull() or not face.isValid() for face in faces):
        return False
    tolerance = max(footprint.Area * 1e-7, 1e-3)
    return abs(sum(face.Area for face in faces) - footprint.Area) <= tolerance


def _boundary_data(footprint):
    face = max(footprint.Faces, key=lambda item: item.Area)
    vertices = [
        (vertex.Point.x, vertex.Point.y)
        for vertex in face.OuterWire.OrderedVertexes
    ]
    cumulative = [0.0]
    for first, second in zip(vertices, vertices[1:] + vertices[:1]):
        cumulative.append(
            cumulative[-1]
            + math.hypot(second[0] - first[0], second[1] - first[1])
        )
    signed_area = 0.5 * sum(
        first[0] * second[1] - second[0] * first[1]
        for first, second in zip(vertices, vertices[1:] + vertices[:1])
    )
    return {
        "vertices": vertices,
        "cumulative": cumulative,
        "perimeter": cumulative[-1],
        "signed_area": signed_area,
    }


def _boundary_candidates(point, boundary):
    candidates = []
    vertices = boundary["vertices"]
    cumulative = boundary["cumulative"]
    scale = max(boundary["perimeter"], 1.0)
    tolerance = scale * 1e-7
    for index, (first, second) in enumerate(
        zip(vertices, vertices[1:] + vertices[:1])
    ):
        delta = (second[0] - first[0], second[1] - first[1])
        squared_length = delta[0] * delta[0] + delta[1] * delta[1]
        if squared_length < 1e-18:
            continue
        ratio = (
            (point[0] - first[0]) * delta[0]
            + (point[1] - first[1]) * delta[1]
        ) / squared_length
        ratio = min(max(ratio, 0.0), 1.0)
        projected = (
            first[0] + delta[0] * ratio,
            first[1] + delta[1] * ratio,
        )
        if math.hypot(projected[0] - point[0], projected[1] - point[1]) <= tolerance:
            candidates.append(
                cumulative[index] + math.sqrt(squared_length) * ratio
            )
    if not candidates:
        raise ValueError("Balanced section does not meet the stair footprint boundary")
    return candidates


def _unwrap_boundary_parameters(points, boundary):
    perimeter = boundary["perimeter"]
    result = []
    for point in points:
        candidates = _boundary_candidates(point, boundary)
        if not result:
            result.append(min(candidates))
            continue
        previous = result[-1]
        expanded = []
        for candidate in candidates:
            cycle = round((previous - candidate) / perimeter)
            expanded.extend(
                candidate + (cycle + offset) * perimeter
                for offset in (-1, 0, 1)
            )
        result.append(min(expanded, key=lambda value: abs(value - previous)))
    return result


def _monotone_boundary_parameters(points, sections, boundary, direction):
    raw = _unwrap_boundary_parameters(points, boundary)
    start = raw[0]
    total = direction * (raw[-1] - start)
    if total <= 1e-7:
        total += boundary["perimeter"]
    progress = [
        min(max(direction * (parameter - start), 0.0), total)
        for parameter in raw
    ]
    progress[0] = 0.0
    progress[-1] = total
    monotone = _isotonic_increasing(progress)
    first_station = sections[0].station
    station_span = max(sections[-1].station - first_station, 1e-9)
    result = []
    for index, (section, fitted) in enumerate(zip(sections, monotone)):
        linear = total * (section.station - first_station) / station_span
        blended = fitted * 0.9 + linear * 0.1
        if index == 0:
            blended = 0.0
        elif index == len(sections) - 1:
            blended = total
        result.append(start + direction * blended)
    return result


def _isotonic_increasing(values):
    blocks = []
    last_index = len(values) - 1
    for index, value in enumerate(values):
        weight = 1e12 if index in (0, last_index) else 1.0
        blocks.append([index, index, weight, value * weight])
        while len(blocks) > 1:
            previous = blocks[-2]
            current = blocks[-1]
            if previous[3] / previous[2] <= current[3] / current[2]:
                break
            blocks[-2:] = [
                [
                    previous[0],
                    current[1],
                    previous[2] + current[2],
                    previous[3] + current[3],
                ]
            ]
    result = [0.0] * len(values)
    for start, end, weight, weighted_value in blocks:
        value = weighted_value / weight
        for index in range(start, end + 1):
            result[index] = value
    result[0] = values[0]
    result[-1] = values[-1]
    return result


def _boundary_point(parameter, boundary):
    perimeter = boundary["perimeter"]
    parameter %= perimeter
    cumulative = boundary["cumulative"]
    vertices = boundary["vertices"]
    index = min(
        bisect.bisect_right(cumulative, parameter) - 1,
        len(vertices) - 1,
    )
    start = cumulative[index]
    end = cumulative[index + 1]
    ratio = (parameter - start) / (end - start) if end > start else 0.0
    first = vertices[index]
    second = vertices[(index + 1) % len(vertices)]
    return (
        first[0] + (second[0] - first[0]) * ratio,
        first[1] + (second[1] - first[1]) * ratio,
    )


def _boundary_vertices_between(start, end, direction, boundary):
    perimeter = boundary["perimeter"]
    lower = min(start, end)
    upper = max(start, end)
    candidates = []
    first_cycle = math.floor(lower / perimeter) - 1
    last_cycle = math.ceil(upper / perimeter) + 1
    for cycle in range(first_cycle, last_cycle + 1):
        for parameter, point in zip(
            boundary["cumulative"][:-1], boundary["vertices"]
        ):
            unwrapped = parameter + cycle * perimeter
            if lower + 1e-7 < unwrapped < upper - 1e-7:
                candidates.append((unwrapped, point))
    candidates.sort(key=lambda item: item[0], reverse=direction < 0.0)
    return [point for _parameter, point in candidates]


def _without_duplicate_points(points):
    result = []
    for point in points:
        if not result or math.hypot(
            point[0] - result[-1][0], point[1] - result[-1][1]
        ) > 1e-7:
            result.append(point)
    if len(result) > 1 and math.hypot(
        result[0][0] - result[-1][0], result[0][1] - result[-1][1]
    ) < 1e-7:
        result.pop()
    return result


def _horizontal_face(points, elevation):
    vectors = [FreeCAD.Vector(point[0], point[1], elevation) for point in points]
    vectors.append(vectors[0])
    return Part.Face(Part.makePolygon(vectors))


def _balanced_step_faces(
    front,
    rear,
    footprint,
    front_offset=0.0,
    rear_offset=0.0,
):
    front_center = _shifted(front.center, front.tangent, front_offset)
    rear_center = _shifted(rear.center, rear.tangent, rear_offset)
    extent = max(
        footprint.BoundBox.DiagonalLength * 4.0,
        abs(front_offset) + abs(rear_offset) + 1000.0,
    )
    after_front = _half_plane_face(front_center, front.tangent, extent)
    before_rear = _half_plane_face(
        rear_center,
        (-rear.tangent[0], -rear.tangent[1]),
        extent,
    )
    clipped = footprint.common(after_front).common(before_rear)
    candidates = clipped.Faces
    if not candidates:
        return []
    midpoint = FreeCAD.Vector(
        (front.center[0] + rear.center[0]) / 2.0,
        (front.center[1] + rear.center[1]) / 2.0,
        0.0,
    )
    for face in candidates:
        if face.isInside(midpoint, 1e-6, True):
            seed = face
            break
    else:
        point = Part.Vertex(midpoint)
        seed = min(candidates, key=lambda face: face.distToShape(point)[0])

    selected = [seed]
    remaining = [face for face in candidates if not face.isSame(seed)]
    changed = True
    while changed:
        changed = False
        for face in list(remaining):
            if any(face.distToShape(current)[0] < 1e-7 for current in selected):
                selected.append(face)
                remaining.remove(face)
                changed = True
    return selected


def _half_plane_face(center, forward, extent):
    normal = (-forward[1], forward[0])
    start_left = (
        center[0] + normal[0] * extent,
        center[1] + normal[1] * extent,
    )
    start_right = (
        center[0] - normal[0] * extent,
        center[1] - normal[1] * extent,
    )
    return _horizontal_face(
        (
            start_left,
            start_right,
            (
                start_right[0] + forward[0] * extent,
                start_right[1] + forward[1] * extent,
            ),
            (
                start_left[0] + forward[0] * extent,
                start_left[1] + forward[1] * extent,
            ),
        ),
        0.0,
    )
