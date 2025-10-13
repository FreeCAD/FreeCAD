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


import FreeCAD
import FreeCADGui
import Part
import Path
import Path.Op.Base as OpBase
import Path.Op.Util as PathOpUtil
import Path.Base.Util as PathUtil

import PathScripts.PathUtils as PathUtils

from PySide.QtCore import QT_TRANSLATE_NOOP

import math

__title__ = "CAM Path from Shape with Tool Controller"
__author__ = "tarman3"
__inspirer__ = "Russ4262"
__url__ = "https://forum.freecad.org/viewtopic.php?t=93896"
__doc__ = ""


if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())

translate = FreeCAD.Qt.translate


class ObjectPathShape:
    def __init__(self, obj, job):
        self.Type = "PathShapeObject"
        self.obj = obj
        self.job = job
        obj.Proxy = self

        # Base properties group
        obj.addProperty(
            "App::PropertyBool",
            "Active",
            "Base",
            QT_TRANSLATE_NOOP(
                "App::Property", "Make 'False' to prevent operation from generating code"
            ),
        )
        obj.addProperty(
            "App::PropertyString",
            "Comment",
            "Base",
            QT_TRANSLATE_NOOP("App::Property", "An optional comment for this operation"),
        )
        obj.addProperty(
            "App::PropertyString",
            "CycleTime",
            "Base",
            QT_TRANSLATE_NOOP("App::Property", "Operations cycle time estimation"),
        )
        obj.addProperty(
            "App::PropertyLinkList",
            "Sources",
            "Base",
            QT_TRANSLATE_NOOP("App::Property", "Sources of the shapes"),
        )

        # Tool properties group
        obj.addProperty(
            "App::PropertyLink",
            "ToolController",
            "Tool",
            QT_TRANSLATE_NOOP(
                "App::Property", "The tool controller that will be used to calculate the path"
            ),
        )
        obj.addProperty(
            "App::PropertyEnumeration",
            "CoolantMode",
            "Tool",
            QT_TRANSLATE_NOOP("App::Property", "Coolant mode for this operation"),
        )

        # Feed properties group
        obj.addProperty(
            "App::PropertySpeed",
            "HorizFeed",
            "Feed",
            QT_TRANSLATE_NOOP("App::Property", "Normal move feed rate"),
        )
        obj.addProperty(
            "App::PropertySpeed",
            "VertFeed",
            "Feed",
            QT_TRANSLATE_NOOP("App::Property", "Vertical only (step down) move feed rate"),
        )

        # Start point properties group
        obj.addProperty(
            "App::PropertyVectorDistance",
            "StartPoint",
            "StartPoint",
            QT_TRANSLATE_NOOP("App::Property", "Feed start position"),
        )
        obj.addProperty(
            "App::PropertyBool",
            "EnableStartPoint",
            "StartPoint",
            QT_TRANSLATE_NOOP("App::Property", "Enable feed start position"),
        )

        # Curves properties group
        obj.addProperty(
            "App::PropertyBool",
            "AbsoluteArcCenter",
            "Curves",
            QT_TRANSLATE_NOOP("App::Property", "Use absolute arc center mode (G90.1)"),
        )
        obj.addProperty(
            "App::PropertyEnumeration",
            "ArcPlane",
            "Curves",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Arc drawing plane, corresponding to G17, G18 and G19.\n"
                "\nIf not 'None', the output wires will be transformed to align with the selected plane,\n"
                "and the corresponding GCode will be inserted.\n"
                "\n'Auto' means the plane is determined by the first encountered arc plane.\n"
                "If the found plane does not align to any GCode plane, XY plane is used.\n"
                "\n'Variable' means the arc plane can be changed during operation\n"
                "to align to the arc encountered.",
            ),
        )
        obj.addProperty(
            "App::PropertyLength",
            "Segmentation",
            "Curves",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Break long curves into segments of this length.\n"
                "One use case is for PCB autolevel,\n"
                "so that more correction points can be inserted.",
            ),
        )
        obj.addProperty(
            "App::PropertyFloat",
            "Deflection",
            "Curves",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Deflection for non circular curve discretization.\n"
                "\nIt also also used for discretizing circular wires,\n"
                "when you 'Explode' the shape for wire operations",
            ),
        )
        obj.addProperty(
            "App::PropertyLength",
            "MinDistance",
            "Curves",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Minimum distance for the generated new wires.\n"
                "Wires maybe broken if the algorithm see fits.\n"
                "\nSet zero to disable wire breaking.",
            ),
        )

        # Path properties group
        obj.addProperty(
            "App::PropertyEnumeration",
            "Orientation",
            "Path",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Enforce loop orientation.\n"
                "\n'Normal' means CCW for outer wires when looking against\n"
                "the positive axis direction and CW for inner wires.\n"
                "\n'Reversed' means the other way round.",
            ),
        )
        obj.addProperty(
            "App::PropertyEnumeration",
            "Direction",
            "Path",
            QT_TRANSLATE_NOOP("App::Property", "Enforce open path direction"),
        )
        obj.addProperty(
            "App::PropertyBool",
            "DualDirection",
            "Path",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Invert direction on each step down.\n"
                "Only if 'HandleMultipleFeatures' is 'Individually'.",
            ),
        )
        obj.addProperty(
            "App::PropertyLength",
            "RetractThreshold",
            "Path",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "If two wire's end points are separated within this threshold,\n"
                "they are consider as connected.\n"
                "You may want to set this to the tool diameter to keep the tool down.",
            ),
        )
        obj.addProperty(
            "App::PropertyBool",
            "SafetyFinish",
            "Path",
            QT_TRANSLATE_NOOP("App::Property", "Add move to Clearanc Height Out in the end"),
        )
        obj.addProperty(
            "App::PropertyEnumeration",
            "RetractAxis",
            "Path",
            QT_TRANSLATE_NOOP("App::Property", "Tool retraction axis"),
        )
        obj.addProperty(
            "App::PropertyEnumeration",
            "HandleMultipleFeatures",
            "Path",
            QT_TRANSLATE_NOOP(
                "App::Property", "Choose how to process multiple Base Geometry features"
            ),
        )
        obj.addProperty(
            "App::PropertyString",
            "PathParams",
            "Path",
            QT_TRANSLATE_NOOP("App::Property", "Parameters of Path.fromShapes(**params)"),
        )

        # Sorting properties group
        obj.addProperty(
            "App::PropertyEnumeration",
            "SortMode",
            "Sorting",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Wire sorting mode to optimize travel distance.\n"
                "\n'2D5' explode shapes into wires, and groups the shapes by its plane.\n"
                "The 'start' position chooses the first plane to start.\n"
                "The algorithm will then sort within the plane and then move on to the next nearest plane.\n"
                "\n'3D' makes no assumption of planarity. The sorting is done across 3D space.\n"
                "\n'Greedy' like '2D5' but will try to minimize travel by searching for nearest path below the current milling layer.\n"
                "The path in lower layer is only selected if the moving distance is within the value given in 'threshold'.",
            ),
        )
        obj.addProperty(
            "App::PropertyLength",
            "SortAbscissa",
            "Sorting",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Controls vertex sampling on wire for nearest point searching.\n"
                "The sampling is dong using OCC GCPnts_UniformAbscissa.",
            ),
        )
        obj.addProperty(
            "App::PropertyInteger",
            "NearestK",
            "Sorting",
            QT_TRANSLATE_NOOP(
                "App::Property", "Nearest k sampling vertices are considered during sorting"
            ),
        )

        # Gcode properties group
        obj.addProperty(
            "App::PropertyBool",
            "Verbose",
            "Gcode",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "If true, each motion GCode will contain full coordinate and feedrate",
            ),
        )

        obj.addProperty(
            "App::PropertyBool",
            "EmitPreamble",
            "Gcode",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Emit preambles G90.1 G17 G18 G19\n"
                "\nNote that emitting preambles between moves breaks some dressups\n"
                "and prevents path optimization on some controllers",
            ),
        )

        # Retract and Depth properties group
        obj.addProperty(
            "App::PropertyBool",
            "EnableStepDepth",
            "Depth",
            QT_TRANSLATE_NOOP("App::Property", "Apply incremental change step depth"),
        )
        obj.addProperty(
            "App::PropertyDistance",
            "StartDepth",
            "Depth",
            QT_TRANSLATE_NOOP(
                "App::Property", "Start depth with step depth.\nUsing with 'EnableStepDepth'."
            ),
        )
        obj.addProperty(
            "App::PropertyLength",
            "StepDepth",
            "Depth",
            QT_TRANSLATE_NOOP(
                "App::Property", "Max depth of each step depth.\nUsing only with 'EnableStepDepth'."
            ),
        )
        obj.addProperty(
            "App::PropertyLength",
            "RapidOffset",
            "Depth",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Offset for rapid move for path which already processed at previous step depth",
            ),
        )
        obj.addProperty(
            "App::PropertyDistance",
            "ClearanceHeight",
            "Depth",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Retraction\n\nTool retraction absolute coordinate along retraction axis",
            ),
        )
        obj.addProperty(
            "App::PropertyDistance",
            "ClearanceHeightOut",
            "Depth",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Clearance height in the end",
            ),
        )
        obj.addProperty(
            "App::PropertyDistance",
            "SafeHeight",
            "Depth",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Resume Height\n"
                "\nWhen return from last retraction,\n"
                "this gives the pause of the next move",
            ),
        )

        # Offset properties group
        obj.addProperty(
            "App::PropertyBool",
            "EnableOffset",
            "Offset",
            QT_TRANSLATE_NOOP("App::Property", "Apply offset to shape"),
        )
        obj.addProperty(
            "App::PropertyEnumeration",
            "OffsetType",
            "Offset",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "The output wires will be transformed by offset function."
                "\n'makeOffset2D' use Part.Wire.makeOffset2D() directly"
                "\n'offsetWire' use Path.Op.Util.offsetWire()",
            ),
        )
        obj.addProperty(
            "App::PropertyBool",
            "UseComp",
            "Offset",
            QT_TRANSLATE_NOOP("App::Property", "Use tool radius compensation"),
        )
        obj.addProperty(
            "App::PropertyBool",
            "OffsetInvertSide",
            "Offset",
            QT_TRANSLATE_NOOP("App::Property", "Invert offset drection"),
        )
        obj.addProperty(
            "App::PropertyDistance",
            "OffsetExtra",
            "Offset",
            QT_TRANSLATE_NOOP("App::Property", "Offset from shape"),
        )
        obj.addProperty(
            "App::PropertyEnumeration",
            "OffsetJoin",
            "Offset",
            QT_TRANSLATE_NOOP("App::Property", "Method of offsetting non-tangent joints"),
        )
        obj.addProperty(
            "App::PropertyBool",
            "OffsetOpenResult",
            "Offset",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Affects the way open wires are processed.\n"
                "If False, an open wire is made.\n"
                "If True, a closed wire is made from a double-sided offset,\n"
                "with rounds around open vertices.",
            ),
        )

        self.setDefaultValues(obj)
        self.setEditorMode(obj)
        self.addToolController()
        self.setSafetyZ()

    def setDefaultValues(self, obj):
        obj.Active = True
        obj.ArcPlane = ("None", "Auto", "XY", "ZX", "YZ", "Variable")
        obj.ArcPlane = "Auto"
        obj.CoolantMode = ("None", "Flood", "Mist")
        obj.CoolantMode = "None"
        obj.Deflection = 0.05
        obj.Direction = (
            "None",
            "XPositive",
            "XNegative",
            "YPositive",
            "YNegative",
            "ZPositive",
            "ZNegative",
        )
        obj.Direction = "None"
        obj.HandleMultipleFeatures = ("Collectively", "Individually")
        obj.HandleMultipleFeatures = "Individually"
        obj.NearestK = 3
        obj.OffsetJoin = ("arcs", "tangent", "intersection")
        obj.OffsetOpenResult = True
        obj.OffsetType = ("makeOffset2D", "offsetWire")
        obj.OffsetType = "offsetWire"
        obj.Orientation = ("Normal", "Reversed")
        obj.Orientation = "Normal"
        obj.RetractAxis = ("X", "Y", "Z")
        obj.RetractAxis = "Z"
        obj.SafetyFinish = True
        obj.SortAbscissa = 3.0
        obj.SortMode = ("None", "2D5", "3D", "Greedy")
        obj.SortMode = "2D5"
        obj.Verbose = True
        obj.UseComp = True

    def setEditorMode(self, obj):
        obj.setEditorMode("CycleTime", 1)  # read-only
        # obj.setEditorMode("ToolController", 2)  # hidden
        obj.setEditorMode("PathParams", 2)  # hidden

        startPointMode = 0 if obj.EnableStartPoint else 2
        offsetMode = 0 if obj.EnableOffset else 2
        offsetMode2 = 0 if obj.EnableOffset and obj.OffsetType == "makeOffset2D" else 2
        stepDepthMode = 0 if obj.EnableStepDepth else 2
        dualDirectionMode = 0 if obj.HandleMultipleFeatures == "Individually" else 2

        obj.setEditorMode("StartPoint", startPointMode)
        obj.setEditorMode("UseComp", offsetMode)
        obj.setEditorMode("OffsetExtra", offsetMode)
        obj.setEditorMode("OffsetInvertSide", offsetMode)
        obj.setEditorMode("OffsetType", offsetMode)
        obj.setEditorMode("OffsetJoin", offsetMode2)
        obj.setEditorMode("OffsetOpenResult", offsetMode2)
        obj.setEditorMode("StartDepth", stepDepthMode)
        obj.setEditorMode("StepDepth", stepDepthMode)
        obj.setEditorMode("RapidOffset", stepDepthMode)
        obj.setEditorMode("DualDirection", dualDirectionMode)

    def dumps(self):
        return

    def loads(self, state):
        return

    def onChanged(self, obj, prop):
        if prop in (
            "EnableOffset",
            "EnableStartPoint",
            "OffsetType",
            "EnableStepDepth",
            "HandleMultipleFeatures",
        ):
            self.setEditorMode(obj)
        if prop == "Path":
            obj.CycleTime = OpBase.getCycleTimeEstimate(obj)

        if prop == "Active" and obj.ViewObject:
            obj.ViewObject.signalChangeIcon()

    def onDocumentRestored(self, obj):
        self.setEditorMode(obj)

    def execute(self, obj):
        if not obj.Active:
            obj.Path = Path.Path()
            return

        edges = [e for s in obj.Sources for e in s.Shape.Edges]
        wires = [Part.Wire(se) for se in Part.sortEdges(edges)]

        offsetVal = 0
        if obj.EnableOffset:
            offsetVal = obj.OffsetExtra.Value
            if obj.UseComp:
                offsetVal += obj.ToolController.Tool.Diameter.Value / 2
            if obj.OffsetInvertSide:
                offsetVal = -offsetVal

        if offsetVal:
            join = obj.getEnumerationsOfProperty("OffsetJoin").index(obj.OffsetJoin)
            openResult = obj.OffsetOpenResult
            job = PathUtils.findParentJob(obj)
            base = job.Model.Group[0].Shape
            offsets = []
            for wire in wires:
                if obj.OffsetType == "makeOffset2D":
                    offset = wire.makeOffset2D(offsetVal, join=join, openResult=openResult)
                else:
                    offset = PathOpUtil.offsetWire(wire, base, offsetVal, forward=True)
                offsets.append(offset)

            wires = offsets

        if obj.HandleMultipleFeatures == "Collectively" and len(wires) > 1:
            shapes = [Part.makeCompound(wires)]
        else:
            shapes = wires

        params = {}
        if obj.EnableStartPoint:
            params["start"] = obj.StartPoint
        params["return_end"] = False
        params["arc_plane"] = obj.getEnumerationsOfProperty("ArcPlane").index(obj.ArcPlane)
        params["sort_mode"] = obj.getEnumerationsOfProperty("SortMode").index(obj.SortMode)
        params["min_dist"] = obj.MinDistance
        params["abscissa"] = obj.SortAbscissa
        params["nearest_k"] = obj.NearestK
        params["orientation"] = obj.getEnumerationsOfProperty("Orientation").index(obj.Orientation)
        params["direction"] = obj.getEnumerationsOfProperty("Direction").index(obj.Direction)
        params["threshold"] = obj.RetractThreshold
        params["retract_axis"] = obj.getEnumerationsOfProperty("RetractAxis").index(obj.RetractAxis)
        params["retraction"] = obj.ClearanceHeight
        params["resume_height"] = obj.SafeHeight
        params["segmentation"] = obj.Segmentation
        params["feedrate"] = obj.HorizFeed.Value
        params["feedrate_v"] = obj.VertFeed.Value
        params["verbose"] = obj.Verbose
        params["abs_center"] = obj.AbsoluteArcCenter
        params["preamble"] = obj.EmitPreamble
        params["deflection"] = obj.Deflection

        obj.PathParams = str({key: value for key, value in params.items()})

        commands = []
        for shape in shapes:
            params["shapes"] = shape

            path = Path.fromShapes(**params)
            pathReversed = Path.Path()
            if obj.EnableStepDepth:
                if obj.DualDirection and obj.HandleMultipleFeatures == "Individually":
                    # get path with inverted direction
                    for dir in (1, 2, 3, 4, 5, 6):
                        params["direction"] = dir
                        pathReversed = Path.fromShapes(**params)
                        if pathReversed.Length != path.Length:
                            # if length of the path is different, the path is inverted
                            break
                commands.extend(self.processStepDepth(obj, path, pathReversed))
            else:
                commands.extend(path.Commands)

        if obj.SafetyFinish:
            commands.append(Path.Command("G00", {obj.RetractAxis: obj.ClearanceHeightOut.Value}))

        obj.Path = Path.Path(commands)

    """
fromShapes(shapes, start=Vector(), return_end=False arc_plane=1,
sort_mode=1, min_dist=0.0, abscissa=3.0, nearest_k=3, orientation=0,
direction=0, threshold=0.0, retract_axis=2, retraction=0.0,
resume_height=0.0, segmentation=0.0, feedrate=0.0, feedrate_v=0.0,
verbose=true, abs_center=false, preamble=true, deflection=0.01)

Returns a Path object from a list of shapes

* shapes: input list of shapes.
* start (Vector()): feed start position, and also serves as a hint of path entry.
* return_end (False): if True, returns tuple (path, endPosition).
* arc_plane(1): 0=None,1=Auto,2=XY,3=ZX,4=YZ,5=Variable.
    Arc drawing plane, corresponding to G17, G18, and G19.
    If not 'None', the output wires will be transformed to align with the selected plane,
    and the corresponding GCode will be inserted.
    'Auto' means the plane is determined by the first encountered arc plane.
    If the found plane does not align to any GCode plane, XY plane is used.
    'Variable' means the arc plane can be changed during operation to align to the arc encountered.
* sort_mode(1): 0=None, 1=2D5, 2=3D, 3=Greedy.
    Wire sorting mode to optimize travel distance.
    '2D5' explode shapes into wires, and groups the shapes by its plane.
    The 'start' position chooses the first plane to start.
    The algorithm will then sort within the plane and then move on to the next nearest plane.
    '3D' makes no assumption of planarity. The sorting is done across 3D space.
    'Greedy' like2D5 but will try to minimize travel by searching for nearest path below the current milling layer.
    The path in lower layer is only selected if the moving distance is within the value given inthreshold.
* min_dist(0.0): minimum distance for the generated new wires.
    Wires maybe broken if the algorithm see fits. Set to zero to disable wire breaking.
* abscissa(3.0): Controls vertex sampling on wire for nearest point searching.
    The sampling is dong using OCC GCPnts_UniformAbscissa
* nearest_k(3): Nearest k sampling vertices are considered during sorting
* orientation(0): 0=Normal,1=Reversed. Enforce loop orientation
    'Normal' means CCW for outer wires when looking against the positive axis direction,
    and CW for inner wires.
    'Reversed' means the other way round
* direction(0):
    0=None,1=XPositive,2=XNegative,3=YPositive,4=YNegative,5=ZPositive,6=ZNegative.
    Enforce open path direction
* threshold(0.0): If two wire's end points are separated within this threshold,
    they are consider as connected.
    You may want to set this to the tool diameter to keep the tool down.
* retract_axis(2): 0=X, 1=Y, 2=Z. Tool retraction axis
* retraction(0.0): Tool retraction absolute coordinate along retraction axis
* resume_height(0.0): When return from last retraction, this gives the pause
    height relative to the Z value of the next move.
* segmentation(0.0): Break long curves into segments of this length.
    One use case is for PCB autolevel, so that more correction points can be inserted
* feedrate(0.0): Normal move feed rate
* feedrate_v(0.0): Vertical only (step down) move feed rate
* verbose(true): If true, each motion GCode will contain full coordinate and feedrate
* abs_center(false): Use absolute arc center mode (G90.1)
* preamble(true): Emit preambles
* deflection(0.01): Deflection for non circular curve discretization.
    It also also used for discretizing circular wires when you Explode the shape for wire operations"""

    # Get coordinates of each axis from path commands
    def getPoint(self, commands):
        x = y = z = None
        for cmd in commands:
            x = cmd.x if x is None and cmd.x is not None else x
            y = cmd.y if y is None and cmd.y is not None else y
            z = cmd.z if z is None and cmd.z is not None else z
            if x is not None and y is not None and z is not None:
                return FreeCAD.Vector(x, y, z)

        if x is None:
            Path.Log.warning(translate("PathShape", "Can not determinate X"))
        if y is None:
            Path.Log.warning(translate("PathShape", "Can not determinate Y"))
        if z is None:
            Path.Log.warning(translate("PathShape", "Can not determinate Z"))

        return None

    def isClosedPath(self, point1, point2):
        if point1 is None or point2 is None:
            return False
        if not Path.Geom.pointsCoincide(point1, point2):
            return False

        return True

    def isDeeper(self, axis, cmd, limitDepth):
        depth = cmd.__getattribute__(axis)
        if depth is None or limitDepth is None:
            return False
        if Path.Geom.isRoughly(depth, limitDepth):
            return False
        if (not self.invertAxis and depth > limitDepth) or (self.invertAxis and depth < limitDepth):
            return False

        return True

    def getStepDepthRepeats(self, axis, path, startDepth, stepDepth):
        minDepth = None
        for cmd in path.Commands:
            depth = cmd.__getattribute__(axis)
            if depth is not None:
                if minDepth is None:
                    minDepth = depth
                elif (not self.invertAxis and depth < minDepth) or (
                    self.invertAxis and depth > minDepth
                ):
                    minDepth = depth

        if minDepth is not None:
            repeats = abs(math.ceil((startDepth - minDepth) / stepDepth))
            return repeats

        return 1

    # Add several passes with step down
    def processStepDepth(self, obj, path, pathReversed):
        commands = []
        startPoint = self.getPoint(path.Commands)
        endPoint = self.getPoint(reversed(path.Commands))
        rAxis = obj.RetractAxis
        # slaveAxis = "Y" if rAxis == "X" else "X"

        if obj.ClearanceHeight > obj.SafeHeight:
            self.invertAxis = False
            stepDepth = obj.StepDepth.Value
            rapidOffset = obj.RapidOffset.Value
        else:
            self.invertAxis = True
            stepDepth = -obj.StepDepth.Value
            rapidOffset = -obj.RapidOffset.Value

        startDepth = obj.StartDepth.Value
        limitDepth = startDepth
        iter = 0
        changeDir = False

        stepDepthRepeats = self.getStepDepthRepeats(rAxis, path, startDepth, stepDepth)
        print("stepDepthRepeats", stepDepthRepeats)

        while iter < stepDepthRepeats:
            iter += 1
            lastStep = True if iter == stepDepthRepeats else False
            limitDepth -= stepDepth

            skipRetract = (iter > 1) and (
                self.isClosedPath(startPoint, endPoint)
                or (obj.DualDirection and obj.HandleMultipleFeatures == "Individually")
            )
            print(
                f"  iter={iter}  stepDepth={stepDepth}  limitDepth={limitDepth}  skipRetract={skipRetract}"
            )

            if not skipRetract:
                # add safety moves to start point before next step down
                if rAxis == "X":
                    commands.append(Path.Command("G00", {"Z": startPoint.z}))
                    commands.append(Path.Command("G00", {"X": startPoint.x}))
                    commands.append(Path.Command("G00", {"Y": startPoint.y}))
                elif rAxis == "Y":
                    commands.append(Path.Command("G00", {"Z": startPoint.z}))
                    commands.append(Path.Command("G00", {"Y": startPoint.y}))
                    commands.append(Path.Command("G00", {"X": startPoint.x}))
                elif rAxis == "Z":
                    commands.append(Path.Command("G00", {"Z": startPoint.z}))
                    commands.append(Path.Command("G00", {"X": startPoint.x, "Y": startPoint.y}))

            currentPath = pathReversed if changeDir else path

            for cmd in currentPath.Commands:
                if (
                    skipRetract
                    and cmd.x is None
                    and cmd.y is None
                    and (cmd.z == obj.ClearanceHeight or cmd.z == obj.SafeHeight)
                ):
                    # skip start move for closed profile
                    continue

                if cmd.__getattribute__(rAxis) is not None:
                    if self.isDeeper(rAxis, cmd, limitDepth):
                        cmd.__setattr__(rAxis, limitDepth)
                    elif (
                        cmd.Name not in Path.Geom.CmdMoveRapid
                        and iter > 1
                        and rapidOffset
                        and not lastStep
                        and not self.isDeeper(rAxis, cmd, limitDepth + stepDepth + rapidOffset)
                    ):
                        rapidFeed = obj.ToolController.HorizRapid.Value
                        cmd.F = rapidFeed
                        cmd.__setattr__(rAxis, cmd.Parameters[rAxis] + rapidOffset)

                commands.append(cmd)

            if obj.DualDirection and obj.HandleMultipleFeatures == "Individually":
                # change direction after on each step down
                changeDir = not changeDir

        return commands

    # This method must return True and needed for PathUtils.findToolController()
    def isToolSupported(self, obj, tool):
        return True

    def addToolController(self):
        job = self.job
        obj = self.obj
        for op in job.Operations.Group[-2::-1]:
            toolController = PathUtil.toolControllerForOp(op)
            if toolController:
                break
        else:
            toolController = PathUtils.findToolController(obj, self)

        if toolController:
            obj.ToolController = toolController
            obj.HorizFeed = obj.ToolController.HorizFeed.Value
            obj.VertFeed = obj.ToolController.VertFeed.Value
            obj.StepDepth = obj.ToolController.Tool.Diameter / 2
        else:
            Path.Log.warning(
                translate("PathShape", "Tool controller not selected for operation %s") % obj.Label
            )

    # Set safety depth parameters
    def setSafetyZ(self):
        job = self.job
        obj = self.obj
        if job:
            bbStock = job.Stock.Shape.BoundBox
            zmax = bbStock.ZMax
            obj.ClearanceHeight = zmax + 30
            obj.setExpression("ClearanceHeightOut", "ClearanceHeight")
            obj.SafeHeight = zmax + 10
            obj.StartDepth = zmax + 1


# Geometry for selected shapes
class ObjectPartShape:
    def __init__(self, obj, base):
        self.Type = "PartShapeObject"
        self.obj = obj
        obj.Proxy = self
        obj.addProperty(
            "App::PropertyLinkSubListGlobal",
            "Base",
            "Base",
            QT_TRANSLATE_NOOP("App::Property", "The base geometry for this operation"),
        )
        obj.addProperty(
            "App::PropertyBool",
            "FilterEdges",
            "Base",
            QT_TRANSLATE_NOOP("App::Property", "Exclude similar edges"),
        )
        obj.Base = base

    def onDelete(self, obj, args):
        return True

    def dumps(self):
        return

    def loads(self, state):
        return

    def onDocumentRestored(self, obj):
        self.obj = obj

    def onChanged(self, obj, prop):
        pass

    def execute(self, obj):
        edges = []
        for base in obj.Base:
            (baseObj, subNames) = base
            shape = baseObj.Shape
            if not subNames or subNames == ("",):
                edges.extend(shape.Edges)
            else:
                edges.extend([shape.getElement(n) for n in subNames if n.startswith("Edge")])

        if getattr(obj, "FilterEdges", False):
            uEdges = []
            for edge in edges:
                if any(Path.Geom.edgesSimilar(eu, edge) for eu in uEdges):
                    continue
                uEdges.append(edge)
            edges = uEdges

        obj.Shape = Part.makeCompound(edges)


class ViewProviderPathShape:
    def __init__(self, vobj):
        self.Object = vobj.Object
        vobj.Proxy = self

    def attach(self, vobj):
        self.Object = vobj.Object
        return

    def dumps(self):
        return

    def loads(self, state):
        return

    def claimChildren(self):
        return [base for base in self.Object.Sources]

    def onDelete(self, vobj, args):
        for shape in self.Object.Sources:
            if "PartShape" in shape.Name:
                # do not remove external link objects
                shape.Document.removeObject(shape.Name)
        self.Object.Document.removeObject(self.Object.Name)

    def getIcon(self):
        if self.Object.Active:
            return ":/icons/CAM_ShapeTC.svg"
        else:
            return ":/icons/CAM_OpActive.svg"


class CommandPathShapeTC:
    def GetResources(self):
        return {
            "Pixmap": "CAM_ShapeTC",
            "MenuText": QT_TRANSLATE_NOOP("CAM_PathShapeTC", "Path from Shape TC"),
            "ToolTip": QT_TRANSLATE_NOOP(
                "CAM_PathShapeTC", "Creates path from selected shapes with tool controller"
            ),
        }

    def IsActive(self):
        isJob = False
        if FreeCAD.ActiveDocument is not None:
            for o in FreeCAD.ActiveDocument.Objects:
                if o.Name.startswith("Job"):
                    isJob = True
                    break
        if isJob:
            selection = FreeCADGui.Selection.getSelectionEx()
            if selection:
                baseObj = selection[0].Object
                subNames = selection[0].SubElementNames
                if subNames and [edge for edge in subNames if "Edge" in edge]:
                    return True
                elif (
                    hasattr(baseObj, "Shape")
                    and hasattr(baseObj.Shape, "Edges")
                    and baseObj.Shape.Edges
                ):
                    return True
        return False

    def Activated(self):
        doc = FreeCAD.ActiveDocument
        doc.openTransaction("Create PathShape")
        selection = FreeCADGui.Selection.getSelectionEx()
        base = []
        for sel in selection:
            baseObj = sel.Object
            subNames = sel.SubElementNames if sel.SubElementNames else ("",)
            base.append([baseObj, subNames])
        shapeObj = doc.addObject("Part::FeaturePython", "PartShape")
        shapeObj.ViewObject.Proxy = 0
        shapeObj.Visibility = False
        ObjectPartShape(shapeObj, base)

        pathObj = doc.addObject("Path::FeaturePython", "PathShape")
        job = PathUtils.addToJob(pathObj)
        ObjectPathShape(pathObj, job)
        ViewProviderPathShape(pathObj.ViewObject)
        pathObj.Sources = [shapeObj]
        FreeCAD.ActiveDocument.commitTransaction()
        doc.recompute()


if FreeCAD.GuiUp:
    # Register the FreeCAD command
    FreeCADGui.addCommand("CAM_PathShapeTC", CommandPathShapeTC())
