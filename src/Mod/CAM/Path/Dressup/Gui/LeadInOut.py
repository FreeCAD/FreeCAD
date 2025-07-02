#  -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2017 LTS <SammelLothar@gmx.de> under LGPL               *
# *   Copyright (c) 2020-2021 Schildkroet                                   *
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


import FreeCAD as App
import FreeCADGui
import Path
import Path.Base.Language as PathLanguage
import Path.Dressup.Utils as PathDressup
import PathScripts.PathUtils as PathUtils
from Path.Geom import wireForPath

import copy
import math

__doc__ = """LeadInOut Dressup USE ROLL-ON ROLL-OFF to profile"""

from PySide.QtCore import QT_TRANSLATE_NOOP

from PathPythonGui.simple_edit_panel import SimpleEditPanel

translate = App.Qt.translate

if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


class ObjectDressup:
    def __init__(self, obj):
        lead_styles = [
            QT_TRANSLATE_NOOP("CAM_DressupLeadInOut", "Arc"),
            QT_TRANSLATE_NOOP("CAM_DressupLeadInOut", "Arc3d"),
            QT_TRANSLATE_NOOP("CAM_DressupLeadInOut", "ArcZ"),
            QT_TRANSLATE_NOOP("CAM_DressupLeadInOut", "Helix"),
            QT_TRANSLATE_NOOP("CAM_DressupLeadInOut", "Line"),
            QT_TRANSLATE_NOOP("CAM_DressupLeadInOut", "Line3d"),
            QT_TRANSLATE_NOOP("CAM_DressupLeadInOut", "LineZ"),
            QT_TRANSLATE_NOOP("CAM_DressupLeadInOut", "Vertical"),
            QT_TRANSLATE_NOOP("CAM_DressupLeadInOut", "None"),
        ]
        self.obj = obj
        obj.addProperty(
            "App::PropertyLink",
            "Base",
            "Path",
            QT_TRANSLATE_NOOP("App::Property", "The base toolpath to modify"),
        )
        obj.addProperty(
            "App::PropertyLength",
            "RetractThreshold",
            "Path",
            QT_TRANSLATE_NOOP(
                "App::Property", "Set distance which will attempts to avoid unnecessary retractions"
            ),
        )
        obj.addProperty(
            "App::PropertyEnumeration",
            "StyleIn",
            "Path",
            QT_TRANSLATE_NOOP("App::Property", "The Style of Lead-In"),
        )
        obj.StyleIn = lead_styles
        obj.addProperty(
            "App::PropertyEnumeration",
            "StyleOut",
            "Path",
            QT_TRANSLATE_NOOP("App::Property", "The Style of Lead-Out"),
        )
        obj.StyleOut = lead_styles
        obj.addProperty(
            "App::PropertyBool",
            "RapidPlunge",
            "Path",
            QT_TRANSLATE_NOOP("App::Property", "Perform plunges with G0"),
        )
        obj.addProperty(
            "App::PropertyBool",
            "IncludeLayers",
            "Path",
            QT_TRANSLATE_NOOP("App::Property", "Apply LeadInOut to layers within an operation"),
        )
        obj.addProperty(
            "App::PropertyAngle",
            "AngleIn",
            "Path Lead-in",
            QT_TRANSLATE_NOOP("App::Property", "Angle of the Lead-In (1..90)"),
        )
        obj.addProperty(
            "App::PropertyAngle",
            "AngleOut",
            "Path Lead-out",
            QT_TRANSLATE_NOOP("App::Property", "Angle of the Lead-Out (1..90)"),
        )
        obj.addProperty(
            "App::PropertyInteger",
            "PercentageRadiusIn",
            "Path Lead-in",
            QT_TRANSLATE_NOOP("App::Property", "Determine length of the Lead-In"),
        )
        obj.addProperty(
            "App::PropertyInteger",
            "PercentageRadiusOut",
            "Path Lead-out",
            QT_TRANSLATE_NOOP("App::Property", "Determine length of the Lead-Out"),
        )
        obj.addProperty(
            "App::PropertyBool",
            "InvertIn",
            "Path Lead-in",
            QT_TRANSLATE_NOOP("App::Property", "Invert Lead-In direction"),
        )
        obj.addProperty(
            "App::PropertyBool",
            "InvertOut",
            "Path Lead-out",
            QT_TRANSLATE_NOOP("App::Property", "Invert Lead-Out direction"),
        )
        obj.addProperty(
            "App::PropertyDistance",
            "OffsetIn",
            "Path Lead-in",
            QT_TRANSLATE_NOOP("App::Property", "Move start point"),
        )
        obj.addProperty(
            "App::PropertyDistance",
            "OffsetOut",
            "Path Lead-out",
            QT_TRANSLATE_NOOP("App::Property", "Move end point"),
        )
        obj.Proxy = self
        self.wire = None
        self.rapids = None

    def dumps(self):
        return None

    def loads(self, state):
        return None

    def setup(self, obj):
        obj.AngleIn = 45
        obj.AngleOut = 45
        obj.IncludeLayers = True
        obj.InvertIn = False
        obj.InvertOut = False
        obj.PercentageRadiusIn = 150
        obj.PercentageRadiusOut = 150
        obj.RapidPlunge = False
        obj.StyleIn = "Arc"
        obj.StyleOut = "Arc"

    def execute(self, obj):
        if not obj.Base:
            obj.Path = Path.Path()
            return
        if not obj.Base.isDerivedFrom("Path::Feature"):
            obj.Path = Path.Path()
            return
        if not obj.Base.Path:
            obj.Path = Path.Path()
            return

        if obj.PercentageRadiusIn < 1:
            obj.PercentageRadiusIn = 1
        if obj.PercentageRadiusOut < 1:
            obj.PercentageRadiusOut = 1

        if obj.AngleIn > 180:
            obj.AngleIn = 180
        if obj.AngleIn < 1:
            obj.AngleIn = 1

        if obj.AngleOut > 180:
            obj.AngleOut = 180
        if obj.AngleOut < 1:
            obj.AngleOut = 1

        if obj.StyleIn in ["None", "Vertical"]:
            obj.setEditorMode("AngleIn", 2)
        else:
            obj.setEditorMode("AngleIn", 0)

        if obj.StyleIn in ["None", "ArcZ", "LineZ", "Vertical"]:
            obj.setEditorMode("InvertIn", 2)
        else:
            obj.setEditorMode("InvertIn", 0)

        if obj.StyleIn in ["None"]:
            obj.setEditorMode("OffsetIn", 2)
        else:
            obj.setEditorMode("OffsetIn", 0)

        if obj.StyleIn in ["None", "Vertical"]:
            obj.setEditorMode("PercentageRadiusIn", 2)
        else:
            obj.setEditorMode("PercentageRadiusIn", 0)

        if obj.StyleOut in ["None", "Vertical"]:
            obj.setEditorMode("AngleOut", 2)
        else:
            obj.setEditorMode("AngleOut", 0)

        if obj.StyleOut in ["None", "ArcZ", "LineZ", "Vertical"]:
            obj.setEditorMode("InvertOut", 2)
        else:
            obj.setEditorMode("InvertOut", 0)

        if obj.StyleOut in ["None"]:
            obj.setEditorMode("OffsetOut", 2)
        else:
            obj.setEditorMode("OffsetOut", 0)

        if obj.StyleOut in ["None", "Vertical"]:
            obj.setEditorMode("PercentageRadiusOut", 2)
        else:
            obj.setEditorMode("PercentageRadiusOut", 0)

        self.wire, self.rapids = wireForPath(PathUtils.getPathWithPlacement(obj.Base))
        obj.Path = self.generateLeadInOutCurve(obj)

    def onDocumentRestored(self, obj):
        """onDocumentRestored(obj) ... Called automatically when document is restored."""
        lead_styles = [
            QT_TRANSLATE_NOOP("CAM_DressupLeadInOut", "Arc"),
            QT_TRANSLATE_NOOP("CAM_DressupLeadInOut", "Arc3d"),
            QT_TRANSLATE_NOOP("CAM_DressupLeadInOut", "ArcZ"),
            QT_TRANSLATE_NOOP("CAM_DressupLeadInOut", "Helix"),
            QT_TRANSLATE_NOOP("CAM_DressupLeadInOut", "Line"),
            QT_TRANSLATE_NOOP("CAM_DressupLeadInOut", "Line3d"),
            QT_TRANSLATE_NOOP("CAM_DressupLeadInOut", "LineZ"),
            QT_TRANSLATE_NOOP("CAM_DressupLeadInOut", "Vertical"),
            QT_TRANSLATE_NOOP("CAM_DressupLeadInOut", "None"),
        ]
        if hasattr(obj, "LeadIn"):
            obj.removeProperty("LeadIn")
        if hasattr(obj, "LeadOut"):
            obj.removeProperty("LeadOut")
        if hasattr(obj, "StyleOn"):
            # Replace StyleOn by StyleIn
            obj.addProperty(
                "App::PropertyEnumeration",
                "StyleIn",
                "Path",
                QT_TRANSLATE_NOOP("App::Property", "The Style of Lead-In"),
            )
            obj.StyleIn = lead_styles
            obj.removeProperty("StyleOn")
        if hasattr(obj, "StyleOff"):
            # Replace StyleOff by StyleOut
            obj.addProperty(
                "App::PropertyEnumeration",
                "StyleOut",
                "Path",
                QT_TRANSLATE_NOOP("App::Property", "The Style of Lead-Out"),
            )
            obj.StyleOut = lead_styles
            obj.removeProperty("StyleOff")
            obj.StyleIn = "Arc"
            obj.StyleOut = "Arc"

        if hasattr(obj, "Length"):
            # Replace Length by PercentageRadiusIn
            obj.addProperty(
                "App::PropertyInteger",
                "PercentageRadiusIn",
                "Path Lead-in",
                QT_TRANSLATE_NOOP("App::Property", "Determine length of the Lead-In"),
            )
            obj.PercentageRadiusIn = 150
            obj.removeProperty("Length")
        if hasattr(obj, "LengthOut"):
            # Replace Length by PercentageRadiusOut
            obj.addProperty(
                "App::PropertyInteger",
                "PercentageRadiusOut",
                "Path Lead-out",
                QT_TRANSLATE_NOOP("App::Property", "Determine length of the Lead-Out"),
            )
            obj.PercentageRadiusOut = 150
            obj.removeProperty("LengthOut")
        if hasattr(obj, "ExtendLeadIn"):
            # Remove ExtendLeadIn property
            obj.removeProperty("ExtendLeadIn")
        if hasattr(obj, "ExtendLeadOut"):
            # Remove ExtendLeadOut property
            obj.removeProperty("ExtendLeadOut")
        if not hasattr(obj, "AngleIn"):
            obj.addProperty(
                "App::PropertyAngle",
                "AngleIn",
                "Path Lead-in",
                QT_TRANSLATE_NOOP("App::Property", "Angle of the Lead-In (1..90)"),
            )
            obj.AngleIn = 45
        if not hasattr(obj, "AngleOut"):
            obj.addProperty(
                "App::PropertyAngle",
                "AngleOut",
                "Path Lead-out",
                QT_TRANSLATE_NOOP("App::Property", "Angle of the Lead-Out (1..90)"),
            )
            obj.AngleOut = 45
        if not hasattr(obj, "InvertIn"):
            obj.addProperty(
                "App::PropertyBool",
                "InvertIn",
                "Path Lead-in",
                QT_TRANSLATE_NOOP("App::Property", "Invert Lead-In direction"),
            )
        if not hasattr(obj, "InvertOut"):
            obj.addProperty(
                "App::PropertyBool",
                "InvertOut",
                "Path Lead-out",
                QT_TRANSLATE_NOOP("App::Property", "Invert Lead-Out direction"),
            )
        if not hasattr(obj, "OffsetIn"):
            obj.addProperty(
                "App::PropertyDistance",
                "OffsetIn",
                "Path Lead-in",
                QT_TRANSLATE_NOOP("App::Property", "Move start point"),
            )
        if not hasattr(obj, "OffsetOut"):
            obj.addProperty(
                "App::PropertyDistance",
                "OffsetOut",
                "Path Lead-out",
                QT_TRANSLATE_NOOP("App::Property", "Move end point"),
            )
        if not hasattr(obj, "RetractThreshold"):
            obj.addProperty(
                "App::PropertyLength",
                "RetractThreshold",
                "Path",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Set distance which will attempts to avoid unnecessary retractions",
                ),
            )
        if hasattr(obj, "KeepToolDown"):
            obj.removeProperty("KeepToolDown")

    # Get direction for lead-in/lead-out in XY plane
    def getLeadDir(self, obj, invert=False):
        output = math.pi / 2
        op = PathDressup.baseOp(obj.Base)
        side = op.Side if hasattr(op, "Side") else "Inside"
        direction = op.Direction if hasattr(op, "Direction") else "CCW"
        if (side == "Inside" and direction == "CW") or (side == "Outside" and direction == "CCW"):
            output = -output
        if invert:
            output = -output

        return output

    # Get direction of original path
    def getPathOrientation(self, obj):
        # only CW or CCW is matter
        op = PathDressup.baseOp(obj.Base)
        direction = op.Direction if hasattr(op, "Direction") else "CCW"
        output = math.pi / 2
        if direction == "CW":
            output = -output

        return output

    # Create safety movements to start point
    def getTravelStart(self, obj, pos, first, inInstrPrev, outInstrPrev):
        op = PathDressup.baseOp(obj.Base)
        vertfeed = PathDressup.toolController(obj.Base).VertFeed.Value
        commands = []

        print()
        print("getTravelStart")
        print(f" inInstrPrev {inInstrPrev}")
        print(f" outInstrPrev {outInstrPrev}")
        print(" pos", pos)
        posPrev = outInstrPrev.positionEnd() if outInstrPrev else App.Vector()
        posPrevXY = App.Vector(posPrev.x, posPrev.y, 0)
        posXY = App.Vector(pos.x, pos.y, 0)
        distance = posPrevXY.distanceToPoint(posXY)

        if first or (distance > obj.RetractThreshold):
            # move to clearance height
            commands.append(PathLanguage.MoveStraight(None, "G00", {"Z": op.ClearanceHeight.Value}))

            # move to mill X/Y-position (without move Z)
            commands.append(PathLanguage.MoveStraight(None, "G00", {"X": pos.x, "Y": pos.y}))

        if distance > obj.RetractThreshold:
            # move vertical down to mill position
            if obj.RapidPlunge:
                # move to mill position rapidly
                commands.append(PathLanguage.MoveStraight(None, "G00", {"Z": pos.z}))
            else:
                # move to mill position in one or two steps
                if first:
                    # move down to SafeHeight
                    commands.append(
                        PathLanguage.MoveStraight(None, "G00", {"Z": op.SafeHeight.Value})
                    )
                commands.append(PathLanguage.MoveStraight(None, "G01", {"Z": pos.z, "F": vertfeed}))

        elif obj.StyleOut == "Helix":
            # move by helix to next mill position
            if obj.StyleIn == "Helix":
                halfStepZ = (posPrev.z - pos.z) / 2
                stepOutZ = halfStepZ * outInstrPrev.arcAngle() / math.pi
                lastZMove = stepOutZ
            else:
                stepOutZ = posPrev.z - pos.z
                lastZMove = 0
            outInstrPrev.param["Z"] = posPrev.z - stepOutZ
            if not Path.Geom.pointsCoincide(posPrevXY, posXY):
                if obj.RapidPlunge:
                    commands.append(
                        PathLanguage.MoveStraight(
                            outInstrPrev.positionEnd(),
                            "G00",
                            {"X": pos.x, "Y": pos.y, "Z": pos.z + lastZMove},
                        )
                    )
                else:
                    commands.append(
                        PathLanguage.MoveStraight(
                            outInstrPrev.positionEnd(),
                            "G01",
                            {"X": pos.x, "Y": pos.y, "Z": pos.z + lastZMove, "F": vertfeed},
                        )
                    )

        else:
            # move to next mill position by short path
            if obj.RapidPlunge:
                commands.append(
                    PathLanguage.MoveStraight(None, "G00", {"X": pos.x, "Y": pos.y, "Z": pos.z})
                )
            else:
                commands.append(
                    PathLanguage.MoveStraight(
                        None, "G01", {"X": pos.x, "Y": pos.y, "Z": pos.z, "F": vertfeed}
                    )
                )

        return commands

    # Create commands with movements to clearance height
    def getTravelEnd(self, obj):
        commands = []
        op = PathDressup.baseOp(obj.Base)
        z = op.ClearanceHeight.Value
        commands.append(PathLanguage.MoveStraight(None, "G00", {"Z": z}))

        return commands

    # Create vector object from angle
    def angleToVector(self, angle):
        return App.Vector(math.cos(angle), math.sin(angle), 0)

    # Create arc in XY plane with automatic detection G2|G3
    def createArcMove(self, obj, begin, end, offset, invert=False):
        horizfeed = PathDressup.toolController(obj.Base).HorizFeed.Value
        param = {"X": end.x, "Y": end.y, "Z": end.z, "I": offset.x, "J": offset.y, "F": horizfeed}
        if self.getLeadDir(obj, invert) > 0:
            command = PathLanguage.MoveArcCCW(begin, "G3", param)
        else:
            command = PathLanguage.MoveArcCW(begin, "G2", param)

        return command

    # Create arc in XY plane with manually set G2|G3
    def createArcMoveN(self, obj, begin, end, offset, cmdName):
        horizfeed = PathDressup.toolController(obj.Base).HorizFeed.Value
        param = {"X": end.x, "Y": end.y, "I": offset.x, "J": offset.y, "F": horizfeed}
        if cmdName == "G2":
            command = PathLanguage.MoveArcCW(begin, cmdName, param)
        else:
            command = PathLanguage.MoveArcCCW(begin, cmdName, param)

        return command

    # Create line movement G1
    def createStraightMove(self, obj, begin, end):
        horizfeed = PathDressup.toolController(obj.Base).HorizFeed.Value
        param = {"X": end.x, "Y": end.y, "Z": end.z, "F": horizfeed}
        command = PathLanguage.MoveStraight(begin, "G1", param)

        return command

    # Create vertical arc with move Down by line segments
    def createArcZMoveDown(self, obj, begin, end, radius):
        commands = []
        horizfeed = PathDressup.toolController(obj.Base).HorizFeed.Value
        angle = math.acos((radius - begin.z + end.z) / radius)  # start angle
        stepAngle = math.radians(3)  # less angle give more iterations
        iters = math.ceil(angle / stepAngle)
        iterBegin = copy.copy(begin)  # start point of short segment
        iter = 1
        v = end - begin
        n = math.hypot(v.x, v.y)
        u = v / n
        while iter <= iters:
            if iter < iters:
                angle -= stepAngle
                distance = n - radius * math.sin(angle)
                iterEnd = begin + u * distance
                iterEnd.z = end.z + radius * (1 - math.cos(angle))
            else:
                # exclude error of calculations for the last iteration
                iterEnd = copy.copy(end)
            param = {"X": iterEnd.x, "Y": iterEnd.y, "Z": iterEnd.z, "F": horizfeed}
            commands.append(PathLanguage.MoveStraight(iterBegin, "G1", param))
            iterBegin = copy.copy(iterEnd)
            iter += 1

        return commands

    # Create vertical arc with move Up by line segments
    def createArcZMoveUp(self, obj, begin, end, radius):
        commands = []
        horizfeed = PathDressup.toolController(obj.Base).HorizFeed.Value
        angleMax = math.acos((radius - end.z + begin.z) / radius)  # finish angle
        stepAngle = math.radians(10)  # less angle give more iterations
        iters = math.ceil(angleMax / stepAngle)
        iterBegin = copy.copy(begin)  # start point of short segment
        iter = 1
        v = end - begin
        n = math.hypot(v.x, v.y)
        u = v / n
        angle = 0  # start angle
        while iter <= iters:
            if iter < iters:
                angle += stepAngle
                distance = radius * math.sin(angle)
                iterEnd = begin + u * distance
                iterEnd.z = begin.z + radius * (1 - math.cos(angle))
            else:
                # exclude the error of calculations of the last point
                iterEnd = copy.copy(end)
            param = {"X": iterEnd.x, "Y": iterEnd.y, "Z": iterEnd.z, "F": horizfeed}
            commands.append(PathLanguage.MoveStraight(iterBegin, "G1", param))
            iterBegin = copy.copy(iterEnd)
            iter += 1

        return commands

    def getLeadStart(self, obj, move, first, inInstrPrev, outInstrPrev):

        #    tangent  begin      move
        #    <----_-----x-------------------x
        #       /       |
        #     /         | normal
        #    |          |
        #    x          v

        lead = []
        begin = move.positionBegin()

        if obj.StyleIn not in ["None", "Vertical"]:
            toolRadius = PathDressup.toolController(obj.Base).Tool.Diameter.Value / 2
            angleIn = math.radians(obj.AngleIn.Value)
            length = obj.PercentageRadiusIn * toolRadius / 100
            angleTangent = move.anglesOfTangents()[0]
            normalMax = (
                self.angleToVector(angleTangent + self.getLeadDir(obj, obj.InvertIn)) * length
            )

            # Here you can find description of the calculations
            # https://forum.freecad.org/viewtopic.php?t=97641

            # prepend "Arc" style lead-in - arc in XY
            # Arc3d the same as Arc, but increased Z start point
            if obj.StyleIn in ["Arc", "Arc3d", "Helix"]:
                # tangent and normal vectors in XY plane
                arcRadius = length
                tangentLength = math.sin(angleIn) * arcRadius
                normalLength = arcRadius * (1 - math.cos(angleIn))
                tangent = -self.angleToVector(angleTangent) * tangentLength
                normal = (
                    self.angleToVector(angleTangent + self.getLeadDir(obj, obj.InvertIn))
                    * normalLength
                )
                arcBegin = begin + tangent + normal
                arcCenter = begin + normalMax
                arcOffset = arcCenter - arcBegin
                lead.append(self.createArcMove(obj, arcBegin, begin, arcOffset, obj.InvertIn))

            # prepend "Line" style lead-in - line in XY
            # Line3d the same as Line, but increased Z start point
            elif obj.StyleIn in ["Line", "Line3d"]:
                # tangent and normal vectors in XY plane
                tangentLength = math.cos(angleIn) * length
                normalLength = math.sin(angleIn) * length
                tangent = -self.angleToVector(angleTangent) * tangentLength
                normal = (
                    self.angleToVector(angleTangent + self.getLeadDir(obj, obj.InvertIn))
                    * normalLength
                )
                lineBegin = begin + tangent + normal
                lead.append(self.createStraightMove(obj, lineBegin, begin))

            # prepend "LineZ" style lead-in - vertical inclined line
            # Should be apply only on straight Path segment
            elif obj.StyleIn == "LineZ":
                # tangent vector in XY plane
                # normal vector is vertical
                op = PathDressup.baseOp(obj.Base)
                normalLengthMax = op.SafeHeight.Value - begin.z
                normalLength = math.sin(angleIn) * length
                # do not exceed Normal vector max length
                normalLength = min(normalLength, normalLengthMax)
                tangentLength = normalLength / math.tan(angleIn)
                tangent = -self.angleToVector(angleTangent) * tangentLength
                normal = App.Vector(0, 0, normalLength)
                lineBegin = begin + tangent + normal
                lead.append(self.createStraightMove(obj, lineBegin, begin))

            # prepend "ArcZ" style lead-in - vertical Arc
            # Should be apply only on straight Path segment
            elif obj.StyleIn == "ArcZ":
                # tangent vector in XY plane
                # normal vector is vertical
                op = PathDressup.baseOp(obj.Base)
                arcRadius = length
                normalLengthMax = op.SafeHeight.Value - begin.z
                normalLength = arcRadius * (1 - math.cos(angleIn))
                if normalLength > normalLengthMax:
                    # do not exceed Normal vector max length
                    normalLength = normalLengthMax
                    # recalculate angle for limited normal length
                    angleIn = math.acos((arcRadius - normalLength) / arcRadius)
                tangentLength = arcRadius * math.sin(angleIn)
                tangent = -self.angleToVector(angleTangent) * tangentLength
                normal = App.Vector(0, 0, normalLength)
                arcBegin = begin + tangent + normal
                lead.extend(self.createArcZMoveDown(obj, arcBegin, begin, arcRadius))

            # replace 'begin' position by first lead-in command
            begin = lead[0].positionBegin()

            if obj.StyleIn in ["Arc3d", "Line3d"]:
                # up Z start point for Arc3d and Line3d
                op = PathDressup.baseOp(obj.Base)
                if inInstrPrev and inInstrPrev.z() > begin.z:
                    begin.z = inInstrPrev.z()
                else:
                    begin.z = op.StartDepth.Value
                lead[0].setPositionBegin(begin)

        # get complete start travel moves
        if obj.StyleIn != "None":
            travelToStart = self.getTravelStart(obj, begin, first, inInstrPrev, outInstrPrev)
        else:
            # exclude any lead-in commands
            horizfeed = PathDressup.toolController(obj.Base).HorizFeed.Value
            param = {"X": begin.x, "Y": begin.y, "Z": begin.z, "F": horizfeed}
            travelToStart = [PathLanguage.MoveStraight(None, "G01", param)]

        lead = travelToStart + lead

        return lead

    def getLeadEnd(self, obj, move, last, inInstrPrev, outInstrPrev):

        #            move       end   tangent
        #    x-------------------x-----_---->
        #                        |       \
        #                 normal |         \
        #                        |          |
        #                        v          x

        lead = []
        end = move.positionEnd()

        if obj.StyleOut not in ["None", "Vertical"]:
            toolRadius = PathDressup.toolController(obj.Base).Tool.Diameter.Value / 2
            angleOut = math.radians(obj.AngleOut.Value)
            length = obj.PercentageRadiusOut * toolRadius / 100
            angleTangent = move.anglesOfTangents()[1]
            normalMax = (
                self.angleToVector(angleTangent + self.getLeadDir(obj, obj.InvertOut)) * length
            )

            # Here you can find description of the calculations
            # https://forum.freecad.org/viewtopic.php?t=97641

            # append "Arc" style lead-out - arc in XY
            # Arc3d the same as Arc, but increased Z start point
            if obj.StyleOut in ["Arc", "Arc3d", "Helix"]:
                # tangent and normal vectors in XY plane
                arcRadius = length
                tangentLength = math.sin(angleOut) * arcRadius
                normalLength = arcRadius * (1 - math.cos(angleOut))
                tangent = self.angleToVector(angleTangent) * tangentLength
                normal = (
                    self.angleToVector(angleTangent + self.getLeadDir(obj, obj.InvertOut))
                    * normalLength
                )
                arcEnd = end + tangent + normal
                lead.append(self.createArcMove(obj, end, arcEnd, normalMax, obj.InvertOut))

            # append "Line" style lead-out
            # Line3d the same as Line, but increased Z start point
            elif obj.StyleOut in ["Line", "Line3d"]:
                # tangent and normal vectors in XY plane
                tangentLength = math.cos(angleOut) * length
                normalLength = math.sin(angleOut) * length
                tangent = self.angleToVector(angleTangent) * tangentLength
                normal = (
                    self.angleToVector(angleTangent + self.getLeadDir(obj, obj.InvertOut))
                    * normalLength
                )
                lineEnd = end + tangent + normal
                lead.append(self.createStraightMove(obj, end, lineEnd))

            # append "LineZ" style lead-out - vertical inclined line
            # Should be apply only on straight Path segment
            elif obj.StyleOut == "LineZ":
                # tangent vector in XY plane
                # normal vector is vertical
                op = PathDressup.baseOp(obj.Base)
                normalLengthMax = op.StartDepth.Value - end.z
                normalLength = math.sin(angleOut) * length
                # do not exceed Normal vector max length
                normalLength = min(normalLength, normalLengthMax)
                tangentLength = normalLength / math.tan(angleOut)
                tangent = self.angleToVector(angleTangent) * tangentLength
                normal = App.Vector(0, 0, normalLength)
                lineEnd = end + tangent + normal
                lead.append(self.createStraightMove(obj, end, lineEnd))

            # prepend "ArcZ" style lead-out - vertical Arc
            # Should be apply only on straight Path segment
            elif obj.StyleOut == "ArcZ":
                # tangent vector in XY plane
                # normal vector is vertical
                op = PathDressup.baseOp(obj.Base)
                arcRadius = length
                normalLengthMax = op.SafeHeight.Value - end.z
                normalLength = arcRadius * (1 - math.cos(angleOut))
                if normalLength > normalLengthMax:
                    # do not exceed Normal vector max length
                    normalLength = normalLengthMax
                    # recalculate angle for limited normal length
                    angleOut = math.acos((arcRadius - normalLength) / arcRadius)
                tangentLength = arcRadius * math.sin(angleOut)
                tangent = self.angleToVector(angleTangent) * tangentLength
                normal = App.Vector(0, 0, normalLength)
                arcEnd = end + tangent + normal
                lead.extend(self.createArcZMoveUp(obj, end, arcEnd, arcRadius))

        if obj.StyleOut in ["Arc3d", "Line3d"]:
            # Up Z end point for Arc3d and Line3d
            op = PathDressup.baseOp(obj.Base)
            print()
            print("getLeadEnd")
            print(f" inInstrPrev {inInstrPrev}")
            print(f" outInstrPrev {outInstrPrev}")
            if outInstrPrev and outInstrPrev.positionBegin().z > end.z:
                print(f"   begin {outInstrPrev.positionBegin()}")
                lead[-1].param["Z"] = outInstrPrev.positionBegin().z
            else:
                lead[-1].param["Z"] = op.StartDepth.Value

        # append travel moves to cleareance height after finish all profiles
        if last and obj.StyleOut != "None":
            lead += self.getTravelEnd(obj)

        return lead

    # Is this command a mill?
    def isCuttingMove(self, obj, instr):
        result = (
            instr.isMove()
            and not instr.isRapid()
            and (not obj.IncludeLayers or not instr.isPlunge())
        )
        return result

    # Get last index of mill command in whole Path
    def findLastCuttingMoveIndex(self, obj, source):
        for i in range(len(source) - 1, -1, -1):
            if self.isCuttingMove(obj, source[i]):
                return i

        return None

    # Get finish index of mill command for one profile
    def findLastCutMultiProfileIndex(self, obj, source, startIndex):
        for i in range(startIndex, len(source) - 1, +1):
            if not self.isCuttingMove(obj, source[i]):
                return i - 1

        return None

    # Increase travel length from begin
    def getOvertravelIn(self, obj, source, length, start, end):
        startPoint = source[start].positionBegin()
        endPoint = source[end].positionEnd()

        if Path.Geom.pointsCoincide(startPoint, endPoint):
            # closed profile
            # get extra commands from end of the closed profile
            measuredLength = 0
            for i, instr in enumerate(reversed(source[start : end + 1])):
                instrLength = instr.pathLength()

                if Path.Geom.isRoughly(measuredLength + instrLength, length, 1):
                    # get needed length and not need to cut last command
                    commands = source[end - i : end + 1]
                    return commands

                elif measuredLength + instrLength > length:
                    # measured length exceed needed length and need cut command
                    commands = source[end - i + 1 : end + 1]
                    newLength = length - measuredLength
                    newInstr = self.cutInstrBegin(obj, instr, newLength)
                    commands.insert(0, newInstr)
                    return commands

                measuredLength += instrLength

        else:
            # open profile
            # extend first move
            instr = source[start]
            newLength = length + instr.pathLength()
            newInstr = self.cutInstrBegin(obj, instr, newLength)
            return [newInstr]

        return None

    # Increse travel length from end
    def getOvertravelOut(self, obj, source, length, start, end):
        startPoint = source[start].positionBegin()
        endPoint = source[end].positionEnd()

        if Path.Geom.pointsCoincide(startPoint, endPoint):
            # closed profile
            # get extra commands from begin of the closed profile
            measuredLength = 0
            for i, instr in enumerate(source[start:end]):
                instrLength = instr.pathLength()

                if Path.Geom.isRoughly(measuredLength + instrLength, length, 1):
                    # get needed length and not need to cut last command
                    commands = source[start : start + i + 1]
                    return commands

                elif measuredLength + instrLength > length:
                    # measured length exceed needed length and need cut command
                    commands = source[start : start + i]
                    newLength = length - measuredLength
                    newInstr = self.cutInstrEnd(obj, instr, newLength)
                    commands.append(newInstr)
                    return commands

                measuredLength += instrLength

        else:
            # open profile
            # extend last move
            instr = source[end]
            newLength = length + instr.pathLength()
            newInstr = self.cutInstrEnd(obj, instr, newLength)
            return [newInstr]

        return None

    # Cut travel end by distance (negative overtravel out)
    def cutTravelEnd(self, obj, source, cutLength):
        measuredLength = 0

        for i, instr in enumerate(reversed(source)):
            instrLength = instr.pathLength()
            measuredLength += instrLength

            if Path.Geom.isRoughly(measuredLength, cutLength):
                # get needed length and not need to cut any command
                commands = source[: -i - 1]
                return commands

            elif measuredLength > cutLength:
                # measured length exceed needed cut length and need cut command
                commands = source[: -i - 1]
                newLength = measuredLength - cutLength
                newInstr = self.cutInstrEnd(obj, instr, newLength)
                commands.append(newInstr)
                return commands

        return None

    # Change end point of instruction
    def cutInstrEnd(self, obj, instr, newLength):
        command = None
        # Cut straight move from begin
        if instr.isStraight():
            begin = instr.positionBegin()
            end = instr.positionEnd()
            v = end - begin
            n = math.hypot(v.x, v.y)
            u = v / n
            cutEnd = begin + u * newLength
            command = self.createStraightMove(obj, begin, cutEnd)

        # Cut arc move from begin
        elif instr.isArc():
            angleTangent = instr.anglesOfTangents()[0]
            arcBegin = instr.positionBegin()
            arcOffset = App.Vector(instr.i(), instr.j(), instr.k())
            arcRadius = instr.arcRadius()
            arcAngle = newLength / arcRadius
            tangentLength = math.sin(arcAngle) * arcRadius
            normalLength = arcRadius * (1 - math.cos(arcAngle))
            tangent = self.angleToVector(angleTangent) * tangentLength
            normal = self.angleToVector(angleTangent + self.getPathOrientation(obj)) * normalLength
            arcEnd = arcBegin + tangent + normal
            cmdName = "G2" if instr.isCW() else "G3"
            command = self.createArcMoveN(obj, arcBegin, arcEnd, arcOffset, cmdName)

        return command

    # Change start point of instruction
    def cutInstrBegin(self, obj, instr, newLength):
        # Cut straignt move from begin
        if instr.isStraight():
            begin = instr.positionBegin()
            end = instr.positionEnd()
            v = end - begin
            n = math.hypot(v.x, v.y)
            u = v / n
            newBegin = end - u * newLength
            command = self.createStraightMove(obj, newBegin, end)
            return command

        # Cut arc move from begin
        elif instr.isArc():
            angleTangent = instr.anglesOfTangents()[1]
            arcEnd = instr.positionEnd()
            arcCenter = instr.xyCenter()
            arcRadius = instr.arcRadius()
            arcAngle = newLength / arcRadius
            tangentLength = math.sin(arcAngle) * arcRadius
            normalLength = arcRadius * (1 - math.cos(arcAngle))
            tangent = -self.angleToVector(angleTangent) * tangentLength
            normal = self.angleToVector(angleTangent + self.getPathOrientation(obj)) * normalLength
            arcBegin = arcEnd + tangent + normal
            arcOffset = arcCenter - arcBegin
            cmdName = "G2" if instr.isCW() else "G3"
            command = self.createArcMoveN(obj, arcBegin, arcEnd, arcOffset, cmdName)
            return command

        return None

    def generateLeadInOutCurve(self, obj):
        source = PathLanguage.Maneuver.FromPath(PathUtils.getPathWithPlacement(obj.Base)).instr
        maneuver = PathLanguage.Maneuver()

        # Knowing weather a given instruction is the first cutting move is easy,
        # we just use a flag and set it to false afterwards. To find the last
        # cutting move we need to search the list in reverse order.

        first = True  # prepare first move at cleareance height
        firstMillIndex = None  # Index start mill instruction for one profile
        lastCuttingMoveIndex = self.findLastCuttingMoveIndex(obj, source)
        inInstrPrev = None
        outInstrPrev = None
        measuredLength = 0
        skipCounter = 0
        commands = []
        # Process all instructions
        for i, instr in enumerate(source):
            # Process not mill instruction
            if not self.isCuttingMove(obj, instr):
                if not instr.isMove():
                    # non-move instruction get added verbatim
                    commands.append(instr)
                # skip travel and plunge moves
                # travel moves will be added in getLeadStart and getLeadEnd
                continue

            # measuring length for one profile
            if self.isCuttingMove(obj, instr):
                instrLength = instr.pathLength()
                measuredLength += instrLength

            # Process Lead-In
            if first or not self.isCuttingMove(obj, source[i - 1 - skipCounter]):
                # Process negative Offset Lead-In (cut travel from begin)
                if obj.OffsetIn.Value < 0 and obj.StyleIn != "None":
                    if measuredLength <= abs(obj.OffsetIn.Value):
                        # skip mill instruction
                        skipCounter += 1  # count skipped instructions
                        continue
                    else:
                        skipCounter = 0
                        # cut mill instruction
                        newLength = measuredLength - abs(obj.OffsetIn.Value)
                        instr = self.cutInstrBegin(obj, instr, newLength)

                # Process positive offset Lead-In (overtravel)
                firstMillIndex = i
                lastMillIndex = self.findLastCutMultiProfileIndex(obj, source, i + 1)
                overtravelIn = None
                if obj.OffsetIn.Value > 0 and obj.StyleIn != "None":
                    overtravelIn = self.getOvertravelIn(
                        obj,
                        source,
                        obj.OffsetIn.Value,
                        firstMillIndex,
                        lastMillIndex,
                    )
                if overtravelIn:
                    commands.extend(
                        self.getLeadStart(obj, overtravelIn[0], first, inInstrPrev, outInstrPrev)
                    )
                    commands.extend(overtravelIn)
                else:
                    commands.extend(self.getLeadStart(obj, instr, first, inInstrPrev, outInstrPrev))
                first = False
                firstMillIndex = i if not firstMillIndex else firstMillIndex
                inInstrPrev = commands[-1]

            # Add mill instruction
            commands.append(instr)

            # Process Lead-Out
            last = bool(i == lastCuttingMoveIndex)
            if last or not self.isCuttingMove(obj, source[i + 1]):
                measuredLength = 0  # reset measured length for one profile
                lastMillIndex = i  # index last mill instruction for one profile

                # Process negative Offset Lead-Out (cut travel from end)
                if obj.OffsetOut.Value < 0 and obj.StyleOut != "None":
                    commands = self.cutTravelEnd(obj, commands, abs(obj.OffsetOut.Value))

                # Process positive Offset Lead-Out (overtravel)
                if obj.OffsetOut.Value > 0 and obj.StyleOut != "None":
                    overtravelOut = self.getOvertravelOut(
                        obj,
                        source,
                        obj.OffsetOut.Value,
                        firstMillIndex,
                        lastMillIndex,
                    )
                    firstMillIndex = None
                    if overtravelOut:
                        commands.extend(overtravelOut)

                # add lead end and travel moves
                leadEndInstr = self.getLeadEnd(obj, commands[-1], last, inInstrPrev, outInstrPrev)
                commands.extend(leadEndInstr)

                # Last mill position to check RetractThreshold
                if leadEndInstr:
                    outInstrPrev = leadEndInstr[-1]
                else:
                    outInstrPrev = instr

        maneuver.addInstructions(commands)
        return maneuver.toPath()


class TaskDressupLeadInOut(SimpleEditPanel):
    _transaction_name = "Edit LeadInOut Dress-up"
    _ui_file = ":/panels/DressUpLeadInOutEdit.ui"

    def setupUi(self):
        # self.connectWidget("InvertIn", self.form.chkInvertLeadIn)
        # self.connectWidget("InvertOut", self.form.chkInvertLeadOut)
        # self.connectWidget("PercentageRadiusIn", self.form.dspPercentageRadiusIn)
        # self.connectWidget("LengthOut", self.form.dspLenOut)
        # self.connectWidget("ExtendLeadIn", self.form.dspExtendIn)
        # self.connectWidget("ExtendLeadOut", self.form.dspExtendOut)
        # self.connectWidget("StyleIn", self.form.cboStyleIn)
        # self.connectWidget("StyleOut", self.form.cboStyleOut)
        # self.connectWidget("RapidPlunge", self.form.chkRapidPlunge)
        # self.connectWidget("IncludeLayers", self.form.chkLayers)
        self.setFields()


class ViewProviderDressup:
    def __init__(self, vobj):
        self.obj = vobj.Object
        self.setEdit(vobj)

    def attach(self, vobj):
        self.obj = vobj.Object
        self.panel = None

    def claimChildren(self):
        if hasattr(self.obj.Base, "InList"):
            for i in self.obj.Base.InList:
                if hasattr(i, "Group"):
                    group = i.Group
                    for g in group:
                        if g.Name == self.obj.Base.Name:
                            group.remove(g)
                    i.Group = group
        return [self.obj.Base]

    def setEdit(self, vobj, mode=0):
        FreeCADGui.Control.closeDialog()
        panel = TaskDressupLeadInOut(vobj.Object, self)
        FreeCADGui.Control.showDialog(panel)
        return True

    def unsetEdit(self, vobj, mode=0):
        if self.panel:
            self.panel.abort()

    def onDelete(self, arg1=None, arg2=None):
        """this makes sure that the base operation is added back to the project and visible"""
        Path.Log.debug("Deleting Dressup")
        if arg1.Object and arg1.Object.Base:
            FreeCADGui.ActiveDocument.getObject(arg1.Object.Base.Name).Visibility = True
            job = PathUtils.findParentJob(self.obj)
            if job:
                job.Proxy.addOperation(arg1.Object.Base, arg1.Object)
            arg1.Object.Base = None
        return True

    def dumps(self):
        return None

    def loads(self, state):
        return None

    def clearTaskPanel(self):
        self.panel = None


class CommandPathDressupLeadInOut:
    def GetResources(self):
        return {
            "Pixmap": "CAM_Dressup",
            "MenuText": QT_TRANSLATE_NOOP("CAM_DressupLeadInOut", "LeadInOut"),
            "ToolTip": QT_TRANSLATE_NOOP(
                "CAM_DressupLeadInOut",
                "Creates a Cutter Radius Compensation G41/G42 Entry Dressup object from a selected path",
            ),
        }

    def IsActive(self):
        op = PathDressup.selection()
        if op:
            return not PathDressup.hasEntryMethod(op)
        return False

    def Activated(self):
        # check that the selection contains exactly what we want
        selection = FreeCADGui.Selection.getSelection()
        if len(selection) != 1:
            Path.Log.error(
                translate("CAM_DressupLeadInOut", "Please select one toolpath object") + "\n"
            )
            return
        baseObject = selection[0]
        if not baseObject.isDerivedFrom("Path::Feature"):
            Path.Log.error(
                translate("CAM_DressupLeadInOut", "The selected object is not a toolpath") + "\n"
            )
            return
        if baseObject.isDerivedFrom("Path::FeatureCompoundPython"):
            Path.Log.error(translate("CAM_DressupLeadInOut", "Please select a Profile object"))
            return

        # everything ok!
        App.ActiveDocument.openTransaction("Create LeadInOut Dressup")
        FreeCADGui.addModule("Path.Dressup.Gui.LeadInOut")
        FreeCADGui.addModule("PathScripts.PathUtils")
        FreeCADGui.doCommand(
            'obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", "LeadInOutDressup")'
        )
        FreeCADGui.doCommand("dbo = Path.Dressup.Gui.LeadInOut.ObjectDressup(obj)")
        FreeCADGui.doCommand("base = FreeCAD.ActiveDocument." + selection[0].Name)
        FreeCADGui.doCommand("job = PathScripts.PathUtils.findParentJob(base)")
        FreeCADGui.doCommand("obj.Base = base")
        FreeCADGui.doCommand("job.Proxy.addOperation(obj, base)")
        FreeCADGui.doCommand("dbo.setup(obj)")
        FreeCADGui.doCommand(
            "obj.ViewObject.Proxy = Path.Dressup.Gui.LeadInOut.ViewProviderDressup(obj.ViewObject)"
        )
        FreeCADGui.doCommand("Gui.ActiveDocument.getObject(base.Name).Visibility = False")
        App.ActiveDocument.commitTransaction()
        App.ActiveDocument.recompute()


if App.GuiUp:
    # register the FreeCAD command
    FreeCADGui.addCommand("CAM_DressupLeadInOut", CommandPathDressupLeadInOut())

Path.Log.notice("Loading CAM_DressupLeadInOut... done\n")
