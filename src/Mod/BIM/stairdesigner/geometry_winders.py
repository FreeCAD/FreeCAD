# SPDX-License-Identifier: LGPL-2.1-or-later

"""Balanced linear-flight winding construction."""

import bisect
import math

from .geometry_core import (
    BalancedSection,
    distribute_treads,
    tread_stations,
)


def balanced_winder_sections(
    flight_specs,
    tread_count,
    winding_coefficient=1.0,
    turn_types=None,
    start_angle=0.0,
    end_angle=0.0,
    entry_direction="Straight",
    exit_direction="Straight",
    landing_replaces_tread=True,
    winding_parameters=None,
    nosing=0.0,
    extra_widths=None,
):
    """Return riser sections balanced from equal-going nosing lines.

    ``flight_specs`` contains ``(length, width, heading_degrees)`` tuples.
    Quadratic transition curves spread each balanced turn approximately one
    flight width into each adjacent flight. Sections outside those local
    balancing zones remain perpendicular to their flight. A ``Landing`` turn
    instead reserves one complete tread between the incoming and outgoing
    edges of the rectangular corner.

    The walking-line stations belong to the visible nosings.  The returned
    sections are moved behind those stations by ``nosing`` so they remain the
    construction/riser borders consumed by the tread and riser builders.
    """

    if not flight_specs or tread_count < 1:
        return [], 0.0

    specs = [
        (max(float(length), 0.01), max(float(width), 0.01), float(heading))
        for length, width, heading in flight_specs
    ]
    vertices = [(0.0, specs[0][1] / 2.0)]
    directions = []
    for length, _width, heading in specs:
        radians = math.radians(heading)
        direction = (math.cos(radians), math.sin(radians))
        directions.append(direction)
        start = vertices[-1]
        vertices.append((start[0] + direction[0] * length, start[1] + direction[1] * length))

    coefficient = max(float(winding_coefficient), 0.0)
    corner_controls = _winding_controls(len(specs) - 1, coefficient, winding_parameters)
    corner_types = list(turn_types or [])
    corner_types.extend(["Herse balancing"] * (len(specs) - 1 - len(corner_types)))
    corner_types = [
        "Landing" if str(value) == "Landing" else "Herse balancing"
        for value in corner_types[: len(specs) - 1]
    ]
    if "Landing" in corner_types:
        return _landing_winder_sections(
            specs,
            vertices,
            directions,
            tread_count,
            coefficient,
            corner_types,
            start_angle,
            end_angle,
            entry_direction,
            exit_direction,
            landing_replaces_tread,
            winding_parameters,
            extra_widths,
        )

    corner_trims = _herse_corner_trims(specs, corner_controls)
    start_trim = _endpoint_balance_trim(specs[0][0], specs[0][1], coefficient, entry_direction)
    end_trim = _endpoint_balance_trim(specs[-1][0], specs[-1][1], coefficient, exit_direction)
    if corner_trims:
        incoming, outgoing = corner_trims[0]
        start_trim, incoming = _fit_transition_trims(start_trim, incoming, specs[0][0])
        corner_trims[0] = incoming, outgoing
        incoming, outgoing = corner_trims[-1]
        outgoing, end_trim = _fit_transition_trims(outgoing, end_trim, specs[-1][0])
        corner_trims[-1] = incoming, outgoing
    elif start_trim or end_trim:
        start_trim, end_trim = _fit_transition_trims(start_trim, end_trim, specs[0][0])

    dense = []

    def append_point(point, width, flight_index):
        if dense:
            previous = dense[-1]
            if math.hypot(point[0] - previous[0], point[1] - previous[1]) < 1e-9:
                dense[-1] = (point[0], point[1], width, flight_index)
                return
        dense.append((point[0], point[1], width, flight_index))

    if start_trim:
        _append_endpoint_transition(
            append_point,
            vertices[0],
            directions[0],
            specs[0][1],
            start_trim,
            entry_direction,
            0,
            True,
            start_angle,
        )
    else:
        append_point(vertices[0], specs[0][1], 0)
    for index, (_length, width, _heading) in enumerate(specs):
        direction = directions[index]
        flight_end_trim = corner_trims[index][0] if index < len(corner_trims) else end_trim
        straight_end = (
            vertices[index + 1][0] - direction[0] * flight_end_trim,
            vertices[index + 1][1] - direction[1] * flight_end_trim,
        )
        append_point(straight_end, width, index)
        if index >= len(corner_trims):
            continue

        corner = vertices[index + 1]
        outgoing_direction = directions[index + 1]
        outgoing_trim = corner_trims[index][1]
        curve_end = (
            corner[0] + outgoing_direction[0] * outgoing_trim,
            corner[1] + outgoing_direction[1] * outgoing_trim,
        )
        outgoing_width = specs[index + 1][1]
        samples = 64
        for sample in range(1, samples + 1):
            ratio = sample / samples
            point = _herse_curve_point(
                straight_end,
                corner,
                curve_end,
                ratio,
                corner_controls[index][0],
            )
            curve_width = width + (outgoing_width - width) * ratio
            owner = index if ratio < 0.5 else index + 1
            append_point(point, curve_width, owner)

    if end_trim:
        _append_endpoint_transition(
            append_point,
            vertices[-1],
            directions[-1],
            specs[-1][1],
            end_trim,
            exit_direction,
            len(specs) - 1,
            False,
            end_angle,
        )

    cumulative = [0.0]
    for first, second in zip(dense, dense[1:]):
        cumulative.append(cumulative[-1] + math.hypot(second[0] - first[0], second[1] - first[1]))
    total_length = cumulative[-1]
    going, stations = tread_stations(total_length, tread_count, extra_widths)
    nosing = max(float(nosing), 0.0)
    sections = []
    for index in range(tread_count + 1):
        station = stations[index]
        # The terminal section is the rear boundary of the final tread.  All
        # preceding sections have a visible nosing ``nosing`` millimetres
        # before their riser line.  Sample that visible line on the balanced
        # walking path, then derive the construction line behind it.  On a
        # straight run this exactly preserves the existing riser positions;
        # around a turn it prevents a tangent-offset software edge from being
        # the line that controls the balancing.
        sample_station = station if index == tread_count else station - nosing
        segment = max(
            0,
            min(
                bisect.bisect_right(cumulative, sample_station) - 1,
                len(dense) - 2,
            ),
        )
        first = dense[segment]
        second = dense[segment + 1]
        segment_length = cumulative[segment + 1] - cumulative[segment]
        ratio = (sample_station - cumulative[segment]) / segment_length if segment_length else 0.0
        center = (
            first[0] + (second[0] - first[0]) * ratio,
            first[1] + (second[1] - first[1]) * ratio,
        )
        tangent_length = math.hypot(second[0] - first[0], second[1] - first[1])
        tangent = (
            (second[0] - first[0]) / tangent_length,
            (second[1] - first[1]) / tangent_length,
        )
        if index < tread_count:
            center = (
                center[0] + tangent[0] * nosing,
                center[1] + tangent[1] * nosing,
            )
        normal = (-tangent[1], tangent[0])
        width = first[2] + (second[2] - first[2]) * ratio
        half_width = width / 2.0
        sections.append(
            BalancedSection(
                center=center,
                tangent=tangent,
                left=(
                    center[0] + normal[0] * half_width,
                    center[1] + normal[1] * half_width,
                ),
                right=(
                    center[0] - normal[0] * half_width,
                    center[1] - normal[1] * half_width,
                ),
                station=station,
                width=width,
                flight_index=first[3] if ratio < 0.5 else second[3],
                locked_to_flight=_section_is_locked_to_flight(
                    center,
                    tangent,
                    first[3] if ratio < 0.5 else second[3],
                    specs,
                    vertices,
                    directions,
                    corner_types,
                    corner_controls,
                ),
            )
        )
    sections = _fit_sections_to_flight_footprint(sections, specs, vertices, directions)
    sections = _apply_endpoint_boundary_sections(
        sections,
        vertices,
        directions,
        specs,
        start_angle,
        end_angle,
        entry_direction,
        exit_direction,
        coefficient,
    )
    return sections, going


def _winding_controls(count, coefficient, winding_parameters=None):
    """Return ``(local percent, distant factor)`` for each junction."""

    if winding_parameters is None:
        return [(50.0, max(float(coefficient), 0.75))] * count
    parameters = list(winding_parameters)
    controls = []
    for index in range(count):
        local, distant = parameters[index] if index < len(parameters) else (50.0, 50.0)
        local = min(max(float(local), 0.0), 100.0)
        distant = min(max(float(distant), 0.0), 100.0)
        controls.append((local, 1.5 - distant / 100.0))
    return controls


def _herse_curve_point(start, corner, end, ratio, local):
    """Return a cubic Herse point with adjustable near-corner tightening."""

    ratio = min(max(float(ratio), 0.0), 1.0)
    influence = 1.0 / 3.0 + 2.0 / 3.0 * min(max(float(local), 0.0), 100.0) / 100.0
    first_control = (
        start[0] + (corner[0] - start[0]) * influence,
        start[1] + (corner[1] - start[1]) * influence,
    )
    second_control = (
        end[0] + (corner[0] - end[0]) * influence,
        end[1] + (corner[1] - end[1]) * influence,
    )
    inverse = 1.0 - ratio
    return (
        inverse**3 * start[0]
        + 3.0 * inverse**2 * ratio * first_control[0]
        + 3.0 * inverse * ratio**2 * second_control[0]
        + ratio**3 * end[0],
        inverse**3 * start[1]
        + 3.0 * inverse**2 * ratio * first_control[1]
        + 3.0 * inverse * ratio**2 * second_control[1]
        + ratio**3 * end[1],
    )


def _herse_corner_trims(specs, corner_controls):
    """Return the local balancing length on both sides of every turn."""

    trims = []
    for index in range(len(specs) - 1):
        factor = corner_controls[index][1]
        incoming_length, incoming_width, _heading = specs[index]
        outgoing_length, outgoing_width, _heading = specs[index + 1]
        trims.append(
            (
                min(incoming_width * factor, incoming_length * 0.48),
                min(outgoing_width * factor, outgoing_length * 0.48),
            )
        )
    return trims


def _endpoint_side(direction):
    value = str(direction).lower()
    if "left" in value:
        return 1.0
    if "right" in value:
        return -1.0
    return 0.0


def _endpoint_balance_trim(length, width, coefficient, direction):
    if not _endpoint_side(direction):
        return 0.0
    return min(
        max(float(width), 0.01) * max(float(coefficient), 0.75),
        max(float(length), 0.01) * 0.48,
    )


def _fit_transition_trims(first, second, length):
    total = first + second
    available = max(float(length), 0.01) * 0.98
    if total > available:
        scale = available / total
        return first * scale, second * scale
    return first, second


def _safe_angle_tangent(angle):
    limited = min(max(float(angle), -89.0), 89.0)
    return math.tan(math.radians(limited))


def _append_endpoint_transition(
    append_point,
    vertex,
    direction,
    width,
    trim,
    requested_direction,
    flight_index,
    is_start,
    boundary_angle,
):
    """Append a quadratic Herse transition between a side and flight axis."""

    side = _endpoint_side(requested_direction)
    if not side or trim <= 1e-9:
        append_point(vertex, width, flight_index)
        return
    normal = (-direction[1], direction[0])
    side_offset = side * width / 2.0
    boundary_offset = side * width / 2.0 * _safe_angle_tangent(boundary_angle)
    if is_start:
        join = trim
        chord_length = max(join - boundary_offset, 0.01)
        endpoint_station = boundary_offset + chord_length / 2.0
        endpoint = (
            vertex[0] + direction[0] * endpoint_station + normal[0] * side_offset,
            vertex[1] + direction[1] * endpoint_station + normal[1] * side_offset,
        )
        control = (
            vertex[0] + direction[0] * endpoint_station,
            vertex[1] + direction[1] * endpoint_station,
        )
        axis = (
            vertex[0] + direction[0] * join,
            vertex[1] + direction[1] * join,
        )
        first, middle, last = endpoint, control, axis
        first_width, last_width = chord_length, width
    else:
        join = -trim
        chord_length = max(trim + boundary_offset, 0.01)
        endpoint_station = boundary_offset - chord_length / 2.0
        endpoint = (
            vertex[0] + direction[0] * endpoint_station + normal[0] * side_offset,
            vertex[1] + direction[1] * endpoint_station + normal[1] * side_offset,
        )
        control = (
            vertex[0] + direction[0] * endpoint_station,
            vertex[1] + direction[1] * endpoint_station,
        )
        axis = (
            vertex[0] + direction[0] * join,
            vertex[1] + direction[1] * join,
        )
        first, middle, last = axis, control, endpoint
        first_width, last_width = width, chord_length

    for sample in range(65):
        if not is_start and sample == 0:
            continue
        ratio = sample / 64.0
        inverse = 1.0 - ratio
        point = (
            inverse * inverse * first[0]
            + 2.0 * inverse * ratio * middle[0]
            + ratio * ratio * last[0],
            inverse * inverse * first[1]
            + 2.0 * inverse * ratio * middle[1]
            + ratio * ratio * last[1],
        )
        transition_width = first_width + (last_width - first_width) * ratio
        append_point(point, transition_width, flight_index)


def _apply_endpoint_boundary_sections(
    sections,
    vertices,
    directions,
    specs,
    start_angle,
    end_angle,
    entry_direction,
    exit_direction,
    coefficient,
):
    """Make straight entry/exit sections coincide with angled end cuts."""

    if not sections:
        return sections
    result = list(sections)

    def unlocked(section):
        return BalancedSection(
            center=section.center,
            tangent=section.tangent,
            left=section.left,
            right=section.right,
            station=section.station,
            width=section.width,
            flight_index=section.flight_index,
            landing_to_next=section.landing_to_next,
            locked_to_flight=False,
            level_to_next=section.level_to_next,
            riser_index=section.riser_index,
        )

    def boundary_section(section, vertex, direction, width, angle):
        normal = (-direction[1], direction[0])
        offset = width / 2.0 * _safe_angle_tangent(angle)
        left = (
            vertex[0] + normal[0] * width / 2.0 + direction[0] * offset,
            vertex[1] + normal[1] * width / 2.0 + direction[1] * offset,
        )
        right = (
            vertex[0] - normal[0] * width / 2.0 - direction[0] * offset,
            vertex[1] - normal[1] * width / 2.0 - direction[1] * offset,
        )
        chord = (left[0] - right[0], left[1] - right[1])
        chord_length = math.hypot(chord[0], chord[1])
        tangent = (chord[1] / chord_length, -chord[0] / chord_length)
        if tangent[0] * direction[0] + tangent[1] * direction[1] < 0.0:
            tangent = (-tangent[0], -tangent[1])
        return BalancedSection(
            center=((left[0] + right[0]) / 2.0, (left[1] + right[1]) / 2.0),
            tangent=tangent,
            left=left,
            right=right,
            station=section.station,
            width=chord_length,
            flight_index=section.flight_index,
            landing_to_next=section.landing_to_next,
            locked_to_flight=False,
            level_to_next=section.level_to_next,
            riser_index=section.riser_index,
        )

    def side_section(section, vertex, direction, width, angle, requested, start):
        side = _endpoint_side(requested)
        normal = (-direction[1], direction[0])
        boundary_offset = side * width / 2.0 * _safe_angle_tangent(angle)
        length = specs[0][0] if start else specs[-1][0]
        trim = _endpoint_balance_trim(length, width, coefficient, requested)
        if start:
            first_station = boundary_offset
            second_station = trim
            tangent = (-side * normal[0], -side * normal[1])
        else:
            first_station = -trim
            second_station = boundary_offset
            tangent = (side * normal[0], side * normal[1])
        first = (
            vertex[0] + direction[0] * first_station + normal[0] * side * width / 2.0,
            vertex[1] + direction[1] * first_station + normal[1] * side * width / 2.0,
        )
        second = (
            vertex[0] + direction[0] * second_station + normal[0] * side * width / 2.0,
            vertex[1] + direction[1] * second_station + normal[1] * side * width / 2.0,
        )
        section_normal = (-tangent[1], tangent[0])
        projection = (first[0] - second[0]) * section_normal[0] + (
            first[1] - second[1]
        ) * section_normal[1]
        left, right = (first, second) if projection >= 0.0 else (second, first)
        return BalancedSection(
            center=((first[0] + second[0]) / 2.0, (first[1] + second[1]) / 2.0),
            tangent=tangent,
            left=left,
            right=right,
            station=section.station,
            width=math.hypot(first[0] - second[0], first[1] - second[1]),
            flight_index=section.flight_index,
            landing_to_next=section.landing_to_next,
            locked_to_flight=False,
            level_to_next=section.level_to_next,
            riser_index=section.riser_index,
        )

    if _endpoint_side(entry_direction):
        result[0] = side_section(
            result[0],
            vertices[0],
            directions[0],
            specs[0][1],
            start_angle,
            entry_direction,
            True,
        )
        for index in range(1, len(result)):
            section = result[index]
            was_locked = section.locked_to_flight
            result[index] = unlocked(section)
            if was_locked or section.flight_index != 0:
                break
    elif abs(float(start_angle)) > 1e-7:
        result[0] = boundary_section(
            result[0], vertices[0], directions[0], specs[0][1], start_angle
        )
        balance_length = min(
            specs[0][1] * max(float(coefficient), 0.75),
            specs[0][0] * 0.48,
        )
        for index, section in enumerate(result):
            if section.flight_index != 0 or section.station > balance_length:
                break
            result[index] = unlocked(section)
    if _endpoint_side(exit_direction):
        result[-1] = side_section(
            result[-1],
            vertices[-1],
            directions[-1],
            specs[-1][1],
            end_angle,
            exit_direction,
            False,
        )
        for index in range(len(result) - 2, -1, -1):
            section = result[index]
            was_locked = section.locked_to_flight
            result[index] = unlocked(section)
            if was_locked or section.flight_index != len(specs) - 1:
                break
    elif abs(float(end_angle)) > 1e-7:
        result[-1] = boundary_section(
            result[-1], vertices[-1], directions[-1], specs[-1][1], end_angle
        )
        balance_length = min(
            specs[-1][1] * max(float(coefficient), 0.75),
            specs[-1][0] * 0.48,
        )
        final_station = result[-1].station
        for index in range(len(result) - 1, -1, -1):
            section = result[index]
            if (
                section.flight_index != len(specs) - 1
                or final_station - section.station > balance_length
            ):
                break
            result[index] = unlocked(section)
    return result


def _section_is_locked_to_flight(
    center,
    tangent,
    flight_index,
    specs,
    vertices,
    directions,
    corner_types,
    corner_controls,
):
    """Return whether a straight section lies outside every Herse zone."""

    direction = directions[flight_index]
    if not _tangent_matches_direction(tangent, direction):
        return False

    relative = (
        center[0] - vertices[flight_index][0],
        center[1] - vertices[flight_index][1],
    )
    station = relative[0] * direction[0] + relative[1] * direction[1]
    flight_length, flight_width, _heading = specs[flight_index]
    previous_balance_length = (
        min(flight_width * corner_controls[flight_index - 1][1], flight_length)
        if flight_index > 0
        else 0.0
    )
    next_balance_length = (
        min(flight_width * corner_controls[flight_index][1], flight_length)
        if flight_index < len(corner_controls)
        else 0.0
    )
    near_previous_herse = (
        flight_index > 0
        and corner_types[flight_index - 1] == "Herse balancing"
        and station <= previous_balance_length + 1e-7
    )
    near_next_herse = (
        flight_index < len(specs) - 1
        and corner_types[flight_index] == "Herse balancing"
        and flight_length - station <= next_balance_length + 1e-7
    )
    return not (near_previous_herse or near_next_herse)


def _landing_winder_sections(
    specs,
    vertices,
    directions,
    tread_count,
    coefficient,
    corner_types,
    start_angle=0.0,
    end_angle=0.0,
    entry_direction="Straight",
    exit_direction="Straight",
    landing_replaces_tread=True,
    winding_parameters=None,
    extra_widths=None,
):
    """Return sections with each landing kept as one unsampled corner interval."""

    corner_controls = _winding_controls(len(specs) - 1, coefficient, winding_parameters)
    corner_trims = _herse_corner_trims(specs, corner_controls)

    chunks = [[]]

    def append_point(chunk, point, width, flight_index):
        if chunk and math.hypot(point[0] - chunk[-1][0], point[1] - chunk[-1][1]) < 1e-9:
            chunk[-1] = (point[0], point[1], width, flight_index)
        else:
            chunk.append((point[0], point[1], width, flight_index))

    start_trim = _endpoint_balance_trim(specs[0][0], specs[0][1], coefficient, entry_direction)
    end_trim = _endpoint_balance_trim(specs[-1][0], specs[-1][1], coefficient, exit_direction)
    if len(specs) == 1:
        start_trim, end_trim = _fit_transition_trims(start_trim, end_trim, specs[0][0])
    if start_trim:
        _append_endpoint_transition(
            lambda point, width, owner: append_point(chunks[-1], point, width, owner),
            vertices[0],
            directions[0],
            specs[0][1],
            start_trim,
            entry_direction,
            0,
            True,
            start_angle,
        )
    else:
        append_point(chunks[-1], vertices[0], specs[0][1], 0)
    landing_count = 0
    for index, (_length, width, _heading) in enumerate(specs):
        direction = directions[index]
        if index >= len(specs) - 1:
            endpoint = (
                vertices[index + 1][0] - direction[0] * end_trim,
                vertices[index + 1][1] - direction[1] * end_trim,
            )
            append_point(chunks[-1], endpoint, width, index)
            if end_trim:
                _append_endpoint_transition(
                    lambda point, transition_width, owner: append_point(
                        chunks[-1], point, transition_width, owner
                    ),
                    vertices[index + 1],
                    direction,
                    width,
                    end_trim,
                    exit_direction,
                    index,
                    False,
                    end_angle,
                )
            continue

        corner = vertices[index + 1]
        outgoing_direction = directions[index + 1]
        turn_sine = abs(direction[0] * outgoing_direction[1] - direction[1] * outgoing_direction[0])
        is_landing = corner_types[index] == "Landing" and turn_sine > 1e-7
        if is_landing:
            incoming_trim = specs[index + 1][1] / (2.0 * turn_sine)
            outgoing_trim = width / (2.0 * turn_sine)
            entry = (
                corner[0] - direction[0] * incoming_trim,
                corner[1] - direction[1] * incoming_trim,
            )
            exit_point = (
                corner[0] + outgoing_direction[0] * outgoing_trim,
                corner[1] + outgoing_direction[1] * outgoing_trim,
            )
            append_point(chunks[-1], entry, width, index)
            chunks.append([])
            append_point(chunks[-1], exit_point, specs[index + 1][1], index + 1)
            landing_count += 1
            continue

        incoming_trim, outgoing_trim = corner_trims[index]
        straight_end = (
            corner[0] - direction[0] * incoming_trim,
            corner[1] - direction[1] * incoming_trim,
        )
        append_point(chunks[-1], straight_end, width, index)
        curve_end = (
            corner[0] + outgoing_direction[0] * outgoing_trim,
            corner[1] + outgoing_direction[1] * outgoing_trim,
        )
        outgoing_width = specs[index + 1][1]
        for sample in range(1, 65):
            ratio = sample / 64.0
            point = _herse_curve_point(
                straight_end,
                corner,
                curve_end,
                ratio,
                corner_controls[index][0],
            )
            curve_width = width + (outgoing_width - width) * ratio
            owner = index if ratio < 0.5 else index + 1
            append_point(chunks[-1], point, curve_width, owner)

    chunk_lengths = [_dense_path_length(chunk) for chunk in chunks]
    free_tread_count = tread_count - (landing_count if landing_replaces_tread else 0)
    distributed_tread_count = free_tread_count
    if not landing_replaces_tread:
        distributed_tread_count -= landing_count
    required_free_treads = sum(length > 1e-7 for length in chunk_lengths)
    if landing_count == 0 or distributed_tread_count < required_free_treads:
        return balanced_winder_sections(
            specs,
            tread_count,
            coefficient,
            ["Herse balancing" if value == "Landing" else value for value in corner_types],
            start_angle,
            end_angle,
            entry_direction,
            exit_direction,
            landing_replaces_tread,
            winding_parameters,
            extra_widths=extra_widths,
        )

    chunk_tread_counts = distribute_treads(chunk_lengths, distributed_tread_count)
    if not landing_replaces_tread:
        for index in range(min(landing_count, len(chunk_tread_counts) - 1)):
            chunk_tread_counts[index] += 1
    extras = list(extra_widths or [])[:tread_count]
    extras.extend([0.0] * (tread_count - len(extras)))
    sampled = []
    extra_cursor = 0
    for index, (chunk, count) in enumerate(zip(chunks, chunk_tread_counts)):
        if index:
            # The interval between the two chunks is the landing tread.
            extra_cursor += 1
        chunk_extras = extras[extra_cursor : extra_cursor + count]
        chunk_sections = _sample_dense_path(chunk, count, chunk_extras)
        extra_cursor += count
        if index:
            sampled[-1]["landing_to_next"] = True
        sampled.extend(chunk_sections)

    free_length = sum(chunk_lengths)
    nominal_going = free_length / max(free_tread_count, 1)
    sampled_tread_count = max(len(sampled) - 1, 0)
    going, sampled_stations = tread_stations(
        nominal_going * sampled_tread_count,
        sampled_tread_count,
        extras[:sampled_tread_count],
    )
    sections = []
    riser_index = 0
    for index, sample in enumerate(sampled):
        if (
            not landing_replaces_tread
            and index < len(sampled) - 1
            and not sample["landing_to_next"]
        ):
            riser_index += 1
        center = sample["center"]
        tangent = sample["tangent"]
        normal = (-tangent[1], tangent[0])
        half_width = sample["width"] / 2.0
        sections.append(
            BalancedSection(
                center=center,
                tangent=tangent,
                left=(
                    center[0] + normal[0] * half_width,
                    center[1] + normal[1] * half_width,
                ),
                right=(
                    center[0] - normal[0] * half_width,
                    center[1] - normal[1] * half_width,
                ),
                station=sampled_stations[index],
                width=sample["width"],
                flight_index=sample["flight_index"],
                landing_to_next=sample["landing_to_next"],
                locked_to_flight=_section_is_locked_to_flight(
                    center,
                    tangent,
                    sample["flight_index"],
                    specs,
                    vertices,
                    directions,
                    corner_types,
                    corner_controls,
                ),
                riser_index=(riser_index if not landing_replaces_tread else 0),
            )
        )
    sections = _fit_sections_to_flight_footprint(sections, specs, vertices, directions)
    sections = _apply_endpoint_boundary_sections(
        sections,
        vertices,
        directions,
        specs,
        start_angle,
        end_angle,
        entry_direction,
        exit_direction,
        coefficient,
    )
    return sections, going


def _dense_path_length(points):
    return sum(
        math.hypot(second[0] - first[0], second[1] - first[1])
        for first, second in zip(points, points[1:])
    )


def _tangent_matches_direction(tangent, direction):
    return abs(tangent[0] * direction[1] - tangent[1] * direction[0]) < 1e-9


def _sample_dense_path(points, interval_count, extra_widths=None):
    """Sample one uninterrupted straight/balanced path chunk."""

    cumulative = [0.0]
    for first, second in zip(points, points[1:]):
        cumulative.append(cumulative[-1] + math.hypot(second[0] - first[0], second[1] - first[1]))
    total_length = cumulative[-1]
    _general_going, stations = tread_stations(total_length, interval_count, extra_widths)
    result = []
    for index in range(interval_count + 1):
        station = stations[index]
        segment = max(
            0,
            min(
                bisect.bisect_right(cumulative, station) - 1,
                len(points) - 2,
            ),
        )
        first = points[segment]
        second = points[segment + 1]
        segment_length = cumulative[segment + 1] - cumulative[segment]
        ratio = (station - cumulative[segment]) / segment_length if segment_length else 0.0
        tangent_length = math.hypot(second[0] - first[0], second[1] - first[1])
        result.append(
            {
                "center": (
                    first[0] + (second[0] - first[0]) * ratio,
                    first[1] + (second[1] - first[1]) * ratio,
                ),
                "tangent": (
                    (second[0] - first[0]) / tangent_length,
                    (second[1] - first[1]) / tangent_length,
                ),
                "width": first[2] + (second[2] - first[2]) * ratio,
                "flight_index": first[3] if ratio < 0.5 else second[3],
                "landing_to_next": False,
            }
        )
    return result


def _fit_sections_to_flight_footprint(sections, specs, vertices, directions):
    """Extend each nosing to the fixed union of the rectangular flights."""

    start_extensions, end_extensions = _flight_corner_extensions(specs, directions)
    fitted = []
    for index, section in enumerate(sections):
        if section.landing_to_next or (index and sections[index - 1].landing_to_next):
            fitted.append(section)
            continue
        normal = (-section.tangent[1], section.tangent[0])
        intervals = []
        for index, (length, width, _heading) in enumerate(specs):
            direction = directions[index]
            side = (-direction[1], direction[0])
            relative = (
                section.center[0] - vertices[index][0],
                section.center[1] - vertices[index][1],
            )
            local_point = (
                relative[0] * direction[0] + relative[1] * direction[1],
                relative[0] * side[0] + relative[1] * side[1],
            )
            local_line = (
                normal[0] * direction[0] + normal[1] * direction[1],
                normal[0] * side[0] + normal[1] * side[1],
            )
            interval = _line_rectangle_interval(
                local_point,
                local_line,
                (-start_extensions[index], length + end_extensions[index]),
                (-width / 2.0, width / 2.0),
            )
            if interval:
                intervals.append(interval)

        intervals.sort()
        merged = []
        for lower, upper in intervals:
            if merged and lower <= merged[-1][1] + 1e-7:
                merged[-1] = (merged[-1][0], max(merged[-1][1], upper))
            else:
                merged.append((lower, upper))
        selected = next(
            (interval for interval in merged if interval[0] <= 0.0 <= interval[1]),
            min(merged, key=lambda interval: min(abs(interval[0]), abs(interval[1]))),
        )
        right_parameter, left_parameter = selected
        fitted.append(
            BalancedSection(
                center=section.center,
                tangent=section.tangent,
                left=(
                    section.center[0] + normal[0] * left_parameter,
                    section.center[1] + normal[1] * left_parameter,
                ),
                right=(
                    section.center[0] + normal[0] * right_parameter,
                    section.center[1] + normal[1] * right_parameter,
                ),
                station=section.station,
                width=left_parameter - right_parameter,
                flight_index=section.flight_index,
                landing_to_next=section.landing_to_next,
                locked_to_flight=section.locked_to_flight,
                level_to_next=section.level_to_next,
                riser_index=section.riser_index,
            )
        )
    return fitted


def _line_rectangle_interval(point, direction, x_limits, y_limits):
    lower = -float("inf")
    upper = float("inf")
    for coordinate, delta, limits in (
        (point[0], direction[0], x_limits),
        (point[1], direction[1], y_limits),
    ):
        if abs(delta) < 1e-12:
            if coordinate < limits[0] - 1e-9 or coordinate > limits[1] + 1e-9:
                return None
            continue
        first = (limits[0] - coordinate) / delta
        second = (limits[1] - coordinate) / delta
        lower = max(lower, min(first, second))
        upper = min(upper, max(first, second))
        if lower > upper:
            return None
    return lower, upper


def _flight_corner_extensions(specs, directions=None):
    """Return longitudinal extensions that fill every flight intersection."""

    if directions is None:
        directions = []
        for _length, _width, heading in specs:
            radians = math.radians(heading)
            directions.append((math.cos(radians), math.sin(radians)))
    starts = [0.0] * len(specs)
    ends = [0.0] * len(specs)
    for index in range(len(specs) - 1):
        incoming = directions[index]
        outgoing = directions[index + 1]
        turn_sine = abs(incoming[0] * outgoing[1] - incoming[1] * outgoing[0])
        if turn_sine < 1e-7:
            continue
        ends[index] = specs[index + 1][1] / (2.0 * turn_sine)
        starts[index + 1] = specs[index][1] / (2.0 * turn_sine)
    return starts, ends
