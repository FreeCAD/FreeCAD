# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2025 sliptonic <shopinthewoods@gmail.com>
# SPDX-FileCopyrightText: 2026 Dimitris75 <dimitriospana75@gmail.com>
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


__title__ = "CAM Planar Surface Operation"
__author__ = "sliptonic (Brad Collette)"
__url__ = "https://www.freecad.org"
__doc__ = "Unified 3D surface finishing operation replacing Surface and Waterline."

import FreeCAD

translate = FreeCAD.Qt.translate

# OCL must be installed. The import itself is the availability probe: unlike
# importlib.util.find_spec it also catches a present-but-broken binary install.
try:
    import ocl
except ImportError:
    try:
        import opencamlib as ocl
    except ImportError:
        ocl = None

if ocl is None:
    msg = translate("CAM_PlanarSurface", "This operation requires OpenCamLib to be installed.")
    FreeCAD.Console.PrintError(msg + "\n")
    raise ImportError(msg)

import time
from typing import Any, ClassVar

# lazily loaded modules
from lazy_loader.lazy_loader import LazyLoader
from PathScripts import PathUtils
from PySide.QtCore import QT_TRANSLATE_NOOP

import Path
import Path.Op.Base as PathOp
from Path.Base.Generator import (
    surface_common,
    surface_dropcutter,
    surface_mesh,
    surface_pattern,
    surface_postprocess,
    surface_waterline,
)

Part = LazyLoader("Part", globals(), "Part")

Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


class ObjectSurface(PathOp.ObjectOp):
    """Unified 3D surfacing operation.

    Strategies:
    - SurfaceScan: 3D surface finishing via pattern projection
    - Waterline: Constant-Z contours via OCL
    - Z-Level Hybrid: Z-Level Waterline contours via shape slicing (no OCL, fallback)
    """

    # Accuracy level presets for Speed vs Accuracy control
    ACCURACY_PRESETS: ClassVar[dict[int, dict[str, Any]]] = {
        1: {  # Fastest - Coarse, for quick prototyping/verification
            "name": "Fastest",
            "angular_deflection": 0.5,  # Coarse chordal deviation for minimal mesh density
            "linear_deflection": 0.1,  # Relaxed for rough previews (avoids over-precision)
            "mesh_simplification": 7,
            "sample_interval": 1.5,  # Sparse sampling for fast computation
            "min_sample_interval": 0.3,  # Minimum sparse sampling for fast computation
            "description": "Quick verification and rough prototypes",
        },
        2: {  # Very Fast
            "name": "Very Fast",
            "angular_deflection": 0.4,
            "linear_deflection": 0.075,
            "mesh_simplification": 6,
            "sample_interval": 1.0,
            "min_sample_interval": 0.2,
            "description": "Rapid roughing with basic verification",
        },
        3: {  # Fast
            "name": "Fast",
            "angular_deflection": 0.3,
            "linear_deflection": 0.05,
            "mesh_simplification": 5,
            "sample_interval": 0.5,
            "min_sample_interval": 0.1,
            "description": "Efficient processing for initial prototypes",
        },
        4: {  # Balanced
            "name": "Balanced",
            "angular_deflection": 0.2,
            "linear_deflection": 0.025,
            "mesh_simplification": 4,
            "sample_interval": 0.25,
            "min_sample_interval": 0.05,
            "description": "Good compromise for most commercial work",
        },
        5: {  # Good Accuracy
            "name": "Good",
            "angular_deflection": 0.15,
            "linear_deflection": 0.015,
            "mesh_simplification": 3,
            "sample_interval": 0.1,
            "min_sample_interval": 0.05,
            "description": "Reliable quality for commercial machines",
        },
        6: {  # High Accuracy
            "name": "High",
            "angular_deflection": 0.1,
            "linear_deflection": 0.01,
            "mesh_simplification": 2,
            "sample_interval": 0.07,
            "min_sample_interval": 0.05,
            "description": "Detailed surfacing for typical commercial tolerances",
        },
        7: {  # Ultra High Accuracy - For precision commercial jobs
            "name": "Ultra",
            "angular_deflection": 0.05,  # Fine chordal for smooth curves
            "linear_deflection": 0.005,  # Precise but not sub-micron (matches standard high-end commercial)
            "mesh_simplification": 1,
            "sample_interval": 0.05,  # Dense sampling for quality finishes
            "min_sample_interval": 0.05,
            "description": "High quality detailed work, slower processing",
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
                    "Select the 3D surfacing strategy: Surface Scan for projection-based finishing, "
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
                    "Mesh simplification level (1-7): 1=No reduction, 7=Fastest processing. "
                    "Aggressively reduces triangle count on flat surfaces to speed up calculation, "
                    "while safely preserving walls, fillets, and sharp edges. "
                    "(Note: Requires the 'pyvista' Python library to be installed).",
                ),
            ),
            (
                "App::PropertyDistance",
                "SampleInterval",
                "Performance Optimization",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Set the sampling resolution. Smaller values quickly increase processing time.",
                ),
            ),
            (
                "App::PropertyDistance",
                "MinSampleInterval",
                "Performance Optimization",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Set the minimum sampling resolution for Adaptive Sampling.",
                ),
            ),
            (
                "App::PropertyBool",
                "AdaptiveSampling",
                "Performance Optimization",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Dynamically adjusts sampling density in high-curvature areas.",
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
                "AvoidFacesOverlap",
                "Selected Geometry Settings",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Allows the tool to overlap into the avoided area (For positive values only).",
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
                "App::PropertyEnumeration",
                "ProfileEdges",
                "Selected Geometry Settings",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Profile the edges of the selection.",
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
                    "App::Property",
                    "Select the overall boundary for the operation.",
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
                    "App::Property",
                    "Set the geometric clearing pattern to use for the operation.",
                ),
            ),
            (
                "App::PropertyFloat",
                "CutPatternAngle",
                "Clearing Options",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "The yaw angle used for certain clearing patterns",
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
                "PatternCenterAt",
                "Clearing Options",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Choose the center point for radial patterns.",
                ),
            ),
            (
                "App::PropertyVectorDistance",
                "PatternCenterCustom",
                "Clearing Options",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "A custom center point for radial patterns.",
                ),
            ),
            (
                "App::PropertyEnumeration",
                "SamplingAccuracy",
                "Clearing Options",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Number of sub-slices for 3D tool compensation.",
                ),
            ),
            (
                "App::PropertyDistance",
                "StockToLeave",
                "Clearing Options",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Material to leave on the part in the XY plane.",
                ),
            ),
            (
                "App::PropertyBool",
                "ClearPlanarOnly",
                "Clearing Options",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Clears only detected horizontal floors.",
                ),
            ),
            (
                "App::PropertyBool",
                "IgnoreOuter",
                "Clearing Options",
                QT_TRANSLATE_NOOP("App::Property", "Ignore outer waterlines."),
            ),
            (
                "App::PropertyBool",
                "FillSelectedHoles",
                "Clearing Options",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Selected vertical face(s) in the 'Base Geometry' will be filled/capped.",
                ),
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
                "OptimizeMeshConversion",
                "Optimization",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Can drastically decrease processing time from 5% to 150% based on certain criteria."
                    "Still in Beta phase - disable if you experience issues.",
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
                "App::PropertyFloat",
                "KeepToolDownRatio",
                "Optimization",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Max length of keep tool down path compared to direct distance between points",
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
            # -- LeadInOut --
            (
                "App::PropertyBool",
                "LeadInOut",
                "LeadIn/LeadOut",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Enable smart lead-in and lead-out moves for the Surface Scan strategy. "
                    "Disables Keep Tool Down automatically when  is active.",
                ),
            ),
            (
                "App::PropertyPercent",
                "LeadFeed",
                "LeadIn/LeadOut",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Lead-in and lead-out feed rate as a percentage of the horizontal feed rate. "
                    "100% means full feed rate.",
                ),
            ),
            (
                "App::PropertyDistance",
                "LeadLiftDistance",
                "LeadIn/LeadOut",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Vertical lift distance applied to the lead-in / lead-out.",
                ),
            ),
            # -- Volumetric Feed Rate --
            (
                "App::PropertyPercent",
                "VolumetricFeedPercent",
                "Volumetric Feed Rate",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Increases the horizontal feed rate at the top of the cut as a percentage (0% disables the boost).",
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
            # -- Adaptive Z-Level cut pattern--
            (
                "App::PropertyEnumeration",
                "AdaptiveAccuracy",
                "AdaptivePatternSettings",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Influences calculation performance vs stability and accuracy."
                    "\nLarger values will calculate faster; Smaller values will result in more accurate toolpaths.",
                ),
            ),
            (
                "App::PropertyDistance",
                "LiftDistance",
                "AdaptivePatternSettings",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Lift distance for rapid moves",
                ),
            ),
            (
                "App::PropertyDistance",
                "KeepToolDownThreshold",
                "AdaptivePatternSettings",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Max length of keep tool down path compared to direct distance between points",
                ),
            ),
            (
                "App::PropertyBool",
                "ForceInsideOut",
                "AdaptivePatternSettings",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Force plunging into material inside and clearing towards the edges",
                ),
            ),
            (
                "App::PropertyBool",
                "FinishingProfile",
                "AdaptivePatternSettings",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "To take a finishing profile path at the end",
                ),
            ),
            (
                "App::PropertyAngle",
                "HelixMaxRampAngle",
                "AdaptivePatternSettings",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "The maximum allowable helix ramp entry angle (degrees)"
                    "\nSet to zero to disable limitation by ramp angle",
                ),
            ),
            (
                "App::PropertyPercent",
                "HelixMaxDiameterPercent",
                "AdaptivePatternSettings",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Maximum (and nominal) helix entry diameter, as a percentage of the tool diameter",
                ),
            ),
            (
                "App::PropertyBool",
                "EnforceGeofence",
                "AdaptivePatternSettings",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "WARNING: Disabling this allows the Adaptive2d algorithm to roam outside the stock boundary on open pockets. "
                    "This can cause erratic plunges, unpredictable toolpaths, and machine crashes! Proceed with extreme caution.",
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

        enums = {
            "Strategy": [
                (translate("CAM_PlanarSurface", "Surface Scan"), "SurfaceScan"),
                (translate("CAM_PlanarSurface", "Waterline"), "Waterline"),
                (translate("CAM_PlanarSurface", "Z-Level Hybrid"), "ZLevelHybrid"),
            ],
            "BoundBox": [
                (translate("CAM_PlanarSurface", "BaseBoundBox"), "BaseBoundBox"),
                (translate("CAM_PlanarSurface", "Stock"), "Stock"),
            ],
            "PatternCenterAt": [
                (translate("CAM_PlanarSurface", "Center of Boundary"), "CenterOfBoundary"),
                (translate("CAM_PlanarSurface", "Custom"), "Custom"),
            ],
            "CutMode": [
                (translate("CAM_PlanarSurface", "Conventional"), "Conventional"),
                (translate("CAM_PlanarSurface", "Climb"), "Climb"),
            ],
            "CutPattern": [
                (translate("CAM_PlanarSurface", "Line"), "Line"),
                (translate("CAM_PlanarSurface", "ZigZag"), "ZigZag"),
                (translate("CAM_PlanarSurface", "Circular"), "Circular"),
                (translate("CAM_PlanarSurface", "CircularZigZag"), "CircularZigZag"),
                (translate("CAM_PlanarSurface", "Spiral"), "Spiral"),
                (translate("CAM_PlanarSurface", "Offset"), "Offset"),
            ],
            "CutPatternZLevel": [
                (translate("CAM_PlanarSurface", "None"), "None"),
                (translate("CAM_PlanarSurface", "Line"), "Line"),
                (translate("CAM_PlanarSurface", "ZigZag"), "ZigZag"),
                (translate("CAM_PlanarSurface", "Offset"), "Offset"),
                (translate("CAM_PlanarSurface", "Adaptive"), "Adaptive"),
                (translate("CAM_PlanarSurface", "Grid"), "Grid"),
            ],
            "LayerMode": [
                (translate("CAM_PlanarSurface", "Single-pass"), "Single-pass"),
                (translate("CAM_PlanarSurface", "Multi-pass"), "Multi-pass"),
            ],
            "SamplingAccuracy": [
                (translate("CAM_PlanarSurface", "Standard"), "4"),
                (translate("CAM_PlanarSurface", "High"), "8"),
                (translate("CAM_PlanarSurface", "Very High"), "16"),
                (translate("CAM_PlanarSurface", "Ultra"), "32"),
            ],
            "HandleMultipleFeatures": [
                (translate("CAM_PlanarSurface", "Collectively"), "Collectively"),
                (translate("CAM_PlanarSurface", "Individually"), "Individually"),
            ],
            "ProfileEdges": [
                (translate("CAM_PlanarSurface", "None"), "None"),
                (translate("CAM_PlanarSurface", "First"), "First"),
                (translate("CAM_PlanarSurface", "Last"), "Last"),
                (translate("CAM_PlanarSurface", "Only"), "Only"),
            ],
            "AdaptiveAccuracy": [
                (translate("CAM_PlanarSurface", "Very Low"), "0.15"),
                (translate("CAM_PlanarSurface", "Low"), "0.1"),
                (translate("CAM_PlanarSurface", "Standard"), "0.08"),
                (translate("CAM_PlanarSurface", "High"), "0.05"),
                (translate("CAM_PlanarSurface", "Very High"), "0.02"),
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
            "Strategy": "SurfaceScan",
            "AdaptiveSampling": False,
            "OptimizeLinearPaths": True,
            "OptimizeMeshConversion": True,
            "KeepToolDown": True,
            "KeepToolDownRatio": 2.0,
            "UseStartPoint": False,
            "StartPoint": FreeCAD.Vector(0.0, 0.0, obj.ClearanceHeight.Value),
            "CutPatternReversed": False,
            "LayerMode": "Single-pass",
            "CutMode": "Conventional",
            "CutPattern": "Line",
            "CutPatternZLevel": "None",
            "PatternCenterAt": "CenterOfBoundary",
            "PatternCenterCustom": FreeCAD.Vector(0.0, 0.0, 0.0),
            "ClearPlanarOnly": False,
            "IgnoreOuter": False,
            "FillSelectedHoles": False,
            "StockToLeave": 0.0,
            "StepOver": 50.0,
            "CutPatternAngle": 0.0,
            "DepthOffset": 0.0,
            "SampleInterval": 0.25,
            "MinSampleInterval": 0.05,
            "BoundaryAdjustment": 0.0,
            "AvoidLastX_Faces": 0,
            "AvoidFacesOverlap": 0.0,
            "HandleMultipleFeatures": "Collectively",
            "ProfileEdges": "None",
            "GapThreshold": 0.005,
            "AngularDeflection": 0.2,
            "LinearDeflection": 0.025,
            "MeshSimplification": 4,
            "SamplingAccuracy": "4",
            "LeadInOut": False,
            "LeadFeed": 75,
            "VolumetricFeedPercent": 25,
            "LeadLiftDistance": 1.0,
            "AdaptiveAccuracy": "0.1",
            "LiftDistance": 0.05,
            "KeepToolDownThreshold": 3.00,
            "ForceInsideOut": False,
            "FinishingProfile": True,
            "HelixMaxRampAngle": 3.00,
            "HelixMaxDiameterPercent": 75,
            "EnforceGeofence": True,
        }

        return defaults

    def setEditorProperties(self, obj):
        """setEditorProperties(obj) ... Adjust property visibility based on Strategy."""
        Path.Log.track()
        # UI modes: 0 = show, 2 = hide
        show = 0
        hide = 2

        strategy = getattr(obj, "Strategy", "SurfaceScan")
        is_surface_scan = strategy == "SurfaceScan"
        is_zlevel = strategy == "ZLevelHybrid"
        is_waterline = strategy == "Waterline"

        # Logic Groups:
        # A: Surface Scan specific properties
        # B-C: Z-Level Hybrid specific properties
        # D: SurfaceScan/Mesh-specific properties
        # E-F: Pattern-dependent settings (StepOver, etc.)
        A = show if is_surface_scan else hide
        B = show if is_zlevel else hide
        C, D, E = hide, hide, hide
        F = hide if is_zlevel else show

        # SurfaceScan specific contexts
        obj.setEditorMode("AvoidLastX_Faces", A)
        obj.setEditorMode("AvoidFacesOverlap", A)
        obj.setEditorMode("HandleMultipleFeatures", A)
        obj.setEditorMode("CutPattern", A)
        obj.setEditorMode("CutPatternAngle", A)
        obj.setEditorMode("LayerMode", A)
        obj.setEditorMode("ProfileEdges", A)
        obj.setEditorMode("LeadInOut", A)
        obj.setEditorMode("LeadFeed", A)
        obj.setEditorMode("LeadLiftDistance", A)
        obj.setEditorMode("VolumetricFeedPercent", A)

        # Adaptive Sampling Logic
        can_adaptive = is_waterline or is_surface_scan
        obj.setEditorMode("AdaptiveSampling", show if can_adaptive else hide)

        is_adaptive = getattr(obj, "AdaptiveSampling", False) and can_adaptive
        obj.setEditorMode("MinSampleInterval", show if is_adaptive else hide)

        # Pattern center is relevant for circular/spiral patterns in SurfaceScan
        pattern_needs_center = is_surface_scan and not obj.CutPattern in ["Line", "ZigZag"]
        obj.setEditorMode("PatternCenterAt", show if pattern_needs_center else hide)
        obj.setEditorMode("PatternCenterCustom", show if pattern_needs_center else hide)

        if is_zlevel:
            z_pattern = getattr(obj, "CutPatternZLevel", "None")
            C = show if z_pattern == "Adaptive" else hide
            E = hide if z_pattern == "None" else show
            F = hide if z_pattern in ["None", "Offset", "Adaptive"] else show
        if is_surface_scan:
            D = show
            E = show
            F = show if obj.CutPattern in ["Line", "ZigZag"] else hide
        if is_waterline:
            D = show

        # Apply Visibility to Z-Level Group (B-C)
        obj.setEditorMode("CutPatternZLevel", B)
        obj.setEditorMode("StockToLeave", B)
        obj.setEditorMode("ClearPlanarOnly", B)
        obj.setEditorMode("IgnoreOuter", B)
        obj.setEditorMode("FillSelectedHoles", B)
        obj.setEditorMode("SamplingAccuracy", B)
        obj.setEditorMode("StartPoint", B)
        obj.setEditorMode("UseStartPoint", B)
        obj.setEditorMode("AdaptiveAccuracy", C)
        obj.setEditorMode("LiftDistance", C)
        obj.setEditorMode("KeepToolDownThreshold", C)
        obj.setEditorMode("ForceInsideOut", C)
        obj.setEditorMode("FinishingProfile", C)
        obj.setEditorMode("HelixMaxRampAngle", C)
        obj.setEditorMode("HelixMaxDiameterPercent", C)
        obj.setEditorMode("EnforceGeofence", C)

        # Apply Visibility to Mesh/OCL Group (D)
        obj.setEditorMode("AngularDeflection", D)
        obj.setEditorMode("LinearDeflection", D)
        obj.setEditorMode("MeshSimplification", D)
        obj.setEditorMode("OptimizeLinearPaths", D)
        obj.setEditorMode("OptimizeMeshConversion", D)
        obj.setEditorMode("SampleInterval", D)
        obj.setEditorMode("GapThreshold", D)

        # Apply Visibility to Common/Contextual Group (E-F)
        obj.setEditorMode("StepOver", E)
        obj.setEditorMode("CutPatternReversed", E)
        obj.setEditorMode("CutPatternAngle", F)

        # Global Properties
        obj.setEditorMode("CutMode", show)
        obj.setEditorMode("DepthOffset", show)
        obj.setEditorMode("KeepToolDown", show if not is_waterline else hide)
        obj.setEditorMode("KeepToolDownRatio", show if not is_waterline else hide)
        obj.setEditorMode("BoundaryAdjustment", show if not is_waterline else hide)
        obj.setEditorMode("BoundBox", show if not is_waterline else hide)

    def opOnChanged(self, obj, prop):
        if not getattr(self, "propertiesReady", False):
            return
        if prop in ["Strategy", "CutPattern", "CutPatternZLevel", "AdaptiveSampling"]:
            self.setEditorProperties(obj)
        elif prop == "MeshSimplification" and hasattr(obj, "MeshSimplification"):
            if obj.MeshSimplification < 1:
                obj.MeshSimplification = 1
            elif obj.MeshSimplification > 7:
                obj.MeshSimplification = 7

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
                if hasattr(prop, "Value") and isinstance(val, (int, float)):
                    prop.Value = val
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

        Path.Log.debug(f"Default OpFinalDepth: {obj.OpFinalDepth.Value}")
        Path.Log.debug(f"Default OpStartDepth: {obj.OpStartDepth.Value}")

    def opApplyPropertyLimits(self, obj):
        """opApplyPropertyLimits(obj) ... Apply necessary limits to user input property values."""
        # Limit Keep Tool Down threshold to positive values
        obj.KeepToolDownRatio = max(obj.KeepToolDownRatio, 0)

        # Limit linear deflection
        if obj.LinearDeflection.Value < 0.001:
            obj.LinearDeflection.Value = 0.001
            Path.Log.error("Linear Deflection must be between 0.001 and 25.4.")
        if obj.LinearDeflection.Value > 25.4:
            obj.LinearDeflection.Value = 25.4
            Path.Log.error("Linear Deflection must be between 0.001 and 25.4.")

        # Limit angular deflection
        if obj.AngularDeflection.Value < 0.001:
            obj.AngularDeflection.Value = 0.001
            Path.Log.error("Angular deflection must be between 0.001 to 25.4 millimeters.")
        if obj.AngularDeflection.Value > 25.4:
            obj.AngularDeflection.Value = 25.4
            Path.Log.error("Angular deflection must be between 0.001 to 25.4 millimeters.")

        # Limit sample interval
        if obj.SampleInterval.Value < 0.001:
            obj.SampleInterval.Value = 0.001
            Path.Log.error("Sample interval must be between 0.001 to 25.4 millimeters.")
        if obj.SampleInterval.Value > 25.4:
            obj.SampleInterval.Value = 25.4
            Path.Log.error("Sample interval must be between 0.001 to 25.4 millimeters.")

        # Limit min sample interval
        if obj.MinSampleInterval.Value < 0.001:
            obj.MinSampleInterval.Value = 0.001
            Path.Log.error("Min sample interval must be between 0.0001 to 25.4 millimeters.")
        if obj.MinSampleInterval.Value > 25.4:
            obj.MinSampleInterval.Value = 25.4
            Path.Log.error("Min sample interval must be between 0.0001 to 25.4 millimeters.")

        # Limit cut pattern angle
        if obj.CutPatternAngle < -360.0 or obj.CutPatternAngle >= 360.0:
            obj.CutPatternAngle = 0.0

        # Limit StepOver to natural number percentage
        obj.StepOver = min(obj.StepOver, 100.0)
        obj.StepOver = max(obj.StepOver, 1.0)

        # Limit AvoidLastX_Faces to zero and positive values
        if obj.AvoidLastX_Faces < 0:
            obj.AvoidLastX_Faces = 0
            Path.Log.error("AvoidLastX_Faces: Value must be 0 or greater.")
        if obj.AvoidLastX_Faces > 100:
            obj.AvoidLastX_Faces = 100
            Path.Log.error("AvoidLastX_Faces: Avoid last X faces count limited to 100.")

        # Limit Avoid Faces Overlap to zero and positive values
        if obj.AvoidFacesOverlap.Value < 0:
            obj.AvoidFacesOverlap.Value = 0.0

        # Limit StockToLeave to positive values
        obj.StockToLeave = max(obj.StockToLeave, 0)

        # Limit LeadFeed to natural number percentage
        obj.LeadFeed = min(obj.LeadFeed, 100.0)
        obj.LeadFeed = max(obj.LeadFeed, 1.0)

        # Limit LeadLiftDistance to positive values
        obj.LeadLiftDistance = max(obj.LeadLiftDistance, 0)

        # Limit Adaptive Helix max ramp angle
        if obj.HelixMaxRampAngle < 0.0 or obj.HelixMaxRampAngle >= 90.0:
            obj.HelixMaxRampAngle = 3.0

        # Limit Adaptive Helix Max Diameter percentage
        obj.HelixMaxDiameterPercent = min(obj.HelixMaxDiameterPercent, 100.0)
        obj.HelixMaxDiameterPercent = max(obj.HelixMaxDiameterPercent, 10.0)

        # Limit Adaptive Lift Distance to positive values
        obj.LiftDistance = max(obj.LiftDistance, 0)

        # Limit Adaptive Keep Tool Down Ratio to positive values
        obj.KeepToolDownThreshold = max(obj.KeepToolDownThreshold, 0)

        # Limit Volumetric Feed Percent
        obj.VolumetricFeedPercent = min(obj.VolumetricFeedPercent, 100.0)
        obj.VolumetricFeedPercent = max(obj.VolumetricFeedPercent, 0.0)

    def _rotatedShape(self, shape):
        """Return *shape* in the operation's working (Z-up) frame.

        When a 3+2 workplane rotation is active the base class wraps
        ``self.model`` / ``self.stock`` in transformed proxies for the duration
        of ``opExecute()`` and ``baseShapes()`` yields transformed geometry.
        ``updateDepths()`` runs before that wrapping, so depth calculations
        apply the geometry transform here. With no rotation active this is the
        identity.
        """
        matrix = getattr(self, "_geom_transform_matrix", None)
        if matrix is None:
            return shape
        return shape.copy().transformShape(matrix, False, False)

    def opUpdateDepths(self, obj):
        # All Z values below are in the working frame: baseShapes() yields
        # transformed geometry when a workplane rotation is active, and model /
        # stock shapes are transformed explicitly via _rotatedShape().
        if hasattr(obj, "Base") and obj.Base:
            zmin = float("inf")
            for base, sublist in self.baseShapes(obj):
                for sub in sublist:
                    try:
                        fbb = base.Shape.getElement(sub).BoundBox
                        zmin = min(zmin, fbb.ZMin)
                    except Part.OCCError as e:
                        Path.Log.error(e)
            if zmin != float("inf"):
                obj.OpFinalDepth = zmin
        elif self.job:
            if hasattr(obj, "BoundBox"):
                if obj.BoundBox == "BaseBoundBox":
                    models = getattr(self, "model", None) or self.job.Model.Group
                    zmin = min(self._rotatedShape(M.Shape).BoundBox.ZMin for M in models)
                    obj.OpFinalDepth = zmin
                if obj.BoundBox == "Stock":
                    stock = getattr(self, "stock", None) or self.job.Stock
                    obj.OpFinalDepth = self._rotatedShape(stock.Shape).BoundBox.ZMin

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

        Path.Log.debug(
            f"Surface tool: type={tool_type}, diameter={diameter}, edge_height={edge_height}, "
            f"corner_radius={corner_radius}, flat_radius={flat_radius}, edge_angle={edge_angle}"
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

    def _generate_scan_lines(
        self, obj, job, tool_diam, bb, bb_face, cutting_faces, avoid_boundary, is_whole_model_job
    ):
        """Generates the raw 2D scan line geometry for a given machining area."""

        # 1. Gather parameters
        pattern = obj.CutPattern
        boundary_adj = obj.BoundaryAdjustment.Value
        step_over = tool_diam * (obj.StepOver / 100.0)
        sample_interval = obj.SampleInterval.Value
        pattern_reverse = obj.CutPatternReversed
        cut_climb = obj.CutMode == "Climb"
        if pattern_reverse:
            cut_climb = not cut_climb

        # 2. Generate boundary mask
        boundary_face = surface_common.generate_pattern_mask(
            is_whole_model_job,
            bb_face,
            cutting_faces,
            avoid_boundary,
            tool_diam / 2.0,
            boundary_adj,
            obj.LinearDeflection.Value,
        )

        scan_bb = surface_pattern.BBox.from_bbox(bb)
        if not boundary_face and not cutting_faces:
            Path.Log.error("Failed to generate a valid boundary mask for the selected faces.")
            return []

        # 3. Generate Scan Lines (Main Logic)
        angle = obj.CutPatternAngle
        profile_mode = obj.ProfileEdges
        main_scan_lines = []
        profile_scan_lines = []

        # A. Generate Profile Scan
        if profile_mode != "None" and boundary_face:
            outer_wire = boundary_face.Wires[0]
            pts = outer_wire.discretize(Distance=sample_interval)
            if len(pts) >= 2:
                if (pts[0] - pts[-1]).Length > 1e-5:
                    pts.append(pts[0])
                profile_scan_lines.append([(p.x, p.y, 0.0) for p in pts])

        # B. Generate Main Pattern Scan
        if profile_mode != "Only":
            if pattern == "Offset":
                if boundary_face:
                    main_scan_lines = surface_pattern.generate_offset_scan_lines(
                        boundary_face,
                        step_over,
                        tool_diam,
                        sample_interval,
                        pattern_reverse,
                        cut_climb,
                    )
                else:
                    pattern = "Line"

            if pattern in ("Line", "ZigZag", "Circular", "CircularZigZag", "Spiral"):
                center_point = (scan_bb.center[0], scan_bb.center[1])  # Default
                if obj.PatternCenterAt == "Custom":
                    custom_center = obj.PatternCenterCustom
                    center_point = (custom_center.x, custom_center.y)
                is_zigzag = pattern in ("ZigZag", "CircularZigZag")

                main_scan_lines = surface_pattern.fast_generate_pattern(
                    pattern,
                    scan_bb,
                    center_point,
                    step_over,
                    sample_interval,
                    angle,
                    is_zigzag,
                    pattern_reverse,
                    cut_climb,
                    boundary_face,
                    obj.LinearDeflection.Value,
                )

        # C. Assemble final list based on Profile Mode
        if profile_mode == "First":
            return profile_scan_lines + main_scan_lines
        elif profile_mode == "Last":
            return main_scan_lines + profile_scan_lines
        elif profile_mode == "Only":
            return profile_scan_lines
        else:
            return main_scan_lines

    def _project_scan_lines(self, obj, stl, cutter, raw_scan_lines):
        """Projects raw 2D scan lines onto the 3D STL mesh using the optimal OCL algorithm."""

        scan_lines = []

        pattern = obj.CutPattern if hasattr(obj, "CutPattern") else "Line"
        is_adaptive = getattr(obj, "AdaptiveSampling", False)
        sample_interval = obj.SampleInterval.Value
        final_depth = obj.FinalDepth.Value

        adaptive_threshold = 0.30  # adaptive_threshold also in /Gui/Surface.py
        is_truly_adaptive = is_adaptive and sample_interval >= adaptive_threshold

        if is_adaptive and not is_truly_adaptive:
            Path.Log.info(
                f"SampleInterval ({sample_interval:.3f}mm) is below the adaptive threshold ({adaptive_threshold}mm)."
            )
            Path.Log.info("Switching to faster standard dropcutter for this high-density path.")

        if is_truly_adaptive:
            min_sampling = obj.MinSampleInterval.Value
            results_flat = surface_dropcutter.adaptive_path_dropcutter(
                stl, cutter, raw_scan_lines, final_depth, sample_interval, min_sampling
            )
        else:
            if pattern in ("Line", "ZigZag"):  # PathDropCutter
                results_flat = surface_dropcutter.path_dropcutter(
                    stl, cutter, raw_scan_lines, final_depth, sample_interval
                )
            else:  # (Circular, Spiral, Offset) - BatchDropCutter
                results_flat = surface_dropcutter.batch_dropcutter(
                    stl, cutter, raw_scan_lines, final_depth
                )

        # Reconstruct the results
        scan_lines = surface_pattern.reconstruct_scan_lines(results_flat, sample_interval * 2.5)

        return scan_lines

    def _executeSurfaceScan(
        self,
        obj,
        job,
        stl,
        safe_stl,
        cutter,
        tool_diam,
        bb_face,
        avoid_boundary=None,
        cutting_faces=None,
    ):
        """
        Executes the Surface Scan (projection) strategy.

        This is the primary function for generating toolpaths by projecting a 2D pattern
        onto a 3D STL mesh. It follows a highly optimized, multi-stage pipeline:

        Args:
            obj (Path::FeaturePython): The Planar Surface operation object.
            job (Path::Job): The parent Job object.
            stl (ocl.STLSurf): The primary OCL mesh for the toolpath calculation.
            safe_stl (ocl.STLSurf): A secondary OCL mesh including check surfaces, used for
                                   collision-safe "Keep Tool Down" transitions.
            cutter (ocl.Cutter): The OCL representation of the tool.
            tool_diam (float): The diameter of the active tool.
            bb (BoundBox): The bounding box of the entire operation area.
            cutting_faces (list, optional): A list of Part.Face objects if the user
                                             has made a specific selection. Defaults to None.
            avoid_boundary (Part.Shape, optional): Pre-built Avoid Faces "keep-out" boundary.

        Returns:
            list: A list of Path.Command objects representing the final G-code.
        """
        all_final_cmds = []

        is_whole_model_job = not cutting_faces
        sample_interval = obj.SampleInterval.Value
        force_keep_down = obj.CutPattern in ("ZigZag", "CircularZigZag")

        options = {
            "depth_offset": obj.DepthOffset.Value,
            "optimize_transitions": obj.KeepToolDown,
            "optimize_ratio": obj.KeepToolDownRatio,
            "safe_stl": safe_stl,
            "cutter": cutter,
            "force_keep_down": force_keep_down,
            "use_smart_leads": obj.LeadInOut,
            "lead_feed_percent": obj.LeadFeed,
            "lift_lead_z": obj.LeadLiftDistance.Value,
            "volumetric_percent": obj.VolumetricFeedPercent,
            "is_multipass": getattr(obj, "LayerMode", "Single-pass") == "Multi-pass",
        }

        # Ensure we have cutting faces (Fallback to whole model if none selected)
        if not cutting_faces:
            if not bb_face:
                Path.Log.error("Could not determine source faces for pattern generation.")
                return []
            cutting_faces = [bb_face]

        if bb_face is None:
            Path.Log.error("Could not determine the operation boundary face.")
            return []

        # Determine the bounding box
        group_bb = bb_face.BoundBox

        # Construct the list of face groups to process based on user's choice
        handle_mode = getattr(obj, "HandleMultipleFeatures", "Collectively")
        feature_groups = surface_pattern.group_features(cutting_faces, handle_mode)

        # For "Collectively", this loop runs once with all faces.
        # For "Individually", this loop runs once for each face.
        for i, face_group in enumerate(feature_groups):

            # A. Generate all 2D scan lines for this group
            raw_scan_lines = self._generate_scan_lines(
                obj,
                job,
                tool_diam,
                group_bb,
                bb_face,
                face_group,
                avoid_boundary,
                is_whole_model_job,
            )
            if not raw_scan_lines:
                continue

            # B. Project the scan lines onto the 3D STL
            scan_lines = self._project_scan_lines(obj, stl, cutter, raw_scan_lines)

            # C. Add linking moves if we are in "Individual" mode
            if all_final_cmds and len(feature_groups) > 1:
                all_final_cmds.append(
                    Path.Command("G0", {"Z": obj.SafeHeight.Value, "F": self.vertRapid})
                )

            # D. Multi-pass operation
            if getattr(obj, "LayerMode", "Single-pass") == "Multi-pass":
                scan_lines = surface_postprocess.apply_multipass(
                    scan_lines, obj.StartDepth.Value, obj.FinalDepth.Value, obj.StepDown.Value
                )

            # E. Post-process and generate G-code for this group
            if obj.OptimizeLinearPaths:
                scan_lines = [
                    surface_postprocess.filter_cl_points(line, 0.005) for line in scan_lines
                ]

            # F. Generate G-Code
            group_cmds = surface_postprocess.scan_lines_to_gcode(
                scan_lines,
                sample_interval=sample_interval,
                horiz_feed=self.horizFeed,
                vert_feed=self.vertFeed,
                vert_rapid=self.vertRapid,
                horiz_rapid=self.horizRapid,
                safe_z=obj.SafeHeight.Value,
                clearance_z=obj.ClearanceHeight.Value,
                start_z=obj.StartDepth.Value,
                final_z=obj.FinalDepth.Value,
                step_down=obj.StepDown.Value,
                options=options,
            )
            all_final_cmds.extend(group_cmds)

        return all_final_cmds

    def _executeWaterline(self, obj, job, stl, cutter, tool_diam, is_adaptive=False):
        """Execute the Waterline strategy using Phase 1 generators.

        Flow:
        1. Calculate Z-height range from depths
        2. Run waterline_stack at multiple Z-heights
        3. Convert to G-code
        """
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
                    # Filter out samll fragments
                    if len(loop) < 3:
                        continue
                    filter_loop.append(surface_postprocess.filter_cl_points(loop, tolerance))
                wl_data[zh] = filter_loop

        cmds = surface_waterline.waterline_to_gcode(
            wl_data,
            horiz_feed=self.horizFeed,
            vert_feed=self.vertFeed,
            horiz_rapid=self.horizRapid,
            vert_rapid=self.vertRapid,
            safe_z=obj.SafeHeight.Value,
            clearance_z=obj.ClearanceHeight.Value,
            cut_climb=cut_climb,
        )

        return cmds

    def _executeZLevelHybrid(self, obj, job, shape, bb_face, tool_params):
        """Execute the Z-Level Hybrid strategy (no OCL required).

        A high-precision geometric finishing strategy that operates directly on
        B-Rep geometry. It combines constant-Z contouring with automatic floor
        detection and clearing.

        Flow:
        1. Extract ToolBit parameters for specific 3D profile math.
        2. Data preparation
        3. Generate mask for Fill selected holes feature
        4. Generate master boundary (TrimFace) and stable background pool.
        5. Categorize depths, reconciling standard steps with physical model floors.
        6. Dispatch to surface_zlevel generator for C++ accelerated geometry stacking.
        7. Convert the resulting geometry stack into optimized G-code Path commands.
        """
        from Path.Base.Generator import surface_zlevel

        # 1. Extract and Validate Tool Parameters
        tool_diam = tool_params.get("diameter", 0.0)
        radius = tool_diam / 2.0
        shape_type = tool_params.get("tool_type") or ""
        c_rad = tool_params.get("corner_radius", 0.0)
        is_3d = shape_type in ("ballend", "bullnose")

        if tool_diam == 0.0 or (not is_3d and "endmill" not in shape_type):
            Path.Log.error(
                f"The Z-Level Hybrid strategy requires a Ball-end, Bull-nose, or flat Endmill. Found: '{shape_type}'."
            )
            return []

        # 2. Data Preparation & Options
        wpc = Part.makeCircle(1.0, FreeCAD.Vector(0, 0, 0), FreeCAD.Vector(0, 0, 1))
        shape_copy = shape.copy()
        fill_holes_masks = []

        is_adaptive = getattr(obj, "CutPatternZLevel", "None") == "Adaptive"
        enforce_goefence = getattr(obj, "EnforceGeofence", True)
        fill_selected_holes = getattr(obj, "FillSelectedHoles", False)
        clear_planar_only = getattr(obj, "ClearPlanarOnly", True)
        ignore_outer = getattr(obj, "IgnoreOuter", False)

        accuracy_val = getattr(obj, "SamplingAccuracy", "4")
        step_over = (obj.StepOver / 100.0) * tool_diam
        stock_to_leave = obj.StockToLeave.Value
        depth_offset = obj.DepthOffset.Value

        # Start Point handling
        start_point = obj.StartPoint if getattr(obj, "UseStartPoint", False) else None

        zlevel_tool_params = {
            "radius": radius,
            "c_rad": c_rad,
            "profile": shape_type,
            "is_threeD": is_3d,
        }

        pattern_options = {
            "cut_climb": obj.CutMode == "Climb",
            "cut_pattern": getattr(obj, "CutPatternZLevel", "None"),
            "pattern_angle": getattr(obj, "CutPatternAngle", 0.0),
            "reverse_pattern": getattr(obj, "CutPatternReversed", False),
            "keep_tool_down": getattr(obj, "KeepToolDown", True),
            "keep_down_ratio": getattr(obj, "KeepToolDownRatio", 2.0),
        }

        height_params = {
            "safe_hght": obj.SafeHeight.Value,
            "clearance_hght": obj.ClearanceHeight.Value,
            "start_hght": obj.StartDepth.Value,
        }

        feed_params = {
            "horizFeed": self.horizFeed,
            "vertFeed": self.vertFeed,
            "horizRapid": self.horizRapid,
            "vertRapid": self.vertRapid,
        }

        adaptive_params = {
            "op_type": "ClearingInside",
            "adaptive_accuracy": getattr(obj, "AdaptiveAccuracy", 0.1),
            "stock_to_leave": stock_to_leave,
            "lift_distance": getattr(obj, "LiftDistance", 0.05),
            "keep_tool_down": getattr(obj, "KeepToolDownThreshold", 3.0),
            "force_insideout": getattr(obj, "ForceInsideOut", False),
            "finishing_profile": getattr(obj, "FinishingProfile", True),
            "helix_angle": getattr(obj, "HelixMaxRampAngle", 3.0),
            "helix_cone_angle": 0.0,
            "helix_diameter": getattr(obj, "HelixMaxDiameterPercent", 75),
            "helix_min_diameter": tool_diam * 0.10,
        }

        # 3. Fill selected holes
        if fill_selected_holes:
            base_prop = list(self.baseShapes(obj))
            fill_holes_masks = surface_zlevel.fill_selected(base_prop)

        # 4. Boundary preparation
        buffer = tool_diam + obj.BoundaryAdjustment.Value
        border_poly = surface_zlevel.extendedBoundBox(self.stock.Shape.BoundBox, buffer, 0.0)
        border_face = Part.makeFace(border_poly)

        trim_face = surface_zlevel.getTrimFace(border_face, bb_face, wpc)

        # 5. Depth categorization
        cat_steps = surface_zlevel.categorize_floor_steps(
            shape_copy,
            obj.StartDepth.Value,
            obj.FinalDepth.Value,
            obj.StepDown.Value,
            clear_planar_only,
        )

        # 6. Generate Geometry Stack
        wl_data = surface_zlevel.zlevel_hybrid_stack(
            shape,
            cat_steps,
            border_face,
            trim_face,
            fill_holes_masks,
            zlevel_tool_params,
            stock_to_leave,
            accuracy_val,
            depth_offset,
            wpc,
            start_z=obj.StartDepth.Value,
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
            start_point,
            radius,
            is_adaptive,
            adaptive_params,
            bb_face,
            enforce_goefence,
        )

        return cmds

    def opExecute(self, obj):
        """Main execution method for Planar Surface operation.

        This function orchestrates the entire toolpath generation process by following a
        clean, multi-phase pipeline:

        1.  Universal Setup: Initializes the Job, applies property limits, updates
            depths from the Base geometry, and extracts core parameters like the
            strategy and tool information. This phase runs for all strategies.
        2.  Data Preparation: Intelligently prepares only the necessary geometric
            data (STL meshes, OCL cutters, boundary boxes) based on the specific
            requirements of the selected strategy.
        3.  Strategy Dispatch: A simple, clean router that calls the appropriate
            backend execution function (e.g., _executeSurfaceScan, _executeWaterline)
            and passes it the prepared data.
        4.  G-Code Finalization: Assembles the final command list by prepending
            standard headers and startup moves to the commands returned by the
            strategy function.
        """
        Path.Log.track()

        startTime = time.time()

        # Universal Setup
        JOB = PathUtils.findParentJob(obj)
        if JOB is None:
            Path.Log.error(translate("CAM_PlanarSurface", "No JOB"))
            return

        # Impose property limits
        self.opApplyPropertyLimits(obj)

        # Extract basic operation data
        strategy = obj.Strategy
        is_adaptive = getattr(obj, "AdaptiveSampling", False)
        boundary_adjustment = obj.BoundaryAdjustment.Value

        tool_params = self._extractToolParams(obj)
        tool_diam = tool_params.get("diameter", 0.0)
        tool_radius = tool_diam / 2.0
        avoid_overlap = -obj.AvoidFacesOverlap.Value

        # Initialize geometric and OCL containers
        cutter = stl = safe_stl = stl_faces = None
        cutting_faces = avoid_faces = bb_face = None

        # Base Strategy Flags
        is_surface_scan = strategy == "SurfaceScan"
        is_waterline = strategy == "Waterline"
        is_zlevel = strategy == "ZLevelHybrid"

        # Geometry & Generation Requirements
        needs_face_selection = is_surface_scan
        needs_boundary = is_surface_scan or is_zlevel
        needs_stl = is_surface_scan or is_waterline
        needs_ocl_cutter = needs_stl

        # Contextual Requirements
        needs_safe_stl = (
            getattr(obj, "KeepToolDown", False) or getattr(obj, "LeadInOut", False)
        ) and is_surface_scan

        # STL Mesh optimization
        optimize_stl = getattr(obj, "OptimizeMeshConversion", True)
        stl_filter_adj = boundary_adjustment

        if needs_stl and getattr(obj, "LeadInOut", False):
            stl_filter_adj = max(tool_radius, boundary_adjustment)

        # Override cutter length for Waterline Strategy
        if is_waterline:
            # Ensure the OCL cutter shaft is at least as long as the operation 'depth - edge_height'
            # so it cannot pass through vertical walls removed by mesh optimization.
            op_depth = obj.StartDepth.Value - obj.FinalDepth.Value
            tool_params["length_offset"] = op_depth + tool_params["edge_height"]

        # Geometry preparation. self.model / self.stock are provided by the base
        # class and are already in the working frame when a 3+2 workplane
        # rotation is active (see ObjectOp.execute); never read JOB.Model or
        # JOB.Stock directly here or the rotation would be silently bypassed.
        base_objs = self.model
        if not base_objs:
            Path.Log.error("No models found in Job.")
            return

        if getattr(self, "_geom_transform_matrix", None) is not None and any(
            not hasattr(b, "Shape") for b in base_objs
        ):
            # Mesh::Feature bases carry no .Shape, so the base class cannot
            # rotate them. Refuse rather than cut an unrotated mesh on a
            # rotated setup.
            Path.Log.error(
                translate(
                    "CAM_PlanarSurface",
                    "Mesh base objects are not supported with a rotated Workplane.",
                )
            )
            return

        valid_shapes = []
        for b in base_objs:
            shp = getattr(b, "Shape", None)
            if shp is not None and not shp.isNull():
                valid_shapes.append(shp.copy())
        if len(valid_shapes) > 1:
            try:
                # Melt overlapping models into one clean continuous object
                model_shape = valid_shapes[0].fuse(valid_shapes[1:])
                if hasattr(model_shape, "removeSplitter"):
                    model_shape = model_shape.removeSplitter()
            except (Part.OCCError, RuntimeError, ValueError) as e:
                Path.Log.warning(f"Boolean fuse failed, falling back to Compound: {e}")
                model_shape = Part.Compound(valid_shapes)
        elif len(valid_shapes) == 1:
            model_shape = valid_shapes[0]
        else:
            Path.Log.error("No valid shapes found to machine.")
            return

        # Split selected features
        if needs_face_selection:
            base_prop = list(self.baseShapes(obj))
            avoid_count = getattr(obj, "AvoidLastX_Faces", 0)
            cutting_faces, avoid_faces = surface_pattern.split_selected_features(
                base_prop, avoid_count
            )
            if obj.BoundBox not in ["Stock"]:
                # Send selected faces to STL optimization filter
                stl_faces = cutting_faces

        # Create boundary face
        if needs_boundary:
            offset = obj.BoundaryAdjustment.Value - tool_radius - 0.01

            if obj.BoundBox == "Stock":
                bb_face = surface_common.create_boundary_face(self.stock.Shape.Faces, offset)
            elif cutting_faces:
                # Combine bounding boxes and explicitly convert to a 2D Part.Face
                from functools import reduce

                combined_bb = reduce(lambda a, b: a.united(b), [f.BoundBox for f in cutting_faces])

                p1 = FreeCAD.Vector(combined_bb.XMin - offset, combined_bb.YMin - offset, 0)
                p2 = FreeCAD.Vector(combined_bb.XMax + offset, combined_bb.YMin - offset, 0)
                p3 = FreeCAD.Vector(combined_bb.XMax + offset, combined_bb.YMax + offset, 0)
                p4 = FreeCAD.Vector(combined_bb.XMin - offset, combined_bb.YMax + offset, 0)

                bb_face = Part.Face(Part.makePolygon([p1, p2, p3, p4, p1]))
            else:
                # Create a boundary from model_shape
                bb_face = surface_common.create_boundary_face(
                    model_shape.Faces, offset, avoids=False, model_boundary=True
                )

        # Avoid Faces processing
        avoid_boundary = None

        if avoid_faces:
            avoid_boundary = surface_common.build_avoid_boundary(
                avoid_faces,
                avoid_overlap,
                obj.LinearDeflection.Value,
            )

        # Create OCL cutter from tool parameters
        if needs_ocl_cutter:
            cutter = surface_common.make_ocl_cutter(
                tool_params["tool_type"],
                tool_params["diameter"],
                edge_height=tool_params["edge_height"],
                corner_radius=tool_params["corner_radius"],
                flat_radius=tool_params["flat_radius"],
                edge_angle=tool_params["edge_angle"],
                length_offset=tool_params["length_offset"],
            )
            if cutter is None:
                Path.Log.error(
                    translate(
                        "CAM_PlanarSurface", "Error creating OCL cutter from tool parameters."
                    )
                )
                return

            tool_diam = cutter.getDiameter()
            Path.Log.debug(
                f"Surface OCL cutter created: getDiameter()={tool_diam}, StepOver={obj.StepOver}%, "
                f"stepover_dist={tool_diam * (obj.StepOver / 100.0)}"
            )

        # Generate primary and secondary STL meshes
        if needs_stl:
            Path.Log.info(
                f"STL creation — "
                f"LinearDeflection={round(obj.LinearDeflection.Value, 4)}mm, "
                f"AngularDeflection={round(obj.AngularDeflection.Value, 4)}°, "
                f"MeshSimplification*={getattr(obj, 'MeshSimplification', 1)}, "
            )

            stl_start = time.time()

            stl, safe_stl = surface_mesh.generate_stl(
                model_shape=model_shape,
                base_objs=base_objs,
                optimize_stl=optimize_stl,
                strategy=strategy,
                stl_faces=stl_faces,
                stl_filter_adj=stl_filter_adj,
                bb_face=bb_face,
                avoid_boundary=avoid_boundary,
                tool_diam=tool_diam,
                needs_safe_stl=needs_safe_stl,
                boundary_adjustment=boundary_adjustment,
                start_depth=obj.StartDepth.Value,
                final_depth=obj.FinalDepth.Value,
                linear_deflection=obj.LinearDeflection.Value,
                angular_deflection=obj.AngularDeflection.Value,
                mesh_simplification=getattr(obj, "MeshSimplification", 1),
            )
            stl_time = time.time() - stl_start

            Path.Log.info(f"STL creation took {stl_time:.3f}s")
            if stl is None:
                Path.Log.error(
                    "Failed to create a valid Mesh from the model (Check the Start and Final Depth)."
                )
                return

        # Begin GCode for operation with basic information
        if obj.Comment != "":
            self.commandlist.append(Path.Command(f"N ({obj.Comment!s})", {}))
        self.commandlist.append(Path.Command(f"N ({obj.Label})", {}))
        self.commandlist.append(Path.Command(f"N (Strategy: {strategy})", {}))
        self.commandlist.append(
            Path.Command("N (Tool diameter: {:.3f})".format(tool_params["diameter"]), {})
        )
        if not is_zlevel:
            self.commandlist.append(
                Path.Command(f"N (Sample interval: {obj.SampleInterval.Value!s})", {})
            )
        self.commandlist.append(Path.Command(f"N (Step over %: {obj.StepOver!s})", {}))
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
        if strategy == "SurfaceScan":
            cmds = self._executeSurfaceScan(
                obj, JOB, stl, safe_stl, cutter, tool_diam, bb_face, avoid_boundary, cutting_faces
            )
        elif strategy == "Waterline":
            cmds = self._executeWaterline(obj, JOB, stl, cutter, tool_diam, is_adaptive=is_adaptive)
        elif strategy == "ZLevelHybrid":
            cmds = self._executeZLevelHybrid(obj, JOB, model_shape, bb_face, tool_params)
        self.commandlist.extend(cmds)

        elapsed = time.time() - startTime
        hours, remainder = divmod(elapsed, 3600)
        minutes, seconds = divmod(remainder, 60)

        Path.Log.info(
            f"Surface operation completed in {hours:02.0f}h:{minutes:02.0f}m:{seconds:05.2f}s"
        )


def Create(name, obj=None, parentJob=None):
    """Create(name) ... Creates and returns a Planar Surface operation."""
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
    setup.append("CutPatternZLevel")
    setup.append("SamplingAccuracy")
    setup.append("StockToLeave")
    setup.append("ClearPlanarOnly")
    setup.append("IgnoreOuter")
    setup.append("FillSelectedHoles")
    setup.append("OptimizeLinearPaths")
    setup.append("OptimizeMeshConversion")
    setup.append("SampleInterval")
    setup.append("AdaptiveSampling")
    setup.append("MinSampleInterval")
    setup.append("LinearDeflection")
    setup.append("AngularDeflection")
    setup.append("MeshSimplification")
    setup.append("HandleMultipleFeatures")
    setup.append("ProfileEdges")
    setup.append("BoundaryAdjustment")
    setup.append("PatternCenterAt")
    setup.append("PatternCenterCustom")
    setup.append("AvoidLastX_Faces")
    setup.append("AvoidFacesOverlap")
    setup.append("KeepToolDown")
    setup.append("KeepToolDownRatio")
    setup.append("GapThreshold")
    setup.append("UseStartPoint")
    setup.append("StartPoint")
    setup.append("LeadInOut")
    setup.append("LeadFeed")
    setup.append("LeadLiftDistance")
    setup.append("AdaptiveAccuracy")
    setup.append("LiftDistance")
    setup.append("KeepToolDownThreshold")
    setup.append("ForceInsideOut")
    setup.append("FinishingProfile")
    setup.append("HelixMaxRampAngle")
    setup.append("HelixMaxDiameterPercent")
    setup.append("EnforceGeofence")

    return setup
