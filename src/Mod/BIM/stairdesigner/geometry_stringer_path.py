# SPDX-License-Identifier: LGPL-2.1-or-later

"""Stringer paths, profiles, and flight-run preparation."""

from dataclasses import replace
import math

import FreeCAD
import Part

from .geometry_core import (
    BalancedSection,
    _CircularProfile,
    _cross,
    _dot,
    balanced_section_top,
)

def straight_stringer_sections(
    metrics,
    width,
    tread_goings=None,
    top_elevations=None,
):
    """Return balanced-section compatible stations for one straight flight."""

    sections = []
    width = max(float(width), 0.01)
    goings = list(tread_goings or [])
    if len(goings) != metrics.tread_count:
        goings = [metrics.tread_width] * metrics.tread_count
    stations = [0.0]
    for going in goings:
        stations.append(stations[-1] + going)
    elevations = list(top_elevations or [])
    if len(elevations) != metrics.tread_count + 1:
        elevations = [
            (index + 1) * metrics.riser_height
            for index in range(metrics.tread_count + 1)
        ]
    for index in range(metrics.tread_count + 1):
        station = stations[index]
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
                top_elevation=elevations[index],
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
    if len(bottom_values) >= 2:
        # The terminal section is the rear boundary of the final tread and
        # its top is deliberately raised one riser in
        # ``_make_housed_stringer_run``.  Raising the lower curve with it
        # makes that curve cut through the last tread, especially when an
        # angled end makes one side longer.  Keep the terminal support at
        # least as deep as the preceding section.
        bottom_values[-1] = min(bottom_values[-1], bottom_values[-2])
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
    nosing=0.0,
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
    nosing_aligned = [False] * len(sections)
    nosing = max(float(nosing), 0.0)
    if nosing > 1e-9:
        for index, section in enumerate(sections[1:-1], start=1):
            section_length = math.hypot(*section.tangent)
            if section_length < 1e-9:
                continue
            section_tangent = (
                section.tangent[0] / section_length,
                section.tangent[1] / section_length,
            )
            crossing_cosine = _dot(tangent, section_tangent)
            if crossing_cosine > 1e-5:
                # The physical nosing is a parallel line ``nosing`` before
                # the concealed riser.  Its intersection with a straight
                # side board moves by nosing / cos(angle), not merely by the
                # nosing distance.  Profile the board at that real crossing.
                distances[index] -= nosing / crossing_cosine
                nosing_aligned[index] = True
    if start_seam is not None:
        distances[0] = longitudinal(start_seam)
    if end_seam is not None:
        distances[-1] = longitudinal(end_seam)

    result = []
    for section, distance, aligned in zip(
        sections, distances, nosing_aligned
    ):
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
                profile_nosing_aligned=aligned,
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
