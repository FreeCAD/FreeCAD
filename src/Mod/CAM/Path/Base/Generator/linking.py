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


def get_linking_moves(
    start_position: Vector,
    target_position: Vector,
    local_clearance: float,
    global_clearance: float,
    tool_shape: Part.Shape,  # required placeholder
    solids: Optional[List[Part.Shape]] = None,
    retract_height_offset: Optional[float] = None,
) -> list:
    if start_position == target_position:
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
            collision_model = Part.makeFuse(solids)

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

    heights = sorted(candidate_heights, reverse=True)

    # Try each height
    for height in heights:
        wire = make_linking_wire(start_position, target_position, height)
        if is_wire_collision_free(wire, collision_model):
            cmds = Path.fromShape(wire).Commands
            for cmd in cmds:
                cmd.Name = "G0"
            return cmds

    raise RuntimeError("No collision-free path found between start and target positions")


def make_linking_wire(start: Vector, target: Vector, z: float) -> Part.Wire:
    p1 = Vector(start.x, start.y, z)
    p2 = Vector(target.x, target.y, z)
    e1 = Part.makeLine(start, p1)
    e2 = Part.makeLine(p1, p2)
    e3 = Part.makeLine(p2, target)
    return Part.Wire([e1, e2, e3])


def is_wire_collision_free(
    wire: Part.Wire, solid: Optional[Part.Shape], tolerance: float = 0.001
) -> bool:
    if not solid:
        return True
    distance = wire.distToShape(solid)[0]
    return distance >= tolerance
