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
                (translate("PathProfile", "Outside"), "Outside"),
                (translate("PathProfile", "Inside"), "Inside"),
            ],  # side of profile that cutter is on in relation to direction of profile
            "CutMode": [
                (translate("CAM_Helix", "Climb"), "Climb"),
                (translate("CAM_Helix", "Conventional"), "Conventional"),
            ],  # whether the tool "rolls" with or against the feed direction along the profile
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
            "OffsetInnerRadius",
            "Helix Drill",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Offset inner radius\nDefault inner radius is Tool radius\nCan not be less than -(ToolRadius)",
            ),
        )
        obj.addProperty(
            "App::PropertyDistance",
            "OffsetHole",
            "Helix Drill",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Extra offset from the profile",
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
            "RetractCenter",
            "Helix Drill",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Retract in the center of hole if possible",
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

        ENUMS = self.helixOpPropertyEnumerations()
        for n in ENUMS:
            setattr(obj, n[0], n[1])
        obj.StepOver = 50
        obj.FinishHelixCircle = True
        obj.FinishSpiralCircle = True
        obj.setEditorMode("FinishSpiralCircle", 2)  # hide

    def opOnDocumentRestored(self, obj):
        if not hasattr(obj, "OffsetInnerRadius"):
            obj.addProperty(
                "App::PropertyDistance",
                "OffsetInnerRadius",
                "Helix Drill",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Offset inner radius\nDefault inner radius is Tool radius\nCan not be less than -(ToolRadius)",
                ),
            )
        if hasattr(obj, "StartRadius"):
            obj.OffsetInnerRadius = obj.StartRadius
            obj.removeProperty("StartRadius")
        if not hasattr(obj, "OffsetHole"):
            obj.addProperty(
                "App::PropertyDistance",
                "OffsetHole",
                "Helix Drill",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Extra offset from the profile",
                ),
            )
        if hasattr(obj, "OffsetExtra"):
            obj.OffsetHole = obj.OffsetExtra
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
        if not hasattr(obj, "RetractCenter"):
            obj.addProperty(
                "App::PropertyBool",
                "RetractCenter",
                "Helix Drill",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Retract in the center of the hole if possible",
                ),
            )
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

        if obj.SpiralMill:
            obj.setEditorMode("FinishSpiralCircle", 0)  # unhide
        else:
            obj.setEditorMode("FinishSpiralCircle", 2)  # hide

        # Inner radius can not be less than 10% of the tool radius
        if obj.OffsetInnerRadius.Value <= -toolradius * 0.9:
            obj.OffsetInnerRadius = -toolradius * 0.9

        args = {
            "edge": None,
            "hole_radius": None,
            "step_down": obj.StepDown.Value,
            "step_over": obj.StepOver / 100,
            "tool_diameter": tooldiameter,
            "inner_radius": None,
            "retract_height": obj.SafeHeight.Value,
            "direction": obj.Direction,
            "startAt": obj.StartSide,
            "retract_center": obj.RetractCenter,
            "finish_circle": obj.FinishHelixCircle,
        }

        for counter, hole in enumerate(holes):
            if obj.SingleHelix and obj.StartSide == "Inside":
                args["inner_radius"] = toolradius + obj.OffsetInnerRadius.Value
                args["hole_radius"] = args["inner_radius"] + toolradius
            elif obj.SingleHelix and obj.StartSide == "Outside":
                args["hole_radius"] = (hole["r"] / 2) - (obj.OffsetHole.Value)
                args["inner_radius"] = args["hole_radius"]
            else:
                args["inner_radius"] = toolradius + obj.OffsetInnerRadius.Value
                args["hole_radius"] = (hole["r"] / 2) - (obj.OffsetHole.Value)

            startPoint = FreeCAD.Vector(hole["x"], hole["y"], obj.StartDepth.Value)
            endPoint = FreeCAD.Vector(hole["x"], hole["y"], obj.FinalDepth.Value)
            args["edge"] = Part.makeLine(startPoint, endPoint)

            self.commandlist.append(Path.Command(f"(hole {counter + 1})"))
            self.commandlist.append(Path.Command("G0", {"Z": obj.ClearanceHeight.Value}))

            self.commandlist.extend(helix.generate(**args))

            if obj.SpiralMill:
                spiralInnerRadius = obj.OffsetInnerRadius.Value + toolradius
                spiralOuterRadius = hole["r"] / 2 - obj.OffsetHole.Value - toolradius
                if (spiralOuterRadius - spiralInnerRadius) <= 0:
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
                        "center": startPoint,
                        "innerR": spiralInnerRadius,
                        "outerR": spiralOuterRadius,
                        "increment": increment,
                        "startSide": obj.StartSide,
                        "direction": obj.Direction,
                        "finish": obj.FinishSpiralCircle,
                    }

                    self.commandlist.extend(self.createSpiral(**spiralArgs))

                    if obj.RetractCenter:
                        # Add move to center after spiral
                        center_clear = False
                        if args["hole_radius"] <= tooldiameter:  # simple case where center is clear
                            center_clear = True
                        elif (
                            obj.StartSide == "Inside" and args["inner_radius"] <= toolradius
                        ):  # middle is clear
                            center_clear = True
                        if center_clear:
                            self.commandlist.append(
                                Path.Command("G0", {"X": hole["x"], "Y": hole["y"]})
                            )

        PathFeedRate.setFeedRate(self.commandlist, obj.ToolController)

    def createSpiral(self, center, innerR, outerR, increment, startSide, direction, finish):
        # Create spiral Path
        # stepRotate = math.radians(10)  # step size for rotate (for spiral with linear movements)
        stepRotate = math.pi / 3  # step size for rotate
        stepRotate = -stepRotate if direction == "CCW" else stepRotate
        turns = math.ceil((outerR - innerR) / increment)  # amount spiral turns
        iters = int(2 * math.pi * turns / abs(stepRotate))
        stepRadius = (outerR - innerR) / iters  # step size for spiral radius
        stepRadius = -stepRadius if startSide == "Outside" else stepRadius
        count = 0
        angle = math.pi / 2
        spiralR = outerR if startSide == "Outside" else innerR
        commandlist = []
        arcPoints = [FreeCAD.Vector(center.x + spiralR, center.y, 0)]
        while count < iters:
            angle += stepRotate
            spiralR += stepRadius
            x = center.x + spiralR * math.sin(angle)
            y = center.y + spiralR * math.cos(angle)
            arcPoints.append(FreeCAD.Vector(x, y, 0))  # Get all points of arcs
            # commandlist.append(Path.Command(f"G1 X{x} Y{y}"))  # for spiral with linear movements
            count += 1

        arcCmdName = "G2" if direction == "CW" else "G3"

        i = 0
        while i <= len(arcPoints) - 3:
            arcEnd = arcPoints[i + 2]
            arcCenter = self.getArcCenter(arcPoints[i], arcPoints[i + 1], arcPoints[i + 2])
            offset = arcCenter - arcPoints[i]
            commandlist.append(
                Path.Command(f"{arcCmdName} X{arcEnd.x} Y{arcEnd.y} I{offset.x} J{offset.y}")
            )
            i += 2

        # Add finish full circle by two 180 degree arcs
        if finish:
            p1 = FreeCAD.Vector(center.x - spiralR, center.y, 0)
            commandlist.append(Path.Command(f"{arcCmdName} X{p1.x} Y{p1.y} I{-spiralR} J0"))
            p2 = FreeCAD.Vector(center.x + spiralR, center.y, 0)
            commandlist.append(Path.Command(f"{arcCmdName} X{p2.x} Y{p2.y} I{spiralR} J0"))

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


def SetupProperties():
    """Returns property names for which the "Setup Sheet" should provide defaults."""
    setup = []
    setup.append("CutMode")
    setup.append("StartSide")
    setup.append("StepOver")
    setup.append("OffsetInnerRadius")
    return setup


def Create(name, obj=None, parentJob=None):
    """Create(name) ... Creates and returns a Helix operation."""
    if obj is None:
        obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", name)
    obj.Proxy = ObjectHelix(obj, name, parentJob)
    if obj.Proxy:
        obj.Proxy.findAllHoles(obj)
    return obj
