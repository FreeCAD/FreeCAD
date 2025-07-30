import Part
import Path
from FreeCAD import Vector
from typing import List, Optional

def get_linking_moves(
    start_position: Vector,
    target_position: Vector,
    global_clearance: float,
    tool_shape: Part.Shape,  # required placeholder
    local_clearance: Optional[float] = None,
    solids: Optional[List[Part.Shape]] = None,
    retract_height_offset: float = 0.5,
) -> list:

    if start_position == target_position:
        return []

    if local_clearance is None:
        local_clearance = global_clearance

    if local_clearance > global_clearance:
        raise ValueError("Operation safe plane must not exceed job safe plane")

    if retract_height_offset < 0:
        raise ValueError("Retract offset must be positive")


    # build a fusion of solids to test collision
    collision_model = None
    if solids:
        solids = [s for s in solids if s]
        if len(solids) == 1:
            collision_model = solids[0]
        elif len(solids) > 1:
            collision_model = Part.makeFuse(solids)

    # Build list of transit heights
    if retract_height_offset > 0:
        retract_height = max(start_position.z, target_position.z) + retract_height_offset
        candidate_heights = {retract_height, local_clearance, global_clearance}
    else:
        candidate_heights = {local_clearance, global_clearance}

    heights = sorted(candidate_heights, reverse=True)

    wire = None
    for height in candidate_heights:
        trial_wire = make_linking_wire(start_position, target_position, height)
        if is_wire_collision_free(trial_wire, collision_model):
            wire = trial_wire
            break

    if not wire:
        raise RuntimeError("No collision-free path found between start and target positions")

    cmds = Path.fromShape(wire).Commands

    rapids = []
    for cmd in cmds:
        cmd.Name = "G0"
    return cmds


def make_linking_wire(start: Vector, target: Vector, z: float) -> Part.Wire:
    p1 = Vector(start.x, start.y, z)
    p2 = Vector(target.x, target.y, z)
    e1 = Part.makeLine(start, p1)
    e2 = Part.makeLine(p1, p2)
    e3 = Part.makeLine(p2, target)
    return Part.Wire([e1, e2, e3])


def is_wire_collision_free(wire: Part.Wire, solid: Optional[Part.Shape], tolerance: float = 0.001) -> bool:
    if not solid:
        return True
    distance = wire.distToShape(solid)[0]
    return distance >= tolerance
