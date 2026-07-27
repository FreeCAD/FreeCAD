# SPDX-License-Identifier: LGPL-2.1-or-later

"""Housed and notched stringer solids."""

import math

import FreeCAD
import Part

from .geometry_core import (
    _translated_section,
)

from .geometry_helical import (
    _annular_sector_face,
    _circular_profile_between,
    _make_helical_annular_solid,
    _make_helical_band_solid,
    _make_sectioned_helical_band_solid,
)

from .geometry_stringer_path import (
    _circular_stringer_data,
    _make_planar_housed_stringer_shape,
    _stringer_cross_section,
    _stringer_elevations,
    _stringer_inward,
    _stringer_section_runs,
    _stringer_slope,
)


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
        circular_run = abs(circular["profile"].sweep) * circular["path_radius"]
        slope = (elevations[-1] - elevations[0]) / circular_run if circular_run > 1e-9 else 0.0
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
        elevation
        + (0.0 if section.profile_nosing_aligned else nosing_compensation)
        + vertical_offset
        for section, elevation in zip(sections, elevations)
    ]
    if circular is not None:
        profile = circular["profile"]
        direction = 1.0 if profile.sweep > 0.0 else -1.0
        start_angle_extension = start_extension / circular["path_radius"]
        end_angle_extension = end_extension / circular["path_radius"]
        angles = list(circular["angles"])
        circular_tops = list(tops)
        circular_bottoms = [max(top - vertical_width, 0.0) for top in tops]
        if start_angle_extension > 1e-9:
            angles.insert(0, profile.start_angle - direction * start_angle_extension)
            circular_tops.insert(0, tops[0] - slope * start_extension)
            circular_bottoms.insert(
                0,
                max(
                    tops[0] - vertical_width - slope * start_extension,
                    0.0,
                ),
            )
        if end_angle_extension > 1e-9:
            angles.append(profile.start_angle + profile.sweep + direction * end_angle_extension)
            circular_tops.append(tops[-1] + slope * end_extension)
            circular_bottoms.append(
                max(
                    tops[-1] - vertical_width + slope * end_extension,
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
        Part.makeLoft([first, second], True, True) for first, second in zip(wires, wires[1:])
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
    bottoms = [max(elevation - vertical_width, 0.0) for elevation in elevations]
    cell_count = len(sections) - 1

    def shifted_boundary(index):
        if riser_clearance < 1e-9 or index >= cell_count:
            return angles[index]
        available = abs(angles[index + 1] - angles[index]) * path_radius
        clearance = min(riser_clearance, max(available - 0.01, 0.0))
        return angles[index] + direction * clearance / path_radius

    main_top = max(elevation - step_thickness for elevation in elevations[:-1])
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
        sector = _circular_profile_between(profile, start_angle, end_angle)
        face = _annular_sector_face(sector, envelope_bottom)
        envelopes.append(face.extrude(FreeCAD.Vector(0.0, 0.0, top - envelope_bottom)))

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
        end_angle = shifted_boundary(index + 1) if index + 1 < cell_count else angles[index + 1]
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
            if not extended.isNull() and extended.isValid() and len(extended.Solids) == 1:
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
            abs(tangent[0] * section_tangent[1] - tangent[1] * section_tangent[0]) > 1e-7
            or tangent[0] * section_tangent[0] + tangent[1] * section_tangent[1] < 0.0
        ):
            return None
        rail = section.left if side == "Left" else section.right
        if (
            abs(
                (rail[0] - first_rail[0]) * plan_normal[0]
                + (rail[1] - first_rail[1]) * plan_normal[1]
            )
            > 1e-5
        ):
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
    bottoms = [max(elevation - vertical_width, 0.0) for elevation in elevations]

    inward = _stringer_inward(sections[0], side)

    def point(section, elevation, distance=0.0):
        rail = section.left if side == "Left" else section.right
        return FreeCAD.Vector(
            rail[0] + tangent[0] * distance + inward[0] * lateral_offset,
            rail[1] + tangent[1] * distance + inward[1] * lateral_offset,
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
        available = run_length(sections[section_index], sections[section_index + 1])
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

    bottom_points = [point(section, bottom) for section, bottom in zip(sections, bottoms)]
    if shifted_start:
        first_run = max(run_length(sections[0], sections[1]), 0.01)
        start_bottom = bottoms[0] + (bottoms[1] - bottoms[0]) * start_clearance / first_run
        bottom_points[0] = point(sections[0], start_bottom, start_clearance)
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
    bottoms = [max(elevation - vertical_width, 0.0) for elevation in elevations]
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
