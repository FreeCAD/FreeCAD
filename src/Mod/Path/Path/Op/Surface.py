# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2016 sliptonic <shopinthewoods@gmail.com>               *
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


__title__ = "Path Surface Operation"
__author__ = "sliptonic (Brad Collette)"
__url__ = "https://www.freecad.org"
__doc__ = "Class and implementation of 3D Surface operation."
__contributors__ = "russ4262 (Russell Johnson)"

import FreeCAD

translate = FreeCAD.Qt.translate

# OCL must be installed
try:
    try:
        import ocl
    except ImportError:
        import opencamlib as ocl
except ImportError:
    msg = translate(
        "PathSurface", "This operation requires OpenCamLib to be installed."
    )
    FreeCAD.Console.PrintError(msg + "\n")
    raise ImportError
    # import sys
    # sys.exit(msg)

from PySide.QtCore import QT_TRANSLATE_NOOP
import Path
import Path.Op.Base as PathOp
import Path.Op.SurfaceSupport as PathSurfaceSupport
import PathScripts.PathUtils as PathUtils
import math
import time

# lazily loaded modules
from lazy_loader.lazy_loader import LazyLoader

Part = LazyLoader("Part", globals(), "Part")

if FreeCAD.GuiUp:
    import FreeCADGui


if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


class ObjectSurface(PathOp.ObjectOp):
    """Proxy object for Surfacing operation."""

    def opFeatures(self, obj):
        """opFeatures(obj) ... return all standard features"""
        return (
            PathOp.FeatureTool
            | PathOp.FeatureDepths
            | PathOp.FeatureHeights
            | PathOp.FeatureStepDown
            | PathOp.FeatureCoolant
            | PathOp.FeatureBaseFaces
        )

    def initOperation(self, obj):
        """initOperation(obj) ... Initialize the operation by
        managing property creation and property editor status."""
        self.propertiesReady = False

        self.initOpProperties(obj)  # Initialize operation-specific properties

        # For debugging
        if Path.Log.getLevel(Path.Log.thisModule()) != 4:
            obj.setEditorMode("ShowTempObjects", 2)  # hide

        if not hasattr(obj, "DoNotSetDefaultValues"):
            self.setEditorProperties(obj)

    def initOpProperties(self, obj, warn=False):
        """initOpProperties(obj) ... create operation specific properties"""
        self.addNewProps = []

        for (prtyp, nm, grp, tt) in self.opPropertyDefinitions():
            if not hasattr(obj, nm):
                obj.addProperty(prtyp, nm, grp, tt)
                self.addNewProps.append(nm)

        # Set enumeration lists for enumeration properties
        for n in self.propertyEnumerations():
            setattr(obj, n[0], n[1])

        self.propertiesReady = True

    def opPropertyDefinitions(self):
        """opPropertyDefinitions(obj) ... Store operation specific properties"""

        return [
            (
                "App::PropertyBool",
                "ShowTempObjects",
                "Debug",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Show the temporary path construction objects when module is in DEBUG mode.",
                ),
            ),
            (
                "App::PropertyDistance",
                "AngularDeflection",
                "Mesh Conversion",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Smaller values yield a finer, more accurate mesh. Smaller values increase processing time a lot.",
                ),
            ),
            (
                "App::PropertyDistance",
                "LinearDeflection",
                "Mesh Conversion",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Smaller values yield a finer, more accurate mesh. Smaller values do not increase processing time much.",
                ),
            ),
            (
                "App::PropertyFloat",
                "CutterTilt",
                "Rotation",
                QT_TRANSLATE_NOOP(
                    "App::Property", "Stop index(angle) for rotational scan"
                ),
            ),
            (
                "App::PropertyEnumeration",
                "DropCutterDir",
                "Rotation",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Dropcutter lines are created parallel to this axis.",
                ),
            ),
            (
                "App::PropertyVectorDistance",
                "DropCutterExtraOffset",
                "Rotation",
                QT_TRANSLATE_NOOP(
                    "App::Property", "Additional offset to the selected bounding box"
                ),
            ),
            (
                "App::PropertyEnumeration",
                "RotationAxis",
                "Rotation",
                QT_TRANSLATE_NOOP(
                    "App::Property", "The model will be rotated around this axis."
                ),
            ),
            (
                "App::PropertyFloat",
                "StartIndex",
                "Rotation",
                QT_TRANSLATE_NOOP(
                    "App::Property", "Start index(angle) for rotational scan"
                ),
            ),
            (
                "App::PropertyFloat",
                "StopIndex",
                "Rotation",
                QT_TRANSLATE_NOOP(
                    "App::Property", "Stop index(angle) for rotational scan"
                ),
            ),
            (
                "App::PropertyEnumeration",
                "ScanType",
                "Surface",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Planar: Flat, 3D surface scan.  Rotational: 4th-axis rotational scan.",
                ),
            ),
            (
                "App::PropertyInteger",
                "AvoidLastX_Faces",
                "Selected Geometry Settings",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Avoid cutting the last 'N' faces in the Base Geometry list of selected faces.",
                ),
            ),
            (
                "App::PropertyBool",
                "AvoidLastX_InternalFeatures",
                "Selected Geometry Settings",
                QT_TRANSLATE_NOOP(
                    "App::Property", "Do not cut internal features on avoided faces."
                ),
            ),
            (
                "App::PropertyDistance",
                "BoundaryAdjustment",
                "Selected Geometry Settings",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Positive values push the cutter toward, or beyond, the boundary. Negative values retract the cutter away from the boundary.",
                ),
            ),
            (
                "App::PropertyBool",
                "BoundaryEnforcement",
                "Selected Geometry Settings",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "If true, the cutter will remain inside the boundaries of the model or selected face(s).",
                ),
            ),
            (
                "App::PropertyEnumeration",
                "HandleMultipleFeatures",
                "Selected Geometry Settings",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Choose how to process multiple Base Geometry features.",
                ),
            ),
            (
                "App::PropertyDistance",
                "InternalFeaturesAdjustment",
                "Selected Geometry Settings",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Positive values push the cutter toward, or into, the feature. Negative values retract the cutter away from the feature.",
                ),
            ),
            (
                "App::PropertyBool",
                "InternalFeaturesCut",
                "Selected Geometry Settings",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Cut internal feature areas within a larger selected face.",
                ),
            ),
            (
                "App::PropertyEnumeration",
                "BoundBox",
                "Clearing Options",
                QT_TRANSLATE_NOOP(
                    "App::Property", "Select the overall boundary for the operation."
                ),
            ),
            (
                "App::PropertyEnumeration",
                "CutMode",
                "Clearing Options",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Set the direction for the cutting tool to engage the material: Climb (ClockWise) or Conventional (CounterClockWise)",
                ),
            ),
            (
                "App::PropertyEnumeration",
                "CutPattern",
                "Clearing Options",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Set the geometric clearing pattern to use for the operation.",
                ),
            ),
            (
                "App::PropertyFloat",
                "CutPatternAngle",
                "Clearing Options",
                QT_TRANSLATE_NOOP(
                    "App::Property", "The yaw angle used for certain clearing patterns"
                ),
            ),
            (
                "App::PropertyBool",
                "CutPatternReversed",
                "Clearing Options",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Reverse the cut order of the stepover paths. For circular cut patterns, begin at the outside and work toward the center.",
                ),
            ),
            (
                "App::PropertyDistance",
                "DepthOffset",
                "Clearing Options",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Set the Z-axis depth offset from the target surface.",
                ),
            ),
            (
                "App::PropertyEnumeration",
                "LayerMode",
                "Clearing Options",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Complete the operation in a single pass at depth, or multiple passes to final depth.",
                ),
            ),
            (
                "App::PropertyVectorDistance",
                "PatternCenterCustom",
                "Clearing Options",
                QT_TRANSLATE_NOOP(
                    "App::Property", "Set the start point for the cut pattern."
                ),
            ),
            (
                "App::PropertyEnumeration",
                "PatternCenterAt",
                "Clearing Options",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Choose location of the center point for starting the cut pattern.",
                ),
            ),
            (
                "App::PropertyEnumeration",
                "ProfileEdges",
                "Clearing Options",
                QT_TRANSLATE_NOOP(
                    "App::Property", "Profile the edges of the selection."
                ),
            ),
            (
                "App::PropertyDistance",
                "SampleInterval",
                "Clearing Options",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Set the sampling resolution. Smaller values quickly increase processing time.",
                ),
            ),
            (
                "App::PropertyFloat",
                "StepOver",
                "Clearing Options",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Set the stepover percentage, based on the tool's diameter.",
                ),
            ),
            (
                "App::PropertyBool",
                "OptimizeLinearPaths",
                "Optimization",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Enable optimization of linear paths (co-linear points). Removes unnecessary co-linear points from G-code output.",
                ),
            ),
            (
                "App::PropertyBool",
                "OptimizeStepOverTransitions",
                "Optimization",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Enable separate optimization of transitions between, and breaks within, each step over path.",
                ),
            ),
            (
                "App::PropertyBool",
                "CircularUseG2G3",
                "Optimization",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Convert co-planar arcs to G2/G3 G-code commands for `Circular` and `CircularZigZag` cut patterns.",
                ),
            ),
            (
                "App::PropertyDistance",
                "GapThreshold",
                "Optimization",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Collinear and co-radial artifact gaps that are smaller than this threshold are closed in the path.",
                ),
            ),
            (
                "App::PropertyString",
                "GapSizes",
                "Optimization",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Feedback: three smallest gaps identified in the path geometry.",
                ),
            ),
            (
                "App::PropertyVectorDistance",
                "StartPoint",
                "Start Point",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "The custom start point for the path of this operation",
                ),
            ),
            (
                "App::PropertyBool",
                "UseStartPoint",
                "Start Point",
                QT_TRANSLATE_NOOP(
                    "App::Property", "Make True, if specifying a Start Point"
                ),
            ),
        ]

    @classmethod
    def propertyEnumerations(cls, dataType="data"):
        """propertyEnumerations(dataType="data")... return property enumeration lists of specified dataType.
        Args:
            dataType = 'data', 'raw', 'translated'
        Notes:
        'data' is list of internal string literals used in code
        'raw' is list of (translated_text, data_string) tuples
        'translated' is list of translated string literals
        """

        # Enumeration lists for App::PropertyEnumeration properties
        enums = {
            "BoundBox": [
                (translate("Path_Surface", "BaseBoundBox"), "BaseBoundBox"),
                (translate("Path_Surface", "Stock"), "Stock"),
            ],
            "PatternCenterAt": [
                (translate("Path_Surface", "CenterOfMass"), "CenterOfMass"),
                (translate("Path_Surface", "CenterOfBoundBox"), "CenterOfBoundBox"),
                (translate("Path_Surface", "XminYmin"), "XminYmin"),
                (translate("Path_Surface", "Custom"), "Custom"),
            ],
            "CutMode": [
                (translate("Path_Surface", "Conventional"), "Conventional"),
                (translate("Path_Surface", "Climb"), "Climb"),
            ],
            "CutPattern": [
                (translate("Path_Surface", "Circular"), "Circular"),
                (translate("Path_Surface", "CircularZigZag"), "CircularZigZag"),
                (translate("Path_Surface", "Line"), "Line"),
                (translate("Path_Surface", "Offset"), "Offset"),
                (translate("Path_Surface", "Spiral"), "Spiral"),
                (translate("Path_Surface", "ZigZag"), "ZigZag"),
            ],
            "DropCutterDir": [
                (translate("Path_Surface", "X"), "X"),
                (translate("Path_Surface", "Y"), "Y"),
            ],
            "HandleMultipleFeatures": [
                (translate("Path_Surface", "Collectively"), "Collectively"),
                (translate("Path_Surface", "Individually"), "Individually"),
            ],
            "LayerMode": [
                (translate("Path_Surface", "Single-pass"), "Single-pass"),
                (translate("Path_Surface", "Multi-pass"), "Multi-pass"),
            ],
            "ProfileEdges": [
                (translate("Path_Surface", "None"), "None"),
                (translate("Path_Surface", "Only"), "Only"),
                (translate("Path_Surface", "First"), "First"),
                (translate("Path_Surface", "Last"), "Last"),
            ],
            "RotationAxis": [
                (translate("Path_Surface", "X"), "X"),
                (translate("Path_Surface", "Y"), "Y"),
            ],
            "ScanType": [
                (translate("Path_Surface", "Planar"), "Planar"),
                (translate("Path_Surface", "Rotational"), "Rotational"),
            ],
        }

        if dataType == "raw":
            return enums

        data = []
        idx = 0 if dataType == "translated" else 1

        for k, v in enumerate(enums):
            data.append((v, [tup[idx] for tup in enums[v]]))

        return data

    def opPropertyDefaults(self, obj, job):
        """opPropertyDefaults(obj, job) ... returns a dictionary of default values
        for the operation's properties."""
        defaults = {
            "OptimizeLinearPaths": True,
            "InternalFeaturesCut": True,
            "OptimizeStepOverTransitions": False,
            "CircularUseG2G3": False,
            "BoundaryEnforcement": True,
            "UseStartPoint": False,
            "AvoidLastX_InternalFeatures": True,
            "CutPatternReversed": False,
            "StartPoint": FreeCAD.Vector(0.0, 0.0, obj.ClearanceHeight.Value),
            "ProfileEdges": "None",
            "LayerMode": "Single-pass",
            "ScanType": "Planar",
            "RotationAxis": "X",
            "CutMode": "Conventional",
            "CutPattern": "Line",
            "HandleMultipleFeatures": "Collectively",
            "PatternCenterAt": "CenterOfMass",
            "GapSizes": "No gaps identified.",
            "StepOver": 100.0,
            "CutPatternAngle": 0.0,
            "CutterTilt": 0.0,
            "StartIndex": 0.0,
            "StopIndex": 360.0,
            "SampleInterval": 1.0,
            "BoundaryAdjustment": 0.0,
            "InternalFeaturesAdjustment": 0.0,
            "AvoidLastX_Faces": 0,
            "PatternCenterCustom": FreeCAD.Vector(0.0, 0.0, 0.0),
            "GapThreshold": 0.005,
            "AngularDeflection": 0.25,  # AngularDeflection is unused
            # Reasonable compromise between speed & precision
            "LinearDeflection": 0.001,
            # For debugging
            "ShowTempObjects": False,
        }

        warn = True
        if hasattr(job, "GeometryTolerance"):
            if job.GeometryTolerance.Value != 0.0:
                warn = False
                # Tessellation precision dictates the offsets we need to add to
                # avoid false collisions with the model mesh, so make sure we
                # default to tessellating with greater precision than the target
                # GeometryTolerance.
                defaults["LinearDeflection"] = job.GeometryTolerance.Value / 4
        if warn:
            msg = translate("PathSurface", "The GeometryTolerance for this Job is 0.0.")
            msg += translate(
                "PathSurface", "Initializing LinearDeflection to 0.001 mm."
            )
            FreeCAD.Console.PrintWarning(msg + "\n")

        return defaults

    def setEditorProperties(self, obj):
        # Used to hide inputs in properties list

        P0 = R2 = 0  # 0 = show
        P2 = R0 = 2  # 2 = hide
        if obj.ScanType == "Planar":
            # if obj.CutPattern in ['Line', 'ZigZag']:
            if obj.CutPattern in ["Circular", "CircularZigZag", "Spiral"]:
                P0 = 2
                P2 = 0
            elif obj.CutPattern == "Offset":
                P0 = 2
        elif obj.ScanType == "Rotational":
            R2 = P0 = P2 = 2
            R0 = 0
        obj.setEditorMode("DropCutterDir", R0)
        obj.setEditorMode("DropCutterExtraOffset", R0)
        obj.setEditorMode("RotationAxis", R0)
        obj.setEditorMode("StartIndex", R0)
        obj.setEditorMode("StopIndex", R0)
        obj.setEditorMode("CutterTilt", R0)
        obj.setEditorMode("CutPattern", R2)
        obj.setEditorMode("CutPatternAngle", P0)
        obj.setEditorMode("PatternCenterAt", P2)
        obj.setEditorMode("PatternCenterCustom", P2)

    def onChanged(self, obj, prop):
        if hasattr(self, "propertiesReady"):
            if self.propertiesReady:
                if prop in ["ScanType", "CutPattern"]:
                    self.setEditorProperties(obj)

    def opOnDocumentRestored(self, obj):
        self.propertiesReady = False
        job = PathUtils.findParentJob(obj)

        self.initOpProperties(obj, warn=True)
        self.opApplyPropertyDefaults(obj, job, self.addNewProps)

        mode = 2 if Path.Log.getLevel(Path.Log.thisModule()) != 4 else 0
        obj.setEditorMode("ShowTempObjects", mode)

        # Repopulate enumerations in case of changes
        for prop, enums in ObjectSurface.propertyEnumerations():
            restore = False
            if hasattr(obj, prop):
                val = obj.getPropertyByName(prop)
                restore = True
            setattr(obj, prop, enums)
            if restore:
                setattr(obj, prop, val)

        self.setEditorProperties(obj)

    def opApplyPropertyDefaults(self, obj, job, propList):
        # Set standard property defaults
        PROP_DFLTS = self.opPropertyDefaults(obj, job)
        for n in PROP_DFLTS:
            if n in propList:
                prop = getattr(obj, n)
                val = PROP_DFLTS[n]
                setVal = False
                if hasattr(prop, "Value"):
                    if isinstance(val, int) or isinstance(val, float):
                        setVal = True
                if setVal:
                    setattr(prop, "Value", val)
                else:
                    setattr(obj, n, val)

    def opSetDefaultValues(self, obj, job):
        """opSetDefaultValues(obj, job) ... initialize defaults"""
        job = PathUtils.findParentJob(obj)

        self.opApplyPropertyDefaults(obj, job, self.addNewProps)

        # need to overwrite the default depth calculations for facing
        d = None
        if job:
            if job.Stock:
                d = PathUtils.guessDepths(job.Stock.Shape, None)
                Path.Log.debug("job.Stock exists")
            else:
                Path.Log.debug("job.Stock NOT exist")
        else:
            Path.Log.debug("job NOT exist")

        if d is not None:
            obj.OpFinalDepth.Value = d.final_depth
            obj.OpStartDepth.Value = d.start_depth
        else:
            obj.OpFinalDepth.Value = -10
            obj.OpStartDepth.Value = 10

        Path.Log.debug("Default OpFinalDepth: {}".format(obj.OpFinalDepth.Value))
        Path.Log.debug("Default OpStartDepth: {}".format(obj.OpStartDepth.Value))

    def opApplyPropertyLimits(self, obj):
        """opApplyPropertyLimits(obj) ... Apply necessary limits to user input property values before performing main operation."""
        # Limit start index
        if obj.StartIndex < 0.0:
            obj.StartIndex = 0.0
        if obj.StartIndex > 360.0:
            obj.StartIndex = 360.0

        # Limit stop index
        if obj.StopIndex > 360.0:
            obj.StopIndex = 360.0
        if obj.StopIndex < 0.0:
            obj.StopIndex = 0.0

        # Limit cutter tilt
        if obj.CutterTilt < -90.0:
            obj.CutterTilt = -90.0
        if obj.CutterTilt > 90.0:
            obj.CutterTilt = 90.0

        # Limit sample interval
        if obj.SampleInterval.Value < 0.0001:
            obj.SampleInterval.Value = 0.0001
            Path.Log.error("Sample interval limits are 0.001 to 25.4 millimeters.")

        if obj.SampleInterval.Value > 25.4:
            obj.SampleInterval.Value = 25.4
            Path.Log.error("Sample interval limits are 0.001 to 25.4 millimeters.")

        # Limit cut pattern angle
        if obj.CutPatternAngle < -360.0:
            obj.CutPatternAngle = 0.0
            Path.Log.error("Cut pattern angle limits are +-360 degrees.")

        if obj.CutPatternAngle >= 360.0:
            obj.CutPatternAngle = 0.0
            Path.Log.error("Cut pattern angle limits are +- 360 degrees.")

        # Limit StepOver to natural number percentage
        if obj.StepOver > 100.0:
            obj.StepOver = 100.0
        if obj.StepOver < 1.0:
            obj.StepOver = 1.0

        # Limit AvoidLastX_Faces to zero and positive values
        if obj.AvoidLastX_Faces < 0:
            obj.AvoidLastX_Faces = 0
            Path.Log.error("AvoidLastX_Faces: Only zero or positive values permitted.")

        if obj.AvoidLastX_Faces > 100:
            obj.AvoidLastX_Faces = 100
            Path.Log.error("AvoidLastX_Faces: Avoid last X faces count limited to 100.")

    def opUpdateDepths(self, obj):
        if hasattr(obj, "Base") and obj.Base:
            base, sublist = obj.Base[0]
            fbb = base.Shape.getElement(sublist[0]).BoundBox
            zmin = fbb.ZMax
            for base, sublist in obj.Base:
                for sub in sublist:
                    try:
                        fbb = base.Shape.getElement(sub).BoundBox
                        zmin = min(zmin, fbb.ZMin)
                    except Part.OCCError as e:
                        Path.Log.error(e)
            obj.OpFinalDepth = zmin
        elif self.job:
            if hasattr(obj, "BoundBox"):
                if obj.BoundBox == "BaseBoundBox":
                    models = self.job.Model.Group
                    zmin = models[0].Shape.BoundBox.ZMin
                    for M in models:
                        zmin = min(zmin, M.Shape.BoundBox.ZMin)
                    obj.OpFinalDepth = zmin
                if obj.BoundBox == "Stock":
                    models = self.job.Stock
                    obj.OpFinalDepth = self.job.Stock.Shape.BoundBox.ZMin

    def opExecute(self, obj):
        """opExecute(obj) ... process surface operation"""
        Path.Log.track()

        self.modelSTLs = []
        self.safeSTLs = []
        self.modelTypes = []
        self.boundBoxes = []
        self.profileShapes = []
        self.collectiveShapes = []
        self.individualShapes = []
        self.avoidShapes = []
        self.tempGroup = None
        self.CutClimb = False
        self.closedGap = False
        self.tmpCOM = None
        self.gaps = [0.1, 0.2, 0.3]
        self.cancelOperation = False
        CMDS = []
        modelVisibility = []
        FCAD = FreeCAD.ActiveDocument

        try:
            dotIdx = __name__.index(".") + 1
        except Exception:
            dotIdx = 0
        self.module = __name__[dotIdx:]

        # Set debugging behavior
        self.showDebugObjects = False  # Set to true if you want a visual DocObjects created for some path construction objects
        self.showDebugObjects = obj.ShowTempObjects
        deleteTempsFlag = True  # Set to False for debugging
        if Path.Log.getLevel(Path.Log.thisModule()) == 4:
            deleteTempsFlag = False
        else:
            self.showDebugObjects = False

        # mark beginning of operation and identify parent Job
        startTime = time.time()

        # Identify parent Job
        JOB = PathUtils.findParentJob(obj)
        self.JOB = JOB
        if JOB is None:
            Path.Log.error(translate("PathSurface", "No JOB"))
            return
        self.stockZMin = JOB.Stock.Shape.BoundBox.ZMin

        # set cut mode; reverse as needed
        if obj.CutMode == "Climb":
            self.CutClimb = True
        if obj.CutPatternReversed:
            if self.CutClimb:
                self.CutClimb = False
            else:
                self.CutClimb = True

        # Instantiate additional class operation variables
        self.resetOpVariables()

        # Setup cutter for OCL and cutout value for operation - based on tool controller properties
        oclTool = PathSurfaceSupport.OCL_Tool(ocl, obj)
        self.cutter = oclTool.getOclTool()
        if not self.cutter:
            Path.Log.error(
                translate(
                    "PathSurface",
                    "Canceling 3D Surface operation. Error creating OCL cutter.",
                )
            )
            return
        self.toolDiam = self.cutter.getDiameter()  # oclTool.diameter
        self.radius = self.toolDiam / 2.0
        self.useTiltCutter = oclTool.useTiltCutter()
        self.cutOut = self.toolDiam * (float(obj.StepOver) / 100.0)
        self.gaps = [self.toolDiam, self.toolDiam, self.toolDiam]

        # Begin GCode for operation with basic information
        # ... and move cutter to clearance height and startpoint
        output = ""
        if obj.Comment != "":
            self.commandlist.append(Path.Command("N ({})".format(str(obj.Comment)), {}))
        self.commandlist.append(Path.Command("N ({})".format(obj.Label), {}))
        self.commandlist.append(
            Path.Command("N (Tool type: {})".format(oclTool.toolType), {})
        )
        self.commandlist.append(
            Path.Command(
                "N (Compensated Tool Path. Diameter: {})".format(oclTool.diameter), {}
            )
        )
        self.commandlist.append(
            Path.Command(
                "N (Sample interval: {})".format(str(obj.SampleInterval.Value)), {}
            )
        )
        self.commandlist.append(
            Path.Command("N (Step over %: {})".format(str(obj.StepOver)), {})
        )
        self.commandlist.append(Path.Command("N ({})".format(output), {}))
        self.commandlist.append(
            Path.Command("G0", {"Z": obj.ClearanceHeight.Value, "F": self.vertRapid})
        )
        if obj.UseStartPoint is True:
            self.commandlist.append(
                Path.Command(
                    "G0",
                    {
                        "X": obj.StartPoint.x,
                        "Y": obj.StartPoint.y,
                        "F": self.horizRapid,
                    },
                )
            )

        # Impose property limits
        self.opApplyPropertyLimits(obj)

        # Create temporary group for temporary objects, removing existing
        tempGroupName = "tempPathSurfaceGroup"
        if FCAD.getObject(tempGroupName):
            for to in FCAD.getObject(tempGroupName).Group:
                FCAD.removeObject(to.Name)
            FCAD.removeObject(tempGroupName)  # remove temp directory if already exists
        if FCAD.getObject(tempGroupName + "001"):
            for to in FCAD.getObject(tempGroupName + "001").Group:
                FCAD.removeObject(to.Name)
            FCAD.removeObject(
                tempGroupName + "001"
            )  # remove temp directory if already exists
        tempGroup = FCAD.addObject("App::DocumentObjectGroup", tempGroupName)
        tempGroupName = tempGroup.Name
        self.tempGroup = tempGroup
        tempGroup.purgeTouched()
        # Add temp object to temp group folder with following code:
        # ... self.tempGroup.addObject(OBJ)

        # Get height offset values for later use
        self.SafeHeightOffset = JOB.SetupSheet.SafeHeightOffset.Value
        self.ClearHeightOffset = JOB.SetupSheet.ClearanceHeightOffset.Value

        # Calculate default depthparams for operation
        self.depthParams = PathUtils.depth_params(
            obj.ClearanceHeight.Value,
            obj.SafeHeight.Value,
            obj.StartDepth.Value,
            obj.StepDown.Value,
            0.0,
            obj.FinalDepth.Value,
        )
        self.midDep = (obj.StartDepth.Value + obj.FinalDepth.Value) / 2.0

        # Save model visibilities for restoration
        if FreeCAD.GuiUp:
            for model in JOB.Model.Group:
                mNm = model.Name
                modelVisibility.append(
                    FreeCADGui.ActiveDocument.getObject(mNm).Visibility
                )

        # Setup STL, model type, and bound box containers for each model in Job
        for model in JOB.Model.Group:
            self.modelSTLs.append(False)
            self.safeSTLs.append(False)
            self.profileShapes.append(False)
            # Set bound box
            if obj.BoundBox == "BaseBoundBox":
                if model.TypeId.startswith("Mesh"):
                    self.modelTypes.append("M")  # Mesh
                    self.boundBoxes.append(model.Mesh.BoundBox)
                else:
                    self.modelTypes.append("S")  # Solid
                    self.boundBoxes.append(model.Shape.BoundBox)
            elif obj.BoundBox == "Stock":
                self.modelTypes.append("S")  # Solid
                self.boundBoxes.append(JOB.Stock.Shape.BoundBox)

        # ######  MAIN COMMANDS FOR OPERATION ######

        # Begin processing obj.Base data and creating GCode
        PSF = PathSurfaceSupport.ProcessSelectedFaces(JOB, obj)
        PSF.setShowDebugObjects(tempGroup, self.showDebugObjects)
        PSF.radius = self.radius
        PSF.depthParams = self.depthParams
        pPM = PSF.preProcessModel(self.module)

        # Process selected faces, if available
        if pPM:
            self.cancelOperation = False
            (FACES, VOIDS) = pPM
            self.modelSTLs = PSF.modelSTLs
            self.profileShapes = PSF.profileShapes

            for idx, model in enumerate(JOB.Model.Group):
                Path.Log.debug(idx)
                # Create OCL.stl model objects
                PathSurfaceSupport._prepareModelSTLs(self, JOB, obj, idx, ocl)

                if FACES[idx]:
                    Path.Log.debug(
                        "Working on Model.Group[{}]: {}".format(idx, model.Label)
                    )
                    if idx > 0:
                        # Raise to clearance between models
                        CMDS.append(
                            Path.Command(
                                "N (Transition to base: {}.)".format(model.Label)
                            )
                        )
                        CMDS.append(
                            Path.Command(
                                "G0",
                                {"Z": obj.ClearanceHeight.Value, "F": self.vertRapid},
                            )
                        )
                    # make stock-model-voidShapes STL model for avoidance detection on transitions
                    PathSurfaceSupport._makeSafeSTL(
                        self, JOB, obj, idx, FACES[idx], VOIDS[idx], ocl
                    )
                    # Process model/faces - OCL objects must be ready
                    CMDS.extend(
                        self._processCutAreas(JOB, obj, idx, FACES[idx], VOIDS[idx])
                    )
                else:
                    Path.Log.debug("No data for model base: {}".format(model.Label))

            # Save gcode produced
            self.commandlist.extend(CMDS)
        else:
            Path.Log.error("Failed to pre-process model and/or selected face(s).")

        # ######  CLOSING COMMANDS FOR OPERATION ######

        # Delete temporary objects
        # Restore model visibilities for restoration
        if FreeCAD.GuiUp:
            FreeCADGui.ActiveDocument.getObject(tempGroupName).Visibility = False
            for m in range(0, len(JOB.Model.Group)):
                M = JOB.Model.Group[m]
                M.Visibility = modelVisibility[m]

        if deleteTempsFlag is True:
            for to in tempGroup.Group:
                if hasattr(to, "Group"):
                    for go in to.Group:
                        FCAD.removeObject(go.Name)
                FCAD.removeObject(to.Name)
            FCAD.removeObject(tempGroupName)
        else:
            if len(tempGroup.Group) == 0:
                FCAD.removeObject(tempGroupName)
            else:
                tempGroup.purgeTouched()

        # Provide user feedback for gap sizes
        gaps = []
        for g in self.gaps:
            if g != self.toolDiam:
                gaps.append(g)
        if len(gaps) > 0:
            obj.GapSizes = "{} mm".format(gaps)
        else:
            if self.closedGap is True:
                obj.GapSizes = "Closed gaps < Gap Threshold."
            else:
                obj.GapSizes = "No gaps identified."

        # clean up class variables
        self.resetOpVariables()
        self.deleteOpVariables()

        self.modelSTLs = None
        self.safeSTLs = None
        self.modelTypes = None
        self.boundBoxes = None
        self.gaps = None
        self.closedGap = None
        self.SafeHeightOffset = None
        self.ClearHeightOffset = None
        self.depthParams = None
        self.midDep = None
        del self.modelSTLs
        del self.safeSTLs
        del self.modelTypes
        del self.boundBoxes
        del self.gaps
        del self.closedGap
        del self.SafeHeightOffset
        del self.ClearHeightOffset
        del self.depthParams
        del self.midDep

        execTime = time.time() - startTime
        if execTime > 60.0:
            tMins = math.floor(execTime / 60.0)
            tSecs = execTime - (tMins * 60.0)
            exTime = str(tMins) + " min. " + str(round(tSecs, 5)) + " sec."
        else:
            exTime = str(round(execTime, 5)) + " sec."
        msg = translate("PathSurface", "operation time is")
        FreeCAD.Console.PrintMessage("3D Surface " + msg + " {}\n".format(exTime))

        if self.cancelOperation:
            FreeCAD.ActiveDocument.openTransaction(
                translate("PathSurface", "Canceled 3D Surface operation.")
            )
            FreeCAD.ActiveDocument.removeObject(obj.Name)
            FreeCAD.ActiveDocument.commitTransaction()

        return True

    # Methods for constructing the cut area and creating path geometry
    def _processCutAreas(self, JOB, obj, mdlIdx, FCS, VDS):
        """_processCutAreas(JOB, obj, mdlIdx, FCS, VDS)...
        This method applies any avoided faces or regions to the selected faces.
        It then calls the correct scan method depending on the ScanType property."""
        Path.Log.debug("_processCutAreas()")

        final = []

        # Process faces Collectively or Individually
        if obj.HandleMultipleFeatures == "Collectively":
            if FCS is True:
                COMP = False
            else:
                ADD = Part.makeCompound(FCS)
                if VDS is not False:
                    DEL = Part.makeCompound(VDS)
                    COMP = ADD.cut(DEL)
                else:
                    COMP = ADD

            if obj.ScanType == "Planar":
                final.extend(self._processPlanarOp(JOB, obj, mdlIdx, COMP, 0))
            elif obj.ScanType == "Rotational":
                final.extend(self._processRotationalOp(JOB, obj, mdlIdx, COMP))

        elif obj.HandleMultipleFeatures == "Individually":
            for fsi in range(0, len(FCS)):
                fShp = FCS[fsi]
                # self.deleteOpVariables(all=False)
                self.resetOpVariables(all=False)

                if fShp is True:
                    COMP = False
                else:
                    ADD = Part.makeCompound([fShp])
                    if VDS is not False:
                        DEL = Part.makeCompound(VDS)
                        COMP = ADD.cut(DEL)
                    else:
                        COMP = ADD

                if obj.ScanType == "Planar":
                    final.extend(self._processPlanarOp(JOB, obj, mdlIdx, COMP, fsi))
                elif obj.ScanType == "Rotational":
                    final.extend(self._processRotationalOp(JOB, obj, mdlIdx, COMP))
                COMP = None
        # Eif

        return final

    def _processPlanarOp(self, JOB, obj, mdlIdx, cmpdShp, fsi):
        """_processPlanarOp(JOB, obj, mdlIdx, cmpdShp)...
        This method compiles the main components for the procedural portion of a planar operation (non-rotational).
        It creates the OCL PathDropCutter objects: model and safeTravel.
        It makes the necessary facial geometries for the actual cut area.
        It calls the correct Single or Multi-pass method as needed.
        It returns the gcode for the operation."""
        Path.Log.debug("_processPlanarOp()")
        final = []
        SCANDATA = []

        def getTransition(two):
            first = two[0][0][0]  # [step][item][point]
            safe = obj.SafeHeight.Value + 0.1
            trans = [[FreeCAD.Vector(first.x, first.y, safe)]]
            return trans

        # Compute number and size of stepdowns, and final depth
        if obj.LayerMode == "Single-pass":
            depthparams = [obj.FinalDepth.Value]
        elif obj.LayerMode == "Multi-pass":
            depthparams = [i for i in self.depthParams]
        lenDP = len(depthparams)

        # Prepare PathDropCutter objects with STL data
        pdc = self._planarGetPDC(
            self.modelSTLs[mdlIdx],
            depthparams[lenDP - 1],
            obj.SampleInterval.Value,
            self.cutter,
        )
        safePDC = self._planarGetPDC(
            self.safeSTLs[mdlIdx],
            depthparams[lenDP - 1],
            obj.SampleInterval.Value,
            self.cutter,
        )

        profScan = []
        if obj.ProfileEdges != "None":
            prflShp = self.profileShapes[mdlIdx][fsi]
            if prflShp is False:
                msg = translate("PathSurface", "No profile geometry shape returned.")
                Path.Log.error(msg)
                return []
            self.showDebugObject(prflShp, "NewProfileShape")
            # get offset path geometry and perform OCL scan with that geometry
            pathOffsetGeom = self._offsetFacesToPointData(obj, prflShp)
            if pathOffsetGeom is False:
                msg = translate("PathSurface", "No profile path geometry returned.")
                Path.Log.error(msg)
                return []
            profScan = [self._planarPerformOclScan(obj, pdc, pathOffsetGeom, True)]

        geoScan = []
        if obj.ProfileEdges != "Only":
            self.showDebugObject(cmpdShp, "CutArea")
            # get internal path geometry and perform OCL scan with that geometry
            PGG = PathSurfaceSupport.PathGeometryGenerator(obj, cmpdShp, obj.CutPattern)
            if self.showDebugObjects:
                PGG.setDebugObjectsGroup(self.tempGroup)
            self.tmpCOM = PGG.getCenterOfPattern()
            pathGeom = PGG.generatePathGeometry()
            if pathGeom is False:
                msg = translate("PathSurface", "No clearing shape returned.")
                Path.Log.error(msg)
                return []
            if obj.CutPattern == "Offset":
                useGeom = self._offsetFacesToPointData(obj, pathGeom, profile=False)
                if useGeom is False:
                    msg = translate(
                        "PathSurface", "No clearing path geometry returned."
                    )
                    Path.Log.error(msg)
                    return []
                geoScan = [self._planarPerformOclScan(obj, pdc, useGeom, True)]
            else:
                geoScan = self._planarPerformOclScan(obj, pdc, pathGeom, False)

        if obj.ProfileEdges == "Only":  # ['None', 'Only', 'First', 'Last']
            SCANDATA.extend(profScan)
        if obj.ProfileEdges == "None":
            SCANDATA.extend(geoScan)
        if obj.ProfileEdges == "First":
            profScan.append(getTransition(geoScan))
            SCANDATA.extend(profScan)
            SCANDATA.extend(geoScan)
        if obj.ProfileEdges == "Last":
            SCANDATA.extend(geoScan)
            SCANDATA.extend(profScan)

        if len(SCANDATA) == 0:
            msg = translate("PathSurface", "No scan data to convert to G-code.")
            Path.Log.error(msg)
            return []

        # Apply depth offset
        if obj.DepthOffset.Value != 0.0:
            self._planarApplyDepthOffset(SCANDATA, obj.DepthOffset.Value)

        # If cut pattern is `Circular`, there are zero(almost zero) straight lines to optimize
        # Store initial `OptimizeLinearPaths` value for later restoration
        self.preOLP = obj.OptimizeLinearPaths
        if obj.CutPattern in ["Circular", "CircularZigZag"]:
            obj.OptimizeLinearPaths = False

        # Process OCL scan data
        if obj.LayerMode == "Single-pass":
            final.extend(
                self._planarDropCutSingle(JOB, obj, pdc, safePDC, depthparams, SCANDATA)
            )
        elif obj.LayerMode == "Multi-pass":
            final.extend(
                self._planarDropCutMulti(JOB, obj, pdc, safePDC, depthparams, SCANDATA)
            )

        # If cut pattern is `Circular`, restore initial OLP value
        if obj.CutPattern in ["Circular", "CircularZigZag"]:
            obj.OptimizeLinearPaths = self.preOLP

        # Raise to safe height between individual faces.
        if obj.HandleMultipleFeatures == "Individually":
            final.insert(
                0, Path.Command("G0", {"Z": obj.SafeHeight.Value, "F": self.vertRapid})
            )

        return final

    def _offsetFacesToPointData(self, obj, subShp, profile=True):
        Path.Log.debug("_offsetFacesToPointData()")

        offsetLists = []
        dist = obj.SampleInterval.Value / 5.0
        # defl = obj.SampleInterval.Value / 5.0

        if not profile:
            # Reverse order of wires in each face - inside to outside
            for w in range(len(subShp.Wires) - 1, -1, -1):
                W = subShp.Wires[w]
                PNTS = W.discretize(Distance=dist)
                # PNTS = W.discretize(Deflection=defl)
                if self.CutClimb:
                    PNTS.reverse()
                offsetLists.append(PNTS)
        else:
            # Reference https://forum.freecad.org/viewtopic.php?t=28861#p234939
            for fc in subShp.Faces:
                # Reverse order of wires in each face - inside to outside
                for w in range(len(fc.Wires) - 1, -1, -1):
                    W = fc.Wires[w]
                    PNTS = W.discretize(Distance=dist)
                    # PNTS = W.discretize(Deflection=defl)
                    if self.CutClimb:
                        PNTS.reverse()
                    offsetLists.append(PNTS)

        return offsetLists

    def _planarPerformOclScan(self, obj, pdc, pathGeom, offsetPoints=False):
        """_planarPerformOclScan(obj, pdc, pathGeom, offsetPoints=False)...
        Switching function for calling the appropriate path-geometry to OCL points conversion function
        for the various cut patterns."""
        Path.Log.debug("_planarPerformOclScan()")
        SCANS = []

        if offsetPoints or obj.CutPattern == "Offset":
            PNTSET = PathSurfaceSupport.pathGeomToOffsetPointSet(obj, pathGeom)
            for D in PNTSET:
                stpOvr = []
                ofst = []
                for I in D:
                    if I == "BRK":
                        stpOvr.append(ofst)
                        stpOvr.append(I)
                        ofst = []
                    else:
                        # D format is ((p1, p2), (p3, p4))
                        (A, B) = I
                        ofst.extend(self._planarDropCutScan(pdc, A, B))
                if len(ofst) > 0:
                    stpOvr.append(ofst)
                SCANS.extend(stpOvr)
        elif obj.CutPattern in ["Line", "Spiral", "ZigZag"]:
            stpOvr = []
            if obj.CutPattern == "Line":
                # PNTSET = PathSurfaceSupport.pathGeomToLinesPointSet(obj, pathGeom, self.CutClimb, self.toolDiam, self.closedGap, self.gaps)
                PNTSET = PathSurfaceSupport.pathGeomToLinesPointSet(self, obj, pathGeom)
            elif obj.CutPattern == "ZigZag":
                # PNTSET = PathSurfaceSupport.pathGeomToZigzagPointSet(obj, pathGeom, self.CutClimb, self.toolDiam, self.closedGap, self.gaps)
                PNTSET = PathSurfaceSupport.pathGeomToZigzagPointSet(
                    self, obj, pathGeom
                )
            elif obj.CutPattern == "Spiral":
                PNTSET = PathSurfaceSupport.pathGeomToSpiralPointSet(obj, pathGeom)

            for STEP in PNTSET:
                for LN in STEP:
                    if LN == "BRK":
                        stpOvr.append(LN)
                    else:
                        # D format is ((p1, p2), (p3, p4))
                        (A, B) = LN
                        stpOvr.append(self._planarDropCutScan(pdc, A, B))
                SCANS.append(stpOvr)
                stpOvr = []
        elif obj.CutPattern in ["Circular", "CircularZigZag"]:
            # PNTSET is list, by stepover.
            # Each stepover is a list containing arc/loop descriptions, (sp, ep, cp)
            # PNTSET = PathSurfaceSupport.pathGeomToCircularPointSet(obj, pathGeom, self.CutClimb, self.toolDiam, self.closedGap, self.gaps, self.tmpCOM)
            PNTSET = PathSurfaceSupport.pathGeomToCircularPointSet(self, obj, pathGeom)

            for so in range(0, len(PNTSET)):
                stpOvr = []
                erFlg = False
                (aTyp, dirFlg, ARCS) = PNTSET[so]

                if dirFlg == 1:  # 1
                    cMode = True
                else:
                    cMode = False

                for a in range(0, len(ARCS)):
                    Arc = ARCS[a]
                    if Arc == "BRK":
                        stpOvr.append("BRK")
                    else:
                        scan = self._planarCircularDropCutScan(pdc, Arc, cMode)
                        if scan is False:
                            erFlg = True
                        else:
                            if aTyp == "L":
                                scan.append(
                                    FreeCAD.Vector(scan[0].x, scan[0].y, scan[0].z)
                                )
                            stpOvr.append(scan)
                if erFlg is False:
                    SCANS.append(stpOvr)
        # Eif

        return SCANS

    def _planarDropCutScan(self, pdc, A, B):
        (x1, y1) = A
        (x2, y2) = B
        path = ocl.Path()  # create an empty path object
        p1 = ocl.Point(x1, y1, 0)  # start-point of line
        p2 = ocl.Point(x2, y2, 0)  # end-point of line
        lo = ocl.Line(p1, p2)  # line-object
        path.append(lo)  # add the line to the path
        pdc.setPath(path)
        pdc.run()  # run dropcutter algorithm on path
        CLP = pdc.getCLPoints()
        PNTS = [FreeCAD.Vector(p.x, p.y, p.z) for p in CLP]
        return PNTS  # pdc.getCLPoints()

    def _planarCircularDropCutScan(self, pdc, Arc, cMode):
        path = ocl.Path()  # create an empty path object
        (sp, ep, cp) = Arc

        # process list of segment tuples (vect, vect)
        p1 = ocl.Point(sp[0], sp[1], 0)  # start point of arc
        p2 = ocl.Point(ep[0], ep[1], 0)  # end point of arc
        C = ocl.Point(cp[0], cp[1], 0)  # center point of arc
        ao = ocl.Arc(p1, p2, C, cMode)  # arc object
        path.append(ao)  # add the arc to the path
        pdc.setPath(path)
        pdc.run()  # run dropcutter algorithm on path
        CLP = pdc.getCLPoints()

        # Convert OCL object data to FreeCAD vectors
        return [FreeCAD.Vector(p.x, p.y, p.z) for p in CLP]

    # Main planar scan functions
    def _planarDropCutSingle(self, JOB, obj, pdc, safePDC, depthparams, SCANDATA):
        Path.Log.debug("_planarDropCutSingle()")

        GCODE = [Path.Command("N (Beginning of Single-pass layer.)", {})]
        tolrnc = JOB.GeometryTolerance.Value
        lenSCANDATA = len(SCANDATA)
        gDIR = ["G3", "G2"]

        if self.CutClimb:
            gDIR = ["G2", "G3"]

        # Set `ProfileEdges` specific trigger indexes
        peIdx = lenSCANDATA  # off by default
        if obj.ProfileEdges == "Only":
            peIdx = -1
        elif obj.ProfileEdges == "First":
            peIdx = 0
        elif obj.ProfileEdges == "Last":
            peIdx = lenSCANDATA - 1

        # Send cutter to x,y position of first point on first line
        first = SCANDATA[0][0][0]  # [step][item][point]
        GCODE.append(
            Path.Command("G0", {"X": first.x, "Y": first.y, "F": self.horizRapid})
        )

        # Cycle through step-over sections (line segments or arcs)
        odd = True
        lstStpEnd = None
        for so in range(0, lenSCANDATA):
            cmds = []
            PRTS = SCANDATA[so]
            lenPRTS = len(PRTS)
            first = PRTS[0][0]  # first point of arc/line stepover group
            last = None
            cmds.append(Path.Command("N (Begin step {}.)".format(so), {}))

            if so > 0:
                if obj.CutPattern == "CircularZigZag":
                    if odd:
                        odd = False
                    else:
                        odd = True
                cmds.extend(
                    self._stepTransitionCmds(obj, lstStpEnd, first, safePDC, tolrnc)
                )
            # Override default `OptimizeLinearPaths` behavior to allow
            # `ProfileEdges` optimization
            if so == peIdx or peIdx == -1:
                obj.OptimizeLinearPaths = self.preOLP

            # Cycle through current step-over parts
            for i in range(0, lenPRTS):
                prt = PRTS[i]
                lenPrt = len(prt)
                if prt == "BRK":
                    nxtStart = PRTS[i + 1][0]
                    cmds.append(Path.Command("N (Break)", {}))
                    cmds.extend(
                        self._stepTransitionCmds(obj, last, nxtStart, safePDC, tolrnc)
                    )
                else:
                    cmds.append(Path.Command("N (part {}.)".format(i + 1), {}))
                    last = prt[lenPrt - 1]
                    if so == peIdx or peIdx == -1:
                        cmds.extend(self._planarSinglepassProcess(obj, prt))
                    elif (
                        obj.CutPattern in ["Circular", "CircularZigZag"]
                        and obj.CircularUseG2G3 is True
                        and lenPrt > 2
                    ):
                        (rtnVal, gcode) = self._arcsToG2G3(
                            prt, lenPrt, odd, gDIR, tolrnc
                        )
                        if rtnVal:
                            cmds.extend(gcode)
                        else:
                            cmds.extend(self._planarSinglepassProcess(obj, prt))
                    else:
                        cmds.extend(self._planarSinglepassProcess(obj, prt))
            cmds.append(Path.Command("N (End of step {}.)".format(so), {}))
            GCODE.extend(cmds)  # save line commands
            lstStpEnd = last

            # Return `OptimizeLinearPaths` to disabled
            if so == peIdx or peIdx == -1:
                if obj.CutPattern in ["Circular", "CircularZigZag"]:
                    obj.OptimizeLinearPaths = False
        # Efor

        return GCODE

    def _planarSinglepassProcess(self, obj, points):
        if obj.OptimizeLinearPaths:
            points = PathUtils.simplify3dLine(
                points, tolerance=obj.LinearDeflection.Value
            )
        # Begin processing ocl points list into gcode
        commands = []
        for pnt in points:
            commands.append(
                Path.Command(
                    "G1", {"X": pnt.x, "Y": pnt.y, "Z": pnt.z, "F": self.horizFeed}
                )
            )
        return commands

    def _planarDropCutMulti(self, JOB, obj, pdc, safePDC, depthparams, SCANDATA):
        GCODE = [Path.Command("N (Beginning of Multi-pass layers.)", {})]
        tolrnc = JOB.GeometryTolerance.Value
        lenDP = len(depthparams)
        prevDepth = depthparams[0]
        lenSCANDATA = len(SCANDATA)
        gDIR = ["G3", "G2"]

        if self.CutClimb:
            gDIR = ["G2", "G3"]

        # Set `ProfileEdges` specific trigger indexes
        peIdx = lenSCANDATA  # off by default
        if obj.ProfileEdges == "Only":
            peIdx = -1
        elif obj.ProfileEdges == "First":
            peIdx = 0
        elif obj.ProfileEdges == "Last":
            peIdx = lenSCANDATA - 1

        # Process each layer in depthparams
        lastPrvStpLast = None
        for lyr in range(0, lenDP):
            odd = True  # ZigZag directional switch
            lyrHasCmds = False
            actvSteps = 0
            LYR = []
            # if lyr > 0:
            #     if prvStpLast is not None:
            #         lastPrvStpLast = prvStpLast
            prvStpLast = None
            lyrDep = depthparams[lyr]
            Path.Log.debug("Multi-pass lyrDep: {}".format(round(lyrDep, 4)))

            # Cycle through step-over sections (line segments or arcs)
            for so in range(0, len(SCANDATA)):
                SO = SCANDATA[so]
                lenSO = len(SO)

                # Pre-process step-over parts for layer depth and holds
                ADJPRTS = []
                LMAX = []
                soHasPnts = False
                brkFlg = False
                for i in range(0, lenSO):
                    prt = SO[i]
                    lenPrt = len(prt)
                    if prt == "BRK":
                        if brkFlg:
                            ADJPRTS.append(prt)
                            LMAX.append(prt)
                            brkFlg = False
                    else:
                        (PTS, lMax) = self._planarMultipassPreProcess(
                            obj, prt, prevDepth, lyrDep
                        )
                        if len(PTS) > 0:
                            ADJPRTS.append(PTS)
                            soHasPnts = True
                            brkFlg = True
                            LMAX.append(lMax)
                # Efor
                lenAdjPrts = len(ADJPRTS)

                # Process existing parts within current step over
                prtsHasCmds = False
                stepHasCmds = False
                prtsCmds = []
                stpOvrCmds = []
                transCmds = []
                if soHasPnts is True:
                    first = ADJPRTS[0][0]  # first point of arc/line stepover group
                    last = None

                    # Manage step over transition and CircularZigZag direction
                    if so > 0:
                        # Control ZigZag direction
                        if obj.CutPattern == "CircularZigZag":
                            if odd is True:
                                odd = False
                            else:
                                odd = True
                        # Control step over transition
                        if prvStpLast is None:
                            prvStpLast = lastPrvStpLast
                        transCmds.extend(
                            self._stepTransitionCmds(
                                obj, prvStpLast, first, safePDC, tolrnc
                            )
                        )

                    # Override default `OptimizeLinearPaths` behavior to allow `ProfileEdges` optimization
                    if so == peIdx or peIdx == -1:
                        obj.OptimizeLinearPaths = self.preOLP

                    # Cycle through current step-over parts
                    for i in range(0, lenAdjPrts):
                        prt = ADJPRTS[i]
                        lenPrt = len(prt)
                        if prt == "BRK" and prtsHasCmds:
                            if i + 1 < lenAdjPrts:
                                nxtStart = ADJPRTS[i + 1][0]
                                prtsCmds.append(Path.Command("N (--Break)", {}))
                            else:
                                # Transition straight up to Safe Height if no more parts
                                nxtStart = FreeCAD.Vector(
                                    last.x, last.y, obj.SafeHeight.Value
                                )
                            prtsCmds.extend(
                                self._stepTransitionCmds(
                                    obj, last, nxtStart, safePDC, tolrnc
                                )
                            )
                        else:
                            segCmds = False
                            prtsCmds.append(
                                Path.Command("N (part {})".format(i + 1), {})
                            )
                            last = prt[lenPrt - 1]
                            if so == peIdx or peIdx == -1:
                                segCmds = self._planarSinglepassProcess(obj, prt)
                            elif (
                                obj.CutPattern in ["Circular", "CircularZigZag"]
                                and obj.CircularUseG2G3 is True
                                and lenPrt > 2
                            ):
                                (rtnVal, gcode) = self._arcsToG2G3(
                                    prt, lenPrt, odd, gDIR, tolrnc
                                )
                                if rtnVal is True:
                                    segCmds = gcode
                                else:
                                    segCmds = self._planarSinglepassProcess(obj, prt)
                            else:
                                segCmds = self._planarSinglepassProcess(obj, prt)

                            if segCmds is not False:
                                prtsCmds.extend(segCmds)
                                prtsHasCmds = True
                                prvStpLast = last
                        # Eif
                    # Efor
                # Eif

                # Return `OptimizeLinearPaths` to disabled
                if so == peIdx or peIdx == -1:
                    if obj.CutPattern in ["Circular", "CircularZigZag"]:
                        obj.OptimizeLinearPaths = False

                # Compile step over(prts) commands
                if prtsHasCmds is True:
                    stepHasCmds = True
                    actvSteps += 1
                    stpOvrCmds.extend(transCmds)
                    stpOvrCmds.append(Path.Command("N (Begin step {}.)".format(so), {}))
                    stpOvrCmds.append(
                        Path.Command(
                            "G0", {"X": first.x, "Y": first.y, "F": self.horizRapid}
                        )
                    )
                    stpOvrCmds.extend(prtsCmds)
                    stpOvrCmds.append(
                        Path.Command("N (End of step {}.)".format(so), {})
                    )

                # Layer transition at first active step over in current layer
                if actvSteps == 1:
                    LYR.append(Path.Command("N (Layer {} begins)".format(lyr), {}))
                    if lyr > 0:
                        LYR.append(Path.Command("N (Layer transition)", {}))
                        LYR.append(
                            Path.Command(
                                "G0", {"Z": obj.SafeHeight.Value, "F": self.vertRapid}
                            )
                        )
                        LYR.append(
                            Path.Command(
                                "G0", {"X": first.x, "Y": first.y, "F": self.horizRapid}
                            )
                        )

                if stepHasCmds is True:
                    lyrHasCmds = True
                    LYR.extend(stpOvrCmds)
            # Eif

            # Close layer, saving commands, if any
            if lyrHasCmds is True:
                GCODE.extend(LYR)  # save line commands
                GCODE.append(Path.Command("N (End of layer {})".format(lyr), {}))

            # Set previous depth
            prevDepth = lyrDep
        # Efor

        Path.Log.debug("Multi-pass op has {} layers (step downs).".format(lyr + 1))

        return GCODE

    def _planarMultipassPreProcess(self, obj, LN, prvDep, layDep):
        ALL = []
        PTS = []
        optLinTrans = obj.OptimizeStepOverTransitions
        safe = math.ceil(obj.SafeHeight.Value)

        if optLinTrans is True:
            for P in LN:
                ALL.append(P)
                # Handle layer depth AND hold points
                if P.z <= layDep:
                    PTS.append(FreeCAD.Vector(P.x, P.y, layDep))
                elif P.z > prvDep:
                    PTS.append(FreeCAD.Vector(P.x, P.y, safe))
                else:
                    PTS.append(FreeCAD.Vector(P.x, P.y, P.z))
            # Efor
        else:
            for P in LN:
                ALL.append(P)
                # Handle layer depth only
                if P.z <= layDep:
                    PTS.append(FreeCAD.Vector(P.x, P.y, layDep))
                else:
                    PTS.append(FreeCAD.Vector(P.x, P.y, P.z))
            # Efor

        if optLinTrans is True:
            # Remove leading and trailing Hold Points
            popList = []
            for i in range(0, len(PTS)):  # identify leading string
                if PTS[i].z == safe:
                    popList.append(i)
                else:
                    break
            popList.sort(reverse=True)
            for p in popList:  # Remove hold points
                PTS.pop(p)
                ALL.pop(p)
            popList = []
            for i in range(len(PTS) - 1, -1, -1):  # identify trailing string
                if PTS[i].z == safe:
                    popList.append(i)
                else:
                    break
            popList.sort(reverse=True)
            for p in popList:  # Remove hold points
                PTS.pop(p)
                ALL.pop(p)

        # Determine max Z height for remaining points on line
        lMax = obj.FinalDepth.Value
        if len(ALL) > 0:
            lMax = ALL[0].z
            for P in ALL:
                if P.z > lMax:
                    lMax = P.z

        return (PTS, lMax)

    def _planarMultipassProcess(self, obj, PNTS, lMax):
        output = []
        optimize = obj.OptimizeLinearPaths
        safe = math.ceil(obj.SafeHeight.Value)
        lenPNTS = len(PNTS)
        prcs = True
        onHold = False
        onLine = False
        clrScnLn = lMax + 2.0

        # Initialize first three points
        nxt = None
        pnt = PNTS[0]
        prev = FreeCAD.Vector(-442064564.6, 258539656553.27, 3538553425.847)

        #  Add temp end point
        PNTS.append(FreeCAD.Vector(-4895747464.6, -25855763553.2, 35865763425))

        # Begin processing ocl points list into gcode
        for i in range(0, lenPNTS):
            prcs = True
            nxt = PNTS[i + 1]

            if pnt.z == safe:
                prcs = False
                if onHold is False:
                    onHold = True
                    output.append(Path.Command("N (Start hold)", {}))
                    output.append(
                        Path.Command("G0", {"Z": clrScnLn, "F": self.vertRapid})
                    )
            else:
                if onHold is True:
                    onHold = False
                    output.append(Path.Command("N (End hold)", {}))
                    output.append(
                        Path.Command(
                            "G0", {"X": pnt.x, "Y": pnt.y, "F": self.horizRapid}
                        )
                    )

            # Process point
            if prcs is True:
                if optimize is True:
                    # iPOL = prev.isOnLineSegment(nxt, pnt)
                    iPOL = pnt.isOnLineSegment(prev, nxt)
                    if iPOL is True:
                        onLine = True
                    else:
                        onLine = False
                        output.append(
                            Path.Command(
                                "G1",
                                {
                                    "X": pnt.x,
                                    "Y": pnt.y,
                                    "Z": pnt.z,
                                    "F": self.horizFeed,
                                },
                            )
                        )
                else:
                    output.append(
                        Path.Command(
                            "G1",
                            {"X": pnt.x, "Y": pnt.y, "Z": pnt.z, "F": self.horizFeed},
                        )
                    )

            # Rotate point data
            if onLine is False:
                prev = pnt
            pnt = nxt
        # Efor

        PNTS.pop()  # Remove temp end point

        return output

    def _stepTransitionCmds(self, obj, p1, p2, safePDC, tolrnc):
        """Generate transition commands / paths between two dropcutter steps or
        passes, as well as other kinds of breaks. When
        OptimizeStepOverTransitions is enabled, uses safePDC to safely optimize
        short (~order of cutter diameter) transitions."""
        cmds = []
        rtpd = False
        height = obj.SafeHeight.Value
        # Allow cutter-down transitions with a distance up to 2x cutter
        # diameter. We might be able to extend this further to the
        # full-retract-and-rapid break even point in the future, but this will
        # require a safeSTL that has all non-cut surfaces raised sufficiently
        # to avoid inadvertent cutting.
        maxXYDistanceSqrd = (self.cutter.getDiameter() * 2) ** 2

        if obj.OptimizeStepOverTransitions:
            if p1 and p2:
                # Short distance within step over
                xyDistanceSqrd = (p1.x - p2.x) ** 2 + (p1.y - p2.y) ** 2
                # Try to keep cutting for short distances.
                if xyDistanceSqrd <= maxXYDistanceSqrd:
                    # Try to keep cutting, following the model shape
                    (transLine, minZ, maxZ) = self._getTransitionLine(
                        safePDC, p1, p2, obj
                    )
                    # For now, only optimize moderate deviations in Z direction, and
                    # no dropping below the min of p1 and p2, primarily for multi
                    # layer path safety.
                    zFloor = min(p1.z, p2.z)
                    if abs(minZ - maxZ) < self.cutter.getDiameter():
                        for pt in transLine[1:-1]:
                            cmds.append(
                                Path.Command(
                                    "G1",
                                    {
                                        "X": pt.x,
                                        "Y": pt.y,
                                        # Enforce zFloor
                                        "Z": max(pt.z, zFloor),
                                        "F": self.horizFeed,
                                    },
                                )
                            )
                        # Use p2 (start of next step) verbatim
                        cmds.append(
                            Path.Command(
                                "G1",
                                {"X": p2.x, "Y": p2.y, "Z": p2.z, "F": self.horizFeed},
                            )
                        )
                        return cmds
                # For longer distances or large z deltas, we conservatively lift
                # to SafeHeight for lack of an accurate stock model, but then
                # speed up the drop back down when using multi pass, dropping
                # quickly to *previous* layer depth.
                stepDown = obj.StepDown.Value if hasattr(obj, "StepDown") else 0
                rtpd = min(height, p2.z + stepDown + 2)
            elif not p1:
                Path.Log.debug("_stepTransitionCmds() p1 is None")
            elif not p2:
                Path.Log.debug("_stepTransitionCmds() p2 is None")

        # Create raise, shift, and optional lower commands
        if height is not False:
            cmds.append(Path.Command("G0", {"Z": height, "F": self.vertRapid}))
            cmds.append(
                Path.Command("G0", {"X": p2.x, "Y": p2.y, "F": self.horizRapid})
            )
        if rtpd is not False:  # ReturnToPreviousDepth
            cmds.append(Path.Command("G0", {"Z": rtpd, "F": self.vertRapid}))

        return cmds

    def _arcsToG2G3(self, LN, numPts, odd, gDIR, tolrnc):
        cmds = []
        strtPnt = LN[0]
        endPnt = LN[numPts - 1]
        strtHght = strtPnt.z
        coPlanar = True
        isCircle = False
        gdi = 0
        if odd is True:
            gdi = 1

        # Test if pnt set is circle
        if abs(strtPnt.x - endPnt.x) < tolrnc:
            if abs(strtPnt.y - endPnt.y) < tolrnc:
                if abs(strtPnt.z - endPnt.z) < tolrnc:
                    isCircle = True
        isCircle = False

        if isCircle is True:
            # convert LN to G2/G3 arc, consolidating GCode
            # https://wiki.shapeoko.com/index.php/G-Code#G2_-_clockwise_arc
            # https://www.cnccookbook.com/cnc-g-code-arc-circle-g02-g03/
            # Dividing circle into two arcs allows for G2/G3 on inclined surfaces

            # ijk = self.tmpCOM - strtPnt  # vector from start to center
            ijk = self.tmpCOM - strtPnt  # vector from start to center
            xyz = self.tmpCOM.add(ijk)  # end point
            cmds.append(
                Path.Command(
                    "G1",
                    {
                        "X": strtPnt.x,
                        "Y": strtPnt.y,
                        "Z": strtPnt.z,
                        "F": self.horizFeed,
                    },
                )
            )
            cmds.append(
                Path.Command(
                    gDIR[gdi],
                    {
                        "X": xyz.x,
                        "Y": xyz.y,
                        "Z": xyz.z,
                        "I": ijk.x,
                        "J": ijk.y,
                        "K": ijk.z,  # leave same xyz.z height
                        "F": self.horizFeed,
                    },
                )
            )
            cmds.append(
                Path.Command(
                    "G1", {"X": xyz.x, "Y": xyz.y, "Z": xyz.z, "F": self.horizFeed}
                )
            )
            ijk = self.tmpCOM - xyz  # vector from start to center
            rst = strtPnt  # end point
            cmds.append(
                Path.Command(
                    gDIR[gdi],
                    {
                        "X": rst.x,
                        "Y": rst.y,
                        "Z": rst.z,
                        "I": ijk.x,
                        "J": ijk.y,
                        "K": ijk.z,  # leave same xyz.z height
                        "F": self.horizFeed,
                    },
                )
            )
            cmds.append(
                Path.Command(
                    "G1",
                    {
                        "X": strtPnt.x,
                        "Y": strtPnt.y,
                        "Z": strtPnt.z,
                        "F": self.horizFeed,
                    },
                )
            )
        else:
            for pt in LN:
                if abs(pt.z - strtHght) > tolrnc:  # test for horizontal coplanar
                    coPlanar = False
                    break
            if coPlanar is True:
                # ijk = self.tmpCOM - strtPnt
                ijk = self.tmpCOM.sub(strtPnt)  # vector from start to center
                xyz = endPnt
                cmds.append(
                    Path.Command(
                        "G1",
                        {
                            "X": strtPnt.x,
                            "Y": strtPnt.y,
                            "Z": strtPnt.z,
                            "F": self.horizFeed,
                        },
                    )
                )
                cmds.append(
                    Path.Command(
                        gDIR[gdi],
                        {
                            "X": xyz.x,
                            "Y": xyz.y,
                            "Z": xyz.z,
                            "I": ijk.x,
                            "J": ijk.y,
                            "K": ijk.z,  # leave same xyz.z height
                            "F": self.horizFeed,
                        },
                    )
                )
                cmds.append(
                    Path.Command(
                        "G1",
                        {
                            "X": endPnt.x,
                            "Y": endPnt.y,
                            "Z": endPnt.z,
                            "F": self.horizFeed,
                        },
                    )
                )

        return (coPlanar, cmds)

    def _planarApplyDepthOffset(self, SCANDATA, DepthOffset):
        Path.Log.debug("Applying DepthOffset value: {}".format(DepthOffset))
        lenScans = len(SCANDATA)
        for s in range(0, lenScans):
            SO = SCANDATA[s]  # StepOver
            numParts = len(SO)
            for prt in range(0, numParts):
                PRT = SO[prt]
                if PRT != "BRK":
                    numPts = len(PRT)
                    for pt in range(0, numPts):
                        SCANDATA[s][prt][pt].z += DepthOffset

    def _planarGetPDC(self, stl, finalDep, SampleInterval, cutter):
        pdc = ocl.PathDropCutter()  # create a pdc [PathDropCutter] object
        pdc.setSTL(stl)  # add stl model
        pdc.setCutter(cutter)  # add cutter
        pdc.setZ(finalDep)  # set minimumZ (final / target depth value)
        pdc.setSampling(SampleInterval)  # set sampling size
        return pdc

    # Main rotational scan functions
    def _processRotationalOp(self, JOB, obj, mdlIdx, compoundFaces=None):
        Path.Log.debug(
            "_processRotationalOp(self, JOB, obj, mdlIdx, compoundFaces=None)"
        )

        base = JOB.Model.Group[mdlIdx]
        bb = self.boundBoxes[mdlIdx]
        stl = self.modelSTLs[mdlIdx]

        # Rotate model to initial index
        initIdx = obj.CutterTilt + obj.StartIndex
        if initIdx != 0.0:
            self.basePlacement = FreeCAD.ActiveDocument.getObject(base.Name).Placement
            if obj.RotationAxis == "X":
                base.Placement = FreeCAD.Placement(
                    FreeCAD.Vector(0.0, 0.0, 0.0),
                    FreeCAD.Rotation(FreeCAD.Vector(1.0, 0.0, 0.0), initIdx),
                )
            else:
                base.Placement = FreeCAD.Placement(
                    FreeCAD.Vector(0.0, 0.0, 0.0),
                    FreeCAD.Rotation(FreeCAD.Vector(0.0, 1.0, 0.0), initIdx),
                )

        # Prepare global holdpoint container
        if self.holdPoint is None:
            self.holdPoint = FreeCAD.Vector(0.0, 0.0, 0.0)
        if self.layerEndPnt is None:
            self.layerEndPnt = FreeCAD.Vector(0.0, 0.0, 0.0)

        # Avoid division by zero in rotational scan calculations
        if obj.FinalDepth.Value == 0.0:
            zero = obj.SampleInterval.Value  # 0.00001
            self.FinalDepth = zero
            # obj.FinalDepth.Value = 0.0
        else:
            self.FinalDepth = obj.FinalDepth.Value

        # Determine boundbox radius based upon xzy limits data
        if math.fabs(bb.ZMin) > math.fabs(bb.ZMax):
            vlim = bb.ZMin
        else:
            vlim = bb.ZMax
        if obj.RotationAxis == "X":
            # Rotation is around X-axis, cutter moves along same axis
            if math.fabs(bb.YMin) > math.fabs(bb.YMax):
                hlim = bb.YMin
            else:
                hlim = bb.YMax
        else:
            # Rotation is around Y-axis, cutter moves along same axis
            if math.fabs(bb.XMin) > math.fabs(bb.XMax):
                hlim = bb.XMin
            else:
                hlim = bb.XMax

        # Compute max radius of stock, as it rotates, and rotational clearance & safe heights
        self.bbRadius = math.sqrt(hlim**2 + vlim**2)
        self.clearHeight = self.bbRadius + JOB.SetupSheet.ClearanceHeightOffset.Value
        self.safeHeight = self.bbRadius + JOB.SetupSheet.ClearanceHeightOffset.Value

        return self._rotationalDropCutterOp(obj, stl, bb)

    def _rotationalDropCutterOp(self, obj, stl, bb):
        self.resetTolerance = 0.0000001  # degrees
        self.layerEndzMax = 0.0
        commands = []
        scanLines = []
        advances = []
        iSTG = []
        rSTG = []
        rings = []
        lCnt = 0
        rNum = 0
        bbRad = self.bbRadius

        def invertAdvances(advances):
            idxs = [1.1]
            for adv in advances:
                idxs.append(-1 * adv)
            idxs.pop(0)
            return idxs

        def linesToPointRings(scanLines):
            rngs = []
            numPnts = len(
                scanLines[0]
            )  # Number of points per line along axis, at obj.SampleInterval.Value spacing
            for (
                line
            ) in scanLines:  # extract circular set(ring) of points from scan lines
                if len(line) != numPnts:
                    Path.Log.debug("Error: line lengths not equal")
                    return rngs

            for num in range(0, numPnts):
                rngs.append([1.1])  # Initiate new ring
                for (
                    line
                ) in scanLines:  # extract circular set(ring) of points from scan lines
                    rngs[num].append(line[num])
                rngs[num].pop(0)
            return rngs

        def indexAdvances(arc, stepDeg):
            indexes = [0.0]
            numSteps = int(math.floor(arc / stepDeg))
            for ns in range(0, numSteps):
                indexes.append(stepDeg)

            travel = sum(indexes)
            if arc == 360.0:
                indexes.insert(0, 0.0)
            else:
                indexes.append(arc - travel)

            return indexes

        # Compute number and size of stepdowns, and final depth
        if obj.LayerMode == "Single-pass":
            depthparams = [self.FinalDepth]
        else:
            dep_par = PathUtils.depth_params(
                self.clearHeight,
                self.safeHeight,
                self.bbRadius,
                obj.StepDown.Value,
                0.0,
                self.FinalDepth,
            )
            depthparams = [i for i in dep_par]
        prevDepth = depthparams[0]
        lenDP = len(depthparams)

        # Set drop cutter extra offset
        cdeoX = obj.DropCutterExtraOffset.x
        cdeoY = obj.DropCutterExtraOffset.y

        # Set updated bound box values and redefine the new min/mas XY area of the operation based on greatest point radius of model
        bb.ZMin = -1 * bbRad
        bb.ZMax = bbRad
        if obj.RotationAxis == "X":
            bb.YMin = -1 * bbRad
            bb.YMax = bbRad
            ymin = 0.0
            ymax = 0.0
            xmin = bb.XMin - cdeoX
            xmax = bb.XMax + cdeoX
        else:
            bb.XMin = -1 * bbRad
            bb.XMax = bbRad
            ymin = bb.YMin - cdeoY
            ymax = bb.YMax + cdeoY
            xmin = 0.0
            xmax = 0.0

        # Calculate arc
        begIdx = obj.StartIndex
        endIdx = obj.StopIndex
        if endIdx < begIdx:
            begIdx -= 360.0
        arc = endIdx - begIdx

        # Begin gcode operation with raising cutter to safe height
        commands.append(Path.Command("G0", {"Z": self.safeHeight, "F": self.vertRapid}))

        # Complete rotational scans at layer and translate into gcode
        for layDep in depthparams:
            t_before = time.time()

            # Compute circumference and step angles for current layer
            layCircum = 2 * math.pi * layDep
            if lenDP == 1:
                layCircum = 2 * math.pi * bbRad

            # Set axial feed rates
            self.axialFeed = 360 / layCircum * self.horizFeed
            self.axialRapid = 360 / layCircum * self.horizRapid

            # Determine step angle.
            if obj.RotationAxis == obj.DropCutterDir:  # Same == indexed
                stepDeg = (self.cutOut / layCircum) * 360.0
            else:
                stepDeg = (obj.SampleInterval.Value / layCircum) * 360.0

            # Limit step angle and determine rotational index angles [indexes].
            if stepDeg > 120.0:
                stepDeg = 120.0
            advances = indexAdvances(arc, stepDeg)  # Reset for each step down layer

            # Perform rotational indexed scans to layer depth
            if obj.RotationAxis == obj.DropCutterDir:  # Same == indexed OR parallel
                sample = obj.SampleInterval.Value
            else:
                sample = self.cutOut
            scanLines = self._indexedDropCutScan(
                obj, stl, advances, xmin, ymin, xmax, ymax, layDep, sample
            )

            # Complete rotation if necessary
            if arc == 360.0:
                advances.append(360.0 - sum(advances))
                advances.pop(0)
                zero = scanLines.pop(0)
                scanLines.append(zero)

            # Translate OCL scans into gcode
            if (
                obj.RotationAxis == obj.DropCutterDir
            ):  # Same == indexed (cutter runs parallel to axis)

                # Translate scan to gcode
                sumAdv = begIdx
                for sl in range(0, len(scanLines)):
                    sumAdv += advances[sl]
                    # Translate scan to gcode
                    iSTG = self._indexedScanToGcode(
                        obj, sl, scanLines[sl], sumAdv, prevDepth, layDep, lenDP
                    )
                    commands.extend(iSTG)

                    # Raise cutter to safe height after each index cut
                    commands.append(
                        Path.Command("G0", {"Z": self.clearHeight, "F": self.vertRapid})
                    )
                # Eol
            else:
                if self.CutClimb is False:
                    advances = invertAdvances(advances)
                    advances.reverse()
                    scanLines.reverse()

                # Begin gcode operation with raising cutter to safe height
                commands.append(
                    Path.Command("G0", {"Z": self.clearHeight, "F": self.vertRapid})
                )

                # Convert rotational scans into gcode
                rings = linesToPointRings(scanLines)
                rNum = 0
                for rng in rings:
                    rSTG = self._rotationalScanToGcode(
                        obj, rng, rNum, prevDepth, layDep, advances
                    )
                    commands.extend(rSTG)
                    if arc != 360.0:
                        clrZ = self.layerEndzMax + self.SafeHeightOffset
                        commands.append(
                            Path.Command("G0", {"Z": clrZ, "F": self.vertRapid})
                        )
                    rNum += 1
                # Eol

            prevDepth = layDep
            lCnt += 1  # increment layer count
            Path.Log.debug(
                "--Layer "
                + str(lCnt)
                + ": "
                + str(len(advances))
                + " OCL scans and gcode in "
                + str(time.time() - t_before)
                + " s"
            )
        # Eol

        return commands

    def _indexedDropCutScan(
        self, obj, stl, advances, xmin, ymin, xmax, ymax, layDep, sample
    ):
        cutterOfst = 0.0
        iCnt = 0
        Lines = []
        result = None

        pdc = ocl.PathDropCutter()  # create a pdc
        pdc.setCutter(self.cutter)
        pdc.setZ(layDep)  # set minimumZ (final / ta9rget depth value)
        pdc.setSampling(sample)

        # if self.useTiltCutter == True:
        if obj.CutterTilt != 0.0:
            cutterOfst = layDep * math.sin(math.radians(obj.CutterTilt))
            Path.Log.debug("CutterTilt: cutterOfst is " + str(cutterOfst))

        sumAdv = 0.0
        for adv in advances:
            sumAdv += adv
            if adv > 0.0:
                # Rotate STL object using OCL method
                radsRot = math.radians(adv)
                if obj.RotationAxis == "X":
                    stl.rotate(radsRot, 0.0, 0.0)
                else:
                    stl.rotate(0.0, radsRot, 0.0)

            # Set STL after rotation is made
            pdc.setSTL(stl)

            # add Line objects to the path in this loop
            if obj.RotationAxis == "X":
                p1 = ocl.Point(xmin, cutterOfst, 0.0)  # start-point of line
                p2 = ocl.Point(xmax, cutterOfst, 0.0)  # end-point of line
            else:
                p1 = ocl.Point(cutterOfst, ymin, 0.0)  # start-point of line
                p2 = ocl.Point(cutterOfst, ymax, 0.0)  # end-point of line

            # Create line object
            if obj.RotationAxis == obj.DropCutterDir:  # parallel cut
                if obj.CutPattern == "ZigZag":
                    if iCnt % 2 == 0.0:  # even
                        lo = ocl.Line(p1, p2)
                    else:  # odd
                        lo = ocl.Line(p2, p1)
                elif obj.CutPattern == "Line":
                    if self.CutClimb is True:
                        lo = ocl.Line(p2, p1)
                    else:
                        lo = ocl.Line(p1, p2)
                else:
                    # default to line-object
                    lo = ocl.Line(p1, p2)
            else:
                lo = ocl.Line(p1, p2)  # line-object

            path = ocl.Path()  # create an empty path object
            path.append(lo)  # add the line to the path
            pdc.setPath(path)  # set path
            pdc.run()  # run drop-cutter on the path
            result = pdc.getCLPoints()  # request the list of points

            # Convert list of OCL objects to list of Vectors for faster access and Apply depth offset
            if obj.DepthOffset.Value != 0.0:
                Lines.append(
                    [
                        FreeCAD.Vector(p.x, p.y, p.z + obj.DepthOffset.Value)
                        for p in result
                    ]
                )
            else:
                Lines.append([FreeCAD.Vector(p.x, p.y, p.z) for p in result])

            iCnt += 1
        # End loop

        # Rotate STL object back to original position using OCL method
        reset = -1 * math.radians(sumAdv - self.resetTolerance)
        if obj.RotationAxis == "X":
            stl.rotate(reset, 0.0, 0.0)
        else:
            stl.rotate(0.0, reset, 0.0)
        self.resetTolerance = 0.0

        return Lines

    def _indexedScanToGcode(self, obj, li, CLP, idxAng, prvDep, layerDepth, numDeps):
        # generate the path commands
        output = []
        optimize = obj.OptimizeLinearPaths
        holdCount = 0
        holdStart = False
        holdStop = False
        zMax = prvDep
        lenCLP = len(CLP)
        lastCLP = lenCLP - 1
        prev = FreeCAD.Vector(0.0, 0.0, 0.0)
        nxt = FreeCAD.Vector(0.0, 0.0, 0.0)

        # Create first point
        pnt = CLP[0]

        # Rotate to correct index location
        if obj.RotationAxis == "X":
            output.append(Path.Command("G0", {"A": idxAng, "F": self.axialFeed}))
        else:
            output.append(Path.Command("G0", {"B": idxAng, "F": self.axialFeed}))

        if li > 0:
            if pnt.z > self.layerEndPnt.z:
                clrZ = pnt.z + 2.0
                output.append(Path.Command("G1", {"Z": clrZ, "F": self.vertRapid}))
        else:
            output.append(
                Path.Command("G0", {"Z": self.clearHeight, "F": self.vertRapid})
            )

        output.append(
            Path.Command("G0", {"X": pnt.x, "Y": pnt.y, "F": self.horizRapid})
        )
        output.append(Path.Command("G1", {"Z": pnt.z, "F": self.vertFeed}))

        for i in range(0, lenCLP):
            if i < lastCLP:
                nxt = CLP[i + 1]
            else:
                optimize = False

            # Update zMax values
            if pnt.z > zMax:
                zMax = pnt.z

            if obj.LayerMode == "Multi-pass":
                # if z travels above previous layer, start/continue hold high cycle
                if pnt.z > prvDep and optimize is True:
                    if self.onHold is False:
                        holdStart = True
                    self.onHold = True

                if self.onHold is True:
                    if holdStart is True:
                        # go to current coordinate
                        output.append(
                            Path.Command(
                                "G1",
                                {
                                    "X": pnt.x,
                                    "Y": pnt.y,
                                    "Z": pnt.z,
                                    "F": self.horizFeed,
                                },
                            )
                        )
                        # Save holdStart coordinate and prvDep values
                        self.holdPoint = pnt
                        holdCount += 1  # Increment hold count
                        holdStart = False  # cancel holdStart

                    # hold cutter high until Z value drops below prvDep
                    if pnt.z <= prvDep:
                        holdStop = True

                if holdStop is True:
                    # Send hold and current points to
                    zMax += 2.0
                    for cmd in self.holdStopCmds(
                        obj, zMax, prvDep, pnt, "Hold Stop: in-line"
                    ):
                        output.append(cmd)
                    # reset necessary hold related settings
                    zMax = prvDep
                    holdStop = False
                    self.onHold = False
                    self.holdPoint = FreeCAD.Vector(0.0, 0.0, 0.0)

            if self.onHold is False:
                if not optimize or not pnt.isOnLineSegment(prev, nxt):
                    output.append(
                        Path.Command(
                            "G1",
                            {"X": pnt.x, "Y": pnt.y, "Z": pnt.z, "F": self.horizFeed},
                        )
                    )

            # Rotate point data
            prev = pnt
            pnt = nxt
        output.append(
            Path.Command("N (End index angle " + str(round(idxAng, 4)) + ")", {})
        )

        # Save layer end point for use in transitioning to next layer
        self.layerEndPnt = pnt

        return output

    def _rotationalScanToGcode(self, obj, RNG, rN, prvDep, layDep, advances):
        """_rotationalScanToGcode(obj, RNG, rN, prvDep, layDep, advances) ...
        Convert rotational scan data to gcode path commands."""
        output = []
        nxtAng = 0
        zMax = 0.0
        nxt = FreeCAD.Vector(0.0, 0.0, 0.0)

        begIdx = obj.StartIndex
        endIdx = obj.StopIndex
        if endIdx < begIdx:
            begIdx -= 360.0

        # Rotate to correct index location
        axisOfRot = "A"
        if obj.RotationAxis == "Y":
            axisOfRot = "B"

        # Create first point
        ang = 0.0 + obj.CutterTilt
        pnt = RNG[0]

        # Adjust feed rate based on radius/circumference of cutter.
        # Original feed rate based on travel at circumference.
        if rN > 0:
            if pnt.z >= self.layerEndzMax:
                clrZ = pnt.z + 5.0
                output.append(Path.Command("G1", {"Z": clrZ, "F": self.vertRapid}))
        else:
            output.append(
                Path.Command("G1", {"Z": self.clearHeight, "F": self.vertRapid})
            )

        output.append(Path.Command("G0", {axisOfRot: ang, "F": self.axialFeed}))
        output.append(Path.Command("G1", {"X": pnt.x, "Y": pnt.y, "F": self.axialFeed}))
        output.append(Path.Command("G1", {"Z": pnt.z, "F": self.axialFeed}))

        lenRNG = len(RNG)
        lastIdx = lenRNG - 1
        for i in range(0, lenRNG):
            if i < lastIdx:
                nxtAng = ang + advances[i + 1]
                nxt = RNG[i + 1]

            if pnt.z > zMax:
                zMax = pnt.z

            output.append(
                Path.Command(
                    "G1",
                    {
                        "X": pnt.x,
                        "Y": pnt.y,
                        "Z": pnt.z,
                        axisOfRot: ang,
                        "F": self.axialFeed,
                    },
                )
            )
            pnt = nxt
            ang = nxtAng

        # Save layer end point for use in transitioning to next layer
        self.layerEndPnt = RNG[0]
        self.layerEndzMax = zMax

        return output

    def holdStopCmds(self, obj, zMax, pd, p2, txt):
        """holdStopCmds(obj, zMax, pd, p2, txt) ... Gcode commands to be executed at beginning of hold."""
        cmds = []
        msg = "N (" + txt + ")"
        cmds.append(
            Path.Command(msg, {})
        )  # Raise cutter rapid to zMax in line of travel
        cmds.append(
            Path.Command("G0", {"Z": zMax, "F": self.vertRapid})
        )  # Raise cutter rapid to zMax in line of travel
        cmds.append(
            Path.Command("G0", {"X": p2.x, "Y": p2.y, "F": self.horizRapid})
        )  # horizontal rapid to current XY coordinate
        if zMax != pd:
            cmds.append(
                Path.Command("G0", {"Z": pd, "F": self.vertRapid})
            )  # drop cutter down rapidly to prevDepth depth
            cmds.append(
                Path.Command("G0", {"Z": p2.z, "F": self.vertFeed})
            )  # drop cutter down to current Z depth, returning to normal cut path and speed
        return cmds

    # Additional support methods
    def resetOpVariables(self, all=True):
        """resetOpVariables() ... Reset class variables used for instance of operation."""
        self.holdPoint = None
        self.layerEndPnt = None
        self.onHold = False
        self.SafeHeightOffset = 2.0
        self.ClearHeightOffset = 4.0
        self.layerEndzMax = 0.0
        self.resetTolerance = 0.0
        self.holdPntCnt = 0
        self.bbRadius = 0.0
        self.axialFeed = 0.0
        self.axialRapid = 0.0
        self.FinalDepth = 0.0
        self.clearHeight = 0.0
        self.safeHeight = 0.0
        self.faceZMax = -999999999999.0
        if all is True:
            self.cutter = None
            self.stl = None
            self.fullSTL = None
            self.cutOut = 0.0
            self.useTiltCutter = False
        return True

    def deleteOpVariables(self, all=True):
        """deleteOpVariables() ... Reset class variables used for instance of operation."""
        del self.holdPoint
        del self.layerEndPnt
        del self.onHold
        del self.SafeHeightOffset
        del self.ClearHeightOffset
        del self.layerEndzMax
        del self.resetTolerance
        del self.holdPntCnt
        del self.bbRadius
        del self.axialFeed
        del self.axialRapid
        del self.FinalDepth
        del self.clearHeight
        del self.safeHeight
        del self.faceZMax
        if all is True:
            del self.cutter
            del self.stl
            del self.fullSTL
            del self.cutOut
            del self.radius
            del self.useTiltCutter
        return True

    def setOclCutter(self, obj, safe=False):
        """setOclCutter(obj) ... Translation function to convert FreeCAD tool definition to OCL formatted tool."""
        # Set cutter details
        #  https://www.freecad.org/api/dd/dfe/classPath_1_1Tool.html#details
        diam_1 = float(obj.ToolController.Tool.Diameter)
        lenOfst = (
            obj.ToolController.Tool.LengthOffset
            if hasattr(obj.ToolController.Tool, "LengthOffset")
            else 0
        )
        FR = (
            obj.ToolController.Tool.FlatRadius
            if hasattr(obj.ToolController.Tool, "FlatRadius")
            else 0
        )
        CEH = (
            obj.ToolController.Tool.CuttingEdgeHeight
            if hasattr(obj.ToolController.Tool, "CuttingEdgeHeight")
            else 0
        )
        CEA = (
            obj.ToolController.Tool.CuttingEdgeAngle
            if hasattr(obj.ToolController.Tool, "CuttingEdgeAngle")
            else 0
        )

        # Make safeCutter with 2 mm buffer around physical cutter
        if safe is True:
            diam_1 += 4.0
            if FR != 0.0:
                FR += 2.0

        Path.Log.debug("ToolType: {}".format(obj.ToolController.Tool.ToolType))
        if obj.ToolController.Tool.ToolType == "EndMill":
            # Standard End Mill
            return ocl.CylCutter(diam_1, (CEH + lenOfst))

        elif obj.ToolController.Tool.ToolType == "BallEndMill" and FR == 0.0:
            # Standard Ball End Mill
            # OCL -> BallCutter::BallCutter(diameter, length)
            self.useTiltCutter = True
            return ocl.BallCutter(diam_1, (diam_1 / 2 + lenOfst))

        elif obj.ToolController.Tool.ToolType == "BallEndMill" and FR > 0.0:
            # Bull Nose or Corner Radius cutter
            # Reference: https://www.fine-tools.com/halbstabfraeser.html
            # OCL -> BallCutter::BallCutter(diameter, length)
            return ocl.BullCutter(diam_1, FR, (CEH + lenOfst))

        elif obj.ToolController.Tool.ToolType == "Engraver" and FR > 0.0:
            # Bull Nose or Corner Radius cutter
            # Reference: https://www.fine-tools.com/halbstabfraeser.html
            # OCL -> ConeCutter::ConeCutter(diameter, angle, lengthOffset)
            return ocl.ConeCutter(diam_1, (CEA / 2), lenOfst)

        elif obj.ToolController.Tool.ToolType == "ChamferMill":
            # Bull Nose or Corner Radius cutter
            # Reference: https://www.fine-tools.com/halbstabfraeser.html
            # OCL -> ConeCutter::ConeCutter(diameter, angle, lengthOffset)
            return ocl.ConeCutter(diam_1, (CEA / 2), lenOfst)
        else:
            # Default to standard end mill
            Path.Log.warning("Defaulting cutter to standard end mill.")
            return ocl.CylCutter(diam_1, (CEH + lenOfst))

    def _getTransitionLine(self, pdc, p1, p2, obj):
        """Use an OCL PathDropCutter to generate a safe transition path between
        two points in the x/y plane."""
        p1xy, p2xy = ((p1.x, p1.y), (p2.x, p2.y))
        pdcLine = self._planarDropCutScan(pdc, p1xy, p2xy)
        if obj.OptimizeLinearPaths:
            pdcLine = PathUtils.simplify3dLine(
                pdcLine, tolerance=obj.LinearDeflection.Value
            )
        zs = [obj.z for obj in pdcLine]
        # PDC z values are based on the model, and do not take into account
        # any remaining stock / multi layer paths. Adjust raw PDC z values to
        # align with p1 and p2 z values.
        zDelta = p1.z - pdcLine[0].z
        if zDelta > 0:
            for p in pdcLine:
                p.z += zDelta
        return (pdcLine, min(zs), max(zs))

    def showDebugObject(self, objShape, objName):
        if self.showDebugObjects:
            do = FreeCAD.ActiveDocument.addObject("Part::Feature", "tmp_" + objName)
            do.Shape = objShape
            do.purgeTouched()
            self.tempGroup.addObject(do)


# Eclass


def SetupProperties():
    """SetupProperties() ... Return list of properties required for operation."""
    return [tup[1] for tup in ObjectSurface.opPropertyDefinitions(False)]


def Create(name, obj=None, parentJob=None):
    """Create(name) ... Creates and returns a Surface operation."""
    if obj is None:
        obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", name)
    obj.Proxy = ObjectSurface(obj, name, parentJob)
    return obj
