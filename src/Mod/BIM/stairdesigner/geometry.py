# SPDX-License-Identifier: LGPL-2.1-or-later

"""Geometry helpers for the BIM Stair Designer."""

from dataclasses import dataclass, replace
import bisect
import math

import FreeCAD
import Part


BLONDEL_MINIMUM = 620.0
BLONDEL_MAXIMUM = 640.0


@dataclass(frozen=True)
class StraightStairMetrics:
    """Computed dimensions for one straight flight."""

    floor_height: float
    flight_length: float
    riser_count: int
    tread_count: int
    riser_height: float
    tread_width: float
    blondel_value: float
    blondel_compliant: bool


@dataclass(frozen=True)
class BalancedSection:
    """One tread-nosing section across a balanced stair plan."""

    center: tuple
    tangent: tuple
    left: tuple
    right: tuple
    station: float
    width: float
    flight_index: int
    landing_to_next: bool = False
    locked_to_flight: bool = False
    level_to_next: bool = False
    riser_index: int = 0


@dataclass(frozen=True)
class _CircularProfile:
    """Exact annular-sector data shared by circular concrete helpers."""

    center: tuple
    inner_radius: float
    outer_radius: float
    start_angle: float
    sweep: float


def straight_stair_metrics(floor_height, flight_length, riser_count):
    """Return the derived dimensions for a straight flight.

    ``riser_count`` is exposed to users as the number of steps. The upper
    floor supplies the final tread, so a flight has one fewer manufactured
    tread than risers.
    """

    floor_height = max(float(floor_height), 0.0)
    flight_length = max(float(flight_length), 0.0)
    riser_count = max(int(riser_count), 2)
    tread_count = riser_count - 1
    riser_height = floor_height / riser_count
    tread_width = flight_length / tread_count
    blondel_value = 2.0 * riser_height + tread_width
    return StraightStairMetrics(
        floor_height=floor_height,
        flight_length=flight_length,
        riser_count=riser_count,
        tread_count=tread_count,
        riser_height=riser_height,
        tread_width=tread_width,
        blondel_value=blondel_value,
        blondel_compliant=BLONDEL_MINIMUM <= blondel_value <= BLONDEL_MAXIMUM,
    )


def flight_stair_metrics(flight_length, tread_count, riser_height):
    """Return geometry metrics for one flight of a multi-flight stair."""

    flight_length = max(float(flight_length), 0.0)
    tread_count = max(int(tread_count), 0)
    riser_height = max(float(riser_height), 0.0)
    tread_width = flight_length / tread_count if tread_count else 0.0
    blondel_value = 2.0 * riser_height + tread_width
    return StraightStairMetrics(
        floor_height=tread_count * riser_height,
        flight_length=flight_length,
        riser_count=tread_count,
        tread_count=tread_count,
        riser_height=riser_height,
        tread_width=tread_width,
        blondel_value=blondel_value,
        blondel_compliant=BLONDEL_MINIMUM <= blondel_value <= BLONDEL_MAXIMUM,
    )


def distribute_treads(flight_lengths, tread_count):
    """Distribute manufactured treads proportionally over several flights."""

    lengths = [max(float(length), 0.0) for length in flight_lengths]
    if not lengths:
        return []
    tread_count = max(int(tread_count), 0)
    if tread_count < len(lengths):
        return [1 if index < tread_count else 0 for index in range(len(lengths))]
    result = [1] * len(lengths)
    remaining = tread_count - len(lengths)
    if not remaining:
        return result

    total = sum(lengths)
    weights = lengths if total > 0.0 else [1.0] * len(lengths)
    total = sum(weights)
    shares = [remaining * weight / total for weight in weights]
    whole = [int(math.floor(share)) for share in shares]
    result = [base + extra for base, extra in zip(result, whole)]
    unassigned = remaining - sum(whole)
    order = sorted(
        range(len(lengths)),
        key=lambda index: (shares[index] - whole[index], weights[index], -index),
        reverse=True,
    )
    for index in order[:unassigned]:
        result[index] += 1
    return result


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
):
    """Return equal-going sections along a smoothly balanced walking line.

    ``flight_specs`` contains ``(length, width, heading_degrees)`` tuples.
    Quadratic transition curves spread each balanced turn approximately one
    flight width into each adjacent flight. Sections outside those local
    balancing zones remain perpendicular to their flight. A ``Landing`` turn
    instead reserves one complete tread between the incoming and outgoing
    edges of the rectangular corner.
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
        vertices.append(
            (start[0] + direction[0] * length, start[1] + direction[1] * length)
        )

    coefficient = max(float(winding_coefficient), 0.0)
    corner_controls = _winding_controls(
        len(specs) - 1, coefficient, winding_parameters
    )
    corner_types = list(turn_types or [])
    corner_types.extend(
        ["Herse balancing"] * (len(specs) - 1 - len(corner_types))
    )
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
        )

    corner_trims = _herse_corner_trims(specs, corner_controls)
    start_trim = _endpoint_balance_trim(
        specs[0][0], specs[0][1], coefficient, entry_direction
    )
    end_trim = _endpoint_balance_trim(
        specs[-1][0], specs[-1][1], coefficient, exit_direction
    )
    if corner_trims:
        incoming, outgoing = corner_trims[0]
        start_trim, incoming = _fit_transition_trims(
            start_trim, incoming, specs[0][0]
        )
        corner_trims[0] = incoming, outgoing
        incoming, outgoing = corner_trims[-1]
        outgoing, end_trim = _fit_transition_trims(
            outgoing, end_trim, specs[-1][0]
        )
        corner_trims[-1] = incoming, outgoing
    elif start_trim or end_trim:
        start_trim, end_trim = _fit_transition_trims(
            start_trim, end_trim, specs[0][0]
        )

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
        flight_end_trim = (
            corner_trims[index][0]
            if index < len(corner_trims)
            else end_trim
        )
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
        cumulative.append(
            cumulative[-1] + math.hypot(second[0] - first[0], second[1] - first[1])
        )
    total_length = cumulative[-1]
    going = total_length / tread_count
    sections = []
    for index in range(tread_count + 1):
        station = total_length if index == tread_count else index * going
        segment = max(0, min(bisect.bisect_right(cumulative, station) - 1, len(dense) - 2))
        first = dense[segment]
        second = dense[segment + 1]
        segment_length = cumulative[segment + 1] - cumulative[segment]
        ratio = (station - cumulative[segment]) / segment_length if segment_length else 0.0
        center = (
            first[0] + (second[0] - first[0]) * ratio,
            first[1] + (second[1] - first[1]) * ratio,
        )
        tangent_length = math.hypot(second[0] - first[0], second[1] - first[1])
        tangent = (
            (second[0] - first[0]) / tangent_length,
            (second[1] - first[1]) / tangent_length,
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
    sections = _fit_sections_to_flight_footprint(
        sections, specs, vertices, directions
    )
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
        local, distant = (
            parameters[index] if index < len(parameters) else (50.0, 50.0)
        )
        local = min(max(float(local), 0.0), 100.0)
        distant = min(max(float(distant), 0.0), 100.0)
        controls.append((local, 1.5 - distant / 100.0))
    return controls


def _herse_curve_point(start, corner, end, ratio, local):
    """Return a cubic Herse point with adjustable near-corner tightening."""

    ratio = min(max(float(ratio), 0.0), 1.0)
    influence = 1.0 / 3.0 + 2.0 / 3.0 * min(
        max(float(local), 0.0), 100.0
    ) / 100.0
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
    boundary_offset = side * width / 2.0 * _safe_angle_tangent(
        boundary_angle
    )
    if is_start:
        join = trim
        chord_length = max(join - boundary_offset, 0.01)
        endpoint_station = boundary_offset + chord_length / 2.0
        endpoint = (
            vertex[0]
            + direction[0] * endpoint_station
            + normal[0] * side_offset,
            vertex[1]
            + direction[1] * endpoint_station
            + normal[1] * side_offset,
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
            vertex[0]
            + direction[0] * endpoint_station
            + normal[0] * side_offset,
            vertex[1]
            + direction[1] * endpoint_station
            + normal[1] * side_offset,
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
        trim = _endpoint_balance_trim(
            length, width, coefficient, requested
        )
        if start:
            first_station = boundary_offset
            second_station = trim
            tangent = (-side * normal[0], -side * normal[1])
        else:
            first_station = -trim
            second_station = boundary_offset
            tangent = (side * normal[0], side * normal[1])
        first = (
            vertex[0]
            + direction[0] * first_station
            + normal[0] * side * width / 2.0,
            vertex[1]
            + direction[1] * first_station
            + normal[1] * side * width / 2.0,
        )
        second = (
            vertex[0]
            + direction[0] * second_station
            + normal[0] * side * width / 2.0,
            vertex[1]
            + direction[1] * second_station
            + normal[1] * side * width / 2.0,
        )
        section_normal = (-tangent[1], tangent[0])
        projection = (
            (first[0] - second[0]) * section_normal[0]
            + (first[1] - second[1]) * section_normal[1]
        )
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
):
    """Return sections with each landing kept as one unsampled corner interval."""

    corner_controls = _winding_controls(
        len(specs) - 1, coefficient, winding_parameters
    )
    corner_trims = _herse_corner_trims(specs, corner_controls)

    chunks = [[]]

    def append_point(chunk, point, width, flight_index):
        if chunk and math.hypot(
            point[0] - chunk[-1][0], point[1] - chunk[-1][1]
        ) < 1e-9:
            chunk[-1] = (point[0], point[1], width, flight_index)
        else:
            chunk.append((point[0], point[1], width, flight_index))

    start_trim = _endpoint_balance_trim(
        specs[0][0], specs[0][1], coefficient, entry_direction
    )
    end_trim = _endpoint_balance_trim(
        specs[-1][0], specs[-1][1], coefficient, exit_direction
    )
    if len(specs) == 1:
        start_trim, end_trim = _fit_transition_trims(
            start_trim, end_trim, specs[0][0]
        )
    if start_trim:
        _append_endpoint_transition(
            lambda point, width, owner: append_point(
                chunks[-1], point, width, owner
            ),
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
        turn_sine = abs(
            direction[0] * outgoing_direction[1]
            - direction[1] * outgoing_direction[0]
        )
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
            append_point(
                chunks[-1], exit_point, specs[index + 1][1], index + 1
            )
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
    free_tread_count = tread_count - (
        landing_count if landing_replaces_tread else 0
    )
    distributed_tread_count = free_tread_count
    if not landing_replaces_tread:
        distributed_tread_count -= landing_count
    required_free_treads = sum(length > 1e-7 for length in chunk_lengths)
    if landing_count == 0 or distributed_tread_count < required_free_treads:
        return balanced_winder_sections(
            specs,
            tread_count,
            coefficient,
            [
                "Herse balancing" if value == "Landing" else value
                for value in corner_types
            ],
            start_angle,
            end_angle,
            entry_direction,
            exit_direction,
            landing_replaces_tread,
            winding_parameters,
        )

    chunk_tread_counts = distribute_treads(
        chunk_lengths, distributed_tread_count
    )
    if not landing_replaces_tread:
        for index in range(min(landing_count, len(chunk_tread_counts) - 1)):
            chunk_tread_counts[index] += 1
    sampled = []
    for index, (chunk, count) in enumerate(zip(chunks, chunk_tread_counts)):
        chunk_sections = _sample_dense_path(chunk, count)
        if index:
            sampled[-1]["landing_to_next"] = True
        sampled.extend(chunk_sections)

    free_length = sum(chunk_lengths)
    going = free_length / max(free_tread_count, 1)
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
                station=index * going,
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
    sections = _fit_sections_to_flight_footprint(
        sections, specs, vertices, directions
    )
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
    return abs(
        tangent[0] * direction[1] - tangent[1] * direction[0]
    ) < 1e-9


def _sample_dense_path(points, interval_count):
    """Sample one uninterrupted straight/balanced path chunk."""

    cumulative = [0.0]
    for first, second in zip(points, points[1:]):
        cumulative.append(
            cumulative[-1]
            + math.hypot(second[0] - first[0], second[1] - first[1])
        )
    total_length = cumulative[-1]
    result = []
    for index in range(interval_count + 1):
        station = (
            total_length
            if index == interval_count
            else index * total_length / interval_count
        )
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
        ratio = (
            (station - cumulative[segment]) / segment_length
            if segment_length
            else 0.0
        )
        tangent_length = math.hypot(
            second[0] - first[0], second[1] - first[1]
        )
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


def tangent_flight_sections(
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
):
    """Return sections for paths containing tangential circular flights."""

    if not flight_specs or tread_count < 1:
        return [], 0.0
    specs, primitives = _tangent_path_primitives(flight_specs)
    corner_types = list(turn_types or [])
    corner_types.extend(
        ["Herse balancing"] * (len(specs) - 1 - len(corner_types))
    )
    corner_types = corner_types[: len(specs) - 1]
    modes = _tangent_junction_modes(primitives, corner_types)
    corner_controls = _winding_controls(
        len(modes), winding_coefficient, winding_parameters
    )
    start_trims = [0.0] * len(primitives)
    end_trims = [0.0] * len(primitives)
    for index, mode in enumerate(modes):
        incoming = primitives[index]
        outgoing = primitives[index + 1]
        if mode == "Herse balancing":
            factor = corner_controls[index][1]
            end_trims[index] = min(
                incoming["width"] * factor,
                incoming["length"] * 0.48,
            )
            start_trims[index + 1] = min(
                outgoing["width"] * factor,
                outgoing["length"] * 0.48,
            )
        elif mode == "Landing":
            turn_sine = abs(_cross(incoming["end_tangent"], outgoing["tangent"]))
            end_trims[index] = min(
                outgoing["width"] / (2.0 * turn_sine),
                incoming["length"],
            )
            start_trims[index + 1] = min(
                incoming["width"] / (2.0 * turn_sine),
                outgoing["length"],
            )

    if primitives[0]["type"] == "Straight":
        start_trims[0] = _endpoint_balance_trim(
            primitives[0]["length"],
            primitives[0]["width"],
            winding_coefficient,
            entry_direction,
        )
    if primitives[-1]["type"] == "Straight":
        end_trims[-1] = _endpoint_balance_trim(
            primitives[-1]["length"],
            primitives[-1]["width"],
            winding_coefficient,
            exit_direction,
        )

    for index, primitive in enumerate(primitives):
        total_trim = start_trims[index] + end_trims[index]
        available = primitive["length"] * 0.98
        if total_trim > available:
            scale = available / total_trim
            start_trims[index] *= scale
            end_trims[index] *= scale

    chunks = [[]]
    separators = []
    landing_count = 0
    if start_trims[0] and primitives[0]["type"] == "Straight":
        _append_endpoint_transition(
            lambda point, width, owner: _append_dense_point(
                chunks[-1], point, width, owner
            ),
            primitives[0]["start"],
            primitives[0]["tangent"],
            primitives[0]["width"],
            start_trims[0],
            entry_direction,
            0,
            True,
            start_angle,
        )
    for index, primitive in enumerate(primitives):
        _append_primitive_range(
            chunks[-1],
            primitive,
            start_trims[index],
            primitive["length"] - end_trims[index],
        )
        if index >= len(modes):
            continue
        mode = modes[index]
        if mode == "Landing":
            chunks.append([])
            separators.append("Landing")
            landing_count += 1
        elif mode == "Tangent":
            chunks.append([])
            separators.append("Tangent")
        elif mode == "Herse balancing":
            incoming = primitive
            outgoing = primitives[index + 1]
            curve_start = _primitive_point(
                incoming, incoming["length"] - end_trims[index]
            )
            corner = incoming["end"]
            curve_end = _primitive_point(
                outgoing, start_trims[index + 1]
            )
            for sample in range(1, 65):
                ratio = sample / 64.0
                point = _herse_curve_point(
                    curve_start,
                    corner,
                    curve_end,
                    ratio,
                    corner_controls[index][0],
                )
                width = incoming["width"] + (
                    outgoing["width"] - incoming["width"]
                ) * ratio
                owner = index if ratio < 0.5 else index + 1
                _append_dense_point(chunks[-1], point, width, owner)

    if end_trims[-1] and primitives[-1]["type"] == "Straight":
        _append_endpoint_transition(
            lambda point, width, owner: _append_dense_point(
                chunks[-1], point, width, owner
            ),
            primitives[-1]["end"],
            primitives[-1]["end_tangent"],
            primitives[-1]["width"],
            end_trims[-1],
            exit_direction,
            len(primitives) - 1,
            False,
            end_angle,
        )

    chunk_lengths = [_dense_path_length(chunk) for chunk in chunks]
    chunk_is_landing = [
        bool(chunk) and primitives[chunk[0][3]]["is_landing"]
        for chunk in chunks
    ]
    explicit_landing_count = sum(chunk_is_landing)
    free_tread_count = (
        tread_count
        - landing_count
        - (explicit_landing_count if landing_replaces_tread else 0)
    )
    stair_chunk_indices = [
        index
        for index, is_landing in enumerate(chunk_is_landing)
        if not is_landing
    ]
    required_free_treads = sum(
        chunk_lengths[index] > 1e-7 for index in stair_chunk_indices
    )
    distributed_tread_count = free_tread_count
    if not landing_replaces_tread:
        distributed_tread_count -= explicit_landing_count
    if distributed_tread_count < required_free_treads:
        without_landings = [
            "Herse balancing" if mode == "Landing" else mode
            for mode in corner_types
        ]
        if without_landings == corner_types:
            return [], 0.0
        return tangent_flight_sections(
            flight_specs,
            tread_count,
            winding_coefficient,
            without_landings,
            start_angle,
            end_angle,
            entry_direction,
            exit_direction,
            landing_replaces_tread,
            winding_parameters,
        )

    stair_chunk_counts = distribute_treads(
        [chunk_lengths[index] for index in stair_chunk_indices],
        distributed_tread_count,
    )
    if not landing_replaces_tread:
        stair_count_positions = {
            chunk_index: position
            for position, chunk_index in enumerate(stair_chunk_indices)
        }
        for landing_index, is_landing in enumerate(chunk_is_landing):
            if not is_landing:
                continue
            receiving_index = next(
                (
                    chunk_index
                    for chunk_index in reversed(stair_chunk_indices)
                    if chunk_index < landing_index
                ),
                None,
            )
            if receiving_index is None:
                receiving_index = next(
                    (
                        chunk_index
                        for chunk_index in stair_chunk_indices
                        if chunk_index > landing_index
                    ),
                    None,
                )
            if receiving_index is not None:
                stair_chunk_counts[
                    stair_count_positions[receiving_index]
                ] += 1
    chunk_tread_counts = [1 if is_landing else 0 for is_landing in chunk_is_landing]
    for index, count in zip(stair_chunk_indices, stair_chunk_counts):
        chunk_tread_counts[index] = count
    sampled = []
    for index, (chunk, count) in enumerate(zip(chunks, chunk_tread_counts)):
        chunk_sections = _sample_dense_path(chunk, count)
        if chunk_is_landing[index]:
            if sampled:
                sampled[-1]["flight_index"] = chunk_sections[0][
                    "flight_index"
                ]
                sampled[-1]["landing_to_next"] = True
                sampled[-1]["level_to_next"] = True
                sampled.extend(chunk_sections[1:])
            else:
                chunk_sections[0]["landing_to_next"] = True
                chunk_sections[0]["level_to_next"] = True
                sampled.extend(chunk_sections)
        elif index and separators[index - 1] == "Landing":
            sampled[-1]["landing_to_next"] = True
            sampled.extend(chunk_sections)
        elif index and chunk_is_landing[index - 1]:
            sampled[-1] = chunk_sections[0]
            sampled.extend(chunk_sections[1:])
        elif index:
            sampled.extend(chunk_sections[1:])
        else:
            sampled.extend(chunk_sections)

    free_length = sum(
        chunk_lengths[index] for index in stair_chunk_indices
    )
    going = free_length / max(free_tread_count, 1)
    riser_index = 0
    for index, sample in enumerate(sampled):
        if (
            index < len(sampled) - 1
            and not (
                not landing_replaces_tread
                and sample.get("level_to_next", False)
            )
        ):
            riser_index += 1
        sample["riser_index"] = riser_index
    sections = []
    for index, sample in enumerate(sampled):
        center = sample["center"]
        tangent = sample["tangent"]
        primitive = primitives[sample["flight_index"]]
        if primitive["type"] == "Circular":
            radial = (
                center[0] - primitive["circle_center"][0],
                center[1] - primitive["circle_center"][1],
            )
            radial_length = math.hypot(radial[0], radial[1])
            unit_radial = (
                radial[0] / radial_length,
                radial[1] / radial_length,
            )
            center = (
                primitive["circle_center"][0]
                + unit_radial[0] * primitive["radius"],
                primitive["circle_center"][1]
                + unit_radial[1] * primitive["radius"],
            )
            tangent = (
                -primitive["sign"] * unit_radial[1],
                primitive["sign"] * unit_radial[0],
            )
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
                station=index * going,
                width=sample["width"],
                flight_index=sample["flight_index"],
                landing_to_next=sample["landing_to_next"],
                locked_to_flight=True,
                level_to_next=sample.get("level_to_next", False),
                riser_index=sample["riser_index"],
            )
        )
    endpoint_specs = [
        (primitive["length"], primitive["width"], 0.0)
        for primitive in primitives
    ]
    endpoint_vertices = [
        primitive["start"] for primitive in primitives
    ] + [primitives[-1]["end"]]
    endpoint_directions = [
        primitive["tangent"] for primitive in primitives
    ]
    sections = _apply_endpoint_boundary_sections(
        sections,
        endpoint_vertices,
        endpoint_directions,
        endpoint_specs,
        start_angle if primitives[0]["type"] == "Straight" else 0.0,
        end_angle if primitives[-1]["type"] == "Straight" else 0.0,
        entry_direction if primitives[0]["type"] == "Straight" else "Straight",
        exit_direction if primitives[-1]["type"] == "Straight" else "Straight",
        winding_coefficient,
    )
    return sections, going


def _tangent_path_primitives(flight_specs):
    specs = []
    for flight_spec in flight_specs:
        flight_type, dimension, width, angle, rotation = flight_spec[:5]
        entry_direction = (
            str(flight_spec[5]) if len(flight_spec) > 5 else "Straight"
        )
        exit_direction = (
            str(flight_spec[6]) if len(flight_spec) > 6 else "Straight"
        )
        requested_type = str(flight_type)
        is_landing = requested_type in {
            "Straight landing",
            "Circular landing",
        }
        flight_type = (
            "Circular" if requested_type.startswith("Circular") else "Straight"
        )
        width = max(float(width), 0.01)
        sweep = min(max(abs(math.radians(float(angle))), 1e-6), 2.0 * math.pi - 1e-6)
        if flight_type == "Circular":
            inner_radius = max(float(dimension), 0.01)
            radius = inner_radius + width / 2.0
            length = radius * sweep
        else:
            inner_radius = None
            radius = None
            length = max(float(dimension), 0.01)
        specs.append(
            {
                "type": flight_type,
                "length": length,
                "width": width,
                "angle": float(angle),
                "rotation": "Right" if str(rotation) == "Right" else "Left",
                "inner_radius": inner_radius,
                "radius": radius,
                "sweep": sweep,
                "is_landing": is_landing,
                "entry_direction": entry_direction,
                "exit_direction": exit_direction,
            }
        )

    center = (0.0, specs[0]["width"] / 2.0)
    heading = 0.0
    primitives = []
    for index, spec in enumerate(specs):
        if index:
            previous = specs[index - 1]
            landing_entry = _endpoint_side(spec["entry_direction"])
            if (
                spec["is_landing"]
                and spec["type"] == "Straight"
                and landing_entry
            ):
                heading += landing_entry * math.radians(abs(spec["angle"]))
            elif (
                not spec["is_landing"]
                and not previous["is_landing"]
                and previous["type"] == "Straight"
                and spec["type"] == "Straight"
            ):
                turn = math.radians(spec["angle"])
                if spec["rotation"] == "Right":
                    turn = -turn
                heading += turn
        tangent = (math.cos(heading), math.sin(heading))
        primitive = {
            **spec,
            "index": index,
            "start": center,
            "heading": heading,
            "tangent": tangent,
        }
        if spec["type"] == "Circular":
            sign = -1.0 if spec["rotation"] == "Right" else 1.0
            normal = (-tangent[1], tangent[0])
            primitive["sign"] = sign
            primitive["circle_center"] = (
                center[0] + sign * normal[0] * spec["radius"],
                center[1] + sign * normal[1] * spec["radius"],
            )
            center = _primitive_point(primitive, spec["length"])
            heading += sign * spec["sweep"]
        elif spec["is_landing"]:
            normal = (-tangent[1], tangent[0])
            half_width = spec["width"] / 2.0
            entry_side = _endpoint_side(spec["entry_direction"])
            exit_side = _endpoint_side(spec["exit_direction"])
            side_port_offset = min(
                half_width, max((spec["length"] - 0.01) / 2.0, 0.0)
            )
            entry_port = center
            face_start = (
                entry_port[0]
                - tangent[0] * side_port_offset * abs(entry_side)
                - normal[0] * half_width * entry_side,
                entry_port[1]
                - tangent[1] * side_port_offset * abs(entry_side)
                - normal[1] * half_width * entry_side,
            )
            face_end = (
                face_start[0] + tangent[0] * spec["length"],
                face_start[1] + tangent[1] * spec["length"],
            )
            exit_port = (
                face_end[0]
                - tangent[0] * side_port_offset * abs(exit_side)
                + normal[0] * half_width * exit_side,
                face_end[1]
                - tangent[1] * side_port_offset * abs(exit_side)
                + normal[1] * half_width * exit_side,
            )
            primitive["face_start"] = face_start
            primitive["face_end"] = face_end
            primitive["path_start"] = entry_port
            primitive["path_end"] = exit_port
            center = exit_port
            if exit_side:
                heading += exit_side * math.radians(abs(spec["angle"]))
        else:
            center = (
                center[0] + tangent[0] * spec["length"],
                center[1] + tangent[1] * spec["length"],
            )
        primitive["end"] = center
        primitive["end_tangent"] = (math.cos(heading), math.sin(heading))
        primitives.append(primitive)
    return specs, primitives


def _primitive_point(primitive, distance):
    distance = min(max(float(distance), 0.0), primitive["length"])
    if primitive["type"] == "Straight":
        if primitive.get("is_landing"):
            ratio = distance / primitive["length"]
            start = primitive.get("path_start", primitive["start"])
            end = primitive.get("path_end", primitive["end"])
            return (
                start[0] + (end[0] - start[0]) * ratio,
                start[1] + (end[1] - start[1]) * ratio,
            )
        return (
            primitive["start"][0] + primitive["tangent"][0] * distance,
            primitive["start"][1] + primitive["tangent"][1] * distance,
        )
    angle = primitive["sign"] * distance / primitive["radius"]
    relative = (
        primitive["start"][0] - primitive["circle_center"][0],
        primitive["start"][1] - primitive["circle_center"][1],
    )
    cosine = math.cos(angle)
    sine = math.sin(angle)
    return (
        primitive["circle_center"][0]
        + relative[0] * cosine
        - relative[1] * sine,
        primitive["circle_center"][1]
        + relative[0] * sine
        + relative[1] * cosine,
    )


def _primitive_tangent(primitive, distance):
    if primitive["type"] == "Straight":
        return primitive["tangent"]
    heading = primitive["heading"] + (
        primitive["sign"] * distance / primitive["radius"]
    )
    return math.cos(heading), math.sin(heading)


def _append_primitive_range(chunk, primitive, start, end):
    start = min(max(float(start), 0.0), primitive["length"])
    end = min(max(float(end), start), primitive["length"])
    samples = 1
    if primitive["type"] == "Circular":
        samples = max(
            8,
            int(math.ceil(128.0 * (end - start) / primitive["length"])),
        )
    for sample in range(samples + 1):
        ratio = sample / samples
        distance = start + (end - start) * ratio
        _append_dense_point(
            chunk,
            _primitive_point(primitive, distance),
            primitive["width"],
            primitive["index"],
        )


def _append_dense_point(chunk, point, width, flight_index):
    if chunk and math.hypot(
        point[0] - chunk[-1][0], point[1] - chunk[-1][1]
    ) < 1e-9:
        chunk[-1] = (point[0], point[1], width, flight_index)
    else:
        chunk.append((point[0], point[1], width, flight_index))


def _cross(first, second):
    return first[0] * second[1] - first[1] * second[0]


def _dot(first, second):
    return first[0] * second[0] + first[1] * second[1]


def _tangent_junction_modes(primitives, corner_types):
    modes = []
    for index, requested in enumerate(corner_types):
        incoming = primitives[index]
        outgoing = primitives[index + 1]
        tangent = (
            incoming["is_landing"]
            or outgoing["is_landing"]
            or
            incoming["type"] == "Circular"
            or outgoing["type"] == "Circular"
            or abs(_cross(incoming["end_tangent"], outgoing["tangent"])) < 1e-7
        )
        if tangent:
            modes.append("Tangent")
        else:
            modes.append(
                "Landing" if str(requested) == "Landing" else "Herse balancing"
            )
    return modes


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


def _fit_sections_to_flight_footprint(sections, specs, vertices, directions):
    """Extend each nosing to the fixed union of the rectangular flights."""

    start_extensions, end_extensions = _flight_corner_extensions(
        specs, directions
    )
    fitted = []
    for index, section in enumerate(sections):
        if section.landing_to_next or (
            index and sections[index - 1].landing_to_next
        ):
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
        turn_sine = abs(
            incoming[0] * outgoing[1] - incoming[1] * outgoing[0]
        )
        if turn_sine < 1e-7:
            continue
        ends[index] = specs[index + 1][1] / (2.0 * turn_sine)
        starts[index + 1] = specs[index][1] / (2.0 * turn_sine)
    return starts, ends


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


def _shifted(point, tangent, distance):
    return (
        point[0] + tangent[0] * distance,
        point[1] + tangent[1] * distance,
    )


def _horizontal_face(points, elevation):
    vectors = [FreeCAD.Vector(point[0], point[1], elevation) for point in points]
    vectors.append(vectors[0])
    return Part.Face(Part.makePolygon(vectors))


def make_balanced_tread_shape(
    front,
    rear,
    footprint,
    elevation,
    thickness,
    nosing,
    back_extension=0.0,
    base_face=None,
    local_expansion=False,
):
    """Create one polygonal tread between balanced nosing sections."""

    if base_face is None:
        faces = _balanced_step_faces(
            front,
            rear,
            footprint,
            front_offset=-nosing,
            rear_offset=back_extension,
        )
    else:
        plan_shape = base_face.copy()
        if nosing > 0.0 or back_extension > 0.0:
            expansions = _local_step_expansion_faces(
                front,
                rear,
                footprint,
                nosing,
                back_extension,
                local_expansion,
            )
            for expansion in expansions:
                plan_shape = plan_shape.fuse(expansion)
            plan_shape = plan_shape.removeSplitter()
        faces = plan_shape.Faces
    solids = []
    for face in faces:
        placed_face = face.copy()
        placed_face.translate(FreeCAD.Vector(0.0, 0.0, elevation - thickness))
        solids.append(placed_face.extrude(FreeCAD.Vector(0.0, 0.0, thickness)))
    result = solids[0]
    for solid in solids[1:]:
        result = result.fuse(solid)
    return result.removeSplitter()


def _local_step_expansion_faces(
    front,
    rear,
    footprint,
    nosing,
    back_extension,
    local_expansion=False,
):
    """Return edge-local tread bands without following another flight."""

    faces = []
    for section, distance in (
        (front, -nosing),
        (rear, back_extension),
    ):
        if abs(distance) < 1e-9:
            continue
        shifted_left = _shifted(section.left, section.tangent, distance)
        shifted_right = _shifted(section.right, section.tangent, distance)
        raw_strip = _horizontal_face(
            (
                section.left,
                section.right,
                shifted_right,
                shifted_left,
            ),
            0.0,
        )
        shifted_center = _shifted(
            section.center, section.tangent, distance
        )
        probe = FreeCAD.Vector(
            (section.center[0] + shifted_center[0]) / 2.0,
            (section.center[1] + shifted_center[1]) / 2.0,
            0.0,
        )
        # At the entrance or exit the expansion deliberately projects beyond
        # the footprint.  There is no adjoining tread there to provide a
        # clipping outline, so preserve the complete band.
        if not footprint.isInside(probe, 1e-6, True):
            faces.append(raw_strip)
            continue
        if distance < 0.0 and not local_expansion:
            # A regular balanced nosing can cross a footprint vertex.  Its
            # side boundary then changes rails inside the expansion and the
            # complete clipped section band is required to retain the corner.
            faces.extend(
                _section_band_faces(section, footprint, distance)
            )
            continue
        side = (-section.tangent[1], section.tangent[0])
        shifted_left = _continued_section_endpoint(
            section.left,
            shifted_left,
            side,
            section.tangent,
            footprint,
        )
        shifted_right = _continued_section_endpoint(
            section.right,
            shifted_right,
            (-side[0], -side[1]),
            section.tangent,
            footprint,
        )
        strip = _horizontal_face(
            (
                section.left,
                section.right,
                shifted_right,
                shifted_left,
            ),
            0.0,
        )
        clipped = strip.common(footprint)
        candidates = clipped.Faces
        if not candidates:
            continue
        selected = next(
            (
                candidate
                for candidate in candidates
                if candidate.isInside(probe, 1e-6, True)
            ),
            None,
        )
        if selected is None:
            point = Part.Vertex(probe)
            selected = min(
                candidates,
                key=lambda candidate: candidate.distToShape(point)[0],
            )
        faces.append(selected)
    return faces


def _continued_section_endpoint(
    original,
    shifted,
    outward,
    tangent,
    footprint,
):
    """Continue a translated section endpoint to its straight stair rail."""

    original_vertex = Part.Vertex(
        FreeCAD.Vector(original[0], original[1], 0.0)
    )
    tolerance = max(footprint.BoundBox.DiagonalLength * 1e-7, 1e-6)
    candidates = []
    for edge in footprint.OuterWire.Edges:
        if edge.distToShape(original_vertex)[0] > tolerance:
            continue
        middle = (edge.FirstParameter + edge.LastParameter) / 2.0
        if abs(edge.curvatureAt(middle)) > 1e-9:
            continue
        vertices = edge.Vertexes
        if len(vertices) < 2:
            continue
        first = (vertices[0].Point.x, vertices[0].Point.y)
        second = (vertices[-1].Point.x, vertices[-1].Point.y)
        rail = (second[0] - first[0], second[1] - first[1])
        rail_length = math.hypot(rail[0], rail[1])
        if rail_length < 1e-9:
            continue
        rail = (rail[0] / rail_length, rail[1] / rail_length)
        if abs(rail[0] * tangent[0] + rail[1] * tangent[1]) < 1e-7:
            continue
        intersection = _line_intersection(shifted, outward, first, rail)
        if intersection is None:
            continue
        ray_parameter = (
            (intersection[0] - shifted[0]) * outward[0]
            + (intersection[1] - shifted[1]) * outward[1]
        )
        rail_parameter = (
            (intersection[0] - first[0]) * rail[0]
            + (intersection[1] - first[1]) * rail[1]
        )
        if (
            ray_parameter >= -tolerance
            and -tolerance <= rail_parameter <= rail_length + tolerance
        ):
            candidates.append((max(ray_parameter, 0.0), intersection))
    if not candidates:
        return shifted
    return min(candidates, key=lambda candidate: candidate[0])[1]


def _section_band_faces(section, footprint, distance):
    """Return the footprint band between a section and its parallel offset."""

    if abs(distance) < 1e-9:
        return []
    shifted_center = _shifted(section.center, section.tangent, distance)
    if distance > 0.0:
        start = section.center
        end = shifted_center
    else:
        start = shifted_center
        end = section.center
    extent = max(
        footprint.BoundBox.DiagonalLength * 4.0,
        abs(distance) + 1000.0,
    )
    after_start = _half_plane_face(start, section.tangent, extent)
    before_end = _half_plane_face(
        end,
        (-section.tangent[0], -section.tangent[1]),
        extent,
    )
    clipped = footprint.common(after_start).common(before_end)
    candidates = clipped.Faces
    if not candidates:
        return []

    probe = FreeCAD.Vector(
        (section.center[0] + shifted_center[0]) / 2.0,
        (section.center[1] + shifted_center[1]) / 2.0,
        0.0,
    )
    for face in candidates:
        if face.isInside(probe, 1e-6, True):
            return [face]
    point = Part.Vertex(probe)
    return [min(candidates, key=lambda face: face.distToShape(point)[0])]


def _translated_section(section, distance):
    """Return a section translated along its local walking direction."""

    return BalancedSection(
        center=_shifted(section.center, section.tangent, distance),
        tangent=section.tangent,
        left=_shifted(section.left, section.tangent, distance),
        right=_shifted(section.right, section.tangent, distance),
        station=section.station + distance,
        width=section.width,
        flight_index=section.flight_index,
        landing_to_next=section.landing_to_next,
        locked_to_flight=section.locked_to_flight,
        level_to_next=section.level_to_next,
        riser_index=section.riser_index,
    )


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


def make_balanced_riser_shape(
    section,
    base_elevation,
    height,
    thickness,
    footprint=None,
    local_expansion=False,
):
    """Create one vertical riser following a balanced nosing section."""

    if footprint is None:
        rear_left = _shifted(section.left, section.tangent, thickness)
        rear_right = _shifted(section.right, section.tangent, thickness)
        faces = [
            _horizontal_face(
                (section.left, section.right, rear_right, rear_left),
                base_elevation,
            )
        ]
    else:
        faces = _local_step_expansion_faces(
            section,
            section,
            footprint,
            0.0,
            thickness,
            local_expansion,
        )

    solids = []
    for face in faces:
        placed_face = face.copy()
        placed_face.translate(FreeCAD.Vector(0.0, 0.0, base_elevation))
        solids.append(
            placed_face.extrude(
                FreeCAD.Vector(0.0, 0.0, max(height, 0.01))
            )
        )
    result = solids[0]
    for solid in solids[1:]:
        result = result.fuse(solid)
    return result.removeSplitter()


def make_balanced_concrete_shape(
    sections,
    footprint,
    riser_height,
    thickness,
    plan_shapes=None,
):
    """Create a monolithic stepped concrete stair along balanced sections."""

    if len(sections) < 2:
        return Part.Shape()
    going = max(sections[1].station - sections[0].station, 0.01)
    slope = math.atan(riser_height / going)
    slope_cosine = max(math.cos(slope), 0.01)
    minimum_waist = riser_height * slope_cosine
    concrete_thickness = max(float(thickness), 0.01)
    effective_waist = minimum_waist + concrete_thickness
    vertical_waist = effective_waist / slope_cosine

    top_elevations = [
        balanced_section_top(section, index, riser_height)
        for index, section in enumerate(sections)
    ]
    bottom_elevations = [
        max(top - vertical_waist, 0.0) for top in top_elevations
    ]
    bottom_fronts = bottom_elevations[:-1]
    bottom_rears = bottom_elevations[1:]
    if bottom_rears:
        bottom_rears[-1] = max(
            top_elevations[-2] - concrete_thickness, 0.0
        )
    for index, section in enumerate(sections[:-1]):
        if section.landing_to_next:
            landing_top = top_elevations[index]
            landing_bottom = max(
                landing_top - concrete_thickness, 0.0
            )
            bottom_fronts[index] = landing_bottom
            bottom_rears[index] = landing_bottom
            if index + 1 < len(bottom_fronts):
                bottom_fronts[index + 1] = max(
                    bottom_fronts[index + 1], landing_bottom
                )

    _align_straight_concrete_bottoms(
        sections, top_elevations, bottom_fronts, bottom_rears
    )

    solids = []
    if plan_shapes is None:
        plan_shapes = balanced_tread_faces(sections, footprint)
    circular_profiles = []
    for front, rear, plan_shape in zip(
        sections, sections[1:], plan_shapes
    ):
        profile = None
        if len(plan_shape.Faces) == 1:
            profile = _circular_profile_data(
                plan_shape.Faces[0], front, rear
            )
        circular_profiles.append(profile)

    index = 0
    while index < len(plan_shapes):
        if sections[index].landing_to_next:
            landing_top = top_elevations[index]
            landing_bottom = bottom_fronts[index]
            landing_solids = []
            for plan_face in plan_shapes[index].Faces:
                bottom_face = plan_face.copy()
                bottom_face.translate(
                    FreeCAD.Vector(0.0, 0.0, landing_bottom)
                )
                landing_solids.append(
                    bottom_face.extrude(
                        FreeCAD.Vector(
                            0.0, 0.0, landing_top - landing_bottom
                        )
                    )
                )

            incoming_bottom = bottom_elevations[index]
            predecessor_reaches_landing = (
                index > 0
                and abs(top_elevations[index - 1] - landing_top) < 1e-7
            )
            if (
                not predecessor_reaches_landing
                and landing_bottom - incoming_bottom > 1e-7
            ):
                joint_depth = min(
                    0.1, max(footprint.BoundBox.DiagonalLength * 1e-6, 0.01)
                )
                for joint_face in _section_band_faces(
                    sections[index], plan_shapes[index], joint_depth
                ):
                    bottom_face = joint_face.copy()
                    bottom_face.translate(
                        FreeCAD.Vector(0.0, 0.0, incoming_bottom)
                    )
                    landing_solids.append(
                        bottom_face.extrude(
                            FreeCAD.Vector(
                                0.0, 0.0, landing_top - incoming_bottom
                            )
                        )
                    )

            landing = landing_solids[0]
            for addition in landing_solids[1:]:
                landing = landing.fuse(addition)
            solids.append(landing.removeSplitter())
            index += 1
            continue

        profile = circular_profiles[index]
        span_end = index
        if (
            profile is not None
            and abs(
                bottom_rears[index] - bottom_fronts[index]
            )
            > 1e-7
        ):
            pitch = (
                (bottom_rears[index] - bottom_fronts[index]) / profile.sweep
            )
            while span_end + 1 < len(plan_shapes):
                following = circular_profiles[span_end + 1]
                if following is None or not _circular_profiles_join(
                    circular_profiles[span_end], following
                ):
                    break
                following_pitch = (
                    bottom_rears[span_end + 1]
                    - bottom_fronts[span_end + 1]
                ) / following.sweep
                if abs(following_pitch - pitch) > max(abs(pitch), 1.0) * 1e-6:
                    break
                span_end += 1

        if span_end > index:
            circular = _make_circular_concrete_span(
                sections,
                plan_shapes,
                circular_profiles,
                index,
                span_end,
                riser_height,
                bottom_fronts,
                bottom_rears,
                top_elevations,
            )
            if circular is not None:
                solids.append(circular)
                index = span_end + 1
                continue

        front = sections[index]
        rear = sections[index + 1]
        plan_shape = plan_shapes[index]
        top = top_elevations[index]
        bottom_front = bottom_fronts[index]
        bottom_rear = bottom_rears[index]
        tread_solids = [
            _make_profiled_plan_solid(
                plan_face, front, rear, top, bottom_front, bottom_rear
            )
            for plan_face in plan_shape.Faces
        ]
        tread = tread_solids[0]
        for tread_solid in tread_solids[1:]:
            tread = tread.fuse(tread_solid)
        solids.append(tread.removeSplitter())
        index += 1

    result = solids[0]
    for solid in solids[1:]:
        result = result.fuse(solid)
    return result.removeSplitter()


def _align_straight_concrete_bottoms(
    sections, top_elevations, bottom_fronts, bottom_rears
):
    """Make each straight concrete run use one continuous soffit plane."""

    cell_count = min(len(bottom_fronts), len(sections) - 1)

    def is_straight_cell(index):
        front = sections[index]
        rear = sections[index + 1]
        if front.landing_to_next or front.level_to_next:
            return False
        cross = abs(_cross(front.tangent, rear.tangent))
        dot = (
            front.tangent[0] * rear.tangent[0]
            + front.tangent[1] * rear.tangent[1]
        )
        return cross < 1e-7 and dot > 0.0

    index = 0
    while index < cell_count:
        if not is_straight_cell(index):
            index += 1
            continue

        run_start = index
        flight_index = sections[index].flight_index
        run_tangent = sections[index].tangent
        index += 1
        while index < cell_count:
            if not is_straight_cell(index):
                break
            front = sections[index]
            if front.flight_index != flight_index:
                break
            if abs(_cross(run_tangent, front.tangent)) >= 1e-7:
                break
            index += 1
        run_end = index - 1

        start_bottom = bottom_fronts[run_start]
        if run_start > 0 and sections[run_start - 1].landing_to_next:
            start_bottom = bottom_rears[run_start - 1]

        end_bottom = bottom_rears[run_end]
        if (
            run_end + 1 < cell_count
            and sections[run_end + 1].landing_to_next
            and abs(
                top_elevations[run_end]
                - top_elevations[run_end + 1]
            )
            < 1e-7
        ):
            end_bottom = bottom_fronts[run_end + 1]

        smooth_start = run_start
        if (
            run_start == 0
            and run_end > run_start
            and abs(start_bottom) < 1e-7
        ):
            bottom_fronts[run_start] = 0.0
            bottom_rears[run_start] = 0.0
            smooth_start += 1
            start_bottom = 0.0

        if smooth_start > run_end:
            continue

        distances = [0.0]
        for boundary in range(smooth_start, run_end + 1):
            first = sections[boundary].center
            second = sections[boundary + 1].center
            distances.append(
                distances[-1]
                + math.hypot(second[0] - first[0], second[1] - first[1])
            )
        total_distance = distances[-1]
        if total_distance < 1e-9:
            continue

        for offset, cell_index in enumerate(
            range(smooth_start, run_end + 1)
        ):
            front_ratio = distances[offset] / total_distance
            rear_ratio = distances[offset + 1] / total_distance
            bottom_fronts[cell_index] = start_bottom + (
                end_bottom - start_bottom
            ) * front_ratio
            bottom_rears[cell_index] = start_bottom + (
                end_bottom - start_bottom
            ) * rear_ratio


def balanced_section_top(section, index, riser_height):
    """Return the tread-top elevation represented by ``section``."""

    riser_index = int(getattr(section, "riser_index", 0))
    if riser_index <= 0 and not getattr(section, "level_to_next", False):
        riser_index = index + 1
    return riser_index * float(riser_height)


def _make_profiled_plan_solid(
    plan_face,
    front,
    rear,
    top_elevation,
    bottom_front,
    bottom_rear,
):
    """Build a square-edged tread volume above a locally sloping underside."""

    if abs(bottom_rear - bottom_front) < 1e-7:
        bottom_face = plan_face.copy()
        bottom_face.translate(FreeCAD.Vector(0.0, 0.0, bottom_front))
        return bottom_face.extrude(
            FreeCAD.Vector(0.0, 0.0, top_elevation - bottom_front)
        )

    helical = _make_helical_profiled_plan_solid(
        plan_face,
        front,
        rear,
        top_elevation,
        bottom_front,
        bottom_rear,
    )
    if helical is not None:
        return helical

    mesh_points, facets = plan_face.tessellate(0.1)

    def bottom_elevation(point):
        front_distance = (
            (point.x - front.center[0]) * front.tangent[0]
            + (point.y - front.center[1]) * front.tangent[1]
        )
        rear_distance = -(
            (point.x - rear.center[0]) * rear.tangent[0]
            + (point.y - rear.center[1]) * rear.tangent[1]
        )
        denominator = front_distance + rear_distance
        ratio = front_distance / denominator if abs(denominator) > 1e-9 else 0.5
        ratio = min(max(ratio, 0.0), 1.0)
        return bottom_front + (bottom_rear - bottom_front) * ratio

    top_points = [
        FreeCAD.Vector(point.x, point.y, top_elevation) for point in mesh_points
    ]
    bottom_points = [
        FreeCAD.Vector(point.x, point.y, bottom_elevation(point))
        for point in mesh_points
    ]
    faces = []
    oriented_facets = []
    for first, second, third in facets:
        top_triangle = (top_points[first], top_points[second], top_points[third])
        cross = (top_triangle[1] - top_triangle[0]).cross(
            top_triangle[2] - top_triangle[0]
        )
        if cross.z < 0.0:
            second, third = third, second
            top_triangle = (top_triangle[0], top_triangle[2], top_triangle[1])
        oriented_facets.append((first, second, third))
        faces.append(_triangle_face(*top_triangle))
        faces.append(
            _triangle_face(
                bottom_points[first],
                bottom_points[third],
                bottom_points[second],
            )
        )

    boundary_edges = {}
    for first, second, third in oriented_facets:
        for start, end in ((first, second), (second, third), (third, first)):
            key = tuple(sorted((start, end)))
            if key in boundary_edges:
                del boundary_edges[key]
            else:
                boundary_edges[key] = (start, end)
    for first, second in boundary_edges.values():
        faces.append(
            _triangle_face(
                bottom_points[first],
                bottom_points[second],
                top_points[second],
            )
        )
        faces.append(
            _triangle_face(
                bottom_points[first],
                top_points[second],
                top_points[first],
            )
        )
    return Part.makeSolid(Part.makeShell(faces))


def _make_helical_profiled_plan_solid(
    plan_face,
    front,
    rear,
    top_elevation,
    bottom_front,
    bottom_rear,
):
    """Build an annular tread cell with an exact OCC helical underside."""

    profile = _circular_profile_data(plan_face, front, rear)
    if profile is None:
        return None
    return _make_helical_annular_solid(
        profile,
        top_elevation,
        bottom_front,
        bottom_rear,
    )


def _circular_profile_data(plan_face, front, rear):
    """Return exact circle data when ``plan_face`` is an annular sector."""

    curved_edges = []
    for edge in plan_face.Edges:
        parameter = (edge.FirstParameter + edge.LastParameter) / 2.0
        if abs(edge.curvatureAt(parameter)) > 1e-9:
            curved_edges.append(edge)
    if len(curved_edges) != 2:
        return None

    front_direction = (
        front.left[0] - front.right[0],
        front.left[1] - front.right[1],
    )
    rear_direction = (
        rear.left[0] - rear.right[0],
        rear.left[1] - rear.right[1],
    )
    denominator = _cross(front_direction, rear_direction)
    if abs(denominator) < 1e-9:
        return None
    relative = (
        rear.center[0] - front.center[0],
        rear.center[1] - front.center[1],
    )
    front_parameter = _cross(relative, rear_direction) / denominator
    circle_center = (
        front.center[0] + front_direction[0] * front_parameter,
        front.center[1] + front_direction[1] * front_parameter,
    )
    front_radial = (
        front.center[0] - circle_center[0],
        front.center[1] - circle_center[1],
    )
    rear_radial = (
        rear.center[0] - circle_center[0],
        rear.center[1] - circle_center[1],
    )
    radial_dot = (
        front_radial[0] * rear_radial[0]
        + front_radial[1] * rear_radial[1]
    )
    sweep = math.atan2(_cross(front_radial, rear_radial), radial_dot)
    if abs(sweep) < 1e-7:
        return None

    def radii(section):
        return sorted(
            math.hypot(
                point[0] - circle_center[0], point[1] - circle_center[1]
            )
            for point in (section.left, section.right)
        )

    front_radii = radii(front)
    rear_radii = radii(rear)
    tolerance = max(front.width, rear.width, 1.0) * 1e-6
    if any(
        abs(first - second) > tolerance
        for first, second in zip(front_radii, rear_radii)
    ):
        return None
    inner_radius = (front_radii[0] + rear_radii[0]) / 2.0
    outer_radius = (front_radii[1] + rear_radii[1]) / 2.0
    expected_area = (
        0.5
        * (outer_radius * outer_radius - inner_radius * inner_radius)
        * abs(sweep)
    )
    if abs(plan_face.Area - expected_area) > max(expected_area * 1e-6, 0.01):
        return None

    return _CircularProfile(
        circle_center,
        inner_radius,
        outer_radius,
        math.atan2(front_radial[1], front_radial[0]),
        sweep,
    )


def _circular_profiles_join(first, second):
    """Return whether two annular cells belong to one circular flight."""

    tolerance = max(first.outer_radius, second.outer_radius, 1.0) * 1e-6
    if math.hypot(
        first.center[0] - second.center[0],
        first.center[1] - second.center[1],
    ) > tolerance:
        return False
    if abs(first.inner_radius - second.inner_radius) > tolerance:
        return False
    if abs(first.outer_radius - second.outer_radius) > tolerance:
        return False
    if first.sweep * second.sweep <= 0.0:
        return False
    first_end = first.start_angle + first.sweep
    angle_difference = math.atan2(
        math.sin(second.start_angle - first_end),
        math.cos(second.start_angle - first_end),
    )
    return abs(angle_difference) < 1e-6


def _make_circular_concrete_span(
    sections,
    plan_shapes,
    profiles,
    start_index,
    end_index,
    riser_height,
    bottom_fronts,
    bottom_rears,
    top_elevations,
):
    """Build one circular flight with two cylinders and one helical soffit."""

    first = profiles[start_index]
    sweep = sum(
        profiles[index].sweep
        for index in range(start_index, end_index + 1)
    )
    bottom_front = bottom_fronts[start_index]
    bottom_rear = bottom_rears[end_index]
    base_elevation = min(bottom_front, bottom_rear) - max(riser_height, 1.0)
    envelope_solids = []
    for index in range(start_index, end_index + 1):
        top = top_elevations[index]
        for plan_face in plan_shapes[index].Faces:
            placed_face = plan_face.copy()
            placed_face.translate(
                FreeCAD.Vector(0.0, 0.0, base_elevation)
            )
            envelope_solids.append(
                placed_face.extrude(
                    FreeCAD.Vector(0.0, 0.0, top - base_elevation)
                )
            )
    envelope = envelope_solids[0]
    for solid in envelope_solids[1:]:
        envelope = envelope.fuse(solid)
    envelope = envelope.removeSplitter()

    radial_margin = max(first.outer_radius - first.inner_radius, 1.0) * 1e-3
    angular_margin = 1e-4
    direction = 1.0 if sweep > 0.0 else -1.0
    height_per_angle = (bottom_rear - bottom_front) / abs(sweep)
    expanded = _CircularProfile(
        first.center,
        max(first.inner_radius - radial_margin, 0.01),
        first.outer_radius + radial_margin,
        first.start_angle - direction * angular_margin,
        sweep + direction * 2.0 * angular_margin,
    )
    profiled_envelope = _make_helical_annular_solid(
        expanded,
        top_elevations[end_index] + max(riser_height, 1.0),
        bottom_front - height_per_angle * angular_margin,
        bottom_rear + height_per_angle * angular_margin,
    )
    if profiled_envelope is None:
        return None
    result = envelope.common(profiled_envelope).removeSplitter()
    if result.isValid() and len(result.Solids) == 1:
        return result
    return None


def _make_helical_annular_solid(
    profile,
    top_elevation,
    bottom_front,
    bottom_rear,
):
    """Build an annular solid between a horizontal top and helical bottom."""
    return _make_helical_band_solid(
        profile,
        top_elevation,
        top_elevation,
        bottom_front,
        bottom_rear,
    )


def _triangle_face(first, second, third):
    return Part.Face(Part.makePolygon([first, second, third, first]))


def _make_triangulated_solid(top_points, bottom_points):
    """Make a solid whose potentially warped quadrilateral faces are triangulated."""

    def triangle(first, second, third):
        return Part.Face(Part.makePolygon([first, second, third, first]))

    faces = [
        triangle(top_points[0], top_points[1], top_points[2]),
        triangle(top_points[0], top_points[2], top_points[3]),
        triangle(bottom_points[0], bottom_points[2], bottom_points[1]),
        triangle(bottom_points[0], bottom_points[3], bottom_points[2]),
    ]
    for index in range(4):
        following = (index + 1) % 4
        faces.extend(
            (
                triangle(
                    bottom_points[index],
                    bottom_points[following],
                    top_points[following],
                ),
                triangle(
                    bottom_points[index],
                    top_points[following],
                    top_points[index],
                ),
            )
        )
    return Part.makeSolid(Part.makeShell(faces))


def balanced_plan_segments(sections, footprint):
    """Return the fixed flight boundary and balanced nosings."""

    segments = []
    outline_edges = {}
    for face in footprint.Faces:
        for edge in face.OuterWire.Edges:
            if len(edge.Vertexes) >= 2:
                first = edge.Vertexes[0].Point
                last = edge.Vertexes[-1].Point
                endpoints = sorted(
                    (
                        (round(first.x, 7), round(first.y, 7)),
                        (round(last.x, 7), round(last.y, 7)),
                    )
                )
                key = tuple(endpoints)
                outline_edges.setdefault(key, []).append(
                    ((first.x, first.y), (last.x, last.y))
                )
    for matches in outline_edges.values():
        if len(matches) == 1:
            segments.append(matches[0])
    for section in sections:
        segments.append((section.left, section.right))
    return segments


def balanced_plan_geometry(sections, footprint):
    """Return bounded line/arc geometry for the generated plan sketch."""

    result = []
    seen = set()
    for face in footprint.Faces:
        for edge in face.OuterWire.Edges:
            first = edge.valueAt(edge.FirstParameter)
            last = edge.valueAt(edge.LastParameter)
            endpoints = tuple(
                sorted(
                    (
                        (round(first.x, 7), round(first.y, 7)),
                        (round(last.x, 7), round(last.y, 7)),
                    )
                )
            )
            key = endpoints, round(edge.Length, 7)
            if key in seen:
                continue
            seen.add(key)
            middle = edge.valueAt(
                (edge.FirstParameter + edge.LastParameter) / 2.0
            )
            if abs(
                edge.curvatureAt(
                    (edge.FirstParameter + edge.LastParameter) / 2.0
                )
            ) > 1e-9:
                result.append(Part.Arc(first, middle, last))
            else:
                result.append(Part.LineSegment(first, last))
    result.extend(
        Part.LineSegment(
            FreeCAD.Vector(section.left[0], section.left[1], 0.0),
            FreeCAD.Vector(section.right[0], section.right[1], 0.0),
        )
        for section in sections
    )
    return result


def straight_stringer_sections(metrics, width):
    """Return balanced-section compatible stations for one straight flight."""

    sections = []
    width = max(float(width), 0.01)
    for index in range(metrics.tread_count + 1):
        station = index * metrics.tread_width
        sections.append(
            BalancedSection(
                center=(station, width / 2.0),
                tangent=(1.0, 0.0),
                left=(station, width),
                right=(station, 0.0),
                station=station,
                width=width,
                flight_index=0,
                riser_index=index + 1,
            )
        )
    return sections


def _stringer_inward(section, side):
    rail = section.left if side == "Left" else section.right
    inward = (
        section.center[0] - rail[0],
        section.center[1] - rail[1],
    )
    length = math.hypot(inward[0], inward[1])
    if length < 1e-9:
        if side == "Left":
            return (section.tangent[1], -section.tangent[0])
        return (-section.tangent[1], section.tangent[0])
    return (inward[0] / length, inward[1] / length)


def _stringer_cross_section(
    section,
    side,
    bottom,
    top,
    thickness,
    offset,
    housed,
):
    rail = section.left if side == "Left" else section.right
    inward = _stringer_inward(section, side)
    first = (
        rail[0] + inward[0] * offset,
        rail[1] + inward[1] * offset,
    )
    thickness_direction = -1.0 if housed else 1.0
    second = (
        first[0] + inward[0] * thickness * thickness_direction,
        first[1] + inward[1] * thickness * thickness_direction,
    )
    top = max(float(top), float(bottom) + 0.01)
    points = (
        FreeCAD.Vector(first[0], first[1], bottom),
        FreeCAD.Vector(second[0], second[1], bottom),
        FreeCAD.Vector(second[0], second[1], top),
        FreeCAD.Vector(first[0], first[1], top),
    )
    return Part.makePolygon((*points, points[0]))


def _monotone_profile_slopes(parameters, values):
    """Return shape-preserving cubic slopes for ordered profile points."""

    count = len(parameters)
    if count < 2:
        return [0.0] * count
    intervals = [
        parameters[index + 1] - parameters[index]
        for index in range(count - 1)
    ]
    secants = [
        (values[index + 1] - values[index]) / intervals[index]
        for index in range(count - 1)
    ]
    if count == 2:
        return [secants[0], secants[0]]

    slopes = [0.0] * count
    for index in range(1, count - 1):
        previous = secants[index - 1]
        following = secants[index]
        if previous * following <= 0.0:
            continue
        previous_interval = intervals[index - 1]
        following_interval = intervals[index]
        first_weight = 2.0 * following_interval + previous_interval
        second_weight = following_interval + 2.0 * previous_interval
        slopes[index] = (first_weight + second_weight) / (
            first_weight / previous + second_weight / following
        )

    def endpoint_slope(first_interval, second_interval, first, second):
        value = (
            (2.0 * first_interval + second_interval) * first
            - first_interval * second
        ) / (first_interval + second_interval)
        if value * first <= 0.0:
            return 0.0
        if first * second < 0.0 and abs(value) > 3.0 * abs(first):
            return 3.0 * first
        return value

    slopes[0] = endpoint_slope(
        intervals[0], intervals[1], secants[0], secants[1]
    )
    slopes[-1] = endpoint_slope(
        intervals[-1],
        intervals[-2],
        secants[-1],
        secants[-2],
    )
    return slopes


def _profile_bezier_edges(
    origin,
    direction,
    parameters,
    values,
):
    """Create a non-overshooting piecewise-cubic profile."""

    slopes = _monotone_profile_slopes(parameters, values)

    def point(parameter, elevation):
        return FreeCAD.Vector(
            origin[0] + direction[0] * parameter,
            origin[1] + direction[1] * parameter,
            elevation,
        )

    segment_poles = []
    for index in range(len(parameters) - 1):
        first_parameter = parameters[index]
        second_parameter = parameters[index + 1]
        interval = second_parameter - first_parameter
        poles = (
            point(first_parameter, values[index]),
            point(
                first_parameter + interval / 3.0,
                values[index] + slopes[index] * interval / 3.0,
            ),
            point(
                second_parameter - interval / 3.0,
                values[index + 1]
                - slopes[index + 1] * interval / 3.0,
            ),
            point(second_parameter, values[index + 1]),
        )
        segment_poles.append(poles)

    poles = list(segment_poles[0])
    for segment in segment_poles[1:]:
        poles.extend(segment[1:])
    segment_count = len(segment_poles)
    curve = Part.BSplineCurve()
    curve.buildFromPolesMultsKnots(
        poles,
        [4, *([3] * (segment_count - 1)), 4],
        [float(index) for index in range(segment_count + 1)],
        False,
        3,
    )
    return [curve.toShape()]


def _make_planar_housed_stringer_shape(
    sections,
    tops,
    side,
    thickness,
    penetration,
    vertical_width,
):
    """Extrude a stable, shape-preserving side profile for a flat board."""

    if len(sections) < 2 or len(sections) != len(tops):
        return None
    first_tangent = sections[0].tangent
    tangent_length = math.hypot(*first_tangent)
    if tangent_length < 1e-9:
        return None
    tangent = (
        first_tangent[0] / tangent_length,
        first_tangent[1] / tangent_length,
    )
    first_inward = _stringer_inward(sections[0], side)
    surface_points = []
    for section in sections:
        section_tangent = section.tangent
        section_length = math.hypot(*section_tangent)
        if section_length < 1e-9:
            return None
        section_tangent = (
            section_tangent[0] / section_length,
            section_tangent[1] / section_length,
        )
        if (
            abs(_cross(tangent, section_tangent)) > 1e-7
            or _dot(tangent, section_tangent) < 0.0
        ):
            return None
        inward = _stringer_inward(section, side)
        if _dot(first_inward, inward) < 1.0 - 1e-7:
            return None
        rail = section.left if side == "Left" else section.right
        surface_points.append(
            (
                rail[0] + inward[0] * float(penetration),
                rail[1] + inward[1] * float(penetration),
            )
        )

    axis = (
        surface_points[-1][0] - surface_points[0][0],
        surface_points[-1][1] - surface_points[0][1],
    )
    axis_length = math.hypot(*axis)
    if axis_length < 1e-7:
        return None
    direction = (axis[0] / axis_length, axis[1] / axis_length)
    if _dot(direction, tangent) < 0.0:
        direction = (-direction[0], -direction[1])
    normal = (-direction[1], direction[0])
    origin = surface_points[0]
    tolerance = max(axis_length, 1.0) * 1e-7
    if any(
        abs(
            (point[0] - origin[0]) * normal[0]
            + (point[1] - origin[1]) * normal[1]
        )
        > tolerance
        for point in surface_points
    ):
        return None

    parameters = [
        (point[0] - origin[0]) * direction[0]
        + (point[1] - origin[1]) * direction[1]
        for point in surface_points
    ]
    if parameters[-1] < 1e-7:
        return None
    if any(
        following <= previous + 1e-7
        for previous, following in zip(parameters, parameters[1:])
    ):
        # Aggressive winding can make the tread intersections backtrack
        # along a straight board.  A global loft then produces loops.  Keep
        # their distribution but restore the only physically meaningful
        # ordering before constructing the side profile.
        total = parameters[-1]
        ordered = sorted(
            min(max(parameter, 0.0), total)
            for parameter in parameters
        )
        blend = 1e-5
        parameters = [
            (1.0 - blend) * parameter
            + blend * total * index / (len(ordered) - 1)
            for index, parameter in enumerate(ordered)
        ]
        parameters[0] = 0.0
        parameters[-1] = total

    top_values = [float(top) for top in tops]
    bottom_values = [
        max(top - float(vertical_width), 0.0) for top in top_values
    ]
    top_edges = _profile_bezier_edges(
        origin, direction, parameters, top_values
    )
    bottom_edges = _profile_bezier_edges(
        origin, direction, parameters, bottom_values
    )
    top_start = top_edges[0].Vertexes[0].Point
    top_end = top_edges[-1].Vertexes[-1].Point
    bottom_start = bottom_edges[0].Vertexes[0].Point
    bottom_end = bottom_edges[-1].Vertexes[-1].Point
    reversed_bottom = []
    for edge in reversed(bottom_edges):
        reversed_edge = edge.copy()
        reversed_edge.reverse()
        reversed_bottom.append(reversed_edge)
    try:
        profile = Part.Wire(
            [
                *top_edges,
                Part.makeLine(top_end, bottom_end),
                *reversed_bottom,
                Part.makeLine(bottom_start, top_start),
            ]
        )
        face = Part.Face(profile)
        result = face.extrude(
            FreeCAD.Vector(
                -first_inward[0] * float(thickness),
                -first_inward[1] * float(thickness),
                0.0,
            )
        )
    except (Part.OCCError, ValueError):
        return None
    if result.isNull() or not result.isValid() or len(result.Solids) != 1:
        return None
    try:
        return result.removeSplitter()
    except Part.OCCError:
        return result


def _stringer_elevations(sections, riser_height):
    return [
        balanced_section_top(section, index, riser_height)
        for index, section in enumerate(sections)
    ]


def _stringer_slope(sections, elevations):
    if len(sections) < 2:
        return 0.0
    run = 0.0
    rise = 0.0
    for first, second, first_top, second_top in zip(
        sections, sections[1:], elevations, elevations[1:]
    ):
        elevation_change = second_top - first_top
        if abs(elevation_change) < 1e-9:
            continue
        section_run = math.hypot(
            second.center[0] - first.center[0],
            second.center[1] - first.center[1],
        )
        if section_run < 1e-9:
            section_run = max(second.station - first.station, 0.0)
        run += section_run
        rise += elevation_change
    if run < 1e-9:
        return 0.0
    return rise / run


def automatic_stringer_width(
    riser_height,
    going,
    step_thickness,
    nosing,
    nosing_offset,
    offset_direction="Perpendicular",
):
    """Return a practical board width with 50 mm below the step profile."""

    going = max(float(going), 0.01)
    slope = math.atan(max(float(riser_height), 0.0) / going)
    slope_cosine = max(math.cos(slope), 0.01)
    slope_sine = math.sin(slope)
    upper = max(float(nosing_offset), 0.0)
    if str(offset_direction) == "Vertical":
        upper *= slope_cosine
    step_envelope = (
        (max(float(riser_height), 0.0) + max(float(step_thickness), 0.0))
        * slope_cosine
        + max(float(nosing), 0.0) * slope_sine
    )
    return max(235.0, upper + step_envelope + 50.0)


def stringer_flight_runs(sections, flight_types=None):
    """Return ``(flight_index, sections)`` for each stair-bearing flight."""

    sections = [
        replace(section, riser_index=index + 1)
        if int(getattr(section, "riser_index", 0)) <= 0
        and not section.level_to_next
        else section
        for index, section in enumerate(sections)
    ]
    runs = []
    start = None
    flight_index = None
    for index, section in enumerate(sections[:-1]):
        if section.level_to_next:
            if start is not None and index > start:
                runs.append((flight_index, sections[start : index + 1]))
            section_type = (
                str(flight_types[section.flight_index])
                if flight_types
                and section.flight_index < len(flight_types)
                else ""
            )
            if section_type.endswith("landing"):
                runs.append(
                    (
                        section.flight_index,
                        [
                            replace(section, level_to_next=False),
                            replace(
                                sections[index + 1],
                                level_to_next=False,
                            ),
                        ],
                    )
                )
            start = None
            flight_index = None
            continue
        if start is None:
            start = index
            flight_index = section.flight_index
        elif section.flight_index != flight_index:
            previous_type = (
                str(flight_types[flight_index])
                if flight_types and flight_index < len(flight_types)
                else ""
            )
            following_type = (
                str(flight_types[section.flight_index])
                if flight_types and section.flight_index < len(flight_types)
                else ""
            )
            tangent_junction = (
                previous_type.startswith("Circular")
                or following_type.startswith("Circular")
            )
            if tangent_junction and index - start >= 2:
                runs.append((flight_index, sections[start:index]))
                start = index - 1
            else:
                runs.append((flight_index, sections[start : index + 1]))
                start = index
            flight_index = section.flight_index
    if start is not None and len(sections) - start >= 2:
        runs.append((flight_index, sections[start:]))
    return runs


def planar_stringer_sections(
    sections,
    side,
    origin,
    heading,
    width,
    start_seam=None,
    end_seam=None,
):
    """Project a straight-flight stringer onto one vertical board plane.

    Winding changes the longitudinal positions and elevations of the section
    cuts, but it must not bend the board in plan. The resulting upper and
    lower profiles can curve within the flat side face of the plank.
    """

    radians = math.radians(float(heading))
    tangent = (math.cos(radians), math.sin(radians))
    normal = (-tangent[1], tangent[0])
    width = max(float(width), 0.01)
    right_origin = (float(origin[0]), float(origin[1]))
    left_origin = (
        right_origin[0] + normal[0] * width,
        right_origin[1] + normal[1] * width,
    )
    selected_origin = left_origin if side == "Left" else right_origin

    def longitudinal(point):
        return (
            (point[0] - selected_origin[0]) * tangent[0]
            + (point[1] - selected_origin[1]) * tangent[1]
        )

    distances = [
        longitudinal(section.left if side == "Left" else section.right)
        for section in sections
    ]
    if start_seam is not None:
        distances[0] = longitudinal(start_seam)
    if end_seam is not None:
        distances[-1] = longitudinal(end_seam)

    result = []
    for section, distance in zip(sections, distances):
        right = (
            right_origin[0] + tangent[0] * distance,
            right_origin[1] + tangent[1] * distance,
        )
        left = (
            right[0] + normal[0] * width,
            right[1] + normal[1] * width,
        )
        center = (
            right[0] + normal[0] * width / 2.0,
            right[1] + normal[1] * width / 2.0,
        )
        result.append(
            replace(
                section,
                center=center,
                tangent=tangent,
                left=left,
                right=right,
            )
        )
    return result


def _stringer_section_runs(sections):
    """Split stringers at explicit level landing cells.

    The cross-sections on either side of a landing describe the incoming and
    outgoing flights. Lofting directly between them would twist a board across
    the entire landing, so each adjacent stair run must remain a separate
    manufactured part.
    """

    runs = []
    start = 0
    for index, section in enumerate(sections[:-1]):
        if not section.level_to_next:
            continue
        if index - start >= 1:
            runs.append(sections[start : index + 1])
        start = index + 1
    if len(sections) - start >= 2:
        runs.append(sections[start:])
    return runs


def _circular_stringer_data(
    sections,
    side,
    thickness,
    lateral_offset,
    housed,
):
    """Return exact coaxial radii and unwrapped angles for a curved board."""

    if len(sections) < 2:
        return None
    radial_lines = []
    for section in sections:
        direction = (
            section.left[0] - section.right[0],
            section.left[1] - section.right[1],
        )
        length = math.hypot(*direction)
        if length < 1e-9:
            return None
        radial_lines.append(
            (
                section.center,
                (direction[0] / length, direction[1] / length),
            )
        )

    best_pair = None
    best_cross = 0.0
    for first_index, first in enumerate(radial_lines[:-1]):
        for second_index in range(first_index + 1, len(radial_lines)):
            second = radial_lines[second_index]
            magnitude = abs(_cross(first[1], second[1]))
            if magnitude > best_cross:
                best_cross = magnitude
                best_pair = first, second
    if best_pair is None or best_cross < 1e-6:
        return None

    first_line, second_line = best_pair
    denominator = _cross(first_line[1], second_line[1])
    relative = (
        second_line[0][0] - first_line[0][0],
        second_line[0][1] - first_line[0][1],
    )
    distance = _cross(relative, second_line[1]) / denominator
    circle_center = (
        first_line[0][0] + first_line[1][0] * distance,
        first_line[0][1] + first_line[1][1] * distance,
    )

    path_radii = [
        math.hypot(
            section.center[0] - circle_center[0],
            section.center[1] - circle_center[1],
        )
        for section in sections
    ]
    path_radius = sum(path_radii) / len(path_radii)
    tolerance = max(path_radius, sections[0].width, 1.0) * 1e-5
    if max(path_radii) - min(path_radii) > tolerance:
        return None

    raw_angles = [
        math.atan2(
            section.center[1] - circle_center[1],
            section.center[0] - circle_center[0],
        )
        for section in sections
    ]
    angles = [raw_angles[0]]
    for angle in raw_angles[1:]:
        difference = math.atan2(
            math.sin(angle - angles[-1]),
            math.cos(angle - angles[-1]),
        )
        angles.append(angles[-1] + difference)
    sweep = angles[-1] - angles[0]
    if abs(sweep) < 1e-6:
        return None

    thickness = max(float(thickness), 0.01)
    thickness_direction = -1.0 if housed else 1.0
    surface_radii = []
    for section in sections:
        rail = section.left if side == "Left" else section.right
        inward = _stringer_inward(section, side)
        first = (
            rail[0] + inward[0] * float(lateral_offset),
            rail[1] + inward[1] * float(lateral_offset),
        )
        second = (
            first[0] + inward[0] * thickness * thickness_direction,
            first[1] + inward[1] * thickness * thickness_direction,
        )
        surface_radii.extend(
            (
                math.hypot(
                    first[0] - circle_center[0],
                    first[1] - circle_center[1],
                ),
                math.hypot(
                    second[0] - circle_center[0],
                    second[1] - circle_center[1],
                ),
            )
        )
    inner_radius = sum(surface_radii[0::2]) / len(sections)
    outer_radius = sum(surface_radii[1::2]) / len(sections)
    inner_radius, outer_radius = sorted((inner_radius, outer_radius))
    if inner_radius < 0.01 or outer_radius - inner_radius < 0.005:
        return None

    return {
        "profile": _CircularProfile(
            circle_center,
            inner_radius,
            outer_radius,
            angles[0],
            sweep,
        ),
        "angles": angles,
        "path_radius": path_radius,
    }


def _circular_profile_between(profile, start_angle, end_angle):
    return _CircularProfile(
        profile.center,
        profile.inner_radius,
        profile.outer_radius,
        start_angle,
        end_angle - start_angle,
    )


def _annular_sector_face(profile, elevation):
    """Return an exact horizontal annular-sector face."""

    start_angle = profile.start_angle
    middle_angle = start_angle + profile.sweep / 2.0
    end_angle = start_angle + profile.sweep

    def point(radius, angle):
        return FreeCAD.Vector(
            profile.center[0] + radius * math.cos(angle),
            profile.center[1] + radius * math.sin(angle),
            elevation,
        )

    inner_start = point(profile.inner_radius, start_angle)
    outer_start = point(profile.outer_radius, start_angle)
    inner_end = point(profile.inner_radius, end_angle)
    outer_end = point(profile.outer_radius, end_angle)
    outer_arc = Part.Arc(
        outer_start,
        point(profile.outer_radius, middle_angle),
        outer_end,
    ).toShape()
    inner_arc = Part.Arc(
        inner_end,
        point(profile.inner_radius, middle_angle),
        inner_start,
    ).toShape()
    return Part.Face(
        Part.Wire(
            (
                Part.makeLine(inner_start, outer_start),
                outer_arc,
                Part.makeLine(outer_end, inner_end),
                inner_arc,
            )
        )
    )


def _helical_profile_edge(profile, radius, start_elevation, end_elevation):
    """Return an arc or exact helix spanning one circular profile."""

    height = float(end_elevation) - float(start_elevation)
    if abs(height) < 1e-7:
        start_angle = profile.start_angle
        middle_angle = start_angle + profile.sweep / 2.0
        end_angle = start_angle + profile.sweep

        def point(angle):
            return FreeCAD.Vector(
                profile.center[0] + radius * math.cos(angle),
                profile.center[1] + radius * math.sin(angle),
                start_elevation,
            )

        return Part.Arc(
            point(start_angle), point(middle_angle), point(end_angle)
        ).toShape()
    cylinder = Part.Cylinder()
    cylinder.Radius = radius
    cylinder.Center = FreeCAD.Vector(
        profile.center[0], profile.center[1], 0.0
    )
    cylinder.Axis = FreeCAD.Vector(0.0, 0.0, 1.0)
    cylinder.rotate(
        FreeCAD.Placement(
            FreeCAD.Vector(0.0, 0.0, 0.0),
            FreeCAD.Rotation(
                FreeCAD.Vector(0.0, 0.0, 1.0),
                math.degrees(profile.start_angle),
            ),
        )
    )
    return Part.Geom2d.Line2dSegment(
        FreeCAD.Base.Vector2d(0.0, start_elevation),
        FreeCAD.Base.Vector2d(profile.sweep, end_elevation),
    ).toShape(cylinder)


def _make_helical_band_solid(
    profile,
    top_front,
    top_rear,
    bottom_front,
    bottom_rear,
):
    """Create a curved board bounded by four exact coaxial helices."""

    maximum_sweep = math.pi / 2.0
    if abs(profile.sweep) > maximum_sweep + 1e-3:
        segment_count = int(math.ceil(abs(profile.sweep) / maximum_sweep))
        segments = []
        for index in range(segment_count):
            front_fraction = index / segment_count
            rear_fraction = (index + 1) / segment_count
            segment_profile = _CircularProfile(
                profile.center,
                profile.inner_radius,
                profile.outer_radius,
                profile.start_angle + profile.sweep * front_fraction,
                profile.sweep / segment_count,
            )
            segment = _make_helical_band_solid(
                segment_profile,
                top_front + (top_rear - top_front) * front_fraction,
                top_front + (top_rear - top_front) * rear_fraction,
                bottom_front
                + (bottom_rear - bottom_front) * front_fraction,
                bottom_front + (bottom_rear - bottom_front) * rear_fraction,
            )
            if segment is None:
                return None
            segments.append(segment)
        result = segments[0]
        for segment in segments[1:]:
            try:
                result = result.fuse(segment).removeSplitter()
            except (Part.OCCError, ValueError):
                return None
        if result.isValid() and len(result.Solids) == 1:
            return result
        return None

    circle_center = profile.center
    profile = _CircularProfile(
        (0.0, 0.0),
        profile.inner_radius,
        profile.outer_radius,
        profile.start_angle,
        profile.sweep,
    )

    top_inner = _helical_profile_edge(
        profile, profile.inner_radius, top_front, top_rear
    )
    top_outer = _helical_profile_edge(
        profile, profile.outer_radius, top_front, top_rear
    )
    bottom_inner = _helical_profile_edge(
        profile, profile.inner_radius, bottom_front, bottom_rear
    )
    bottom_outer = _helical_profile_edge(
        profile, profile.outer_radius, bottom_front, bottom_rear
    )
    if any(
        edge is None
        for edge in (top_inner, top_outer, bottom_inner, bottom_outer)
    ):
        return None

    def ruled_face(first, second):
        surface = Part.makeRuledSurface(first, second)
        return surface.Faces[0] if surface.Faces else None

    def cylindrical_face(radius):
        cylinder = Part.Cylinder()
        cylinder.Radius = radius
        cylinder.Center = FreeCAD.Vector(0.0, 0.0, 0.0)
        cylinder.Axis = FreeCAD.Vector(0.0, 0.0, 1.0)
        cylinder.rotate(
            FreeCAD.Placement(
                FreeCAD.Vector(0.0, 0.0, 0.0),
                FreeCAD.Rotation(
                    FreeCAD.Vector(0.0, 0.0, 1.0),
                    math.degrees(profile.start_angle),
                ),
            )
        )
        vector = FreeCAD.Base.Vector2d
        start_angle = 0.0
        end_angle = profile.sweep
        parameter_points = (
            vector(start_angle, bottom_front),
            vector(end_angle, bottom_rear),
            vector(end_angle, top_rear),
            vector(start_angle, top_front),
        )
        edges = [
            Part.Geom2d.Line2dSegment(first, second).toShape(cylinder)
            for first, second in zip(
                parameter_points,
                (*parameter_points[1:], parameter_points[0]),
            )
        ]
        return Part.Face(cylinder, Part.Wire(edges))

    top_face = (
        _annular_sector_face(profile, top_front)
        if abs(top_rear - top_front) < 1e-7
        else ruled_face(top_inner, top_outer)
    )
    bottom_face = (
        _annular_sector_face(profile, bottom_front)
        if abs(bottom_rear - bottom_front) < 1e-7
        else ruled_face(bottom_inner, bottom_outer)
    )
    inner_face = cylindrical_face(profile.inner_radius)
    outer_face = cylindrical_face(profile.outer_radius)
    if any(
        face is None
        for face in (top_face, bottom_face, inner_face, outer_face)
    ):
        return None

    start_points = (
        bottom_inner.Vertexes[0].Point,
        bottom_outer.Vertexes[0].Point,
        top_outer.Vertexes[0].Point,
        top_inner.Vertexes[0].Point,
    )
    end_points = (
        bottom_inner.Vertexes[-1].Point,
        bottom_outer.Vertexes[-1].Point,
        top_outer.Vertexes[-1].Point,
        top_inner.Vertexes[-1].Point,
    )
    start_face = Part.Face(
        Part.makePolygon((*start_points, start_points[0]))
    )
    end_face = Part.Face(Part.makePolygon((*end_points, end_points[0])))
    try:
        sewn = Part.makeCompound(
            (
                top_face,
                bottom_face,
                inner_face,
                outer_face,
                start_face,
                end_face,
            )
        )
        sewn.sewShape()
        if len(sewn.Shells) != 1 or not sewn.Shells[0].isClosed():
            return None
        solid = Part.makeSolid(sewn.Shells[0])
    except Part.OCCError:
        return None
    if solid.isValid() and len(solid.Solids) == 1:
        result = solid.removeSplitter()
        translation = FreeCAD.Matrix()
        translation.move(
            FreeCAD.Vector(circle_center[0], circle_center[1], 0.0)
        )
        return result.transformShape(translation, True)
    return None


def _make_sectioned_helical_band_solid(
    profile,
    angles,
    top_elevations,
    bottom_elevations,
):
    """Create a circular board whose pitch follows every stair section."""

    if not (
        len(angles) == len(top_elevations) == len(bottom_elevations)
        and len(angles) >= 2
    ):
        return None

    def interval_pitch(values, index):
        sweep = angles[index + 1] - angles[index]
        if abs(sweep) < 1e-9:
            return None
        return (values[index + 1] - values[index]) / sweep

    ranges = []
    range_start = 0
    previous_pitches = (
        interval_pitch(top_elevations, 0),
        interval_pitch(bottom_elevations, 0),
    )
    for index in range(1, len(angles) - 1):
        pitches = (
            interval_pitch(top_elevations, index),
            interval_pitch(bottom_elevations, index),
        )
        same_pitch = all(
            first is not None
            and second is not None
            and math.isclose(first, second, rel_tol=1e-8, abs_tol=1e-7)
            for first, second in zip(previous_pitches, pitches)
        )
        if not same_pitch:
            ranges.append((range_start, index))
            range_start = index
        previous_pitches = pitches
    ranges.append((range_start, len(angles) - 1))

    shapes = []
    for first, last in ranges:
        segment_profile = _circular_profile_between(
            profile, angles[first], angles[last]
        )
        segment = _make_helical_band_solid(
            segment_profile,
            top_elevations[first],
            top_elevations[last],
            bottom_elevations[first],
            bottom_elevations[last],
        )
        if segment is None:
            return None
        shapes.append(segment)

    result = shapes[0]
    for shape in shapes[1:]:
        try:
            result = result.fuse(shape)
        except (Part.OCCError, ValueError):
            return None
    try:
        result = result.removeSplitter()
    except Part.OCCError:
        pass
    if result.isValid() and len(result.Solids) == 1:
        return result
    return None


def make_handrail_path(
    sections,
    riser_height,
    side,
    lateral_offset,
    height_above_nosing,
    start_extension=0.0,
    end_extension=0.0,
):
    """Return the straight or helical reference path for one handrail side."""

    if len(sections) < 2:
        return None
    elevations = _stringer_elevations(sections, riser_height)
    profile_elevations = list(elevations)
    if (
        abs(profile_elevations[-1] - profile_elevations[-2]) < 1e-9
        and not any(section.landing_to_next for section in sections)
    ):
        profile_elevations[-1] += float(riser_height)
    top_elevations = [
        elevation + float(height_above_nosing)
        for elevation in profile_elevations
    ]
    support_elevations = list(elevations)
    if (
        len(support_elevations) >= 2
        and not sections[-2].landing_to_next
        and not sections[-1].landing_to_next
    ):
        # The final section is the rear boundary of the last tread.  A post
        # placed there bears on that tread, not on the upper-floor elevation.
        support_elevations[-1] = support_elevations[-2]
    start_extension = max(float(start_extension), 0.0)
    end_extension = max(float(end_extension), 0.0)
    lateral_offset = float(lateral_offset)
    points = []
    for section in sections:
        rail = section.left if side == "Left" else section.right
        inward = _stringer_inward(section, side)
        points.append(
            (
                rail[0] + inward[0] * lateral_offset,
                rail[1] + inward[1] * lateral_offset,
            )
        )

    circular = _circular_stringer_data(
        sections,
        side,
        0.01,
        lateral_offset + 0.005,
        True,
    )
    if circular is not None:
        profile = circular["profile"]
        radius = (profile.inner_radius + profile.outer_radius) / 2.0
        direction = 1.0 if profile.sweep >= 0.0 else -1.0
        slope = (
            (top_elevations[-1] - top_elevations[0])
            / (abs(profile.sweep) * radius)
            if abs(profile.sweep) * radius > 1e-9
            else 0.0
        )
        start_angle = (
            profile.start_angle - direction * start_extension / radius
        )
        sweep = profile.sweep + direction * (
            start_extension + end_extension
        ) / radius
        top_elevations[0] -= slope * start_extension
        top_elevations[-1] += slope * end_extension
        return {
            "kind": "Circular",
            "center": profile.center,
            "radius": radius,
            "start_angle": start_angle,
            "sweep": sweep,
            "length": abs(sweep) * radius,
            "top_elevations": top_elevations,
            "support_elevations": support_elevations,
            "cell_count": len(sections) - 1,
        }

    start = points[0]
    end = points[-1]
    direction = (end[0] - start[0], end[1] - start[1])
    length = math.hypot(*direction)
    if length < 1e-9:
        return None
    tangent = (direction[0] / length, direction[1] / length)
    slope = (
        (top_elevations[-1] - top_elevations[0]) / length
        if length > 1e-9
        else 0.0
    )
    start = (
        start[0] - tangent[0] * start_extension,
        start[1] - tangent[1] * start_extension,
    )
    end = (
        end[0] + tangent[0] * end_extension,
        end[1] + tangent[1] * end_extension,
    )
    top_elevations[0] -= slope * start_extension
    top_elevations[-1] += slope * end_extension
    length += start_extension + end_extension
    return {
        "kind": "Linear",
        "start": start,
        "end": end,
        "tangent": tangent,
        "length": length,
        "top_elevations": top_elevations,
        "support_elevations": support_elevations,
        "cell_count": len(sections) - 1,
    }


def sample_handrail_path(path, fraction):
    """Return plan position, tangent, rail top, and stair top along a path."""

    fraction = min(max(float(fraction), 0.0), 1.0)
    top_elevations = path["top_elevations"]
    top = top_elevations[0] + (
        top_elevations[-1] - top_elevations[0]
    ) * fraction
    if path["kind"] == "Circular":
        angle = path["start_angle"] + path["sweep"] * fraction
        direction = 1.0 if path["sweep"] >= 0.0 else -1.0
        point = (
            path["center"][0] + path["radius"] * math.cos(angle),
            path["center"][1] + path["radius"] * math.sin(angle),
        )
        tangent = (
            -direction * math.sin(angle),
            direction * math.cos(angle),
        )
    else:
        point = (
            path["start"][0]
            + (path["end"][0] - path["start"][0]) * fraction,
            path["start"][1]
            + (path["end"][1] - path["start"][1]) * fraction,
        )
        tangent = path["tangent"]

    cell_count = max(int(path["cell_count"]), 1)
    if fraction >= 1.0 - 1e-9:
        support = path["support_elevations"][-1]
    else:
        index = min(int(fraction * cell_count + 1e-9), cell_count - 1)
        support = path["support_elevations"][index]
    return {
        "point": point,
        "tangent": tangent,
        "top": top,
        "support": support,
    }


def handrail_picket_fractions(
    path_length,
    post_size,
    picket_size,
    maximum_spacing,
):
    """Return the smallest evenly spaced picket layout within the clear limit."""

    path_length = max(float(path_length), 0.0)
    post_size = max(float(post_size), 0.0)
    picket_size = max(float(picket_size), 0.0)
    maximum_spacing = max(float(maximum_spacing), 0.01)
    maximum_center_spacing = min(
        maximum_spacing + picket_size,
        maximum_spacing + (post_size + picket_size) / 2.0,
    )
    picket_count = max(
        int(math.ceil(path_length / maximum_center_spacing)) - 1,
        0,
    )
    return [
        (index + 1) / (picket_count + 1)
        for index in range(picket_count)
    ]


def make_handrail_top_rail_shape(
    path,
    rail_shape,
    width,
    thickness,
    post_penetration,
    post_size,
):
    """Create one straight extrusion or one exact helical top rail."""

    width = max(float(width), 0.01)
    thickness = max(float(thickness), 0.01)
    post_penetration = max(float(post_penetration), 0.0)
    post_size = max(float(post_size), 0.01)
    # Penetration is measured from each post's inner face.  Therefore half
    # the post size must be removed before shifting from its center line.
    terminal_offset = post_penetration - post_size / 2.0
    top_front = path["top_elevations"][0]
    top_rear = path["top_elevations"][-1]
    slope = (
        (top_rear - top_front) / path["length"]
        if path["length"] > 1e-9
        else 0.0
    )

    if path["kind"] == "Circular":
        direction = 1.0 if path["sweep"] >= 0.0 else -1.0
        extension_angle = terminal_offset / max(path["radius"], 0.01)
        profile = _CircularProfile(
            path["center"],
            max(path["radius"] - width / 2.0, 0.01),
            path["radius"] + width / 2.0,
            path["start_angle"] - direction * extension_angle,
            path["sweep"] + direction * 2.0 * extension_angle,
        )
        extended_front = top_front - slope * terminal_offset
        extended_rear = top_rear + slope * terminal_offset
        if str(rail_shape) != "Circular":
            result = _make_helical_band_solid(
                profile,
                extended_front,
                extended_rear,
                extended_front - thickness,
                extended_rear - thickness,
            )
            return result if result is not None else Part.Shape()

        center_profile = _CircularProfile(
            path["center"],
            path["radius"],
            path["radius"],
            profile.start_angle,
            profile.sweep,
        )
        radius = width / 2.0
        vertical_radius = radius / math.sqrt(1.0 + slope * slope)
        center_front = extended_front - vertical_radius
        center_rear = extended_rear - vertical_radius
        edge = _helical_profile_edge(
            center_profile,
            path["radius"],
            center_front,
            center_rear,
        )
        if edge is None:
            return Part.Shape()
        tangent = edge.tangentAt(edge.FirstParameter)
        circle = Part.makeCircle(
            radius,
            edge.valueAt(edge.FirstParameter),
            tangent,
        )
        try:
            return Part.Wire([edge]).makePipeShell(
                [Part.Wire([circle])], True, False
            )
        except Part.OCCError:
            return Part.Shape()

    tangent = path["tangent"]
    normal = (-tangent[1], tangent[0])
    start = (
        path["start"][0] - tangent[0] * terminal_offset,
        path["start"][1] - tangent[1] * terminal_offset,
    )
    end = (
        path["end"][0] + tangent[0] * terminal_offset,
        path["end"][1] + tangent[1] * terminal_offset,
    )
    extended_front = top_front - slope * terminal_offset
    extended_rear = top_rear + slope * terminal_offset
    extrusion = FreeCAD.Vector(
        end[0] - start[0],
        end[1] - start[1],
        extended_rear - extended_front,
    )
    if str(rail_shape) == "Circular":
        radius = width / 2.0
        axis = FreeCAD.Vector(extrusion)
        axis.normalize()
        vertical_radius = radius * math.sqrt(max(1.0 - axis.z * axis.z, 0.0))
        center = FreeCAD.Vector(
            start[0],
            start[1],
            extended_front - vertical_radius,
        )
        circle = Part.makeCircle(radius, center, axis)
        try:
            return Part.Face(Part.Wire([circle])).extrude(extrusion)
        except Part.OCCError:
            return Part.Shape()

    points = (
        FreeCAD.Vector(
            start[0] - normal[0] * width / 2.0,
            start[1] - normal[1] * width / 2.0,
            extended_front - thickness,
        ),
        FreeCAD.Vector(
            start[0] + normal[0] * width / 2.0,
            start[1] + normal[1] * width / 2.0,
            extended_front - thickness,
        ),
        FreeCAD.Vector(
            start[0] + normal[0] * width / 2.0,
            start[1] + normal[1] * width / 2.0,
            extended_front,
        ),
        FreeCAD.Vector(
            start[0] - normal[0] * width / 2.0,
            start[1] - normal[1] * width / 2.0,
            extended_front,
        ),
    )
    try:
        return Part.Face(
            Part.makePolygon((*points, points[0]))
        ).extrude(extrusion)
    except Part.OCCError:
        return Part.Shape()


def make_handrail_vertical_member_shape(
    point,
    tangent,
    bottom,
    top,
    member_shape,
    width,
    thickness,
):
    """Create one vertical post or picket."""

    bottom = float(bottom)
    top = max(float(top), bottom + 0.01)
    width = max(float(width), 0.01)
    thickness = max(float(thickness), 0.01)
    if str(member_shape) == "Circular":
        return Part.makeCylinder(
            width / 2.0,
            top - bottom,
            FreeCAD.Vector(point[0], point[1], bottom),
        )

    tangent_length = max(math.hypot(*tangent), 1e-9)
    tangent = (
        tangent[0] / tangent_length,
        tangent[1] / tangent_length,
    )
    normal = (-tangent[1], tangent[0])
    points = (
        FreeCAD.Vector(
            point[0] - normal[0] * width / 2.0
            - tangent[0] * thickness / 2.0,
            point[1] - normal[1] * width / 2.0
            - tangent[1] * thickness / 2.0,
            bottom,
        ),
        FreeCAD.Vector(
            point[0] + normal[0] * width / 2.0
            - tangent[0] * thickness / 2.0,
            point[1] + normal[1] * width / 2.0
            - tangent[1] * thickness / 2.0,
            bottom,
        ),
        FreeCAD.Vector(
            point[0] + normal[0] * width / 2.0
            + tangent[0] * thickness / 2.0,
            point[1] + normal[1] * width / 2.0
            + tangent[1] * thickness / 2.0,
            bottom,
        ),
        FreeCAD.Vector(
            point[0] - normal[0] * width / 2.0
            + tangent[0] * thickness / 2.0,
            point[1] - normal[1] * width / 2.0
            + tangent[1] * thickness / 2.0,
            bottom,
        ),
    )
    try:
        return Part.Face(
            Part.makePolygon((*points, points[0]))
        ).extrude(FreeCAD.Vector(0.0, 0.0, top - bottom))
    except Part.OCCError:
        return Part.Shape()


def make_housed_stringer_shape(
    sections,
    riser_height,
    side,
    thickness,
    width,
    penetration,
    start_extension,
    end_extension,
    nosing_offset,
    offset_direction="Perpendicular",
    nosing=0.0,
):
    """Create one smooth closed stringer housing the tread and riser ends."""

    if len(sections) < 2:
        return Part.Shape()
    runs = _stringer_section_runs(sections)
    if any(section.level_to_next for section in sections[:-1]):
        shapes = [
            _make_housed_stringer_run(
                run,
                riser_height,
                side,
                thickness,
                width,
                penetration,
                start_extension if index == 0 else 0.0,
                end_extension if index == len(runs) - 1 else 0.0,
                nosing_offset,
                offset_direction,
                nosing,
            )
            for index, run in enumerate(runs)
        ]
        if not shapes:
            return Part.Shape()
        return shapes[0] if len(shapes) == 1 else Part.makeCompound(shapes)
    return _make_housed_stringer_run(
        sections,
        riser_height,
        side,
        thickness,
        width,
        penetration,
        start_extension,
        end_extension,
        nosing_offset,
        offset_direction,
        nosing,
    )


def _make_housed_stringer_run(
    sections,
    riser_height,
    side,
    thickness,
    width,
    penetration,
    start_extension,
    end_extension,
    nosing_offset,
    offset_direction,
    nosing,
):
    """Create one continuous housed-stringer run."""

    thickness = max(float(thickness), 0.01)
    width = max(float(width), 0.01)
    start_extension = max(float(start_extension), 0.0)
    end_extension = max(float(end_extension), 0.0)
    elevations = _stringer_elevations(sections, riser_height)
    if (
        len(elevations) >= 2
        and abs(elevations[-1] - elevations[-2]) < 1e-9
        and not any(section.landing_to_next for section in sections)
    ):
        # The last section is the rear edge of the final tread, rather than
        # another nosing. Continue the board at the stair pitch through it;
        # otherwise a smooth loft flattens and overshoots before its extension.
        elevations[-1] = elevations[-2] + float(riser_height)
    circular = _circular_stringer_data(
        sections,
        side,
        thickness,
        penetration,
        True,
    )
    if circular is not None:
        circular_run = (
            abs(circular["profile"].sweep) * circular["path_radius"]
        )
        slope = (
            (elevations[-1] - elevations[0]) / circular_run
            if circular_run > 1e-9
            else 0.0
        )
    else:
        slope = _stringer_slope(sections, elevations)
    slope_cosine = 1.0 / math.sqrt(1.0 + slope * slope)
    vertical_width = width / max(slope_cosine, 0.01)
    nosing_compensation = slope * max(float(nosing), 0.0)
    vertical_offset = max(float(nosing_offset), 0.0)
    if str(offset_direction) == "Perpendicular":
        vertical_offset /= max(slope_cosine, 0.01)

    stations = list(sections)
    tops = [
        elevation + nosing_compensation + vertical_offset
        for elevation in elevations
    ]
    if circular is not None:
        profile = circular["profile"]
        direction = 1.0 if profile.sweep > 0.0 else -1.0
        start_angle_extension = (
            start_extension / circular["path_radius"]
        )
        end_angle_extension = end_extension / circular["path_radius"]
        angles = list(circular["angles"])
        circular_tops = list(tops)
        circular_bottoms = [
            max(top - vertical_width, 0.0) for top in tops
        ]
        if start_angle_extension > 1e-9:
            angles.insert(
                0, profile.start_angle - direction * start_angle_extension
            )
            circular_tops.insert(0, tops[0] - slope * start_extension)
            circular_bottoms.insert(
                0,
                max(
                    tops[0]
                    - vertical_width
                    - slope * start_extension,
                    0.0,
                ),
            )
        if end_angle_extension > 1e-9:
            angles.append(
                profile.start_angle
                + profile.sweep
                + direction * end_angle_extension
            )
            circular_tops.append(tops[-1] + slope * end_extension)
            circular_bottoms.append(
                max(
                    tops[-1]
                    - vertical_width
                    + slope * end_extension,
                    0.0,
                )
            )
        circular_shape = _make_sectioned_helical_band_solid(
            profile,
            angles,
            circular_tops,
            circular_bottoms,
        )
        if circular_shape is not None:
            return circular_shape

    if start_extension > 1e-9:
        stations.insert(0, _translated_section(sections[0], -start_extension))
        tops.insert(0, tops[0] - slope * start_extension)
    if end_extension > 1e-9:
        stations.append(_translated_section(sections[-1], end_extension))
        tops.append(tops[-1] + slope * end_extension)

    planar_shape = _make_planar_housed_stringer_shape(
        stations,
        tops,
        side,
        thickness,
        penetration,
        vertical_width,
    )
    if planar_shape is not None:
        return planar_shape

    wires = [
        _stringer_cross_section(
            section,
            side,
            max(top - vertical_width, 0.0),
            top,
            thickness,
            float(penetration),
            True,
        )
        for section, top in zip(stations, tops)
    ]
    result = Part.makeLoft(wires, True, False)
    if result.isValid() and len(result.Solids) == 1:
        try:
            return result.removeSplitter()
        except Part.OCCError:
            return result

    segments = [
        Part.makeLoft([first, second], True, True)
        for first, second in zip(wires, wires[1:])
    ]
    result = segments[0]
    for segment in segments[1:]:
        result = result.fuse(segment)
    try:
        return result.removeSplitter()
    except Part.OCCError:
        return result


def make_notched_stringer_shape(
    sections,
    riser_height,
    step_thickness,
    side,
    thickness,
    width,
    lateral_offset,
    end_extension,
    riser_clearance=0.0,
):
    """Create one notched board supporting the undersides of the treads."""

    if len(sections) < 2:
        return Part.Shape()
    runs = _stringer_section_runs(sections)
    if any(section.level_to_next for section in sections[:-1]):
        shapes = [
            _make_notched_stringer_run(
                run,
                riser_height,
                step_thickness,
                side,
                thickness,
                width,
                lateral_offset,
                end_extension if index == len(runs) - 1 else 0.0,
                riser_clearance,
            )
            for index, run in enumerate(runs)
        ]
        if not shapes:
            return Part.Shape()
        return shapes[0] if len(shapes) == 1 else Part.makeCompound(shapes)
    return _make_notched_stringer_run(
        sections,
        riser_height,
        step_thickness,
        side,
        thickness,
        width,
        lateral_offset,
        end_extension,
        riser_clearance,
    )


def _make_circular_notched_stringer_shape(
    sections,
    riser_height,
    step_thickness,
    side,
    thickness,
    width,
    lateral_offset,
    end_extension,
    riser_clearance,
):
    """Cut a sawtooth top into one exact helical annular board."""

    circular = _circular_stringer_data(
        sections,
        side,
        thickness,
        lateral_offset,
        False,
    )
    if circular is None:
        return None
    profile = circular["profile"]
    angles = circular["angles"]
    path_radius = circular["path_radius"]
    direction = 1.0 if profile.sweep > 0.0 else -1.0
    step_thickness = max(float(step_thickness), 0.0)
    width = max(float(width), step_thickness + 0.01)
    riser_clearance = max(float(riser_clearance), 0.0)
    elevations = _stringer_elevations(sections, riser_height)
    slope = _stringer_slope(sections, elevations)
    slope_cosine = 1.0 / math.sqrt(1.0 + slope * slope)
    vertical_width = width / max(slope_cosine, 0.01)
    bottoms = [
        max(elevation - vertical_width, 0.0)
        for elevation in elevations
    ]
    cell_count = len(sections) - 1

    def shifted_boundary(index):
        if riser_clearance < 1e-9 or index >= cell_count:
            return angles[index]
        available = abs(angles[index + 1] - angles[index]) * path_radius
        clearance = min(riser_clearance, max(available - 0.01, 0.0))
        return angles[index] + direction * clearance / path_radius

    main_top = max(
        elevation - step_thickness for elevation in elevations[:-1]
    )
    base = _make_helical_annular_solid(
        profile,
        main_top,
        bottoms[0],
        bottoms[-1],
    )
    if base is None:
        return None

    envelope_bottom = min(bottoms) - max(width, float(riser_height), 1.0)
    envelopes = []

    def add_envelope(start_angle, end_angle, top):
        if abs(end_angle - start_angle) < 1e-8:
            return
        sector = _circular_profile_between(
            profile, start_angle, end_angle
        )
        face = _annular_sector_face(sector, envelope_bottom)
        envelopes.append(
            face.extrude(
                FreeCAD.Vector(0.0, 0.0, top - envelope_bottom)
            )
        )

    first_start = shifted_boundary(0)
    first_riser_index = int(getattr(sections[0], "riser_index", 0))
    if first_riser_index > 1 and abs(first_start - angles[0]) > 1e-8:
        add_envelope(
            angles[0],
            first_start,
            elevations[0] - step_thickness - float(riser_height),
        )
    for index in range(cell_count):
        start_angle = shifted_boundary(index)
        end_angle = (
            shifted_boundary(index + 1)
            if index + 1 < cell_count
            else angles[index + 1]
        )
        add_envelope(
            start_angle,
            end_angle,
            elevations[index] - step_thickness,
        )
    if not envelopes:
        return None
    envelope = envelopes[0]
    for addition in envelopes[1:]:
        envelope = envelope.fuse(addition)
    result = base.common(envelope).removeSplitter()
    if not result.isValid() or len(result.Solids) != 1:
        return None

    end_extension = max(float(end_extension), 0.0)
    if end_extension > 1e-9:
        extension_sweep = direction * end_extension / path_radius
        extension_profile = _circular_profile_between(
            profile,
            angles[-1],
            angles[-1] + extension_sweep,
        )
        extension = _make_helical_band_solid(
            extension_profile,
            elevations[-1] - step_thickness,
            elevations[-1] - step_thickness + slope * end_extension,
            bottoms[-1],
            max(bottoms[-1] + slope * end_extension, 0.0),
        )
        if extension is not None:
            try:
                extended = result.fuse(extension).removeSplitter()
            except (Part.OCCError, ValueError):
                extended = Part.Shape()
            if (
                not extended.isNull()
                and extended.isValid()
                and len(extended.Solids) == 1
            ):
                result = extended
    return result


def _planar_notched_stringer_shape(
    sections,
    riser_height,
    step_thickness,
    side,
    thickness,
    width,
    lateral_offset,
    end_extension,
    riser_clearance,
):
    """Extrude one sawtooth side profile for a straight-flight board."""

    first_rail = sections[0].left if side == "Left" else sections[0].right
    tangent_length = math.hypot(*sections[0].tangent)
    if tangent_length < 1e-9:
        return None
    tangent = (
        sections[0].tangent[0] / tangent_length,
        sections[0].tangent[1] / tangent_length,
    )
    plan_normal = (-tangent[1], tangent[0])
    for section in sections:
        section_length = math.hypot(*section.tangent)
        if section_length < 1e-9:
            return None
        section_tangent = (
            section.tangent[0] / section_length,
            section.tangent[1] / section_length,
        )
        if (
            abs(
                tangent[0] * section_tangent[1]
                - tangent[1] * section_tangent[0]
            )
            > 1e-7
            or tangent[0] * section_tangent[0]
            + tangent[1] * section_tangent[1]
            < 0.0
        ):
            return None
        rail = section.left if side == "Left" else section.right
        if abs(
            (rail[0] - first_rail[0]) * plan_normal[0]
            + (rail[1] - first_rail[1]) * plan_normal[1]
        ) > 1e-5:
            return None

    thickness = max(float(thickness), 0.01)
    width = max(float(width), float(step_thickness) + 0.01)
    step_thickness = max(float(step_thickness), 0.0)
    lateral_offset = float(lateral_offset)
    riser_clearance = max(float(riser_clearance), 0.0)
    elevations = _stringer_elevations(sections, riser_height)
    slope = _stringer_slope(sections, elevations)
    slope_cosine = 1.0 / math.sqrt(1.0 + slope * slope)
    vertical_width = width / max(slope_cosine, 0.01)
    bottoms = [
        max(elevation - vertical_width, 0.0)
        for elevation in elevations
    ]

    inward = _stringer_inward(sections[0], side)

    def point(section, elevation, distance=0.0):
        rail = section.left if side == "Left" else section.right
        return FreeCAD.Vector(
            rail[0]
            + tangent[0] * distance
            + inward[0] * lateral_offset,
            rail[1]
            + tangent[1] * distance
            + inward[1] * lateral_offset,
            elevation,
        )

    def run_length(first, second):
        first_rail_point = first.left if side == "Left" else first.right
        second_rail_point = second.left if side == "Left" else second.right
        return max(
            (second_rail_point[0] - first_rail_point[0]) * tangent[0]
            + (second_rail_point[1] - first_rail_point[1]) * tangent[1],
            0.0,
        )

    def clearance_after(section_index):
        if riser_clearance < 1e-9 or section_index + 1 >= len(sections):
            return 0.0
        available = run_length(
            sections[section_index], sections[section_index + 1]
        )
        return min(riser_clearance, max(available - 0.01, 0.0))

    points = []

    def append(point_to_add):
        if not points or (point_to_add - points[-1]).Length > 1e-7:
            points.append(point_to_add)

    first_top = elevations[0] - step_thickness
    first_riser_index = int(getattr(sections[0], "riser_index", 0))
    start_clearance = clearance_after(0)
    shifted_start = first_riser_index <= 1 and start_clearance > 1e-9
    if shifted_start:
        append(point(sections[0], first_top, start_clearance))
    else:
        append(point(sections[0], first_top))
        if start_clearance > 1e-9:
            lower_top = first_top - float(riser_height)
            points[-1] = point(sections[0], lower_top)
            append(point(sections[0], lower_top, start_clearance))
            append(point(sections[0], first_top, start_clearance))

    cell_count = len(sections) - 1
    for index, rear in enumerate(sections[1:]):
        tread_bottom = elevations[index] - step_thickness
        append(point(rear, tread_bottom))
        if index + 1 >= cell_count:
            continue
        clearance = clearance_after(index + 1)
        if clearance > 1e-9:
            append(point(rear, tread_bottom, clearance))
        next_tread_bottom = elevations[index + 1] - step_thickness
        append(point(rear, next_tread_bottom, clearance))

    end_extension = max(float(end_extension), 0.0)
    if end_extension > 1e-9:
        end_top = elevations[-1] - step_thickness
        append(point(sections[-1], end_top))
        append(
            point(
                sections[-1],
                end_top + slope * end_extension,
                end_extension,
            )
        )

    bottom_points = [
        point(section, bottom)
        for section, bottom in zip(sections, bottoms)
    ]
    if shifted_start:
        first_run = max(run_length(sections[0], sections[1]), 0.01)
        start_bottom = bottoms[0] + (
            bottoms[1] - bottoms[0]
        ) * start_clearance / first_run
        bottom_points[0] = point(
            sections[0], start_bottom, start_clearance
        )
    if end_extension > 1e-9:
        bottom_points.append(
            point(
                sections[-1],
                bottoms[-1] + slope * end_extension,
                end_extension,
            )
        )
    for bottom_point in reversed(bottom_points):
        append(bottom_point)

    if len(points) < 3:
        return None
    if (points[0] - points[-1]).Length > 1e-7:
        points.append(points[0])
    try:
        side_face = Part.Face(Part.makePolygon(points))
        result = side_face.extrude(
            FreeCAD.Vector(
                inward[0] * thickness,
                inward[1] * thickness,
                0.0,
            )
        )
    except Part.OCCError:
        return None
    if not result.isValid() or len(result.Solids) != 1:
        return None
    try:
        return result.removeSplitter()
    except Part.OCCError:
        return result


def _make_notched_stringer_run(
    sections,
    riser_height,
    step_thickness,
    side,
    thickness,
    width,
    lateral_offset,
    end_extension,
    riser_clearance,
):
    """Create one continuous notched-stringer run."""

    circular = _make_circular_notched_stringer_shape(
        sections,
        riser_height,
        step_thickness,
        side,
        thickness,
        width,
        lateral_offset,
        end_extension,
        riser_clearance,
    )
    if circular is not None:
        return circular

    planar = _planar_notched_stringer_shape(
        sections,
        riser_height,
        step_thickness,
        side,
        thickness,
        width,
        lateral_offset,
        end_extension,
        riser_clearance,
    )
    if planar is not None:
        return planar

    thickness = max(float(thickness), 0.01)
    width = max(float(width), float(step_thickness) + 0.01)
    elevations = _stringer_elevations(sections, riser_height)
    slope = _stringer_slope(sections, elevations)
    slope_cosine = 1.0 / math.sqrt(1.0 + slope * slope)
    vertical_width = width / max(slope_cosine, 0.01)
    bottoms = [
        max(elevation - vertical_width, 0.0)
        for elevation in elevations
    ]
    solids = []
    for index, (front, rear) in enumerate(zip(sections, sections[1:])):
        tread_bottom = elevations[index] - float(step_thickness)
        front_wire = _stringer_cross_section(
            front,
            side,
            bottoms[index],
            tread_bottom,
            thickness,
            float(lateral_offset),
            False,
        )
        rear_wire = _stringer_cross_section(
            rear,
            side,
            bottoms[index + 1],
            tread_bottom,
            thickness,
            float(lateral_offset),
            False,
        )
        solids.append(Part.makeLoft([front_wire, rear_wire], True, True))

    end_extension = max(float(end_extension), 0.0)
    if end_extension > 1e-9:
        end = sections[-1]
        extended = _translated_section(end, end_extension)
        start_top = elevations[-1] - float(step_thickness)
        start_wire = _stringer_cross_section(
            end,
            side,
            bottoms[-1],
            start_top,
            thickness,
            float(lateral_offset),
            False,
        )
        end_wire = _stringer_cross_section(
            extended,
            side,
            max(bottoms[-1] + slope * end_extension, 0.0),
            start_top + slope * end_extension,
            thickness,
            float(lateral_offset),
            False,
        )
        solids.append(Part.makeLoft([start_wire, end_wire], True, True))

    result = solids[0]
    for solid in solids[1:]:
        result = result.fuse(solid)
    return result.removeSplitter()


def make_tread_shape(index, metrics, width, thickness, nosing, back_extension=0.0):
    """Create one local-coordinate wooden tread shape."""

    x = index * metrics.tread_width - nosing
    z = (index + 1) * metrics.riser_height - thickness
    depth = metrics.tread_width + nosing + back_extension
    return Part.makeBox(depth, width, thickness, FreeCAD.Vector(x, 0.0, z))


def make_riser_shape(
    index,
    metrics,
    width,
    thickness,
    step_thickness,
    upper_offset,
    lower_offset,
    priority_to_riser=False,
):
    """Create one local-coordinate wooden riser shape."""

    bottom_extension = step_thickness if priority_to_riser and index > 0 else 0.0
    height = max(
        metrics.riser_height
        - step_thickness
        + bottom_extension
        - upper_offset
        - lower_offset,
        0.01,
    )
    x = index * metrics.tread_width
    z = index * metrics.riser_height - bottom_extension + lower_offset
    return Part.makeBox(thickness, width, height, FreeCAD.Vector(x, 0.0, z))


def make_concrete_shape(metrics, width, thickness):
    """Create a stepped concrete stair with a sloping waist slab."""

    if not metrics.tread_count or metrics.tread_width <= 0.0:
        return Part.Shape()

    first_tread_height = metrics.riser_height
    pitch = metrics.riser_height / metrics.tread_width
    slope = math.atan(pitch)
    slope_cosine = max(math.cos(slope), 0.01)
    minimum_waist = metrics.riser_height * slope_cosine
    # An exact zero makes the underside touch every inner corner and creates
    # zero-width connections. Keep the user-facing zero while applying a
    # modeling tolerance that preserves one valid solid.
    effective_waist = minimum_waist + max(float(thickness), 0.01)
    vertical_waist = effective_waist / slope_cosine

    def pitch_height(x):
        return first_tread_height + pitch * x

    natural_start = (vertical_waist - first_tread_height) / pitch
    underside_start_x = min(
        max(0.0, natural_start),
        metrics.flight_length,
    )
    underside_start_z = max(pitch_height(underside_start_x) - vertical_waist, 0.0)
    underside_end = max(pitch_height(metrics.flight_length) - vertical_waist, 0.0)

    points = [FreeCAD.Vector(0.0, 0.0, first_tread_height)]
    for index in range(metrics.tread_count):
        x = (index + 1) * metrics.tread_width
        z = (index + 1) * metrics.riser_height
        points.append(FreeCAD.Vector(x, 0.0, z))
        if index < metrics.tread_count - 1:
            points.append(FreeCAD.Vector(x, 0.0, z + metrics.riser_height))

    def append_unique(x, z):
        previous = points[-1]
        if abs(previous.x - x) > 1e-9 or abs(previous.z - z) > 1e-9:
            points.append(FreeCAD.Vector(x, 0.0, z))

    append_unique(metrics.flight_length, underside_end)
    append_unique(underside_start_x, underside_start_z)
    append_unique(underside_start_x, 0.0)
    append_unique(0.0, 0.0)
    points.append(points[0])
    profile = Part.Face(Part.makePolygon(points))
    return profile.extrude(FreeCAD.Vector(0.0, width, 0.0))


def default_concrete_thickness(metrics):
    """Return added waist thickness starting below the second step."""

    if metrics.tread_width <= 0.0:
        return 150.0
    slope = math.atan(metrics.riser_height / metrics.tread_width)
    return metrics.riser_height * math.cos(slope)


def plan_segments(metrics, width, nosing):
    """Return line endpoints for the generated XY plan sketch."""

    start = -nosing
    end = metrics.flight_length
    segments = [
        ((start, 0.0), (end, 0.0)),
        ((end, 0.0), (end, width)),
        ((end, width), (start, width)),
        ((start, width), (start, 0.0)),
    ]
    for index in range(metrics.tread_count):
        x = index * metrics.tread_width
        segments.append(((x, 0.0), (x, width)))
    return segments
