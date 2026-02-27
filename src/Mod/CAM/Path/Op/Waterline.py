# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2019 Russell Johnson (russ4262) <russ4262@gmail.com>    *
# *   Copyright (c) 2019 sliptonic <shopinthewoods@gmail.com>               *
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

import FreeCAD

__title__ = "CAM Waterline Operation"
__author__ = "russ4262 (Russell Johnson), sliptonic (Brad Collette)"
__url__ = "https://www.freecad.org"
__doc__ = "Class and implementation of Waterline operation."
__contributors__ = ""

translate = FreeCAD.Qt.translate

# OCL must be installed
try:
    try:
        import ocl
    except ImportError:
        import opencamlib as ocl
except ImportError:
    msg = translate("path_waterline", "This operation requires OpenCamLib to be installed.")
    FreeCAD.Console.PrintError(msg + "\n")
    raise ImportError

import Path
import Path.Op.Base as PathOp
import Path.Op.SurfaceSupport as PathSurfaceSupport
import PathScripts.PathUtils as PathUtils
import math
import time
from PySide.QtCore import QT_TRANSLATE_NOOP

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


class ObjectWaterline(PathOp.ObjectOp):
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

    @classmethod
    def propertyEnumerations(self, dataType="data"):
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
            "Algorithm": [
                (translate("path_waterline", "OCL Dropcutter"), "OCL Dropcutter"),
                (translate("path_waterline", "OCL Adaptive"), "OCL Adaptive"),
                (translate("path_waterline", "Experimental Z-Level Hybrid"), "Experimental"),
            ],
            "BoundBox": [
                (translate("path_waterline", "BaseBoundBox"), "BaseBoundBox"),
                (translate("path_waterline", "Stock"), "Stock"),
            ],
            "PatternCenterAt": [
                (translate("path_waterline", "CenterOfMass"), "CenterOfMass"),
                (translate("path_waterline", "CenterOfBoundBox"), "CenterOfBoundBox"),
                (translate("path_waterline", "XminYmin"), "XminYmin"),
                (translate("path_waterline", "Custom"), "Custom"),
            ],
            "CutMode": [
                (translate("path_waterline", "Conventional"), "Conventional"),
                (translate("path_waterline", "Climb"), "Climb"),
            ],
            "CutPattern": [
                (translate("path_waterline", "None"), "None"),
                (translate("path_waterline", "Circular"), "Circular"),
                (translate("path_waterline", "CircularZigZag"), "CircularZigZag"),
                (translate("path_waterline", "Line"), "Line"),
                (translate("path_waterline", "Offset"), "Offset"),
                (translate("path_waterline", "Spiral"), "Spiral"),
                (translate("path_waterline", "ZigZag"), "ZigZag"),
            ],
            "HandleMultipleFeatures": [
                (translate("path_waterline", "Collectively"), "Collectively"),
                (translate("path_waterline", "Individually"), "Individually"),
            ],
            "LayerMode": [
                (translate("path_waterline", "Single-pass"), "Single-pass"),
                (translate("path_waterline", "Multi-pass"), "Multi-pass"),
            ],
            "SamplingAccuracy": [
                (translate("path_waterline", "Standard"), "4"),
                (translate("path_waterline", "High"), "8"),
                (translate("path_waterline", "Very High"), "16"),
                (translate("path_waterline", "Ultra"), "32"),
                (translate("path_waterline", "Extreme"), "64"),
            ],
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
        self.addNewProps = list()

        for prtyp, nm, grp, tt in self.opPropertyDefinitions():
            if not hasattr(obj, nm):
                obj.addProperty(prtyp, nm, grp, tt)
                self.addNewProps.append(nm)

        # Set enumeration lists for enumeration properties
        if len(self.addNewProps) > 0:
            ENUMS = self.propertyEnumerations()
            for n in ENUMS:
                if n[0] in self.addNewProps:
                    setattr(obj, n[0], n[1])

            if warn:
                newPropMsg = translate("PathWaterline", "New property added to")
                newPropMsg += ' "{}": {}'.format(obj.Label, self.addNewProps) + ". "
                newPropMsg += translate("PathWaterline", "Check default value(s).")
                FreeCAD.Console.PrintWarning(newPropMsg + "\n")

        self.propertiesReady = True

    def opPropertyDefinitions(self):
        """opPropertyDefinitions() ... return list of tuples containing operation specific properties"""
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
                    "Smaller values yield a finer, more accurate the mesh. Smaller values increase processing time a lot.",
                ),
            ),
            (
                "App::PropertyDistance",
                "LinearDeflection",
                "Mesh Conversion",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Smaller values yield a finer, more accurate the mesh. Smaller values do not increase processing time much.",
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
                "Algorithm",
                "Clearing Options",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Select the algorithm to use: OCL Dropcutter*, OCL Adaptive or Experimental - Z-Level Hybrid.",
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
                "App::PropertyBool",
                "ClearPlanarOnly",
                "Clearing Options",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "If true, identifies horizontal surfaces and clears only them with the selected Cut Pattern.",
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
                "App::PropertyBool",
                "IgnoreOuter",
                "Clearing Options",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "If true, the operation ignores the outermost boundary and only machines internal perimeters (typically is used for the stock).",
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
                QT_TRANSLATE_NOOP("App::Property", "Set the start point for the cut pattern."),
            ),
            (
                "App::PropertyDistance",
                "StockToLeave",
                "Clearing Options",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "The amount of material to leave on the part in the XY plane. Useful for leaving room for a separate finishing pass.",
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
                "App::PropertyDistance",
                "SampleInterval",
                "Clearing Options",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Set the sampling resolution. Smaller values quickly increase processing time.",
                ),
            ),
            (
                "App::PropertyDistance",
                "MinSampleInterval",
                "Clearing Options",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Set the minimum sampling resolution. Smaller values quickly increase processing time.",
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
                "App::PropertyEnumeration",
                "SamplingAccuracy",
                "Clearing Options",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Number of samples used for 3D tool compensation. Only increase to higher settings if you encounter issues.",
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
                QT_TRANSLATE_NOOP("App::Property", "Make True, if specifying a Start Point"),
            ),
        ]

    def opPropertyDefaults(self, obj, job):
        """opPropertyDefaults(obj, job) ... returns a dictionary
        of default values for the operation's properties."""
        defaults = {
            "OptimizeLinearPaths": True,
            "InternalFeaturesCut": True,
            "OptimizeStepOverTransitions": False,
            "BoundaryEnforcement": True,
            "UseStartPoint": False,
            "AvoidLastX_InternalFeatures": True,
            "CutPatternReversed": False,
            "IgnoreOuter": False,
            "StartPoint": FreeCAD.Vector(0.0, 0.0, obj.ClearanceHeight.Value),
            "Algorithm": "OCL Dropcutter",
            "LayerMode": "Single-pass",
            "CutMode": "Conventional",
            "CutPattern": "None",
            "HandleMultipleFeatures": "Collectively",
            "PatternCenterAt": "CenterOfMass",
            "GapSizes": "No gaps identified.",
            "ClearPlanarOnly": False,
            "StepOver": 100.0,
            "CutPatternAngle": 0.0,
            "DepthOffset": 0.0,
            "SampleInterval": 1.0,
            "MinSampleInterval": 0.005,
            "BoundaryAdjustment": 0.0,
            "InternalFeaturesAdjustment": 0.0,
            "AvoidLastX_Faces": 0,
            "PatternCenterCustom": FreeCAD.Vector(0.0, 0.0, 0.0),
            "GapThreshold": 0.005,
            "AngularDeflection": 0.25,
            "LinearDeflection": 0.0001,
            "SamplingAccuracy": "4",
            "StockToLeave": 0.0,
            # For debugging
            "ShowTempObjects": False,
        }

        warn = True
        if hasattr(job, "GeometryTolerance"):
            if job.GeometryTolerance.Value != 0.0:
                warn = False
                defaults["LinearDeflection"] = job.GeometryTolerance.Value
        if warn:
            msg = translate("PathWaterline", "The GeometryTolerance for this Job is 0.0.")
            msg += translate("PathWaterline", "Initializing LinearDeflection to 0.0001 mm.")
            FreeCAD.Console.PrintWarning(msg + "\n")

        return defaults

    def setEditorProperties(self, obj):
        # Used to hide inputs in properties list (UI modes: 0 = show, 2 = hide)
        expMode = G = 0
        show = hide = A = B = C = D = E = 2

        # Default hidden properties for all algorithms
        obj.setEditorMode("BoundaryEnforcement", hide)
        obj.setEditorMode("InternalFeaturesAdjustment", hide)
        obj.setEditorMode("InternalFeaturesCut", hide)
        obj.setEditorMode("AvoidLastX_Faces", hide)
        obj.setEditorMode("AvoidLastX_InternalFeatures", hide)
        obj.setEditorMode("BoundaryAdjustment", hide)
        obj.setEditorMode("HandleMultipleFeatures", hide)
        obj.setEditorMode("OptimizeLinearPaths", hide)
        obj.setEditorMode("OptimizeStepOverTransitions", hide)
        obj.setEditorMode("GapThreshold", hide)
        obj.setEditorMode("GapSizes", hide)

        if obj.Algorithm == "OCL Dropcutter":
            pass
        elif obj.Algorithm == "OCL Adaptive":
            D = 0
            expMode = 2
        elif obj.Algorithm == "Experimental":
            # Default visibility for Experimental properties
            A = B = C = E = 0
            expMode = G = D = 2

            cutPattern = obj.CutPattern

            if cutPattern == "None":
                # If no pattern, hide clearing-specific settings
                A = B = E = 2
                show = 2
            elif cutPattern == "Offset":
                # Offset uses StepOver and Planar toggle, but not Angle
                show = 2
            elif cutPattern in ["Line", "ZigZag"]:
                show = 0  # Show Angle
            elif cutPattern in ["Circular", "CircularZigZag"]:
                show = 2
                hide = 0  # Show Pattern Center
            elif cutPattern == "Spiral":
                G = hide = 0  # Show Sample Interval and Pattern Center

        # Apply visibility states to properties
        obj.setEditorMode("CutPatternAngle", show)
        obj.setEditorMode("PatternCenterAt", hide)
        obj.setEditorMode("PatternCenterCustom", hide)
        obj.setEditorMode("CutPatternReversed", A)
        obj.setEditorMode("IgnoreOuter", C)
        obj.setEditorMode("BoundaryAdjustment", C)
        obj.setEditorMode("StepOver", B)
        obj.setEditorMode("ClearPlanarOnly", E)
        obj.setEditorMode("CutPattern", C)
        obj.setEditorMode("SamplingAccuracy", C)
        obj.setEditorMode("StockToLeave", C)
        obj.setEditorMode("MinSampleInterval", D)
        obj.setEditorMode("SampleInterval", G)
        obj.setEditorMode("LinearDeflection", expMode)
        obj.setEditorMode("AngularDeflection", expMode)

    def onChanged(self, obj, prop):
        if hasattr(self, "propertiesReady"):
            if self.propertiesReady:
                if prop in ["Algorithm", "CutPattern"]:
                    self.setEditorProperties(obj)

        if prop == "Active" and obj.ViewObject:
            obj.ViewObject.signalChangeIcon()

    def opOnDocumentRestored(self, obj):
        self.propertiesReady = False
        job = PathUtils.findParentJob(obj)

        self.initOpProperties(obj, warn=True)
        self.opApplyPropertyDefaults(obj, job, self.addNewProps)

        mode = 2 if Path.Log.getLevel(Path.Log.thisModule()) != 4 else 0
        obj.setEditorMode("ShowTempObjects", mode)

        # Repopulate enumerations in case of changes

        ENUMS = self.propertyEnumerations()
        for n in ENUMS:
            restore = False
            if hasattr(obj, n[0]):
                val = obj.getPropertyByName(n[0])
                restore = True
            setattr(obj, n[0], n[1])
            if restore:
                setattr(obj, n[0], val)

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
        # Limit sample interval
        if obj.SampleInterval.Value < 0.0001:
            obj.SampleInterval.Value = 0.0001
            Path.Log.error(
                translate(
                    "PathWaterline",
                    "Sample interval limits are 0.0001 to 25.4 millimeters.",
                )
            )
        if obj.SampleInterval.Value > 25.4:
            obj.SampleInterval.Value = 25.4
            Path.Log.error(
                translate(
                    "PathWaterline",
                    "Sample interval limits are 0.0001 to 25.4 millimeters.",
                )
            )

        # Limit min sample interval
        if obj.MinSampleInterval.Value < 0.0001:
            obj.MinSampleInterval.Value = 0.0001
            Path.Log.error(
                translate(
                    "PathWaterline",
                    "Min Sample interval limits are 0.0001 to 25.4 millimeters.",
                )
            )
        if obj.MinSampleInterval.Value > 25.4:
            obj.MinSampleInterval.Value = 25.4
            Path.Log.error(
                translate(
                    "PathWaterline",
                    "Min Sample interval limits are 0.0001 to 25.4 millimeters.",
                )
            )

        # Limit cut pattern angle
        if obj.CutPatternAngle < -360.0:
            obj.CutPatternAngle = 0.0
            Path.Log.error(
                translate("PathWaterline", "Cut pattern angle limits are +-360 degrees.")
            )
        if obj.CutPatternAngle >= 360.0:
            obj.CutPatternAngle = 0.0
            Path.Log.error(
                translate("PathWaterline", "Cut pattern angle limits are +- 360 degrees.")
            )

        # Limit StepOver to natural number percentage
        if obj.StepOver > 100.0:
            obj.StepOver = 100.0
        if obj.StepOver < 1.0:
            obj.StepOver = 1.0

        # Limit StockToLeave to zero and positive values
        if obj.StockToLeave < 0:
            obj.StockToLeave = 0
            Path.Log.error(
                translate(
                    "PathWaterline",
                    "StockToLEave: Only zero or positive values permitted.",
                )
            )
        if obj.StockToLeave > 100:
            obj.StockToLeave = 100
            Path.Log.error(
                translate(
                    "PathWaterline",
                    "StockToLEave: Stock to leave count limited to 100.",
                )
            )

        # Limit AvoidLastX_Faces to zero and positive values
        if obj.AvoidLastX_Faces < 0:
            obj.AvoidLastX_Faces = 0
            Path.Log.error(
                translate(
                    "PathWaterline",
                    "AvoidLastX_Faces: Only zero or positive values permitted.",
                )
            )
        if obj.AvoidLastX_Faces > 100:
            obj.AvoidLastX_Faces = 100
            Path.Log.error(
                translate(
                    "PathWaterline",
                    "AvoidLastX_Faces: Avoid last X faces count limited to 100.",
                )
            )

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

        self.modelSTLs = list()
        self.safeSTLs = list()
        self.modelTypes = list()
        self.boundBoxes = list()
        self.profileShapes = list()
        self.collectiveShapes = list()
        self.individualShapes = list()
        self.avoidShapes = list()
        self.geoTlrnc = None
        self.tempGroup = None
        self.CutClimb = False
        self.closedGap = False
        self.tmpCOM = None
        self.gaps = [0.1, 0.2, 0.3]
        CMDS = list()
        modelVisibility = list()
        FCAD = FreeCAD.ActiveDocument

        try:
            dotIdx = __name__.index(".") + 1
        except Exception:
            dotIdx = 0
        self.module = __name__[dotIdx:]

        # make circle for workplane
        self.wpc = Part.makeCircle(2.0)

        # Set debugging behavior
        self.showDebugObjects = False  # Set to true if you want a visual DocObjects created for some path construction objects
        self.showDebugObjects = obj.ShowTempObjects
        deleteTempsFlag = True  # Set to False for debugging
        if Path.Log.getLevel(Path.Log.thisModule()) == 4:
            deleteTempsFlag = False
        else:
            self.showDebugObjects = False

        # mark beginning of operation and identify parent Job
        Path.Log.info("\nBegin Waterline operation...")
        startTime = time.time()

        # Identify parent Job
        JOB = PathUtils.findParentJob(obj)
        if JOB is None:
            Path.Log.error(translate("PathWaterline", "No JOB"))
            return
        self.stockZMin = JOB.Stock.Shape.BoundBox.ZMin

        # set cut mode; reverse as needed
        if obj.CutMode == "Climb":
            self.CutClimb = True
        if obj.CutPatternReversed is True:
            if self.CutClimb is True:
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
                    "PathWaterline",
                    "Canceling Waterline operation. Error creating OCL cutter.",
                )
            )
            return
        self.toolDiam = self.cutter.getDiameter()
        self.radius = self.toolDiam / 2.0
        self.cutOut = self.toolDiam * (float(obj.StepOver) / 100.0)
        self.gaps = [self.toolDiam, self.toolDiam, self.toolDiam]

        # Begin GCode for operation with basic information
        # ... and move cutter to clearance height and startpoint
        output = ""
        if obj.Comment != "":
            self.commandlist.append(Path.Command("N ({})".format(str(obj.Comment)), {}))
        self.commandlist.append(Path.Command("N ({})".format(obj.Label), {}))
        self.commandlist.append(Path.Command("N (Tool type: {})".format(oclTool.toolType), {}))
        self.commandlist.append(
            Path.Command("N (Compensated Tool Path. Diameter: {})".format(oclTool.diameter), {})
        )
        self.commandlist.append(
            Path.Command("N (Sample interval: {})".format(str(obj.SampleInterval.Value)), {})
        )
        self.commandlist.append(Path.Command("N (Step over %: {})".format(str(obj.StepOver)), {}))
        self.commandlist.append(Path.Command("N ({})".format(output), {}))
        self.commandlist.append(
            Path.Command("G0", {"Z": obj.ClearanceHeight.Value, "F": self.vertRapid})
        )
        if obj.UseStartPoint:
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
        tempGroupName = "tempPathWaterlineGroup"
        if FCAD.getObject(tempGroupName):
            for to in FCAD.getObject(tempGroupName).Group:
                FCAD.removeObject(to.Name)
            FCAD.removeObject(tempGroupName)  # remove temp directory if already exists
        if FCAD.getObject(tempGroupName + "001"):
            for to in FCAD.getObject(tempGroupName + "001").Group:
                FCAD.removeObject(to.Name)
            FCAD.removeObject(tempGroupName + "001")  # remove temp directory if already exists
        tempGroup = FCAD.addObject("App::DocumentObjectGroup", tempGroupName)
        tempGroupName = tempGroup.Name
        self.tempGroup = tempGroup
        tempGroup.purgeTouched()
        # Add temp object to temp group folder with following code:
        # ... self.tempGroup.addObject(OBJ)

        # Get height offset values for later use
        self.SafeHeightOffset = JOB.SetupSheet.SafeHeightOffset.Value
        self.ClearHeightOffset = JOB.SetupSheet.ClearanceHeightOffset.Value

        # Set deflection values for mesh generation
        useDGT = False
        try:  # try/except is for Path Jobs created before GeometryTolerance
            self.geoTlrnc = JOB.GeometryTolerance.Value
            if self.geoTlrnc == 0.0:
                useDGT = True
        except AttributeError as ee:
            Path.Log.warning(
                "{}\nPlease set Job.GeometryTolerance to an acceptable value. Using Path.Preferences.defaultGeometryTolerance().".format(
                    ee
                )
            )
            useDGT = True
        if useDGT:
            self.geoTlrnc = Path.Preferences.defaultGeometryTolerance()

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
            for m in range(0, len(JOB.Model.Group)):
                mNm = JOB.Model.Group[m].Name
                modelVisibility.append(FreeCADGui.ActiveDocument.getObject(mNm).Visibility)

        # Setup STL, model type, and bound box containers for each model in Job
        for m in range(0, len(JOB.Model.Group)):
            M = JOB.Model.Group[m]
            self.modelSTLs.append(False)
            self.safeSTLs.append(False)
            self.profileShapes.append(False)
            # Set bound box
            if obj.BoundBox == "BaseBoundBox":
                if M.TypeId.startswith("Mesh"):
                    self.modelTypes.append("M")  # Mesh
                    self.boundBoxes.append(M.Mesh.BoundBox)
                else:
                    self.modelTypes.append("S")  # Solid
                    self.boundBoxes.append(M.Shape.BoundBox)
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
        if pPM is False:
            Path.Log.error("Unable to pre-process obj.Base.")
        else:
            (FACES, VOIDS) = pPM
            self.modelSTLs = PSF.modelSTLs
            self.profileShapes = PSF.profileShapes

            for m in range(0, len(JOB.Model.Group)):
                # Create OCL.stl model objects
                if obj.Algorithm == "OCL Dropcutter" or obj.Algorithm == "OCL Adaptive":
                    PathSurfaceSupport._prepareModelSTLs(self, JOB, obj, m, ocl)

                Mdl = JOB.Model.Group[m]
                if FACES[m] is False:
                    Path.Log.error("No data for model base: {}".format(JOB.Model.Group[m].Label))
                else:
                    if m > 0:
                        # Raise to clearance between models
                        CMDS.append(Path.Command("N (Transition to base: {}.)".format(Mdl.Label)))
                        CMDS.append(
                            Path.Command(
                                "G0",
                                {"Z": obj.ClearanceHeight.Value, "F": self.vertRapid},
                            )
                        )
                        Path.Log.info("Working on Model.Group[{}]: {}".format(m, Mdl.Label))
                    # make stock-model-voidShapes STL model for avoidance detection on transitions
                    if obj.Algorithm == "OCL Dropcutter" or obj.Algorithm == "OCL Adaptive":
                        PathSurfaceSupport._makeSafeSTL(self, JOB, obj, m, FACES[m], VOIDS[m], ocl)
                    # Process model/faces - OCL objects must be ready
                    CMDS.extend(self._processWaterlineAreas(JOB, obj, m, FACES[m], VOIDS[m]))

            # Save gcode produced
            self.commandlist.extend(CMDS)

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
        gaps = list()
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
        msg = translate("PathWaterline", "operation time is")
        Path.Log.info("Waterline " + msg + " {} sec.".format(execTime))

        # IMPORTANT: This prevents the 'OK' button double-recompute bug
        obj.purgeTouched()

        return True

    # Methods for constructing the cut area and creating path geometry
    def _processWaterlineAreas(self, JOB, obj, mdlIdx, FCS, VDS):
        """_processWaterlineAreas(JOB, obj, mdlIdx, FCS, VDS)...
        This method applies any avoided faces or regions to the selected faces.
        It then calls the correct method."""
        Path.Log.debug("_processWaterlineAreas()")

        final = list()

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

            final.append(Path.Command("G0", {"Z": obj.SafeHeight.Value, "F": self.vertRapid}))
            if obj.Algorithm == "OCL Dropcutter" or obj.Algorithm == "OCL Adaptive":
                final.extend(
                    self._oclWaterlineOp(JOB, obj, mdlIdx, COMP)
                )  # independent method set for Waterline
            else:
                final.extend(
                    self._experimentalWaterlineOp(JOB, obj, mdlIdx, COMP)
                )  # independent method set for Waterline

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

                final.append(Path.Command("G0", {"Z": obj.SafeHeight.Value, "F": self.vertRapid}))
                if obj.Algorithm == "OCL Dropcutter" or obj.Algorithm == "OCL Adaptive":
                    final.extend(
                        self._oclWaterlineOp(JOB, obj, mdlIdx, COMP)
                    )  # independent method set for Waterline
                else:
                    final.extend(
                        self._experimentalWaterlineOp(JOB, obj, mdlIdx, COMP)
                    )  # independent method set for Waterline
                COMP = None
        # Eif

        return final

    # Main planar scan functions
    def _stepTransitionCmds(self, obj, cutPattern, lstPnt, first, minSTH, tolrnc):
        cmds = list()
        rtpd = False
        horizGC = "G0"
        hSpeed = self.horizRapid
        height = obj.SafeHeight.Value

        if cutPattern in ["Line", "Circular", "Spiral"]:
            if obj.OptimizeStepOverTransitions is True:
                height = minSTH + 2.0
        elif cutPattern in ["ZigZag", "CircularZigZag"]:
            if obj.OptimizeStepOverTransitions is True:
                zChng = first.z - lstPnt.z
                if abs(zChng) < tolrnc:  # transitions to same Z height
                    if (minSTH - first.z) > tolrnc:
                        height = minSTH + 2.0
                    else:
                        horizGC = "G1"
                        height = first.z
                elif (minSTH + (2.0 * tolrnc)) >= max(first.z, lstPnt.z):
                    height = False  # allow end of Zig to cut to beginning of Zag

        # Create raise, shift, and optional lower commands
        if height is not False:
            cmds.append(Path.Command("G0", {"Z": height, "F": self.vertRapid}))
            cmds.append(Path.Command(horizGC, {"X": first.x, "Y": first.y, "F": hSpeed}))
        if rtpd is not False:  # ReturnToPreviousDepth
            cmds.append(Path.Command("G0", {"Z": rtpd, "F": self.vertRapid}))

        return cmds

    def _breakCmds(self, obj, cutPattern, lstPnt, first, minSTH, tolrnc):
        cmds = list()
        rtpd = False
        horizGC = "G0"
        hSpeed = self.horizRapid
        height = obj.SafeHeight.Value

        if cutPattern in ["Line", "Circular", "Spiral"]:
            if obj.OptimizeStepOverTransitions is True:
                height = minSTH + 2.0
        elif cutPattern in ["ZigZag", "CircularZigZag"]:
            if obj.OptimizeStepOverTransitions is True:
                zChng = first.z - lstPnt.z
                if abs(zChng) < tolrnc:  # transitions to same Z height
                    if (minSTH - first.z) > tolrnc:
                        height = minSTH + 2.0
                    else:
                        height = first.z + 2.0  # first.z

        cmds.append(Path.Command("G0", {"Z": height, "F": self.vertRapid}))
        cmds.append(Path.Command(horizGC, {"X": first.x, "Y": first.y, "F": hSpeed}))
        if rtpd is not False:  # ReturnToPreviousDepth
            cmds.append(Path.Command("G0", {"Z": rtpd, "F": self.vertRapid}))

        return cmds

    def _planarGetPDC(self, stl, finalDep, SampleInterval, cutter):
        pdc = ocl.PathDropCutter()  # create a pdc [PathDropCutter] object
        pdc.setSTL(stl)  # add stl model
        pdc.setCutter(cutter)  # add cutter
        pdc.setZ(finalDep)  # set minimumZ (final / target depth value)
        pdc.setSampling(SampleInterval)  # set sampling size
        return pdc

    # OCL Dropcutter - OCL Adaptive waterline functions
    def _oclWaterlineOp(self, JOB, obj, mdlIdx, subShp=None):
        """_oclWaterlineOp(obj, base) ... Main waterline function to perform waterline extraction from model."""
        commands = []

        base = JOB.Model.Group[mdlIdx]
        bb = self.boundBoxes[mdlIdx]
        stl = self.modelSTLs[mdlIdx]
        depOfst = obj.DepthOffset.Value

        # Prepare global holdpoint and layerEndPnt containers
        if self.holdPoint is None:
            self.holdPoint = FreeCAD.Vector(0.0, 0.0, 0.0)
        if self.layerEndPnt is None:
            self.layerEndPnt = FreeCAD.Vector(0.0, 0.0, 0.0)

        smplInt = obj.SampleInterval.Value
        minSmplInt = obj.MinSampleInterval.Value
        if minSmplInt > smplInt:
            minSmplInt = smplInt

        # Compute number and size of stepdowns, and final depth
        if obj.LayerMode == "Single-pass":
            depthparams = [obj.FinalDepth.Value]
        else:
            depthparams = [dp for dp in self.depthParams]
        lenDP = len(depthparams)

        # Scan the piece to depth at smplInt
        if obj.Algorithm == "OCL Adaptive":
            # Get Stock Bounding Box
            BS = JOB.Stock
            stock_bb = BS.Shape.BoundBox

            # Stock Limits
            s_xmin = stock_bb.XMin
            s_xmax = stock_bb.XMax
            s_ymin = stock_bb.YMin
            s_ymax = stock_bb.YMax

            # Calculate Tool Path Limits based on OCL STL
            path_min_x = stl.bb.minpt.x - self.radius
            path_min_y = stl.bb.minpt.y - self.radius
            path_max_x = stl.bb.maxpt.x + self.radius
            path_max_y = stl.bb.maxpt.y + self.radius

            # Compare with a tiny tolerance
            tol = 0.001
            if (
                (path_min_x < s_xmin - tol)
                or (path_min_y < s_ymin - tol)
                or (path_max_x > s_xmax + tol)
                or (path_max_y > s_ymax + tol)
            ):

                newPropMsg = translate(
                    "PathWaterline",
                    "The toolpath has exceeded the stock bounding box limits. Consider using a Boundary Dressup.",
                )
                FreeCAD.Console.PrintWarning(newPropMsg + "\n")

            # Run the Scan (Processing ALL depths at once)
            scanLines = self._waterlineAdaptiveScan(stl, smplInt, minSmplInt, depthparams, depOfst)

            # Generate G-Code
            layTime = time.time()
            for loop in scanLines:
                # We pass '0.0' as layDep because Adaptive loops have their own Z embedded
                cmds = self._loopToGcode(obj, 0.0, loop)
                commands.extend(cmds)

            Path.Log.debug("--Adaptive generation took " + str(time.time() - layTime) + " s")

        else:
            # Setup BoundBox for Dropcutter grid
            if subShp is None:
                # Get correct boundbox
                if obj.BoundBox == "Stock":
                    BS = JOB.Stock
                    bb = BS.Shape.BoundBox
                elif obj.BoundBox == "BaseBoundBox":
                    BS = base
                    bb = BS.Shape.BoundBox

                xmin = bb.XMin
                xmax = bb.XMax
                ymin = bb.YMin
                ymax = bb.YMax
            else:
                xmin = subShp.BoundBox.XMin
                xmax = subShp.BoundBox.XMax
                ymin = subShp.BoundBox.YMin
                ymax = subShp.BoundBox.YMax

            # Determine bounding box length for the OCL scan
            bbLength = math.fabs(ymax - ymin)
            numScanLines = int(math.ceil(bbLength / smplInt) + 1)

            # Run Scan (Grid  based)
            fd = depthparams[-1]
            oclScan = self._waterlineDropCutScan(stl, smplInt, xmin, xmax, ymin, fd, numScanLines)
            oclScan = [FreeCAD.Vector(P.x, P.y, P.z + depOfst) for P in oclScan]

            # Convert point list to grid (scanLines)
            lenOS = len(oclScan)
            ptPrLn = int(lenOS / numScanLines)
            scanLines = []
            for L in range(0, numScanLines):
                scanLines.append([])
                for P in range(0, ptPrLn):
                    pi = L * ptPrLn + P
                    scanLines[L].append(oclScan[pi])

            # Extract Waterline Layers Iteratively
            lenSL = len(scanLines)
            pntsPerLine = len(scanLines[0])
            msg = "--OCL scan: " + str(lenSL * pntsPerLine) + " points, with "
            msg += str(numScanLines) + " lines and " + str(pntsPerLine) + " pts/line"
            Path.Log.debug(msg)

            lyr = 0
            cmds = []
            layTime = time.time()
            self.topoMap = []
            for layDep in depthparams:
                cmds = self._getWaterline(obj, scanLines, layDep, lyr, lenSL, pntsPerLine)
                commands.extend(cmds)
                lyr += 1
            Path.Log.debug("--All layer scans combined took " + str(time.time() - layTime) + " s")

        return commands

    def _waterlineDropCutScan(self, stl, smplInt, xmin, xmax, ymin, fd, numScanLines):
        """_waterlineDropCutScan(stl, smplInt, xmin, xmax, ymin, fd, numScanLines) ...
        Perform OCL scan for waterline purpose."""
        pdc = ocl.PathDropCutter()  # create a pdc
        pdc.setSTL(stl)
        pdc.setCutter(self.cutter)
        pdc.setZ(fd)  # set minimumZ (final / target depth value)
        pdc.setSampling(smplInt)

        # Create line object as path
        path = ocl.Path()  # create an empty path object
        for nSL in range(0, numScanLines):
            yVal = ymin + (nSL * smplInt)
            p1 = ocl.Point(xmin, yVal, fd)  # start-point of line
            p2 = ocl.Point(xmax, yVal, fd)  # end-point of line
            path.append(ocl.Line(p1, p2))
            # path.append(l)        # add the line to the path
        pdc.setPath(path)
        pdc.run()  # run drop-cutter on the path

        # return the list of points
        return pdc.getCLPoints()

    def _waterlineAdaptiveScan(self, stl, smplInt, minSmplInt, zheights, depOfst):
        """Perform OCL Adaptive scan for waterline purpose."""

        msg = translate(
            "Waterline", ": Steps below the model's top Face will be the only ones processed."
        )
        Path.Log.info("Waterline " + msg)

        # Setup OCL AdaptiveWaterline
        awl = ocl.AdaptiveWaterline()
        awl.setSTL(stl)
        awl.setCutter(self.cutter)
        awl.setSampling(smplInt)
        awl.setMinSampling(minSmplInt)

        adapt_loops = []

        # Iterate through each Z-depth
        for zh in zheights:
            awl.setZ(zh)
            awl.run()

            # OCL returns a list of separate loops (list of lists of Points)
            # Example: [[PerimeterPoints], [HolePoints]]
            temp_loops = awl.getLoops()

            if not temp_loops:
                # Warn if the step is outside the model bounds
                newPropMsg = translate("PathWaterline", "Step Down above model. Skipping height : ")
                newPropMsg += "{} mm".format(zh)
                FreeCAD.Console.PrintWarning(newPropMsg + "\n")
                continue

            # Process each loop separately.
            # This ensures that islands (holes) remain distinct from perimeters.
            for loop in temp_loops:
                # Convert OCL Points to FreeCAD Vectors and apply Z offset
                fc_loop = [FreeCAD.Vector(P.x, P.y, P.z + depOfst) for P in loop]
                adapt_loops.append(fc_loop)

        return adapt_loops

    def _getWaterline(self, obj, scanLines, layDep, lyr, lenSL, pntsPerLine):
        """_getWaterline(obj, scanLines, layDep, lyr, lenSL, pntsPerLine) ... Get waterline."""
        commands = []
        cmds = []
        loopList = []
        self.topoMap = []
        if obj.Algorithm == "OCL Adaptive":
            loopList = scanLines
        else:
            # Create topo map from scanLines (highs and lows)
            self.topoMap = self._createTopoMap(scanLines, layDep, lenSL, pntsPerLine)
            # Add buffer lines and columns to topo map
            self._bufferTopoMap(lenSL, pntsPerLine)
            # Identify layer waterline from OCL scan
            self._highlightWaterline(4, 9)
            # Extract waterline and convert to gcode
            loopList = self._extractWaterlines(obj, scanLines, lyr, layDep)

        # save commands
        for loop in loopList:
            cmds = self._loopToGcode(obj, layDep, loop)
            commands.extend(cmds)
        return commands

    def _createTopoMap(self, scanLines, layDep, lenSL, pntsPerLine):
        """_createTopoMap(scanLines, layDep, lenSL, pntsPerLine) ... Create topo map version of OCL scan data."""
        topoMap = []
        for L in range(0, lenSL):
            topoMap.append([])
            for P in range(0, pntsPerLine):
                if scanLines[L][P].z > layDep:
                    topoMap[L].append(2)
                else:
                    topoMap[L].append(0)
        return topoMap

    def _bufferTopoMap(self, lenSL, pntsPerLine):
        """_bufferTopoMap(lenSL, pntsPerLine) ... Add buffer boarder of zeros to all sides to topoMap data."""
        pre = [0, 0]
        post = [0, 0]
        for p in range(0, pntsPerLine):
            pre.append(0)
            post.append(0)
        for i in range(0, lenSL):
            self.topoMap[i].insert(0, 0)
            self.topoMap[i].append(0)
        self.topoMap.insert(0, pre)
        self.topoMap.append(post)
        return True

    def _highlightWaterline(self, extraMaterial, insCorn):
        """_highlightWaterline(extraMaterial, insCorn) ... Highlight the waterline data, separating from extra material."""
        TM = self.topoMap
        lastPnt = len(TM[1]) - 1
        lastLn = len(TM) - 1
        highFlag = 0

        # ("--Convert parallel data to ridges")
        for lin in range(1, lastLn):
            for pt in range(1, lastPnt):  # Ignore first and last points
                if TM[lin][pt] == 0:
                    if TM[lin][pt + 1] == 2:  # step up
                        TM[lin][pt] = 1
                    if TM[lin][pt - 1] == 2:  # step down
                        TM[lin][pt] = 1

        # ("--Convert perpendicular data to ridges and highlight ridges")
        for pt in range(1, lastPnt):  # Ignore first and last points
            for lin in range(1, lastLn):
                if TM[lin][pt] == 0:
                    highFlag = 0
                    if TM[lin + 1][pt] == 2:  # step up
                        TM[lin][pt] = 1
                    if TM[lin - 1][pt] == 2:  # step down
                        TM[lin][pt] = 1
                elif TM[lin][pt] == 2:
                    highFlag += 1
                    if highFlag == 3:
                        if TM[lin - 1][pt - 1] < 2 or TM[lin - 1][pt + 1] < 2:
                            highFlag = 2
                        else:
                            TM[lin - 1][pt] = extraMaterial
                            highFlag = 2

        # ("--Square corners")
        for pt in range(1, lastPnt):
            for lin in range(1, lastLn):
                if TM[lin][pt] == 1:  # point == 1
                    cont = True
                    if TM[lin + 1][pt] == 0:  # forward == 0
                        if TM[lin + 1][pt - 1] == 1:  # forward left == 1
                            if TM[lin][pt - 1] == 2:  # left == 2
                                TM[lin + 1][pt] = 1  # square the corner
                                cont = False

                        if cont is True and TM[lin + 1][pt + 1] == 1:  # forward right == 1
                            if TM[lin][pt + 1] == 2:  # right == 2
                                TM[lin + 1][pt] = 1  # square the corner
                        cont = True

                    if TM[lin - 1][pt] == 0:  # back == 0
                        if TM[lin - 1][pt - 1] == 1:  # back left == 1
                            if TM[lin][pt - 1] == 2:  # left == 2
                                TM[lin - 1][pt] = 1  # square the corner
                                cont = False

                        if cont is True and TM[lin - 1][pt + 1] == 1:  # back right == 1
                            if TM[lin][pt + 1] == 2:  # right == 2
                                TM[lin - 1][pt] = 1  # square the corner

        # remove inside corners
        for pt in range(1, lastPnt):
            for lin in range(1, lastLn):
                if TM[lin][pt] == 1:  # point == 1
                    if TM[lin][pt + 1] == 1:
                        if TM[lin - 1][pt + 1] == 1 or TM[lin + 1][pt + 1] == 1:
                            TM[lin][pt + 1] = insCorn
                    elif TM[lin][pt - 1] == 1:
                        if TM[lin - 1][pt - 1] == 1 or TM[lin + 1][pt - 1] == 1:
                            TM[lin][pt - 1] = insCorn

        return True

    def _extractWaterlines(self, obj, oclScan, lyr, layDep):
        """_extractWaterlines(obj, oclScan, lyr, layDep) ... Extract water lines from OCL scan data."""
        srch = True
        lastPnt = len(self.topoMap[0]) - 1
        lastLn = len(self.topoMap) - 1
        maxSrchs = 5
        srchCnt = 1
        loopList = []
        loop = []
        loopNum = 0

        if self.CutClimb is True:
            lC = [
                -1,
                -1,
                -1,
                0,
                1,
                1,
                1,
                0,
                -1,
                -1,
                -1,
                0,
                1,
                1,
                1,
                0,
                -1,
                -1,
                -1,
                0,
                1,
                1,
                1,
                0,
            ]
            pC = [
                -1,
                0,
                1,
                1,
                1,
                0,
                -1,
                -1,
                -1,
                0,
                1,
                1,
                1,
                0,
                -1,
                -1,
                -1,
                0,
                1,
                1,
                1,
                0,
                -1,
                -1,
            ]
        else:
            lC = [
                1,
                1,
                1,
                0,
                -1,
                -1,
                -1,
                0,
                1,
                1,
                1,
                0,
                -1,
                -1,
                -1,
                0,
                1,
                1,
                1,
                0,
                -1,
                -1,
                -1,
                0,
            ]
            pC = [
                -1,
                0,
                1,
                1,
                1,
                0,
                -1,
                -1,
                -1,
                0,
                1,
                1,
                1,
                0,
                -1,
                -1,
                -1,
                0,
                1,
                1,
                1,
                0,
                -1,
                -1,
            ]

        while srch is True:
            srch = False
            if srchCnt > maxSrchs:
                Path.Log.debug(
                    "Max search scans, "
                    + str(maxSrchs)
                    + " reached\nPossible incomplete waterline result!"
                )
                break
            for L in range(1, lastLn):
                for P in range(1, lastPnt):
                    if self.topoMap[L][P] == 1:
                        # start loop follow
                        srch = True
                        loopNum += 1
                        loop = self._trackLoop(oclScan, lC, pC, L, P, loopNum)
                        self.topoMap[L][P] = 0  # Mute the starting point
                        loopList.append(loop)
            srchCnt += 1
        Path.Log.debug(
            "Search count for layer "
            + str(lyr)
            + " is "
            + str(srchCnt)
            + ", with "
            + str(loopNum)
            + " loops."
        )
        return loopList

    def _trackLoop(self, oclScan, lC, pC, L, P, loopNum):
        """_trackLoop(oclScan, lC, pC, L, P, loopNum) ... Track the loop direction."""
        loop = [oclScan[L - 1][P - 1]]  # Start loop point list
        cur = [L, P, 1]
        prv = [L, P - 1, 1]
        nxt = [L, P + 1, 1]
        follow = True
        ptc = 0
        ptLmt = 200000
        while follow is True:
            ptc += 1
            if ptc > ptLmt:
                Path.Log.debug(
                    "Loop number "
                    + str(loopNum)
                    + " at ["
                    + str(nxt[0])
                    + ", "
                    + str(nxt[1])
                    + "] pnt count exceeds, "
                    + str(ptLmt)
                    + ".  Stopped following loop."
                )
                break
            nxt = self._findNextWlPoint(lC, pC, cur[0], cur[1], prv[0], prv[1])  # get next point
            loop.append(oclScan[nxt[0] - 1][nxt[1] - 1])  # add it to loop point list
            self.topoMap[nxt[0]][nxt[1]] = nxt[2]  # Mute the point, if not Y stem
            if nxt[0] == L and nxt[1] == P:  # check if loop complete
                follow = False
            elif nxt[0] == cur[0] and nxt[1] == cur[1]:  # check if line cannot be detected
                follow = False
            prv = cur
            cur = nxt
        return loop

    def _findNextWlPoint(self, lC, pC, cl, cp, pl, pp):
        """_findNextWlPoint(lC, pC, cl, cp, pl, pp) ...
        Find the next waterline point in the point cloud layer provided."""
        dl = cl - pl
        dp = cp - pp
        num = 0
        i = 3
        s = 0
        mtch = 0
        found = False
        while mtch < 8:  # check all 8 points around current point
            if lC[i] == dl:
                if pC[i] == dp:
                    s = i - 3
                    found = True
                    # Check for y branch where current point is connection between branches
                    for y in range(1, mtch):
                        if lC[i + y] == dl:
                            if pC[i + y] == dp:
                                num = 1
                                break
                    break
            i += 1
            mtch += 1
        if found is False:
            # ("_findNext: No start point found.")
            return [cl, cp, num]

        for r in range(0, 8):
            l = cl + lC[s + r]
            p = cp + pC[s + r]
            if self.topoMap[l][p] == 1:
                return [l, p, num]

        # ("_findNext: No next pnt found")
        return [cl, cp, num]

    def _loopToGcode(self, obj, layDep, loop):
        """_loopToGcode(obj, layDep, loop) ... Convert set of loop points to Gcode."""
        # generate the path commands
        output = []

        # Safety check for empty loops
        if not loop:
            return output

        nxt = FreeCAD.Vector(0.0, 0.0, 0.0)

        # Create (first and last) point
        if obj.Algorithm == "OCL Adaptive":
            if obj.CutMode == "Climb":
                # Reverse loop for Climb Milling
                loop.reverse()
            pnt = pnt1 = FreeCAD.Vector(loop[0].x, loop[0].y, loop[0].z)
        else:
            pnt = FreeCAD.Vector(loop[0].x, loop[0].y, layDep)

        # Position cutter to begin loop
        if self.layerEndPnt.x == 0 and self.layerEndPnt.y == 0:  # First to Clearance Height
            output.append(Path.Command("G0", {"Z": obj.ClearanceHeight.Value, "F": self.vertRapid}))
        else:
            output.append(Path.Command("G0", {"Z": obj.SafeHeight.Value, "F": self.vertRapid}))

        output.append(Path.Command("G0", {"X": pnt.x, "Y": pnt.y, "F": self.horizRapid}))
        output.append(Path.Command("G1", {"Z": pnt.z, "F": self.vertFeed}))

        lenCLP = len(loop)
        lastIdx = lenCLP - 1
        # Cycle through each point on loop
        for i in range(0, lenCLP):
            if i < lastIdx:
                nxt.x = loop[i + 1].x
                nxt.y = loop[i + 1].y
                if obj.Algorithm == "OCL Adaptive":
                    nxt.z = loop[i + 1].z
                else:
                    nxt.z = layDep
            output.append(Path.Command("G1", {"X": pnt.x, "Y": pnt.y, "F": self.horizFeed}))

            # Rotate point data
            pnt = nxt

        # Connect first and last points for Adaptive
        if obj.Algorithm == "OCL Adaptive":
            output.append(Path.Command("G1", {"X": pnt1.x, "Y": pnt1.y, "F": self.horizFeed}))

        # Save layer end point for use in transitioning to next layer
        self.layerEndPnt = pnt

        return output

    # Experimental waterline functions
    def _experimentalWaterlineOp(self, JOB, obj, mdlIdx, subShp=None):
        """_waterlineOp(JOB, obj, mdlIdx, subShp=None) ...
        Main waterline function to perform waterline extraction from model."""
        Path.Log.debug("_experimentalWaterlineOp()")

        commands = []
        base = JOB.Model.Group[mdlIdx]
        radius = self.radius
        stock_to_leave = obj.StockToLeave.Value
        adj = obj.BoundaryAdjustment.Value - radius - 0.01  # Minus a small buffer
        self.endVector = None
        bbFace = None
        self.sampAccuracy = int(obj.SamplingAccuracy)

        # Identify Tool Type
        tool_params = self._getToolParams(obj)
        if tool_params is None:
            error_msg = translate(
                "PathWaterline",
                "Operation failed: A Tool Type has been selected that is not supported by Experimental Algorithm.",
            )
            FreeCAD.Console.PrintError(error_msg + "\n")
            Path.Log.error("_experimentalWaterlineOp: _getToolParams returned None.")
            return []
        Path.Log.info("Tool Profile detected: {}".format(tool_params["profile"]))

        # Create a copy of the base shape
        shape = base.Shape.copy()
        # Clean up redundant edges/faces and get outer hull
        try:
            shape = shape.removeSplitter()
        except:
            pass
        # Get Outer Hull
        shape = self._getOuterHull(shape)

        finDep = obj.FinalDepth.Value + (self.geoTlrnc / 10.0)
        depthParams = PathUtils.depth_params(
            obj.ClearanceHeight.Value,
            obj.SafeHeight.Value,
            obj.StartDepth.Value,
            obj.StepDown.Value,
            0.0,
            finDep,
        )

        # Compute number and size of stepdowns, and final depth
        if obj.LayerMode == "Single-pass":
            depthparams = [finDep]
        else:
            depthparams = [dp for dp in depthParams]
        Path.Log.debug("Experimental Waterline depthparams:\n{}".format(depthparams))

        buffer = self.cutter.getDiameter() * 5.0
        borderFace = Part.Face(self._makeExtendedBoundBox(JOB.Stock.Shape.BoundBox, buffer, 0.0))

        # Use this engine to perform the Cut between borderFace and bbFace
        border_engine = Path.Area()
        border_engine.setPlane(self.wpc)
        border_engine.add(borderFace)

        # Get boundbox envelope
        envelop = None
        if obj.BoundBox == "Stock":
            envelop = PathSurfaceSupport.getShapeEnvelope(JOB.Stock.Shape)
        elif obj.BoundBox == "BaseBoundBox":
            envelop = PathSurfaceSupport.getShapeEnvelope(shape)

        # Apply BoundaryAdjustment
        bbFace = None
        if envelop:
            bbFace = self._getBoundaryAdj(envelop, adj)
        if bbFace:
            border_engine.add(bbFace, op=1)

        # Extract the final trimFace
        trimFace = border_engine.getShape()
        if hasattr(trimFace, "removeSplitter"):
            trimFace = trimFace.removeSplitter()

        self.showDebugObject(trimFace, "TrimFace_Clipper")

        # Cycle through layer depths
        (CUTAREAS, LAYER_METADATA) = self._getCutAreas(
            shape, depthparams, borderFace, tool_params, stock_to_leave
        )

        if not CUTAREAS:
            Path.Log.error("No cross-section cut areas identified.")
            return commands

        caCnt = 0
        caLen = len(CUTAREAS)
        indicator = FreeCAD.Base.ProgressIndicator()
        indicator.start("Experimental Z-Level Hybrid: Generating G-Code...", caLen)
        for ca in range(0, caLen):
            area = CUTAREAS[ca]
            meta = LAYER_METADATA[ca]
            status = meta["status"]
            footprint = meta["footprint"]

            csHght = area.BoundBox.ZMin + obj.DepthOffset.Value
            cont = False
            caCnt += 1

            if area.Area > 0.0:
                cont = True
                self.showDebugObject(area, "CutArea_{}".format(caCnt))
            else:
                indicator.next()
                data = FreeCAD.Units.Quantity(csHght, FreeCAD.Units.Length).UserString
                Path.Log.debug("Cut area at {} is zero.".format(data))

            # get offset wire(s) based upon cross-section cut area
            if cont:
                area.translate(FreeCAD.Vector(0.0, 0.0, 0.0 - area.BoundBox.ZMin))
                footprint.translate(FreeCAD.Vector(0.0, 0.0, 0.0 - footprint.BoundBox.ZMin))

                try:
                    # Use this engine to perform the Cut between area and trimFace
                    clear_engine = Path.Area()
                    clear_engine.setPlane(self.wpc)
                    clear_engine.add(area)
                    clear_engine.add(trimFace, op=1)

                    clearArea = clear_engine.getShape()
                    if hasattr(clearArea, "removeSplitter"):
                        clearArea = clearArea.removeSplitter()
                    # Check if the resulting shape is valid and has actual geometry
                    if not clearArea or clearArea.BoundBox.DiagonalLength < 1e-6:
                        Path.Log.debug(
                            "Depth {}: Clear area vanished (below pocket). skipping.".format(csHght)
                        )
                        cont = False
                except Exception as e:
                    # If the math fails (FloatingPointError) or the area is Null
                    Path.Log.debug(
                        "Depth {}: Invalid geometry after cut. skipping.".format(csHght, str(e))
                    )
                    cont = False

                planarArea = clearArea

                if cont and obj.ClearPlanarOnly and status in ["Mixed", "Extra"]:
                    try:
                        # Use a fresh engine to isolate this specific Boolean math
                        intersect_engine = Path.Area()
                        intersect_engine.setPlane(self.wpc)
                        intersect_engine.add(clearArea)
                        intersect_engine.add(footprint, op=2)

                        res_planar = intersect_engine.getShape()

                        # Validate the result of the intersection
                        if res_planar and not res_planar.isNull() and res_planar.Area > 1e-7:
                            if hasattr(res_planar, "removeSplitter"):
                                res_planar = res_planar.removeSplitter()
                            planarArea = res_planar
                            Path.Log.debug(
                                "Depth {}: Targeted planar clearing successfully restricted to footprint.".format(
                                    csHght
                                )
                            )
                        else:
                            Path.Log.debug(
                                "Depth {}: Intersection result empty, falling back to full area.".format(
                                    csHght
                                )
                            )

                    except Exception as e:
                        # If the intersection fails, just proceed using the full clearArea.
                        Path.Log.warning(
                            "Depth {}: Footprint intersection failed ({}), using fallback.".format(
                                csHght, str(e)
                            )
                        )

            if cont:
                data = FreeCAD.Units.Quantity(csHght, FreeCAD.Units.Length).UserString
                Path.Log.debug("... Clearning area at {}.".format(data))
                # Make waterline path for current CUTAREA depth (csHght)
                commands.extend(self._wiresToWaterlinePath(obj, clearArea, csHght))
                clearArea.translate(FreeCAD.Vector(0.0, 0.0, 0.0 - clearArea.BoundBox.ZMin))

                # Clear layer as needed
                pattern_to_use = "None"
                if obj.ClearPlanarOnly:
                    # If feature is ON, only clear layers detected as physical floors
                    if status in ["Mixed", "Extra"]:
                        pattern_to_use = obj.CutPattern
                else:
                    # If feature is OFF, clear EVERY layer if a pattern is selected
                    pattern_to_use = obj.CutPattern

                # Generate Clearing
                if pattern_to_use == "Offset":
                    # The 'Offset' pattern follows the part profile recursively
                    Path.Log.debug(" - Clearing planar face via Offset at Z: {}".format(csHght))
                    commands.extend(self._makeOffsetLayerPaths(obj, clearArea, csHght))
                elif pattern_to_use != "None":
                    # All other patterns (ZigZag, Line, Spiral, etc.) use the generator
                    Path.Log.debug(
                        " - Clearing planar face via {} at Z: {}".format(pattern_to_use, csHght)
                    )
                    commands.extend(
                        self._makeCutPatternLayerPaths(JOB, obj, clearArea, csHght, pattern_to_use)
                    )

            indicator.next()

        # Efor

        indicator.stop()
        return commands

    def _getToolParams(self, obj):
        """Identifies tool geometry based on ShapeType string."""
        if not hasattr(obj, "ToolController") or not obj.ToolController:
            if not hasattr(self.tool, "ShapeType"):
                return None

        tool = obj.ToolController.Tool

        dia = float(tool.Diameter)
        radius = dia / 2.0

        # Read the ShapeType (Endmill, Ballend, or Bullnose)
        shape_type = getattr(tool, "ShapeType")

        if "Ballend" in shape_type:
            profile = "Ballend"
            c_rad = radius
            is_threeD = True
        elif "Bullnose" in shape_type:
            profile = "Bullnose"
            # Retrieve CornerRadius specifically for Bullnose tools
            c_rad = float(getattr(tool, "CornerRadius", 0.0))
            is_threeD = True
        elif "Endmill" in shape_type:
            profile = "Endmill"
            c_rad = 0.0
            is_threeD = False
        else:
            return None

        return {
            "radius": radius,
            "corner_radius": c_rad,
            "profile": profile,
            "is_threeD": is_threeD,
        }

    def _getOuterHull(self, shape):
        """Returns a version of the shape with all internal cavities removed."""
        Path.Log.debug("_getOuterHull")
        try:
            # Extract the outer shell of a solid
            if hasattr(shape, "OuterShell"):
                hull = Part.Solid(shape.OuterShell)
                return hull
            # Fallback:
            return shape
        except:
            return shape

    def _getBoundaryAdj(self, envelop, adj):
        """Returns Boundary Adjustment face at Z=0."""
        Path.Log.debug("Waterline: Executing _getBoundaryAdj")

        boundary_adj = None

        try:
            # Setup the Path.Area engine
            env_engine = Path.Area()
            # Generate the workplane from the envelope geometry
            wpc = PathUtils.makeWorkplane(envelop)
            env_engine.setPlane(wpc)
            env_engine.add(envelop)

            # Define Slicing height
            bb = envelop.BoundBox
            slice_z = bb.ZMin + 0.001

            # Configure Parameters
            params = env_engine.getDefaultParams()
            params["SectionTolerance"] = 0.0001
            params["Offset"] = adj
            env_engine.setParams(**params)

            # Execute with Projection
            boundary_adj = env_engine.makeSections(mode=0, project=True, heights=[slice_z])

        except Exception as e:
            Path.Log.warning("Waterline: BoundaryAdjustment C++ engine failed: {}".format(str(e)))
            return None

        # Extract and Clean result
        if boundary_adj:
            bbFace = boundary_adj[0].getShape()

            if bbFace and not bbFace.isNull():
                if hasattr(bbFace, "removeSplitter"):
                    bbFace = bbFace.removeSplitter()

                # Normalize to Z=0 for the border_engine
                bbFace.translate(FreeCAD.Vector(0, 0, -bbFace.BoundBox.ZMin))
                return bbFace

        return None

    def _getCutAreas(self, shape, depthparams, borderFace, tool_params, stock_to_leave):
        """
        Calculates 2D clearing areas with maximum optimization.
        Loads the model geometry into the C++ CAM engine exactly once.
        """
        Path.Log.debug("_getCutAreas: Ultra-Optimized Native Mode")

        # --- Helpers ---
        def _getEffectiveRadius(h, radius, c_rad, profile):
            if profile == "Ballend":
                return math.sqrt(max(0, radius**2 - (radius - h) ** 2))
            elif profile == "Bullnose":
                if h < c_rad:
                    dist_to_arc_center = c_rad - h
                    return (radius - c_rad) + math.sqrt(max(0, c_rad**2 - dist_to_arc_center**2))
            return radius

        def _determineSliceHght(csHght, modelBottom, modelTop, epsilon):
            """Determine the Slice Height (Model Footprint)."""
            sliceHght = None
            if csHght < modelBottom:
                sliceHght = modelBottom + 0.0005  # This must always be positive
            else:
                slcHght = csHght + epsilon
                # Ensure slice plane stays within geometry
                sliceHght = (modelTop - epsilon) if slcHght > (modelTop - epsilon) else slcHght
            return sliceHght

        # --- Initialization ---
        CUTAREAS = list()
        LAYER_METADATA = list()
        lenDP = len(depthparams)
        allPrevComp = Part.makeCompound([])
        isFirst = True

        # Initialize Progress Indicator
        indicator = FreeCAD.Base.ProgressIndicator()

        # Initialize area and load the 3D model
        area = Path.Area()
        wpc = PathUtils.makeWorkplane(shape)
        area.setPlane(wpc)
        area.add(shape)

        # Extract tool variables for local math
        radius = tool_params["radius"]
        c_rad = tool_params["corner_radius"]
        profile = tool_params["profile"]
        is_threeD = tool_params["is_threeD"]

        # Determine boundaries
        z_dir = -1.0 if (lenDP > 1 and depthparams[0] > depthparams[-1]) else 1.0
        epsilon = z_dir * -0.0005  # Higher than area tolerance
        modelBottom, modelTop = shape.BoundBox.ZMin, shape.BoundBox.ZMax

        # Determine tool sampling accuracy
        num_slices = self.sampAccuracy if is_threeD else 1

        categorizedSteps = self._categorizeFloorSteps(shape, depthparams)
        total_layers = len(categorizedSteps)

        # Start the indicator
        indicator.start("Experimental Z-Level Hybrid: Processing Geometry...", total_layers)

        # Main Depth Loop
        for idx, (z_target, status, floor_geo) in enumerate(categorizedSteps):

            if z_target > (modelTop + 0.0005):
                indicator.next()  # Increment anyway to keep bar moving
                continue

            sub_envelope_list = []

            # Layer Calibration
            # Determine the bias profile for this specific layer height
            dist_submerged = max(0, modelTop - z_target)
            max_h = min(c_rad, dist_submerged)

            # Horizontal Bias (Chordal/Top Correction)
            # Apply 2.2% reduction if we are at the top of a spherical tool
            r_bias = 0.978 if (profile == "Ballend" and dist_submerged < c_rad) else 1.0

            # Vertical Bias (Floor Accuracy Correction)
            # Corrects for epsilon-drift on detected CAD floors
            h_bias_nudge = -0.0004 if (status == "Mixed" and profile == "Ballend") else 0.0

            # Sub-Sampling Loop
            for i in range(num_slices):
                if is_threeD:
                    # Ballend and Bullnose End-Mills
                    # h is calculated, then nudged by the vertical bias
                    h_base = (max_h / (num_slices - 1)) * i if num_slices > 1 else 0.0
                    h = h_base + h_bias_nudge

                    # r_eff is calculated from base h, then biased horizontally
                    r_base = _getEffectiveRadius(h_base, radius, c_rad, profile)
                    r_eff = (r_base + stock_to_leave) * r_bias
                    target_h = z_target + h
                else:
                    # Flat Endmill (single slice)
                    r_eff = radius + stock_to_leave
                    target_h = z_target

                sliceHght = _determineSliceHght(target_h, modelBottom, modelTop, epsilon)

                # Tool compensation Params
                params = area.getDefaultParams()
                params["SectionTolerance"] = 0.0001
                params["Offset"] = r_eff
                area.setParams(**params)

                # Tool compensation
                sections = area.makeSections(mode=0, project=False, heights=[sliceHght])

                if sections:
                    compFace = sections[0].getShape()
                    if compFace and not compFace.isNull():
                        # Standard cleanup
                        if hasattr(compFace, "removeSplitter"):
                            compFace = compFace.removeSplitter()
                        compFace.translate(FreeCAD.Vector(0, 0, -compFace.BoundBox.ZMin))
                        sub_envelope_list.append(compFace)

            if not sub_envelope_list:
                continue

            fusion_engine = Path.Area()
            fusion_engine.setPlane(self.wpc)

            for env_item in sub_envelope_list:
                # Clipper (Path.Area) automatically unions overlapping shapes
                fusion_engine.add(env_item)

            # Extract the single, dissolved silhouette
            compAdjFaces = fusion_engine.getShape()

            if not compAdjFaces or compAdjFaces.isNull():
                continue

            # Standard cleanup to merge collinear edges
            if hasattr(compAdjFaces, "removeSplitter"):
                compAdjFaces = compAdjFaces.removeSplitter()

            # Use this engine to perform the Cut and Masking in one C++ pass
            layer_engine = Path.Area()
            layer_engine.setPlane(self.wpc)

            if status == "Extra" and floor_geo:
                # Logic: Cut only the specific detected floor
                layer_engine.add(floor_geo)
                layer_engine.add(compAdjFaces, op=1)
            else:
                # Standard Waterline Logic: New Material = (Stock - Model) - Already Cut
                layer_engine.add(borderFace)
                layer_engine.add(compAdjFaces, op=1)

                if not isFirst and allPrevComp:
                    # Subtract everything already cleared in layers above
                    layer_engine.add(allPrevComp, op=1)

            # Extract result from C++ to OpenCASCADE
            cutArea = layer_engine.getShape()

            # Use a second engine to fuse the footprint for the next layer down
            mask_engine = Path.Area()
            mask_engine.setPlane(self.wpc)
            if not isFirst and allPrevComp:
                mask_engine.add(allPrevComp)

            mask_engine.add(compAdjFaces)
            if (status == "Mixed" or status == "Extra") and floor_geo:
                mask_engine.add(floor_geo)

            # Save the new cumulative mask (Tool Center Path footprint)
            allPrevComp = mask_engine.getShape()

            # Translate toolpath to the ACTUAL target tip depth
            cutArea.translate(FreeCAD.Vector(0.0, 0.0, z_target))

            if cutArea.Area > 1e-9:
                CUTAREAS.append(cutArea)
                LAYER_METADATA.append(
                    {"status": status, "footprint": compAdjFaces.copy()}  # The 'mask' of the part
                )
                isFirst = False
                self.showDebugObject(cutArea, "CutArea_Z_{}".format(round(z_target, 5)))

            # Increment the progress bar after each layer is finished
            indicator.next()

        # Stop the indicator when finished
        indicator.stop()
        return CUTAREAS, LAYER_METADATA

    def _categorizeFloorSteps(self, shape, depthparams):
        """
        Reconciles detected CAD floors with standard depth steps.
        Returns a sorted list of (height, status, geometry_at_Z0).
        """
        Path.Log.debug("Z-Level: Categorizing Floor Steps")

        startZ, finalZ = depthparams[0], depthparams[-1]
        is_top_down = startZ > finalZ

        # Get all accessible, fused floor geometry grouped by height
        fused_geometry = self._getFusedFloorGeometry(
            shape,
            startZ,
            finalZ,
        )

        # Reconcile with standard depthparams
        final_depth_logic = []
        accounted_floors = set()

        for z_std in depthparams:
            z_std_round = round(z_std, 5)
            match_z = None
            for floor_z in fused_geometry.keys():
                # If the detected floor is within 0.0001mm of our standard step
                if abs(floor_z - z_std_round) < 0.0001:
                    match_z = floor_z
                    break  # We found a match, stop looking

            # Standard depth and floor height match - "Mixed" step
            if match_z is not None:
                final_depth_logic.append((z_std, "Mixed", fused_geometry[match_z]))
                accounted_floors.add(match_z)
            # No floor found in standard depths - "Pure" step
            else:
                final_depth_logic.append((z_std, "Pure", None))

        # Add remaining detected floors as "Extra"
        for z_f, geo in fused_geometry.items():
            if z_f not in accounted_floors:
                final_depth_logic.append((z_f, "Extra", geo))

        # Final sorting based on direction
        final_depth_logic.sort(key=lambda x: x[0], reverse=is_top_down)
        return final_depth_logic

    def _getFusedFloorGeometry(self, shape, startZ, finalZ, tolerance=0.0001):
        """Identifies, tests, and fuses upward-facing horizontal faces."""

        # --- Helpers ---
        def _isUpwardFacing(face):
            """True if the face normal points toward the tool (+Z)."""
            u1, u2, v1, v2 = face.ParameterRange
            norm = face.normalAt((u1 + u2) / 2.0, (v1 + v2) / 2.0)
            if face.Orientation == "Reversed":
                norm = norm.multiply(-1)
            return norm.z > 0.9

        def _isInRange(face, z_min, z_max, abs_top, abs_bottom, tolerance):
            """True if face is within start/final range and not an absolute model boundary."""
            z = round(face.Vertexes[0].Z, 5)
            if abs(z - abs_top) < 0.001 or abs(z - abs_bottom) < 0.001:
                return False
            return (z >= z_min - tolerance) and (z <= z_max + tolerance)

        def _isAccessibleFromTop(face, shape, abs_top):
            """Accessibility Check: Solid Projection (Shadow Test)."""
            try:
                z = face.Vertexes[0].Z
                extrude_h = (abs_top - z) + 5.0
                test_face = face.copy()
                test_face.translate(FreeCAD.Vector(0, 0, 0.001))  # Nudge above floor
                projection = test_face.extrude(FreeCAD.Vector(0, 0, extrude_h))

                # If the intersection with the model is empty, path is clear
                return not shape.common(projection).Vertexes
            except:
                return False

        # --- Initialization ---
        floor_accumulator = {}
        abs_top, abs_bottom = shape.BoundBox.ZMax, shape.BoundBox.ZMin
        z_min, z_max = min(startZ, finalZ), max(startZ, finalZ)

        for face in shape.Faces:
            if not (hasattr(face.Surface, "TypeId") and "Plane" in face.Surface.TypeId):
                continue

            # Check orientation and normal
            if _isUpwardFacing(face) and _isInRange(
                face, z_min, z_max, abs_top, abs_bottom, tolerance
            ):
                if _isAccessibleFromTop(face, shape, abs_top):
                    z = round(face.Vertexes[0].Z, 5)

                    # Normalize to Z=0
                    f_copy = face.copy()
                    f_copy.translate(FreeCAD.Vector(0, 0, -f_copy.BoundBox.ZMin))

                    if z not in floor_accumulator:
                        floor_accumulator[z] = []
                    floor_accumulator[z].append(f_copy)

        # Fuse coincident faces
        fused = {}
        for z, faces in floor_accumulator.items():
            res = faces[0]
            if len(faces) > 1:
                for i in range(1, len(faces)):
                    res = res.fuse(faces[i])
                if hasattr(res, "removeSplitter"):
                    res = res.removeSplitter()
            fused[z] = res
        return fused

    def _wiresToWaterlinePath(self, obj, ofstPlnrShp, csHght):
        Path.Log.debug("_wiresToWaterlinePath()")
        commands = list()

        # Translate path geometry to layer height
        ofstPlnrShp.translate(FreeCAD.Vector(0.0, 0.0, csHght - ofstPlnrShp.BoundBox.ZMin))
        self.showDebugObject(ofstPlnrShp, "WaterlinePathArea_{}".format(round(csHght, 2)))

        commands.append(Path.Command("N (Cut Area {}.)".format(round(csHght, 2))))

        start = 0
        if obj.IgnoreOuter and not obj.CutPattern == "Offset":
            start = 1

        for w in range(start, len(ofstPlnrShp.Wires)):
            wire = ofstPlnrShp.Wires[w]
            if not wire.isClosed():  # filter
                continue
            # Additional healing to prevent errors (rare but)
            if hasattr(wire, "removeSplitter"):
                wire = wire.removeSplitter()
            wire.fix(1e-6, 1e-6, 1e-4)

            V = wire.Vertexes
            if obj.CutMode == "Climb":
                lv = len(V) - 1
                startVect = FreeCAD.Vector(V[lv].X, V[lv].Y, V[lv].Z)
            else:
                startVect = FreeCAD.Vector(V[0].X, V[0].Y, V[0].Z)

            commands.append(Path.Command("N (Wire {}.)".format(w)))

            # This ensures the tool is directly above the entry point before plunging,
            # preventing diagonal moves through the material.
            commands.append(
                Path.Command("G0", {"X": startVect.x, "Y": startVect.y, "F": self.horizRapid})
            )
            (cmds, endVect) = self._wireToPath(obj, wire, startVect)
            commands.extend(cmds)
            commands.append(Path.Command("G0", {"Z": obj.SafeHeight.Value, "F": self.vertRapid}))

        return commands

    def _makeCutPatternLayerPaths(self, JOB, obj, clrAreaShp, csHght, cutPattern):
        Path.Log.debug("_makeCutPatternLayerPaths()")
        commands = []
        pntSet = None

        clrAreaShp.translate(FreeCAD.Vector(0.0, 0.0, 0.0 - clrAreaShp.BoundBox.ZMin))

        # Convert pathGeom to gcode more efficiently
        if cutPattern == "Offset":
            commands.extend(self._makeOffsetLayerPaths(obj, clrAreaShp, csHght))
        else:
            # Request path geometry from external support class
            PGG = PathSurfaceSupport.PathGeometryGenerator(obj, clrAreaShp, cutPattern)
            if self.showDebugObjects:
                PGG.setDebugObjectsGroup(self.tempGroup)
            self.tmpCOM = PGG.getCenterOfPattern()
            pathGeom = PGG.generatePathGeometry()
            if not pathGeom or (hasattr(pathGeom, "Edges") and not pathGeom.Edges):
                # Empty pathGeom from small areas - Skip
                return commands
            pathGeom.translate(FreeCAD.Vector(0.0, 0.0, csHght - pathGeom.BoundBox.ZMin))

            self.showDebugObject(pathGeom, "PathGeom_{}".format(round(csHght, 2)))

            if cutPattern == "Line":
                pntSet = PathSurfaceSupport.pathGeomToLinesPointSet(self, obj, pathGeom)
            elif cutPattern == "ZigZag":
                pntSet = PathSurfaceSupport.pathGeomToZigzagPointSet(self, obj, pathGeom)
            elif cutPattern in ["Circular", "CircularZigZag"]:
                pntSet = PathSurfaceSupport.pathGeomToCircularPointSet(self, obj, pathGeom)
            elif cutPattern == "Spiral":
                pntSet = PathSurfaceSupport.pathGeomToSpiralPointSet(obj, pathGeom)

            stpOVRS = self._getExperimentalWaterlinePaths(pntSet, csHght, cutPattern)
            safePDC = False
            cmds = self._clearGeomToPaths(JOB, obj, safePDC, stpOVRS, cutPattern)
            commands.extend(cmds)

        return commands

    def _getExperimentalWaterlinePaths(self, PNTSET, csHght, cutPattern):
        """_getExperimentalWaterlinePaths(PNTSET, csHght, cutPattern)...
        Switching function for calling the appropriate path-geometry to OCL points conversion function
        for the various cut patterns."""
        Path.Log.debug("_getExperimentalWaterlinePaths()")
        SCANS = list()

        # PNTSET is list, by stepover.
        if cutPattern in ["Line", "Spiral", "ZigZag"]:
            stpOvr = list()
            for STEP in PNTSET:
                for SEG in STEP:
                    if SEG == "BRK":
                        stpOvr.append(SEG)
                    else:
                        (A, B) = SEG  # format is ((p1, p2), (p3, p4))
                        P1 = FreeCAD.Vector(A[0], A[1], csHght)
                        P2 = FreeCAD.Vector(B[0], B[1], csHght)
                        stpOvr.append((P1, P2))
                SCANS.append(stpOvr)
                stpOvr = list()
        elif cutPattern in ["Circular", "CircularZigZag"]:
            # Each stepover is a list containing arc/loop descriptions, (sp, ep, cp)
            for so in range(0, len(PNTSET)):
                stpOvr = list()
                erFlg = False
                (aTyp, dirFlg, ARCS) = PNTSET[so]

                if dirFlg == 1:  # 1
                    cMode = True  # Climb mode
                else:
                    cMode = False

                for a in range(0, len(ARCS)):
                    Arc = ARCS[a]
                    if Arc == "BRK":
                        stpOvr.append("BRK")
                    else:
                        (sp, ep, cp) = Arc
                        S = FreeCAD.Vector(sp[0], sp[1], csHght)
                        E = FreeCAD.Vector(ep[0], ep[1], csHght)
                        C = FreeCAD.Vector(cp[0], cp[1], csHght)
                        scan = (S, E, C, cMode)
                        if scan is False:
                            erFlg = True
                        else:
                            stpOvr.append(scan)
                if erFlg is False:
                    SCANS.append(stpOvr)

        return SCANS

    def _makeOffsetLayerPaths(self, obj, clrAreaShp, csHght):
        Path.Log.debug("_makeOffsetLayerPaths() - Fragment Filter Version")

        command_blocks = []

        shape = clrAreaShp
        offset = -self.cutOut
        tol = 0.005
        while True:
            offsetArea = PathUtils.getOffsetArea(shape, offset, plane=self.wpc, tolerance=tol)
            if not offsetArea:
                # Area fully consumed
                break
            # Current ring's commands
            current_ring_cmds = []
            for f in offsetArea.Faces:
                for w in f.Wires:
                    # Filter tiny fragments
                    if w.Length > self.radius:
                        # Generate the G-code for this specific ring
                        current_ring_cmds.extend(self._wiresToWaterlinePath(obj, w, csHght))

            if current_ring_cmds:
                command_blocks.append(current_ring_cmds)

            offset -= self.cutOut

        if obj.CutPatternReversed:
            command_blocks.reverse()

        # Flatten the blocks into a single command list
        final_cmds = []
        for block in command_blocks:
            final_cmds.extend(block)

        return final_cmds

    def _clearGeomToPaths(self, JOB, obj, safePDC, stpOVRS, cutPattern):
        Path.Log.debug("_clearGeomToPaths()")

        GCODE = [Path.Command("N (Beginning of Single-pass layer.)", {})]
        tolrnc = JOB.GeometryTolerance.Value
        lenstpOVRS = len(stpOVRS)
        gDIR = ["G3", "G2"]

        if self.CutClimb is True:
            gDIR = ["G2", "G3"]

        # Send cutter to x,y position of first point on first line
        first = stpOVRS[0][0][0]  # [step][item][point]
        GCODE.append(Path.Command("G0", {"X": first.x, "Y": first.y, "F": self.horizRapid}))

        # Cycle through step-over sections (line segments or arcs)
        odd = True
        lstStpEnd = None
        for so in range(0, lenstpOVRS):
            cmds = list()
            PRTS = stpOVRS[so]
            lenPRTS = len(PRTS)
            first = PRTS[0][0]  # first point of arc/line stepover group
            last = None
            cmds.append(Path.Command("N (Begin step {}.)".format(so), {}))

            if so > 0:
                if cutPattern == "CircularZigZag":
                    if odd:
                        odd = False
                    else:
                        odd = True
                minTrnsHght = obj.SafeHeight.Value
                cmds.extend(
                    self._stepTransitionCmds(obj, cutPattern, lstStpEnd, first, minTrnsHght, tolrnc)
                )

            # Cycle through current step-over parts
            for i in range(0, lenPRTS):
                prt = PRTS[i]
                # Path.Log.debug('prt: {}'.format(prt))
                if prt == "BRK":
                    nxtStart = PRTS[i + 1][0]
                    minSTH = obj.SafeHeight.Value
                    cmds.append(Path.Command("N (Break)", {}))
                    cmds.extend(self._breakCmds(obj, cutPattern, last, nxtStart, minSTH, tolrnc))
                else:
                    cmds.append(Path.Command("N (part {}.)".format(i + 1), {}))
                    if cutPattern in ["Line", "ZigZag", "Spiral"]:
                        start, last = prt
                        cmds.append(
                            Path.Command(
                                "G1",
                                {
                                    "X": start.x,
                                    "Y": start.y,
                                    "Z": start.z,
                                    "F": self.horizFeed,
                                },
                            )
                        )
                        cmds.append(
                            Path.Command("G1", {"X": last.x, "Y": last.y, "F": self.horizFeed})
                        )
                    elif cutPattern in ["Circular", "CircularZigZag"]:
                        isZigZag = True if cutPattern == "CircularZigZag" else False
                        Path.Log.debug(
                            "so, isZigZag, odd, cMode: {}, {}, {}, {}".format(
                                so, isZigZag, odd, prt[3]
                            )
                        )
                        gcode = self._makeGcodeArc(prt, gDIR, odd, isZigZag)
                        cmds.extend(gcode)
            cmds.append(Path.Command("N (End of step {}.)".format(so), {}))
            GCODE.extend(cmds)  # save line commands
            lstStpEnd = last
        # Efor

        # Raise to safe height after clearing
        GCODE.append(Path.Command("G0", {"Z": obj.SafeHeight.Value, "F": self.vertRapid}))

        return GCODE

    def _wireToPath(self, obj, wire, startVect):
        """_wireToPath(obj, wire, startVect) ... wire to path."""
        Path.Log.track()

        paths = []
        pathParams = {}

        pathParams["shapes"] = [wire]
        pathParams["feedrate"] = self.horizFeed
        pathParams["feedrate_v"] = self.vertFeed
        pathParams["verbose"] = True
        pathParams["resume_height"] = obj.SafeHeight.Value
        pathParams["retraction"] = obj.SafeHeight.Value
        pathParams["return_end"] = True
        # Note that emitting preambles between moves breaks some dressups and prevents path optimization on some controllers
        pathParams["preamble"] = False
        pathParams["start"] = startVect

        (pp, end_vector) = Path.fromShapes(**pathParams)
        paths.extend(pp.Commands)

        self.endVector = end_vector

        return (paths, end_vector)

    def _makeExtendedBoundBox(self, wBB, bbBfr, zDep):
        pl = FreeCAD.Placement()
        pl.Rotation = FreeCAD.Rotation(FreeCAD.Vector(0, 0, 1), 0)
        pl.Base = FreeCAD.Vector(0, 0, 0)

        p1 = FreeCAD.Vector(wBB.XMin - bbBfr, wBB.YMin - bbBfr, zDep)
        p2 = FreeCAD.Vector(wBB.XMax + bbBfr, wBB.YMin - bbBfr, zDep)
        p3 = FreeCAD.Vector(wBB.XMax + bbBfr, wBB.YMax + bbBfr, zDep)
        p4 = FreeCAD.Vector(wBB.XMin - bbBfr, wBB.YMax + bbBfr, zDep)
        bb = Part.makePolygon([p1, p2, p3, p4, p1])

        return bb

    def _makeGcodeArc(self, prt, gDIR, odd, isZigZag):
        cmds = list()
        strtPnt, endPnt, cntrPnt, cMode = prt
        gdi = 0
        if odd:
            gdi = 1
        else:
            if not cMode and isZigZag:
                gdi = 1
        gCmd = gDIR[gdi]

        ijk = cntrPnt.sub(strtPnt)  # vector from start to center
        xyz = endPnt
        cmds.append(
            Path.Command(
                "G1",
                {"X": strtPnt.x, "Y": strtPnt.y, "Z": strtPnt.z, "F": self.horizFeed},
            )
        )
        cmds.append(
            Path.Command(
                gCmd,
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
            Path.Command("G1", {"X": endPnt.x, "Y": endPnt.y, "Z": endPnt.z, "F": self.horizFeed})
        )

        return cmds

    # Support methods
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

    def showDebugObject(self, objShape, objName):
        if self.showDebugObjects:
            do = FreeCAD.ActiveDocument.addObject("Part::Feature", "tmp_" + objName)
            do.Shape = objShape
            do.purgeTouched()
            self.tempGroup.addObject(do)


def SetupProperties():
    """SetupProperties() ... Return list of properties required for operation."""
    return [tup[1] for tup in ObjectWaterline.opPropertyDefinitions(False)]


def Create(name, obj=None, parentJob=None):
    """Create(name) ... Creates and returns a Waterline operation."""
    if obj is None:
        obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", name)
    obj.Proxy = ObjectWaterline(obj, name, parentJob)
    return obj
