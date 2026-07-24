# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileNotice: Part of the FreeCAD project.

################################################################################
#                                                                              #
#   FreeCAD is free software: you can redistribute it and/or modify            #
#   it under the terms of the GNU Lesser General Public License as             #
#   published by the Free Software Foundation, either version 2.1              #
#   of the License, or (at your option) any later version.                     #
#                                                                              #
#   FreeCAD is distributed in the hope that it will be useful,                 #
#   but WITHOUT ANY WARRANTY; without even the implied warranty                #
#   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                    #
#   See the GNU Lesser General Public License for more details.                #
#                                                                              #
#   You should have received a copy of the GNU Lesser General Public           #
#   License along with FreeCAD. If not, see https://www.gnu.org/licenses       #
#                                                                              #
################################################################################

import Constants
import Path
from Path.Base.MachineState import MachineState


def get_path(path, step, retract_height, vert_feed, step_min=0.001):
    """
    Returns plunge milling for path

    path: Path.Path object
    step: distance between points, mm
    retract_height: height for rapid moves between points, mm
    vert_feed: vertical feed, mm/sec
    step_min: skip points which is closer than this distance, mm
    """

    if not isinstance(path, Path.Path):
        raise TypeError("Invalid type of path")

    if not isinstance(step, (float, int)):
        raise TypeError("Invalid type of step_max")

    if step < 0 or Path.Geom.isRoughly(step, 0):
        raise ValueError("step <= 0")

    if not isinstance(step_min, (float, int)):
        raise TypeError("Invalid type of step_max")

    if step_min < 0:
        raise ValueError("step_min < 0")

    if not isinstance(retract_height, (float, int)):
        raise TypeError("Invalid type of retract_height")

    if not isinstance(vert_feed, (float, int)):
        raise TypeError("Invalid type of vert_feed")

    last = None
    machine = MachineState()
    commands = []
    for cmd in path.Commands:
        if cmd.Name not in Constants.GCODE_MOVE_MILL:
            commands.append(cmd)

        else:
            position = machine.getPosition()
            edge = Path.Geom.edgeForCmd(cmd, position)
            if not edge:
                continue

            if Path.Geom.isVertical(edge):
                machine.addCommand(cmd)
                continue

            number = Path.Geom.ceil(edge.Length / step) + 1
            points = edge.discretize(Number=number) if number >= 2 else edge.Vertexes
            for p in points:
                if last and Path.Geom.pointsCoincide(p, last, step_min):
                    # skip too close point
                    continue
                commands.append(Path.Command("G0", {"X": p.x, "Y": p.y}))
                commands.append(Path.Command("G1", {"Z": p.z, "F": vert_feed}))
                commands.append(Path.Command("G0", {"Z": retract_height}))
                last = p

        machine.addCommand(cmd)

    return Path.Path(commands)
