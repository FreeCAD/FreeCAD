# SPDX-License-Identifier: LGPL-2.1-or-later

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
import Path.Base.Generator.spiral as spiral
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


def _caclulatePathDirection(obj):
    """Calculates the path direction"""
    compass = PathOp.Compass(obj.ToolController.SpindleDir)
    compass.cut_mode = obj.CutMode

    sides = ("Inside", "Outside")
    sideIndex = sides.index(obj.StartSide)
    if obj.ProfileSide == "External":
        sideIndex = not sideIndex
    compass.cut_side = sides[sideIndex]

    return compass.path_dir


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
        obj.addProperty(
            "App::PropertyEnumeration",
            "StartSide",
            "Helix Drill",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Start cutting from the inside or outside",
            ),
        )
        obj.addProperty(
            "App::PropertyEnumeration",
            "CutMode",
            "Helix Drill",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "The direction of the circular cuts",
            ),
        )
        obj.addProperty(
            "App::PropertyPercent",
            "StepOver",
            "Helix Drill",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Percent of cutter diameter to step over on each pass",
            ),
        )
        obj.addProperty(
            "App::PropertyDistance",
            "RadialStockToLeaveInner",
            "Helix Drill",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Offset inner radius"
                "\nDefault inner radius for Internal profile is Tool radius,"
                " and can not be less than (-ToolRadius)"
                "\nFor External profile - profile radius",
            ),
        )
        obj.addProperty(
            "App::PropertyDistance",
            "RadialStockToLeaveOuter",
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
                "Create spiral mill\nCan be used only with Single Helix",
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
            "OverrideProfileDiameter",
            "Helix Drill",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Replace profiles diameter to get identical size of the holes"
                "\nThis value can not be less than tool diameter",
            ),
        )
        obj.addProperty(
            "App::PropertyAngle",
            "DirectionAngle",
            "Helix Drill",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Determine direction of the start and end points"
                "\nSet -1 to calculate optimal angle automatically",
            ),
        )
        obj.addProperty(
            "App::PropertyLength",
            "SplitStepDown",
            "Helix Drill",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Split processing hole to several parts by depth"
                "\nSet 0 to get helices with full depth",
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

        self.opSetDefaultEditorModes(obj)

    def opOnChanged(self, obj, prop):
        if prop == "SingleHelix" and not obj.Document.Restoring:
            self.opSetEditorModes(obj)
        if not obj.Document.Restoring:
            self.opCheckParameters(obj)

    def opSetDefaultEditorModes(self, obj):
        obj.setEditorMode("Direction", ("ReadOnly", "Hidden"))
        obj.setPropertyStatus("Direction", ("ReadOnly", "Output"))

        obj.setEditorMode("DirectionAngle", 2)  # hide
        obj.setEditorMode("FinishHelixCircle", 2)  # hide
        obj.setEditorMode("FinishSpiralCircle", 2)  # hide
        obj.setEditorMode("OverrideProfileDiameter", 2)  # hide
        obj.setEditorMode("SplitStepDown", 2)  # hide

    def opSetEditorModes(self, obj):
        SpiralMillMode = 0 if obj.SingleHelix else 2
        obj.setEditorMode("SpiralMill", SpiralMillMode)

    def opCheckParameters(self, obj):
        if getattr(obj, "ToolController", None):
            tooldiam = obj.ToolController.Tool.Diameter.Value
            if obj.OverrideProfileDiameter and obj.OverrideProfileDiameter.Value < tooldiam:
                obj.OverrideProfileDiameter = 0
                Path.Log.warning(
                    translate(
                        "PathHelix",
                        "Value of OverrideProfileDiameter can not be less than tool diameter %.3f",
                    )
                    % tooldiam
                )

            if obj.ProfileSide == "Internal" and obj.RadialStockToLeaveInner.Value < -tooldiam / 2:
                obj.RadialStockToLeaveInner = -tooldiam / 2

    def opOnDocumentRestored(self, obj):
        if not hasattr(obj, "RadialStockToLeaveInner"):
            obj.addProperty(
                "App::PropertyDistance",
                "RadialStockToLeaveInner",
                "Helix Drill",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Offset inner radius"
                    "\nDefault inner radius is Tool radius"
                    " and can not be less than (-ToolRadius)"
                    "\nFor External profile - profile radius",
                ),
            )
        if hasattr(obj, "StartRadius"):
            obj.RadialStockToLeaveInner = obj.StartRadius
            obj.removeProperty("StartRadius")
        if not hasattr(obj, "RadialStockToLeaveOuter"):
            obj.addProperty(
                "App::PropertyDistance",
                "RadialStockToLeaveOuter",
                "Helix Drill",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Extra offset from the profile",
                ),
            )
        if hasattr(obj, "OffsetExtra"):
            obj.RadialStockToLeaveOuter = obj.OffsetExtra
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
                    "Create spiral mill\nCan be used only with Single Helix",
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
            obj.ProfileSide = ("Internal", "External")
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
        if not hasattr(obj, "OverrideProfileDiameter"):
            obj.addProperty(
                "App::PropertyLength",
                "OverrideProfileDiameter",
                "Helix Drill",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Replace profiles diameter to get identical size of the holes"
                    "\nThis value can not be less than tool diameter",
                ),
            )
        if not hasattr(obj, "DirectionAngle"):
            obj.addProperty(
                "App::PropertyAngle",
                "DirectionAngle",
                "Helix Drill",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Determine direction of the start and end points"
                    "\nSet -1 to calculate optimal angle automatically",
                ),
            )
        if not hasattr(obj, "SplitStepDown"):
            obj.addProperty(
                "App::PropertyLength",
                "SplitStepDown",
                "Helix Drill",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Split processing hole to several parts by depth"
                    "\nSet 0 to get helices with full depth",
                ),
            )
        if not hasattr(obj, "CutMode"):
            obj.addProperty(
                "App::PropertyEnumeration",
                "CutMode",
                "Helix Drill",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "The direction of the circular cuts",
                ),
            )
            obj.CutMode = ("Climb", "Conventional")
            if obj.Direction in ("Climb", "Conventional"):
                # For some month, late in the v1.0 release cycle, we had the cut mode assigned
                # to the direction (see PR#14364). Let's fix files created in this time as well.
                new_dir = "CW" if obj.Direction == "Climb" else "CCW"
                obj.Direction = ("CW", "CCW")
                obj.Direction = new_dir
            obj.CutMode = _caclulateCutMode(obj.Direction, obj.StartSide)

        self.opSetDefaultEditorModes(obj)
        self.opSetEditorModes(obj)

    # Automatic calculation angle of direction
    def getDirAngle(self, obj, holes, i):
        p1 = FreeCAD.Vector(holes[i]["x"], holes[i]["y"], 0)
        p2 = FreeCAD.Vector()  # by default orient to (0,0)

        if obj.StartSide == "Inside":
            if i < len(holes) - 1:
                # orient each hole (except last) to next hole
                p2 = FreeCAD.Vector(holes[i + 1]["x"], holes[i + 1]["y"], 0)
            elif len(holes) > 1:
                # orient last hole to previous hole
                p2 = FreeCAD.Vector(holes[i - 1]["x"], holes[i - 1]["y"], 0)
        else:
            if i:
                # orient each hole (except first) to previous hole
                p2 = FreeCAD.Vector(holes[i - 1]["x"], holes[i - 1]["y"], 0)
        dirAngleRad = Path.Geom.getAngle(p2 - p1)

        if math.isnan(dirAngleRad):
            # exclude 'nan' if hole placed in (0,0)
            return 0
        else:
            return dirAngleRad

    def circularHoleExecute(self, obj, holes):
        """circularHoleExecute(obj, holes) ... generate helix commands for each hole in holes"""
        Path.Log.track()
        obj.Direction = _caclulatePathDirection(obj)

        tooldiameter = obj.ToolController.Tool.Diameter.Value
        toolradius = tooldiameter / 2

        args = {
            "edge": None,
            "outer_radius": None,
            "step_down": obj.StepDown.Value,
            "step": obj.StepOver * tooldiameter / 100,
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

        self.commandlist.append(Path.Command("(helix cut operation)"))
        for index, hole in enumerate(holes):

            if obj.DirectionAngle.Value == -1:
                # Automatic calculation angle of direction
                dirAngleRad = self.getDirAngle(obj, holes, index)
            else:
                dirAngleRad = math.radians(obj.DirectionAngle.Value)

            args["dir_angle_rad"] = dirAngleRad

            if obj.OverrideProfileDiameter.Value:
                hole["d"] = obj.OverrideProfileDiameter.Value

            if obj.ProfileSide == "Internal":
                if obj.SingleHelix and obj.StartSide == "Inside":
                    args["inner_radius"] = toolradius + obj.RadialStockToLeaveInner.Value
                    args["outer_radius"] = args["inner_radius"]
                elif obj.SingleHelix and obj.StartSide == "Outside":
                    args["outer_radius"] = (
                        hole["d"] / 2 - toolradius - obj.RadialStockToLeaveOuter.Value
                    )
                    args["inner_radius"] = args["outer_radius"]
                else:
                    # not single helix mode
                    args["inner_radius"] = toolradius + obj.RadialStockToLeaveInner.Value
                    args["outer_radius"] = (
                        hole["d"] / 2 - toolradius - obj.RadialStockToLeaveOuter.Value
                    )
                    if args["inner_radius"] > args["outer_radius"]:
                        # exclude overlap inner and outer helices
                        args["inner_radius"] = args["outer_radius"]
                    elif Path.Geom.isRoughly(args["inner_radius"], 0):
                        args["inner_radius"] = toolradius

            elif obj.ProfileSide == "External":
                if obj.SingleHelix and obj.StartSide == "Inside":
                    args["inner_radius"] = (
                        hole["d"] / 2 + toolradius + obj.RadialStockToLeaveInner.Value
                    )
                    args["outer_radius"] = args["inner_radius"]
                elif obj.SingleHelix and obj.StartSide == "Outside":
                    args["outer_radius"] = (
                        hole["d"] / 2 + toolradius + obj.RadialStockToLeaveOuter.Value
                    )
                    args["inner_radius"] = args["outer_radius"]
                else:
                    args["inner_radius"] = (
                        hole["d"] / 2 + toolradius + obj.RadialStockToLeaveInner.Value
                    )
                    args["outer_radius"] = (
                        hole["d"] / 2 + toolradius + obj.RadialStockToLeaveOuter.Value
                    )
                    if args["inner_radius"] > args["outer_radius"]:
                        # exclude overlap inner and outer helices
                        args["inner_radius"] = args["outer_radius"]

            self.commandlist.append(Path.Command(f"(hole {index + 1})"))
            self.commandlist.append(Path.Command("G0", {"Z": obj.ClearanceHeight.Value}))

            # Split depth by step down with SplitStepDown
            work_distance = obj.StartDepth.Value - obj.FinalDepth.Value
            stepHeight = obj.SplitStepDown.Value if obj.SplitStepDown.Value else work_distance
            iters = math.ceil(work_distance / stepHeight)
            centerTop = FreeCAD.Vector(hole["x"], hole["y"], obj.StartDepth.Value)
            centerBottom = FreeCAD.Vector(hole["x"], hole["y"], obj.StartDepth.Value)
            safeDistance = obj.SafeHeight.Value - obj.StartDepth.Value
            iter = 0
            while iter < iters:
                iter += 1
                if iters > 1:
                    self.commandlist.append(
                        Path.Command(f"(hole {index + 1}, step {iter}/{iters})")
                    )
                centerBottom.z -= stepHeight
                if centerBottom.z < obj.FinalDepth.Value or Path.Geom.isRoughly(
                    centerBottom.z, obj.FinalDepth.Value
                ):
                    centerBottom.z = obj.FinalDepth.Value

                args["edge"] = Part.makeLine(centerTop, centerBottom)
                safeHeight = centerTop.z + safeDistance
                centerTop.z = centerBottom.z + safeDistance

                if Path.Geom.isRoughly(args["inner_radius"], 0) or Path.Geom.isRoughly(
                    args["outer_radius"], 0
                ):
                    # vertical drill for zero radius
                    self.commandlist.append(Path.Command("G0", {"X": hole["x"], "Y": hole["y"]}))
                    zDrill = obj.StartDepth.Value
                    while zDrill > centerBottom.z:
                        # drill in peck mode
                        zDrill -= min(obj.StepDown.Value, stepHeight)
                        if zDrill < centerBottom.z or Path.Geom.isRoughly(zDrill, centerBottom.z):
                            zDrill = centerBottom.z
                        self.commandlist.append(Path.Command("G0", {"Z": obj.SafeHeight.Value}))
                        self.commandlist.append(Path.Command("G1", {"Z": zDrill}))

                    if not obj.SingleHelix:
                        self.commandlist.append(Path.Command("G0", {"Z": obj.SafeHeight.Value}))

                if args["inner_radius"] > 0 and args["outer_radius"] > 0:
                    # helix drill for non zero radius
                    helixCommands = helix.generate(**args)
                    self.commandlist.append(helixCommands[1])  # move to helix start point (x,y)
                    self.commandlist.append(Path.Command("G0", {"Z": safeHeight}))
                    self.commandlist.extend(helixCommands[2:])

                if obj.SpiralMill and obj.SingleHelix:
                    if obj.ProfileSide == "Internal":
                        spiralInnerRadius = toolradius + obj.RadialStockToLeaveInner.Value
                        spiralOuterRadius = (
                            hole["d"] / 2 - toolradius - obj.RadialStockToLeaveOuter.Value
                        )
                    elif obj.ProfileSide == "External":
                        spiralInnerRadius = (
                            hole["d"] / 2 + toolradius + obj.RadialStockToLeaveInner.Value
                        )
                        spiralOuterRadius = (
                            hole["d"] / 2 + toolradius + obj.RadialStockToLeaveOuter.Value
                        )

                    if spiralOuterRadius <= spiralInnerRadius:
                        Path.Log.warning(
                            translate(
                                "PathHelix",
                                "Spiral outer radius %.3f is equal or less than inner %.3f",
                            )
                            % (spiralOuterRadius, spiralInnerRadius)
                        )
                    else:
                        while self.commandlist[-1].Name in Path.Geom.CmdMoveRapid:
                            # remove last retract movements
                            self.commandlist.pop()
                        spiralArgs = {
                            "center": centerBottom,
                            "outer_radius": spiralOuterRadius,
                            "step": obj.StepOver * tooldiameter / 100,
                            "inner_radius": spiralInnerRadius,
                            "direction": obj.Direction,
                            "startAt": obj.StartSide,
                            "dir_angle_rad": dirAngleRad,
                        }

                        # create spiral
                        spiralCommands = spiral.generate(**spiralArgs)
                        if obj.FinishSpiralCircle and (
                            (obj.StartSide == "Outside" and spiralInnerRadius)
                            or obj.StartSide == "Inside"
                        ):
                            self.commandlist.extend(spiralCommands[3:])
                        else:
                            self.commandlist.extend(spiralCommands[3:-2])

                        # Calculate retract after spiral
                        workWidth = spiralOuterRadius - spiralInnerRadius

                        if obj.StartSide == "Inside":
                            r = spiralOuterRadius
                            if spiralInnerRadius <= toolradius:
                                # center of profile is clear
                                retract_offset = -min(toolradius / 2, spiralOuterRadius)
                            else:
                                # center of profile is not clear
                                retract_offset = -min(toolradius / 2, workWidth / 2)

                        elif obj.StartSide == "Outside":
                            r = spiralInnerRadius
                            if spiralInnerRadius > toolradius:
                                # move from wall only if center is not clear
                                retract_offset = min(toolradius / 2, workWidth / 2)
                            else:
                                retract_offset = 0

                        if obj.RetractFromWall:
                            retract_offset = 0

                        # move to safe height after finish spiral
                        dx = (r + retract_offset) * math.cos(dirAngleRad)
                        dy = (r + retract_offset) * math.sin(dirAngleRad)
                        self.commandlist.append(
                            Path.Command(
                                "G0",
                                {
                                    "X": centerBottom.x + dx,
                                    "Y": centerBottom.y + dy,
                                    "Z": safeHeight,
                                },
                            )
                        )

        PathFeedRate.setFeedRate(self.commandlist, obj.ToolController)

        horizFeed = obj.ToolController.HorizFeed.Value
        vertFeed = obj.ToolController.VertFeed.Value

        if obj.OverrideArcFeedRate and horizFeed and vertFeed:
            self.overrideArcFeed(
                self.commandlist, tooldiameter, horizFeed, vertFeed, obj.ProfileSide
            )

    def overrideArcFeed(self, commandlist, tooldiameter, horizFeedTC, vertFeedTC, profileSide):
        source = PathLanguage.Maneuver.FromPath(Path.Path(commandlist)).instr
        for i, instr in enumerate(source):
            if instr.isMove() and instr.isArc():
                vertCutterMoveLength = instr.positionBegin().z - instr.positionEnd().z

                # Calculate horizontal length of the path for cutting edge
                if profileSide == "Internal":
                    horizCutterMoveLength = instr.arcAngle() * (
                        instr.arcRadius() + tooldiameter / 2
                    )
                else:
                    horizCutterMoveLength = instr.arcAngle() * (
                        instr.arcRadius() - tooldiameter / 2
                    )

                # angle between horizontal and vertical moves
                angle = math.atan(vertCutterMoveLength / horizCutterMoveLength)

                # Calculate needed horizontal feed
                factorH = abs(math.cos(angle))
                # Do not increase feed by more than x10
                factorH = max(factorH, 0.1)
                pathFeedH = horizFeedTC / factorH

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

                # Calculate needed vertical feed
                factorV = abs(math.sin(angle))
                # Do not increase feed by more than x10
                factorV = max(factorV, 0.1)
                pathFeedV = vertFeedTC / factorV

                # use minimal value from calculated vertical and horizontal feeds
                commandlist[i].F = min(pathFeedH, pathFeedV)


def SetupProperties():
    """Returns property names for which the "Setup Sheet" should provide defaults."""
    setup = []
    setup.append("CutMode")
    setup.append("StartSide")
    setup.append("StepOver")
    setup.append("RadialStockToLeaveInner")
    return setup


def Create(name, obj=None, parentJob=None):
    """Create(name) ... Creates and returns a Helix operation."""
    if obj is None:
        obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", name)
    obj.Proxy = ObjectHelix(obj, name, parentJob)
    if obj.Proxy:
        obj.Proxy.findAllHoles(obj)
    return obj
