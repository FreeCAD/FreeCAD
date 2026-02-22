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


import Path
from Path.Geom import isRoughly
from Path.Geom import pointsCoincide

import copy
import math


"""
import Path.Base.Generator.ramp_entry_helix as helix
helix_commands = helix.Helix(commands, [maxStepDown, tc, ignoreAbove])
helix_commands.generate()

Arguments:
    commands:    list of Path.Command objects

    maxStepDown: maximum height for each helix turn (optional)
                 if not set, create only one helix turn

    tc:          tool controller to apply feed rate (optional)

    ignoreAbove: helix ramp will start from this height (optional)
                 upper path will stay without modifications
"""


if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


class AnnotatedGCode:
    def __init__(self, command, start_point):
        self.start_point = start_point
        self.command = command
        self.end_point = (
            command.Parameters.get("X", start_point[0]),
            command.Parameters.get("Y", start_point[1]),
            command.Parameters.get("Z", start_point[2]),
        )
        self.is_line = command.Name in Path.Geom.CmdMoveStraight
        self.is_arc = command.Name in Path.Geom.CmdMoveArc
        self.xy_length = None
        if self.is_line:
            self.xy_length = (
                (start_point[0] - self.end_point[0]) ** 2
                + (start_point[1] - self.end_point[1]) ** 2
            ) ** 0.5
        elif self.is_arc:
            self.center_xy = (
                start_point[0] + command.Parameters.get("I", 0),
                start_point[1] + command.Parameters.get("J", 0),
            )
            self.start_angle = math.atan2(
                start_point[1] - self.center_xy[1],
                start_point[0] - self.center_xy[0],
            )
            self.end_angle = math.atan2(
                self.end_point[1] - self.center_xy[1],
                self.end_point[0] - self.center_xy[0],
            )
            if self.command.Name in Path.Geom.CmdMoveCCW and self.end_angle < self.start_angle:
                self.end_angle += 2 * math.pi
            if self.command.Name in Path.Geom.CmdMoveCW and self.end_angle > self.start_angle:
                self.end_angle -= 2 * math.pi
            self.radius = (
                (start_point[0] - self.center_xy[0]) ** 2
                + (start_point[1] - self.center_xy[1]) ** 2
            ) ** 0.5
            self.xy_length = self.radius * abs(self.end_angle - self.start_angle)

    def clone(self, z_start=None, z_end=None, reverse=False):
        """Makes a copy of this annotated gcode at the given z height"""
        z_start = z_start if z_start is not None else self.start_point[2]
        z_end = z_end if z_end is not None else self.end_point[2]

        other = copy.copy(self)
        otherParams = copy.copy(self.command.Parameters)
        otherCommandName = self.command.Name
        other.start_point = (self.start_point[0], self.start_point[1], z_start)
        other.end_point = (self.end_point[0], self.end_point[1], z_end)
        otherParams.update({"Z": z_end})
        if reverse:
            other.start_point, other.end_point = other.end_point, other.start_point
            params = {"X": other.end_point[0], "Y": other.end_point[1], "Z": other.end_point[2]}
            otherParams.update(params)
            if other.is_arc:
                other.start_angle, other.end_angle = other.end_angle, other.start_angle
                otherCommandName = "G2" if self.command.Name in Path.Geom.CmdMoveCCW else "G3"
                otherParams.update(
                    {
                        "I": other.center_xy[0] - other.start_point[0],
                        "J": other.center_xy[1] - other.start_point[1],
                    }
                )
        other.command = Path.Command(otherCommandName, otherParams)

        return other


class Helix:
    def __init__(self, commands, maxStepDown=None, tc=None, ignoreAbove=None):
        self.commands = commands
        self.tc = tc
        self.ignoreAbove = ignoreAbove
        self.maxStepDown = maxStepDown

    def checks(self):
        if any(not isinstance(cmd, Path.Command) for cmd in self.commands[:10]):
            raise TypeError("'commands' must be a list of Path.Command objects")

        if self.maxStepDown is not None:
            if not isinstance(self.maxStepDown, (float, int)):
                raise TypeError("'maxStepDown' must be a int or float")
            if self.maxStepDown < 0:
                raise ValueError("'maxStepDown' < 0")

        if self.tc is not None:
            if not getattr(self.tc, "Proxy", None) or not isinstance(
                self.tc.Proxy, Path.Tool.Controller.ToolController
            ):
                raise TypeError("'tc' must be a tool controller object")
            if not self.tc.HorizFeed.Value:
                raise ValueError("'HorizFeed' is 0")
            if not self.tc.VertFeed.Value:
                raise ValueError("'VertFeed' is 0")
            if not self.tc.RampFeed.Value:
                raise ValueError("'RampFeed' is 0")

        if self.ignoreAbove is not None and not isinstance(self.ignoreAbove, (float, int)):
            raise TypeError("'ignoreAbove' must be a int or float")

    def generate(self):
        self.checks()

        self.edges = []
        start_point = (0, 0, 0)
        last_params = {}
        for cmd in self.commands:
            # Skip repeat move commands
            params = cmd.Parameters
            if (
                cmd.Name in Path.Geom.CmdMoveAll
                and self.edges
                and cmd.Name == self.edges[-1].command.Name
            ):
                found_diff = False
                for k, v in params.items():
                    if last_params.get(k, None) != v:
                        found_diff = True
                        break
                if not found_diff:
                    continue

            last_params.update(params)

            annotated = AnnotatedGCode(cmd, start_point)
            self.edges.append(annotated)
            start_point = annotated.end_point

        self.outedges = self.generateHelix()

        return self.createCommands(self.outedges)

    def generateHelix(self):
        edges = self.edges
        minZ = self.findMinZ(edges)
        Path.Log.debug("Minimum Z in this path is {}".format(minZ))
        outedges = []
        i = 0
        while i < len(edges):
            edge = edges[i]
            if edge.is_line or edge.is_arc:
                if isRoughly(edge.xy_length, 0) and edge.end_point[2] < edge.start_point[2]:
                    noramp_edge, edge = self.processIgnoreAbove(edge)
                    if noramp_edge is not None:
                        outedges.append(noramp_edge)
                    if edge is None:
                        i += 1
                        continue

                    # next need to find a loop
                    loopFound = False
                    rampedges = []
                    j = i + 1
                    while j < len(edges) and not loopFound:
                        candidate = edges[j]
                        if not isRoughly(candidate.start_point[2], candidate.end_point[2]):
                            # this edge is not parallel to XY plane, not qualified for ramping.
                            # exit early, no loop found
                            break
                        if pointsCoincide(edge.end_point, candidate.end_point):
                            loopFound = True
                        rampedges.append(candidate)
                        j += 1
                    if not loopFound:
                        Path.Log.warning("No suitable helix found, leaving as a plunge")
                        outedges.append(edge)
                    else:
                        outedges.extend(self.createHelix(rampedges, edge.start_point[2]))
                        if not isRoughly(edge.end_point[2], minZ):
                            # the edges covered by the helix not handled again,
                            # unless reached the bottom height
                            i = j - 1

                else:
                    outedges.append(edge)
            else:
                outedges.append(edge)
            i += 1

        return outedges

    def processIgnoreAbove(self, edge):
        """Edges, or parts of edges, above self.ignoreAbove should not be ramped.
        This method is a helper for splitting edges into a portion that should be
        ramped and a portion that should not be ramped.
        Returns (noramp_edge, ramp_edge). Either of these variables may be None"""
        if self.ignoreAbove is None:
            return None, edge

        z0, z1 = edge.start_point[2], edge.end_point[2]
        if z0 > self.ignoreAbove:
            if z1 > self.ignoreAbove or isRoughly(z1, self.ignoreAbove):
                # Entire plunge is above ignoreAbove
                return edge, None
            elif not isRoughly(z0, self.ignoreAbove):
                # Split the edge into regions above and below
                return (
                    edge.clone(z0, self.ignoreAbove),
                    edge.clone(self.ignoreAbove, z1),
                )

        # Entire plunge is below ignoreAbove
        return None, edge

    def createHelix(self, rampedges, startZ):
        outedges = []
        ramplen = 0
        for redge in rampedges:
            ramplen += redge.xy_length
        rampheight = abs(startZ - rampedges[-1].end_point[2])
        if not self.maxStepDown or isRoughly(rampheight, self.maxStepDown):
            num_loops = 1
        else:
            num_loops = math.ceil(rampheight / self.maxStepDown)
        rampedges *= num_loops
        ramplen *= num_loops

        rampangle_rad = math.atan(ramplen / rampheight)
        curZ = startZ
        for i, redge in enumerate(rampedges):
            deltaZ = redge.xy_length / math.tan(rampangle_rad)
            # compute new z, or clamp to end segment to avoid rounding error
            newZ = curZ - deltaZ if i < len(rampedges) - 1 else rampedges[-1].end_point[2]
            outedges.append(redge.clone(curZ, newZ))
            curZ = newZ

        return outedges

    def findMinZ(self, edges):
        minZ = None
        for edge in edges:
            if edge.command.Name in Path.Geom.CmdMoveAll:
                if minZ is None or edge.end_point[2] < minZ:
                    minZ = edge.end_point[2]

        return minZ

    def createCommands(self, edges):
        commands = [edge.command for edge in edges]
        outCommands = []

        lastX = lastY = lastZ = 0
        for cmd in commands:
            params = cmd.Parameters
            x = params.get("X", lastX)
            y = params.get("Y", lastY)
            z = params.get("Z", lastZ)

            if self.tc:
                # set feed rate
                if cmd.Name in Path.Geom.CmdMoveMill:
                    if not isRoughly(lastZ, z):
                        if pointsCoincide((x, y), (lastX, lastY)):
                            params["F"] = self.tc.VertFeed.Value
                        else:
                            params["F"] = self.tc.RampFeed.Value
                    else:
                        params["F"] = self.tc.HorizFeed.Value

                elif cmd.Name in Path.Geom.CmdMoveRapid:
                    if not isRoughly(lastZ, z) and self.tc.VertRapid.Value:
                        params["F"] = self.tc.VertRapid.Value
                    elif self.tc.HorizRapid.Value:
                        params["F"] = self.tc.HorizRapid.Value

            lastX, lastY, lastZ = x, y, z

            outCommands.append(Path.Command(cmd.Name, params))

        return outCommands
