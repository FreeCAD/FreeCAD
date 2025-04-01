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


from numpy import ceil, linspace, isclose, delete
import Path
import math

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
    hole_radius,
    step_down,
    step_over,
    tool_diameter,
    inner_radius=0.0,
    direction="CW",
    startAt="Outside",
    feedRateAdj=False,
):
    """generate(edge, hole_radius, inner_radius, step_over) ... generate helix commands.
    hole_radius, inner_radius: outer and inner radius of the hole
    step_over: step over % of tool diameter"""

    startPoint = edge.Vertexes[0].Point
    endPoint = edge.Vertexes[1].Point

    Path.Log.track(
        "(helix: <{}, {}>\n hole radius {}\n inner radius {}\n step over {}\n start point {}\n end point {}\n step_down {}\n tool diameter {}\n direction {}\n startAt {})".format(
            startPoint.x,
            startPoint.y,
            hole_radius,
            inner_radius,
            step_over,
            startPoint.z,
            endPoint.z,
            step_down,
            tool_diameter,
            direction,
            startAt,
            feedRateAdj,
        )
    )

    # hole_radius contains the designed hole radius - OffsetExtra.
    # inner_radius contains the start radius + OffsetExtra.

    if type(hole_radius) not in [float, int]:
        raise TypeError("Invalid type for hole radius")

    if hole_radius < 0.0:
        raise ValueError("hole_radius < 0")

    if type(inner_radius) not in [float, int]:
        raise TypeError("inner_radius must be a float")

    if inner_radius < -tool_diameter / 2:
        # inner_radius also depends on StartRadius
        raise ValueError(
            "inner_radius {0} with a tool of diameter {1} gives a negative helix radius !".format(
                inner_radius, tool_diameter
            )
        )

    if type(tool_diameter) not in [float, int]:
        raise TypeError("tool_diameter must be a float")

    if not hole_radius * 2 >= tool_diameter:  # will allow plunge cut, zero radius helix
        raise ValueError(
            "Cannot helix a hole of diameter {0} with a tool of diameter {1}".format(
                2 * hole_radius, tool_diameter
            )
        )

    if startAt not in ["Inside", "Outside"]:
        raise ValueError("Invalid value for parameter 'startAt'")

    if direction not in ["CW", "CCW"]:
        raise ValueError("Invalid value for parameter 'direction'")

    if type(step_over) not in [float, int]:
        raise TypeError("Invalid value for parameter 'step_over'")

    if step_over <= 0 or step_over > 1:
        raise ValueError("Invalid value for parameter 'step_over'")
    step_over_distance = step_over * tool_diameter

    if not (
        isclose(startPoint.sub(endPoint).x, 0, rtol=1e-05, atol=1e-06)
        and (isclose(startPoint.sub(endPoint).y, 0, rtol=1e-05, atol=1e-06))
    ):
        raise ValueError("edge is not aligned with Z axis")

    if startPoint.z < endPoint.z:
        raise ValueError("start point is below end point")

    tool_radius = tool_diameter / 2
    if hole_radius <= tool_diameter:
        Path.Log.debug("(single helix path)\n")
        radii = [hole_radius - tool_radius]
        if radii[0] < 0:
            raise ValueError(
                "Cannot creatre a helical path of diameter {0} with a tool of diameter {1}".format(
                    2 * hole_radius, tool_diameter
                )
            )
    else:  # inner_radius >= 0:
        Path.Log.debug("(annular clearance or entire hole)\n")
        outer_path_radius = hole_radius - tool_radius
        inner_path_radius = inner_radius + tool_radius

        if abs((outer_path_radius - inner_path_radius) / step_over_distance) < 1e-5:
            radii = [outer_path_radius]  # below tolerance cutoff, just use outer radius
        else:
            nr = int(ceil((outer_path_radius - inner_path_radius) / step_over_distance) + 1)
            radii = linspace(outer_path_radius, inner_path_radius, nr)
            if (startAt == "Outside") and (inner_radius <= 0):
                radii = delete(radii, -1)  # cutting air. useful for spacing

    Path.Log.debug("Radii: {}".format(radii))
    # calculate the number of full and partial turns required
    # Each full turn is two 180 degree arcs. Zsteps is equally spaced step
    # down values
    turncount = max(int(ceil((startPoint.z - endPoint.z) / step_down)), 2)
    zsteps = linspace(startPoint.z, endPoint.z, 2 * turncount + 1)

    def helix_cut_r(r):
        commandlist = []
        arc_cmd = "G2" if direction == "CW" else "G3"
        commandlist.append(Path.Command("G0", {"X": startPoint.x + r, "Y": startPoint.y}))
        commandlist.append(Path.Command("G1", {"Z": startPoint.z}))

        # FeedRateCheckbox.checked
        if not feedRateAdj:
            feedRateRatio = 0
        else:
            if (startAt == "Inside") or (
                r == radii[0]
            ):  # first cut, always adj for outer wall chip-load
                feedRateRatio = r / (
                    r + tool_radius
                )  # classic (hole_rad-tool_rad)/hole_rad adjustment
            else:
                if tool_radius > 200 * r:
                    feedRateRatio = 200  # prevent div zero
                else:
                    feedRateRatio = (
                        r + tool_radius
                    ) / r  # startAt outside: increase spindle feed rate to maintain inner-cut chip-load

        # Note: if (r == -tool_radius), helix is plugne, hence VH_gradient==0; feedrate==vFeed , OK
        for i in range(1, turncount + 1):
            commandlist.append(
                Path.Command(
                    arc_cmd,
                    {
                        "X": startPoint.x - r,
                        "Y": startPoint.y,
                        "Z": zsteps[2 * i - 1],
                        "I": -r,
                        "J": 0.0,
                    },
                )
            )
            if feedRateAdj:
                commandlist[-1].Parameters["FeedFactor"] = feedRateRatio
            commandlist.append(
                Path.Command(
                    arc_cmd,
                    {
                        "X": startPoint.x + r,
                        "Y": startPoint.y,
                        "Z": zsteps[2 * i],
                        "I": r,
                        "J": 0.0,
                    },
                )
            )
            if feedRateAdj:
                commandlist[-1].Parameters["FeedFactor"] = feedRateRatio
        commandlist.append(
            Path.Command(
                arc_cmd,
                {
                    "X": startPoint.x - r,
                    "Y": startPoint.y,
                    "Z": endPoint.z,
                    "I": -r,
                    "J": 0.0,
                },
            )
        )
        if feedRateAdj:
            commandlist[-1].Parameters["FeedFactor"] = feedRateRatio
        commandlist.append(
            Path.Command(
                arc_cmd,
                {
                    "X": startPoint.x + r,
                    "Y": startPoint.y,
                    "Z": endPoint.z,
                    "I": r,
                    "J": 0.0,
                },
            )
        )
        if feedRateAdj:
            commandlist[-1].Parameters["FeedFactor"] = feedRateRatio
        return commandlist

    def retract():
        # try to move to a safe place to retract without leaving a dwell mark
        retractcommands = []
        if tool_diameter >= hole_radius:
            center_clear = True  # single cut operation
        else:
            center_clear = (startAt == "Inside") and (inner_radius <= 0.0)
            # hole centre is already clear (hole inner radius<=tool_radius)

        # use G1 since tool tip still in contact with workpiece.
        if center_clear and (prev_r is None):
            retractcommands.append(Path.Command("G1", {"X": endPoint.x, "Y": endPoint.y}))
        elif prev_r is not None:
            dwell_r = (r + prev_r) / 2
            retractcommands.append(
                Path.Command(
                    "G1",
                    {
                        "X": endPoint.x + dwell_r,
                        "Y": endPoint.y,
                    },
                )
            )
            # else annular slot, no pulloff, just retract along wall

        retractcommands.append(Path.Command("G0", {"Z": startPoint.z}))

        return retractcommands

    if startAt == "Inside":
        radii = radii[::-1]

    commands = []
    prev_r = None
    for r in radii:
        commands.extend(helix_cut_r(r))
        commands.extend(retract())
        prev_r = r
    return commands
