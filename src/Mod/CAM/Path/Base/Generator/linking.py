# -*- coding: utf-8 -*-
# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileNotice: Part of the FreeCAD project.

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2025 sliptonic sliptonic@freecad.org                    *
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
    tolerance: float = 0.001,
) -> bool:
    """
    Check if a direct move from start to target would collide with solids.
    Returns True if collision detected, False if path is clear.
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

    # Create direct path wire
    wire = Part.Wire([Part.makeLine(start_position, target_position)])
    distance = wire.distToShape(collision_model)[0]
    return distance < tolerance


def get_linking_moves(
    start_position: Vector,
    target_position: Vector,
    local_clearance: float,
    global_clearance: float,
    tool_shape: Part.Shape,  # required placeholder
    solids: Optional[List[Part.Shape]] = None,
    retract_height_offset: Optional[float] = None,
    skip_if_no_collision: bool = False,
) -> list:
    """
    Generate linking moves from start to target position.

    If skip_if_no_collision is True and the direct path at the current height
    is collision-free, returns empty list (useful for canned drill cycles that
    handle their own retraction).
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

    # Try each height
    for height in heights:
        wire = make_linking_wire(start_position, target_position, height)
        if is_wire_collision_free(wire, collision_model):
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


def is_wire_collision_free(
    wire: Part.Wire, solid: Optional[Part.Shape], tolerance: float = 0.001
) -> bool:
    if not solid:
        return True
    distance = wire.distToShape(solid)[0]
    return distance >= tolerance
