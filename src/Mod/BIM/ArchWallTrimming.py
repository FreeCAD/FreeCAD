# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 FreeCAD Project Association
# SPDX-FileNotice: Part of the FreeCAD project.
################################################################################
#                                                                              #
#   FreeCAD is free software: you can redistribute it and/or modify it        #
#   under the terms of the GNU Lesser General Public License as published by  #
#   the Free Software Foundation, either version 2.1 or (at your option) any   #
#   later version.                                                         #
#                                                                              #
################################################################################

"""Geometry operations used to apply resolved wall end conditions.

This module deliberately knows only about Part shapes, wall placements, and
resolved global baseline geometry.  Provider ordering and relation ownership
remain in :mod:`ArchWallEndCondition` and
:mod:`ArchWallRelationResolver`; the wall proxy only sequences construction,
subshape processing, and final trimming.
"""

import math

import FreeCAD
import Part

from draftutils.translate import translate


def extend_solid_along_baseline(solid, baseline, wall_placement, end_name, extension):
    """Extend a construction solid by translating it along its baseline.

    The extension is made before wall openings and other subshapes are
    processed.  Translating the construction solid follows an oblique or
    sketch-derived baseline and retains its resolved section; an axis-aligned
    bounding box would fill unrelated corners and could refill openings when
    applied to an already processed wall.
    """
    if extension <= 1e-9 or solid.isNull() or baseline is None:
        return solid

    direction = baseline.end_point.sub(baseline.start_point)
    direction = wall_placement.inverse().Rotation.multVec(direction)
    if direction.Length <= 1e-9:
        return solid
    direction.normalize()
    if end_name == "Start":
        direction = direction.negative()

    baseline_length = baseline.end_point.sub(baseline.start_point).Length
    if baseline_length <= 1e-9:
        return solid

    try:
        # Keep neighboring translated copies overlapping so the result stays a
        # single solid even when the requested extension exceeds the wall
        # length.  A half-length step leaves a generous overlap for short,
        # acute-angle miter extensions.
        step = min(extension, baseline_length * 0.5)
        extended = solid.copy()
        copy_count = math.ceil(extension / step)
        for index in range(1, copy_count):
            translated = solid.copy()
            translated.translate(direction * step * index)
            extended = extended.fuse(translated)
        translated = solid.copy()
        translated.translate(direction * extension)
        extended = extended.fuse(translated)
    except (Part.OCCError, RuntimeError):
        return solid

    if (
        extended.isNull()
        or not extended.Solids
        or len(extended.Solids) > len(solid.Solids)
        or not extended.isValid()
    ):
        return solid
    return extended


def apply_cutting_plane(
    obj,
    solid,
    wall_placement,
    plane_placement,
    ref_point,
    tool_size,
    is_global=False,
):
    """Keep the portion of ``solid`` on the selected side of a plane.

    Cutting tools are finite and sized from the current solid bounds.  Any
    OCC or invalid-result failure leaves the current solid untouched and emits
    a warning, so a failed end condition cannot replace a valid wall shape
    with an exception or stale geometry.
    """
    global_plane_placement = (
        plane_placement if is_global else wall_placement.multiply(plane_placement)
    )
    try:
        face_size, depth = _get_cutting_tool_extents(
            solid,
            wall_placement,
            global_plane_placement,
            ref_point,
            tool_size,
        )
        cutting_tool_global = _create_cutting_tool_from_plane(
            global_plane_placement, ref_point, face_size, depth
        )
        cutting_tool_local = cutting_tool_global.copy()
        cutting_tool_local.transformShape(wall_placement.inverse().toMatrix())
        trimmed = solid.common(cutting_tool_local)
        if trimmed.isNull() or not trimmed.Solids or not trimmed.isValid():
            raise ValueError("the cutting plane produced no valid solid")
        return trimmed
    except (Part.OCCError, RuntimeError, ValueError) as exc:
        FreeCAD.Console.PrintWarning(
            translate(
                "Arch",
                "Could not apply a wall end condition to {0}: {1}",
            ).format(obj.Label, str(exc))
            + "\n"
        )
        return solid


def _get_cutting_tool_extents(solid, wall_placement, cutting_placement, ref_point, min_tool_size):
    """Return face size and depth that cover the current solid bounds."""
    local_points = _get_bound_box_corners(solid.BoundBox)
    global_points = [wall_placement.multVec(point) for point in local_points]
    global_points.append(ref_point)

    plane_inverse = cutting_placement.inverse()
    plane_points = [plane_inverse.multVec(point) for point in global_points]
    margin = max(min_tool_size * 0.05, 1.0)
    max_projection = max(max(abs(point.x), abs(point.y)) for point in plane_points)
    keep_sign = 1.0 if plane_inverse.multVec(ref_point).z >= 0 else -1.0
    depth = max(max(point.z * keep_sign for point in plane_points), 0.0)
    return max(min_tool_size, 2.0 * (max_projection + margin)), max(min_tool_size, depth + margin)


def _get_bound_box_corners(bound_box):
    return [
        FreeCAD.Vector(x, y, z)
        for x in (bound_box.XMin, bound_box.XMax)
        for y in (bound_box.YMin, bound_box.YMax)
        for z in (bound_box.ZMin, bound_box.ZMax)
    ]


def _create_cutting_tool_from_plane(cutting_placement, ref_point, face_size, depth):
    """Create a finite solid on the side of ``cutting_placement`` at ref_point."""
    p1 = FreeCAD.Vector(-face_size / 2, -face_size / 2, 0)
    p2 = FreeCAD.Vector(face_size / 2, -face_size / 2, 0)
    p3 = FreeCAD.Vector(face_size / 2, face_size / 2, 0)
    p4 = FreeCAD.Vector(-face_size / 2, face_size / 2, 0)
    bounded_face = Part.Face(Part.makePolygon([p1, p2, p3, p4, p1]))
    bounded_face.Placement = cutting_placement

    keep_direction = cutting_placement.Rotation.multVec(FreeCAD.Vector(0, 0, 1))
    if (ref_point - cutting_placement.Base).dot(keep_direction) < 0:
        keep_direction = keep_direction.negative()
    return bounded_face.extrude(keep_direction.normalize() * depth)
