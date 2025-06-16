# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2016 Lorenz Hüdepohl <dev@stellardeath.org>             *
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

import Path.Base.Generator.helix as helix
from PySide.QtCore import QT_TRANSLATE_NOOP
import FreeCAD
import Part
import Path
import Path.Base.FeedRate as PathFeedRate
import Path.Op.Base as PathOp
import Path.Op.CircularHoleBase as PathCircularHoleBase
import Path.Base.Language as PathLanguage

import math

__title__ = "CAM Helix Operation"
__author__ = "Lorenz Hüdepohl"
__url__ = "https://www.freecad.org"
__doc__ = "Class and implementation of Helix Drill operation"
__contributors__ = "russ4262 (Russell Johnson)"
__created__ = "2016"
__scriptVersion__ = "1b testing"
__lastModified__ = "2019-07-12 09:50 CST"


if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())

translate = FreeCAD.Qt.translate


def _caclulatePathDirection(mode, side):
    """Calculates the path direction from cut mode and cut side"""
    if mode == "Conventional" and side == "Inside":
        return "CW"
    elif mode == "Conventional" and side == "Outside":
        return "CCW"
    elif mode == "Climb" and side == "Inside":
        return "CCW"
    elif mode == "Climb" and side == "Outside":
        return "CW"
    else:
        raise ValueError(f"No mapping for '{mode}'/'{side}'")


def _caclulateCutMode(direction, side):
    """Calculates the cut mode from path direction and cut side"""
    if direction == "CW" and side == "Inside":
        return "Conventional"
    elif direction == "CW" and side == "Outside":
        return "Climb"
    elif direction == "CCW" and side == "Inside":
        return "Climb"
    elif direction == "CCW" and side == "Outside":
        return "Conventional"
    else:
        raise ValueError(f"No mapping for '{direction}'/'{side}'")


class ObjectHelix(PathCircularHoleBase.ObjectOp):
    """Proxy class for Helix operations."""

    @classmethod
    def helixOpPropertyEnumerations(self, dataType="data"):
        """helixOpPropertyEnumerations(dataType="data")... return property enumeration lists of specified dataType.
        Args:
            dataType = 'data', 'raw', 'translated'
        Notes:
        'data' is list of internal string literals used in code
        'raw' is list of (translated_text, data_string) tuples
        'translated' is list of translated string literals
        """

        # Enumeration lists for App::PropertyEnumeration properties
        enums = {
            "Direction": [
                (translate("CAM_Helix", "CW"), "CW"),
                (translate("CAM_Helix", "CCW"), "CCW"),
            ],  # this is the direction that the profile runs
            "StartSide": [
                (translate("PathProfile", "Inside"), "Inside"),
                (translate("PathProfile", "Outside"), "Outside"),
            ],  # side of profile that cutter is on in relation to direction of profile
            "CutMode": [
                (translate("CAM_Helix", "Climb"), "Climb"),
                (translate("CAM_Helix", "Conventional"), "Conventional"),
            ],  # whether the tool "rolls" with or against the feed direction along the profile
            "ProfileSide": [
                (translate("CAM_Helix", "Internal"), "Internal"),
                (translate("CAM_Helix", "External"), "External"),
            ],  # side of profile on which create Path
        }

        if dataType == "raw":
            return enums

        data = list()
        idx = 0 if dataType == "translated" else 1

        Path.Log.debug(enums)

        for k, v in enumerate(enums):
            data.append((v, [tup[idx] for tup in enums[v]]))
        Path.Log.debug(data)

        return data

    def circularHoleFeatures(self, obj):
        """circularHoleFeatures(obj) ... enable features supported by Helix."""
        return PathOp.FeatureStepDown | PathOp.FeatureBaseEdges | PathOp.FeatureBaseFaces

    def initCircularHoleOperation(self, obj):
        """initCircularHoleOperation(obj) ... create helix specific properties."""
        obj.addProperty(
            "App::PropertyEnumeration",
            "Direction",
            "Helix Drill",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "The direction of the circular cuts, ClockWise (CW), or CounterClockWise (CCW)",
            ),
        )
        obj.setEditorMode("Direction", ["ReadOnly", "Hidden"])
        obj.setPropertyStatus("Direction", ["ReadOnly", "Output"])

        obj.addProperty(
            "App::PropertyEnumeration",
            "StartSide",
            "Helix Drill",
            QT_TRANSLATE_NOOP("App::Property", "Start cutting from the inside or outside"),
        )
        obj.addProperty(
            "App::PropertyEnumeration",
            "CutMode",
            "Helix Drill",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "The direction of the circular cuts, ClockWise (Climb), or CounterClockWise (Conventional)",
            ),
        )
        obj.addProperty(
            "App::PropertyPercent",
            "StepOver",
            "Helix Drill",
            QT_TRANSLATE_NOOP(
                "App::Property", "Percent of cutter diameter to step over on each pass"
            ),
        )
        obj.addProperty(
            "App::PropertyDistance",
            "OffsetInner",
            "Helix Drill",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Offset inner radius\nDefault inner radius for Internal profile is Tool radius,\nfor External profile - profile radius",
            ),
        )
        obj.addProperty(
            "App::PropertyDistance",
            "OffsetProfile",
            "Helix Drill",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Extra offset from the profile",
            ),
        )
        obj.addProperty(
            "App::PropertyAngle",
            "HelixConeAngle",
            "Helix Drill",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Cone angle of the Helix",
            ),
        )
        obj.addProperty(
            "App::PropertyBool",
            "SingleHelix",
            "Helix Drill",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Create only one Helix",
            ),
        )
        obj.addProperty(
            "App::PropertyBool",
            "SpiralMill",
            "Helix Drill",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Create one helix and spiral mill",
            ),
        )
        obj.addProperty(
            "App::PropertyBool",
            "FinishHelixCircle",
            "Helix Drill",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Create finish full circle for helix",
            ),
        )
        obj.addProperty(
            "App::PropertyBool",
            "FinishSpiralCircle",
            "Helix Drill",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Create finish full circle for spiral",
            ),
        )
        obj.addProperty(
            "App::PropertyEnumeration",
            "ProfileSide",
            "Helix Drill",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Side of profile on which create Path",
            ),
        )
        obj.addProperty(
            "App::PropertyBool",
            "RetractFromWall",
            "Helix Drill",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Move from wall while retract if there is free space",
            ),
        )
        obj.addProperty(
            "App::PropertyBool",
            "OverrideArcFeedRate",
            "Helix Drill",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Override arcs feed rate to get constant tool cutting speed",
            ),
        )
        obj.addProperty(
            "App::PropertyLength",
            "ReplaceProfileDiameter",
            "Helix Drill",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Replace profiles diameter to get identical size of the holes\nThis value can not be less than tool diameter",
            ),
        )
        obj.addProperty(
            "App::PropertyAngle",
            "DirectionAngle",
            "Helix Drill",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Determine direction of the start and end points\nSet -1 to calculate angle automatically",
            ),
        )
        ENUMS = self.helixOpPropertyEnumerations()
        for n in ENUMS:
            setattr(obj, n[0], n[1])
        obj.StepOver = 50
        obj.FinishHelixCircle = True
        obj.FinishSpiralCircle = True
        obj.CutMode = "Conventional"
        obj.RetractFromWall = True
        obj.OverrideArcFeedRate = True
        obj.DirectionAngle = -1
        obj.setEditorMode("DirectionAngle", 2)  # hide
        obj.setEditorMode("FinishHelixCircle", 2)  # hide
        obj.setEditorMode("FinishSpiralCircle", 2)  # hide
        obj.setEditorMode("ReplaceProfileDiameter", 2)  # hide

    def opOnDocumentRestored(self, obj):
        if not hasattr(obj, "OffsetInner"):
            obj.addProperty(
                "App::PropertyDistance",
                "OffsetInner",
                "Helix Drill",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Offset inner radius\nDefault inner radius is Tool radius\nCan not be less than -(ToolRadius)",
                ),
            )
        if hasattr(obj, "StartRadius"):
            obj.OffsetInner = obj.StartRadius
            obj.removeProperty("StartRadius")
        if not hasattr(obj, "OffsetProfile"):
            obj.addProperty(
                "App::PropertyDistance",
                "OffsetProfile",
                "Helix Drill",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Extra offset from the profile",
                ),
            )
        if hasattr(obj, "OffsetExtra"):
            obj.OffsetProfile = obj.OffsetExtra
            obj.removeProperty("OffsetExtra")
        if not hasattr(obj, "SingleHelix"):
            obj.addProperty(
                "App::PropertyBool",
                "SingleHelix",
                "Helix Drill",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Create only one Helix",
                ),
            )
        if not hasattr(obj, "SpiralMill"):
            obj.addProperty(
                "App::PropertyBool",
                "SpiralMill",
                "Helix Drill",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Create one helix and spiral mill",
                ),
            )
            obj.setEditorMode("SpiralMill", 2)  # hide
        if not hasattr(obj, "FinishHelixCircle"):
            obj.addProperty(
                "App::PropertyBool",
                "FinishHelixCircle",
                "Helix Drill",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Create finish full circle for helix",
                ),
            )
            obj.FinishHelixCircle = True
            obj.setEditorMode("FinishHelixCircle", 2)  # hide
        if not hasattr(obj, "FinishSpiralCircle"):
            obj.addProperty(
                "App::PropertyBool",
                "FinishSpiralCircle",
                "Helix Drill",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Create finish full circle for spiral",
                ),
            )
            obj.FinishSpiralCircle = True
            obj.setEditorMode("FinishSpiralCircle", 2)  # hide
        if not hasattr(obj, "HelixConeAngle"):
            obj.addProperty(
                "App::PropertyAngle",
                "HelixConeAngle",
                "Helix Drill",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Cone angle of the Helix",
                ),
            )
        if not hasattr(obj, "ProfileSide"):
            obj.addProperty(
                "App::PropertyEnumeration",
                "ProfileSide",
                "Helix Drill",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Side of profile on which create Path",
                ),
            )
            obj.ProfileSide = ["Internal", "External"]
            obj.ProfileSide = "Internal"
        if not hasattr(obj, "RetractFromWall"):
            obj.addProperty(
                "App::PropertyBool",
                "RetractFromWall",
                "Helix Drill",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Move from wall while retract if there is free space",
                ),
            )
            obj.RetractFromWall = True
        if not hasattr(obj, "OverrideArcFeedRate"):
            obj.addProperty(
                "App::PropertyBool",
                "OverrideArcFeedRate",
                "Helix Drill",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Override arcs feed rate to get constant tool cutting speed",
                ),
            )
        if not hasattr(obj, "ReplaceProfileDiameter"):
            obj.addProperty(
                "App::PropertyLength",
                "ReplaceProfileDiameter",
                "Helix Drill",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Replace profiles diameter to get identical size of the holes\nThis value can not be less than tool diameter",
                ),
            )
            obj.setEditorMode("ReplaceProfileDiameter", 2)  # hide
        if not hasattr(obj, "DirectionAngle"):
            obj.addProperty(
                "App::PropertyAngle",
                "DirectionAngle",
                "Helix Drill",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Determine direction of the start and end points\nSet -1 to calculate angle automatically",
                ),
            )
            obj.DirectionAngle = 0
        if not hasattr(obj, "CutMode"):
            # TODO: consolidate the duplicate definitions from opOnDocumentRestored and
            # initCircularHoleOperation once back on the main line
            obj.addProperty(
                "App::PropertyEnumeration",
                "CutMode",
                "Helix Drill",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "The direction of the circular cuts, ClockWise (Climb), or CounterClockWise (Conventional)",
                ),
            )
            obj.CutMode = ["Climb", "Conventional"]
            if obj.Direction in ["Climb", "Conventional"]:
                # For some month, late in the v1.0 release cycle, we had the cut mode assigned
                # to the direction (see PR#14364). Let's fix files created in this time as well.
                new_dir = "CW" if obj.Direction == "Climb" else "CCW"
                obj.Direction = ["CW", "CCW"]
                obj.Direction = new_dir
            obj.CutMode = _caclulateCutMode(obj.Direction, obj.StartSide)
            obj.setEditorMode("Direction", ["ReadOnly", "Hidden"])
            obj.setPropertyStatus("Direction", ["ReadOnly", "Output"])

    def circularHoleExecute(self, obj, holes):
        """circularHoleExecute(obj, holes) ... generate helix commands for each hole in holes"""
        Path.Log.track()
        obj.Direction = _caclulatePathDirection(obj.CutMode, obj.StartSide)

        self.commandlist.append(Path.Command("(helix cut operation)"))

        tooldiameter = obj.ToolController.Tool.Diameter.Value
        toolradius = tooldiameter / 2

        if 0 < obj.ReplaceProfileDiameter.Value < tooldiameter:
            # replacement value can not be less than tool diameter
            obj.ReplaceProfileDiameter = 0
            Path.Log.warning(
                translate(
                    "PathHelix",
                    "Value of ReplaceProfileDiameter can not be less than tool diameter %.3f",
                )
                % tooldiameter
            )

        if obj.SingleHelix:
            obj.setEditorMode("SpiralMill", 0)  # unhide
        else:
            obj.setEditorMode("SpiralMill", 2)  # hide

        if obj.ProfileSide == "Internal":
            # Limit min InnerRadius by 5% of the tool radius
            # So OffsetInner can not be less than -95% of the tool radius
            if obj.OffsetInner.Value < -toolradius:
                obj.OffsetInner = -toolradius

        args = {
            "edge": None,
            "outer_radius": None,
            "step_down": obj.StepDown.Value,
            "step_over": obj.StepOver * tooldiameter / 100,
            "tool_diameter": 0,
            "inner_radius": None,
            "retract_height": obj.SafeHeight.Value,
            "direction": obj.Direction,
            "startAt": obj.StartSide,
            "finish_circle": obj.FinishHelixCircle,
            "cone_angle_rad": None,
            "dir_angle_rad": None,
        }

        if obj.RetractFromWall:
            # do not send tooldiameter to generator for vertical retract
            args["tool_diameter"] = tooldiameter

        if obj.ProfileSide == "Internal":
            args["cone_angle_rad"] = math.radians(obj.HelixConeAngle.Value)
        else:
            args["cone_angle_rad"] = -math.radians(obj.HelixConeAngle.Value)

        for index, hole in enumerate(holes):
            # Automatic calculation angle of direction
            if obj.DirectionAngle.Value == -1:
                if obj.StartSide == "Inside" and index < len(holes) - 1:
                    p1 = FreeCAD.Vector(hole["x"], hole["y"], 0)
                    p2 = FreeCAD.Vector(holes[index + 1]["x"], holes[index + 1]["y"], 0)
                    dirAngleRad = Path.Geom.getAngle(p2 - p1)
                elif obj.StartSide == "Outside" and index > 0:
                    p1 = FreeCAD.Vector(hole["x"], hole["y"], 0)
                    p2 = FreeCAD.Vector(holes[index - 1]["x"], holes[index - 1]["y"], 0)
                    dirAngleRad = Path.Geom.getAngle(p2 - p1)
                else:
                    dirAngleRad = 0
            else:
                dirAngleRad = math.radians(obj.DirectionAngle.Value)

            args["dir_angle_rad"] = dirAngleRad

            if obj.ReplaceProfileDiameter.Value:
                hole["d"] = obj.ReplaceProfileDiameter.Value

            if obj.ProfileSide == "Internal":
                if obj.SingleHelix and obj.StartSide == "Inside":
                    args["inner_radius"] = toolradius + obj.OffsetInner.Value
                    args["outer_radius"] = args["inner_radius"]
                elif obj.SingleHelix and obj.StartSide == "Outside":
                    args["outer_radius"] = hole["d"] / 2 - toolradius - obj.OffsetProfile.Value
                    args["inner_radius"] = args["outer_radius"]
                else:
                    args["inner_radius"] = toolradius + obj.OffsetInner.Value
                    args["outer_radius"] = hole["d"] / 2 - toolradius - obj.OffsetProfile.Value
                    if args["inner_radius"] > args["outer_radius"]:
                        # exclude overlap inner and outer helices
                        args["inner_radius"] = args["outer_radius"]

            elif obj.ProfileSide == "External":
                if obj.SingleHelix and obj.StartSide == "Inside":
                    args["inner_radius"] = hole["d"] / 2 + toolradius + obj.OffsetInner.Value
                    args["outer_radius"] = args["inner_radius"]
                elif obj.SingleHelix and obj.StartSide == "Outside":
                    args["outer_radius"] = hole["d"] / 2 + toolradius + obj.OffsetProfile.Value
                    args["inner_radius"] = args["outer_radius"]
                else:
                    args["inner_radius"] = hole["d"] / 2 + toolradius + obj.OffsetInner.Value
                    args["outer_radius"] = hole["d"] / 2 + toolradius + obj.OffsetProfile.Value
                    if args["inner_radius"] > args["outer_radius"]:
                        # exclude overlap inner and outer helices
                        args["inner_radius"] = args["outer_radius"]

            centerTop = FreeCAD.Vector(hole["x"], hole["y"], obj.StartDepth.Value)
            centerBottom = FreeCAD.Vector(hole["x"], hole["y"], obj.FinalDepth.Value)
            args["edge"] = Part.makeLine(centerTop, centerBottom)

            self.commandlist.append(Path.Command(f"(hole {index + 1})"))
            self.commandlist.append(Path.Command("G0", {"Z": obj.ClearanceHeight.Value}))

            if (obj.ProfileSide == "Internal" and obj.OffsetInner.Value == -toolradius) or args[
                "outer_radius"
            ] == 0:
                # vertical drill for zero radius
                self.commandlist.append(Path.Command("G0", {"X": hole["x"], "Y": hole["y"]}))
                zDrill = obj.SafeHeight.Value
                while zDrill > obj.FinalDepth.Value:
                    # drill in peck mode
                    zDrill -= obj.StepDown.Value
                    if zDrill < obj.FinalDepth.Value or Path.Geom.isRoughly(
                        zDrill, obj.FinalDepth.Value
                    ):
                        zDrill = obj.FinalDepth.Value
                    self.commandlist.append(Path.Command("G0", {"Z": obj.SafeHeight.Value}))
                    self.commandlist.append(Path.Command("G1", {"Z": zDrill}))
            else:
                # helix drill for non zero radius
                helixCommands = helix.generate(**args)
                self.commandlist.append(helixCommands[1])  # move to helix start point (x,y)
                self.commandlist.append(Path.Command("G0", {"Z": obj.SafeHeight.Value}))
                self.commandlist.extend(helixCommands[2:])

            if obj.SpiralMill and obj.SingleHelix:
                if obj.ProfileSide == "Internal":
                    spiralInnerRadius = toolradius + obj.OffsetInner.Value
                    spiralOuterRadius = hole["d"] / 2 - toolradius - obj.OffsetProfile.Value
                elif obj.ProfileSide == "External":
                    spiralInnerRadius = hole["d"] / 2 + toolradius + obj.OffsetInner.Value
                    spiralOuterRadius = hole["d"] / 2 + toolradius + obj.OffsetProfile.Value

                if spiralOuterRadius <= spiralInnerRadius:
                    Path.Log.warning(
                        translate(
                            "PathHelix", "Spiral outer radius %.3f is equal or less than inner %.3f"
                        )
                        % (spiralOuterRadius, spiralInnerRadius)
                    )
                else:
                    while self.commandlist[-1].Name == "G0":
                        # Remove last retract movements
                        self.commandlist.pop()
                    increment = obj.StepOver * tooldiameter / 100
                    spiralArgs = {
                        "center": centerBottom,
                        "innerR": spiralInnerRadius,
                        "outerR": spiralOuterRadius,
                        "increment": increment,
                        "startSide": obj.StartSide,
                        "direction": obj.Direction,
                        "finish": obj.FinishSpiralCircle,
                        "dirAngleRad": dirAngleRad,
                    }

                    # create spiral
                    self.commandlist.extend(self.createSpiral(**spiralArgs))

                    # Add move from wall after spiral if possible
                    retract_offset = 0
                    workWidth = spiralOuterRadius - spiralInnerRadius

                    if obj.StartSide == "Inside":
                        r = spiralOuterRadius
                        if spiralInnerRadius <= toolradius:
                            # center of profile is clear
                            retract_offset = -min(toolradius / 2, spiralInnerRadius)
                        else:
                            # center of profile is not clear
                            retract_offset = -min(toolradius / 2, workWidth / 2)

                    elif obj.StartSide == "Outside":
                        r = spiralInnerRadius
                        if spiralInnerRadius > toolradius:
                            # move from wall only if center is not clear
                            retract_offset = min(toolradius / 2, workWidth / 2)

                    if retract_offset:
                        # move from wall
                        dx = (r + retract_offset) * math.cos(dirAngleRad)
                        dy = (r + retract_offset) * math.sin(dirAngleRad)
                        self.commandlist.append(
                            Path.Command(
                                "G0",
                                {
                                    "X": centerBottom.x + dx,
                                    "Y": centerBottom.y + dy,
                                    "Z": obj.SafeHeight.Value,
                                },
                            )
                        )

        PathFeedRate.setFeedRate(self.commandlist, obj.ToolController)

        if obj.OverrideArcFeedRate:
            self.overrideArcFeed(self.commandlist, obj.ToolController, obj.ProfileSide)

    def createSpiral(
        self, center, innerR, outerR, increment, startSide, direction, finish, dirAngleRad
    ):
        # Create spiral Path
        # stepRotate = math.radians(10)  # step size for rotate (for spiral with linear movements)
        stepRotate = math.pi / 3  # step size for rotate
        stepRotate = -stepRotate if direction == "CCW" else stepRotate
        turns = math.ceil((outerR - innerR) / increment)  # amount spiral turns
        iters = int(math.tau * turns / abs(stepRotate))
        stepRadius = (outerR - innerR) / iters  # step size for spiral radius
        stepRadius = -stepRadius if startSide == "Outside" else stepRadius
        count = 0
        angle = math.pi / 2 - dirAngleRad
        spiralR = outerR if startSide == "Outside" else innerR
        commandlist = []
        arcPoints = []
        while count <= iters:
            x = center.x + spiralR * math.sin(angle)
            y = center.y + spiralR * math.cos(angle)
            arcPoints.append(FreeCAD.Vector(x, y, 0))  # Get all points of arcs
            # commandlist.append(Path.Command("G1", {"X":x, "Y":y}))  # for spiral with linear movements
            if count == iters:
                break
            angle += stepRotate
            spiralR += stepRadius
            count += 1

        arcCmdName = "G2" if direction == "CW" else "G3"

        i = 0
        while i <= len(arcPoints) - 3:
            arcEnd = arcPoints[i + 2]
            arcCenter = self.getArcCenter(arcPoints[i], arcPoints[i + 1], arcPoints[i + 2])
            offset = arcCenter - arcPoints[i]
            commandlist.append(
                Path.Command(
                    arcCmdName, {"X": arcEnd.x, "Y": arcEnd.y, "I": offset.x, "J": offset.y}
                )
            )
            i += 2

        # Add finish full circle by two 180 degree arcs
        if finish:
            dx = spiralR * math.cos(dirAngleRad)
            dy = spiralR * math.sin(dirAngleRad)
            p1 = FreeCAD.Vector(center.x - dx, center.y - dy, 0)
            commandlist.append(Path.Command(arcCmdName, {"X": p1.x, "Y": p1.y, "I": -dx, "J": -dy}))
            p2 = FreeCAD.Vector(center.x + dx, center.y + dy, 0)
            commandlist.append(Path.Command(arcCmdName, {"X": p2.x, "Y": p2.y, "I": dx, "J": dy}))

        return commandlist

    def getArcCenter(self, p1, p2, p3):
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

    def overrideArcFeed(self, commandlist, toolController, profileSide):
        tooldiameter = toolController.Tool.Diameter.Value
        horizFeedTC = toolController.HorizFeed.Value
        vertFeedTC = toolController.VertFeed.Value

        source = PathLanguage.Maneuver.FromPath(Path.Path(commandlist)).instr
        for i, instr in enumerate(source):
            if instr.isMove() and instr.isArc():
                vertMoveLength = instr.positionBegin().z - instr.positionEnd().z
                horizMoveLength = instr.arcAngle() * instr.arcRadius()
                # angle between horizontal and vertical moves
                angle = math.atan(vertMoveLength / horizMoveLength)

                # to get HorizFeed from tool controller, path feed shoud be
                if angle < math.pi / 2:
                    pathFeedH = horizFeedTC / math.cos(angle)
                else:
                    pathFeedH = 10 * horizFeedTC
                pathFeedH = min(10 * horizFeedTC, pathFeedH)
                # circular tool paths correction
                # https://www.harveyperformance.com/in-the-loupe/machining-circular-tool-paths
                if profileSide == "Internal":
                    # internal circular tool path
                    majorDiameter = 2 * instr.arcRadius() + tooldiameter
                    factorHoriz = (majorDiameter - tooldiameter) / majorDiameter
                else:
                    # external circular tool path
                    majorDiameter = 2 * instr.arcRadius() - tooldiameter
                    factorHoriz = (majorDiameter + tooldiameter) / majorDiameter
                pathFeedH *= factorHoriz

                # to get VertFeed from tool controller, path feed should be
                if angle > 0:
                    pathFeedV = vertFeedTC / math.sin(angle)
                else:
                    pathFeedV = 10 * vertFeedTC
                pathFeedV = min(10 * vertFeedTC, pathFeedV)

                # use minimal value from calculated vertical and horizontal feeds
                commandlist[i].F = min(pathFeedH, pathFeedV)


def SetupProperties():
    """Returns property names for which the "Setup Sheet" should provide defaults."""
    setup = []
    setup.append("CutMode")
    setup.append("StartSide")
    setup.append("StepOver")
    setup.append("OffsetInner")
    return setup


def Create(name, obj=None, parentJob=None):
    """Create(name) ... Creates and returns a Helix operation."""
    if obj is None:
        obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", name)
    obj.Proxy = ObjectHelix(obj, name, parentJob)
    if obj.Proxy:
        obj.Proxy.findAllHoles(obj)
    return obj
