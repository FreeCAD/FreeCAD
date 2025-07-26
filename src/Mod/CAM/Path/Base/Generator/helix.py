# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2021 sliptonic <shopinthewoods@gmail.com>               *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************


from numpy import ceil, linspace, isclose
import FreeCAD
import math
import Path

__title__ = "Helix toolpath Generator"
__author__ = "sliptonic (Brad Collette)"
__url__ = "https://www.freecad.org"
__doc__ = "Generates the helical toolpath for a single spot targetshape"
__contributors__ = "russ4262 (Russell Johnson), Lorenz Hüdepohl"


if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


def generate(
    edge,
    outer_radius,
    step_down,
    step_over=0,
    tool_diameter=0,
    inner_radius=None,
    retract_height=None,
    direction="CW",
    startAt="Outside",
    finish_circle=True,
    cone_angle=0,
):
    """
    Example of use in Mod/CAM/Path/Op/Helix.py

    generate(edge, outer_radius, step_down)  # generate helix commands
        edge: vertical line in the center of helix
        outer_radius: radius of the helix Path
        step_down: vertical step for one turn of helix

    import Path.Base.Generator.helix as helix
    helixCommands = helix.generate(**args)
    helixCommands[0]  # move to retract height
    helixCommands[1]  # move to start point
    helixCommands[2:] # all other commands
    """

    topCenterPoint = edge.Vertexes[0].Point
    bottomCenterPoint = edge.Vertexes[1].Point
    if not retract_height or retract_height < topCenterPoint.z:
        retract_height = topCenterPoint.z

    Path.Log.track(
        "(helix: <{}, {}>\n outer radius {}\n inner radius {}\n retract height {}\n step over {}\n start point {}\n end point {}\n step_down {}\n tool diameter {}\n direction {}\n startAt {})".format(
            topCenterPoint.x,
            topCenterPoint.y,
            outer_radius,
            inner_radius,
            retract_height,
            step_over,
            topCenterPoint.z,
            bottomCenterPoint.z,
            step_down,
            tool_diameter,
            direction,
            startAt,
        )
    )

    if type(outer_radius) not in [float, int]:
        raise TypeError("Invalid type for outer radius")

    if outer_radius <= 0:
        raise ValueError("outer_radius <= 0")

    if inner_radius and type(inner_radius) not in [float, int]:
        raise TypeError("inner_radius must be a float")

    if not inner_radius or inner_radius > outer_radius:
        # Do not overlap outer and inner helix
        inner_radius = outer_radius

    if type(tool_diameter) not in [float, int]:
        raise TypeError("tool_diameter must be a float")
    tool_radius = tool_diameter / 2

    if startAt not in ["Inside", "Outside"]:
        raise ValueError("Invalid value for parameter 'startAt'")

    if direction not in ["CW", "CCW"]:
        raise ValueError("Invalid value for parameter 'direction'")

    if type(step_over) not in [float, int]:
        raise TypeError("Invalid value for parameter 'step_over'")

    if step_over < 0:
        raise ValueError("Invalid value for parameter 'step_over'")

    if not (
        isclose(topCenterPoint.sub(bottomCenterPoint).x, 0, rtol=1e-05, atol=1e-06)
        and (isclose(topCenterPoint.sub(bottomCenterPoint).y, 0, rtol=1e-05, atol=1e-06))
    ):
        raise ValueError("edge is not aligned with Z axis")

    if topCenterPoint.z < bottomCenterPoint.z:
        raise ValueError("start point is below end point")

    if outer_radius <= inner_radius:
        Path.Log.debug("(single helix mode)\n")
        radii = [outer_radius]
    else:
        Path.Log.debug("(annulus mode)\n")
        work_distance = outer_radius - inner_radius
        nr = int(ceil(work_distance / step_over)) + 1
        radii = linspace(outer_radius, inner_radius, nr)

    if startAt == "Inside":
        # reverse order if going from inside to outside
        radii = radii[::-1]

    Path.Log.debug("Radii: {}".format(radii))
    # calculate the number of full and partial turns required
    # Each full turn is two 180 degree arcs. Zsteps is equally spaced step
    # down values
    helixHeight = topCenterPoint.z - bottomCenterPoint.z
    turncount = int(ceil(helixHeight / step_down))
    zsteps = linspace(topCenterPoint.z, bottomCenterPoint.z, 2 * turncount + 1)

    # simple vertical helix
    def helix_vertical(r):
        commandlist = []
        arc_cmd = "G2" if direction == "CW" else "G3"
        commandlist.append(Path.Command("G0", {"X": topCenterPoint.x + r, "Y": topCenterPoint.y}))
        commandlist.append(Path.Command("G1", {"Z": topCenterPoint.z}))
        for i in range(1, turncount + 1):
            # first half turn arc
            commandlist.append(
                Path.Command(
                    arc_cmd,
                    {
                        "X": topCenterPoint.x - r,
                        "Y": topCenterPoint.y,
                        "Z": zsteps[2 * i - 1],
                        "I": -r,
                        "J": 0.0,
                    },
                )
            )
            # second half turn arc
            commandlist.append(
                Path.Command(
                    arc_cmd,
                    {
                        "X": topCenterPoint.x + r,
                        "Y": topCenterPoint.y,
                        "Z": zsteps[2 * i],
                        "I": r,
                        "J": 0.0,
                    },
                )
            )
        if finish_circle:
            # add finish circle by two 180 degree arcs
            commandlist.append(
                Path.Command(
                    arc_cmd,
                    {
                        "X": topCenterPoint.x - r,
                        "Y": topCenterPoint.y,
                        "Z": bottomCenterPoint.z,
                        "I": -r,
                        "J": 0.0,
                    },
                )
            )
            commandlist.append(
                Path.Command(
                    arc_cmd,
                    {
                        "X": topCenterPoint.x + r,
                        "Y": topCenterPoint.y,
                        "Z": bottomCenterPoint.z,
                        "I": r,
                        "J": 0.0,
                    },
                )
            )

        return commandlist

    # cone helix inclined by angle
    def helix_cone(bottomRadius):
        topRadius = bottomRadius + math.tan(math.radians(cone_angle)) * helixHeight
        commandlist = []
        arcCmdName = "G2" if direction == "CW" else "G3"
        commandlist.append(
            Path.Command("G0", {"X": topCenterPoint.x + topRadius, "Y": topCenterPoint.y})
        )
        commandlist.append(Path.Command("G1", {"Z": topCenterPoint.z}))
        stepRotate = math.pi / 3  # step size for rotate
        stepRotate = -stepRotate if direction == "CCW" else stepRotate
        iters = int(2 * math.pi * turncount / abs(stepRotate))
        stepRadius = (topRadius - bottomRadius) / iters  # step size for spiral radius
        stepZ = helixHeight / iters
        count = 0
        angle = math.pi / 2
        arcR = topRadius
        arcZ = topCenterPoint.z
        arcPoints = [FreeCAD.Vector(topCenterPoint.x + arcR, topCenterPoint.y, 0)]
        while count < iters:
            angle += stepRotate
            arcR -= stepRadius
            arcZ -= stepZ
            arcX = topCenterPoint.x + arcR * math.sin(angle)
            arcY = topCenterPoint.y + arcR * math.cos(angle)
            arcPoints.append(FreeCAD.Vector(arcX, arcY, arcZ))  # Get all points of arcs
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

        # Add finish full circle by two 180 degree arcs
        if finish_circle:
            p1 = FreeCAD.Vector(topCenterPoint.x - arcR, topCenterPoint.y, 0)
            commandlist.append(Path.Command(arcCmdName, {"X": p1.x, "Y": p1.y, "I": -arcR, "J": 0}))
            p2 = FreeCAD.Vector(topCenterPoint.x + arcR, topCenterPoint.y, 0)
            commandlist.append(Path.Command(arcCmdName, {"X": p2.x, "Y": p2.y, "I": arcR, "J": 0}))

        return commandlist

    def getArcCenter(p1, p2, p3):
        # Calculate arc center by three points on arc
        # https://paulbourke.net/geometry/circlesphere
        ma = (p2.y - p1.y) / (p2.x - p1.x)
        mb = (p3.y - p2.y) / (p3.x - p2.x)
        arcCenter = FreeCAD.Vector()
        arcCenter.x = (ma * mb * (p1.y - p3.y) + mb * (p1.x + p2.x) - ma * (p2.x + p3.x)) / (
            2 * (mb - ma)
        )
        arcCenter.y = -1 * (arcCenter.x - (p1.x + p2.x) / 2) / ma + (p1.y + p2.y) / 2

        return arcCenter

    def retract(r, last_step):
        retractcommands = []
        retract_offset = 0
        if not last_step and r <= tool_radius:
            # this is first helix which clearing center
            retract_offset = -min(tool_radius / 2, r)

        elif last_step and startAt == "Inside":
            retract_offset = -min(tool_radius / 2, last_step / 2)

        elif last_step and startAt == "Outside" and r > tool_radius:
            retract_offset = min(tool_radius / 2, last_step / 2)

        if retract_offset:
            # move from wall and to retract height
            retractcommands.append(
                Path.Command(
                    "G0",
                    {
                        "X": bottomCenterPoint.x + r + retract_offset,
                        "Y": bottomCenterPoint.y,
                        "Z": retract_height,
                    },
                )
            )
        else:
            # move to retract height
            retractcommands.append(Path.Command("G0", {"Z": retract_height}))

        return retractcommands

    commands = []
    commands.append(Path.Command("G0", {"Z": retract_height}))
    for i, r in enumerate(radii):
        if cone_angle:
            commands.extend(helix_cone(r))
        else:
            commands.extend(helix_vertical(r))

        # last real step over to limit horizontal move while retract
        last_step = abs(radii[i] - radii[i - 1]) if i > 0 else 0

        commands.extend(retract(r, last_step))

    return commands
