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

from .geometry_profiles import make_member_shape, rail_profile_face
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
    if (
        abs(profile_elevations[-1] - profile_elevations[-2]) < 1e-9
        and not any(section.landing_to_next for section in sections)
        and math.dist(
            sections[-1].left if side == "Left" else sections[-1].right,
            sections[-2].left if side == "Left" else sections[-2].right,
        )
        > 1e-7
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


def fit_handrail_to_nosings(path, sections, side):
    """Fit a straight rail to flight nosings; newel heights are separate."""
    if path["kind"] != "Linear":
        return
    tangent = path["tangent"]
    stations = []
    for section in sections:
        point = section.left if side == "Left" else section.right
        stations.append(sum((point[i] - path["start"][i]) * tangent[i] for i in (0, 1)))
    elevations = path["top_elevations"]
    samples = [
        (x, z)
        for section, x, z in zip(sections, stations, elevations)
        if not section.profile_at_newel
    ]
    if len(samples) < 2:
        samples = list(zip(stations, elevations))
    mean_x = sum(x for x, _ in samples) / len(samples)
    mean_z = sum(z for _, z in samples) / len(samples)
    variance = sum((x - mean_x) ** 2 for x, _ in samples)
    slope = sum((x - mean_x) * (z - mean_z) for x, z in samples) / max(variance, 1e-9)
    intercept = max(z - slope * x for x, z in samples)
    path["top_elevations"] = [intercept + slope * x for x in stations]
    path["top_elevations"][0] = intercept
    path["top_elevations"][-1] = intercept + slope * path["length"]


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


def _straight_profiled_rail(
    rail_shape, width, thickness, start, end, tangent, normal, top, extrusion
):
    """Return a straight rail of a classic section, or ``None``.

    The section stands square to the rail; its ends are cut plumb where
    the plain rail's are.
    """

    axis = FreeCAD.Vector(extrusion)
    length = axis.Length
    if length < 1e-6:
        return None
    axis.normalize()
    across = FreeCAD.Vector(normal[0], normal[1], 0.0)
    down = axis.cross(across)
    if down.z > 0.0:
        down = -down
    level = math.hypot(axis.x, axis.y)
    if level < 1e-6:
        return None
    # Long enough for both plumb cuts to pass through the whole section.
    reach = thickness * abs(axis.z) / level + 1.0
    try:
        face = rail_profile_face(
            rail_shape,
            width,
            thickness,
            FreeCAD.Vector(start[0], start[1], top) - axis * reach,
            across,
            down,
        )
        if face is None:
            return None
        solid = face.extrude(axis * (length + 2.0 * reach))
        bounds = solid.BoundBox
        corners = [
            FreeCAD.Vector(
                point[0] + normal[0] * side * width,
                point[1] + normal[1] * side * width,
                bounds.ZMin - 1.0,
            )
            for point, side in ((start, -1.0), (end, -1.0), (end, 1.0), (start, 1.0))
        ]
        slab = Part.Face(Part.makePolygon(corners + corners[:1])).extrude(
            FreeCAD.Vector(0.0, 0.0, bounds.ZLength + 2.0)
        )
        result = solid.common(slab)
    except (Part.OCCError, ValueError):
        return None
    if result.isNull() or not result.isValid() or len(result.Solids) != 1:
        return None
    return result.Solids[0]


def _helical_profiled_rail(path, profile, rail_shape, width, thickness, front, rear):
    """Return a helical rail of a classic section swept along its top
    line, upright all the way, or ``None``."""

    centre = _CircularProfile(
        path["center"], path["radius"], path["radius"], profile.start_angle, profile.sweep
    )
    try:
        edge = _helical_profile_edge(centre, path["radius"], front, rear)
        if edge is None:
            return None
        origin = edge.valueAt(edge.FirstParameter)
        axis = edge.tangentAt(edge.FirstParameter)
        across = FreeCAD.Vector(origin.x - path["center"][0], origin.y - path["center"][1], 0.0)
        across.normalize()
        down = axis.cross(across)
        if down.z > 0.0:
            down = -down
        face = rail_profile_face(rail_shape, width, thickness, origin, across, down)
        if face is None:
            return None
        sweep = Part.BRepOffsetAPI.MakePipeShell(Part.Wire([edge]))
        sweep.setBiNormalMode(FreeCAD.Vector(0.0, 0.0, 1.0))
        sweep.add(face.OuterWire)
        if not sweep.isReady():
            return None
        sweep.build()
        sweep.makeSolid()
        result = sweep.shape()
    except (Part.OCCError, ValueError, RuntimeError, AttributeError):
        return None
    if result.isNull() or not result.isValid() or len(result.Solids) != 1:
        return None
    return result


def handrail_plumb_depth(thickness, slope):
    """Return how deep a rail ``thickness`` thick, or across, stands plumb
    under its top line when it runs at ``slope``."""

    return float(thickness) * math.sqrt(1.0 + float(slope) * float(slope))


def make_handrail_top_rail_shape(
    path,
    rail_shape,
    width,
    thickness,
    post_penetration,
    post_size,
):
    """Create one straight extrusion or one exact helical top rail.

    A square rail is ``thickness`` thick square to its pitch, whatever the
    pitch: its plumb depth under the top line grows with it. The other
    sections of ``RAIL_PROFILES`` stand square to the rail within its
    ``width`` and ``thickness``; where one cannot be built the rail is
    square.
    """

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
    depth = handrail_plumb_depth(thickness, slope)

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
        if str(rail_shape) not in ("Square", "Circular"):
            profiled = _helical_profiled_rail(
                path, profile, rail_shape, width, thickness, extended_front, extended_rear
            )
            if profiled is not None:
                return profiled
        if str(rail_shape) != "Circular":
            result = _make_helical_band_solid(
                profile,
                extended_front,
                extended_rear,
                extended_front - depth,
                extended_rear - depth,
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
        # A round rail's top stands this far plumb above its axis.
        vertical_radius = handrail_plumb_depth(radius, slope)
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
    if str(rail_shape) not in ("Square", "Circular"):
        profiled = _straight_profiled_rail(
            rail_shape, width, thickness, start, end, tangent, normal, extended_front, extrusion
        )
        if profiled is not None:
            return profiled
    if str(rail_shape) == "Circular":
        radius = width / 2.0
        axis = FreeCAD.Vector(extrusion)
        axis.normalize()
        # A round rail's top stands this far plumb above its axis.
        vertical_radius = radius / max(math.sqrt(max(1.0 - axis.z * axis.z, 0.0)), 0.01)
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
            extended_front - depth,
        ),
        FreeCAD.Vector(
            start[0] + normal[0] * width / 2.0,
            start[1] + normal[1] * width / 2.0,
            extended_front - depth,
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
    blocks=None,
    cap="Flat",
):
    """Create one vertical post or picket.

    A member worked to a classic shape, or with a worked top end, is made
    by ``make_member_shape``, which describes ``blocks`` and ``cap``.
    """

    if str(member_shape) not in ("Square", "Circular") or str(cap) != "Flat":
        return make_member_shape(
            point, tangent, bottom, top, member_shape, width, thickness, blocks, cap
        )
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
