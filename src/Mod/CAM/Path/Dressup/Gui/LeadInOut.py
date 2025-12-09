# SPDX-License-Identifier: LGPL-2.1-or-later

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
import Path.Base.Gui.Util as PathGuiUtil
from Path.Base.Util import toolControllerForOp
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

lead_styles = (
    # common options first
    QT_TRANSLATE_NOOP("CAM_DressupLeadInOut", "Arc"),
    QT_TRANSLATE_NOOP("CAM_DressupLeadInOut", "Line"),
    QT_TRANSLATE_NOOP("CAM_DressupLeadInOut", "Perpendicular"),
    QT_TRANSLATE_NOOP("CAM_DressupLeadInOut", "Tangent"),
    # additional options, alphabetical order
    QT_TRANSLATE_NOOP("CAM_DressupLeadInOut", "Arc3d"),
    QT_TRANSLATE_NOOP("CAM_DressupLeadInOut", "ArcZ"),
    QT_TRANSLATE_NOOP("CAM_DressupLeadInOut", "Helix"),
    QT_TRANSLATE_NOOP("CAM_DressupLeadInOut", "Line3d"),
    QT_TRANSLATE_NOOP("CAM_DressupLeadInOut", "LineZ"),
    QT_TRANSLATE_NOOP("CAM_DressupLeadInOut", "No Retract"),
    QT_TRANSLATE_NOOP("CAM_DressupLeadInOut", "Vertical"),
)


class ObjectDressup:
    def __init__(self, obj):
        self.obj = obj
        obj.addProperty(
            "App::PropertyLink",
            "Base",
            "Path",
            QT_TRANSLATE_NOOP("App::Property", "The base toolpath to modify"),
        )
        obj.addProperty(
            "App::PropertyBool",
            "LeadIn",
            "Path",
            QT_TRANSLATE_NOOP("App::Property", "Modify lead in to toolpath"),
        )
        obj.addProperty(
            "App::PropertyBool",
            "LeadOut",
            "Path",
            QT_TRANSLATE_NOOP("App::Property", "Modify lead out from toolpath"),
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
            QT_TRANSLATE_NOOP("App::Property", "The style of motion into the toolpath"),
        )
        obj.StyleIn = lead_styles
        obj.addProperty(
            "App::PropertyEnumeration",
            "StyleOut",
            "Path",
            QT_TRANSLATE_NOOP("App::Property", "The style of motion out of the toolpath"),
        )
        obj.StyleOut = lead_styles
        obj.addProperty(
            "App::PropertyBool",
            "RapidPlunge",
            "Path",
            QT_TRANSLATE_NOOP("App::Property", "Perform plunges with G0"),
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
            "App::PropertyLength",
            "RadiusIn",
            "Path Lead-in",
            QT_TRANSLATE_NOOP("App::Property", "Determine length of the Lead-In"),
        )
        obj.addProperty(
            "App::PropertyLength",
            "RadiusOut",
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

    def dumps(self):
        return None

    def loads(self, state):
        return None

    def onChanged(self, obj, prop):
        if prop == "Path" and obj.ViewObject:
            obj.ViewObject.signalChangeIcon()

    def setup(self, obj):
        obj.LeadIn = True
        obj.LeadOut = True
        obj.AngleIn = 90
        obj.AngleOut = 90
        obj.InvertIn = False
        obj.InvertOut = False
        obj.RapidPlunge = False
        obj.StyleIn = "Arc"
        obj.StyleOut = "Arc"

        baseWithTC = self.getBaseWithTC(obj)
        if baseWithTC and baseWithTC.ToolController:
            expr = f"{baseWithTC.Name}.ToolController.Tool.Diameter.Value/2*1.5"
            obj.setExpression("RadiusIn", expr)
            obj.setExpression("RadiusOut", expr)
        else:
            obj.RadiusIn = 10
            obj.RadiusOut = 10

    def getBaseWithTC(self, obj):
        if hasattr(obj, "ToolController"):
            return obj
        if not hasattr(obj, "Base"):
            return None
        if isinstance(obj.Base, list) and obj.Base and obj.Base[0].isDerivedFrom("Path::Feature"):
            return self.getBaseWithTC(obj.Base[0])
        if not isinstance(obj.Base, list) and obj.Base.isDerivedFrom("Path::Feature"):
            return self.getBaseWithTC(obj.Base)
        return None

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

        if obj.RadiusIn <= 0:
            obj.RadiusIn = 1
        if obj.RadiusOut <= 0:
            obj.RadiusOut = 1

        nonZeroAngleStyles = ("Arc", "Arc3d", "ArcZ", "Helix", "LineZ")
        limit_angle_in = 1 if obj.StyleIn in nonZeroAngleStyles else 0
        limit_angle_out = 1 if obj.StyleOut in nonZeroAngleStyles else 0
        if obj.AngleIn > 180:
            obj.AngleIn = 180
        if obj.AngleIn < limit_angle_in:
            obj.AngleIn = limit_angle_in

        if obj.AngleOut > 180:
            obj.AngleOut = 180
        if obj.AngleOut < limit_angle_out:
            obj.AngleOut = limit_angle_out

        # Use shared hideModes from TaskDressupLeadInOut
        for k, v in TaskDressupLeadInOut.hideModes.items():
            obj.setEditorMode(k + "In", 2 if obj.StyleIn in v else 0)
            obj.setEditorMode(k + "Out", 2 if obj.StyleOut in v else 0)

        self.baseOp = PathDressup.baseOp(obj.Base)
        self.toolController = toolControllerForOp(obj.Base)
        if not self.toolController:
            obj.Path = Path.Path()
            Path.Log.warning(
                translate(
                    "CAM_DressupLeadInOut", "Tool controller not selected for base operation: %s"
                )
                % obj.Base.Label
            )
            return

        self.invertAlt = False
        self.job = PathUtils.findParentJob(obj)
        self.horizFeed = self.toolController.HorizFeed.Value
        self.vertFeed = self.toolController.VertFeed.Value
        self.clearanceHeight = self.baseOp.ClearanceHeight.Value
        self.safeHeight = self.baseOp.SafeHeight.Value
        self.startDepth = self.baseOp.StartDepth.Value
        self.side = self.baseOp.Side if hasattr(self.baseOp, "Side") else "Inside"
        if hasattr(self.baseOp, "Direction") and self.baseOp.Direction in ("CW", "CCW"):
            self.direction = self.baseOp.Direction
        else:
            self.direction = "CCW"
        self.entranceFeed = self.toolController.LeadInFeed.Value
        self.exitFeed = self.toolController.LeadOutFeed.Value

        obj.Path = self.generateLeadInOutCurve(obj)

    def onDocumentRestored(self, obj):
        """onDocumentRestored(obj) ... Called automatically when document is restored."""
        styleOn = styleOff = None
        if hasattr(obj, "StyleOn"):
            # Replace StyleOn by StyleIn
            styleOn = obj.StyleOn
            obj.addProperty(
                "App::PropertyEnumeration",
                "StyleIn",
                "Path",
                QT_TRANSLATE_NOOP("App::Property", "The style of motion into the toolpath"),
            )
            obj.StyleIn = lead_styles
            obj.removeProperty("StyleOn")
            # Set previous value if possible
            if styleOn in lead_styles:
                obj.StyleIn = styleOn
            elif styleOn == "Arc":
                obj.StyleIn = "Arc"
                obj.AngleIn = 90
        if hasattr(obj, "StyleOff"):
            # Replace StyleOff by StyleOut
            styleOff = obj.StyleOff
            obj.addProperty(
                "App::PropertyEnumeration",
                "StyleOut",
                "Path",
                QT_TRANSLATE_NOOP("App::Property", "The style of motion out of the toolpath"),
            )
            obj.StyleOut = lead_styles
            obj.removeProperty("StyleOff")
            # Set previous value if possible
            if styleOff in lead_styles:
                obj.StyleOut = styleOff
            elif styleOff == "Arc":
                obj.StyleOut = "Arc"
                obj.AngleOut = 90

        if not hasattr(obj, "AngleIn"):
            obj.addProperty(
                "App::PropertyAngle",
                "AngleIn",
                "Path Lead-in",
                QT_TRANSLATE_NOOP("App::Property", "Angle of the Lead-In (1..90)"),
            )
            obj.AngleIn = 90
        if not hasattr(obj, "AngleOut"):
            obj.addProperty(
                "App::PropertyAngle",
                "AngleOut",
                "Path Lead-out",
                QT_TRANSLATE_NOOP("App::Property", "Angle of the Lead-Out (1..90)"),
            )
            obj.AngleOut = 90

        if styleOn:
            if styleOn == "Arc":
                obj.StyleIn = "Arc"
                obj.AngleIn = 90

        if styleOff:
            if styleOff == "Arc":
                obj.StyleOut = "Arc"
                obj.AngleOut = 90

        for prop in ("Length", "LengthIn"):
            if hasattr(obj, prop):
                obj.renameProperty(prop, "RadiusIn")
                break

        if hasattr(obj, "LengthOut"):
            obj.renameProperty("LengthOut", "RadiusOut")

        if hasattr(obj, "PercentageRadiusIn") or hasattr(obj, "PercentageRadiusOut"):
            baseWithTC = self.getBaseWithTC(obj)
            if hasattr(obj, "PercentageRadiusIn"):
                obj.addProperty(
                    "App::PropertyLength",
                    "RadiusIn",
                    "Path Lead-in",
                    QT_TRANSLATE_NOOP("App::Property", "Determine length of the Lead-In"),
                )
                if baseWithTC and baseWithTC.ToolController:
                    valIn = obj.PercentageRadiusIn / 100
                    exprIn = f"{baseWithTC.Name}.ToolController.Tool.Diameter.Value/2*{valIn}"
                    obj.setExpression("RadiusIn", exprIn)
                else:
                    obj.RadiusIn = 10
                obj.removeProperty("PercentageRadiusIn")

            if hasattr(obj, "PercentageRadiusOut"):
                obj.addProperty(
                    "App::PropertyLength",
                    "RadiusOut",
                    "Path Lead-out",
                    QT_TRANSLATE_NOOP("App::Property", "Determine length of the Lead-Out"),
                )
                if baseWithTC and baseWithTC.ToolController:
                    valOut = obj.PercentageRadiusOut / 100
                    exprOut = f"{baseWithTC.Name}.ToolController.Tool.Diameter.Value/2*{valOut}"
                    obj.setExpression("RadiusOut", exprOut)
                else:
                    obj.RadiusOut = 10
                obj.removeProperty("PercentageRadiusOut")

        # The new features do not have a good analog for ExtendLeadIn/Out, so these old values will be ignored
        if hasattr(obj, "ExtendLeadIn"):
            # Remove ExtendLeadIn property
            obj.removeProperty("ExtendLeadIn")
        if hasattr(obj, "ExtendLeadOut"):
            # Remove ExtendLeadOut property
            obj.removeProperty("ExtendLeadOut")
        if hasattr(obj, "IncludeLayers"):
            obj.removeProperty("IncludeLayers")

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
            if obj.KeepToolDown:
                obj.RetractThreshold = 999999
            obj.removeProperty("KeepToolDown")

        # Ensure correct initial visibility of fields after defaults are set
        for k, v in TaskDressupLeadInOut.hideModes.items():
            obj.setEditorMode(k + "In", 2 if obj.StyleIn in v else 0)
            obj.setEditorMode(k + "Out", 2 if obj.StyleOut in v else 0)

    # Get direction for lead-in/lead-out in XY plane
    def getLeadDir(self, obj, invert=False):
        output = math.pi / 2
        side = self.side
        direction = self.direction
        if (side == "Inside" and direction == "CW") or (side == "Outside" and direction == "CCW"):
            output = -output
        if invert:
            output = -output
        if self.invertAlt:
            output = -output

        return output

    # Get direction of original path
    def getArcPathDir(self, obj, cmdName):
        # only CW/CCW and G2/G3 matters
        direction = self.direction
        output = math.pi / 2
        if direction == "CW":
            output = -output

        if cmdName == "G2" and direction == "CCW":
            output = -output
        elif cmdName == "G3" and direction == "CW":
            output = -output

        return output

    # Create safety movements to start point
    def getTravelStart(self, obj, pos, first, outInstrPrev):
        commands = []
        posPrev = outInstrPrev.positionEnd() if outInstrPrev else App.Vector()
        posPrevXY = App.Vector(posPrev.x, posPrev.y, 0)
        posXY = App.Vector(pos.x, pos.y, 0)
        distance = posPrevXY.distanceToPoint(posXY)

        if first or (distance > obj.RetractThreshold):
            # move to clearance height
            commands.append(PathLanguage.MoveStraight(None, "G00", {"Z": self.clearanceHeight}))

            # move to mill position at clearance height
            commands.append(PathLanguage.MoveStraight(None, "G00", {"X": pos.x, "Y": pos.y}))

            # move vertical down to mill position
            if obj.RapidPlunge:
                # move to mill position rapidly
                commands.append(PathLanguage.MoveStraight(None, "G00", {"Z": pos.z}))
            else:
                # move to mill position in two steps
                commands.append(PathLanguage.MoveStraight(None, "G00", {"Z": self.safeHeight}))
                commands.append(
                    PathLanguage.MoveStraight(None, "G01", {"Z": pos.z, "F": self.vertFeed})
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
                        None, "G01", {"X": pos.x, "Y": pos.y, "Z": pos.z, "F": self.vertFeed}
                    )
                )

        return commands

    # Create commands with movements to clearance height
    def getTravelEnd(self, obj):
        commands = []
        z = self.clearanceHeight
        commands.append(PathLanguage.MoveStraight(None, "G00", {"Z": z}))

        return commands

    # Create vector object from angle
    def angleToVector(self, angle):
        return App.Vector(math.cos(angle), math.sin(angle), 0)

    # Create arc in XY plane with automatic detection G2|G3
    def createArcMove(self, obj, begin, end, offset, invert, feedRate):
        param = {
            "X": end.x,
            "Y": end.y,
            "Z": end.z,
            "I": offset.x,
            "J": offset.y,
            "F": feedRate,
        }
        if self.getLeadDir(obj, invert) > 0:
            command = PathLanguage.MoveArcCCW(begin, "G3", param)
        else:
            command = PathLanguage.MoveArcCW(begin, "G2", param)

        return command

    # Create arc in XY plane with manually set G2|G3
    def createArcMoveN(self, obj, begin, end, offset, cmdName, feedRate):
        param = {"X": end.x, "Y": end.y, "I": offset.x, "J": offset.y, "F": feedRate}
        if cmdName == "G2":
            command = PathLanguage.MoveArcCW(begin, cmdName, param)
        else:
            command = PathLanguage.MoveArcCCW(begin, cmdName, param)

        return command

    # Create line movement G1
    def createStraightMove(self, obj, begin, end, feedRate):
        param = {"X": end.x, "Y": end.y, "Z": end.z, "F": feedRate}
        command = PathLanguage.MoveStraight(begin, "G1", param)

        return command

    # Get optimal step angle for iteration ArcZ
    def getStepAngleArcZ(self, obj, radius, segm=1):
        minArcLength = self.job.GeometryTolerance.Value * 2
        maxArcLength = segm
        stepAngle = math.pi / 60
        stepArcLength = stepAngle * radius
        if stepArcLength > maxArcLength:
            # limit max arc length by 1 mm
            stepAngle = maxArcLength / radius
        elif stepArcLength < minArcLength:
            # limit min arc length by geometry tolerance
            stepAngle = minArcLength / radius

        return stepAngle

    # Create vertical arc with move Down by line segments
    def createArcZMoveDown(self, obj, begin, end, radius, feedRate):
        commands = []
        angle = math.acos((radius - begin.z + end.z) / radius)  # start angle
        stepAngle = self.getStepAngleArcZ(obj, radius)
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
            param = {"X": iterEnd.x, "Y": iterEnd.y, "Z": iterEnd.z, "F": feedRate}
            commands.append(PathLanguage.MoveStraight(iterBegin, "G1", param))
            iterBegin = copy.copy(iterEnd)
            iter += 1

        return commands

    # Create vertical arc with move Up by line segments
    def createArcZMoveUp(self, obj, begin, end, radius, feedRate):
        commands = []
        angleMax = math.acos((radius - end.z + begin.z) / radius)  # finish angle
        stepAngle = self.getStepAngleArcZ(obj, radius)
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
            param = {"X": iterEnd.x, "Y": iterEnd.y, "Z": iterEnd.z, "F": feedRate}
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
        beginZ = move.positionBegin().z  # do not change this variable below

        if not obj.LeadIn and obj.LeadOut:
            # can not skip leadin if leadout
            # override style to get correct move to next step down
            styleIn = "Vertical"
        else:
            styleIn = obj.StyleIn

        if styleIn not in ("No Retract", "Vertical"):
            if styleIn == "Perpendicular":
                angleIn = math.pi / 2
            elif styleIn == "Tangent":
                angleIn = 0
            else:
                angleIn = math.radians(obj.AngleIn.Value)

            length = obj.RadiusIn.Value
            angleTangent = move.anglesOfTangents()[0]
            normalMax = (
                self.angleToVector(angleTangent + self.getLeadDir(obj, obj.InvertIn)) * length
            )

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
                    self.angleToVector(angleTangent + self.getLeadDir(obj, obj.InvertIn))
                    * normalLength
                )
                arcBegin = begin + tangent + normal
                arcCenter = begin + normalMax
                arcOffset = arcCenter - arcBegin
                lead.append(
                    self.createArcMove(
                        obj, arcBegin, begin, arcOffset, obj.InvertIn, self.entranceFeed
                    )
                )

            # prepend "Line" style lead-in - line in XY
            # Line3d the same as Line, but increased Z start point
            elif styleIn in ("Line", "Line3d", "Perpendicular", "Tangent"):
                # tangent and normal vectors in XY plane
                tangentLength = math.cos(angleIn) * length
                normalLength = math.sin(angleIn) * length
                tangent = -self.angleToVector(angleTangent) * tangentLength
                normal = (
                    self.angleToVector(angleTangent + self.getLeadDir(obj, obj.InvertIn))
                    * normalLength
                )
                lineBegin = begin + tangent + normal
                lead.append(self.createStraightMove(obj, lineBegin, begin, self.entranceFeed))

            # prepend "LineZ" style lead-in - vertical inclined line
            # Should be applied only on straight Path segment
            elif styleIn == "LineZ":
                # tangent vector in XY plane
                # normal vector is vertical
                normalLengthMax = self.safeHeight - begin.z
                normalLength = math.sin(angleIn) * length
                # do not exceed Normal vector max length
                normalLength = min(normalLength, normalLengthMax)
                tangentLength = normalLength / math.tan(angleIn)
                tangent = -self.angleToVector(angleTangent) * tangentLength
                normal = App.Vector(0, 0, normalLength)
                lineBegin = begin + tangent + normal
                lead.append(self.createStraightMove(obj, lineBegin, begin, self.entranceFeed))

            # prepend "ArcZ" style lead-in - vertical Arc
            # Should be applied only on straight Path segment or open profile
            elif styleIn == "ArcZ":
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
                normal = App.Vector(0, 0, normalLength)
                arcBegin = begin + tangent + normal
                lead.extend(
                    self.createArcZMoveDown(obj, arcBegin, begin, arcRadius, self.entranceFeed)
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

        if obj.StyleOut == "Helix" and outInstrPrev:
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
            travelToStart = self.getTravelStart(obj, begin, first, outInstrPrev)
        else:
            # exclude any lead-in commands
            param = {"X": begin.x, "Y": begin.y, "Z": begin.z, "F": self.entranceFeed}
            travelToStart = [PathLanguage.MoveStraight(None, "G01", param)]

        lead = travelToStart + lead

        return lead

    def getLeadEnd(self, obj, move, last, outInstrPrev):
        #            move       end   tangent
        #    x-------------------x-----_---->
        #                        |       \
        #                 normal |         \
        #                        |          |
        #                        v          x

        lead = []
        end = move.positionEnd()

        if obj.StyleOut not in ("No Retract", "Vertical"):
            if obj.StyleOut == "Perpendicular":
                angleOut = math.pi / 2
            elif obj.StyleOut == "Tangent":
                angleOut = 0
            else:
                angleOut = math.radians(obj.AngleOut.Value)

            length = obj.RadiusOut.Value
            angleTangent = move.anglesOfTangents()[1]
            normalMax = (
                self.angleToVector(angleTangent + self.getLeadDir(obj, obj.InvertOut)) * length
            )

            # Here you can find description of the calculations
            # https://forum.freecad.org/viewtopic.php?t=97641

            # append "Arc" style lead-out - arc in XY
            # Arc3d the same as Arc, but increased Z start point
            if obj.StyleOut in ("Arc", "Arc3d", "Helix"):
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
                lead.append(
                    self.createArcMove(obj, end, arcEnd, normalMax, obj.InvertOut, self.exitFeed)
                )

            # append "Line" style lead-out
            # Line3d the same as Line, but increased Z start point
            elif obj.StyleOut in ("Line", "Line3d", "Perpendicular", "Tangent"):
                # tangent and normal vectors in XY plane
                tangentLength = math.cos(angleOut) * length
                normalLength = math.sin(angleOut) * length
                tangent = self.angleToVector(angleTangent) * tangentLength
                normal = (
                    self.angleToVector(angleTangent + self.getLeadDir(obj, obj.InvertOut))
                    * normalLength
                )
                lineEnd = end + tangent + normal
                lead.append(self.createStraightMove(obj, end, lineEnd, self.exitFeed))

            # append "LineZ" style lead-out - vertical inclined line
            # Should be apply only on straight Path segment
            elif obj.StyleOut == "LineZ":
                # tangent vector in XY plane
                # normal vector is vertical
                normalLengthMax = self.startDepth - end.z
                normalLength = math.sin(angleOut) * length
                # do not exceed Normal vector max length
                normalLength = min(normalLength, normalLengthMax)
                tangentLength = normalLength / math.tan(angleOut)
                tangent = self.angleToVector(angleTangent) * tangentLength
                normal = App.Vector(0, 0, normalLength)
                lineEnd = end + tangent + normal
                lead.append(self.createStraightMove(obj, end, lineEnd, self.exitFeed))

            # prepend "ArcZ" style lead-out - vertical Arc
            # Should be apply only on straight Path segment
            elif obj.StyleOut == "ArcZ":
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
                normal = App.Vector(0, 0, normalLength)
                arcEnd = end + tangent + normal
                lead.extend(self.createArcZMoveUp(obj, end, arcEnd, arcRadius, self.exitFeed))

        if obj.StyleOut in ("Arc3d", "Line3d"):
            # Up Z end point for Arc3d and Line3d
            if outInstrPrev and outInstrPrev.positionBegin().z > end.z:
                lead[-1].param["Z"] = outInstrPrev.positionBegin().z
            else:
                lead[-1].param["Z"] = self.startDepth

        # append travel moves to clearance height after finishing all profiles
        if last and obj.StyleOut != "No Retract":
            lead += self.getTravelEnd(obj)

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
        if startIndex >= len(self.source):
            return len(self.source) - 1
        for i in range(startIndex, len(self.source), +1):
            if not self.isCuttingMove(self.source[i]):
                return i - 1

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
    def getOvertravelIn(self, obj, length):
        start = self.firstMillIndex
        end = self.lastMillIndex

        if self.closedProfile:
            # closed profile
            # get extra commands from end of the closed profile
            measuredLength = 0
            for i, instr in enumerate(reversed(self.source[start : end + 1])):
                instrLength = instr.pathLength()

                if Path.Geom.isRoughly(measuredLength + instrLength, length):
                    # get needed length without needing to cut last command
                    commands = self.source[end - i : end + 1]
                    return commands

                elif measuredLength + instrLength > length:
                    # measured length exceeds needed length and needs cut command
                    commands = self.source[end - i + 1 : end + 1]
                    newLength = length - measuredLength
                    newInstr = self.cutInstrBegin(obj, instr, newLength)
                    commands.insert(0, newInstr)
                    return commands

                measuredLength += instrLength

        else:
            # open profile
            # extend first move
            instr = self.source[start]
            newLength = length + instr.pathLength()
            newInstr = self.cutInstrBegin(obj, instr, newLength)
            return [newInstr]

        return None

    # Increase travel length from end, take commands from profile start
    def getOvertravelOut(self, obj, length):
        start = self.firstMillIndex
        end = self.lastMillIndex
        if self.closedProfile:
            # closed profile
            # get extra commands from begin of the closed profile
            measuredLength = 0
            for i, instr in enumerate(self.source[start : end + 1]):
                instrLength = instr.pathLength()

                if Path.Geom.isRoughly(measuredLength + instrLength, length):
                    # get needed length without needing to cut last command
                    commands = self.source[start : start + i + 1]
                    return commands

                elif measuredLength + instrLength > length:
                    # measured length exceeds needed length and needs cut command
                    commands = self.source[start : start + i]
                    newLength = length - measuredLength
                    newInstr = self.cutInstrEnd(obj, instr, newLength)
                    commands.append(newInstr)
                    return commands

                measuredLength += instrLength

        else:
            # open profile
            # extend last move
            instr = self.source[end]
            newLength = length + instr.pathLength()
            newInstr = self.cutInstrEnd(obj, instr, newLength)
            return [newInstr]

        return None

    # Cut travel end by distance (negative overtravel out)
    def cutTravelEnd(self, obj, commands, cutLength):
        measuredLength = 0
        for i, instr in enumerate(reversed(commands)):
            if instr.positionBegin() is None:
                # workaround if cut whole profile by negative offset
                cmds = commands[:-i]
                newInstr = self.cutInstrEnd(obj, commands[-i], 0.1)
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
                newInstr = self.cutInstrEnd(obj, instr, newLength)
                cmds.append(newInstr)
                return cmds

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
            command = self.createStraightMove(obj, begin, cutEnd, self.horizFeed)

        # Cut arc move from begin
        elif instr.isArc():
            cmdName = instr.cmd
            angleTangent = instr.anglesOfTangents()[0]
            arcBegin = instr.positionBegin()
            arcOffset = App.Vector(instr.i(), instr.j(), instr.k())
            arcRadius = instr.arcRadius()
            arcAngle = newLength / arcRadius
            tangentLength = math.sin(arcAngle) * arcRadius
            normalLength = arcRadius * (1 - math.cos(arcAngle))
            tangent = self.angleToVector(angleTangent) * tangentLength
            normal = (
                self.angleToVector(angleTangent + self.getArcPathDir(obj, cmdName)) * normalLength
            )
            arcEnd = arcBegin + tangent + normal
            command = self.createArcMoveN(obj, arcBegin, arcEnd, arcOffset, cmdName, self.horizFeed)

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
            command = self.createStraightMove(obj, newBegin, end, self.horizFeed)
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
            normal = (
                self.angleToVector(angleTangent + self.getArcPathDir(obj, cmdName)) * normalLength
            )
            arcBegin = arcEnd + tangent + normal
            arcOffset = arcCenter - arcBegin
            command = self.createArcMoveN(obj, arcBegin, arcEnd, arcOffset, cmdName, self.horizFeed)
            return command

        return None

    def generateLeadInOutCurve(self, obj):
        source = PathLanguage.Maneuver.FromPath(PathUtils.getPathWithPlacement(obj.Base)).instr
        self.source = source
        maneuver = PathLanguage.Maneuver()

        # Knowing whether a given instruction is the first cutting move is easy,
        # we just use a flag and set it to false afterwards. To find the last
        # cutting move we need to search the list in reverse order.

        commands = []
        first = True  # prepare first move at clearance height
        self.firstMillIndex = None  # Index start mill instruction for one profile
        self.lastMillIndex = None  # Index end mill instruction for one profile
        self.lastCuttingMoveIndex = self.findLastCuttingMoveIndex()
        self.closedProfile = True
        inInstrPrev = None  # for RetractThreshold
        outInstrPrev = None  # for RetractThreshold
        measuredLength = 0  # for negative OffsetIn
        skipCounter = 0  # for negative OffsetIn
        moveDir = None

        # Process all instructions
        for i, instr in enumerate(source):
            # Process without mill instruction
            if not self.isCuttingMove(instr):
                if not instr.isMove():
                    # non-move instruction gets added verbatim
                    commands.append(instr)
                else:
                    moveDir = self.getMoveDir(instr)
                    if (not obj.LeadIn and not obj.LeadOut) and (
                        moveDir in ("Down", "Hor") or first
                    ):
                        # keep original Lead-in movements
                        commands.append(instr)
                    if not obj.LeadOut and moveDir == "Up" and not first:
                        # keep original Lead-out movements
                        commands.append(instr)
                # skip travel and plunge moves if LeadInOut will be process
                # travel moves will be added in getLeadStart and getLeadEnd
                continue

            # measuring length for one profile
            if self.isCuttingMove(instr):
                measuredLength += instr.pathLength()

            # Process Lead-In
            if first or not self.isCuttingMove(source[i - 1 - skipCounter]):
                if obj.LeadIn or obj.LeadOut:
                    # can not skip leadin if leadout

                    self.firstMillIndex = i if self.firstMillIndex is None else self.firstMillIndex
                    self.lastMillIndex = (
                        self.findLastCutMultiProfileIndex()
                        if self.lastMillIndex is None
                        else self.lastMillIndex
                    )

                    self.closedProfile = self.isProfileClosed()

                    overtravelIn = None
                    if obj.OffsetIn.Value < 0 and obj.StyleIn != "No Retract":
                        # Process negative Offset Lead-In (cut travel from begin)
                        if measuredLength <= abs(obj.OffsetIn.Value):
                            # skip mill instruction
                            skipCounter += 1  # count skipped instructions
                            continue
                        else:
                            skipCounter = 0
                            # cut mill instruction
                            newLength = measuredLength - abs(obj.OffsetIn.Value)
                            instr = self.cutInstrBegin(obj, instr, newLength)

                    elif obj.OffsetIn.Value > 0 and obj.StyleIn != "No Retract":
                        # Process positive offset Lead-In (overtravel)
                        overtravelIn = self.getOvertravelIn(obj, obj.OffsetIn.Value)
                    if overtravelIn:
                        commands.extend(
                            self.getLeadStart(
                                obj, overtravelIn[0], first, inInstrPrev, outInstrPrev
                            )
                        )
                        commands.extend(overtravelIn)
                    else:
                        commands.extend(
                            self.getLeadStart(obj, instr, first, inInstrPrev, outInstrPrev)
                        )
                    inInstrPrev = commands[-1]
                first = False

            # Add mill instruction
            commands.append(instr)

            # Process Lead-Out
            last = bool(i == self.lastCuttingMoveIndex)
            if last or not self.isCuttingMove(source[i + 1]):
                if obj.LeadOut:

                    # Process negative Offset Lead-Out (cut travel from end)
                    if obj.OffsetOut.Value < 0 and obj.StyleOut != "No Retract":
                        commands = self.cutTravelEnd(obj, commands, abs(obj.OffsetOut.Value))

                    # Process positive Offset Lead-Out (overtravel)
                    if obj.OffsetOut.Value > 0 and obj.StyleOut != "No Retract":
                        overtravelOut = self.getOvertravelOut(obj, obj.OffsetOut.Value)
                        if overtravelOut:
                            commands.extend(overtravelOut)

                    # add lead end and travel moves
                    leadEndInstr = self.getLeadEnd(obj, commands[-1], last, outInstrPrev)
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
                self.invertAlt = not self.invertAlt if getattr(obj, "InvertAlt", None) else False

        maneuver.addInstructions(commands)
        return maneuver.toPath()


class TaskDressupLeadInOut(SimpleEditPanel):
    _transaction_name = "Edit LeadInOut Dress-up"
    _ui_file = ":/panels/DressUpLeadInOutEdit.ui"

    def setupUi(self):
        self.setupSpinBoxes()
        self.setupGroupBoxes()
        self.setupDynamicVisibility()
        self.setFields()
        self.pageRegisterSignalHandlers()

    def setupSpinBoxes(self):
        self.connectWidget("InvertIn", self.form.chkInvertDirectionIn)
        self.connectWidget("InvertOut", self.form.chkInvertDirectionOut)
        self.connectWidget("StyleIn", self.form.cboStyleIn)
        self.connectWidget("StyleOut", self.form.cboStyleOut)
        self.radiusIn = PathGuiUtil.QuantitySpinBox(self.form.dspRadiusIn, self.obj, "RadiusIn")
        self.radiusOut = PathGuiUtil.QuantitySpinBox(self.form.dspRadiusOut, self.obj, "RadiusOut")
        self.angleIn = PathGuiUtil.QuantitySpinBox(self.form.dspAngleIn, self.obj, "AngleIn")
        self.angleOut = PathGuiUtil.QuantitySpinBox(self.form.dspAngleOut, self.obj, "AngleOut")
        self.offsetIn = PathGuiUtil.QuantitySpinBox(self.form.dspOffsetIn, self.obj, "OffsetIn")
        self.offsetOut = PathGuiUtil.QuantitySpinBox(self.form.dspOffsetOut, self.obj, "OffsetOut")
        self.connectWidget("RapidPlunge", self.form.chkRapidPlunge)
        self.retractThreshold = PathGuiUtil.QuantitySpinBox(
            self.form.dspRetractThreshold, self.obj, "RetractThreshold"
        )

        self.radiusIn.updateWidget()
        self.radiusOut.updateWidget()
        self.angleIn.updateWidget()
        self.angleOut.updateWidget()
        self.offsetIn.updateWidget()
        self.offsetOut.updateWidget()
        self.retractThreshold.updateWidget()

    def setupGroupBoxes(self):
        self.form.groupBoxIn.setChecked(self.obj.LeadIn)
        self.form.groupBoxOut.setChecked(self.obj.LeadOut)
        self.form.groupBoxIn.clicked.connect(self.handleGroupBoxCheck)
        self.form.groupBoxOut.clicked.connect(self.handleGroupBoxCheck)

    def handleGroupBoxCheck(self):
        self.obj.LeadIn = self.form.groupBoxIn.isChecked()
        self.obj.LeadOut = self.form.groupBoxOut.isChecked()

    def setupDynamicVisibility(self):
        self.form.cboStyleIn.currentIndexChanged.connect(self.updateLeadInVisibility)
        self.form.cboStyleOut.currentIndexChanged.connect(self.updateLeadOutVisibility)
        self.updateLeadInVisibility()
        self.updateLeadOutVisibility()

    def getSignalsForUpdate(self):
        signals = []
        signals.append(self.form.dspRadiusIn.editingFinished)
        signals.append(self.form.dspRadiusOut.editingFinished)
        signals.append(self.form.dspAngleIn.editingFinished)
        signals.append(self.form.dspAngleOut.editingFinished)
        signals.append(self.form.dspOffsetIn.editingFinished)
        signals.append(self.form.dspOffsetOut.editingFinished)
        signals.append(self.form.dspRetractThreshold.editingFinished)
        return signals

    def pageGetFields(self):
        PathGuiUtil.updateInputField(self.obj, "RadiusIn", self.form.dspRadiusIn)
        PathGuiUtil.updateInputField(self.obj, "RadiusOut", self.form.dspRadiusOut)
        PathGuiUtil.updateInputField(self.obj, "AngleIn", self.form.dspAngleIn)
        PathGuiUtil.updateInputField(self.obj, "AngleOut", self.form.dspAngleOut)
        PathGuiUtil.updateInputField(self.obj, "OffsetIn", self.form.dspOffsetIn)
        PathGuiUtil.updateInputField(self.obj, "OffsetOut", self.form.dspOffsetOut)
        PathGuiUtil.updateInputField(self.obj, "RetractThreshold", self.form.dspRetractThreshold)

    def pageRegisterSignalHandlers(self):
        for signal in self.getSignalsForUpdate():
            signal.connect(self.pageGetFields)

    # Shared hideModes for both LeadIn and LeadOut
    hideModes = {
        "Angle": ("No Retract", "Perpendicular", "Tangent", "Vertical"),
        "Invert": ("No Retract", "ArcZ", "LineZ", "Vertical", "Perpendicular", "Tangent"),
        "Offset": ("No Retract"),
        "Radius": ("No Retract", "Vertical"),
    }

    def updateLeadVisibility(self, style, angleWidget, invertWidget, angleLabel, radiusLabel=None):
        # Dynamic label for Radius/Length
        arc_styles = ("Arc", "Arc3d", "ArcZ", "Helix")
        if radiusLabel and hasattr(self.form, radiusLabel):
            if style in arc_styles:
                getattr(self.form, radiusLabel).setText("Radius")
                # Will do translation later
                # getattr(self.form, radiusLabel).setText(translate("CAM_DressupLeadInOut", "Radius"))
            else:
                getattr(self.form, radiusLabel).setText("Length")
                # Will do translation later
                # getattr(self.form, radiusLabel).setText(translate("CAM_DressupLeadInOut", "Length"))

        # Angle
        if style in self.hideModes["Angle"]:
            angleWidget.hide()
            if hasattr(self.form, angleLabel):
                getattr(self.form, angleLabel).hide()
        else:
            angleWidget.show()
            if hasattr(self.form, angleLabel):
                getattr(self.form, angleLabel).show()
        # Invert Direction
        if style in self.hideModes["Invert"]:
            invertWidget.hide()
        else:
            invertWidget.show()

    def updateLeadInVisibility(self):
        style = self.form.cboStyleIn.currentText()
        self.updateLeadVisibility(
            style, self.form.dspAngleIn, self.form.chkInvertDirectionIn, "label_1", "label_5"
        )

    def updateLeadOutVisibility(self):
        style = self.form.cboStyleOut.currentText()
        self.updateLeadVisibility(
            style, self.form.dspAngleOut, self.form.chkInvertDirectionOut, "label_11", "label_15"
        )


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

    def getIcon(self):
        if getattr(PathDressup.baseOp(self.obj), "Active", True):
            return ":/icons/CAM_Dressup.svg"
        else:
            return ":/icons/CAM_OpActive.svg"


class CommandPathDressupLeadInOut:
    def GetResources(self):
        return {
            "Pixmap": "CAM_Dressup",
            "MenuText": QT_TRANSLATE_NOOP("CAM_DressupLeadInOut", "Lead In/Out"),
            "ToolTip": QT_TRANSLATE_NOOP(
                "CAM_DressupLeadInOut",
                "Creates entry and exit motions for a selected path",
            ),
        }

    def IsActive(self):
        selection = FreeCADGui.Selection.getSelection()
        if len(selection) != 1:
            return False
        if not selection[0].isDerivedFrom("Path::Feature"):
            return False
        baseOp = PathDressup.baseOp(selection[0])
        if not hasattr(baseOp, "ClearanceHeight"):
            return False
        if not hasattr(baseOp, "SafeHeight"):
            return False
        if not hasattr(baseOp, "StartDepth"):
            return False

        return True

    def Activated(self):
        # check that the selection contains exactly what we want
        selection = FreeCADGui.Selection.getSelection()
        if len(selection) != 1:
            Path.Log.error(translate("CAM_DressupLeadInOut", "Select one toolpath object") + "\n")
            return
        baseObject = selection[0]
        if not baseObject.isDerivedFrom("Path::Feature"):
            Path.Log.error(
                translate("CAM_DressupLeadInOut", "The selected object is not a toolpath") + "\n"
            )
            return
        if baseObject.isDerivedFrom("Path::FeatureCompoundPython"):
            Path.Log.error(translate("CAM_DressupLeadInOut", "Select a Profile object"))
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

Path.Log.notice("Loading CAM_DressupLeadInOut… done\n")
