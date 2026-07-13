# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 FreeCAD Project Association
# SPDX-FileNotice: Part of the FreeCAD project.
################################################################################
#                                                                              #
#   FreeCAD is free software: you can redistribute it and/or modify            #
#   it under the terms of the GNU Lesser General Public                       #
#   License as published by the Free Software Foundation, either version 2    #
#   of the License, or (at your option) any later version.                     #
#                                                                              #
################################################################################

"""Utilities for calculating and applying wall endpoint edits.

This module deliberately has no dependency on :mod:`Arch`.  The public Arch
API and the wall proxy share these helpers, keeping endpoint conversion out
of the recompute-time factory module.
"""

from dataclasses import dataclass

import FreeCAD
import Part


@dataclass(frozen=True)
class WallEndpointEdit:
    """Fully calculated state for one endpoint edit.

    The state is resolved before the wall is mutated.  ``child_placements``
    preserves hosted objects in global coordinates when a based wall becomes
    baseless.  Applying this value never recomputes the document; the caller
    controls the surrounding transaction and recompute boundary.
    """

    base: object
    length: float
    placement: FreeCAD.Placement
    child_placements: tuple = ()


def is_debasable(wall):
    """Return whether an Arch wall can be converted to a baseless wall."""
    if getattr(getattr(wall, "Proxy", None), "Type", None) != "Wall":
        return False

    base = getattr(wall, "Base", None)
    if not base or not hasattr(base, "Shape") or base.Shape.isNull():
        return False

    if len(base.Shape.Edges) != 1:
        return False
    edge = base.Shape.Edges[0]
    return isinstance(edge.Curve, (Part.Line, Part.LineSegment))


def debaseWall(wall):
    """Convert a line-based Arch wall to a baseless wall in place."""
    state = resolve_debase_edit(wall)
    if state is None:
        FreeCAD.Console.PrintWarning(f"Wall '{wall.Label}' is not eligible for debasing.\n")
        return False

    apply_endpoint_edit(wall, state)
    wall.Document.recompute()
    return True


def resolve_endpoint_edit(wall, points):
    """Calculate a complete endpoint edit without changing the document.

    ``points`` are global endpoints.  Invalid coincident points raise before
    any based-wall conversion can occur.  Unsupported based walls return
    ``None`` so callers can preserve the existing behavior for non-debasable
    wall bases.
    """
    if len(points) < 2:
        return None

    p1 = FreeCAD.Vector(points[0])
    p2 = FreeCAD.Vector(points[1])
    length = p1.distanceToPoint(p2)
    if length <= 1e-9:
        raise ValueError("Wall endpoints must be distinct")

    direction = p2.sub(p1)
    direction.normalize()
    placement = FreeCAD.Placement(
        (p1 + p2) * 0.5,
        FreeCAD.Rotation(FreeCAD.Vector(1, 0, 0), direction),
    )
    if wall.Base:
        if not is_debasable(wall):
            return None
        child_placements = tuple(
            (child, child.Placement.copy()) for child in wall.Proxy.getMovableChildren(wall)
        )
        return WallEndpointEdit(None, length, placement, child_placements)
    return WallEndpointEdit(wall.Base, length, placement)


def apply_endpoint_edit(wall, state):
    """Apply a previously resolved endpoint edit without recomputing."""
    if state is None:
        return False

    wall.Base = state.base
    wall.Length = state.length
    wall.Placement = state.placement
    wall.Proxy.connectEdges = []
    for child, child_placement in state.child_placements:
        child.Placement = child_placement
    return True


def resolve_debase_edit(wall):
    """Calculate the baseless state for a debasable wall without mutation."""
    if not is_debasable(wall):
        return None

    child_placements = _get_child_placements(wall)
    final_placement = _get_debased_placement(wall)
    return WallEndpointEdit(
        None,
        wall.Length.Value,
        final_placement,
        tuple(child_placements.items()),
    )


def _get_child_placements(wall):
    """Return movable child placements that must remain fixed in world space."""
    children = wall.Proxy.getMovableChildren(wall)
    return {child: child.Placement.copy() for child in children}


def _get_debased_placement(wall):
    """Calculate the placement for a wall whose straight base is removed."""
    base_obj = wall.Base
    p1_global, p2_global = get_oriented_base_points(base_obj)

    normal = wall.Normal
    if normal.Length == 0:
        normal = base_obj.Placement.Rotation.multVec(FreeCAD.Vector(0, 0, 1))
        if normal.Length == 0:
            normal = FreeCAD.Vector(0, 0, 1)

    z_axis = normal.normalize()
    x_axis = (p2_global - p1_global).normalize()
    y_axis = x_axis.cross(z_axis).normalize()
    rotation = FreeCAD.Rotation(x_axis, y_axis, z_axis)
    position = (p1_global + p2_global) * 0.5
    return FreeCAD.Placement(position, rotation)


def get_oriented_base_points(base):
    """Return semantic global endpoints for a supported straight base.

    Draft lines expose ``Start`` and ``End`` properties; sketches expose the
    first line geometry's start and end points.  Only the generic fallback
    relies on shape vertex order, and it is retained for legacy line-shaped
    document objects that provide no semantic endpoint API.
    """
    if hasattr(base, "Start") and hasattr(base, "End"):
        return FreeCAD.Vector(base.Start), FreeCAD.Vector(base.End)

    if base.isDerivedFrom("Sketcher::SketchObject") and len(base.Geometry) == 1:
        geometry = base.Geometry[0]
        if isinstance(geometry, Part.LineSegment):
            placement = base.Placement
            return (
                placement.multVec(geometry.StartPoint),
                placement.multVec(geometry.EndPoint),
            )

    edge = base.Shape.Edges[0]
    return edge.Vertexes[0].Point, edge.Vertexes[1].Point
