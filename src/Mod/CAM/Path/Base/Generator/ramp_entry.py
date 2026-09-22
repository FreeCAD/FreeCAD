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

if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


class RampEntry:
    """
    Generator ramp enty

    Example of uses:
    from Path.Base.Generator.ramp_entry import RampEntry
    ramp_entry = RampEntry(commands)
    ramp_entry_commands = ramp_entry.generate()

    Parameters:
    - commands:    List of Path.Command objects of closed profile
    - method:      0 - Helix, 1 - Ramp1, 2 - Ramp2, 3 - Ramp3
    - angle_rad:   Maximum angle of the ramp entry (radians)
    - pitch:       Maximum height for each helix turn (mm)
                   If angle_rad and pitch not set for Helix, create only one helix turn
    - tc:          Tool controller to apply feed rate (optional)
    - ignoreAbove: Helix ramp will start from this height (optional)
                   Upper path will stay without modifications

              1 |                          2 |                              3 |
         plunge |                     plunge |                         plunge |
    start depth v                start depth v--->                start depth v
               /                                 /                           /
        ramp  /                           ramp  /                     ramp  /
             /                                 /                            \
            /--->                             /                       ramp   \

    Ramp Method 0
    - Helix like path
    - Can be applied only to closed path

    Ramp Method 1
    1. Start from the original startpoint of the plunge
    2. Ramp down along the path that comes after the plunge
    3. When reaching the Z level of the original plunge, return back to the beginning
        by going the path backwards until the original plunge end point is reached
    4. Continue with the original path

    Ramp Method 2
    1. Start from the original startpoint of the plunge
    2. Travel at start depth along the path, for a distance required for step 3
    3. Ramp backwards along the path at rampangle, arriving exactly at the bottom of the plunge
    4. Continue with the original path

    Ramp Method 3
    1. Start from the original startpoint of the plunge
    2. Ramp down along the path that comes after the plunge until
        traveled half of the Z distance
    3. Change direction and ramp backwards to the original plunge end point
    4. Continue with the original path
    """

    def __init__(self, commands, method=0, angle_rad=None, pitch=None, tc=None, ignoreAbove=None):
        self.commands = commands
        self.method = method
        self.angle = angle_rad
        self.pitch = pitch
        self.tc = tc
        self.ignoreAbove = ignoreAbove
        self.checks()

    def checks(self):
        if any(not isinstance(cmd, Path.Command) for cmd in self.commands[:10]):
            raise TypeError("'commands' must be a list of Path.Command objects")

        if not isinstance(self.method, int):
            raise TypeError("'method' must be a int")

        if self.method < 0 or self.method > 3:
            raise ValueError("'method' must be 0, 1, 2 or 3")

        if self.angle is not None:
            if not isinstance(self.angle, (float, int)):
                raise TypeError("'angle' must be a int or float")
            if self.angle < 0:
                raise ValueError("'angle' < 0")

        if self.pitch is not None:
            if not isinstance(self.pitch, (float, int)):
                raise TypeError("'pitch' must be a int or float")
            if self.pitch < 0:
                raise ValueError("'pitch' < 0")

        if self.tc is not None:
            if not getattr(self.tc, "Proxy", None) or not isinstance(
                self.tc.Proxy, Path.Tool.Controller.ToolController
            ):
                raise TypeError("'tc' must be a tool controller object")
            Path.Log.debug("tool controller: {}".format(self.tc.Name))
            if not self.tc.HorizFeed.Value:
                raise ValueError("'HorizFeed' is 0")
            if not self.tc.VertFeed.Value:
                raise ValueError("'VertFeed' is 0")
            if not self.tc.RampFeed.Value:
                raise ValueError("'RampFeed' is 0")

        if self.ignoreAbove is not None and not isinstance(self.ignoreAbove, (float, int)):
            raise TypeError("'ignoreAbove' must be a int or float")
        Path.Log.debug("ignoreAboveZ: {}".format(self.ignoreAbove))

    def generate(self):
        self.edges = []
        last_params = {}
        for cmd in self.commands:
            params = cmd.Parameters
            if (
                cmd.Name in Path.Geom.CmdMoveAll
                and self.edges
                and cmd.Name == self.edges[-1].command.Name
            ):  # skip repeat move command
                if all(last_params.get(k, None) == v for k, v in params.items()):
                    continue

            start_point = self.edges[-1].end_point if self.edges else (0, 0, 0)
            edge = AnnotatedGCode(cmd, start_point)
            last_params.update(params)

            if (
                self.edges
                and edge.is_line
                and self.edges[-1].is_line
                and Path.Geom.isRoughly(self.edges[-1].xy_length, 0)
                and Path.Geom.isRoughly(edge.xy_length, 0)
                and self.edges[-1].end_point[2] < self.edges[-1].start_point[2]
                and edge.end_point[2] < edge.start_point[2]
            ):  # combine plunge commands
                self.edges[-1] = AnnotatedGCode(cmd, self.edges[-1].start_point)
                continue

            self.edges.append(edge)

        if self.method:
            outedges = self.generateRamps()
        else:
            outedges = self.generateHelix()

        return self.createCommands(outedges)

    def generateHelix(self):
        edges = self.edges
        minZ = min(e.end_point[2] for e in edges if e.command.Name in Path.Geom.CmdMoveAll)
        Path.Log.debug("Minimum Z in this path is {}".format(minZ))
        outedges = []
        i = 0
        while i < len(edges):
            edge = edges[i]
            if edge.is_line or edge.is_arc:  # this is plunge edge
                if isRoughly(edge.xy_length, 0) and edge.end_point[2] < edge.start_point[2]:
                    # check if edge is above ignore height
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
                        Path.Log.info(
                            "No suitable helix found, leaving as a plunge: %s" % edge.command
                        )
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

    def createHelix(self, rampedges, startZ):
        outedges = []

        ramplen = sum(redge.xy_length for redge in rampedges)
        rampheight = abs(startZ - rampedges[-1].end_point[2])

        num_loops_from_pitch = 1
        num_loops_from_angle = 1
        if self.pitch:
            num_loops_from_pitch = Path.Geom.ceil(rampheight / self.pitch)
        if self.angle:
            num_loops_from_angle = Path.Geom.ceil(rampheight / ramplen / math.tan(self.angle))

        num_loops = max(num_loops_from_pitch, num_loops_from_angle)
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

    def generateRamps(self):
        edges = self.edges
        outedges = []
        for edgei, edge in enumerate(edges):
            if edge.is_line or edge.is_arc:
                # check for plunge
                if edge.xy_length < 1e-6 and edge.end_point[2] < edge.start_point[2]:
                    # check if above ignoreAbove parameter - do not generate ramp if it is
                    noramp_edge, edge = self.processIgnoreAbove(edge)
                    if noramp_edge is not None:
                        outedges.append(noramp_edge)
                    if edge is None:
                        continue

                    plungelen = abs(edge.start_point[2] - edge.end_point[2])
                    # length of the forthcoming ramp projected to XY plane
                    projectionlen = plungelen / math.tan(self.angle)
                    # Path.Log.debug(
                    #    "Found plunge move at X:{} Y:{} From Z:{} to Z{}, length of ramp: {}".format(
                    #        p0.x, p0.y, p0.z, p1.z, projectionlen
                    #    )
                    # )
                    if self.method == 3:
                        projectionlen = projectionlen / 2

                    # next need to determine how many edges in the path after
                    # plunge are needed to cover the length:
                    covered = False
                    coveredlen = 0
                    rampedges = []
                    i = edgei + 1
                    while not covered and i < len(edges):
                        candidate = edges[i]
                        if abs(candidate.start_point[2] - candidate.end_point[2]) > 1e-6 or (
                            not candidate.is_line and not candidate.is_arc
                        ):
                            # this edge is not an edge/arc in the XY plane; not qualified for ramping
                            break
                        # Path.Log.debug("Next edge length {}".format(candidate.Length))
                        rampedges.append(candidate)
                        coveredlen = coveredlen + candidate.xy_length

                        if coveredlen > projectionlen:
                            covered = True
                        i += 1
                    if len(rampedges) == 0:
                        Path.Log.info("No suitable edges for ramping, plunge will remain as such")
                        outedges.append(edge)
                    else:
                        # Path.Log.debug("Doing ramp to edges: {}".format(rampedges))
                        if self.method == 1:
                            outedges.extend(
                                self.createRampMethod1(
                                    rampedges, edge.start_point, projectionlen, self.angle
                                )
                            )
                        elif self.method == 2:
                            outedges.extend(
                                self.createRampMethod2(
                                    rampedges, edge.start_point, projectionlen, self.angle
                                )
                            )
                        else:
                            # if the ramp cannot be covered with Method3, revert to Method1
                            # because Method1 support going back-and-forth and thus results in same path as Method3 when
                            # length of the ramp is smaller than needed for single ramp.
                            if not covered:
                                projectionlen = projectionlen * 2
                                outedges.extend(
                                    self.createRampMethod1(
                                        rampedges, edge.start_point, projectionlen, self.angle
                                    )
                                )
                            else:
                                outedges.extend(
                                    self.createRampMethod3(
                                        rampedges, edge.start_point, projectionlen, self.angle
                                    )
                                )
                else:
                    outedges.append(edge)
            else:
                outedges.append(edge)
        return outedges

    def _createRampHelper(self, rampedges, p0, projectionlen, rampangle):
        """
        Helper method for generating ramps. Computes ramp method 1, but returns the result in pieces to allow for implementing the other ramp methods.
        Returns (ramp, reset)
        - ramp: array of commands ramping down
        - reset: array of commands returning from the bottom of the ramp to the bottom of the original plunge
        """
        ramp = []
        reset = []
        reversed_edges = [redge.clone(reverse=True) for redge in rampedges]
        rampremaining = projectionlen
        z = p0[2]  # start from the upper point of plunge
        goingForward = True
        i = 0  # current position = start of this edge. May be len(rampremaining) if going backwards
        while rampremaining > 0:
            redge = rampedges[i] if goingForward else reversed_edges[i - 1]

            # for i, redge in enumerate(rampedges):
            if redge.xy_length > rampremaining:
                # will reach end of ramp within this edge, needs to be split
                split_first, split_remaining = redge.split(rampremaining)
                ramp.append(split_first.clone(z_start=z))
                # now we have reached the end of the ramp. Go back to plunge position with constant Z
                # start that by going to the beginning of this splitEdge
                if goingForward:
                    reset.append(split_first.clone(reverse=True))
                else:
                    # if we were reversing, we continue to the same direction as the ramp
                    reset.append(split_remaining)
                    i -= 1
                break
            else:
                deltaZ = redge.xy_length * math.tan(rampangle)
                new_z = z - deltaZ
                ramp.append(redge.clone(z, new_z))
                z = new_z
                rampremaining = rampremaining - redge.xy_length
                i = i + 1 if goingForward else i - 1
                if i == 0:
                    goingForward = True
                if i == len(rampedges):
                    goingForward = False

        # now we need to return to original position.
        while i >= 1:
            reset.append(reversed_edges[i - 1])
            i -= 1

        return ramp, reset

    def createRampMethod1(self, rampedges, p0, projectionlen, rampangle):
        """
        This method generates ramp with following pattern:
        1. Start from the original startpoint of the plunge
        2. Ramp down along the path that comes after the plunge
        3. When reaching the Z level of the original plunge, return back to the beginning
            by going the path backwards until the original plunge end point is reached
        4. Continue with the original path
        """
        ramp, reset = self._createRampHelper(rampedges, p0, projectionlen, rampangle)
        return ramp + reset

    def createRampMethod2(self, rampedges, p0, projectionlen, rampangle):
        """
        This method generates ramp with following pattern:
        1. Start from the original startpoint of the plunge
        2. Travel at start depth along the path, for a distance required for step 3
        3. Ramp backwards along the path at rampangle, arriving exactly at the bottom of the plunge
        4. Continue with the original path

        This path is computed using ramp method 1:
        1. Move all edges up to the start height
        2. Perform ramp method 1 from the bottom of the plunge *up* to the relocated path
        3. Reverse the resulting path (both edge order and direction)
        """
        r1_rampedges = [redge.clone(p0[2], p0[2]) for redge in rampedges]
        r1_p0 = rampedges[0].start_point
        r1_rampangle = -rampangle
        r1_result = self.createRampMethod1(r1_rampedges, r1_p0, projectionlen, r1_rampangle)
        outedges = [redge.clone(reverse=True) for redge in r1_result[::-1]]
        return outedges

    def createRampMethod3(self, rampedges, p0, projectionlen, rampangle):
        """
        This method generates ramp with following pattern:
        1. Start from the original startpoint of the plunge
        2. Ramp down along the path that comes after the plunge until
           traveled half of the Z distance
        3. Change direction and ramp backwards to the original plunge end point
        4. Continue with the original path

        This path is computed using ramp method 1.
        """
        z_half = (p0[2] + rampedges[0].start_point[2]) / 2
        r1_rampedges = [redge.clone(z_half, z_half) for redge in rampedges]
        ramp, _ = self._createRampHelper(r1_rampedges, p0, projectionlen, rampangle)
        ramp_back = [
            redge.clone(
                2 * z_half - redge.start_point[2], 2 * z_half - redge.end_point[2], reverse=True
            )
            for redge in ramp[::-1]
        ]
        return ramp + ramp_back

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

    def createCommands(self, edges):
        outCommands = []
        lastX = lastY = lastZ = 0

        for cmd in [edge.command for edge in edges]:
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

    def split(self, split_length):
        """Splits the edge into two parts, the first split_length (if less than xy_length) long.
        Only supported for lines and arcs (no rapids)"""
        split_length = min(split_length, self.xy_length)
        p = split_length / self.xy_length
        firstParams = copy.copy(self.command.Parameters)
        secondParams = copy.copy(self.command.Parameters)
        split_point = None
        if self.is_line:
            split_point = (
                self.start_point[0] * (1 - p) + self.end_point[0] * p,
                self.start_point[1] * (1 - p) + self.end_point[1] * p,
                self.start_point[2] * (1 - p) + self.end_point[2] * p,
            )
        elif self.is_arc:
            angle = self.start_angle * (1 - p) + self.end_angle * p
            split_point = (
                self.center_xy[0] + self.radius * math.cos(angle),
                self.center_xy[1] + self.radius * math.sin(angle),
                self.start_point[2] * (1 - p) + self.end_point[2] * p,
            )
            secondParams.update(
                {
                    "I": self.center_xy[0] - split_point[0],
                    "J": self.center_xy[1] - split_point[1],
                }
            )
        else:
            raise Exception("Invalid type, can only split (non-rapid) lines and arcs")

        firstParams.update({"X": split_point[0], "Y": split_point[1], "Z": split_point[2]})
        first_command = Path.Command(self.command.Name, firstParams)
        second_command = Path.Command(self.command.Name, secondParams)
        return AnnotatedGCode(first_command, self.start_point), AnnotatedGCode(
            second_command, split_point
        )
