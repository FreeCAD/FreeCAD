# -*- coding: utf-8 -*-
# SPDX-License-Identifier: LGPL-2.1-or-later
# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2025 sliptonic sliptonic@freecad.org                    *
# *                                                                         *
# *   This file is part of FreeCAD.                                         *
# *                                                                         *
# *   FreeCAD is free software: you can redistribute it and/or modify it    *
# *   under the terms of the GNU Lesser General Public License as           *
# *   published by the Free Software Foundation, either version 2.1 of the  *
# *   License, or (at your option) any later version.                       *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful, but        *
# *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
# *   Lesser General Public License for more details.                       *
# *                                                                         *
# *   You should have received a copy of the GNU Lesser General Public      *
# *   License along with FreeCAD. If not, see                               *
# *   <https://www.gnu.org/licenses/>.                                      *
# *                                                                         *
# ***************************************************************************

import Part
import Path
from FreeCAD import Vector
from typing import List, Optional

if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


def check_collision(
    start_position: Vector,
    target_position: Vector,
    solids: Optional[List[Part.Shape]] = None,
    tool_shape: Optional[Part.Shape] = None,
    tool_diameter: Optional[float] = None,
    safety_margin: float = 1,
) -> bool:
    """
    Check if a direct move from start to target would collide with solids.
    Returns True if collision detected, False if path is clear.

    For collision detection uses one of the methods below:
    - tool_shape: cross-section of the tool shape (most long computation)
    - tool_diameter: uses horizontal face with width of the tool diameter (middle computation)
    - if no tool_shape and tool_diameter uses simple wire (fast computation)
    """

    if start_position == target_position:
        return False

    # Build collision model
    collision_model = None
    if solids:
        solids = [s for s in solids if s]
        if len(solids) == 1:
            collision_model = solids[0]
        elif len(solids) > 1:
            collision_model = Part.makeCompound(solids)

    if not collision_model:
        return False

    safety_margin = max(safety_margin, 0) or 1

    # Create direct path wire
    wire = Part.Wire([Part.makeLine(start_position, target_position)])

    bbDistance = wire.BoundBox.ZMin - collision_model.BoundBox.ZMax
    if bbDistance >= safety_margin or Path.Geom.isRoughly(bbDistance, safety_margin):
        # attempt to skip long time computation for simple model
        return False

    shape = None
    if tool_shape:
        shape = _create_tool_path_shape(wire, tool_shape)
    elif tool_diameter:
        shape = _create_horizontal_face(wire, tool_diameter)
    if shape:
        distance = shape.distToShape(collision_model)[0]
    else:
        distance = wire.distToShape(collision_model)[0]
    return distance < safety_margin and not Path.Geom.isRoughly(distance, safety_margin)


def get_linking_moves(
    start_position: Vector,
    target_position: Vector,
    local_clearance: float,
    global_clearance: float,
    tool_shape: Optional[Part.Shape] = None,
    tool_diameter: Optional[float] = None,
    solids: Optional[List[Part.Shape]] = None,
    retract_height_offset: Optional[float] = None,
    skip_if_no_collision: bool = False,
    safety_margin: float = 1,
) -> list:
    """
    Generate linking moves from start to target position.

    If skip_if_no_collision is True and the direct path at the current height
    is collision-free, returns empty list (useful for canned drill cycles that
    handle their own retraction).

    For collision detection uses one of the methods below:
    - tool_shape: cross-section of the tool shape (most long computation)
    - tool_diameter: uses horizontal face with width of the tool diameter (middle computation)
    - if no tool_shape and tool_diameter uses simple wire (fast computation)
    """
    if start_position == target_position:
        return []

    # For canned cycles: if we're already at a safe height and can move directly, skip linking
    if skip_if_no_collision:
        if not check_collision(start_position, target_position, solids):
            return []

    if local_clearance > global_clearance:
        raise ValueError("Local clearance must not exceed global clearance")

    if retract_height_offset is not None and retract_height_offset < 0:
        raise ValueError("Retract offset must be positive")

    # Collision model
    collision_model = None
    if solids:
        solids = [s for s in solids if s]
        if len(solids) == 1:
            collision_model = solids[0]
        elif len(solids) > 1:
            collision_model = Part.makeCompound(solids)

    # Determine candidate heights
    if retract_height_offset is not None:
        if retract_height_offset > 0:
            retract_height = max(start_position.z, target_position.z) + retract_height_offset
            candidate_heights = {retract_height, local_clearance, global_clearance}
        else:  # explicitly 0
            retract_height = max(start_position.z, target_position.z)
            candidate_heights = {retract_height, local_clearance, global_clearance}
    else:
        candidate_heights = {local_clearance, global_clearance}

    heights = sorted(candidate_heights)

    safety_margin = max(safety_margin, 0) or 1

    # Try each height
    for height in heights:
        wire = make_linking_wire(start_position, target_position, height)
        if is_travel_collision_free(
            wire, collision_model, tool_shape, tool_diameter, safety_margin
        ):
            cmds = Path.fromShape(wire).Commands
            # Ensure all commands have complete XYZ coordinates
            # Path.fromShape() may omit coordinates that don't change
            current_pos = start_position
            complete_cmds = []
            for i, cmd in enumerate(cmds):
                params = dict(cmd.Parameters)
                # Fill in missing coordinates from current position
                x = params.get("X", current_pos.x)
                y = params.get("Y", current_pos.y)
                # For the last command (plunge to target), use target.z if Z is missing
                if "Z" not in params and i == len(cmds) - 1:
                    z = target_position.z
                else:
                    z = params.get("Z", current_pos.z)
                complete_cmds.append(Path.Command("G0", {"X": x, "Y": y, "Z": z}))
                current_pos = Vector(x, y, z)
            return complete_cmds

    raise RuntimeError("No collision-free path found between start and target positions")


def make_linking_wire(start: Vector, target: Vector, z: float) -> Part.Wire:
    p1 = Vector(start.x, start.y, z)
    p2 = Vector(target.x, target.y, z)
    edges = []

    # Only add retract edge if there's actual movement
    if not start.isEqual(p1, 1e-6):
        edges.append(Part.makeLine(start, p1))

    # Only add traverse edge if there's actual movement
    if not p1.isEqual(p2, 1e-6):
        edges.append(Part.makeLine(p1, p2))

    # Only add plunge edge if there's actual movement
    if not p2.isEqual(target, 1e-6):
        edges.append(Part.makeLine(p2, target))

    return Part.Wire(edges) if edges else Part.Wire([Part.makeLine(start, target)])


def is_travel_collision_free(
    wire: Part.Wire,
    solid: Optional[Part.Shape],
    tool_shape: Optional[Part.Shape] = None,
    tool_diameter: Optional[float] = None,
    safety_margin: float = 1,
) -> bool:
    """
    Check if a horizontal edge of wire would not collide with solids.
    Returns True if path is clear, False if collision detected.
    """
    if not solid:
        return True

    safety_margin = max(safety_margin, 0) or 1

    bbDistance = wire.BoundBox.ZMax - solid.BoundBox.ZMax
    if bbDistance >= safety_margin or Path.Geom.isRoughly(bbDistance, safety_margin):
        # attempt to skip long time computation for simple model
        return True

    shape = None
    if tool_shape:
        shape = _create_tool_path_shape(wire, tool_shape)
    elif tool_diameter:
        shape = _create_horizontal_face(wire, tool_diameter)
    if shape:
        distance = shape.distToShape(solid)[0]
    else:
        distance = wire.distToShape(solid)[0]
    return distance >= safety_margin or Path.Geom.isRoughly(distance, safety_margin)


def _create_horizontal_face(wire, width):
    """Return a rectangular horizontal face sweeping the wire's horizontal
    edge laterally by `width` (tool diameter). Used for tool-diameter
    collision checks. Returns None if the wire has no horizontal edge."""
    edge = None
    for e in wire.Edges:
        if Path.Geom.isHorizontal(e):
            edge = e
            break
    if not edge:
        return None
    direction = edge.Vertexes[1].Point - edge.Vertexes[0].Point
    normal = direction.cross(Vector(0, 0, 1))
    offset = normal.normalize() * width / 2
    e1 = edge.translated(offset)
    e2 = edge.translated(-offset)
    e3 = Part.makeLine(e1.Vertexes[0].Point, e2.Vertexes[0].Point)
    e4 = Part.makeLine(e1.Vertexes[1].Point, e2.Vertexes[1].Point)
    wire = Part.Wire(Part.__sortEdges__([e1, e2, e3, e4]))

    return Part.makeFace(wire)


def _create_tool_path_shape(wire, tool_shape):
    """Return a solid representing the volume swept by the tool's
    cross-section along the wire's horizontal edge. Used for full
    tool-shape collision checks. Returns None if there is no
    horizontal edge or the slice is empty."""
    edge = None
    for e in wire.Edges:
        if Path.Geom.isHorizontal(e):
            edge = e
            break
    if not edge:
        return None
    p0 = edge.Vertexes[0].Point
    p1 = edge.Vertexes[1].Point
    direction = p1 - p0
    section = tool_shape.slice(direction, 0)
    if not section:
        return None
    section[0].translate(p0)

    return section[0].extrude(direction)
