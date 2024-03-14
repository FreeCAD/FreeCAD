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
    hole_radius,
    step_down,
    step_over,
    tool_diameter,
    inner_radius=0.0,
    direction="CW",
    startAt="Outside",
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
        )
    )

    # inner_radius contains not a radius but the value from Extra Offset, which is the distance between the hole radius as designed and the hole radius to be cut.
    # hole_radius contains the designed hole radius - inner_radius.

    if type(hole_radius) not in [float, int]:
        raise TypeError("Invalid type for hole radius")

    if hole_radius < 0.0:
        raise ValueError("hole_radius < 0")

    if type(inner_radius) not in [float, int]:
        raise TypeError("inner_radius must be a float")

    if type(tool_diameter) not in [float, int]:
        raise TypeError("tool_diameter must be a float")

    if not hole_radius * 2 > tool_diameter:
        raise ValueError(
            "Cannot helix a hole of diameter {0} with a tool of diameter {1}".format(
                2 * hole_radius, tool_diameter
            )
        )

    elif startAt not in ["Inside", "Outside"]:
        raise ValueError("Invalid value for parameter 'startAt'")

    elif direction not in ["CW", "CCW"]:
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

    if hole_radius <= tool_diameter:
        Path.Log.debug("(single helix mode)\n")
        radii = [hole_radius - tool_diameter / 2]
        if radii[0] <= 0:
            raise ValueError(
                "Cannot helix a hole of diameter {0} with a tool of diameter {1}".format(
                    2 * hole_radius, tool_diameter
                )
            )
        outer_radius = hole_radius

    else:  # inner_radius > 0:
        Path.Log.debug("(annulus mode / full hole)\n")
        outer_radius = hole_radius - tool_diameter / 2
        step_radius = inner_radius + tool_diameter / 2
        if abs((outer_radius - step_radius) / step_over_distance) < 1e-5:
            radii = [(outer_radius + inner_radius) / 2]
        else:
            nr = max(int(ceil((outer_radius - inner_radius) / step_over_distance)), 2)
            radii = linspace(outer_radius, step_radius, nr)

    Path.Log.debug("Radii: {}".format(radii))
    # calculate the number of full and partial turns required
    # Each full turn is two 180 degree arcs. Zsteps is equally spaced step
    # down values
    turncount = max(int(ceil((startPoint.z - endPoint.z) / step_down)), 2)
    zsteps = linspace(startPoint.z, endPoint.z, 2 * turncount + 1)

    def helix_cut_r(r):
        commandlist = []
        arc_cmd = "G2" if direction == "CW" else "G3"
        commandlist.append(
            Path.Command("G0", {"X": startPoint.x + r, "Y": startPoint.y})
        )
        commandlist.append(Path.Command("G1", {"Z": startPoint.z}))
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
        return commandlist

    def retract():
        # try to move to a safe place to retract without leaving a dwell
        # mark
        retractcommands = []
        # Calculate retraction
        if hole_radius <= tool_diameter:  # simple case where center is clear
            center_clear = True

        elif startAt == "Inside" and inner_radius == 0.0:  # middle is clear
            center_clear = True
        else:
            center_clear = False

        if center_clear:
            retractcommands.append(
                Path.Command("G0", {"X": endPoint.x, "Y": endPoint.y, "Z": endPoint.z})
            )

        # Technical Debt.
        # If the operation is clearing multiple passes in annulus mode (inner
        # radius > 0.0 and len(radii) > 1) then there is a derivable
        # safe place which does not touch the inner or outer wall on all radii except
        # the first.  This is left as a future improvement.

        retractcommands.append(Path.Command("G0", {"Z": startPoint.z}))

        return retractcommands

    if startAt == "Inside":
        radii = radii[::-1]

    commands = []
    for r in radii:
        commands.extend(helix_cut_r(r))
        commands.extend(retract())

    return commands
