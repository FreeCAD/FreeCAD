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

"""Geometry-independent control-cage topology helpers."""

import math


def validate_manifold_boundary(edge_counts, label="control cage"):
    """Reject free boundaries that branch or meet only at one vertex."""
    neighbors = {}
    for edge, count in edge_counts.items():
        if count != 1:
            continue
        first, second = edge
        neighbors.setdefault(first, set()).add(second)
        neighbors.setdefault(second, set()).add(first)
    if any(len(adjacent) != 2 for adjacent in neighbors.values()):
        raise ValueError(
            f"The {label} has non-manifold boundaries meeting at a vertex"
        )


def _average(points):
    count = len(points)
    return tuple(sum(point[axis] for point in points) / count for axis in range(3))


def _validate_topology(vertices, faces):
    if not vertices:
        raise ValueError("A control cage requires at least one vertex")
    for face in faces:
        if len(face) < 3:
            raise ValueError("Control-cage faces require at least three vertices")
        if len(set(face)) != len(face):
            raise ValueError("Control-cage faces cannot repeat a vertex")
        if any(index < 0 or index >= len(vertices) for index in face):
            raise ValueError("Control-cage face index is out of range")


def _largest_covariance_axis(points):
    """Return the centroid and principal least-squares line direction."""
    center = tuple(sum(point[axis] for point in points) / len(points) for axis in range(3))
    covariance = [[0.0] * 3 for _axis in range(3)]
    for point in points:
        delta = tuple(point[axis] - center[axis] for axis in range(3))
        for row in range(3):
            for column in range(3):
                covariance[row][column] += delta[row] * delta[column]
    axis = max(range(3), key=lambda index: covariance[index][index])
    direction = [1.0 if index == axis else 0.0 for index in range(3)]
    for _iteration in range(32):
        candidate = [
            sum(covariance[row][column] * direction[column] for column in range(3))
            for row in range(3)
        ]
        magnitude = math.sqrt(sum(value * value for value in candidate))
        if magnitude <= 1.0e-12:
            raise ValueError("Straighten requires distinct control points")
        candidate = [value / magnitude for value in candidate]
        if sum((candidate[index] - direction[index]) ** 2 for index in range(3)) <= 1.0e-24:
            direction = candidate
            break
        direction = candidate
    return center, tuple(direction)


def straighten_points(vertices, selected_indices, line=None):
    """Project selected vertices onto a best-fit or explicit line.

    ``line`` is an optional ``(origin, direction)`` pair. An origin of
    ``None`` places a parallel line through the selected points' centroid.
    """
    result = [tuple(float(value) for value in point) for point in vertices]
    indices = sorted({int(index) for index in selected_indices})
    if not indices or any(index < 0 or index >= len(result) for index in indices):
        raise ValueError("Straighten requires valid control points")
    points = [result[index] for index in indices]
    if line is None:
        if len(indices) < 3:
            raise ValueError("Best-fit Straighten requires at least three control points")
        origin, direction = _largest_covariance_axis(points)
    else:
        if line[0] is None:
            if len(indices) < 2:
                raise ValueError("Parallel Straighten requires at least two control points")
            origin = tuple(
                sum(point[axis] for point in points) / len(points) for axis in range(3)
            )
        else:
            origin = tuple(float(value) for value in line[0])
        direction = tuple(float(value) for value in line[1])
        magnitude = math.sqrt(sum(value * value for value in direction))
        if magnitude <= 1.0e-12:
            raise ValueError("Straighten line direction must not be zero")
        direction = tuple(value / magnitude for value in direction)
    for index in indices:
        offset = tuple(result[index][axis] - origin[axis] for axis in range(3))
        parameter = sum(offset[axis] * direction[axis] for axis in range(3))
        result[index] = tuple(
            origin[axis] + parameter * direction[axis] for axis in range(3)
        )
    return result


def cage_vertex_range(faces, start, end):
    """Return the unique shortest cage-edge path between two controls."""
    start, end = int(start), int(end)
    if start == end:
        raise ValueError("Range selection requires two different control points")
    adjacency = {}
    for first, second in cage_edges(faces):
        adjacency.setdefault(first, set()).add(second)
        adjacency.setdefault(second, set()).add(first)
    if start not in adjacency or end not in adjacency:
        raise ValueError("Range selection requires controls in the same cage")
    distance = {start: 0}
    paths = {start: 1}
    previous = {}
    pending = [start]
    for current in pending:
        for neighbor in adjacency[current]:
            candidate = distance[current] + 1
            if neighbor not in distance:
                distance[neighbor] = candidate
                paths[neighbor] = paths[current]
                previous[neighbor] = current
                pending.append(neighbor)
            elif distance[neighbor] == candidate:
                paths[neighbor] += paths[current]
    if end not in distance:
        raise ValueError("The selected controls are not connected")
    if paths[end] != 1:
        raise ValueError("Range selection is ambiguous; select controls in one cage row")
    result = [end]
    while result[-1] != start:
        result.append(previous[result[-1]])
    return list(reversed(result))


def _all_shortest_range(adjacency, start, end):
    """Return every graph node lying on a shortest route between two nodes."""
    if start == end:
        return {start}
    if start not in adjacency or end not in adjacency:
        raise ValueError("Range selection requires elements in the same cage")

    def distances(origin):
        result = {origin: 0}
        pending = [origin]
        for current in pending:
            for neighbor in adjacency.get(current, ()):
                if neighbor not in result:
                    result[neighbor] = result[current] + 1
                    pending.append(neighbor)
        return result

    from_start = distances(start)
    if end not in from_start:
        raise ValueError("The selected elements are not connected")
    from_end = distances(end)
    length = from_start[end]
    return {
        node
        for node in adjacency
        if from_start.get(node, length + 1) + from_end.get(node, length + 1) == length
    }


def cage_vertex_selection_range(faces, start, end):
    """Return the Excel-style shortest-path range between two cage vertices."""
    adjacency = {}
    for first, second in cage_edges(faces):
        adjacency.setdefault(first, set()).add(second)
        adjacency.setdefault(second, set()).add(first)
    return _all_shortest_range(adjacency, int(start), int(end))


def cage_edge_selection_range(faces, start, end):
    """Return the shortest connected range between two cage edges."""
    edges = cage_edges(faces)
    by_vertex = {}
    for edge in edges:
        by_vertex.setdefault(edge[0], set()).add(edge)
        by_vertex.setdefault(edge[1], set()).add(edge)
    adjacency = {edge: set() for edge in edges}
    for connected in by_vertex.values():
        for edge in connected:
            adjacency[edge].update(connected.difference((edge,)))
    start = tuple(sorted((int(start[0]), int(start[1]))))
    end = tuple(sorted((int(end[0]), int(end[1]))))
    return _all_shortest_range(adjacency, start, end)


def cage_face_selection_range(faces, start, end):
    """Return every face on a shortest dual-graph route between two faces."""
    faces = [tuple(int(vertex) for vertex in face) for face in faces]
    edge_faces = {}
    for face_index, face in enumerate(faces):
        for position, first in enumerate(face):
            edge = tuple(sorted((first, face[(position + 1) % len(face)])))
            edge_faces.setdefault(edge, []).append(face_index)
    adjacency = {face_index: set() for face_index in range(len(faces))}
    for adjacent in edge_faces.values():
        if len(adjacent) == 2:
            first, second = adjacent
            adjacency[first].add(second)
            adjacency[second].add(first)
    return _all_shortest_range(adjacency, int(start), int(end))


def _smallest_covariance_axis(points):
    """Return the least-variance axis of 3D points using Jacobi rotations."""
    center = tuple(sum(point[axis] for point in points) / len(points) for axis in range(3))
    covariance = [[0.0] * 3 for _ in range(3)]
    for point in points:
        delta = tuple(point[axis] - center[axis] for axis in range(3))
        for row in range(3):
            for column in range(3):
                covariance[row][column] += delta[row] * delta[column]
    vectors = [[1.0 if row == column else 0.0 for column in range(3)] for row in range(3)]
    for _iteration in range(24):
        p, q = max(((0, 1), (0, 2), (1, 2)), key=lambda pair: abs(covariance[pair[0]][pair[1]]))
        if abs(covariance[p][q]) <= 1.0e-14:
            break
        angle = 0.5 * math.atan2(2.0 * covariance[p][q], covariance[q][q] - covariance[p][p])
        cosine, sine = math.cos(angle), math.sin(angle)
        for index in range(3):
            if index in (p, q):
                continue
            first = covariance[index][p]
            second = covariance[index][q]
            covariance[index][p] = covariance[p][index] = cosine * first - sine * second
            covariance[index][q] = covariance[q][index] = sine * first + cosine * second
        app, aqq, apq = covariance[p][p], covariance[q][q], covariance[p][q]
        covariance[p][p] = cosine * cosine * app - 2.0 * sine * cosine * apq + sine * sine * aqq
        covariance[q][q] = sine * sine * app + 2.0 * sine * cosine * apq + cosine * cosine * aqq
        covariance[p][q] = covariance[q][p] = 0.0
        for row in range(3):
            first = vectors[row][p]
            second = vectors[row][q]
            vectors[row][p] = cosine * first - sine * second
            vectors[row][q] = sine * first + cosine * second
    axis = min(range(3), key=lambda index: covariance[index][index])
    normal = tuple(vectors[row][axis] for row in range(3))
    magnitude = math.sqrt(sum(value * value for value in normal))
    return center, tuple(value / magnitude for value in normal)


def flatten_points(vertices, selected_indices, plane=None):
    """Project selected vertices onto a best-fit or explicit plane.

    ``plane`` is an optional ``(origin, normal)`` pair expressed in the same
    coordinate system as the control points. If origin is ``None``, the plane
    passes through the selected points' centroid.
    """
    result = [tuple(float(value) for value in point) for point in vertices]
    indices = sorted({int(index) for index in selected_indices})
    if len(indices) < 3 or any(index < 0 or index >= len(result) for index in indices):
        raise ValueError("Flatten requires at least three valid control points")
    points = [result[index] for index in indices]
    if plane is None:
        center, normal = _smallest_covariance_axis(points)
        spread = max(
            math.sqrt(sum((point[axis] - center[axis]) ** 2 for axis in range(3)))
            for point in points
        )
        area = max(
            math.sqrt(
                sum(
                    value * value
                    for value in (
                        (points[j][1] - points[0][1]) * (points[k][2] - points[0][2])
                        - (points[j][2] - points[0][2]) * (points[k][1] - points[0][1]),
                        (points[j][2] - points[0][2]) * (points[k][0] - points[0][0])
                        - (points[j][0] - points[0][0]) * (points[k][2] - points[0][2]),
                        (points[j][0] - points[0][0]) * (points[k][1] - points[0][1])
                        - (points[j][1] - points[0][1]) * (points[k][0] - points[0][0]),
                    )
                )
            )
            for j in range(1, len(points))
            for k in range(j + 1, len(points))
        )
        if spread <= 1.0e-12 or area <= spread * spread * 1.0e-10:
            raise ValueError("Flatten requires non-collinear control points")
    else:
        center = (
            tuple(sum(point[axis] for point in points) / len(points) for axis in range(3))
            if plane[0] is None
            else tuple(float(value) for value in plane[0])
        )
        normal = tuple(float(value) for value in plane[1])
        magnitude = math.sqrt(sum(value * value for value in normal))
        if magnitude <= 1.0e-12:
            raise ValueError("Flatten plane normal must not be zero")
        normal = tuple(value / magnitude for value in normal)
    for index in indices:
        point = result[index]
        distance = sum((point[axis] - center[axis]) * normal[axis] for axis in range(3))
        result[index] = tuple(point[axis] - distance * normal[axis] for axis in range(3))
    return result


def box_control_cage(length, width, height, x_segments=1, y_segments=1, z_segments=1):
    """Return unique vertices and outward-facing quads for a segmented box.

    Vertices are tuples, making this function testable without loading FreeCAD.
    Every boundary grid point is shared by all incident faces.
    """
    dimensions = (float(length), float(width), float(height))
    segments = (int(x_segments), int(y_segments), int(z_segments))
    if any(value <= 0 for value in dimensions):
        raise ValueError("Box dimensions must be positive")
    if any(value < 1 for value in segments):
        raise ValueError("Box segment counts must be at least one")

    vertices = []
    vertex_indices = {}
    faces = []

    def vertex(i, j, k):
        key = (i, j, k)
        if key not in vertex_indices:
            vertex_indices[key] = len(vertices)
            vertices.append(
                (
                    dimensions[0] * i / segments[0] - dimensions[0] / 2.0,
                    dimensions[1] * j / segments[1] - dimensions[1] / 2.0,
                    dimensions[2] * k / segments[2] - dimensions[2] / 2.0,
                )
            )
        return vertex_indices[key]

    sx, sy, sz = segments

    # X-normal faces.
    for j in range(sy):
        for k in range(sz):
            faces.append(
                (
                    vertex(0, j, k),
                    vertex(0, j, k + 1),
                    vertex(0, j + 1, k + 1),
                    vertex(0, j + 1, k),
                )
            )
            faces.append(
                (
                    vertex(sx, j, k),
                    vertex(sx, j + 1, k),
                    vertex(sx, j + 1, k + 1),
                    vertex(sx, j, k + 1),
                )
            )

    # Y-normal faces.
    for i in range(sx):
        for k in range(sz):
            faces.append(
                (
                    vertex(i, 0, k),
                    vertex(i + 1, 0, k),
                    vertex(i + 1, 0, k + 1),
                    vertex(i, 0, k + 1),
                )
            )
            faces.append(
                (
                    vertex(i, sy, k),
                    vertex(i, sy, k + 1),
                    vertex(i + 1, sy, k + 1),
                    vertex(i + 1, sy, k),
                )
            )

    # Z-normal faces.
    for i in range(sx):
        for j in range(sy):
            faces.append(
                (
                    vertex(i, j, 0),
                    vertex(i, j + 1, 0),
                    vertex(i + 1, j + 1, 0),
                    vertex(i + 1, j, 0),
                )
            )
            faces.append(
                (
                    vertex(i, j, sz),
                    vertex(i + 1, j, sz),
                    vertex(i + 1, j + 1, sz),
                    vertex(i, j + 1, sz),
                )
            )

    return vertices, faces


def face_control_cage(length, width, x_segments=2, y_segments=2):
    """Return a rectangular open quad grid in the XY plane."""
    length = float(length)
    width = float(width)
    x_segments = int(x_segments)
    y_segments = int(y_segments)
    if length <= 0 or width <= 0:
        raise ValueError("Face dimensions must be positive")
    if x_segments < 1 or y_segments < 1:
        raise ValueError("Face segment counts must be at least one")
    vertices = [
        (
            length * i / x_segments - length / 2.0,
            width * j / y_segments - width / 2.0,
            0.0,
        )
        for j in range(y_segments + 1)
        for i in range(x_segments + 1)
    ]

    def index(i, j):
        return j * (x_segments + 1) + i

    faces = [
        (index(i, j), index(i + 1, j), index(i + 1, j + 1), index(i, j + 1))
        for j in range(y_segments)
        for i in range(x_segments)
    ]
    return vertices, faces


def resize_structured_cage(
    vertices,
    old_segments,
    new_segments,
    surface=False,
    vertex_sharpness=None,
    edge_sharpness=None,
    return_sharpness=False,
):
    """Increase structured cage segments while retaining its deformed control net.

    New control rows are linearly interpolated from the existing logical grid.
    Counts may only increase; removing rows would necessarily discard edits.
    """
    old_segments = tuple(int(value) for value in old_segments)
    new_segments = tuple(int(value) for value in new_segments)
    expected_axes = 2 if surface else 3
    if len(old_segments) != expected_axes or len(new_segments) != expected_axes:
        raise ValueError("Structured cage segment dimensions do not match")
    if any(value < 1 for value in old_segments + new_segments):
        raise ValueError("Segment counts must be at least one")
    if any(new < old for old, new in zip(old_segments, new_segments)):
        raise ValueError("Structured cage segments cannot be removed")

    def topology(segments):
        if surface:
            reference, faces = face_control_cage(2.0, 2.0, *segments)
            sx, sy = segments
            keys = [
                (round((x + 1.0) * sx / 2.0), round((y + 1.0) * sy / 2.0)) for x, y, _z in reference
            ]
        else:
            reference, faces = box_control_cage(2.0, 2.0, 2.0, *segments)
            sx, sy, sz = segments
            keys = [
                (
                    round((x + 1.0) * sx / 2.0),
                    round((y + 1.0) * sy / 2.0),
                    round((z + 1.0) * sz / 2.0),
                )
                for x, y, z in reference
            ]
        return keys, faces

    current_vertices = [tuple(float(component) for component in point) for point in vertices]
    current_segments = old_segments
    current_keys, current_faces = topology(current_segments)
    if len(current_vertices) != len(current_keys):
        raise ValueError("Control points do not match the structured cage topology")
    current_vertex_sharpness = [max(0.0, float(value)) for value in (vertex_sharpness or ())]
    current_vertex_sharpness.extend([0.0] * (len(current_vertices) - len(current_vertex_sharpness)))
    current_vertex_sharpness = current_vertex_sharpness[: len(current_vertices)]
    current_edge_sharpness = {
        tuple(sorted((int(edge[0]), int(edge[1])))): max(0.0, float(value))
        for edge, value in (edge_sharpness or {}).items()
        if float(value) > 0.0
    }

    for axis, target_count in enumerate(new_segments):
        if target_count == current_segments[axis]:
            continue
        target_segments = list(current_segments)
        target_segments[axis] = target_count
        target_segments = tuple(target_segments)
        target_keys, target_faces = topology(target_segments)
        current_by_key = dict(zip(current_keys, current_vertices))
        sharpness_by_key = dict(zip(current_keys, current_vertex_sharpness))
        source_count = current_segments[axis]
        resized = []
        resized_vertex_sharpness = []
        for key in target_keys:
            source_position = key[axis] * source_count / target_count
            lower = int(math.floor(source_position))
            upper = int(math.ceil(source_position))
            fraction = source_position - lower
            lower_key = list(key)
            upper_key = list(key)
            lower_key[axis] = lower
            upper_key[axis] = upper
            lower_point = current_by_key[tuple(lower_key)]
            upper_point = current_by_key[tuple(upper_key)]
            resized.append(
                tuple(
                    lower_point[component] * (1.0 - fraction) + upper_point[component] * fraction
                    for component in range(3)
                )
            )
            resized_vertex_sharpness.append(
                sharpness_by_key[tuple(lower_key)] * (1.0 - fraction)
                + sharpness_by_key[tuple(upper_key)] * fraction
            )

        source_edge_values = {}
        for edge, value in current_edge_sharpness.items():
            if edge[0] < len(current_keys) and edge[1] < len(current_keys):
                source_edge_values[
                    tuple(sorted((current_keys[edge[0]], current_keys[edge[1]])))
                ] = value
        target_index = {key: index for index, key in enumerate(target_keys)}
        resized_edge_sharpness = {}
        for start, end in cage_edges(target_faces):
            start_key = target_keys[start]
            end_key = target_keys[end]
            varying_axis = next(
                dimension
                for dimension in range(expected_axes)
                if start_key[dimension] != end_key[dimension]
            )
            if varying_axis == axis:
                midpoint = (start_key[axis] + end_key[axis]) * 0.5 * source_count / target_count
                lower = min(int(math.floor(midpoint)), source_count - 1)
                first = list(start_key)
                second = list(end_key)
                first[axis] = lower
                second[axis] = lower + 1
                value = source_edge_values.get(tuple(sorted((tuple(first), tuple(second)))), 0.0)
            else:
                source_position = start_key[axis] * source_count / target_count
                lower = int(math.floor(source_position))
                upper = int(math.ceil(source_position))
                fraction = source_position - lower

                def edge_value(layer):
                    first = list(start_key)
                    second = list(end_key)
                    first[axis] = layer
                    second[axis] = layer
                    return source_edge_values.get(tuple(sorted((tuple(first), tuple(second)))), 0.0)

                value = edge_value(lower) * (1.0 - fraction) + edge_value(upper) * fraction
            if value > 0.0:
                resized_edge_sharpness[
                    tuple(sorted((target_index[start_key], target_index[end_key])))
                ] = value
        current_vertices = resized
        current_vertex_sharpness = resized_vertex_sharpness
        current_edge_sharpness = resized_edge_sharpness
        current_segments = target_segments
        current_keys = target_keys
        current_faces = target_faces

    final_keys, final_faces = topology(new_segments)
    if current_keys != final_keys:
        raise RuntimeError("Structured cage resizing produced inconsistent topology")
    if return_sharpness:
        return (
            current_vertices,
            final_faces,
            current_vertex_sharpness,
            current_edge_sharpness,
        )
    return current_vertices, final_faces


def cylinder_control_cage(radius, height, side_segments=2, height_segments=2):
    """Return a closed all-quad cylinder cage projected from a segmented box."""
    radius = float(radius)
    height = float(height)
    side_segments = int(side_segments)
    height_segments = int(height_segments)
    if radius <= 0 or height <= 0:
        raise ValueError("Cylinder dimensions must be positive")
    if side_segments < 1 or height_segments < 1:
        raise ValueError("Cylinder segment counts must be at least one")
    vertices, faces = box_control_cage(
        2.0 * radius,
        2.0 * radius,
        height,
        2 * side_segments,
        2 * side_segments,
        height_segments,
    )
    projected = []
    for x_value, y_value, z_value in vertices:
        if math.isclose(max(abs(x_value), abs(y_value)), radius):
            length = math.hypot(x_value, y_value)
            x_value *= radius / length
            y_value *= radius / length
        projected.append((x_value, y_value, z_value))
    return projected, faces


def quadball_control_cage(radius, segments=2):
    """Return a closed all-quad cube-sphere (quadball) control cage."""
    radius = float(radius)
    segments = int(segments)
    if radius <= 0:
        raise ValueError("Sphere radius must be positive")
    if segments < 1:
        raise ValueError("Sphere segment count must be at least one")
    vertices, faces = box_control_cage(2.0, 2.0, 2.0, segments, segments, segments)
    projected = []
    for x_value, y_value, z_value in vertices:
        x_squared = x_value * x_value
        y_squared = y_value * y_value
        z_squared = z_value * z_value
        projected.append(
            (
                radius
                * x_value
                * math.sqrt(
                    max(
                        0.0,
                        1.0 - y_squared / 2.0 - z_squared / 2.0 + y_squared * z_squared / 3.0,
                    )
                ),
                radius
                * y_value
                * math.sqrt(
                    max(
                        0.0,
                        1.0 - z_squared / 2.0 - x_squared / 2.0 + z_squared * x_squared / 3.0,
                    )
                ),
                radius
                * z_value
                * math.sqrt(
                    max(
                        0.0,
                        1.0 - x_squared / 2.0 - y_squared / 2.0 + x_squared * y_squared / 3.0,
                    )
                ),
            )
        )
    return projected, faces


def sphere_control_cage(radius, longitude_segments=2, latitude_segments=2):
    """Return a closed all-quad UV-sphere cage with north and south poles.

    Each polar cap uses quads spanning two longitude cells, allowing the cage
    to terminate at one real pole vertex without degenerate faces.  The rings
    between the caps retain the usual latitude/longitude layout.
    """
    radius = float(radius)
    longitude_segments = int(longitude_segments)
    latitude_segments = int(latitude_segments)
    if radius <= 0.0:
        raise ValueError("Sphere radius must be positive")
    if longitude_segments < 1 or latitude_segments < 1:
        raise ValueError("Sphere segment counts must be at least one")

    longitude_count = 8 * longitude_segments
    ring_count = 4 * latitude_segments - 1
    vertices = [(0.0, 0.0, radius)]

    for ring_index in range(1, ring_count + 1):
        polar_angle = math.pi * ring_index / (ring_count + 1)
        ring_radius = radius * math.sin(polar_angle)
        z_value = radius * math.cos(polar_angle)
        for longitude_index in range(longitude_count):
            longitude_angle = 2.0 * math.pi * longitude_index / longitude_count
            vertices.append(
                (
                    ring_radius * math.cos(longitude_angle),
                    ring_radius * math.sin(longitude_angle),
                    z_value,
                )
            )

    south_pole = len(vertices)
    vertices.append((0.0, 0.0, -radius))

    def ring_vertex(ring_index, longitude_index):
        return 1 + ring_index * longitude_count + longitude_index % longitude_count

    faces = []
    north_pole = 0
    for longitude_index in range(0, longitude_count, 2):
        faces.append(
            (
                north_pole,
                ring_vertex(0, longitude_index),
                ring_vertex(0, longitude_index + 1),
                ring_vertex(0, longitude_index + 2),
            )
        )

    for ring_index in range(ring_count - 1):
        for longitude_index in range(longitude_count):
            faces.append(
                (
                    ring_vertex(ring_index, longitude_index),
                    ring_vertex(ring_index + 1, longitude_index),
                    ring_vertex(ring_index + 1, longitude_index + 1),
                    ring_vertex(ring_index, longitude_index + 1),
                )
            )

    last_ring = ring_count - 1
    for longitude_index in range(0, longitude_count, 2):
        faces.append(
            (
                south_pole,
                ring_vertex(last_ring, longitude_index + 2),
                ring_vertex(last_ring, longitude_index + 1),
                ring_vertex(last_ring, longitude_index),
            )
        )
    return vertices, faces


def torus_control_cage(major_radius, minor_radius, major_segments=2, minor_segments=2):
    """Return a periodic all-quad torus cage around the Z axis."""
    major_radius = float(major_radius)
    minor_radius = float(minor_radius)
    major_count = 4 * int(major_segments)
    minor_count = 4 * int(minor_segments)
    if major_radius <= 0.0 or minor_radius <= 0.0 or minor_radius >= major_radius:
        raise ValueError("Torus radii require 0 < minor radius < major radius")
    if major_count < 4 or minor_count < 4:
        raise ValueError("Torus segment counts must be at least one per quadrant")
    vertices = []
    for major_index in range(major_count):
        major_angle = 2.0 * math.pi * major_index / major_count
        for minor_index in range(minor_count):
            minor_angle = 2.0 * math.pi * minor_index / minor_count
            radial = major_radius + minor_radius * math.cos(minor_angle)
            vertices.append(
                (
                    radial * math.cos(major_angle),
                    radial * math.sin(major_angle),
                    minor_radius * math.sin(minor_angle),
                )
            )

    def index(major_index, minor_index):
        return (major_index % major_count) * minor_count + minor_index % minor_count

    faces = [
        (
            index(major_index, minor_index),
            index(major_index + 1, minor_index),
            index(major_index + 1, minor_index + 1),
            index(major_index, minor_index + 1),
        )
        for major_index in range(major_count)
        for minor_index in range(minor_count)
    ]
    return vertices, faces


def tube_control_cage(
    outer_radius,
    inner_radius,
    height,
    side_segments=2,
    height_segments=2,
):
    """Return a closed hollow cylindrical all-quad cage around the Z axis."""
    outer_radius = float(outer_radius)
    inner_radius = float(inner_radius)
    height = float(height)
    side_count = 4 * int(side_segments)
    height_segments = int(height_segments)
    if outer_radius <= 0.0 or inner_radius <= 0.0 or inner_radius >= outer_radius:
        raise ValueError("Tube radii require 0 < inner radius < outer radius")
    if height <= 0.0:
        raise ValueError("Tube height must be positive")
    if side_count < 4 or height_segments < 1:
        raise ValueError("Tube segment counts must be positive")
    vertices = []
    for radius in (outer_radius, inner_radius):
        for layer in range(height_segments + 1):
            z_value = height * layer / height_segments - height / 2.0
            for side in range(side_count):
                angle = 2.0 * math.pi * side / side_count
                vertices.append((radius * math.cos(angle), radius * math.sin(angle), z_value))

    layer_size = side_count
    wall_size = (height_segments + 1) * layer_size

    def outer(layer, side):
        return layer * layer_size + side % side_count

    def inner(layer, side):
        return wall_size + layer * layer_size + side % side_count

    faces = []
    for layer in range(height_segments):
        for side in range(side_count):
            next_side = side + 1
            faces.append(
                (
                    outer(layer, side),
                    outer(layer, next_side),
                    outer(layer + 1, next_side),
                    outer(layer + 1, side),
                )
            )
            faces.append(
                (
                    inner(layer, side),
                    inner(layer + 1, side),
                    inner(layer + 1, next_side),
                    inner(layer, next_side),
                )
            )
    for side in range(side_count):
        next_side = side + 1
        faces.append(
            (
                outer(height_segments, side),
                outer(height_segments, next_side),
                inner(height_segments, next_side),
                inner(height_segments, side),
            )
        )
        faces.append(
            (
                outer(0, next_side),
                outer(0, side),
                inner(0, side),
                inner(0, next_side),
            )
        )
    return vertices, faces


def cage_edges(faces):
    """Return the unique undirected edges in a face sequence."""
    edges = set()
    for face in faces:
        for index, start in enumerate(face):
            end = face[(index + 1) % len(face)]
            edges.add(tuple(sorted((start, end))))
    return sorted(edges)


def cage_edge_loop(faces, start_edge):
    """Return the continuous quad edge loop containing *start_edge*.

    At each vertex, the walk continues along the unique incident edge that
    does not share a face with the incoming edge. This is the topological
    opposite edge at a regular quad vertex. The walk closes on loop topology
    and stops where continuation is absent or ambiguous.
    """
    start_edge = tuple(sorted((int(start_edge[0]), int(start_edge[1]))))
    edge_faces = {}
    vertex_edges = {}
    for face_index, face in enumerate(faces):
        face = tuple(int(index) for index in face)
        if len(face) != 4:
            raise ValueError("Edge-loop selection requires quad faces")
        for position, start in enumerate(face):
            edge = tuple(sorted((start, face[(position + 1) % 4])))
            edge_faces.setdefault(edge, set()).add(face_index)
            vertex_edges.setdefault(edge[0], set()).add(edge)
            vertex_edges.setdefault(edge[1], set()).add(edge)
    if start_edge not in edge_faces:
        return []

    result = {start_edge}

    def walk(vertex, incoming):
        while True:
            candidates = [
                edge
                for edge in vertex_edges.get(vertex, ())
                if edge != incoming and edge_faces[edge].isdisjoint(edge_faces[incoming])
            ]
            if len(candidates) != 1:
                return
            outgoing = candidates[0]
            if outgoing in result:
                return
            result.add(outgoing)
            vertex = outgoing[0] if outgoing[1] == vertex else outgoing[1]
            incoming = outgoing

    walk(start_edge[0], start_edge)
    walk(start_edge[1], start_edge)
    return sorted(result)


def connected_edge_component(edges, start_edge):
    """Return the edge-connected component containing *start_edge*."""
    edges = {tuple(sorted((int(edge[0]), int(edge[1])))) for edge in edges}
    start = tuple(sorted((int(start_edge[0]), int(start_edge[1]))))
    if start not in edges:
        return []
    by_vertex = {}
    for edge in edges:
        by_vertex.setdefault(edge[0], set()).add(edge)
        by_vertex.setdefault(edge[1], set()).add(edge)
    component = set()
    pending = [start]
    while pending:
        edge = pending.pop()
        if edge in component:
            continue
        component.add(edge)
        for vertex in edge:
            pending.extend(by_vertex[vertex].difference(component))
    return sorted(component)


def catmull_clark_step(vertices, faces, edge_sharpness=None, vertex_sharpness=None):
    """Apply one Catmull-Clark refinement step to a manifold polygon cage.

    Closed and boundary cages are supported. Non-manifold edges are rejected so
    preview failures remain explicit instead of producing corrupt geometry.
    """
    result = _catmull_clark_step_details(vertices, faces, edge_sharpness, vertex_sharpness)
    return result[0], result[1]


def catmull_clark_step_details(vertices, faces, edge_sharpness=None, vertex_sharpness=None):
    """Return one refinement step and its stable topology maps.

    The public wrapper is used by hierarchical editing features which retain
    selected vertices of a refinement level as editable controls.
    """
    return _catmull_clark_step_details(vertices, faces, edge_sharpness, vertex_sharpness)


def _catmull_clark_step_details(vertices, faces, edge_sharpness=None, vertex_sharpness=None):
    """Return one refinement step plus old-to-new topology maps."""
    vertices = [tuple(float(component) for component in point) for point in vertices]
    faces = [tuple(int(index) for index in face) for face in faces]
    _validate_topology(vertices, faces)
    edge_sharpness = {
        tuple(sorted((int(edge[0]), int(edge[1])))): max(0.0, float(value))
        for edge, value in (edge_sharpness or {}).items()
    }
    vertex_sharpness = list(vertex_sharpness or ())
    vertex_sharpness.extend([0.0] * (len(vertices) - len(vertex_sharpness)))

    face_points = [_average([vertices[index] for index in face]) for face in faces]
    edge_faces = {}
    vertex_faces = [set() for _point in vertices]
    vertex_edges = [set() for _point in vertices]

    for face_index, face in enumerate(faces):
        for index, start in enumerate(face):
            end = face[(index + 1) % len(face)]
            edge = tuple(sorted((start, end)))
            edge_faces.setdefault(edge, []).append(face_index)
            vertex_faces[start].add(face_index)
            vertex_edges[start].add(edge)
            vertex_edges[end].add(edge)

    if any(len(adjacent) > 2 for adjacent in edge_faces.values()):
        raise ValueError("Catmull-Clark subdivision requires manifold edges")

    new_vertices = []
    original_indices = {}
    for vertex_index, point in enumerate(vertices):
        incident_edges = vertex_edges[vertex_index]
        boundary_neighbors = []
        for edge in incident_edges:
            if len(edge_faces[edge]) == 1:
                boundary_neighbors.append(edge[0] if edge[1] == vertex_index else edge[1])

        if boundary_neighbors:
            if len(boundary_neighbors) != 2:
                raise ValueError("A boundary vertex must have exactly two boundary neighbors")
            neighbors = [vertices[index] for index in boundary_neighbors]
            updated = tuple(
                0.75 * point[axis] + 0.125 * neighbors[0][axis] + 0.125 * neighbors[1][axis]
                for axis in range(3)
            )
        else:
            valence = len(incident_edges)
            if valence == 0:
                updated = point
            else:
                average_faces = _average(
                    [face_points[index] for index in vertex_faces[vertex_index]]
                )
                edge_midpoints = [
                    _average([vertices[edge[0]], vertices[edge[1]]]) for edge in incident_edges
                ]
                average_edges = _average(edge_midpoints)
                updated = tuple(
                    (
                        average_faces[axis]
                        + 2.0 * average_edges[axis]
                        + (valence - 3.0) * point[axis]
                    )
                    / valence
                    for axis in range(3)
                )
        sharp_edges = sorted(
            (
                (edge_sharpness.get(edge, 0.0), edge)
                for edge in incident_edges
                if edge_sharpness.get(edge, 0.0) > 0.0
            ),
            reverse=True,
        )
        sharp_weight = min(max(0.0, float(vertex_sharpness[vertex_index])), 1.0)
        sharp_target = point
        if len(sharp_edges) >= 3:
            sharp_weight = max(sharp_weight, min(sharp_edges[2][0], 1.0))
        elif len(sharp_edges) >= 2:
            neighbors = [
                edge[0] if edge[1] == vertex_index else edge[1] for _value, edge in sharp_edges[:2]
            ]
            sharp_target = tuple(
                0.75 * point[axis]
                + 0.125 * vertices[neighbors[0]][axis]
                + 0.125 * vertices[neighbors[1]][axis]
                for axis in range(3)
            )
            sharp_weight = max(sharp_weight, min(sharp_edges[1][0], 1.0))
        if sharp_weight:
            updated = tuple(
                updated[axis] * (1.0 - sharp_weight) + sharp_target[axis] * sharp_weight
                for axis in range(3)
            )
        original_indices[vertex_index] = len(new_vertices)
        new_vertices.append(updated)

    edge_indices = {}
    for edge, adjacent_faces in sorted(edge_faces.items()):
        midpoint = _average([vertices[edge[0]], vertices[edge[1]]])
        if len(adjacent_faces) == 1:
            point = midpoint
        else:
            smooth_point = _average(
                [
                    vertices[edge[0]],
                    vertices[edge[1]],
                    face_points[adjacent_faces[0]],
                    face_points[adjacent_faces[1]],
                ]
            )
            weight = min(edge_sharpness.get(edge, 0.0), 1.0)
            point = tuple(
                smooth_point[axis] * (1.0 - weight) + midpoint[axis] * weight for axis in range(3)
            )
        edge_indices[edge] = len(new_vertices)
        new_vertices.append(point)

    face_indices = {}
    for face_index, point in enumerate(face_points):
        face_indices[face_index] = len(new_vertices)
        new_vertices.append(point)

    new_faces = []
    for face_index, face in enumerate(faces):
        for index, vertex_index in enumerate(face):
            previous_index = face[index - 1]
            next_index = face[(index + 1) % len(face)]
            previous_edge = tuple(sorted((previous_index, vertex_index)))
            next_edge = tuple(sorted((vertex_index, next_index)))
            new_faces.append(
                (
                    original_indices[vertex_index],
                    edge_indices[next_edge],
                    face_indices[face_index],
                    edge_indices[previous_edge],
                )
            )

    new_edge_sharpness = {}
    for edge, value in edge_sharpness.items():
        child_value = max(value - 1.0, 0.0)
        if not child_value or edge not in edge_indices:
            continue
        midpoint = edge_indices[edge]
        new_edge_sharpness[tuple(sorted((original_indices[edge[0]], midpoint)))] = child_value
        new_edge_sharpness[tuple(sorted((midpoint, original_indices[edge[1]])))] = child_value
    new_vertex_sharpness = [0.0] * len(new_vertices)
    for old_index, new_index in original_indices.items():
        new_vertex_sharpness[new_index] = max(vertex_sharpness[old_index] - 1.0, 0.0)

    return (
        new_vertices,
        new_faces,
        original_indices,
        edge_indices,
        face_indices,
        new_edge_sharpness,
        new_vertex_sharpness,
    )


def catmull_clark(vertices, faces, levels=1):
    """Return a cage refined by *levels* Catmull-Clark steps."""
    levels = int(levels)
    if levels < 0:
        raise ValueError("Subdivision level cannot be negative")
    result_vertices = list(vertices)
    result_faces = list(faces)
    _validate_topology(result_vertices, result_faces)
    for _level in range(levels):
        result_vertices, result_faces = catmull_clark_step(result_vertices, result_faces)
    return result_vertices, result_faces


def catmull_clark_limit_points(vertices, faces, edge_sharpness=None, vertex_sharpness=None):
    """Return the Catmull-Clark limit position of every cage vertex.

    The closed-cage rule supports arbitrary vertex valence but currently
    requires quad faces. Boundary vertices use the cubic B-spline limit rule.
    """
    vertices = [tuple(float(component) for component in point) for point in vertices]
    faces = [tuple(int(index) for index in face) for face in faces]
    _validate_topology(vertices, faces)
    edge_sharpness = {
        tuple(sorted(edge)): max(0.0, float(value))
        for edge, value in (edge_sharpness or {}).items()
    }
    vertex_sharpness = list(vertex_sharpness or ())
    vertex_sharpness.extend([0.0] * (len(vertices) - len(vertex_sharpness)))
    if any(len(face) != 4 for face in faces):
        (
            refined_vertices,
            refined_faces,
            old_map,
            _edge_map,
            _face_map,
            refined_edges,
            refined_corners,
        ) = catmull_clark_step_details(
            vertices,
            faces,
            edge_sharpness,
            vertex_sharpness,
        )
        refined_limits = catmull_clark_limit_points(
            refined_vertices,
            refined_faces,
            refined_edges,
            refined_corners,
        )
        return [refined_limits[old_map[index]] for index in range(len(vertices))]

    edge_faces = {}
    vertex_faces = [[] for _point in vertices]
    vertex_neighbors = [set() for _point in vertices]
    opposite_vertices = [[] for _point in vertices]
    for face_index, face in enumerate(faces):
        for position, vertex_index in enumerate(face):
            previous_index = face[position - 1]
            next_index = face[(position + 1) % 4]
            opposite_index = face[(position + 2) % 4]
            vertex_faces[vertex_index].append(face_index)
            vertex_neighbors[vertex_index].update((previous_index, next_index))
            opposite_vertices[vertex_index].append(opposite_index)
            edge = tuple(sorted((vertex_index, next_index)))
            edge_faces.setdefault(edge, []).append(face_index)

    if any(len(adjacent) > 2 for adjacent in edge_faces.values()):
        raise ValueError("Catmull-Clark subdivision requires manifold edges")

    result = []
    for vertex_index, point in enumerate(vertices):
        boundary_neighbors = []
        for neighbor in vertex_neighbors[vertex_index]:
            edge = tuple(sorted((vertex_index, neighbor)))
            if len(edge_faces[edge]) == 1:
                boundary_neighbors.append(neighbor)

        if boundary_neighbors:
            if len(boundary_neighbors) != 2:
                raise ValueError("A boundary vertex must have exactly two boundary neighbors")
            neighbors = [vertices[index] for index in boundary_neighbors]
            limit = tuple(
                (2.0 / 3.0) * point[axis] + (neighbors[0][axis] + neighbors[1][axis]) / 6.0
                for axis in range(3)
            )
        else:
            valence = len(vertex_faces[vertex_index])
            if valence < 3 or len(vertex_neighbors[vertex_index]) != valence:
                raise ValueError("A closed cage vertex requires a manifold face fan")
            denominator = float(valence * (valence + 5))
            limit = tuple(
                (
                    valence * valence * point[axis]
                    + 4.0 * sum(vertices[index][axis] for index in vertex_neighbors[vertex_index])
                    + sum(vertices[index][axis] for index in opposite_vertices[vertex_index])
                )
                / denominator
                for axis in range(3)
            )
        sharp_neighbors = sorted(
            (
                (edge_sharpness.get(tuple(sorted((vertex_index, neighbor))), 0.0), neighbor)
                for neighbor in vertex_neighbors[vertex_index]
                if edge_sharpness.get(tuple(sorted((vertex_index, neighbor))), 0.0) > 0.0
            ),
            reverse=True,
        )
        weight = min(max(0.0, vertex_sharpness[vertex_index]), 1.0)
        sharp_limit = point
        if len(sharp_neighbors) >= 3:
            weight = max(weight, min(sharp_neighbors[2][0], 1.0))
        elif len(sharp_neighbors) >= 2:
            sharp_limit = tuple(
                (
                    4.0 * point[axis]
                    + vertices[sharp_neighbors[0][1]][axis]
                    + vertices[sharp_neighbors[1][1]][axis]
                )
                / 6.0
                for axis in range(3)
            )
            weight = max(weight, min(sharp_neighbors[1][0], 1.0))
        if weight:
            limit = tuple(
                limit[axis] * (1.0 - weight) + sharp_limit[axis] * weight for axis in range(3)
            )
        result.append(limit)
    return result


def catmull_clark_patch_grids(vertices, faces, level, edge_sharpness=None, vertex_sharpness=None):
    """Sample each original quad at dyadic Catmull-Clark limit positions.

    The returned list contains one ``(2**level + 1)`` square point grid for
    each input face. Grid orientation follows the input face winding.
    """
    level = int(level)
    if level < 1:
        raise ValueError("Patch sampling level must be at least one")
    vertices = [tuple(float(component) for component in point) for point in vertices]
    faces = [tuple(int(index) for index in face) for face in faces]
    _validate_topology(vertices, faces)
    if any(len(face) != 4 for face in faces):
        raise ValueError("Patch sampling currently requires quad faces")

    # Each cell retains the UV coordinates corresponding to its face tuple.
    cells = []
    patch_vertices = []
    for patch_index, face in enumerate(faces):
        uv = ((0, 0), (1, 0), (1, 1), (0, 1))
        cells.append((patch_index, patch_index, uv))
        patch_vertices.append({uv[index]: vertex for index, vertex in enumerate(face)})

    current_vertices = vertices
    current_faces = faces
    current_edge_sharpness = edge_sharpness or {}
    current_vertex_sharpness = vertex_sharpness or []
    scale = 1
    for _iteration in range(level):
        (
            next_vertices,
            next_faces,
            old_map,
            edge_map,
            face_map,
            next_edge_sharpness,
            next_vertex_sharpness,
        ) = _catmull_clark_step_details(
            current_vertices,
            current_faces,
            current_edge_sharpness,
            current_vertex_sharpness,
        )
        next_cells = []
        child_start = []
        offset = 0
        for face in current_faces:
            child_start.append(offset)
            offset += len(face)

        patch_vertices = [
            {(u * 2, v * 2): old_map[index] for (u, v), index in coordinate_map.items()}
            for coordinate_map in patch_vertices
        ]

        for patch_index, face_index, coordinates in cells:
            face = current_faces[face_index]
            doubled = tuple((u * 2, v * 2) for u, v in coordinates)
            center = (
                sum(coordinate[0] for coordinate in doubled) // 4,
                sum(coordinate[1] for coordinate in doubled) // 4,
            )
            for position, vertex_index in enumerate(face):
                next_position = (position + 1) % 4
                previous_position = position - 1
                next_edge = tuple(sorted((vertex_index, face[next_position])))
                previous_edge = tuple(sorted((face[previous_position], vertex_index)))
                corner = doubled[position]
                next_mid = (
                    (doubled[position][0] + doubled[next_position][0]) // 2,
                    (doubled[position][1] + doubled[next_position][1]) // 2,
                )
                previous_mid = (
                    (doubled[previous_position][0] + doubled[position][0]) // 2,
                    (doubled[previous_position][1] + doubled[position][1]) // 2,
                )
                child_coordinates = (corner, next_mid, center, previous_mid)
                child_index = child_start[face_index] + position
                child_face = next_faces[child_index]
                for coordinate, child_vertex in zip(child_coordinates, child_face):
                    patch_vertices[patch_index][coordinate] = child_vertex
                next_cells.append((patch_index, child_index, child_coordinates))

        current_vertices = next_vertices
        current_faces = next_faces
        current_edge_sharpness = next_edge_sharpness
        current_vertex_sharpness = next_vertex_sharpness
        cells = next_cells
        scale *= 2

    limit_points = catmull_clark_limit_points(
        current_vertices,
        current_faces,
        current_edge_sharpness,
        current_vertex_sharpness,
    )
    grids = []
    for coordinate_map in patch_vertices:
        grids.append(
            [
                [limit_points[coordinate_map[(u, v)]] for v in range(scale + 1)]
                for u in range(scale + 1)
            ]
        )
    return grids
