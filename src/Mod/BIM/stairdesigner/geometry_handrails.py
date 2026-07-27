# SPDX-License-Identifier: LGPL-2.1-or-later

"""Handrail paths, rails, posts, and pickets."""

import math

import FreeCAD
import Part

from .geometry_core import (
    _CircularProfile,
)

from .geometry_helical import (
    _helical_profile_edge,
    _make_helical_band_solid,
)

from .geometry_stringer_path import (
    _circular_stringer_data,
    _stringer_elevations,
    _stringer_inward,
)


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
    if abs(profile_elevations[-1] - profile_elevations[-2]) < 1e-9 and not any(
        section.landing_to_next for section in sections
    ):
        profile_elevations[-1] += float(riser_height)
    top_elevations = [elevation + float(height_above_nosing) for elevation in profile_elevations]
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
            (top_elevations[-1] - top_elevations[0]) / (abs(profile.sweep) * radius)
            if abs(profile.sweep) * radius > 1e-9
            else 0.0
        )
        start_angle = profile.start_angle - direction * start_extension / radius
        sweep = profile.sweep + direction * (start_extension + end_extension) / radius
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
    slope = (top_elevations[-1] - top_elevations[0]) / length if length > 1e-9 else 0.0
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
    top = top_elevations[0] + (top_elevations[-1] - top_elevations[0]) * fraction
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
            path["start"][0] + (path["end"][0] - path["start"][0]) * fraction,
            path["start"][1] + (path["end"][1] - path["start"][1]) * fraction,
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
    return [(index + 1) / (picket_count + 1) for index in range(picket_count)]


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
    slope = (top_rear - top_front) / path["length"] if path["length"] > 1e-9 else 0.0

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
            return Part.Wire([edge]).makePipeShell([Part.Wire([circle])], True, False)
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
        return Part.Face(Part.makePolygon((*points, points[0]))).extrude(extrusion)
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
            point[0] - normal[0] * width / 2.0 - tangent[0] * thickness / 2.0,
            point[1] - normal[1] * width / 2.0 - tangent[1] * thickness / 2.0,
            bottom,
        ),
        FreeCAD.Vector(
            point[0] + normal[0] * width / 2.0 - tangent[0] * thickness / 2.0,
            point[1] + normal[1] * width / 2.0 - tangent[1] * thickness / 2.0,
            bottom,
        ),
        FreeCAD.Vector(
            point[0] + normal[0] * width / 2.0 + tangent[0] * thickness / 2.0,
            point[1] + normal[1] * width / 2.0 + tangent[1] * thickness / 2.0,
            bottom,
        ),
        FreeCAD.Vector(
            point[0] - normal[0] * width / 2.0 + tangent[0] * thickness / 2.0,
            point[1] - normal[1] * width / 2.0 + tangent[1] * thickness / 2.0,
            bottom,
        ),
    )
    try:
        return Part.Face(Part.makePolygon((*points, points[0]))).extrude(
            FreeCAD.Vector(0.0, 0.0, top - bottom)
        )
    except Part.OCCError:
        return Part.Shape()
