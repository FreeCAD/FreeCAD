# SPDX-License-Identifier: LGPL-2.1-or-later

"""Straight-flight tread, riser, concrete, and plan primitives."""

import math

import FreeCAD
import Part


def make_tread_shape(
    index,
    metrics,
    width,
    thickness,
    nosing,
    back_extension=0.0,
    tread_goings=None,
    riser_heights=None,
):
    """Create one local-coordinate wooden tread shape."""

    goings = list(tread_goings or [])
    if len(goings) != metrics.tread_count:
        goings = [metrics.tread_width] * metrics.tread_count
    heights = list(riser_heights or [])
    if len(heights) != metrics.tread_count:
        heights = [metrics.riser_height] * metrics.tread_count
    x = sum(goings[:index]) - nosing
    z = sum(heights[: index + 1]) - thickness
    depth = goings[index] + nosing + back_extension
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
    tread_goings=None,
    riser_heights=None,
    concrete_dressing=False,
):
    """Create one local-coordinate riser shape."""

    heights = list(riser_heights or [])
    if len(heights) <= index:
        heights = [metrics.riser_height] * (metrics.tread_count + 1)
    goings = list(tread_goings or [])
    if len(goings) != metrics.tread_count:
        goings = [metrics.tread_width] * metrics.tread_count
    if concrete_dressing:
        lower_edge_cover = step_thickness if priority_to_riser and index > 0 else 0.0
        upper_tread_space = step_thickness if index < metrics.tread_count else 0.0
        base = sum(heights[:index]) - lower_edge_cover + lower_offset
        top = sum(heights[: index + 1]) - upper_tread_space - upper_offset
        x = sum(goings[:index]) - thickness
        return Part.makeBox(
            thickness,
            width,
            max(top - base, 0.01),
            FreeCAD.Vector(x, 0.0, base),
        )

    bottom_extension = step_thickness if priority_to_riser and index > 0 else 0.0
    upper_step_thickness = 0.0 if index >= metrics.tread_count else step_thickness
    height = max(
        heights[index] - upper_step_thickness + bottom_extension - upper_offset - lower_offset,
        0.01,
    )
    x = sum(goings[:index])
    z = sum(heights[:index]) - bottom_extension + lower_offset
    return Part.makeBox(thickness, width, height, FreeCAD.Vector(x, 0.0, z))


def make_concrete_shape(
    metrics,
    width,
    thickness,
    bottom_cut_distance=0.0,
    top_cut_distance=0.0,
    finish_thickness=0.0,
    structure_width_offset=0.0,
):
    """Create a stepped concrete stair with a sloping waist slab."""

    if not metrics.tread_count or metrics.tread_width <= 0.0:
        return Part.Shape()

    finish_thickness = max(float(finish_thickness), 0.0)
    first_tread_height = metrics.riser_height - finish_thickness
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

    bottom_cut_level = -max(float(bottom_cut_distance), 0.0)
    natural_start = (bottom_cut_level + vertical_waist - first_tread_height) / pitch
    underside_start_x = min(
        max(0.0, natural_start),
        metrics.flight_length,
    )
    underside_start_z = max(
        pitch_height(underside_start_x) - vertical_waist,
        bottom_cut_level,
    )
    underside_end = max(
        pitch_height(metrics.flight_length) - vertical_waist,
        bottom_cut_level,
    )

    points = [FreeCAD.Vector(0.0, 0.0, first_tread_height)]
    for index in range(metrics.tread_count):
        x = (index + 1) * metrics.tread_width
        z = (index + 1) * metrics.riser_height - finish_thickness
        points.append(FreeCAD.Vector(x, 0.0, z))
        if index < metrics.tread_count - 1:
            points.append(FreeCAD.Vector(x, 0.0, z + metrics.riser_height))

    def append_unique(x, z):
        previous = points[-1]
        if abs(previous.x - x) > 1e-9 or abs(previous.z - z) > 1e-9:
            points.append(FreeCAD.Vector(x, 0.0, z))

    top_cut_distance = max(float(top_cut_distance), 0.0)
    terminal_top = points[-1].z
    available_top_extension = max(
        (terminal_top - underside_end) / pitch,
        0.0,
    )
    top_extension = min(top_cut_distance, available_top_extension)
    if top_extension > 1e-9:
        append_unique(
            metrics.flight_length + top_extension,
            terminal_top,
        )
        append_unique(
            metrics.flight_length + top_extension,
            underside_end + pitch * top_extension,
        )
        append_unique(metrics.flight_length, underside_end)
    else:
        append_unique(metrics.flight_length, underside_end)
    append_unique(underside_start_x, underside_start_z)
    if natural_start > 0.0:
        append_unique(underside_start_x, bottom_cut_level)
        append_unique(0.0, bottom_cut_level)
    points.append(points[0])
    profile = Part.Face(Part.makePolygon(points))
    maximum_side_offset = max((float(width) - 0.01) / 2.0, 0.0)
    side_offset = min(
        max(float(structure_width_offset), 0.0),
        maximum_side_offset,
    )
    profile.translate(FreeCAD.Vector(0.0, side_offset, 0.0))
    structure_width = max(float(width) - 2.0 * side_offset, 0.01)
    return profile.extrude(FreeCAD.Vector(0.0, structure_width, 0.0))


def default_concrete_thickness(metrics):
    """Return added waist thickness starting below the second step."""

    if metrics.tread_width <= 0.0:
        return 150.0
    slope = math.atan(metrics.riser_height / metrics.tread_width)
    return metrics.riser_height * math.cos(slope)


def plan_segments(metrics, width, nosing, tread_goings=None):
    """Return line endpoints for the generated XY plan sketch."""

    start = -nosing
    end = metrics.flight_length
    segments = [
        ((start, 0.0), (end, 0.0)),
        ((end, 0.0), (end, width)),
        ((end, width), (start, width)),
        ((start, width), (start, 0.0)),
    ]
    goings = list(tread_goings or [])
    if len(goings) != metrics.tread_count:
        goings = [metrics.tread_width] * metrics.tread_count
    x = 0.0
    for index in range(metrics.tread_count):
        segments.append(((x, 0.0), (x, width)))
        x += goings[index]
    return segments
