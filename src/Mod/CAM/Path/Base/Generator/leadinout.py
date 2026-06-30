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

"""
LeadInOut path generator
Adding lead-in and lead-out moves at the beginning and end of the toolpath.

Example of usage
----------------
import Path.Base.Generator.leadinout as leadinout
path = leadinout.LeadInOut(**args).generate()

Arguments
---------
path : Path.Path
    base path
side : str
    "Outside" or "Inside"
direction : str
    "CW" or "CCW"

leadIn : bool
    add lead-in moves
leadOut : bool
    add lead-out moves

styleIn: str
    style of lead-in
styleOut : str
    style of lead-out

    "Arc", "Line", "Perpendicular", "Tangent",
    "Arc3d", "ArcZ", "ArcZFollow",
    "Helix",
    "Line3d", "LineZ", "LineZFollow",
    "No Retract",
    "Vertical",

angleIn : float (degrees)
    angle of lead-in move
angleOut : float (degrees)
    angle of lead-out move

radiusIn : float
    radius of lead-in move
radiusOut : float
    radius of lead-out move

offsetIn : float
    move start point of lead-in along path
offsetOut : float
    move start point of lead-out along path

extendLeadIn : float
    increase length lead-in (only for styles "Arc", "Line", "Perpendicular", "Tangent")
extendLeadOut: float
    increase length lead-out (...)

invertIn : bool
    invert direction of lead-in
invertOut : bool
    invert direction of lead-out
invertAlt : bool
    alternating inversion, can be useful for ZigZag path

rapidPlunge : bool
    use G0 for plunge move, otherwise G1
retractThreshold : float
    distance which will attempts to avoid unnecessary retractions

horizFeed : float
    horizontal feed rate
vertFeed : float
    vertical feed rate
entranceFeed : float
    feed rate for lead-in moves
exitFeed : float
    feed rate for lead-out moves

clearanceHeight : float
    height needed to clear clamps and obstructions
clearanceHeightOut : float or None
    height for last lead-out move
safeHeight : float
    rapid safety height between locations
startDepth : float
    starting cut depth

tolerance : float
    define precision moves of "ArcZ..." styles
"""

from FreeCAD import Vector
from Path.Base import Language as PathLanguage

import Constants
import FreeCAD
import Part
import Path

import copy
import math

translate = FreeCAD.Qt.translate


class LeadInOut:
    def __init__(
        self,
        path,
        side="Outside",
        direction="CCW",
        leadIn=True,
        leadOut=True,
        styleIn="Arc",
        styleOut="Arc",
        angleIn=45,
        angleOut=45,
        radiusIn=10,
        radiusOut=10,
        offsetIn=0,
        offsetOut=0,
        extendLeadIn=0,
        extendLeadOut=0,
        invertIn=False,
        invertOut=False,
        invertAlt=False,
        rapidPlunge=False,
        retractThreshold=0,
        horizFeed=0,
        vertFeed=0,
        entranceFeed=None,
        exitFeed=None,
        clearanceHeight=0,
        clearanceHeightOut=None,
        safeHeight=0,
        startDepth=0,
        tolerance=0.1,
    ):
        self.source = PathLanguage.Maneuver.FromPath(path).instr
        self.side = side
        self.direction = direction
        self.leadIn = leadIn
        self.leadOut = leadOut
        self.styleIn = styleIn
        self.styleOut = styleOut
        self.angleIn = angleIn
        self.angleOut = angleOut
        self.radiusIn = radiusIn
        self.radiusOut = radiusOut
        self.offsetIn = offsetIn
        self.offsetOut = offsetOut
        self.extendLeadIn = extendLeadIn
        self.extendLeadOut = extendLeadOut
        self.invertIn = invertIn
        self.invertOut = invertOut
        self.invertAlt = invertAlt
        self.invertAltCur = False
        self.rapidPlunge = rapidPlunge
        self.retractThreshold = retractThreshold
        self.horizFeed = horizFeed
        self.vertFeed = vertFeed
        self.entranceFeed = horizFeed if entranceFeed is None else entranceFeed
        self.exitFeed = horizFeed if exitFeed is None else exitFeed
        self.clearanceHeight = clearanceHeight
        self.clearanceHeightOut = (
            clearanceHeight if clearanceHeightOut is None else clearanceHeightOut
        )
        self.safeHeight = safeHeight
        self.startDepth = startDepth
        self.tolerance = tolerance

    # Get direction for lead-in/lead-out in XY plane
    def getLeadDir(self, invert=False):
        output = math.pi / 2
        side = self.side
        direction = self.direction
        if (side == "Inside" and direction == "CW") or (side == "Outside" and direction == "CCW"):
            output = -output
        if invert:
            output = -output
        if self.invertAltCur:
            output = -output

        return output

    # Get direction of original path
    def getArcPathDir(self, cmdName):
        # only CW/CCW and G2/G3 matters
        direction = self.direction
        output = math.pi / 2
        if direction == "CW":
            output = -output

        if cmdName in Constants.GCODE_MOVE_CW and direction == "CCW":
            output = -output
        elif cmdName in Constants.GCODE_MOVE_CCW and direction == "CW":
            output = -output

        return output

    # Create safety movements to start point
    def getTravelStart(self, pos, first, outInstrPrev):
        commands = []
        posPrev = outInstrPrev.positionEnd() if outInstrPrev else Vector()
        posPrevXY = Vector(posPrev.x, posPrev.y, 0)
        posXY = Vector(pos.x, pos.y, 0)
        distance = posPrevXY.distanceToPoint(posXY)

        if first or (distance > self.retractThreshold):
            # move to clearance height
            commands.append(PathLanguage.MoveStraight(None, "G0", {"Z": self.clearanceHeight}))

            # move to mill position at clearance height
            commands.append(PathLanguage.MoveStraight(None, "G0", {"X": pos.x, "Y": pos.y}))

            # move vertical down to mill position
            if self.rapidPlunge:
                # move to mill position rapidly
                commands.append(PathLanguage.MoveStraight(None, "G0", {"Z": pos.z}))
            else:
                # move to mill position in two steps
                commands.append(PathLanguage.MoveStraight(None, "G0", {"Z": self.safeHeight}))
                commands.append(
                    PathLanguage.MoveStraight(None, "G1", {"Z": pos.z, "F": self.vertFeed})
                )

        else:
            # move to next mill position by short path
            if self.rapidPlunge:
                commands.append(
                    PathLanguage.MoveStraight(None, "G0", {"X": pos.x, "Y": pos.y, "Z": pos.z})
                )
            else:
                commands.append(
                    PathLanguage.MoveStraight(
                        None, "G1", {"X": pos.x, "Y": pos.y, "Z": pos.z, "F": self.vertFeed}
                    )
                )

        return commands

    # Create commands with movements to clearance height
    def getTravelEnd(self):
        commands = []
        z = self.clearanceHeightOut
        commands.append(PathLanguage.MoveStraight(None, "G0", {"Z": z}))

        return commands

    # Create vector object from angle
    def angleToVector(self, angle):
        return Vector(math.cos(angle), math.sin(angle), 0)

    # Create arc in XY plane with automatic detection G2|G3
    def createArcMove(self, begin, end, offset, invert, feedRate):
        param = {
            "X": end.x,
            "Y": end.y,
            "Z": end.z,
            "I": offset.x,
            "J": offset.y,
            "F": feedRate,
        }
        if self.getLeadDir(invert) > 0:
            command = PathLanguage.MoveArcCCW(begin, "G3", param)
        else:
            command = PathLanguage.MoveArcCW(begin, "G2", param)

        return command

    # Create arc in XY plane with manually set G2|G3
    def createArcMoveN(self, begin, end, offset, cmdName, feedRate):
        param = {"X": end.x, "Y": end.y, "I": offset.x, "J": offset.y, "F": feedRate}
        if cmdName in Constants.GCODE_MOVE_CW:
            command = PathLanguage.MoveArcCW(begin, cmdName, param)
        else:
            command = PathLanguage.MoveArcCCW(begin, cmdName, param)

        return command

    # Create line movement G1
    def createStraightMove(self, begin, end, feedRate):
        param = {"X": end.x, "Y": end.y, "Z": end.z, "F": feedRate}
        command = PathLanguage.MoveStraight(begin, "G1", param)

        return command

    # Get optimal step angle for iteration ArcZ
    def getStepAngleArcZ(self, angle, radius, div=None):
        if div:
            stepAngle = angle / div
        else:
            stepLength = self.tolerance * 10
            stepAngle = stepLength / radius
        stepAngle = angle / Path.Geom.ceil(angle / stepAngle)

        return stepAngle

    # Get commands from original path for follow lead in
    def getCommandsFollowIn(self, distance):
        cmds = []
        offset = self.offsetIn
        if distance + offset > 0:
            cmds.extend([copy.deepcopy(cmd) for cmd in self.extendTravelIn(distance + offset)])
            if offset > 0:
                cmds = self.cutTravelEnd(cmds, offset)
        if offset < 0:
            # need process as closed profile even if profile is open
            cmds.extend([copy.deepcopy(cmd) for cmd in self.extendTravelOut(abs(offset), True)])
            if distance + offset < 0:
                cmds = self.cutTravelBegin(cmds, abs(distance + offset))

        return cmds

    # Get commands from original path for follow lead out
    def getCommandsFollowOut(self, distance):
        offset = self.offsetOut
        cmds = []
        if offset < 0:
            # need process as closed profile even if profile is open
            cmds.extend([copy.deepcopy(cmd) for cmd in self.extendTravelIn(abs(offset), True)])
            if distance + offset < 0:
                cmds = self.cutTravelEnd(cmds, abs(distance + offset))
        if distance + offset > 0:
            cmds.extend([copy.deepcopy(cmd) for cmd in self.extendTravelOut(distance + offset)])
            if offset > 0:
                cmds = self.cutTravelBegin(cmds, offset)

        return cmds

    # Create lead-in line which follows profile
    def createLineZFollowIn(self, begin, end, feedRate):
        z = begin.z
        length = begin.distanceToPoint(end)
        angle = math.asin((begin.z - end.z) / length)
        distance = length * math.cos(angle)
        cmds = self.getCommandsFollowIn(distance)
        for i, cmd in enumerate(cmds):
            distance -= cmd.pathLength()
            cmd.begin.z = z
            if i == len(cmds) - 1:  # last command
                # forcing end position to exclude precision error
                z = end.z
            else:
                z = end.z + math.tan(angle) * distance
            cmd.param["Z"] = z
        return cmds

    # Create lead-out line which follows profile
    def createLineZFollowOut(self, begin, end, feedRate):
        z = begin.z
        length = begin.distanceToPoint(end)
        angle = math.asin((end.z - begin.z) / length)
        distance = length * math.cos(angle)
        cmds = self.getCommandsFollowOut(distance)
        dist = 0
        for i, cmd in enumerate(cmds):
            dist += cmd.pathLength()
            cmd.begin.z = z
            if i == len(cmds) - 1:  # last command
                # forcing end position to exclude precision error
                z = end.z
            else:
                z = begin.z + math.tan(angle) * dist
            cmd.param["Z"] = z
        return cmds

    # Create vertical arc with move Down by line segments
    def createArcZIn(self, begin, end, radius, feedRate):
        commands = []
        angle = math.acos((radius - begin.z + end.z) / radius)  # start angle
        stepAngle = self.getStepAngleArcZ(angle, radius, div=20)
        angle -= stepAngle
        iterBegin = copy.copy(begin)  # start point of short segment
        v = end - begin
        n = math.hypot(v.x, v.y)
        u = v / n
        while angle > 0 and not Path.Geom.isRoughly(angle, 0):
            distance = n - radius * math.sin(angle)
            iterEnd = begin + u * distance
            iterEnd.z = end.z + radius * (1 - math.cos(angle))
            param = {"X": iterEnd.x, "Y": iterEnd.y, "Z": iterEnd.z, "F": feedRate}
            commands.append(PathLanguage.MoveStraight(iterBegin, "G1", param))
            iterBegin = copy.copy(iterEnd)
            angle -= stepAngle

        # last move to end point
        param = {"X": end.x, "Y": end.y, "Z": end.z, "F": feedRate}
        commands.append(PathLanguage.MoveStraight(iterBegin, "G1", param))

        return commands

    # Create vertical arc with move Up by line segments
    def createArcZOut(self, begin, end, radius, feedRate):
        commands = []
        angleMax = math.acos((radius - end.z + begin.z) / radius)  # finish angle
        stepAngle = self.getStepAngleArcZ(angleMax, radius, div=20)
        iterBegin = copy.copy(begin)  # start point of short segment
        v = end - begin
        n = math.hypot(v.x, v.y)
        u = v / n
        angle = stepAngle  # start angle
        while angle < angleMax and not Path.Geom.isRoughly(angle, angleMax):
            distance = radius * math.sin(angle)
            iterEnd = begin + u * distance
            iterEnd.z = begin.z + radius * (1 - math.cos(angle))
            param = {"X": iterEnd.x, "Y": iterEnd.y, "Z": iterEnd.z, "F": feedRate}
            commands.append(PathLanguage.MoveStraight(iterBegin, "G1", param))
            iterBegin = copy.copy(iterEnd)
            angle += stepAngle

        # last move to end point
        param = {"X": end.x, "Y": end.y, "Z": end.z, "F": feedRate}
        commands.append(PathLanguage.MoveStraight(iterBegin, "G1", param))

        return commands

    # Create vertical arc with move Down and follow profile
    def createArcZFollowIn(self, begin, end, radius, feedRate):
        p1 = Vector(begin.x, begin.y, 0)
        p2 = Vector(end.x, end.y, 0)
        distance = p1.distanceToPoint(p2)
        cmds = self.getCommandsFollowIn(distance)
        edges = [Path.Geom.edgeForCmd(cmd.toCommand(), cmd.positionBegin()) for cmd in cmds]
        wire = Part.Wire(Part.__sortEdges__(edges))
        points = []
        for edge in wire.Edges:
            # skip first point of each edge
            points.extend(edge.discretize(Distance=self.tolerance * 10)[1:])
        points.insert(0, wire.OrderedVertexes[0].Point)

        prevZ = begin.z
        commands = []
        for i, p in enumerate(points):
            distance -= p.distanceToPoint(points[i + 1])
            nextZ = end.z + radius - math.sqrt(radius**2 - distance**2)
            param = {"X": points[i + 1].x, "Y": points[i + 1].y, "Z": nextZ, "F": feedRate}
            commands.append(PathLanguage.MoveStraight(Vector(p.x, p.y, prevZ), "G1", param))
            if i == len(points) - 2:
                break
            prevZ = nextZ

        return commands

    # Create vertical arc with move Down and follow profile
    def createArcZFollowOut(self, begin, end, radius, feedRate):
        p1 = Vector(begin.x, begin.y, 0)
        p2 = Vector(end.x, end.y, 0)
        distance = p1.distanceToPoint(p2)
        cmds = self.getCommandsFollowOut(distance)
        edges = [Path.Geom.edgeForCmd(cmd.toCommand(), cmd.positionBegin()) for cmd in cmds]
        wire = Part.Wire(Part.__sortEdges__(edges))
        points = []
        for edge in wire.Edges:
            # skip first point of each edge
            points.extend(edge.discretize(Distance=self.tolerance * 10)[1:])
        points.insert(0, wire.OrderedVertexes[0].Point)
        prevZ = begin.z
        commands = []
        dist = 0
        for i, p in enumerate(points):
            dist += p.distanceToPoint(points[i + 1])
            nextZ = begin.z + radius - math.sqrt(radius**2 - dist**2)
            param = {"X": points[i + 1].x, "Y": points[i + 1].y, "Z": nextZ, "F": feedRate}
            commands.append(PathLanguage.MoveStraight(Vector(p.x, p.y, prevZ), "G1", param))
            if i == len(points) - 2:
                break
            prevZ = nextZ
        return commands

    def getLeadStart(self, move, first, inInstrPrev, outInstrPrev):
        #    tangent  begin      move
        #    <----_-----x-------------------x
        #       /       |
        #     /         | normal
        #    |          |
        #    x          v

        lead = []
        begin = move.positionBegin()
        beginZ = move.positionBegin().z  # do not change this variable below

        if not self.leadIn and self.leadOut:
            # can not skip leadin if leadout
            # override style to get correct move to next step down
            styleIn = "Vertical"
        else:
            styleIn = self.styleIn

        if styleIn not in ("No Retract", "Vertical"):
            if styleIn == "Perpendicular":
                angleIn = math.pi / 2
            elif styleIn == "Tangent":
                angleIn = 0
            else:
                angleIn = math.radians(self.angleIn)

            length = self.radiusIn
            angleTangent = move.anglesOfTangents()[0]
            normalMax = self.angleToVector(angleTangent + self.getLeadDir(self.invertIn)) * length

            # Here you can find description of the calculations
            # https://forum.freecad.org/viewtopic.php?t=97641

            # prepend "Arc" style lead-in - arc in XY
            # Arc3d the same as Arc, but increased Z start point
            if styleIn in ("Arc", "Arc3d", "Helix"):
                # tangent and normal vectors in XY plane
                arcRadius = length
                tangentLength = math.sin(angleIn) * arcRadius
                normalLength = arcRadius * (1 - math.cos(angleIn))
                tangent = -self.angleToVector(angleTangent) * tangentLength
                normal = (
                    self.angleToVector(angleTangent + self.getLeadDir(self.invertIn)) * normalLength
                )
                arcBegin = begin + tangent + normal
                arcCenter = begin + normalMax
                arcOffset = arcCenter - arcBegin
                lead.append(
                    self.createArcMove(arcBegin, begin, arcOffset, self.invertIn, self.entranceFeed)
                )
                if self.extendLeadIn and styleIn == "Arc":
                    extAngleTangent = lead[-1].anglesOfTangents()[0]
                    extTangent = -self.angleToVector(extAngleTangent) * self.extendLeadIn
                    arcBegin = lead[-1].positionBegin()
                    extBegin = arcBegin + extTangent
                    lead.insert(0, self.createStraightMove(extBegin, arcBegin, self.entranceFeed))

            # prepend "Line" style lead-in - line in XY
            # Line3d the same as Line, but increased Z start point
            elif styleIn in ("Line", "Line3d", "Perpendicular", "Tangent"):
                if styleIn in ("Line", "Perpendicular", "Tangent"):
                    length += self.extendLeadIn
                # tangent and normal vectors in XY plane
                tangentLength = math.cos(angleIn) * length
                normalLength = math.sin(angleIn) * length
                tangent = -self.angleToVector(angleTangent) * tangentLength
                normal = (
                    self.angleToVector(angleTangent + self.getLeadDir(self.invertIn)) * normalLength
                )
                lineBegin = begin + tangent + normal
                lead.append(self.createStraightMove(lineBegin, begin, self.entranceFeed))

            # prepend "LineZ" style lead-in - vertical inclined line
            # Should be applied only on straight Path segment
            elif styleIn in ("LineZ", "LineZFollow"):
                # tangent vector in XY plane
                # normal vector is vertical
                normalLengthMax = self.safeHeight - begin.z
                normalLength = math.sin(angleIn) * length
                # do not exceed Normal vector max length
                normalLength = min(normalLength, normalLengthMax)
                tangentLength = normalLength / math.tan(angleIn)
                tangent = -self.angleToVector(angleTangent) * tangentLength
                normal = Vector(0, 0, normalLength)
                lineBegin = begin + tangent + normal
                if styleIn == "LineZ":
                    lead.append(self.createStraightMove(lineBegin, begin, self.entranceFeed))
                else:
                    lead.extend(self.createLineZFollowIn(lineBegin, begin, self.entranceFeed))

            # prepend "ArcZ" style lead-in - vertical Arc
            # Should be applied only on straight Path segment or open profile
            elif styleIn in ("ArcZ", "ArcZFollow"):
                # tangent vector in XY plane
                # normal vector is vertical
                arcRadius = length
                normalLengthMax = self.safeHeight - begin.z
                normalLength = arcRadius * (1 - math.cos(angleIn))
                if normalLength > normalLengthMax:
                    # do not exceed Normal vector max length
                    normalLength = normalLengthMax
                    # recalculate angle for limited normal length
                    angleIn = math.acos((arcRadius - normalLength) / arcRadius)
                tangentLength = arcRadius * math.sin(angleIn)
                tangent = -self.angleToVector(angleTangent) * tangentLength
                normal = Vector(0, 0, normalLength)
                arcBegin = begin + tangent + normal
                if styleIn == "ArcZ":
                    lead.extend(self.createArcZIn(arcBegin, begin, arcRadius, self.entranceFeed))
                else:
                    lead.extend(
                        self.createArcZFollowIn(arcBegin, begin, arcRadius, self.entranceFeed)
                    )

            # replace 'begin' position with first lead-in command
            begin = lead[0].positionBegin()

            if styleIn in ("Arc3d", "Line3d"):
                # up Z start point for Arc3d and Line3d
                if inInstrPrev and inInstrPrev.z() > begin.z:
                    begin.z = inInstrPrev.z()
                else:
                    begin.z = self.startDepth
                lead[0].setPositionBegin(begin)

            elif styleIn == "Helix":
                # change Z for current helix lead-in
                posPrevZ = None
                if outInstrPrev:
                    posPrevZ = outInstrPrev.positionEnd().z
                if posPrevZ is not None and posPrevZ > beginZ:
                    halfStepZ = (posPrevZ - beginZ) / 2
                    begin.z += halfStepZ
                else:
                    begin.z = self.startDepth

        if self.styleOut == "Helix" and outInstrPrev:
            """change Z for previous helix lead-out
            Unable to do it in getLeadEnd(), due to lack of
            existing information about next moves while creating Lead-out"""
            posPrevZ = outInstrPrev.positionEnd().z
            if posPrevZ > beginZ:
                """previous profile upper than this
                mean processing one stepdown profile"""
                halfStepZ = (posPrevZ - beginZ) / 2
                outInstrPrev.param["Z"] = posPrevZ - halfStepZ

        # get complete start travel moves
        if styleIn != "No Retract":
            travelToStart = self.getTravelStart(begin, first, outInstrPrev)
        else:
            # exclude any lead-in commands
            param = {"X": begin.x, "Y": begin.y, "Z": begin.z, "F": self.entranceFeed}
            travelToStart = [PathLanguage.MoveStraight(None, "G1", param)]

        lead = travelToStart + lead

        return lead

    def getLeadEnd(self, move, last, outInstrPrev):
        #            move       end   tangent
        #    x-------------------x-----_---->
        #                        |       \
        #                 normal |         \
        #                        |          |
        #                        v          x

        lead = []
        end = move.positionEnd()

        if self.styleOut not in ("No Retract", "Vertical"):
            if self.styleOut == "Perpendicular":
                angleOut = math.pi / 2
            elif self.styleOut == "Tangent":
                angleOut = 0
            else:
                angleOut = math.radians(self.angleOut)

            length = self.radiusOut
            angleTangent = move.anglesOfTangents()[1]
            normalMax = self.angleToVector(angleTangent + self.getLeadDir(self.invertOut)) * length

            # Here you can find description of the calculations
            # https://forum.freecad.org/viewtopic.php?t=97641

            # append "Arc" style lead-out - arc in XY
            # Arc3d the same as Arc, but increased Z start point
            if self.styleOut in ("Arc", "Arc3d", "Helix"):
                # tangent and normal vectors in XY plane
                arcRadius = length
                tangentLength = math.sin(angleOut) * arcRadius
                normalLength = arcRadius * (1 - math.cos(angleOut))
                tangent = self.angleToVector(angleTangent) * tangentLength
                normal = (
                    self.angleToVector(angleTangent + self.getLeadDir(self.invertOut))
                    * normalLength
                )
                arcEnd = end + tangent + normal
                lead.append(
                    self.createArcMove(end, arcEnd, normalMax, self.invertOut, self.exitFeed)
                )
                if self.extendLeadOut and self.styleOut == "Arc":
                    extAngleTangent = lead[-1].anglesOfTangents()[1]
                    extTangent = self.angleToVector(extAngleTangent) * self.extendLeadOut
                    arcEnd = lead[-1].positionEnd()
                    extEnd = arcEnd + extTangent
                    lead.append(self.createStraightMove(arcEnd, extEnd, self.exitFeed))

            # append "Line" style lead-out
            # Line3d the same as Line, but increased Z start point
            elif self.styleOut in ("Line", "Line3d", "Perpendicular", "Tangent"):
                if self.styleOut in ("Line", "Perpendicular", "Tangent"):
                    length += self.extendLeadOut
                # tangent and normal vectors in XY plane
                tangentLength = math.cos(angleOut) * length
                normalLength = math.sin(angleOut) * length
                tangent = self.angleToVector(angleTangent) * tangentLength
                normal = (
                    self.angleToVector(angleTangent + self.getLeadDir(self.invertOut))
                    * normalLength
                )
                lineEnd = end + tangent + normal
                lead.append(self.createStraightMove(end, lineEnd, self.exitFeed))

            # append "LineZ" style lead-out - vertical inclined line
            # Should be apply only on straight Path segment
            elif self.styleOut in ("LineZ", "LineZFollow"):
                # tangent vector in XY plane
                # normal vector is vertical
                normalLengthMax = self.startDepth - end.z
                normalLength = math.sin(angleOut) * length
                # do not exceed Normal vector max length
                normalLength = min(normalLength, normalLengthMax)
                tangentLength = normalLength / math.tan(angleOut)
                tangent = self.angleToVector(angleTangent) * tangentLength
                normal = Vector(0, 0, normalLength)
                lineEnd = end + tangent + normal
                if self.styleOut == "LineZ":
                    lead.append(self.createStraightMove(end, lineEnd, self.exitFeed))
                else:
                    lead.extend(self.createLineZFollowOut(end, lineEnd, self.exitFeed))

            # prepend "ArcZ" style lead-out - vertical Arc
            # Should be apply only on straight Path segment
            elif self.styleOut in ("ArcZ", "ArcZFollow"):
                # tangent vector in XY plane
                # normal vector is vertical
                arcRadius = length
                normalLengthMax = self.safeHeight - end.z
                normalLength = arcRadius * (1 - math.cos(angleOut))
                if normalLength > normalLengthMax:
                    # do not exceed Normal vector max length
                    normalLength = normalLengthMax
                    # recalculate angle for limited normal length
                    angleOut = math.acos((arcRadius - normalLength) / arcRadius)
                tangentLength = arcRadius * math.sin(angleOut)
                tangent = self.angleToVector(angleTangent) * tangentLength
                normal = Vector(0, 0, normalLength)
                arcEnd = end + tangent + normal
                if self.styleOut == "ArcZ":
                    lead.extend(self.createArcZOut(end, arcEnd, arcRadius, self.exitFeed))
                else:
                    lead.extend(self.createArcZFollowOut(end, arcEnd, arcRadius, self.exitFeed))

        if self.styleOut in ("Arc3d", "Line3d"):
            # Up Z end point for Arc3d and Line3d
            if outInstrPrev and outInstrPrev.positionBegin().z > end.z:
                lead[-1].param["Z"] = outInstrPrev.positionBegin().z
            else:
                lead[-1].param["Z"] = self.startDepth

        # append travel moves to clearance height after finishing all profiles
        if last and self.styleOut != "No Retract":
            lead += self.getTravelEnd()

        return lead

    # Check command
    def isCuttingMove(self, instr):
        result = instr.isMove() and not instr.isRapid() and not instr.isPlunge()
        return result

    # Get direction of non cut movements
    def getMoveDir(self, instr):
        if instr.positionBegin().z > instr.positionEnd().z:
            return "Down"
        elif instr.positionBegin().z < instr.positionEnd().z:
            return "Up"
        elif instr.pathLength() != 0:
            return "Hor"
        else:
            # move command without change position
            return None

    # Get last index of mill command in whole Path
    def findLastCuttingMoveIndex(self):
        for i in range(len(self.source) - 1, -1, -1):
            if self.isCuttingMove(self.source[i]):
                return i

        return None

    # Get finish index of mill command for one profile
    def findLastCutMultiProfileIndex(self):
        startIndex = self.firstMillIndex
        self.profileLength = 0
        if startIndex >= len(self.source):
            return len(self.source) - 1
        startPoint = self.source[startIndex].positionBegin()
        for i in range(startIndex, len(self.source)):
            if not self.isCuttingMove(self.source[i]):
                return i - 1
            self.profileLength += self.source[i].pathLength()
            if Path.Geom.pointsCoincide(startPoint, self.source[i].positionEnd()):
                # Workaround for several closed paths without retraction
                return i

        return i

    def isProfileClosed(self):
        start = self.firstMillIndex
        end = self.lastMillIndex
        startPoint = self.source[start].positionBegin()
        endPoint = self.source[end].positionEnd()

        if Path.Geom.pointsCoincide(startPoint, endPoint):
            return True
        else:
            return False

    # Increase travel length from 'begin', take commands from profile 'end'
    def extendTravelIn(self, length, forceClosed=None):
        start = self.firstMillIndex
        end = self.lastMillIndex
        closedProfile = forceClosed if forceClosed is not None else self.closedProfile
        if closedProfile:
            # closed profile
            # get extra commands from end of the closed profile
            ratio = int(length / self.profileLength)  # number extra repeats
            length = length - ratio * self.profileLength
            measuredLength = 0
            commands = []
            for i, instr in enumerate(reversed(self.source[start : end + 1])):
                instrLength = instr.pathLength()
                if Path.Geom.isRoughly(measuredLength + instrLength, length):
                    # get needed length without needing to cut last command
                    commands = self.source[end - i : end + 1]
                    break
                elif measuredLength + instrLength > length:
                    # measured length exceeds needed length and needs cut command
                    commands = self.source[end - i + 1 : end + 1]
                    newLength = length - measuredLength
                    newInstr = self.cutInstrBegin(instr, newLength)
                    commands.insert(0, newInstr)
                    break
                measuredLength += instrLength

            return commands + self.source[start : end + 1] * ratio

        else:
            # open profile
            # extend first move
            instr = self.source[start]
            newLength = length + instr.pathLength()
            newInstr = self.cutInstrBegin(instr, newLength)
            newInstr.setPositionEnd(instr.positionBegin())
            return [newInstr]

    # Increase travel length from 'end', take commands from profile 'start'
    def extendTravelOut(self, length, forceClosed=None):
        start = self.firstMillIndex
        end = self.lastMillIndex
        closedProfile = forceClosed if forceClosed is not None else self.closedProfile
        if closedProfile:
            # closed profile
            # get extra commands from begin of the closed profile
            ratio = int(length / self.profileLength)  # number extra repeats
            length = length - ratio * self.profileLength
            measuredLength = 0
            commands = []
            for i, instr in enumerate(self.source[start : end + 1]):
                instrLength = instr.pathLength()
                if Path.Geom.isRoughly(measuredLength + instrLength, length):
                    # get needed length without needing to cut last command
                    commands = self.source[start : start + i + 1]
                    break
                elif measuredLength + instrLength > length:
                    # measured length exceeds needed length and needs cut command
                    commands = self.source[start : start + i]
                    newLength = length - measuredLength
                    newInstr = self.cutInstrEnd(instr, newLength)
                    commands.append(newInstr)
                    break
                measuredLength += instrLength

            return self.source[start : end + 1] * ratio + commands

        else:
            # open profile
            # extend last move
            instr = self.source[end]
            newLength = length + instr.pathLength()
            newInstr = self.cutInstrEnd(instr, newLength)
            newInstr = self.cutInstrBegin(newInstr, length)
            return [newInstr]

    # Cut travel end by distance (negative overtravel out)
    def cutTravelEnd(self, commands, cutLength):
        measuredLength = 0
        for i, instr in enumerate(reversed(commands)):
            if instr.positionBegin() is None:
                # workaround if cut whole profile by negative offset
                cmds = commands[:-i]
                newInstr = self.cutInstrEnd(commands[-i], 0.1)
                cmds.append(newInstr)
                return cmds

            instrLength = instr.pathLength()
            measuredLength += instrLength
            if Path.Geom.isRoughly(measuredLength, cutLength):
                # get needed length and not need to cut any command
                return commands[: -i - 1]

            elif measuredLength > cutLength:
                # measured length exceed needed cut length and need cut command
                cmds = commands[: -i - 1]
                newLength = measuredLength - cutLength
                newInstr = self.cutInstrEnd(instr, newLength)
                cmds.append(newInstr)
                return cmds

        Path.Log.warning(translate("CAM", "Exceeded length in cutTravelEnd"))
        return []

    # Cut travel from begin by distance
    def cutTravelBegin(self, commands, cutLength):
        measuredLength = 0
        for i, instr in enumerate(commands):
            instrLength = instr.pathLength()
            measuredLength += instrLength
            if Path.Geom.isRoughly(measuredLength, cutLength):
                # get needed length and not need to cut any command
                return commands[i + 1 :]

            elif measuredLength > cutLength:
                # measured length exceed needed cut length and need cut command
                cmds = commands[i + 1 :]
                newLength = measuredLength - cutLength
                newInstr = self.cutInstrBegin(instr, newLength)
                cmds.insert(0, newInstr)
                return cmds

        Path.Log.warning(translate("CAM", "Exceeded length in cutTravelBegin"))
        return []

    # Change end point of instruction
    def cutInstrEnd(self, instr, newLength):
        command = None
        # Cut straight move from begin
        if instr.isStraight():
            begin = instr.positionBegin()
            end = instr.positionEnd()
            v = end - begin
            n = math.hypot(v.x, v.y)
            u = v / n
            cutEnd = begin + u * newLength
            command = self.createStraightMove(begin, cutEnd, self.horizFeed)

        # Cut arc move from begin
        elif instr.isArc():
            cmdName = instr.cmd
            angleTangent = instr.anglesOfTangents()[0]
            arcBegin = instr.positionBegin()
            arcOffset = Vector(instr.i(), instr.j(), instr.k())
            arcRadius = instr.arcRadius()
            arcAngle = newLength / arcRadius
            tangentLength = math.sin(arcAngle) * arcRadius
            normalLength = arcRadius * (1 - math.cos(arcAngle))
            tangent = self.angleToVector(angleTangent) * tangentLength
            normal = self.angleToVector(angleTangent + self.getArcPathDir(cmdName)) * normalLength
            arcEnd = arcBegin + tangent + normal
            command = self.createArcMoveN(arcBegin, arcEnd, arcOffset, cmdName, self.horizFeed)

        return command

    # Change start point of instruction
    def cutInstrBegin(self, instr, newLength):
        # Cut straignt move from begin
        if instr.isStraight():
            begin = instr.positionBegin()
            end = instr.positionEnd()
            v = end - begin
            n = math.hypot(v.x, v.y)
            u = v / n
            newBegin = end - u * newLength
            command = self.createStraightMove(newBegin, end, self.horizFeed)
            return command

        # Cut arc move from begin
        elif instr.isArc():
            cmdName = instr.cmd
            angleTangent = instr.anglesOfTangents()[1]
            arcEnd = instr.positionEnd()
            arcCenter = instr.xyCenter()
            arcRadius = instr.arcRadius()
            arcAngle = newLength / arcRadius
            tangentLength = math.sin(arcAngle) * arcRadius
            normalLength = arcRadius * (1 - math.cos(arcAngle))
            tangent = -self.angleToVector(angleTangent) * tangentLength
            normal = self.angleToVector(angleTangent + self.getArcPathDir(cmdName)) * normalLength
            arcBegin = arcEnd + tangent + normal
            arcOffset = arcCenter - arcBegin
            command = self.createArcMoveN(arcBegin, arcEnd, arcOffset, cmdName, self.horizFeed)
            return command

        return None

    def generate(self):
        maneuver = PathLanguage.Maneuver()
        commands = []
        first = True  # prepare first move at clearance height
        self.firstMillIndex = None  # Index start mill instruction for one profile
        self.lastMillIndex = None  # Index end mill instruction for one profile
        self.lastCuttingMoveIndex = self.findLastCuttingMoveIndex()
        self.closedProfile = None
        inInstrPrev = None  # for RetractThreshold
        outInstrPrev = None  # for RetractThreshold
        measuredLength = 0  # for negative OffsetIn
        skipCounter = 0  # for negative OffsetIn
        moveDir = None

        # Process all instructions
        for i, instr in enumerate(self.source):
            # Process without mill instruction
            if not self.isCuttingMove(instr):
                if not instr.isMove():
                    # non-move instruction gets added verbatim
                    commands.append(instr)
                else:
                    moveDir = self.getMoveDir(instr)
                    if (not self.leadIn and not self.leadOut) and (
                        moveDir in ("Down", "Hor") or first
                    ):
                        # keep original Lead-in movements
                        commands.append(instr)
                    if not self.leadOut and moveDir == "Up" and not first:
                        # keep original Lead-out movements
                        commands.append(instr)
                # skip travel and plunge moves if LeadInOut will be process
                # travel moves will be added in getLeadStart and getLeadEnd
                continue

            # measuring length for one profile
            if self.isCuttingMove(instr):
                measuredLength += instr.pathLength()

            # Process Lead-In
            if first or not self.isCuttingMove(self.source[i - 1 - skipCounter]):
                if self.leadIn or self.leadOut:
                    # can not skip leadin if leadout

                    self.firstMillIndex = i if self.firstMillIndex is None else self.firstMillIndex
                    self.lastMillIndex = (
                        self.findLastCutMultiProfileIndex()
                        if self.lastMillIndex is None
                        else self.lastMillIndex
                    )

                    self.closedProfile = self.isProfileClosed()

                    overtravelIn = None
                    if self.offsetIn < 0 and self.styleIn != "No Retract":
                        # Process negative Offset Lead-In (cut travel from begin)
                        if measuredLength <= abs(self.offsetIn):
                            # skip mill instruction
                            skipCounter += 1  # count skipped instructions
                            continue
                        else:
                            skipCounter = 0
                            # cut mill instruction
                            newLength = measuredLength - abs(self.offsetIn)
                            instr = self.cutInstrBegin(instr, newLength)

                    elif self.offsetIn > 0 and self.styleIn != "No Retract":
                        # Process positive offset Lead-In (overtravel)
                        overtravelIn = self.extendTravelIn(self.offsetIn)
                    if overtravelIn:
                        commands.extend(
                            self.getLeadStart(overtravelIn[0], first, inInstrPrev, outInstrPrev)
                        )
                        commands.extend(overtravelIn)
                    else:
                        commands.extend(self.getLeadStart(instr, first, inInstrPrev, outInstrPrev))
                    inInstrPrev = commands[-1]
                first = False

            # Add mill instruction
            commands.append(instr)

            # Process Lead-Out
            last = bool(i == self.lastCuttingMoveIndex)
            if last or not self.isCuttingMove(self.source[i + 1]):
                if self.leadOut:

                    # Process negative Offset Lead-Out (cut travel from end)
                    if self.offsetOut < 0 and self.styleOut != "No Retract":
                        commands = self.cutTravelEnd(commands, abs(self.offsetOut))

                    # Process positive Offset Lead-Out (overtravel)
                    elif self.offsetOut > 0 and self.styleOut != "No Retract":
                        overtravelOut = self.extendTravelOut(self.offsetOut)
                        if overtravelOut:
                            commands.extend(overtravelOut)

                    # add lead end and travel moves
                    leadEndInstr = self.getLeadEnd(commands[-1], last, outInstrPrev)
                    commands.extend(leadEndInstr)

                    # Last mill position to check RetractThreshold
                    if leadEndInstr:
                        outInstrPrev = leadEndInstr[-1]
                    else:
                        outInstrPrev = instr

                # preparing for the next profile
                measuredLength = 0
                self.firstMillIndex = None
                self.lastMillIndex = None
                self.invertAltCur = not self.invertAltCur if self.invertAlt else False

            # Workaround for several closed paths without retraction
            if not Path.Geom.isRoughly(instr.positionBegin().z, instr.positionEnd().z):
                # reset firstMillIndex, if move not in XY plane
                self.firstMillIndex = i + 1
            if self.lastMillIndex and i >= self.lastMillIndex:
                # lastMillIndex not correct any more, find new
                self.lastMillIndex = self.findLastCutMultiProfileIndex()
            if self.lastMillIndex and Path.Geom.pointsCoincide(
                instr.positionBegin(), self.source[self.lastMillIndex].positionEnd()
            ):
                # get firstMillIndex for last closed path
                self.firstMillIndex = i
                self.closedProfile = True

        maneuver.addInstructions(commands)
        return maneuver.toPath()
