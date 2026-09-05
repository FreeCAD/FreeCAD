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

"""Path-driven Forms pipe primitive and wire-network segmentation."""

import json
import math

from .feature import reset_cage

import FreeCAD as App
import Part

from .feature import FormFeatureProxy
from .viewprovider import ViewProviderForm as ViewProviderFormBox
from .brep import ConversionError, cage_to_solid
from .cage import ControlCage
from .elementmap import map_form_shape
from .topology import box_control_cage, cylinder_control_cage


def _point_key(point, tolerance=1.0e-7):
    return tuple(int(round(component / tolerance)) for component in (point.x, point.y, point.z))


def _edge_endpoints(edge):
    return (
        App.Vector(edge.valueAt(edge.FirstParameter)),
        App.Vector(edge.valueAt(edge.LastParameter)),
    )


def _split_path_edges(shape, tolerance=1.0e-7):
    """Split edges where another path edge terminates on their interior."""
    edges = list(getattr(shape, "Edges", ()))
    result = []
    endpoints = [point for edge in edges for point in _edge_endpoints(edge)]
    for original_index, edge in enumerate(edges):
        parameters = [float(edge.FirstParameter), float(edge.LastParameter)]
        low, high = sorted(parameters)
        for point in endpoints:
            if Part.Vertex(point).distToShape(edge)[0] > tolerance:
                continue
            try:
                parameter = float(edge.Curve.parameter(point))
            except (Part.OCCError, RuntimeError, ValueError):
                continue
            if low + tolerance < parameter < high - tolerance:
                parameters.append(parameter)
        parameters = sorted(set(round(value, 12) for value in parameters))
        for fragment_index, (first, last) in enumerate(
            zip(parameters, parameters[1:]), 1
        ):
            fragment = edge.Curve.toShape(first, last)
            result.append((fragment, original_index, fragment_index))
    return result


def path_segments(shape, include_edges=False):
    """Return maximal ordered edge chains between endpoints and T-junctions."""
    edge_records = _split_path_edges(shape)
    if not edge_records:
        raise ValueError("The pipe path contains no edges")
    edges = [record[0] for record in edge_records]
    records = []
    adjacency = {}
    for index, edge in enumerate(edges):
        first, last = _edge_endpoints(edge)
        start = _point_key(first)
        end = _point_key(last)
        records.append((start, end))
        adjacency.setdefault(start, []).append(index)
        adjacency.setdefault(end, []).append(index)

    unused = set(range(len(edges)))
    result = []

    def walk(start_node, first_edge):
        node = start_node
        edge_index = first_edge
        chain = []
        while edge_index in unused:
            unused.remove(edge_index)
            start, end = records[edge_index]
            forward = start == node
            next_node = end if forward else start
            chain.append((edge_index, forward))
            candidates = [candidate for candidate in adjacency[next_node] if candidate in unused]
            if len(adjacency[next_node]) != 2 or not candidates:
                break
            node = next_node
            edge_index = candidates[0]
        return chain

    for node in sorted(adjacency):
        if len(adjacency[node]) == 2:
            continue
        for edge_index in tuple(adjacency[node]):
            if edge_index in unused:
                result.append(walk(node, edge_index))
    while unused:
        edge_index = min(unused)
        result.append(walk(records[edge_index][0], edge_index))
    if include_edges:
        return result, adjacency, edge_records
    return result, adjacency


def segment_key(segment, edge_records=None):
    if edge_records is None:
        return ",".join(str(index + 1) for index, _forward in segment)
    return ",".join(
        f"{edge_records[index][1] + 1}.{edge_records[index][2]}"
        for index, _forward in segment
    )


def decode_segment_overrides(values):
    result = {}
    for value in values or ():
        try:
            record = json.loads(str(value))
            key = str(record["segment"])
            diameter = float(record["diameter"])
            if diameter > 0.0 and math.isfinite(diameter):
                result[key] = diameter
        except (KeyError, TypeError, ValueError, json.JSONDecodeError):
            continue
    return result


def encode_segment_overrides(values):
    return [
        json.dumps({"segment": key, "diameter": float(diameter)}, sort_keys=True)
        for key, diameter in sorted(values.items())
        if float(diameter) > 0.0 and math.isfinite(float(diameter))
    ]


def set_segment_diameter(obj, key, diameter):
    """Set one path-segment override; zero removes the override."""
    overrides = decode_segment_overrides(obj.SegmentDiameters)
    diameter = float(diameter)
    if diameter > 0.0:
        overrides[str(key)] = diameter
    else:
        overrides.pop(str(key), None)
    obj.SegmentDiameters = encode_segment_overrides(overrides)
    obj.touch()


def decode_segment_sample_overrides(values):
    result = {}
    for value in values or ():
        try:
            record = json.loads(str(value))
            key = str(record["segment"])
            samples = int(record["samples"])
            if samples > 0:
                result[key] = samples
        except (KeyError, TypeError, ValueError, json.JSONDecodeError):
            continue
    return result


def encode_segment_sample_overrides(values):
    return [
        json.dumps({"segment": key, "samples": int(samples)}, sort_keys=True)
        for key, samples in sorted(values.items())
        if int(samples) > 0
    ]


def set_segment_samples(obj, key, samples):
    """Set one segment's longitudinal intervals per source edge; zero uses global."""
    overrides = decode_segment_sample_overrides(obj.SegmentSamples)
    samples = int(samples)
    if samples > 0:
        overrides[str(key)] = samples
    else:
        overrides.pop(str(key), None)
    obj.SegmentSamples = encode_segment_sample_overrides(overrides)
    obj.touch()


def _sample_edge(edge, forward, interval_count):
    """Return arc-length-spaced controls, including both edge endpoints."""
    interval_count = max(1, int(interval_count))
    try:
        points = [
            App.Vector(edge.valueAt(edge.getParameterByLength(edge.Length * index / interval_count)))
            for index in range(interval_count + 1)
        ]
    except (Part.OCCError, RuntimeError, ValueError):
        points = [
            App.Vector(point)
            for point in edge.discretize(Number=interval_count + 1)
        ]
    first, _last = _edge_endpoints(edge)
    if points and points[0].sub(first).Length > points[-1].sub(first).Length:
        points.reverse()
    if not forward:
        points.reverse()
    return points


def _segment_points(edges, segment, sample_count):
    # Use PathSamples as the average interval count per source edge, then
    # distribute those intervals by arc length over the complete segment.
    # Equal counts per edge make the controls immediately before and after a
    # junction arbitrarily uneven when its source edges have different
    # lengths, which biases the Catmull-Clark tangent at that junction.
    interval_target = max(len(segment), int(sample_count) * len(segment))
    interval_counts = [1] * len(segment)
    lengths = [max(float(edges[index].Length), 1.0e-12) for index, _forward in segment]
    while sum(interval_counts) < interval_target:
        index = max(
            range(len(segment)),
            key=lambda item: lengths[item] / interval_counts[item],
        )
        interval_counts[index] += 1

    result = []
    for (edge_index, forward), interval_count in zip(segment, interval_counts):
        points = _sample_edge(edges[edge_index], forward, interval_count)
        if result and points and result[-1].sub(points[0]).Length <= 1.0e-7:
            if len(result) >= 2 and len(points) >= 2:
                previous = result[-2]
                corner = result[-1]
                following = points[1]
                incoming = _normalize(corner - previous)
                outgoing = _normalize(following - corner)
                if incoming.dot(outgoing) < 1.0 - 1.0e-9:
                    cut = min(corner.sub(previous).Length, following.sub(corner).Length) * 0.5
                    result[-1] = corner - incoming * cut
                    result.append(corner + outgoing * cut)
            points = points[1:]
        result.extend(points)
    if len(result) >= 4 and result[-1].sub(result[0]).Length <= 1.0e-7:
        previous = result[-2]
        corner = result[0]
        following = result[1]
        incoming = _normalize(corner - previous)
        outgoing = _normalize(following - corner)
        if incoming.dot(outgoing) < 1.0 - 1.0e-9:
            cut = min(corner.sub(previous).Length, following.sub(corner).Length) * 0.5
            result[0] = corner + outgoing * cut
            result[-1] = corner - incoming * cut
            result.append(App.Vector(result[0]))
    compact = []
    for point in result:
        if not compact or compact[-1].sub(point).Length > 1.0e-7:
            compact.append(point)
    if len(compact) < 2:
        raise ValueError("A pipe segment has no measurable length")
    return compact


def _normalize(vector):
    vector = App.Vector(vector)
    if vector.Length <= 1.0e-12:
        raise ValueError("The pipe path contains coincident samples")
    vector.normalize()
    return vector


def _frames(points, closed=False):
    tangents = []
    for index in range(len(points)):
        if closed:
            direction = points[(index + 1) % len(points)].sub(
                points[(index - 1) % len(points)]
            )
        elif index == 0:
            tangents.append(_normalize(points[1].sub(points[0])))
            continue
        elif index == len(points) - 1:
            tangents.append(_normalize(points[-1].sub(points[-2])))
            continue
        else:
            # Catmull-Clark's longitudinal limit tangent follows this central
            # chord. Keeping the section perpendicular to the same derivative
            # avoids a sheared ring where adjacent sampled edges have unequal
            # interval lengths.
            direction = points[index + 1].sub(points[index - 1])
        tangents.append(_normalize(direction))
    axes = (App.Vector(1, 0, 0), App.Vector(0, 1, 0), App.Vector(0, 0, 1))
    reference = min(axes, key=lambda axis: abs(tangents[0].dot(axis)))
    normal = _normalize(tangents[0].cross(reference))
    result = []
    previous_tangent = tangents[0]
    for tangent in tangents:
        axis = previous_tangent.cross(tangent)
        sine = axis.Length
        cosine = max(-1.0, min(1.0, previous_tangent.dot(tangent)))
        if sine > 1.0e-9:
            axis = axis / sine
            normal = (
                normal * cosine
                + axis.cross(normal) * sine
                + axis * (axis.dot(normal) * (1.0 - cosine))
            )
        projected = normal - tangent * normal.dot(tangent)
        if projected.Length <= 1.0e-9:
            reference = min(axes, key=lambda axis: abs(tangent.dot(axis)))
            projected = tangent.cross(reference)
        normal = _normalize(projected)
        binormal = _normalize(tangent.cross(normal))
        result.append((normal, binormal))
        previous_tangent = tangent
    return result


def swept_segment_cage(
    points,
    diameter,
    side_segments=2,
    open_start=False,
    open_end=False,
    include_boundaries=False,
):
    """Deform a closed all-quad cylinder cage along an ordered 3D polyline."""
    diameter = float(diameter)
    if diameter <= 0.0 or not math.isfinite(diameter):
        raise ValueError("Pipe diameter must be finite and positive")
    points = [App.Vector(point) for point in points]
    closed = points[0].sub(points[-1]).Length <= 1.0e-7
    if closed:
        points.pop()
        if len(points) < 3:
            raise ValueError("A closed pipe path needs at least three samples")
        frames = _frames(points, closed=True)
        # cylinder_control_cage() produces eight perimeter segments for each
        # section-density level.  Keep closed and open paths consistent.
        side_count = 8 * int(side_segments)
        radius = diameter * 0.5
        transformed = []
        for center, (normal, binormal) in zip(points, frames):
            for side_index in range(side_count):
                angle = 2.0 * math.pi * side_index / side_count
                point = (
                    center
                    + normal * (radius * math.cos(angle))
                    + binormal * (radius * math.sin(angle))
                )
                transformed.append((point.x, point.y, point.z))

        def vertex(layer, side):
            return (layer % len(points)) * side_count + side % side_count

        faces = [
            (
                vertex(layer, side),
                vertex(layer + 1, side),
                vertex(layer + 1, side + 1),
                vertex(layer, side + 1),
            )
            for layer in range(len(points))
            for side in range(side_count)
        ]
        if include_boundaries:
            return transformed, faces, None, None
        return transformed, faces

    frames = _frames(points)
    path_length = sum(
        first.sub(second).Length for first, second in zip(points, points[1:])
    )
    vertices, faces = cylinder_control_cage(
        diameter * 0.5,
        max(path_length, 1.0),
        int(side_segments),
        len(points) - 1,
    )
    z_values = sorted({round(point[2], 12) for point in vertices})
    layers = {value: index for index, value in enumerate(z_values)}
    minimum_z = min(point[2] for point in vertices)
    maximum_z = max(point[2] for point in vertices)
    retained_faces = []
    for face in faces:
        face_z = [vertices[index][2] for index in face]
        if open_start and all(abs(value - minimum_z) <= 1.0e-9 for value in face_z):
            continue
        if open_end and all(abs(value - maximum_z) <= 1.0e-9 for value in face_z):
            continue
        retained_faces.append(face)
    faces = retained_faces
    transformed = []
    for x_value, y_value, z_value in vertices:
        layer = layers[round(z_value, 12)]
        center = points[layer]
        normal, binormal = frames[layer]
        point = center + normal * x_value + binormal * y_value
        transformed.append((point.x, point.y, point.z))
    if not include_boundaries:
        return transformed, faces
    boundaries = _directed_boundary_loops(faces)
    start_loop = end_loop = None
    for loop in boundaries:
        average_layer = sum(layers[round(vertices[index][2], 12)] for index in loop) / len(loop)
        if average_layer <= 0.5:
            start_loop = loop
        elif average_layer >= len(points) - 1.5:
            end_loop = loop
    return transformed, faces, start_loop, end_loop


def _directed_boundary_loops(faces):
    """Return consistently directed boundary loops of an oriented face mesh."""
    directed = {}
    counts = {}
    for face in faces:
        for position, first in enumerate(face):
            second = face[(position + 1) % len(face)]
            edge = tuple(sorted((first, second)))
            counts[edge] = counts.get(edge, 0) + 1
            directed[edge] = (first, second)
    following = {
        directed[edge][0]: directed[edge][1]
        for edge, count in counts.items()
        if count == 1
    }
    loops = []
    while following:
        start = min(following)
        loop = [start]
        current = start
        while current in following:
            current = following.pop(current)
            if current == start:
                break
            loop.append(current)
        if current != start:
            raise ValueError("The pipe cage has an open boundary chain")
        loops.append(loop)
    return loops


def _boundary_guides(faces, loop, vertices):
    """Return the first interior control adjacent to each boundary control."""
    adjacency = {}
    for face in faces:
        for position, first in enumerate(face):
            second = face[(position + 1) % len(face)]
            adjacency.setdefault(first, set()).add(second)
            adjacency.setdefault(second, set()).add(first)
    boundary = set(loop)
    guides = {}
    for vertex in loop:
        candidates = adjacency.get(vertex, set()).difference(boundary)
        if not candidates:
            raise ValueError("A pipe rim has no adjacent interior control row")
        neighbor = min(
            candidates,
            key=lambda index: App.Vector(vertices[index]).sub(
                App.Vector(vertices[vertex])
            ).Length,
        )
        guides[vertex] = vertices[neighbor]
    return guides


def _junction_envelope_point(point, ports, density):
    """Project a control direction onto the union envelope of branch tubes.

    For a ray from the junction, the intersection with a cylindrical branch
    of radius ``r`` is ``r / sin(angle)``.  Taking the nearest such
    intersection follows the outer union of the incident tubes instead of
    introducing an isotropic ball around the node.  Catmull-Clark then
    supplies the blend across the piecewise-cylinder envelope.
    """
    direction = _normalize(App.Vector(*point))
    candidates = []
    for port in ports:
        branch_direction, diameter = port[0], port[2]
        cosine = max(0.0, min(1.0, direction.dot(branch_direction)))
        sine = math.sqrt(max(0.0, 1.0 - cosine * cosine))
        if sine > 1.0e-6:
            candidates.append(float(diameter) * 0.5 / sine)
    maximum_radius = max(float(port[2]) * 0.5 for port in ports)
    # The small compensation offsets subdivision shrinkage without changing
    # the directional envelope into a bulb.
    radius = max(candidates or [maximum_radius * 1.35])
    radius *= 1.0 + 0.08 / max(1, int(density))
    radius = min(radius, maximum_radius * 1.4)
    projected = direction * radius
    return projected.x, projected.y, projected.z


def _port_assignments(directions):
    """Assign branches to distinct cube faces with the least angular distortion."""
    axes = (
        App.Vector(1, 0, 0), App.Vector(-1, 0, 0),
        App.Vector(0, 1, 0), App.Vector(0, -1, 0),
        App.Vector(0, 0, 1), App.Vector(0, 0, -1),
    )
    if len(directions) > len(axes):
        raise ValueError("A pipe junction supports at most six branches")
    best = None

    def visit(index, used, selected, score):
        nonlocal best
        if index == len(directions):
            candidate = (score, tuple(selected))
            if best is None or candidate[0] > best[0]:
                best = candidate
            return
        for axis_index, axis in enumerate(axes):
            if axis_index not in used:
                visit(
                    index + 1,
                    used | {axis_index},
                    selected + [axis],
                    score + directions[index].dot(axis),
                )

    visit(0, set(), [], 0.0)
    return best[1]


def _junction_cage(center, ports, section_density):
    """Build one perforated quad junction sharing its rims with branch cages."""
    density = max(1, int(section_density))
    grid = 2 * density + 2
    raw_vertices, raw_faces = box_control_cage(2.0, 2.0, 2.0, grid, grid, grid)
    directions = [port[0] for port in ports]
    assigned_axes = _port_assignments(directions)

    def on_port(face, axis):
        component = max(range(3), key=lambda index: abs(axis[index]))
        side = 1.0 if axis[component] > 0.0 else -1.0
        if not all(abs(raw_vertices[index][component] - side) <= 1.0e-9 for index in face):
            return False
        other = [index for index in range(3) if index != component]
        limit = 1.0 - 2.0 / grid + 1.0e-9
        center_point = tuple(
            sum(raw_vertices[index][item] for index in face) / 4.0
            for item in range(3)
        )
        return all(abs(center_point[item]) <= limit for item in other)

    retained_faces = [
        face
        for face in raw_faces
        if not any(on_port(face, axis) for axis in assigned_axes)
    ]
    loops = _directed_boundary_loops(retained_faces)
    if len(loops) != len(ports):
        raise ValueError("Pipe junction ports overlap; reduce the section density")

    loop_by_axis = {}
    for loop in loops:
        average = App.Vector(
            *(sum(raw_vertices[index][item] for index in loop) / len(loop) for item in range(3))
        )
        axis = max(assigned_axes, key=lambda candidate: average.dot(candidate))
        loop_by_axis[tuple(axis)] = loop

    local_vertices = []
    for point in raw_vertices:
        projected = _junction_envelope_point(point, ports, density)
        local_vertices.append(
            tuple(center[item] + value for item, value in enumerate(projected))
        )
    boundary_maps = []
    for port, axis in zip(ports, assigned_axes):
        _direction, branch_loop, _diameter = port
        junction_loop = loop_by_axis[tuple(axis)]
        if len(junction_loop) != len(branch_loop):
            raise ValueError("Pipe branch and junction rims have incompatible topology")
        # Boundary directions must oppose one another when the two oriented
        # surfaces share an edge.  A cyclic shift selects the least-twisted
        # correspondence without introducing a transition solid or Boolean.
        reversed_branch = list(reversed(branch_loop))
        shift = min(
            range(len(branch_loop)),
            key=lambda offset: sum(
                App.Vector(local_vertices[junction_loop[index]]).sub(
                    App.Vector(reversed_branch[(offset + index) % len(branch_loop)][1])
                ).Length ** 2
                for index in range(len(branch_loop))
            ),
        )
        ordered_branch = reversed_branch[shift:] + reversed_branch[:shift]
        boundary_maps.append(dict(zip(junction_loop, (item[0] for item in ordered_branch))))
        for junction_vertex, branch_item in zip(junction_loop, ordered_branch):
            local_vertices[junction_vertex] = tuple(branch_item[1])
    return local_vertices, retained_faces, boundary_maps, {}


def _boundary_vertex_map(
    local_loop,
    global_loop,
    local_vertices,
    global_guides=None,
    radial_axis=None,
):
    """Match oppositely directed boundary loops with the least cyclic twist."""
    reversed_global = list(reversed(global_loop))
    target_points = dict(global_loop)
    if global_guides:
        target_points.update(global_guides)

    if radial_axis is not None:
        axis = _normalize(radial_axis)
        local_center = sum(
            (App.Vector(local_vertices[index]) for index in local_loop), App.Vector()
        ) / len(local_loop)
        global_center = sum(
            (App.Vector(point) for _index, point in global_loop), App.Vector()
        ) / len(global_loop)

        def radial(point, center):
            vector = App.Vector(point) - center
            vector = vector - axis * vector.dot(axis)
            return _normalize(vector)

        local_radials = {
            index: radial(local_vertices[index], local_center) for index in local_loop
        }
        global_radials = {
            index: radial(point, global_center) for index, point in global_loop
        }

        def mapping_cost(offset):
            return sum(
                1.0
                - local_radials[local_loop[index]].dot(
                    global_radials[
                        reversed_global[(offset + index) % len(global_loop)][0]
                    ]
                )
                for index in range(len(global_loop))
            )

    else:

        def mapping_cost(offset):
            return sum(
                App.Vector(local_vertices[local_loop[index]]).sub(
                    App.Vector(
                        target_points[
                            reversed_global[(offset + index) % len(global_loop)][0]
                        ]
                    )
                ).Length
                ** 2
                for index in range(len(global_loop))
            )

    shift = min(
        range(len(global_loop)),
        key=mapping_cost,
    )
    ordered = reversed_global[shift:] + reversed_global[:shift]
    return dict(zip(local_loop, (item[0] for item in ordered))), ordered


def _three_way_junction_cage(center, ports, section_density):
    """Build a three-port pair-of-pants cage without a rectangular side hole.

    Each circular rim is divided at the two sides of the junction.  Its two
    semicircular face bands continue independently toward the other two rims.
    The six band ends on either side are closed by three quads around one
    extraordinary control.  Consequently no circular face strip is forced to
    turn around a rectangular hole corner (the source of the visible pinch).
    """
    density = max(1, int(section_density))
    side_count = 8 * density
    half_count = side_count // 2
    junction_center = App.Vector(center)

    # The plane through the three outgoing directions supplies a common
    # "top" and "bottom" for splitting every circular rim.  Projecting it
    # into each rim plane also handles non-planar Y junctions.
    directions = [port[0] for port in ports]
    split_axis = (directions[1] - directions[0]).cross(
        directions[2] - directions[0]
    )
    if split_axis.Length <= 1.0e-8:
        split_axis = directions[0].cross(directions[1])
    if split_axis.Length <= 1.0e-8:
        split_axis = _frames([junction_center, junction_center + directions[0]])[0][0]
    split_axis = _normalize(split_axis)

    local_vertices = []
    rings = []
    boundary_maps = []
    global_for_local = []
    for port in ports:
        loop = list(port[1])
        if len(loop) != side_count:
            raise ValueError("Pipe rims require the same section density at a junction")
        ring_center = sum((App.Vector(point) for _index, point in loop), App.Vector()) / len(loop)
        radial_axis = split_axis - port[0] * split_axis.dot(port[0])
        if radial_axis.Length <= 1.0e-8:
            radial_axis = App.Vector(loop[0][1]) - ring_center
        radial_axis = _normalize(radial_axis)
        start = max(
            range(side_count),
            key=lambda index: (App.Vector(loop[index][1]) - ring_center).dot(radial_axis),
        )
        loop = loop[start:] + loop[:start]
        ring = []
        mapping = {}
        for global_index, point in loop:
            local_index = len(local_vertices)
            local_vertices.append(tuple(point))
            global_for_local.append(global_index)
            ring.append(local_index)
            mapping[local_index] = global_index
        rings.append(ring)
        boundary_maps.append(mapping)

    # Both possible semicircles run from the positive split control to the
    # negative one. Assign each half to the neighboring branch it faces.
    arcs = {}
    for port_index, port in enumerate(ports):
        ring = rings[port_index]
        forward = ring[: half_count + 1]
        backward = [ring[0]] + list(reversed(ring[half_count:]))
        others = [index for index in range(3) if index != port_index]

        def facing(arc, other):
            midpoint = App.Vector(local_vertices[arc[half_count // 2]])
            ring_center = sum(
                (App.Vector(local_vertices[index]) for index in ring), App.Vector()
            ) / side_count
            radial = midpoint - ring_center
            projected = directions[other] - port[0] * directions[other].dot(port[0])
            return radial.dot(projected)

        if facing(forward, others[0]) >= facing(backward, others[0]):
            arcs[port_index, others[0]] = forward
            arcs[port_index, others[1]] = backward
        else:
            arcs[port_index, others[0]] = backward
            arcs[port_index, others[1]] = forward

    local_faces = []
    band_middle = {}
    for first in range(3):
        for second in range(first + 1, 3):
            first_arc = arcs[first, second]
            second_arc = arcs[second, first]
            middle = []
            for position, (first_vertex, second_vertex) in enumerate(
                zip(first_arc, second_arc)
            ):
                first_point = App.Vector(local_vertices[first_vertex])
                second_point = App.Vector(local_vertices[second_vertex])
                first_global = global_for_local[first_vertex]
                second_global = global_for_local[second_vertex]
                first_tangent = first_point * 2.0 - App.Vector(
                    ports[first][3][first_global]
                )
                second_tangent = second_point * 2.0 - App.Vector(
                    ports[second][3][second_global]
                )
                point = (first_tangent + second_tangent) * 0.5
                local_index = len(local_vertices)
                local_vertices.append((point.x, point.y, point.z))
                middle.append(local_index)
                if position in (0, half_count):
                    band_middle[first, second, position] = local_index
            for position in range(half_count):
                local_faces.append(
                    (
                        first_arc[position],
                        first_arc[position + 1],
                        middle[position + 1],
                        middle[position],
                    )
                )
                local_faces.append(
                    (
                        middle[position],
                        middle[position + 1],
                        second_arc[position + 1],
                        second_arc[position],
                    )
                )

    # Close the two six-edge openings using three quads around a valence-three
    # extraordinary point. These are the only extraordinary controls.
    for end_position in (0, half_count):
        cap_boundary = [rings[index][0 if end_position == 0 else half_count] for index in range(3)]
        cap_boundary.extend(
            band_middle[first, second, end_position]
            for first in range(3)
            for second in range(first + 1, 3)
        )
        cap_point = sum(
            (App.Vector(local_vertices[index]) for index in cap_boundary), App.Vector()
        ) / len(cap_boundary)
        cap = len(local_vertices)
        local_vertices.append((cap_point.x, cap_point.y, cap_point.z))
        for port_index in range(3):
            others = [index for index in range(3) if index != port_index]
            first_pair = tuple(sorted((port_index, others[0])))
            second_pair = tuple(sorted((port_index, others[1])))
            local_faces.append(
                (
                    rings[port_index][0 if end_position == 0 else half_count],
                    band_middle[first_pair[0], first_pair[1], end_position],
                    cap,
                    band_middle[second_pair[0], second_pair[1], end_position],
                )
            )

    # Orient the new manifold consistently, then make every port boundary run
    # opposite to the already-oriented swept segment boundary.
    occurrences = {}
    for face_index, face in enumerate(local_faces):
        for position, first in enumerate(face):
            second = face[(position + 1) % 4]
            edge = tuple(sorted((first, second)))
            occurrences.setdefault(edge, []).append((face_index, first < second))
    flips = {0: False}
    pending = [0]
    while pending:
        face_index = pending.pop()
        face = local_faces[face_index]
        for position, first in enumerate(face):
            second = face[(position + 1) % 4]
            edge = tuple(sorted((first, second)))
            for neighbor, ascending in occurrences[edge]:
                if neighbor == face_index:
                    continue
                current_ascending = first < second
                required = flips[face_index] ^ (current_ascending == ascending)
                if neighbor in flips and flips[neighbor] != required:
                    raise ValueError("Pipe junction faces cannot be oriented consistently")
                if neighbor not in flips:
                    flips[neighbor] = required
                    pending.append(neighbor)
    local_faces = [
        tuple(reversed(face)) if flips.get(index, False) else face
        for index, face in enumerate(local_faces)
    ]
    first_edge = (rings[0][0], rings[0][1])
    same_boundary_direction = any(
        face[position] == first_edge[0]
        and face[(position + 1) % 4] == first_edge[1]
        for face in local_faces
        for position in range(4)
    )
    if same_boundary_direction:
        local_faces = [tuple(reversed(face)) for face in local_faces]
    return local_vertices, local_faces, boundary_maps, {}


def pipe_control_cage(
    source,
    diameter,
    side_segments=2,
    path_samples=3,
    overrides=None,
    debug=False,
    sample_overrides=None,
):
    """Return compact closed quad cages for path segments and branch junctions."""
    if source is None or not hasattr(source, "Shape"):
        raise ValueError("A pipe path object is required")
    shape = source.Shape
    if shape.isNull() or not shape.Edges:
        raise ValueError("The pipe path contains no edges")
    if shape.Faces:
        raise ValueError("The pipe path must contain wires, not faces")
    segments, adjacency, edge_records = path_segments(shape, include_edges=True)
    edges = [record[0] for record in edge_records]
    records = []
    node_points = {}
    for edge in edges:
        first, last = _edge_endpoints(edge)
        first_key = _point_key(first)
        last_key = _point_key(last)
        records.append((first_key, last_key))
        node_points.setdefault(first_key, first)
        node_points.setdefault(last_key, last)
    overrides = dict(overrides or {})
    sample_overrides = dict(sample_overrides or {})
    vertices = []
    faces = []
    descriptions = []
    keys = []
    junction_ports = {}
    if debug:
        App.Console.PrintMessage(
            f"[Forms Pipe] source_edges={len(shape.Edges)} fragments={len(edges)} "
            f"segments={len(segments)} adjacency="
            f"{sorted((node, len(connected)) for node, connected in adjacency.items())}\n"
        )
    for index, segment in enumerate(segments, 1):
        key = segment_key(segment, edge_records)
        selected_diameter = overrides.get(key, float(diameter))
        selected_samples = sample_overrides.get(key, int(path_samples))
        points = _segment_points(edges, segment, selected_samples)
        first_edge, first_forward = segment[0]
        last_edge, last_forward = segment[-1]
        first_node = records[first_edge][0 if first_forward else 1]
        last_node = records[last_edge][1 if last_forward else 0]
        open_start = len(adjacency[first_node]) >= 3
        open_end = len(adjacency[last_node]) >= 3
        start_direction = end_direction = None
        if open_start:
            start_direction = _normalize(points[1] - points[0])
            trim = min(selected_diameter, points[1].sub(points[0]).Length * 0.45)
            points[0] = points[0] + start_direction * trim
            remaining = points[1].sub(points[0]).Length
            if remaining > selected_diameter * 1.5:
                points.insert(
                    1,
                    points[0] + start_direction * min(selected_diameter, remaining * 0.5),
                )
        if open_end:
            end_direction = _normalize(points[-2] - points[-1])
            trim = min(selected_diameter, points[-2].sub(points[-1]).Length * 0.45)
            points[-1] = points[-1] + end_direction * trim
            remaining = points[-2].sub(points[-1]).Length
            if remaining > selected_diameter * 1.5:
                points.insert(
                    -1,
                    points[-1] + end_direction * min(selected_diameter, remaining * 0.5),
                )
        if debug:
            App.Console.PrintMessage(
                f"[Forms Pipe] segment={index} key={key} diameter={selected_diameter:.12g} "
                f"samples={selected_samples} "
                f"chain={segment} nodes={first_node}(degree={len(adjacency[first_node])})->"
                f"{last_node}(degree={len(adjacency[last_node])}) edge_lengths="
                f"{[round(edges[edge].Length, 9) for edge, _forward in segment]} "
                f"controls={[tuple(round(value, 9) for value in point) for point in points]} "
                f"steps={[round(first.sub(second).Length, 9) for first, second in zip(points, points[1:])]}\n"
            )
        local_vertices, local_faces, start_loop, end_loop = swept_segment_cage(
            points,
            selected_diameter,
            side_segments,
            open_start,
            open_end,
            True,
        )
        offset = len(vertices)
        vertices.extend(local_vertices)
        faces.extend(tuple(offset + vertex for vertex in face) for face in local_faces)
        if open_start:
            start_guides = _boundary_guides(local_faces, start_loop, local_vertices)
            junction_ports.setdefault(first_node, []).append(
                (
                    start_direction,
                    [(offset + vertex, local_vertices[vertex]) for vertex in start_loop],
                    selected_diameter,
                    {
                        offset + vertex: point
                        for vertex, point in start_guides.items()
                    },
                )
            )
        if open_end:
            end_guides = _boundary_guides(local_faces, end_loop, local_vertices)
            junction_ports.setdefault(last_node, []).append(
                (
                    end_direction,
                    [(offset + vertex, local_vertices[vertex]) for vertex in end_loop],
                    selected_diameter,
                    {
                        offset + vertex: point for vertex, point in end_guides.items()
                    },
                )
            )
        original_edges = dict.fromkeys(edge_records[edge][1] + 1 for edge, _forward in segment)
        edge_names = ", ".join(f"Edge{edge}" for edge in original_edges)
        descriptions.append(
            App.Qt.translate("Forms_Pipe", "Segment %1 (%2)")
            .replace("%1", str(index))
            .replace("%2", edge_names)
        )
        keys.append(key)
    for junction_index, (node, ports) in enumerate(sorted(junction_ports.items()), 1):
        # A three-way node continues the most nearly opposite pair as one
        # trunk and inserts the remaining branch into a side opening.  Higher
        # valences retain the generic perforated junction topology.
        center = node_points[node]
        if len(ports) == 3:
            local_vertices, local_faces, boundary_maps, vertex_updates = (
                _three_way_junction_cage(center, ports, side_segments)
            )
        else:
            local_vertices, local_faces, boundary_maps, vertex_updates = _junction_cage(
                center, ports, side_segments
            )
        for global_index, point in vertex_updates.items():
            vertices[global_index] = point
        boundary_map = {
            local: global_index
            for mapping in boundary_maps
            for local, global_index in mapping.items()
        }
        vertex_map = dict(boundary_map)
        for local_index, point in enumerate(local_vertices):
            if local_index not in vertex_map:
                vertex_map[local_index] = len(vertices)
                vertices.append(point)
        faces.extend(
            tuple(vertex_map[vertex] for vertex in face) for face in local_faces
        )
        if debug:
            junction_diameter = max(port[2] for port in ports)
            App.Console.PrintMessage(
                f"[Forms Pipe] junction={junction_index} node={node} "
                f"degree={len(adjacency[node])} diameter={junction_diameter:.12g} "
                f"cage_vertices={len(local_vertices)} cage_faces={len(local_faces)}\n"
            )
    if debug:
        App.Console.PrintMessage(
            f"[Forms Pipe] cage_vertices={len(vertices)} cage_faces={len(faces)} "
            f"junctions={len(junction_ports)}\n"
        )
    return vertices, faces, keys, descriptions


def _cage_components(cage, include_maps=False):
    by_vertex = {}
    for face_index, face in enumerate(cage.faces):
        for vertex in face:
            by_vertex.setdefault(vertex, set()).add(face_index)
    unused = set(range(len(cage.faces)))
    components = []
    while unused:
        pending = [min(unused)]
        face_indices = set()
        while pending:
            face_index = pending.pop()
            if face_index not in unused:
                continue
            unused.remove(face_index)
            face_indices.add(face_index)
            for vertex in cage.faces[face_index]:
                pending.extend(by_vertex[vertex].intersection(unused))
        used_vertices = sorted(
            {vertex for face_index in face_indices for vertex in cage.faces[face_index]}
        )
        remap = {vertex: index for index, vertex in enumerate(used_vertices)}
        faces = [
            tuple(remap[vertex] for vertex in cage.faces[face_index])
            for face_index in sorted(face_indices)
        ]
        edge_sharpness = {
            tuple(sorted((remap[first], remap[second]))): value
            for (first, second), value in cage.edge_sharpness.items()
            if first in remap and second in remap
        }
        component = ControlCage(
            [cage.vertices[vertex] for vertex in used_vertices],
            faces,
            [cage.vertex_sharpness[vertex] for vertex in used_vertices],
            edge_sharpness,
        )
        components.append((component, remap) if include_maps else component)
    return components


def update_pipe_shape(obj):
    """Evaluate every closed path-segment cage and preserve their patch faces."""
    try:
        if obj.CageMode == "Editable":
            from .cage import update_object_shape
            update_object_shape(obj)
            if not obj.Shape.isNull() and len(obj.Shape.Solids) > 1:
                obj.Shape = map_form_shape(obj, fused_pipe_shape(obj.Shape))
            return
        cage = ControlCage.from_object(obj)
        shapes = []
        debug = bool(getattr(obj, "DebugGeometry", False))
        maximum_deviation = 0.0
        conversion_level = 0
        dissolved_edges = set()
        for encoded in getattr(obj, "DissolvedEdges", ()):
            first, second = str(encoded).split()
            dissolved_edges.add(tuple(sorted((int(first), int(second)))))
        components = _cage_components(cage, True)
        for index, (component, remap) in enumerate(components, 1):
            if not component.is_closed:
                raise ConversionError("Every editable pipe segment must remain closed")
            max_refinement = (
                min(int(obj.MaxRefinement), 2)
                if len(component.faces) > 256
                else obj.MaxRefinement
            )
            shape, deviation, level = cage_to_solid(
                component.vertices,
                component.faces,
                obj.BRepTolerance.Value,
                max_refinement,
                component.edge_sharpness,
                component.vertex_sharpness,
                dissolved_edges={
                    tuple(sorted((remap[first], remap[second])))
                    for first, second in dissolved_edges
                    if first in remap and second in remap
                },
            )
            shapes.append(shape)
            if debug:
                App.Console.PrintMessage(
                    f"[Forms Pipe] solid={index} cage_vertices={len(component.vertices)} "
                    f"cage_faces={len(component.faces)} brep_faces={len(shape.Faces)} "
                    f"valid={shape.isValid()}\n"
                )
            maximum_deviation = max(maximum_deviation, deviation)
            conversion_level = max(conversion_level, level)
        if not shapes:
            raise ConversionError("The pipe has no evaluable segments")
        if debug:
            for first in range(len(shapes)):
                for second in range(first + 1, len(shapes)):
                    distance = shapes[first].distToShape(shapes[second])[0]
                    App.Console.PrintMessage(
                        f"[Forms Pipe] solid_pair={first + 1},{second + 1} "
                        f"distance={distance:.12g}\n"
                    )
        shape = (
            shapes[0]
            if len(shapes) == 1
            else fused_pipe_shape(Part.makeCompound(shapes), debug)
        )
        obj.Shape = map_form_shape(obj, shape)
        obj.MaximumDeviation = maximum_deviation
        obj.ConversionLevel = conversion_level
        obj.ConversionStatus = App.Qt.translate("Forms_Conversion", "Valid pipe")
        if maximum_deviation > obj.BRepTolerance.Value:
            obj.ConversionStatus += "; requested deviation was not reached"
    except (ConversionError, Part.OCCError, ValueError, RuntimeError) as error:
        obj.Shape = Part.Shape()
        obj.MaximumDeviation = 0.0
        obj.ConversionLevel = 0
        obj.ConversionStatus = App.Qt.translate("Forms_Conversion", "Failed: %1").replace(
            "%1", str(error)
        )


def fused_pipe_shape(shape, debug=False):
    """Fuse the overlapping segment solids used by a Part Design operation."""
    solids = list(shape.Solids)
    if not solids:
        return Part.Shape()
    try:
        result = solids[0].multiFuse(solids[1:]) if len(solids) > 1 else solids[0]
        if debug:
            App.Console.PrintMessage(
                f"[Forms Pipe] multi_fuse_inputs={len(solids)} "
                f"result_solids={len(result.Solids)} "
                f"result_faces={len(result.Faces)} valid={result.isValid()}\n"
            )
    except (Part.OCCError, RuntimeError):
        # Retain a conservative fallback for older OCC versions where a
        # simultaneous Boolean can reject otherwise valid disjoint inputs.
        result = solids[0]
        for index, solid in enumerate(solids[1:], 2):
            result = result.fuse(solid)
            if debug:
                App.Console.PrintMessage(
                    f"[Forms Pipe] fallback_fuse_through={index} "
                    f"result_solids={len(result.Solids)} "
                    f"result_faces={len(result.Faces)} valid={result.isValid()}\n"
                )
    try:
        result = result.removeSplitter()
    except (Part.OCCError, RuntimeError):
        pass
    if len(result.Solids) == 1:
        return result.Solids[0]
    return result


class FormPipeProxy(FormFeatureProxy):
    Type = "Forms::Pipe"
    ParameterNames = ("Diameter", "SectionSegments", "PathSamples")

    def __init__(self, obj, path_object=None):
        self._add_common_properties(obj)
        self._ensure_pipe_properties(obj)
        obj.PathObject = path_object
        obj.Diameter = 10.0
        obj.SectionSegments = (1, 1, 12, 1)
        obj.PathSamples = (3, 1, 20, 1)
        obj.DebugGeometry = False
        self._finish_initialization(obj)

    @staticmethod
    def _ensure_pipe_properties(obj):
        group = "Pipe"
        if "PathObject" not in obj.PropertiesList:
            obj.addProperty("App::PropertyLink", "PathObject", group, "Wire-network path")
        if "Diameter" not in obj.PropertiesList:
            obj.addProperty("App::PropertyLength", "Diameter", group, "Global pipe diameter")
        if "SectionSegments" not in obj.PropertiesList:
            obj.addProperty(
                "App::PropertyIntegerConstraint",
                "SectionSegments",
                group,
                "Radial subdivision level (eight control segments around the pipe per level)",
            )
        if "PathSamples" not in obj.PropertiesList:
            obj.addProperty(
                "App::PropertyIntegerConstraint",
                "PathSamples",
                group,
                "Average control intervals per source edge, distributed by path length",
            )
        if "PipeSegmentKeys" not in obj.PropertiesList:
            obj.addProperty(
                "App::PropertyStringList", "PipeSegmentKeys", group, "Stable segment edge keys"
            )
        if "PipeSegments" not in obj.PropertiesList:
            obj.addProperty(
                "App::PropertyStringList", "PipeSegments", group, "Detected path segments"
            )
        if "SegmentDiameters" not in obj.PropertiesList:
            obj.addProperty(
                "App::PropertyStringList",
                "SegmentDiameters",
                group,
                "Per-segment diameter overrides",
            )
        if "SegmentSamples" not in obj.PropertiesList:
            obj.addProperty(
                "App::PropertyStringList",
                "SegmentSamples",
                group,
                "Per-segment longitudinal intervals per source edge",
            )
        if "DebugGeometry" not in obj.PropertiesList:
            obj.addProperty(
                "App::PropertyBool",
                "DebugGeometry",
                group,
                "Print path segmentation and solid-joining diagnostics during recompute",
            )
        obj.setEditorMode("PipeSegmentKeys", 2)
        obj.setEditorMode("PipeSegments", 2)
        obj.setEditorMode("SegmentDiameters", 2)
        obj.setEditorMode("SegmentSamples", 2)

    def _topology(self, obj):
        vertices, faces, keys, descriptions = pipe_control_cage(
            obj.PathObject,
            obj.Diameter.Value,
            obj.SectionSegments,
            obj.PathSamples,
            decode_segment_overrides(obj.SegmentDiameters),
            bool(obj.DebugGeometry),
            decode_segment_sample_overrides(obj.SegmentSamples),
        )
        obj.PipeSegmentKeys = keys
        obj.PipeSegments = descriptions
        return vertices, faces

    def execute(self, obj):
        if obj.CageMode == "Parametric":
            try:
                vertices, faces = self._topology(obj)
                reset_cage(obj, vertices, faces)
            except (Part.OCCError, RuntimeError, ValueError) as error:
                obj.Shape = Part.Shape()
                obj.ConversionStatus = App.Qt.translate(
                    "Forms_Conversion", "Failed: %1"
                ).replace("%1", str(error))
                return
        update_pipe_shape(obj)

    def onDocumentRestored(self, obj):
        self._ensure_pipe_properties(obj)
        super().onDocumentRestored(obj)


class ViewProviderFormPipe(ViewProviderFormBox):
    IconName = "Forms_Pipe.svg"


def create_pipe(document=None, path_object=None, name="FormPipe"):
    document = document or App.ActiveDocument
    if document is None:
        raise RuntimeError("A document is required to create a Form Pipe")
    if path_object is None or path_object.Document is not document:
        raise ValueError("Select a path object from the active document")
    obj = document.addObject("Part::FeaturePython", name)
    obj.Label = App.Qt.translate("Forms_Create", "Form Pipe")
    FormPipeProxy(obj, path_object)
    if App.GuiUp:
        ViewProviderFormPipe(obj.ViewObject)
    obj.recompute()
    return obj
