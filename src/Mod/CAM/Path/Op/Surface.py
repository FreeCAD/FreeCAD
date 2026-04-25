# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2025 sliptonic <shopinthewoods@gmail.com>               *
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


__title__ = "CAM 3D Surface Operation"
__author__ = "sliptonic (Brad Collette)"
__url__ = "https://www.freecad.org"
__doc__ = "Unified 3D surface finishing operation replacing Surface and Waterline."

import FreeCAD

translate = FreeCAD.Qt.translate

# OCL must be installed
try:
    try:
        import ocl
    except ImportError:
        import opencamlib as ocl
except ImportError:
    msg = translate("CAM_Surface", "This operation requires OpenCamLib to be installed.")
    FreeCAD.Console.PrintError(msg + "\n")
    raise ImportError

from PySide.QtCore import QT_TRANSLATE_NOOP
import Path
import Path.Op.Base as PathOp
import Path.Base.Generator.surface_common as surface_common
import Path.Base.Generator.surface_stl as surface_stl
import Path.Base.Generator.surface_scan as surface_scan
import Path.Base.Generator.surface_dropcutter as surface_dropcutter
import Path.Base.Generator.surface_waterline as surface_waterline
import PathScripts.PathUtils as PathUtils
import time

# lazily loaded modules
from lazy_loader.lazy_loader import LazyLoader

Part = LazyLoader("Part", globals(), "Part")

if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


class ObjectSurface(PathOp.ObjectOp):
    """Unified 3D surfacing operation.

    Strategies:
    - SurfacePattern: 3D surface finishing via pattern projection
    - Waterline: Constant-Z contours via OCL
    - Z-Level Hybrid: Z-Level Waterline contours via shape slicing (no OCL, fallback)
    """

    # Accuracy level presets for Speed vs Accuracy control
    ACCURACY_PRESETS = {
        1: {  # Fastest - Coarse, for quick prototyping/verification
            "name": "Fastest",
            "angular_deflection": 0.5,  # Coarse chordal deviation for minimal mesh density
            "linear_deflection": 0.1,  # Relaxed for rough previews (avoids over-precision)
            "mesh_simplification": 7,  # Maximum reduction to speed up
            "sample_interval": 1.5,  # Sparse sampling for fast computation
            "min_sample_interval": 0.3,  # Minimum sparse sampling for fast computation
            "description": "Quick toolpath verification and rough prototypes",
        },
        2: {  # Very Fast
            "name": "Very Fast",
            "angular_deflection": 0.4,
            "linear_deflection": 0.075,
            "mesh_simplification": 6,  # Aggressive reduction
            "sample_interval": 1.0,
            "min_sample_interval": 0.20,
            "description": "Rapid roughing with basic verification",
        },
        3: {  # Fast
            "name": "Fast",
            "angular_deflection": 0.3,
            "linear_deflection": 0.05,
            "mesh_simplification": 5,  # Strong reduction
            "sample_interval": 0.5,
            "min_sample_interval": 0.10,
            "description": "Efficient processing for initial prototypes",
        },
        4: {  # Balanced
            "name": "Balanced",
            "angular_deflection": 0.2,
            "linear_deflection": 0.025,
            "mesh_simplification": 4,  # Moderate reduction
            "sample_interval": 0.25,
            "min_sample_interval": 0.05,
            "description": "Good compromise—fast with solid results for commercial work",
        },
        5: {  # Good Accuracy
            "name": "Good",
            "angular_deflection": 0.15,
            "linear_deflection": 0.015,
            "mesh_simplification": 3,  # Balanced reduction
            "sample_interval": 0.1,
            "min_sample_interval": 0.05,
            "description": "Reliable quality for most commercial machines, still quick",
        },
        6: {  # High Accuracy
            "name": "High",
            "angular_deflection": 0.1,
            "linear_deflection": 0.01,
            "mesh_simplification": 2,  # Light reduction
            "sample_interval": 0.07,
            "min_sample_interval": 0.05,
            "description": "Detailed surfacing for typical commercial tolerances",
        },
        7: {  # Ultra High Accuracy - For precision commercial jobs
            "name": "Ultra",
            "angular_deflection": 0.05,  # Fine chordal for smooth curves
            "linear_deflection": 0.005,  # Precise but not sub-micron (matches standard high-end commercial)
            "mesh_simplification": 1,  # Minimal reduction
            "sample_interval": 0.05,  # Dense sampling for quality finishes
            "min_sample_interval": 0.05,
            "description": "High quality for detailed commercial work, moderate processing time",
        },
    }

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
        Path.Log.track()
        self.propertiesReady = False

        self.initOpProperties(obj)

        if not hasattr(obj, "DoNotSetDefaultValues"):
            self.setEditorProperties(obj)

    def initOpProperties(self, obj, warn=False):
        """initOpProperties(obj) ... create operation specific properties"""
        Path.Log.track()
        self.addNewProps = []

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

        self.propertiesReady = True

    def opPropertyDefinitions(self):
        """opPropertyDefinitions() ... return list of tuples containing operation specific properties"""
        return [
            # -- Strategy --
            (
                "App::PropertyEnumeration",
                "Strategy",
                "Strategy",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Select the 3D surfacing strategy: Surface Pattern for projection-based finishing, "
                    "Waterline for constant-Z contours, "
                    "or Z-Level Hybrid for non-OCL fallback.",
                ),
            ),
            # -- Mesh Conversion --
            (
                "App::PropertyDistance",
                "AngularDeflection",
                "Performance Optimization",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Smaller values yield a finer, more accurate mesh. Smaller values increase processing time a lot.",
                ),
            ),
            (
                "App::PropertyDistance",
                "LinearDeflection",
                "Performance Optimization",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Smaller values yield a finer, more accurate mesh. Smaller values do not increase processing time much.",
                ),
            ),
            # -- Performance Optimization --
            (
                "App::PropertyInteger",
                "MeshSimplification",
                "Performance Optimization",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Mesh simplification level (1-7): 1=Highest accuracy, 7=Fastest processing. Higher values reduce triangle count for faster computation but lower accuracy.",
                ),
            ),
            # -- Selected Geometry Settings --
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
                "App::PropertyDistance",
                "BoundaryAdjustment",
                "Selected Geometry Settings",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Positive values push the cutter toward, or beyond, the boundary. Negative values retract the cutter away from the boundary.",
                ),
            ),
            # -- Clearing Options --
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
                "App::PropertyEnumeration",
                "CutPatternZLevel",
                "Clearing Options",
                QT_TRANSLATE_NOOP(
                    "App::Property", "Set the geometric clearing pattern to use for the operation."
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
                QT_TRANSLATE_NOOP("App::Property", "Set the start point for the cut pattern."),
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
                "App::PropertyBool",
                "AdaptiveSampling",
                "Clearing Options",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Dynamically adjusts sampling density in high-curvature areas.",
                ),
            ),
            (
                "App::PropertyDistance",
                "MinSampleInterval",
                "Clearing Options",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Set the minimum sampling resolution for Adaptive Sampling.",
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
            # -- Waterline-specific --
            (
                "App::PropertyEnumeration",
                "SamplingAccuracy",
                "Clearing Options",
                QT_TRANSLATE_NOOP(
                    "App::Property", "Number of sub-slices for 3D tool compensation."
                ),
            ),
            (
                "App::PropertyDistance",
                "StockToLeave",
                "Clearing Options",
                QT_TRANSLATE_NOOP(
                    "App::Property", "Material to leave on the part in the XY plane."
                ),
            ),
            (
                "App::PropertyBool",
                "ClearPlanarOnly",
                "Clearing Options",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "If true, clears only detected horizontal floors.",
                ),
            ),
            (
                "App::PropertyBool",
                "IgnoreOuter",
                "Clearing Options",
                QT_TRANSLATE_NOOP("App::Property", "Ignore outer waterlines."),
            ),
            # -- Optimization --
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
                "KeepToolDown",
                "Optimization",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Keep tool down during short transitions instead of retracting to safe height.",
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
            # -- Start Point --
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

        enums = {
            "Strategy": [
                (translate("CAM_Surface", "Surface Pattern"), "SurfacePattern"),
                (translate("CAM_Surface", "Waterline"), "Waterline"),
                (translate("CAM_Surface", "Z-Level Hybrid"), "ZLevelHybrid"),
            ],
            "BoundBox": [
                (translate("CAM_Surface", "BaseBoundBox"), "BaseBoundBox"),
                (translate("CAM_Surface", "Stock"), "Stock"),
            ],
            "PatternCenterAt": [
                (translate("CAM_Surface", "CenterOfMass"), "CenterOfMass"),
                (translate("CAM_Surface", "CenterOfBoundBox"), "CenterOfBoundBox"),
                (translate("CAM_Surface", "XminYmin"), "XminYmin"),
                (translate("CAM_Surface", "Custom"), "Custom"),
            ],
            "CutMode": [
                (translate("CAM_Surface", "Conventional"), "Conventional"),
                (translate("CAM_Surface", "Climb"), "Climb"),
            ],
            "CutPattern": [
                (translate("CAM_Surface", "Line"), "Line"),
                (translate("CAM_Surface", "ZigZag"), "ZigZag"),
                (translate("CAM_Surface", "Circular"), "Circular"),
                (translate("CAM_Surface", "CircularZigZag"), "CircularZigZag"),
                (translate("CAM_Surface", "Spiral"), "Spiral"),
                (translate("CAM_Surface", "Offset"), "Offset"),
            ],
            "CutPatternZLevel": [
                (translate("CAM_Surface", "None"), "None"),
                (translate("CAM_Surface", "Line"), "Line"),
                (translate("CAM_Surface", "ZigZag"), "ZigZag"),
                (translate("CAM_Surface", "Offset"), "Offset"),
                (translate("CAM_Surface", "Grid"), "Grid"),
            ],
            "LayerMode": [
                (translate("CAM_Surface", "Single-pass"), "Single-pass"),
                (translate("CAM_Surface", "Multi-pass"), "Multi-pass"),
            ],
            "SamplingAccuracy": [
                (translate("path_waterline", "Standard"), "4"),
                (translate("path_waterline", "High"), "8"),
                (translate("path_waterline", "Very High"), "16"),
                (translate("path_waterline", "Ultra"), "32"),
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
            "Strategy": "SurfacePattern",
            "AdaptiveSampling": False,
            "OptimizeLinearPaths": True,
            "KeepToolDown": True,
            "UseStartPoint": False,
            "StartPoint": FreeCAD.Vector(0.0, 0.0, obj.ClearanceHeight.Value),
            "CutPatternReversed": False,
            "LayerMode": "Single-pass",
            "CutMode": "Conventional",
            "CutPattern": "Line",
            "CutPatternZLevel": "None",
            "PatternCenterAt": "CenterOfMass",
            "ClearPlanarOnly": False,
            "IgnoreOuter": False,
            "StockToLeave": 0.0,
            "StepOver": 50.0,
            "CutPatternAngle": 0.0,
            "DepthOffset": 0.0,
            "SampleInterval": 1.00,
            "MinSampleInterval": 0.20,
            "BoundaryAdjustment": 0.0,
            "AvoidLastX_Faces": 0,
            "PatternCenterCustom": FreeCAD.Vector(0.0, 0.0, 0.0),
            "GapThreshold": 0.005,
            "AngularDeflection": 0.25,
            "LinearDeflection": 0.001,
            "MeshSimplification": 1,  # Default to highest accuracy (no simplification)
            "SamplingAccuracy": "4",
        }

        warn = True
        if hasattr(job, "GeometryTolerance"):
            if job.GeometryTolerance.Value != 0.0:
                warn = False
                defaults["LinearDeflection"] = job.GeometryTolerance.Value / 4
        if warn:
            msg = translate("CAM_Surface", "The GeometryTolerance for this Job is 0.0.")
            msg += translate("CAM_Surface", "Initializing LinearDeflection to 0.001 mm.")
            FreeCAD.Console.PrintWarning(msg + "\n")

        return defaults

    def setEditorProperties(self, obj):
        """setEditorProperties(obj) ... Adjust property visibility based on Strategy."""
        Path.Log.track()
        # UI modes: 0 = show, 2 = hide
        show = 0
        hide = 2

        strategy = getattr(obj, "Strategy", "SurfacePattern")
        is_surface_pattern = strategy == "SurfacePattern"
        is_zlevel = strategy == "ZLevelHybrid"
        is_waterline = strategy == "Waterline"

        # Logic Groups:
        # A: Z-Level Hybrid specific properties
        # B: SurfacePattern/Mesh-specific properties
        # C: Pattern-dependent settings (StepOver, etc.)
        A = show if is_zlevel else hide
        B = hide if is_zlevel else show
        C = show if is_surface_pattern else hide

        # SurfacePattern specific contexts
        obj.setEditorMode("AvoidLastX_Faces", show if is_surface_pattern else hide)
        obj.setEditorMode("StartPoint", show if is_surface_pattern else hide)
        obj.setEditorMode("UseStartPoint", show if is_surface_pattern else hide)
        obj.setEditorMode("CutPattern", show if is_surface_pattern else hide)
        obj.setEditorMode("CutPatternAngle", show if is_surface_pattern else hide)
        obj.setEditorMode("BoundBox", show if is_surface_pattern else hide)
        obj.setEditorMode("KeepToolDown", show if is_surface_pattern else hide)
        obj.setEditorMode("GapThreshold", show if is_surface_pattern else hide)
        obj.setEditorMode("LayerMode", show if is_surface_pattern else hide)

        # Adaptive Sampling Logic
        can_adaptive = is_waterline or is_surface_pattern
        obj.setEditorMode("AdaptiveSampling", show if can_adaptive else hide)

        is_adaptive = getattr(obj, "AdaptiveSampling", False) and can_adaptive
        obj.setEditorMode("MinSampleInterval", show if is_adaptive else hide)

        # Pattern center is relevant for circular/spiral patterns in SurfacePattern
        pattern_needs_center = is_surface_pattern and not obj.CutPattern in ["Line", "ZigZag"]
        obj.setEditorMode("PatternCenterAt", show if pattern_needs_center else hide)
        obj.setEditorMode("PatternCenterCustom", show if pattern_needs_center else hide)

        if is_zlevel:
            z_pattern = getattr(obj, "CutPatternZLevel", "None")
            C = hide if z_pattern == "None" else show

        # Apply Visibility to Z-Level Group (A)
        obj.setEditorMode("ClearPlanarOnly", A)
        obj.setEditorMode("IgnoreOuter", A)
        obj.setEditorMode("StockToLeave", A)
        obj.setEditorMode("CutPatternZLevel", A)
        obj.setEditorMode("SamplingAccuracy", A)

        # Apply Visibility to Mesh/OCL Group (B)
        obj.setEditorMode("AngularDeflection", B)
        obj.setEditorMode("LinearDeflection", B)
        obj.setEditorMode("MeshSimplification", B)
        obj.setEditorMode("OptimizeLinearPaths", B)
        obj.setEditorMode("SampleInterval", B)

        # Apply Visibility to Common/Contextual Group (C)
        obj.setEditorMode("StepOver", C)
        obj.setEditorMode("CutPatternReversed", C)
        obj.setEditorMode("CutPatternAngle", C)

        # Global Properties
        obj.setEditorMode("CutMode", show)
        obj.setEditorMode("DepthOffset", show)
        obj.setEditorMode("BoundaryAdjustment", show if not is_waterline else hide)

    def opOnChanged(self, obj, prop):
        if hasattr(self, "propertiesReady"):
            if self.propertiesReady:
                if prop in ["Strategy", "CutPattern", "CutPatternZLevel", "AdaptiveSampling"]:
                    self.setEditorProperties(obj)
                elif prop == "MeshSimplification":
                    if hasattr(obj, "MeshSimplification"):
                        if obj.MeshSimplification < 1:
                            obj.MeshSimplification = 1
                        elif obj.MeshSimplification > 7:
                            obj.MeshSimplification = 7

    def apply_accuracy_preset(self, obj, level):
        """Apply preset values based on accuracy level (1-7).

        Args:
            obj: The 3D Surface operation object
            level: Accuracy level (1=Fastest, 7=Ultra)
        """
        Path.Log.track()
        preset = self.ACCURACY_PRESETS.get(level, self.ACCURACY_PRESETS[4])

        if hasattr(obj, "AngularDeflection"):
            obj.AngularDeflection = preset["angular_deflection"]
        if hasattr(obj, "LinearDeflection"):
            obj.LinearDeflection = preset["linear_deflection"]
        if hasattr(obj, "MeshSimplification"):
            obj.MeshSimplification = preset["mesh_simplification"]
        if hasattr(obj, "SampleInterval"):
            obj.SampleInterval = preset["sample_interval"]
        if hasattr(obj, "MinSampleInterval"):
            obj.MinSampleInterval = preset["min_sample_interval"]

    def get_accuracy_level(self, obj):
        """Determine accuracy level from current property values.

        Args:
            obj: The 3D Surface operation object

        Returns:
            int: Accuracy level (1-7) if current values match a preset exactly
            None: If user has customized settings and they don't match any preset
        """
        Path.Log.track()
        for level, preset in self.ACCURACY_PRESETS.items():
            if (
                hasattr(obj, "AngularDeflection")
                and hasattr(obj, "LinearDeflection")
                and hasattr(obj, "MeshSimplification")
                and hasattr(obj, "SampleInterval")
                and hasattr(obj, "MinSampleInterval")
            ):

                if (
                    obj.AngularDeflection == preset["angular_deflection"]
                    and obj.LinearDeflection == preset["linear_deflection"]
                    and obj.MeshSimplification == preset["mesh_simplification"]
                    and obj.SampleInterval == preset["sample_interval"]
                    and obj.MinSampleInterval == preset["min_sample_interval"]
                ):
                    return level

        # No exact match found - user has customized settings
        return None

    def opOnDocumentRestored(self, obj):
        self.propertiesReady = False
        job = PathUtils.findParentJob(obj)

        self.initOpProperties(obj, warn=True)
        self.opApplyPropertyDefaults(obj, job, self.addNewProps)

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
        """opApplyPropertyLimits(obj) ... Apply necessary limits to user input property values."""
        # Limit sample interval
        if obj.SampleInterval.Value < 0.0001:
            obj.SampleInterval.Value = 0.0001
            Path.Log.error("Sample interval limits are 0.0001 to 25.4 millimeters.")
        if obj.SampleInterval.Value > 25.4:
            obj.SampleInterval.Value = 25.4
            Path.Log.error("Sample interval limits are 0.0001 to 25.4 millimeters.")

        # Limit min sample interval
        if obj.MinSampleInterval.Value < 0.0001:
            obj.MinSampleInterval.Value = 0.0001
            Path.Log.error("Min sample interval limits are 0.0001 to 25.4 millimeters.")
        if obj.MinSampleInterval.Value > 25.4:
            obj.MinSampleInterval.Value = 25.4
            Path.Log.error("Min sample interval limits are 0.0001 to 25.4 millimeters.")

        # Limit cut pattern angle
        if obj.CutPatternAngle < -360.0 or obj.CutPatternAngle >= 360.0:
            obj.CutPatternAngle = 0.0

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

        # Limit StockToLeave to positive values
        if obj.StockToLeave < 0:
            obj.StockToLeave = 0

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
                    obj.OpFinalDepth = self.job.Stock.Shape.BoundBox.ZMin

    # ---- Strategy execution methods ----

    def _extractToolParams(self, obj):
        """Extract tool parameters from the ToolController for OCL cutter creation."""
        tc = obj.ToolController
        tool = tc.Tool

        tool_type = None
        diameter = 0.0
        corner_radius = 0.0
        flat_radius = 0.0
        edge_height = 0.0
        edge_angle = 0.0
        length_offset = 0.0

        if hasattr(tool, "ShapeType"):
            tool_type = tool.ShapeType.lower()
        elif hasattr(tool, "ShapeName"):
            tool_type = tool.ShapeName.lower()

        if hasattr(tool, "Diameter"):
            diameter = float(tool.Diameter)
        if hasattr(tool, "FlatRadius"):
            flat_radius = float(tool.FlatRadius)
        if hasattr(tool, "CornerRadius"):
            corner_radius = float(tool.CornerRadius)
            flat_radius = (diameter / 2.0) - corner_radius
        if hasattr(tool, "CuttingEdgeHeight"):
            edge_height = float(tool.CuttingEdgeHeight)
        if hasattr(tool, "CuttingEdgeAngle"):
            edge_angle = float(tool.CuttingEdgeAngle)
        if hasattr(tool, "LengthOffset"):
            length_offset = float(tool.LengthOffset)

        Path.Log.info(
            "Surface tool: type={}, diameter={}, edge_height={}, "
            "corner_radius={}, flat_radius={}, edge_angle={}".format(
                tool_type, diameter, edge_height, corner_radius, flat_radius, edge_angle
            )
        )

        return {
            "tool_type": tool_type,
            "diameter": diameter,
            "corner_radius": corner_radius,
            "flat_radius": flat_radius,
            "edge_height": edge_height,
            "edge_angle": edge_angle,
            "length_offset": length_offset,
        }

    def _getSelectedFaces(self, obj):
        """Extract selected faces from obj.Base, applying AvoidLastX_Faces.

        Returns a list of Part.Face objects to cut, or an empty list if
        no faces are selected (meaning the whole model should be used).
        """
        import time

        Path.Log.debug(
            "_getSelectedFaces: hasattr Base={}, Base={}".format(
                hasattr(obj, "Base"), obj.Base if hasattr(obj, "Base") else "N/A"
            )
        )
        if not hasattr(obj, "Base") or not obj.Base:
            Path.Log.debug("_getSelectedFaces: no Base geometry, using whole model")
            return []

        # Benchmark face extraction
        extract_start = time.time()
        all_faces = []
        total_subs = 0
        for base, subs in obj.Base:
            Path.Log.debug("_getSelectedFaces: base={}, subs={}".format(base.Label, subs))
            for sub in subs:
                # Skip empty sub-element names - they indicate whole object selection
                if not sub:
                    Path.Log.debug("_getSelectedFaces: skipping empty sub-element for whole object")
                    continue
                total_subs += 1
                shape = getattr(base.Shape, sub, None)
                if shape is not None and isinstance(shape, Part.Face):
                    all_faces.append(shape)
                    Path.Log.debug("_getSelectedFaces: added face {}".format(sub))
                else:
                    Path.Log.debug(
                        "_getSelectedFaces: sub '{}' not found or not a face".format(sub)
                    )
        extract_time = time.time() - extract_start
        Path.Log.debug(
            "_getSelectedFaces: extraction took {:.3f}s for {} subs, {} faces".format(
                extract_time, total_subs, len(all_faces)
            )
        )

        if not all_faces:
            return []

        # Benchmark AvoidLastX_Faces processing
        avoid_start = time.time()
        avoid = obj.AvoidLastX_Faces if hasattr(obj, "AvoidLastX_Faces") else 0
        if avoid > 0 and avoid < len(all_faces):
            return all_faces[: len(all_faces) - avoid]
        elif avoid >= len(all_faces):
            return []

        return all_faces

    def _getBoundBox(self, obj, job, selected_faces=None):
        """Get the bounding box for the operation based on BoundBox property.

        If selected_faces is provided, the BaseBoundBox is computed from
        those faces instead of the full model.
        """
        Path.Log.debug(
            "_getBoundBox: BoundBox={}, selected_faces={}".format(
                obj.BoundBox, len(selected_faces) if selected_faces else 0
            )
        )
        if obj.BoundBox == "Stock":
            bb = job.Stock.Shape.BoundBox
            Path.Log.debug("_getBoundBox: using Stock BB: {}".format(bb))
            return bb

        # If we have selected faces, use their bounding box
        if selected_faces:
            bb = selected_faces[0].BoundBox
            for f in selected_faces[1:]:
                bb.add(f.BoundBox)
            Path.Log.debug("_getBoundBox: using selected faces BB: {}".format(bb))
            return bb

        # Fallback: union of all model bounding boxes
        models = job.Model.Group
        bb = models[0].Shape.BoundBox
        for m in models[1:]:
            bb.add(m.Shape.BoundBox)
        Path.Log.debug("_getBoundBox: using model BB: {}".format(bb))
        return bb

    def _executeSurfacePattern(self, obj, job, stl, cutter, tool_diam, bb, selected_faces=None):
        """
        Executes the Surface Pattern (projection) strategy.

        This is the primary function for generating toolpaths by projecting a 2D pattern
        onto a 3D STL mesh. It follows a highly optimized, multi-stage pipeline:

        Args:
            obj (Path::FeaturePython): The 3D Surface operation object.
            job (Path::Job): The parent Job object.
            stl (ocl.STLSurf): The primary OCL mesh for the toolpath calculation.
            safe_stl (ocl.STLSurf): A secondary OCL mesh including check surfaces, used for
                                   collision-safe "Keep Tool Down" transitions.
            cutter (ocl.Cutter): The OCL representation of the tool.
            tool_diam (float): The diameter of the active tool.
            bb (BoundBox): The bounding box of the entire operation area.
            selected_faces (list, optional): A list of Part.Face objects if the user
                                             has made a specific selection. Defaults to None.

        Returns:
            list: A list of Path.Command objects representing the final G-code.
        """
        cmds = []

        step_over = tool_diam * (float(obj.StepOver) / 100.0)
        cut_climb = obj.CutMode == "Climb"

        final_depth = obj.FinalDepth.Value
        depth_offset = obj.DepthOffset.Value
        sample_interval = obj.SampleInterval.Value
        pattern_reverse = obj.CutPatternReversed
        pattern = obj.CutPattern if hasattr(obj, "CutPattern") else "Line"
        is_adaptive = getattr(obj, "AdaptiveSampling", False)

        if pattern_reverse:
            cut_climb = not cut_climb

        # Generate the pattern mask (boundary face)
        boundary_adj = obj.BoundaryAdjustment.Value if hasattr(obj, "BoundaryAdjustment") else 0.0
        boundary_face = surface_common.generate_pattern_mask(
            job, obj, selected_faces, tool_diam / 2.0, boundary_adj
        )
        if boundary_face:
            b_bb = boundary_face.BoundBox
            scan_bb = surface_scan.BBox(b_bb.XMin, b_bb.XMax, b_bb.YMin, b_bb.YMax)
        else:
            # Fallback
            scan_bb = surface_scan.BBox(bb.XMin, bb.XMax, bb.YMin, bb.YMax)

        # Generate 2D scan lines based on pattern
        angle = obj.CutPatternAngle if hasattr(obj, "CutPatternAngle") else 0.0
        raw_scan_lines = []

        if pattern == "Offset":
            if boundary_face:
                raw_scan_lines = surface_scan.generate_offset_scan_lines(
                    boundary_face, step_over, sample_interval, pattern_reverse, cut_climb
                )
            else:
                Path.Log.warning("Offset pattern requires a valid boundary. Aborting.")
                return []

        else:  # (Line, ZigZag, Circular, CircularZigZag, Spiral)
            center = (scan_bb.center[0], scan_bb.center[1])
            is_zigzag = pattern in ("ZigZag", "CircularZigZag")

            tolerance = obj.LinearDeflection.Value if hasattr(obj, "LinearDeflection") else 0.005

            raw_scan_lines = surface_scan.fast_generate_pattern(
                pattern,
                scan_bb,
                center,
                step_over,
                sample_interval,
                angle,
                is_zigzag,
                pattern_reverse,
                cut_climb,
                boundary_face,
                tolerance,
            )

        # Build one combined OCL path
        path_obj = ocl.Path()

        for line in raw_scan_lines:
            if len(line) < 2:
                continue
            for i in range(len(line) - 1):
                p1 = ocl.Point(line[i][0], line[i][1], final_depth)
                p2 = ocl.Point(line[i + 1][0], line[i + 1][1], final_depth)
                path_obj.append(ocl.Line(p1, p2))

        # Project scan lines to 3D surface
        results_flat = []

        adaptive_threshold = 0.25  # Switch to Standard DropCutter beyond this point
        is_truly_adaptive = is_adaptive and sample_interval >= adaptive_threshold

        if is_adaptive and not is_truly_adaptive:
            Path.Log.info(
                f"SampleInterval ({sample_interval:.3f}mm) is below the adaptive threshold (0.25mm)."
            )
            Path.Log.info("Switching to faster standard dropcutter for this high-density path.")

        if is_truly_adaptive:  # AdaptivePathDropCutter
            min_sampling = (
                obj.MinSampleInterval.Value
                if hasattr(obj, "MinSampleInterval")
                else sample_interval / 10.0
            )

            results_flat = surface_dropcutter.adaptive_path_dropcutter(
                stl, cutter, path_obj, final_depth, sample_interval, min_sampling
            )
        else:
            if pattern in ("Line", "ZigZag"):  # PathDropCutter
                results_flat = surface_dropcutter.path_dropcutter(
                    stl, cutter, path_obj, final_depth, sample_interval
                )
            else:  # (Circular, Spiral, Offset) - BatchDropCutter
                results_flat = surface_dropcutter.batch_dropcutter(
                    stl, cutter, raw_scan_lines, final_depth
                )

        # Reconstruct & optimize the results
        scan_lines = surface_scan.reconstruct_scan_lines(results_flat, sample_interval * 2.5)

        if obj.OptimizeLinearPaths:
            scan_lines = [surface_common.filter_cl_points(line, 0.005) for line in scan_lines]

        # Apply Multi-pass roughing
        if getattr(obj, "LayerMode", "Single-pass") == "Multi-pass":
            scan_lines = surface_dropcutter.apply_multipass(
                scan_lines, obj.StartDepth.Value, obj.FinalDepth.Value, obj.StepDown.Value
            )

        # Generate G-Code
        opt_transitions = getattr(obj, "KeepToolDown", False)
        cmds = surface_dropcutter.scan_lines_to_gcode(
            scan_lines,
            horiz_feed=self.horizFeed,
            vert_rapid=self.vertRapid,
            horiz_rapid=self.horizRapid,
            safe_z=obj.SafeHeight.Value,
            clearance_z=obj.ClearanceHeight.Value,
            depth_offset=depth_offset,
            optimize_transitions=opt_transitions,
            safe_stl=stl if opt_transitions else None,
            cutter=cutter if opt_transitions else None,
        )

        return cmds

    def _executeWaterline(self, obj, job, stl, cutter, tool_diam, bb, is_adaptive=False):
        """Execute the Waterline strategy using Phase 1 generators.

        Flow:
        1. Calculate Z-height range from depths
        2. Run waterline_stack at multiple Z-heights
        3. Convert to G-code
        """
        startTime = time.time()

        sample_interval = obj.SampleInterval.Value
        min_sampling = obj.MinSampleInterval.Value
        min_z = obj.FinalDepth.Value
        max_z = obj.StartDepth.Value
        depth_offset = obj.DepthOffset.Value
        step_down = obj.StepDown.Value
        cut_climb = obj.CutMode == "Climb"

        adaptive_threshold = (
            0.25  # If SampleInterval is already this fine, standard dropcutter is faster.
        )
        is_truly_adaptive = is_adaptive and sample_interval >= adaptive_threshold

        if is_adaptive and not is_truly_adaptive:
            Path.Log.info(
                f"SampleInterval ({sample_interval:.3f}mm) is below the adaptive threshold ({adaptive_threshold}mm)."
            )
            Path.Log.info("Switching to faster standard dropcutter for this high-density path.")

        if obj.CutPatternReversed:
            cut_climb = not cut_climb

        Path.Log.info(
            "Waterline: min_z={:.2f}, max_z={:.2f}, step_down={:.2f}, is_truly_adaptive={}".format(
                min_z, max_z, step_down, is_truly_adaptive
            )
        )

        wl_data = surface_waterline.waterline_stack(
            stl,
            cutter,
            sample_interval,
            min_sampling,
            min_z=min_z,
            max_z=max_z,
            step_down=step_down,
            adaptive=is_truly_adaptive,
            depth_offset=depth_offset,
        )

        # Filter collinear points if optimization is enabled
        if obj.OptimizeLinearPaths:
            tolerance = obj.GapThreshold.Value if hasattr(obj.GapThreshold, "Value") else 0.005
            for zh in wl_data:
                filter_loop = []
                for loop in wl_data[zh]:
                    filter_loop.append(surface_common.filter_cl_points(loop, tolerance))
                wl_data[zh] = filter_loop

        cmds = surface_waterline.waterline_to_gcode(
            wl_data,
            horiz_feed=self.horizFeed,
            vert_rapid=self.vertRapid,
            horiz_rapid=self.horizRapid,
            safe_z=obj.SafeHeight.Value,
            clearance_z=obj.ClearanceHeight.Value,
            cut_climb=cut_climb,
        )

        elapsed = time.time() - startTime
        Path.Log.info(
            "Waterline strategy completed in {:.2f}s, {} commands".format(elapsed, len(cmds))
        )

        return cmds

    def _executeZLevelHybrid(self, obj, job):
        """Execute the Z-Level Hybrid strategy (no OCL required).

        A high-precision geometric finishing strategy that operates directly on
        B-Rep geometry. It combines constant-Z contouring with automatic floor
        detection and clearing.

        Flow:
        1. Prepare and fuse model geometry into a manifold shape.
        2. Extract ToolBit parameters for specific 3D profile math.
        3. Arguments and Dictionaries preparation
        4. Generate master boundary (TrimFace) and stable background pool.
        5. Categorize depths, reconciling standard steps with physical model floors.
        6. Dispatch to surface_zlevel generator for C++ accelerated geometry stacking.
        7. Convert the resulting geometry stack into optimized G-code Path commands.
        """

        def _getZLevelToolParams():
            """Specialized helper for Z-Level Hybrid math requirements."""
            tool = obj.ToolController.Tool
            dia = float(tool.Diameter)
            shape_type = getattr(tool, "ShapeType")
            radius = dia / 2.0

            is_3d = False
            c_rad = 0.0
            if "Ballend" in shape_type:
                is_3d = True
                c_rad = radius
            elif "Bullnose" in shape_type:
                is_3d = True
                c_rad = float(getattr(tool, "CornerRadius", 0.0))
            elif "Endmill" in shape_type:
                is_3d = False
                c_rad = 0.0
            else:
                return None

            return {"radius": radius, "c_rad": c_rad, "profile": shape_type, "is_threeD": is_3d}

        def _makeExtendedBoundBox(wBB, bbBfr, zDep):
            """Creates a large rectangular wire around the stock."""
            p1 = FreeCAD.Vector(wBB.XMin - bbBfr, wBB.YMin - bbBfr, zDep)
            p2 = FreeCAD.Vector(wBB.XMax + bbBfr, wBB.YMin - bbBfr, zDep)
            p3 = FreeCAD.Vector(wBB.XMax + bbBfr, wBB.YMax + bbBfr, zDep)
            p4 = FreeCAD.Vector(wBB.XMin - bbBfr, wBB.YMax + bbBfr, zDep)
            return Part.makePolygon([p1, p2, p3, p4, p1])

        def _getZLevelTrimFace(shape, borderFace, tool_params, wpc):
            """Calculates the 'Outside World' mask to clip the toolpath."""

            # In Z-Level Hybrid, we always use the entire model silhouette
            # make_boundary_face(faces, radius, extra_offset)
            radius = -tool_params["radius"]
            adj = obj.BoundaryAdjustment.Value - 0.01
            offset = radius + adj

            if obj.BoundBox == "Stock":
                bbFace = surface_common.make_boundary_face(job.Stock.Shape.Faces, offset)
            else:
                bbFace = surface_common.make_boundary_face(shape.Faces, offset)

            trim_engine = Path.Area()
            trim_engine.setPlane(wpc)
            trim_engine.add(borderFace)

            if bbFace:
                bbFace.translate(FreeCAD.Vector(0, 0, -bbFace.BoundBox.ZMin))
                trim_engine.add(bbFace, op=1)

            return trim_engine.getShape()

        startTime = time.time()

        # 1. Geometry prep - Exclusively use the whole model
        models = job.Model.Group
        if not models:
            Path.Log.error("Z-Level Hybrid: No model found in Job.")
            return []

        shape = (
            models[0].Shape
            if len(models) == 1
            else models[0].Shape.multiFuse([m.Shape for m in models[1:]])
        )

        # 2. Extract ToolBit parameters
        tool_params = _getZLevelToolParams()
        if tool_params is None:
            error_msg = translate(
                "Surface",
                "Operation failed: A Tool Type has been selected that is not supported by Z-Level Hybrid Algorithm.",
            )
            FreeCAD.Console.PrintError(error_msg + "\n")
            return []

        # 3. Arguments and Dictionaries preparation
        wpc = Part.makeCircle(2.0, FreeCAD.Vector(0, 0, 0), FreeCAD.Vector(0, 0, 1))
        radius = tool_params["radius"]

        clear_planar_only = getattr(obj, "ClearPlanarOnly", True)
        depth_offset = obj.DepthOffset.Value
        ignore_outer = getattr(obj, "IgnoreOuter", False)
        accuracy_val = getattr(obj, "SamplingAccuracy", "4")
        step_over = (obj.StepOver / 100.0) * (radius * 2)
        stock_to_leave = obj.StockToLeave.Value

        pattern_options = {
            "cut_climb": obj.CutMode == "Climb",
            "cut_pattern": getattr(obj, "CutPatternZLevel", "None"),
            "pattern_angle": getattr(obj, "CutPatternAngle", "45"),
            "reverse_pattern": getattr(obj, "CutPatternReversed", False),
        }

        height_params = {
            "safe_hght": obj.SafeHeight.Value,
            "clearance_hght": obj.ClearanceHeight.Value,
        }

        feed_params = {
            "horizFeed": self.horizFeed,
            "vertFeed": self.vertFeed,
            "horizRapid": self.horizRapid,
            "vertRapid": self.vertRapid,
        }

        # 4. Boundary preparation
        buffer = radius * 10.0
        border_poly = _makeExtendedBoundBox(job.Stock.Shape.BoundBox, buffer, 0.0)
        borderFace = Part.makeFace(border_poly)
        trimFace = _getZLevelTrimFace(shape, borderFace, tool_params, wpc)

        import Path.Base.Generator.surface_zlevel as surface_zlevel

        # 5. Depth categorization
        cat_steps = surface_zlevel.categorize_floor_steps(
            shape, obj.OpStartDepth.Value, obj.OpFinalDepth.Value, obj.StepDown.Value
        )

        # 6. Generate Geometry Stack
        wl_data = surface_zlevel.zlevel_hybrid_stack(
            shape,
            cat_steps,
            borderFace,
            trimFace,
            tool_params,
            stock_to_leave,
            accuracy_val,
            depth_offset,
            wpc,
        )

        # 7. Convert to G-Code
        cmds = surface_zlevel.zlevel_hybrid_to_gcode(
            wl_data,
            feed_params,
            height_params,
            pattern_options,
            ignore_outer,
            clear_planar_only,
            step_over,
            radius,
        )

        elapsed = time.time() - startTime
        Path.Log.info(
            "Z-Level Hybrid strategy completed in {:.2f}s, {} commands".format(elapsed, len(cmds))
        )

        return cmds

    def opExecute(self, obj):
        """Main execution method for 3D Surface operation.

        Dispatches to the appropriate strategy based on obj.Strategy:
        1. Extract tool parameters and create OCL cutter
        2. Prepare geometry (baseShapes handles multi-axis transform)
        3. Create STL mesh from model
        4. Dispatch to strategy-specific execution method
        5. Append commands to self.commandlist
        """
        Path.Log.track()

        startTime = time.time()

        # Identify parent Job
        JOB = PathUtils.findParentJob(obj)
        if JOB is None:
            Path.Log.error(translate("CAM_Surface", "No JOB"))
            return

        # Impose property limits
        self.opApplyPropertyLimits(obj)

        strategy = obj.Strategy
        is_adaptive = getattr(obj, "AdaptiveSampling", False)

        # Extract selected faces and apply AvoidLastX_Faces filtering
        faces_start = time.time()
        selected_faces = self._getSelectedFaces(obj)
        faces_time = time.time() - faces_start
        Path.Log.info("opExecute: face selection took {:.3f}s".format(faces_time))

        if strategy in ["ZLevelHybrid", "Waterline"]:
            selected_faces = []

        # Z-Level Hybrid doesn't need OCL cutter or STL
        if strategy == "ZLevelHybrid":
            cmds = self._executeZLevelHybrid(obj, JOB)
            self.commandlist.extend(cmds)
            return

        if hasattr(obj, "Base") and obj.Base and not selected_faces:
            # Check if this is whole-model selection (empty sub-elements) vs actual face selection
            has_face_selections = False
            for base, subs in obj.Base:
                for sub in subs:
                    if sub:  # Non-empty sub-element means actual face selection
                        has_face_selections = True
                        break
                if has_face_selections:
                    break

            if has_face_selections:
                # User selected specific faces but all were avoided
                Path.Log.warning(
                    "Surface: User selected faces but all were avoided by AvoidLastX_Faces. Using whole model."
                )
                # Don't return - continue with whole model processing
            else:
                # This is whole-model selection (empty sub-elements), which is normal
                Path.Log.debug("Surface: Processing whole model (no specific face selection)")
                # Continue with whole model processing

        # Extract tool parameters and create OCL cutter
        tool_start = time.time()
        tool_params = self._extractToolParams(obj)
        cutter = surface_common.make_ocl_cutter(
            tool_params["tool_type"],
            tool_params["diameter"],
            edge_height=tool_params["edge_height"],
            corner_radius=tool_params["corner_radius"],
            flat_radius=tool_params["flat_radius"],
            edge_angle=tool_params["edge_angle"],
            length_offset=tool_params["length_offset"],
        )
        tool_time = time.time() - tool_start
        Path.Log.info("opExecute: tool creation took {:.3f}s".format(tool_time))

        if cutter is None:
            Path.Log.error(
                translate("CAM_Surface", "Error creating OCL cutter from tool parameters.")
            )
            return

        tool_diam = cutter.getDiameter()
        Path.Log.info(
            "Surface OCL cutter created: getDiameter()={}, StepOver={}%, "
            "stepover_dist={}".format(tool_diam, obj.StepOver, tool_diam * (obj.StepOver / 100.0))
        )

        # Get bounding box (constrained to selected faces if available)
        bbox_start = time.time()
        bb = self._getBoundBox(obj, JOB, selected_faces)
        bbox_time = time.time() - bbox_start
        Path.Log.info("opExecute: bounding box took {:.3f}s".format(bbox_time))

        # Fallback to the Job's Model Group if no Base geometry is specified in the operation
        base_objs = (
            [base for base, subs in obj.Base]
            if hasattr(obj, "Base") and obj.Base
            else JOB.Model.Group
        )

        # Create STL from model shapes
        stl_start = time.time()
        stl = None
        for base in base_objs:
            model_shape = base.Shape

            if hasattr(base, "TypeId") and base.TypeId.startswith("Mesh"):
                Path.Log.info("opExecute: Using mesh-based STL conversion")
                mesh = base.Mesh
                points = [tuple(p) for p in mesh.Points]
                facets = [tuple(f) for f in mesh.Facets]
                stl = surface_stl.mesh_to_stl(points, facets)
            else:
                # Check which STL method will be used
                if (
                    hasattr(surface_stl, "_HAS_CPP")
                    and surface_stl._HAS_CPP
                    and strategy == "SurfacePattern"
                ):
                    Path.Log.info("opExecute: Using C++ accelerated STL conversion")
                else:
                    Path.Log.info("opExecute: Using Python fallback STL conversion")

                stl = surface_stl.shape_to_stl(
                    model_shape,
                    obj.LinearDeflection.Value,
                    obj.AngularDeflection.Value,
                    mesh_simplification=getattr(obj, "MeshSimplification", 1),
                    final_depth=obj.OpFinalDepth.Value if hasattr(obj, "OpFinalDepth") else None,
                    use_cpp=(strategy == "SurfacePattern"),
                )
            break  # Fix that
        stl_time = time.time() - stl_start
        Path.Log.info("opExecute: STL creation took {:.3f}s".format(stl_time))

        if stl is None:
            Path.Log.error(translate("CAM_Surface", "Failed to create STL from model."))
            return

        # Begin GCode for operation with basic information
        if obj.Comment != "":
            self.commandlist.append(Path.Command("N ({})".format(str(obj.Comment)), {}))
        self.commandlist.append(Path.Command("N ({})".format(obj.Label), {}))
        self.commandlist.append(Path.Command("N (Strategy: {})".format(strategy), {}))
        self.commandlist.append(
            Path.Command("N (Tool diameter: {:.3f})".format(tool_params["diameter"]), {})
        )
        self.commandlist.append(
            Path.Command("N (Sample interval: {})".format(str(obj.SampleInterval.Value)), {})
        )
        self.commandlist.append(Path.Command("N (Step over %: {})".format(str(obj.StepOver)), {}))
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

        # Dispatch to strategy
        cmds = []
        if strategy == "SurfacePattern":
            strategy_start = time.time()
            cmds = self._executeSurfacePattern(obj, JOB, stl, cutter, tool_diam, bb, selected_faces)
            strategy_time = time.time() - strategy_start
            Path.Log.info(
                f"DropCutter strategy completed in {strategy_time:.2f}s, {len(cmds)} commands"
            )
        elif strategy == "Waterline":
            cmds = self._executeWaterline(
                obj, JOB, stl, cutter, tool_diam, bb, is_adaptive=is_adaptive
            )

        self.commandlist.extend(cmds)

        elapsed = time.time() - startTime
        Path.Log.info("Surface operation completed in {:.2f}s".format(elapsed))


def Create(name, obj=None, parentJob=None):
    """Create(name) ... Creates and returns a 3D Surface operation."""
    if obj is None:
        obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", name)
    obj.Proxy = ObjectSurface(obj, name, parentJob)
    return obj


def SetupProperties():
    setup = []
    setup.append("Strategy")
    setup.append("BoundBox")
    setup.append("CutMode")
    setup.append("CutPattern")
    setup.append("CutPatternAngle")
    setup.append("CutPatternReversed")
    setup.append("DepthOffset")
    setup.append("LayerMode")
    setup.append("StepOver")
    setup.append("OptimizeLinearPaths")
    setup.append("CutPatternZLevel")
    setup.append("SamplingAccuracy")
    setup.append("StockToLeave")
    setup.append("ClearPlanarOnly")
    setup.append("IgnoreOuter")
    setup.append("OptimizeLinearPaths")
    setup.append("SampleInterval")
    setup.append("AdaptiveSampling")
    setup.append("MinSampleInterval")
    setup.append("LinearDeflection")
    setup.append("AngularDeflection")
    setup.append("MeshSimplification")
    return setup
