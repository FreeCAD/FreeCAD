# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2017 LTS <SammelLothar@gmx.de>
# SPDX-FileCopyrightText: 2020 Schildkroet
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

import Constants
import FreeCAD as App
import FreeCADGui
import Part
import Path
import Path.Base.Generator.leadinout as leadinout

from Path.Base.Gui import Util as PathGuiUtil
from Path.Base.Util import toolControllerForOp
from Path.Dressup import Utils as PathDressup
from PathPythonGui.simple_edit_panel import SimpleEditPanel
from PathScripts import PathUtils as PathUtils
from Path.Base.MachineState import MachineState

__doc__ = """LeadInOut Dressup USE ROLL-ON ROLL-OFF to profile"""

from PySide.QtCore import QT_TRANSLATE_NOOP

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
    QT_TRANSLATE_NOOP("CAM_DressupLeadInOut", "ArcZFollow"),
    QT_TRANSLATE_NOOP("CAM_DressupLeadInOut", "Helix"),
    QT_TRANSLATE_NOOP("CAM_DressupLeadInOut", "Line3d"),
    QT_TRANSLATE_NOOP("CAM_DressupLeadInOut", "LineZ"),
    QT_TRANSLATE_NOOP("CAM_DressupLeadInOut", "LineZFollow"),
    QT_TRANSLATE_NOOP("CAM_DressupLeadInOut", "No Retract"),
    QT_TRANSLATE_NOOP("CAM_DressupLeadInOut", "Vertical"),
)


class ObjectDressup:
    def __init__(self, obj, base):
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
        obj.addProperty(
            "App::PropertyLength",
            "ExtendLeadIn",
            "Path Lead-in",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Extends Lead-in distance\nOnly for styles: Arc, Line, Perpendicular and Tangent",
            ),
        )
        obj.addProperty(
            "App::PropertyLength",
            "ExtendLeadOut",
            "Path Lead-out",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Extends Lead-out distance\nOnly for styles: Arc, Line, Perpendicular and Tangent",
            ),
        )
        self.obj = obj
        obj.Proxy = self
        obj.Base = base

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
        obj.AngleIn = 45
        obj.AngleOut = 45
        obj.InvertIn = False
        obj.InvertOut = False
        obj.RapidPlunge = False
        obj.StyleIn = "Arc"
        obj.StyleOut = "Arc"

        baseOp = PathDressup.baseOp(obj.Base)
        if baseOp and getattr(baseOp, "ToolController", None):
            expr = f"{baseOp.Name}.ToolController.Tool.Diameter.Value/2*1.5"
            obj.setExpression("RadiusIn", expr)
            obj.setExpression("RadiusOut", expr)
        else:
            obj.RadiusIn = 10
            obj.RadiusOut = 10

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
            baseOp = PathDressup.baseOp(obj.Base)
            if hasattr(obj, "PercentageRadiusIn"):
                obj.addProperty(
                    "App::PropertyLength",
                    "RadiusIn",
                    "Path Lead-in",
                    QT_TRANSLATE_NOOP("App::Property", "Determine length of the Lead-In"),
                )
                if baseOp and getattr(baseOp, "ToolController", None):
                    valIn = obj.PercentageRadiusIn / 100
                    exprIn = f"{baseOp.Name}.ToolController.Tool.Diameter.Value/2*{valIn}"
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
                if baseOp and getattr(baseOp, "ToolController", None):
                    valOut = obj.PercentageRadiusOut / 100
                    exprOut = f"{baseOp.Name}.ToolController.Tool.Diameter.Value/2*{valOut}"
                    obj.setExpression("RadiusOut", exprOut)
                else:
                    obj.RadiusOut = 10
                obj.removeProperty("PercentageRadiusOut")

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
        if not hasattr(obj, "ExtendLeadIn"):
            obj.addProperty(
                "App::PropertyLength",
                "ExtendLeadIn",
                "Path Lead-in",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Extends Lead-in distance"
                    "\nOnly for styles: Arc, Line, Perpendicular and Tangent",
                ),
            )
            extLeadInMode = 0 if obj.StyleIn in ("Arc", "Line", "Perpendicular", "Tangent") else 2
            obj.setEditorMode("ExtendLeadIn", extLeadInMode)
        if not hasattr(obj, "ExtendLeadOut"):
            obj.addProperty(
                "App::PropertyLength",
                "ExtendLeadOut",
                "Path Lead-out",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Extends Lead-out distance"
                    "\nOnly for styles: Arc, Line, Perpendicular and Tangent",
                ),
            )
            extLeadOutMode = 0 if obj.StyleOut in ("Arc", "Line", "Perpendicular", "Tangent") else 2
            obj.setEditorMode("ExtendLeadOut", extLeadOutMode)

        # Ensure correct initial visibility of fields after defaults are set
        for k, v in TaskDressupLeadInOut.hideModes.items():
            obj.setEditorMode(k + "In", 2 if obj.StyleIn in v else 0)
            obj.setEditorMode(k + "Out", 2 if obj.StyleOut in v else 0)

    def getPathParams(self, obj):
        self.horizFeed = 0
        self.vertFeed = 0
        self.entranceFeed = 0
        self.exitFeed = 0
        self.clearanceHeight = None
        self.safeHeight = None
        self.startDepth = None

        baseOp = PathDressup.baseOp(obj.Base)

        if getattr(baseOp, "Side", None) in ("Inside", "Outside"):
            self.side = baseOp.Side
        else:
            self.side = "Inside"

        if getattr(baseOp, "Direction", None) in ("CW", "CCW"):
            self.direction = baseOp.Direction
        else:
            self.direction = "CCW"

        toolController = toolControllerForOp(obj)
        if toolController:
            self.horizFeed = toolController.HorizFeed.Value
            self.vertFeed = toolController.VertFeed.Value
            if hasattr(toolController, "LeadInFeed"):
                self.entranceFeed = toolController.LeadInFeed.Value
            if hasattr(toolController, "LeadOutFeed"):
                self.exitFeed = toolController.LeadOutFeed.Value

        if hasattr(baseOp, "ClearanceHeight"):
            self.clearanceHeight = baseOp.ClearanceHeight.Value

        if hasattr(baseOp, "SafeHeight"):
            self.safeHeight = baseOp.SafeHeight.Value

        if (
            self.clearanceHeight is None
            or self.safeHeight is None
            or not self.horizFeed
            or not self.vertFeed
        ):

            def _isVertical(currentposition, cmd):
                x = cmd.Parameters["X"] if "X" in cmd.Parameters else currentposition.x
                y = cmd.Parameters["Y"] if "Y" in cmd.Parameters else currentposition.y
                z = cmd.Parameters["Z"] if "Z" in cmd.Parameters else currentposition.z
                endpoint = App.Vector(x, y, z)
                if Path.Geom.pointsCoincide(currentposition, endpoint):
                    return True
                return Path.Geom.isVertical(Part.makeLine(currentposition, endpoint))

            machine = MachineState()
            rapidZ = []
            for cmd in baseOp.Path.Commands:
                if cmd.Name not in Constants.GCODE_MOVE_ALL:
                    continue
                if _isVertical(machine.getPosition(), cmd):
                    if cmd.Name in Constants.GCODE_MOVE_RAPID and len(rapidZ) < 2:
                        machine.addCommand(cmd)
                        rapidZ.append(machine.getPosition().z)
                        continue
                    if (
                        cmd.Name in Constants.GCODE_MOVE_MILL
                        and not self.vertFeed
                        and "F" in cmd.Parameters
                    ):
                        machine.addCommand(cmd)
                        self.vertFeed = machine.getState()["F"]
                        continue
                elif (
                    cmd.Name in Constants.GCODE_MOVE_MILL
                    and not self.horizFeed
                    and "F" in cmd.Parameters
                ):
                    machine.addCommand(cmd)
                    self.horizFeed = machine.getState()["F"]
                    continue
                if len(rapidZ) >= 2 and self.horizFeed and self.vertFeed:
                    break
                machine.addCommand(cmd)

            if len(rapidZ) >= 2:
                if self.clearanceHeight is None:
                    self.clearanceHeight = rapidZ[0]
                if self.safeHeight is None:
                    self.safeHeight = rapidZ[1]

        if hasattr(baseOp, "StartDepth"):
            self.startDepth = baseOp.StartDepth.Value
        else:
            self.startDepth = self.safeHeight

        if not self.entranceFeed:
            self.entranceFeed = self.vertFeed
        if not self.exitFeed:
            self.exitFeed = self.horizFeed

        self.clearanceHeightOut = self.clearanceHeight
        if hasattr(baseOp, "ClearanceHeightOut"):
            self.clearanceHeightOut = baseOp.ClearanceHeightOut.Value

        return self.clearanceHeight is not None and self.safeHeight is not None

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
        if not PathDressup.baseOp(obj.Base).Active:
            path = Path.Path("(inactive operation)")
            obj.Path = path
            return
        if not obj.LeadIn and not obj.LeadOut:
            obj.Path = PathUtils.getPathWithPlacement(obj.Base)

        if obj.RadiusIn <= 0:
            obj.RadiusIn = 1
        if obj.RadiusOut <= 0:
            obj.RadiusOut = 1

        nonZeroAngleStyles = ("Arc", "Arc3d", "ArcZ", "ArcZFollow", "Helix", "LineZ", "LineZFollow")
        limit_angle_in = 0.1 if obj.StyleIn in nonZeroAngleStyles else 0
        limit_angle_out = 0.1 if obj.StyleOut in nonZeroAngleStyles else 0

        if obj.AngleIn > 180:
            obj.AngleIn = 180
        if obj.AngleIn < limit_angle_in:
            obj.AngleIn = limit_angle_in

        if obj.StyleIn in ("ArcZ", "ArcZFollow") and obj.AngleIn > 179:
            obj.AngleIn = 179
        elif obj.StyleIn == "LineZFollow" and obj.AngleIn > 89:
            obj.AngleIn = 89

        if obj.AngleOut > 180:
            obj.AngleOut = 180
        if obj.AngleOut < limit_angle_out:
            obj.AngleOut = limit_angle_out

        if obj.StyleOut in ("ArcZ", "ArcZFollow") and obj.AngleOut > 179:
            obj.AngleOut = 179
        elif obj.StyleOut == "LineZFollow" and obj.AngleOut > 89:
            obj.AngleOut = 89

        extStyles = ("Arc", "Line", "Perpendicular", "Tangent")
        extLeadInMode = 0 if obj.StyleIn in extStyles else 2
        obj.setEditorMode("ExtendLeadIn", extLeadInMode)
        extLeadOutMode = 0 if obj.StyleOut in extStyles else 2
        obj.setEditorMode("ExtendLeadOut", extLeadOutMode)

        # Use shared hideModes from TaskDressupLeadInOut
        for k, v in TaskDressupLeadInOut.hideModes.items():
            obj.setEditorMode(k + "In", 2 if obj.StyleIn in v else 0)
            obj.setEditorMode(k + "Out", 2 if obj.StyleOut in v else 0)

        job = PathUtils.findParentJob(obj)

        if not self.getPathParams(obj):
            obj.Path = Path.Path()
            Path.Log.warning(
                translate(
                    "CAM_DressupLeadInOut", "Can not get parameters from base operation and path"
                )
            )
            return

        args = {
            "path": PathUtils.getPathWithPlacement(obj.Base),
            "side": self.side,
            "direction": self.direction,
            "leadIn": obj.LeadIn,
            "leadOut": obj.LeadOut,
            "styleIn": obj.StyleIn,
            "styleOut": obj.StyleOut,
            "angleIn": obj.AngleIn.Value,
            "angleOut": obj.AngleOut.Value,
            "radiusIn": obj.RadiusIn.Value,
            "radiusOut": obj.RadiusOut.Value,
            "offsetIn": obj.OffsetIn.Value,
            "offsetOut": obj.OffsetOut.Value,
            "extendLeadIn": obj.ExtendLeadIn.Value,
            "extendLeadOut": obj.ExtendLeadOut.Value,
            "invertIn": obj.InvertIn,
            "invertOut": obj.InvertOut,
            "invertAlt": getattr(obj, "InvertAlt", False),
            "rapidPlunge": obj.RapidPlunge,
            "retractThreshold": obj.RetractThreshold.Value,
            "horizFeed": self.horizFeed,
            "vertFeed": self.vertFeed,
            "entranceFeed": self.entranceFeed,
            "exitFeed": self.exitFeed,
            "clearanceHeight": self.clearanceHeight,
            "clearanceHeightOut": self.clearanceHeightOut,
            "safeHeight": self.safeHeight,
            "startDepth": self.startDepth,
            "tolerance": job.GeometryTolerance.Value if job else 0.01,
        }

        obj.Path = leadinout.LeadInOut(**args).generate()


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
        "Invert": (
            "No Retract",
            "ArcZ",
            "ArcZFollow",
            "LineZ",
            "LineZFollow",
            "Vertical",
            "Perpendicular",
            "Tangent",
        ),
        "Offset": ("No Retract"),
        "Radius": ("No Retract", "Vertical"),
    }

    def updateLeadVisibility(self, style, angleWidget, invertWidget, angleLabel, radiusLabel=None):
        # Dynamic label for Radius/Length
        arc_styles = ("Arc", "Arc3d", "ArcZ", "ArcZFollow", "Helix")
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
        vobj.Proxy = self

    def attach(self, vobj):
        self.obj = vobj.Object
        self.panel = None

        if self.obj and self.obj.Base:
            for i in self.obj.Base.InList:
                if hasattr(i, "Group") and self.obj.Base.Name in [o.Name for o in i.Group]:
                    i.Group = [o for o in i.Group if o.Name != self.obj.Base.Name]
            if self.obj.Base.ViewObject:
                self.obj.Base.ViewObject.Visibility = False

    def claimChildren(self):
        return [self.obj.Base]

    def setEdit(self, vobj, mode=0):
        if mode == 1:
            FreeCADGui.runCommand("Std_TransformManip")
        elif mode == 0:
            FreeCADGui.Control.closeDialog()
            panel = TaskDressupLeadInOut(vobj.Object, self)
            FreeCADGui.Control.showDialog(panel)
        return True

    def unsetEdit(self, vobj, mode=0):
        if mode == 0 and self.panel:
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


class CommandPathDressup:
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
        return bool(PathDressup.selection())

    def Activated(self):
        # check that the selection contains exactly what we want
        op = PathDressup.selection(verbose=True)
        if not op:
            return

        # everything ok!
        App.ActiveDocument.openTransaction("Create LeadInOut Dressup")
        FreeCADGui.addModule("Path.Dressup.Gui.LeadInOut")
        FreeCADGui.doCommand(f"base = FreeCAD.ActiveDocument.getObject('{op.Name}')")
        FreeCADGui.doCommand("Path.Dressup.Gui.LeadInOut.Create(base)")
        # FreeCAD.ActiveDocument.commitTransaction()  # Final `commitTransaction()` called via TaskPanel.accept()
        App.ActiveDocument.recompute()


def Create(baseObject, name="DressupLeadInOut", mode=0):
    """
    Create(baseObject, name='DressupLeadInOut', mode=0) … create LeadInOut dressup object for the given base path.

    import Path.Dressup.Gui.LeadInOut as lead
    lead.Create(basePath)  # to show Task panel
    lead.Create(basePath, 2)  # to skip Task panel
    """
    if not baseObject.isDerivedFrom("Path::Feature"):
        Path.Log.error(
            translate("CAM_DressupLeadInOut", "The selected object is not a path") + "\n"
        )
        return None

    if baseObject.isDerivedFrom("Path::FeatureCompoundPython"):
        Path.Log.error(translate("CAM_DressupLeadInOut", "Select a profile object"))
        return None

    App.ActiveDocument.openTransaction("Create a DressupLeadInOut")
    obj = App.ActiveDocument.addObject("Path::FeaturePython", name)
    dbo = ObjectDressup(obj, baseObject)
    job = PathUtils.findParentJob(baseObject)
    job.Proxy.addOperation(obj, baseObject)
    dbo.setup(obj)
    ViewProviderDressup(obj.ViewObject)
    App.ActiveDocument.commitTransaction()
    obj.ViewObject.Document.setEdit(obj.ViewObject, mode)

    return obj


if App.GuiUp:
    # register the FreeCAD command
    FreeCADGui.addCommand("CAM_DressupLeadInOut", CommandPathDressup())

Path.Log.notice("Loading CAM_DressupLeadInOut… done\n")
