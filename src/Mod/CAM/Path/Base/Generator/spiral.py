# SPDX-License-Identifier: LGPL-2.1-or-later AND MIT
# SPDX-FileNotice: Part of the FreeCAD project.
################################################################################
#                                                                              #
#   © 2026                                                                     #
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


import FreeCAD
import math
import Path

__title__ = "Spiral toolpath Generator"
__author__ = "tarman3"
__url__ = "https://www.freecad.org"
__doc__ = "Generates the spiral toolpath"


if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


def generate(
    center,
    outer_radius,
    step,
    inner_radius=0,
    direction="CW",
    startAt="Outside",
    dir_angle_rad=0,
):
    """
    Example of use in Mod/CAM/Path/Op/Helix.py

    generate(center, outer_radius, step)  # generate spiral commands
        center: point in the center of the spiral
        outer_radius: outer radius of the spiral Path
        step: change radius on each turn

    import Path.Base.Generator.spiral as spiral
    args = {
        "center": FreeCAD.Vector(),
        "outer_radius": 100,
        "step": 5,
        "inner_radius": 10,
        "direction": "CW",
        "startAt": "Inside",
        "dir_angle_rad": 0,
    }
    spiralCommands = spiral.generate(**args)
    spiralCommands[0]  # move to start point
    spiralCommands[1:3] # commands of init circle
    spiralCommands[3:-2] # all other commands
    spiralCommands[-2:] # commands of finish circle

    cmd = spiralCommands[0]
    startPoint = FreeCAD.Vector(cmd.x, cmd.y, cmd.z)
        or
    startPoint = cmd.Placement.Base
    """

    Path.Log.track(
        "(spiral: <{}, {}, {}>\n outer radius {}\n inner radius {}\n step {}\n direction {}\n startAt {})".format(
            center.x,
            center.y,
            center.z,
            outer_radius,
            inner_radius,
            step,
            direction,
            startAt,
        )
    )

    if not isinstance(outer_radius, (float, int)):
        raise TypeError("Invalid type for outer radius")

    if outer_radius <= 0:
        raise ValueError("outer_radius <= 0")

    if not isinstance(inner_radius, (float, int)):
        raise TypeError("inner_radius must be a float")

    if inner_radius < 0:
        raise ValueError("inner_radius < 0")

    if outer_radius <= inner_radius:
        raise ValueError("outer_radius equal or less than inner_radius")

    if startAt not in ("Inside", "Outside"):
        raise ValueError("Invalid value for parameter 'startAt'")

    if direction not in ("CW", "CCW"):
        raise ValueError("Invalid value for parameter 'direction'")

    if not isinstance(step, (float, int)) or step <= 0:
        raise TypeError("Invalid value for parameter 'step'")

    if not isinstance(dir_angle_rad, (float, int)):
        raise TypeError("Invalid value for parameter 'dir_angle_rad'")

    if not isinstance(center, FreeCAD.Vector):
        raise ValueError("'center' is not a point")

    def createInitMove(radius, cmdName="G1"):
        commands = []
        dx = radius * math.cos(dir_angle_rad)
        dy = radius * math.sin(dir_angle_rad)
        p = FreeCAD.Vector(center.x + dx, center.y + dy, center.z)
        commands.append(Path.Command(cmdName, {"X": p.x, "Y": p.y, "Z": p.z}))

        return commands

    # Create circle by two 180 degree arcs
    def createCircle(radius, arcCmdName):
        commands = []
        dx = radius * math.cos(dir_angle_rad)
        dy = radius * math.sin(dir_angle_rad)
        p1 = FreeCAD.Vector(center.x - dx, center.y - dy, center.z)
        p2 = FreeCAD.Vector(center.x + dx, center.y + dy, center.z)
        commands.append(
            Path.Command(arcCmdName, {"X": p1.x, "Y": p1.y, "Z": p1.z, "I": -dx, "J": -dy})
        )
        commands.append(
            Path.Command(arcCmdName, {"X": p2.x, "Y": p2.y, "Z": p2.z, "I": dx, "J": dy})
        )

        return commands

    # Calculate arc center by three points
    def getArcCenter(p1, p2, p3):
        # https://paulbourke.net/geometry/circlesphere
        ma = (p2.y - p1.y) / (p2.x - p1.x)
        mb = (p3.y - p2.y) / (p3.x - p2.x)
        arcCenter = FreeCAD.Vector()
        arcCenter.x = (ma * mb * (p1.y - p3.y) + mb * (p1.x + p2.x) - ma * (p2.x + p3.x)) / (
            2 * (mb - ma)
        )
        arcCenter.y = -1 * (arcCenter.x - (p1.x + p2.x) / 2) / ma + (p1.y + p2.y) / 2

        return arcCenter

    # turnDivider should be n*6 (e.g. 6, 12, ...)
    n = 2 if inner_radius <= 5 else 1
    turnDivider = n * 6
    stepAngle = math.tau / turnDivider  # step angle for rotate
    # stepAngle = math.radians(10)  # step angle for rotate (for spiral with linear movements)
    stepAngle = -stepAngle if direction == "CCW" else stepAngle
    turns = math.ceil((outer_radius - inner_radius) / step)  # amount of spiral turns
    iters = round(math.tau * turns / abs(stepAngle))
    stepRadius = (outer_radius - inner_radius) / iters  # changes spiral radius on each step
    stepRadius = -stepRadius if startAt == "Outside" else stepRadius
    angle = math.pi / 2 - dir_angle_rad
    spiralR = outer_radius if startAt == "Outside" else inner_radius
    arcCmdName = "G2" if direction == "CW" else "G3"
    commandlist = []

    # add straight move to start point
    commandlist.extend(createInitMove(spiralR, "G1"))

    # add init circle
    commandlist.extend(createCircle(spiralR, arcCmdName))

    arcPoints = []
    count = 0
    while count <= iters:
        x = center.x + spiralR * math.sin(angle)
        y = center.y + spiralR * math.cos(angle)
        z = center.z
        arcPoints.append(FreeCAD.Vector(x, y, z))  # Get all points of arcs
        # commandlist.append(Path.Command("G1", {"X":x, "Y":y}))  # for spiral with linear movements
        if count == iters:
            # need break because 'spiralR' using below
            break
        angle += stepAngle
        spiralR += stepRadius
        count += 1

    i = 0
    while i <= len(arcPoints) - 3:
        arcEnd = arcPoints[i + 2]
        arcCenter = getArcCenter(arcPoints[i], arcPoints[i + 1], arcPoints[i + 2])
        offset = arcCenter - arcPoints[i]
        commandlist.append(
            Path.Command(
                arcCmdName,
                {"X": arcEnd.x, "Y": arcEnd.y, "Z": arcEnd.z, "I": offset.x, "J": offset.y},
            )
        )
        i += 2

    # add finish circle
    commandlist.extend(createCircle(spiralR, arcCmdName))

    return commandlist
