# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 sliptonic <shopinthewoods@gmail.com>
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


__title__ = "CAM 3D Surface Operation"
__author__ = "sliptonic (Brad Collette)"
__url__ = "https://www.freecad.org"
__doc__ = "3D surface finishing operation built on the surface_* generators."


import FreeCAD
from PySide import QtCore

import Path

if FreeCAD.GuiUp:
    import FreeCADGui


def _yield_to_gui():
    """Pump the Qt event loop so long OCL runs don't trigger an OS
    'Application Not Responding' warning.  No-op when running headless.
    """
    if FreeCAD.GuiUp:
        FreeCADGui.updateGui()


import Path.Op.Base as PathOp
import Path.Base.Generator.surface_common as surface_common
import Path.Base.Generator.surface_dropcutter as surface_dropcutter
import Path.Base.Generator.surface_mesh as surface_mesh
import Path.Base.Generator.surface_pattern as surface_pattern
import Path.Base.Generator.surface_postprocess as surface_postprocess
import Path.Base.Generator.surface_waterline as surface_waterline
import Path.Base.Generator.surface_zlevel as surface_zlevel
import PathScripts.PathUtils as PathUtils

# OCL is loaded lazily so the module can be imported when OCL is missing.
try:
    import ocl  # noqa: F401
except ImportError:  # pragma: no cover
    try:
        import opencamlib as ocl  # noqa: F401
    except ImportError:  # pragma: no cover
        ocl = None


translate = FreeCAD.Qt.translate


if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


class ObjectSurface3D(PathOp.ObjectOp):
    """3D surface finishing operation.

    Strategies (full set arrives in later slices):
      - SurfacePattern: project a 2D pattern (Line/ZigZag/Circular/Spiral)
        onto the model surface via OCL drop-cutter.
      - Waterline: constant-Z contours via OCL (added in a later slice).
      - ZLevelHybrid: native, no-OCL constant-Z via Path.Area Clipper
        (added in a later slice).
    """

    def opFeatures(self, obj):
        return (
            PathOp.FeatureTool
            | PathOp.FeatureDepths
            | PathOp.FeatureHeights
            | PathOp.FeatureStepDown
            | PathOp.FeatureCoolant
            | PathOp.FeatureBaseFaces
        )

    def initOperation(self, obj):
        self.propertiesReady = False
        self.initOpProperties(obj)

    def opOnDocumentRestored(self, obj):
        self.initOpProperties(obj, warn=True)
        self.setEditorProperties(obj)

    def initOpProperties(self, obj, warn=False):
        Path.Log.track()
        self.addNewProps = list()
        for prtyp, nm, grp, tt in self.opPropertyDefinitions():
            if not hasattr(obj, nm):
                obj.addProperty(prtyp, nm, grp, tt)
                self.addNewProps.append(nm)

        if len(self.addNewProps) > 0:
            ENUMS = self.propertyEnumerations()
            for n in ENUMS:
                if n[0] in self.addNewProps:
                    setattr(obj, n[0], n[1])
            if warn:
                msg = translate("CAM_Surface3D", "New property added to")
                msg += ' "{}": {}'.format(obj.Label, self.addNewProps) + ". "
                msg += translate("CAM_Surface3D", "Check the default values.")
                FreeCAD.Console.PrintWarning(msg + "\n")

        self.propertiesReady = True

    def opPropertyDefinitions(self):
        return [
            (
                "App::PropertyEnumeration",
                "Strategy",
                "Strategy",
                QtCore.QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Top-level finishing strategy. Surface Pattern projects a 2D pattern "
                    "onto the surface via drop-cutter. Waterline and Z-Level Hybrid "
                    "are also available.",
                ),
            ),
            (
                "App::PropertyEnumeration",
                "CutPattern",
                "Pattern",
                QtCore.QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Pattern projected onto the surface for SurfacePattern strategy.",
                ),
            ),
            (
                "App::PropertyAngle",
                "CutPatternAngle",
                "Pattern",
                QtCore.QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Rotation of Line/ZigZag scan lines (degrees).",
                ),
            ),
            (
                "App::PropertyBool",
                "CutPatternReversed",
                "Pattern",
                QtCore.QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Reverse the direction in which the pattern is generated.",
                ),
            ),
            (
                "App::PropertyEnumeration",
                "CutMode",
                "Pattern",
                QtCore.QT_TRANSLATE_NOOP("App::Property", "Climb or Conventional cutting."),
            ),
            (
                "App::PropertyDistance",
                "StepOver",
                "Pattern",
                QtCore.QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Distance between adjacent scan lines (mm).",
                ),
            ),
            (
                "App::PropertyDistance",
                "SampleInterval",
                "Pattern",
                QtCore.QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Distance between sampled points along a scan line (mm). "
                    "For Waterline this is the fiber spacing.",
                ),
            ),
            (
                "App::PropertyDistance",
                "MinSampleInterval",
                "Waterline",
                QtCore.QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Minimum fiber spacing for adaptive refinement in the " "Waterline strategy.",
                ),
            ),
            (
                "App::PropertyDistance",
                "StockToLeave",
                "ZLevel",
                QtCore.QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Horizontal (XY) finishing allowance left on the model "
                    "surface for the ZLevelHybrid strategy.",
                ),
            ),
            (
                "App::PropertyEnumeration",
                "SamplingAccuracy",
                "ZLevel",
                QtCore.QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Number of sub-slices per Z layer for ZLevelHybrid (1=coarse, "
                    "7=fine). Higher values capture more curvature detail.",
                ),
            ),
            (
                "App::PropertyBool",
                "ClearPlanarOnly",
                "ZLevel",
                QtCore.QT_TRANSLATE_NOOP(
                    "App::Property",
                    "ZLevelHybrid only: clear floor regions detected as Mixed/Extra "
                    "(planar pockets) instead of every layer.",
                ),
            ),
            (
                "App::PropertyBool",
                "IgnoreOuter",
                "ZLevel",
                QtCore.QT_TRANSLATE_NOOP(
                    "App::Property",
                    "ZLevelHybrid only: skip the outermost stock-edge contour.",
                ),
            ),
            (
                "App::PropertyEnumeration",
                "CutPatternZLevel",
                "ZLevel",
                QtCore.QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Floor-clearing pattern used after each ZLevel contour pass.",
                ),
            ),
            (
                "App::PropertyDistance",
                "DepthOffset",
                "Pattern",
                QtCore.QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Constant Z offset added to every projected point (e.g. finish "
                    "stock-to-leave).",
                ),
            ),
            (
                "App::PropertyDistance",
                "BoundaryAdjustment",
                "Pattern",
                QtCore.QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Outward (+) or inward (-) offset applied to the boundary mask, "
                    "added on top of the tool radius.",
                ),
            ),
            (
                "App::PropertyEnumeration",
                "HandleMultipleFeatures",
                "Pattern",
                QtCore.QT_TRANSLATE_NOOP(
                    "App::Property",
                    "How to treat multiple selected faces: process collectively or "
                    "individually (with retracts between).",
                ),
            ),
            (
                "App::PropertyBool",
                "OptimizeLinearPaths",
                "Optimization",
                QtCore.QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Drop collinear sample points before emitting G-code.",
                ),
            ),
            (
                "App::PropertyInteger",
                "AccuracyLevel",
                "Mesh",
                QtCore.QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Speed-vs-Accuracy preset (1=Fastest..7=Ultra). Sets "
                    "LinearDeflection, AngularDeflection, SampleInterval, "
                    "MinSampleInterval, and MeshSimplification together.",
                ),
            ),
            (
                "App::PropertyInteger",
                "MeshSimplification",
                "Mesh",
                QtCore.QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Mesh decimation level (1=no reduction, 7=maximum).",
                ),
            ),
            (
                "App::PropertyDistance",
                "LinearDeflection",
                "Mesh",
                QtCore.QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Tessellation linear deflection. Smaller = finer mesh.",
                ),
            ),
            (
                "App::PropertyAngle",
                "AngularDeflection",
                "Mesh",
                QtCore.QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Tessellation angular deflection. Smaller = finer mesh.",
                ),
            ),
        ]

    @classmethod
    def propertyEnumerations(cls, dataType="data"):
        Path.Log.track()
        enums = {
            "Strategy": [
                (translate("CAM_Surface3D", "Surface Pattern"), "SurfacePattern"),
                (translate("CAM_Surface3D", "Waterline"), "Waterline"),
                (translate("CAM_Surface3D", "Z-Level Hybrid"), "ZLevelHybrid"),
            ],
            "CutPattern": [
                (translate("CAM_Surface3D", "Line"), "Line"),
                (translate("CAM_Surface3D", "ZigZag"), "ZigZag"),
                (translate("CAM_Surface3D", "Circular"), "Circular"),
                (translate("CAM_Surface3D", "Spiral"), "Spiral"),
            ],
            "CutPatternZLevel": [
                (translate("CAM_Surface3D", "None"), "None"),
                (translate("CAM_Surface3D", "Line"), "Line"),
                (translate("CAM_Surface3D", "Spiral"), "Spiral"),
            ],
            "CutMode": [
                (translate("CAM_Surface3D", "Climb"), "Climb"),
                (translate("CAM_Surface3D", "Conventional"), "Conventional"),
            ],
            "HandleMultipleFeatures": [
                (translate("CAM_Surface3D", "Collectively"), "Collectively"),
                (translate("CAM_Surface3D", "Individually"), "Individually"),
            ],
            "SamplingAccuracy": [
                ("1", "1"),
                ("2", "2"),
                ("3", "3"),
                ("4", "4"),
                ("5", "5"),
                ("6", "6"),
                ("7", "7"),
            ],
        }
        if dataType == "raw":
            return enums
        data = list()
        idx = 0 if dataType == "translated" else 1
        for k in enums:
            data.append((k, [tup[idx] for tup in enums[k]]))
        return data

    def opPropertyDefaults(self, obj, job):
        return {
            "Strategy": "SurfacePattern",
            "CutPattern": "ZigZag",
            "CutPatternZLevel": "None",
            "CutPatternAngle": 0.0,
            "CutPatternReversed": False,
            "CutMode": "Climb",
            "StepOver": 1.0,
            "SampleInterval": 0.4,
            "MinSampleInterval": 0.05,
            "StockToLeave": 0.0,
            "SamplingAccuracy": "4",
            "ClearPlanarOnly": True,
            "IgnoreOuter": False,
            "DepthOffset": 0.0,
            "BoundaryAdjustment": 0.0,
            "HandleMultipleFeatures": "Collectively",
            "OptimizeLinearPaths": True,
            "AccuracyLevel": 4,
            "MeshSimplification": 4,
            "LinearDeflection": 0.025,
            "AngularDeflection": 0.2,
        }

    def opSetDefaultValues(self, obj, job):
        Path.Log.track()
        defaults = self.opPropertyDefaults(obj, job)
        for name, value in defaults.items():
            if hasattr(obj, name):
                setattr(obj, name, value)
        # Drive deflection / sampling / decimation from the AccuracyLevel
        # preset so the table in opPropertyDefaults and ACCURACY_PRESETS
        # can't drift apart.
        self.apply_accuracy_preset(obj, getattr(obj, "AccuracyLevel", 4))

    def opUpdateDepths(self, obj):
        """Default FinalDepth to the lowest Z of selected faces (or model).

        The base ``updateDepths`` sets ``OpFinalDepth`` to the *top* of
        the model when no Base is selected, which would cause
        ``surface_mesh.generate_stl``'s pre-clip to produce an empty
        volume.  For a surfacing op the natural floor is the bottom of
        the geometry being machined, so override accordingly.
        """
        zmin = None
        if hasattr(obj, "Base") and obj.Base:
            for base, sublist in obj.Base:
                for sub in sublist:
                    try:
                        fbb = base.Shape.getElement(sub).BoundBox
                    except Exception:
                        continue
                    zmin = fbb.ZMin if zmin is None else min(zmin, fbb.ZMin)
        if zmin is None and getattr(self, "job", None):
            for m in self.job.Model.Group:
                if not getattr(m, "Shape", None) or m.Shape.isNull():
                    continue
                bb = m.Shape.BoundBox
                zmin = bb.ZMin if zmin is None else min(zmin, bb.ZMin)
        if zmin is not None:
            obj.OpFinalDepth = zmin

    # ------------------------------------------------------------------
    # Speed-vs-Accuracy presets
    # ------------------------------------------------------------------

    ACCURACY_PRESETS = {
        1: {
            "linear_deflection": 0.1,
            "angular_deflection": 0.5,
            "sample_interval": 1.5,
            "min_sample_interval": 0.3,
            "mesh_simplification": 7,
        },
        2: {
            "linear_deflection": 0.075,
            "angular_deflection": 0.4,
            "sample_interval": 1.0,
            "min_sample_interval": 0.20,
            "mesh_simplification": 6,
        },
        3: {
            "linear_deflection": 0.05,
            "angular_deflection": 0.3,
            "sample_interval": 0.5,
            "min_sample_interval": 0.10,
            "mesh_simplification": 5,
        },
        4: {
            "linear_deflection": 0.025,
            "angular_deflection": 0.2,
            "sample_interval": 0.25,
            "min_sample_interval": 0.05,
            "mesh_simplification": 4,
        },
        5: {
            "linear_deflection": 0.015,
            "angular_deflection": 0.15,
            "sample_interval": 0.1,
            "min_sample_interval": 0.05,
            "mesh_simplification": 3,
        },
        6: {
            "linear_deflection": 0.01,
            "angular_deflection": 0.1,
            "sample_interval": 0.075,
            "min_sample_interval": 0.025,
            "mesh_simplification": 2,
        },
        7: {
            "linear_deflection": 0.005,
            "angular_deflection": 0.05,
            "sample_interval": 0.05,
            "min_sample_interval": 0.01,
            "mesh_simplification": 1,
        },
    }

    def apply_accuracy_preset(self, obj, level):
        """Push ACCURACY_PRESETS values onto obj for the given level (1-7)."""
        level = max(1, min(7, int(level)))
        preset = self.ACCURACY_PRESETS[level]
        if hasattr(obj, "LinearDeflection"):
            obj.LinearDeflection = preset["linear_deflection"]
        if hasattr(obj, "AngularDeflection"):
            obj.AngularDeflection = preset["angular_deflection"]
        if hasattr(obj, "SampleInterval"):
            obj.SampleInterval = preset["sample_interval"]
        if hasattr(obj, "MinSampleInterval"):
            obj.MinSampleInterval = preset["min_sample_interval"]
        if hasattr(obj, "MeshSimplification"):
            obj.MeshSimplification = preset["mesh_simplification"]

    def opOnChanged(self, obj, prop):
        if prop == "AccuracyLevel" and getattr(self, "propertiesReady", False):
            self.apply_accuracy_preset(obj, obj.AccuracyLevel)
        if prop == "Strategy" and getattr(self, "propertiesReady", False):
            self.setEditorProperties(obj)

    def setEditorProperties(self, obj):
        """Hide strategy-irrelevant properties from the property editor.

        Only properties tied to a specific strategy are toggled.  Common
        properties (Strategy, depths, mesh, tool) stay visible always.
        """
        if not getattr(self, "propertiesReady", False):
            return
        strategy = getattr(obj, "Strategy", "SurfacePattern")

        # Map: property -> strategies that USE it ({"All"} means always show)
        usage = {
            "CutPattern": {"SurfacePattern"},
            "CutPatternAngle": {"SurfacePattern", "ZLevelHybrid"},
            "CutPatternReversed": {"SurfacePattern", "ZLevelHybrid"},
            "BoundaryAdjustment": {"SurfacePattern"},
            "HandleMultipleFeatures": {"SurfacePattern"},
            "OptimizeLinearPaths": {"SurfacePattern"},
            "MinSampleInterval": {"Waterline"},
            "StockToLeave": {"ZLevelHybrid"},
            "SamplingAccuracy": {"ZLevelHybrid"},
            "ClearPlanarOnly": {"ZLevelHybrid"},
            "IgnoreOuter": {"ZLevelHybrid"},
            "CutPatternZLevel": {"ZLevelHybrid"},
            "MeshSimplification": {"SurfacePattern", "Waterline"},
        }
        for prop, strategies in usage.items():
            if not hasattr(obj, prop):
                continue
            mode = 0 if strategy in strategies else 2  # 2 = hidden
            obj.setEditorMode(prop, mode)

    # ------------------------------------------------------------------
    # Tool extraction
    # ------------------------------------------------------------------

    def _extractToolParams(self, obj):
        """Pull cutter parameters off the active ToolController."""
        tc = obj.ToolController
        tool = tc.Tool

        tool_type = None
        if hasattr(tool, "ShapeType") and tool.ShapeType:
            tool_type = str(tool.ShapeType).lower()
        elif hasattr(tool, "ShapeName") and tool.ShapeName:
            tool_type = str(tool.ShapeName).lower()

        diameter = float(getattr(tool, "Diameter", 0.0))
        flat_radius = float(getattr(tool, "FlatRadius", 0.0))
        corner_radius = float(getattr(tool, "CornerRadius", 0.0))
        if corner_radius and diameter:
            flat_radius = max(diameter / 2.0 - corner_radius, 0.0)
        edge_height = float(getattr(tool, "CuttingEdgeHeight", 0.0))
        edge_angle = float(getattr(tool, "CuttingEdgeAngle", 0.0))
        length_offset = float(getattr(tool, "LengthOffset", 0.0))

        return {
            "tool_type": tool_type,
            "diameter": diameter,
            "corner_radius": corner_radius,
            "flat_radius": flat_radius,
            "edge_height": edge_height,
            "edge_angle": edge_angle,
            "length_offset": length_offset,
        }

    # ------------------------------------------------------------------
    # SurfacePattern strategy
    # ------------------------------------------------------------------

    def _buildBoundary(self, obj, job, tool_radius, cutting_faces, avoid_faces):
        """Construct the 2D boundary face used to clip the scan pattern.

        Falls back to the Job's whole-model bounding-box face if the user
        has not selected explicit Base faces.
        """
        boundary_face = None
        bbox = None

        if cutting_faces:
            boundary_face = surface_common.generate_pattern_mask(
                False,
                None,
                cutting_faces,
                avoid_faces,
                tool_radius,
                float(obj.BoundaryAdjustment.Value),
                float(obj.LinearDeflection.Value),
            )
            if boundary_face is not None:
                bbox = boundary_face.BoundBox
        else:
            # Use stock projection — flatten the stock to the XY plane.
            stock_shape = job.Stock.Shape
            bb = stock_shape.BoundBox
            import Part

            poly = Part.makePolygon(
                [
                    FreeCAD.Vector(bb.XMin, bb.YMin, 0.0),
                    FreeCAD.Vector(bb.XMax, bb.YMin, 0.0),
                    FreeCAD.Vector(bb.XMax, bb.YMax, 0.0),
                    FreeCAD.Vector(bb.XMin, bb.YMax, 0.0),
                    FreeCAD.Vector(bb.XMin, bb.YMin, 0.0),
                ]
            )
            boundary_face = Part.Face(Part.Wire([poly]))
            bbox = boundary_face.BoundBox

        return boundary_face, bbox

    def _executeSurfacePattern(self, obj, job, stl, cutter, tool_radius):
        """Run the SurfacePattern pipeline: pattern → drop-cutter → G-code."""
        cutting_faces, avoid_faces = surface_pattern.split_selected_features(
            getattr(obj, "Base", None), 0
        )

        boundary_face, bbox = self._buildBoundary(obj, job, tool_radius, cutting_faces, avoid_faces)
        if boundary_face is None or bbox is None:
            Path.Log.error("Surface3D: failed to build boundary mask.")
            return []

        scan_bb = surface_pattern.BBox.from_bbox(bbox)
        center = scan_bb.center
        cut_climb = obj.CutMode == "Climb"
        if obj.CutPatternReversed:
            cut_climb = not cut_climb

        is_zigzag = obj.CutPattern == "ZigZag"

        raw_scan_lines = surface_pattern.fast_generate_pattern(
            obj.CutPattern,
            scan_bb,
            center,
            float(obj.StepOver.Value),
            float(obj.SampleInterval.Value),
            float(obj.CutPatternAngle),
            is_zigzag,
            bool(obj.CutPatternReversed),
            boundary_face,
        )
        if not raw_scan_lines:
            Path.Log.warning("Surface3D: pattern generator produced no scan lines.")
            return []
        _yield_to_gui()

        # Project: BatchDropCutter for grid-style patterns, PathDropCutter for
        # Line/ZigZag where ordering matters.
        if obj.CutPattern in ("Line", "ZigZag"):
            results_flat = surface_dropcutter.path_dropcutter(
                stl,
                cutter,
                raw_scan_lines,
                float(obj.FinalDepth.Value),
                float(obj.SampleInterval.Value),
            )
        else:
            results_flat = surface_dropcutter.batch_dropcutter(
                stl,
                cutter,
                raw_scan_lines,
                float(obj.FinalDepth.Value),
            )

        scan_lines = surface_pattern.reconstruct_scan_lines(
            results_flat, float(obj.SampleInterval.Value) * 2.5
        )
        if not scan_lines:
            Path.Log.warning("Surface3D: drop-cutter returned no projected points.")
            return []

        if obj.OptimizeLinearPaths:
            scan_lines = [surface_postprocess.filter_cl_points(line, 0.005) for line in scan_lines]

        # Multi-pass: replay the full-depth drop-cutter scan at each
        # StepDown level, with air-cut elimination above the previous
        # layer.  When StepDown >= (StartDepth - FinalDepth) this
        # collapses to a single layer.
        scan_lines = surface_postprocess.apply_multipass(
            scan_lines,
            float(obj.StartDepth.Value),
            float(obj.FinalDepth.Value),
            float(obj.StepDown.Value),
        )

        return surface_postprocess.scan_lines_to_gcode(
            scan_lines,
            horiz_feed=self.horizFeed,
            vert_rapid=self.vertRapid,
            horiz_rapid=self.horizRapid,
            safe_z=float(obj.SafeHeight.Value),
            step_down=float(obj.StepDown.Value),
            sample_interval=float(obj.SampleInterval.Value),
            clearance_z=float(obj.ClearanceHeight.Value),
            start_z=float(obj.StartDepth.Value),
            final_z=float(obj.FinalDepth.Value),
            depth_offset=float(obj.DepthOffset.Value),
        )

    # ------------------------------------------------------------------
    # Waterline strategy (W1.1 redirect — always uses AdaptiveWaterline)
    # ------------------------------------------------------------------

    def _executeWaterline(self, obj, job, stl, cutter):
        """Run the Waterline strategy via AdaptiveWaterline.

        Per the v1 W1.1 redirect, this always goes through OCL's
        ``AdaptiveWaterline`` (SmartWeave) — never ``ocl.Waterline``
        (SimpleWeave) which has a hard-assert segfault on certain
        geometry.  Setting ``min_sampling = sampling`` disables adaptive
        refinement for the equivalent of fixed sampling.
        """
        sampling = float(obj.SampleInterval.Value)
        min_sampling = float(obj.MinSampleInterval.Value) or sampling
        cut_climb = obj.CutMode == "Climb"
        if obj.CutPatternReversed:
            cut_climb = not cut_climb

        wl_data = surface_waterline.waterline_stack(
            stl,
            cutter,
            sampling=sampling,
            min_sampling=min_sampling,
            min_z=float(obj.FinalDepth.Value),
            max_z=float(obj.StartDepth.Value),
            step_down=float(obj.StepDown.Value),
            adaptive=True,
            depth_offset=float(obj.DepthOffset.Value),
        )

        return surface_waterline.waterline_to_gcode(
            wl_data,
            horiz_feed=self.horizFeed,
            vert_rapid=self.vertRapid,
            horiz_rapid=self.horizRapid,
            safe_z=float(obj.SafeHeight.Value),
            clearance_z=float(obj.ClearanceHeight.Value),
            cut_climb=cut_climb,
        )

    # ------------------------------------------------------------------
    # ZLevelHybrid strategy (no-OCL via Path.Area Clipper)
    # ------------------------------------------------------------------

    def _executeZLevelHybrid(self, obj, job, model_shape, tool_params):
        """Run the ZLevelHybrid pipeline (no OCL required).

        Geometric constant-Z contouring via FreeCAD's slicer + Path.Area
        Clipper.  Robust on geometry that triggers OCL waterline edge
        cases.
        """
        import Part

        diameter = tool_params["diameter"]
        radius = diameter / 2.0
        shape_type = (tool_params["tool_type"] or "").lower()
        corner_radius = tool_params["corner_radius"]
        is_3d = shape_type in ("ballend", "bullnose")
        if diameter <= 0 or (not is_3d and "endmill" not in shape_type):
            Path.Log.error(
                "Surface3D ZLevelHybrid: tool shape '{}' is not supported "
                "(use endmill, ball-end, or bull-nose).".format(shape_type)
            )
            return []

        # Workplane circle used by surface_zlevel as the 2D calculation plane.
        wpc = Part.makeCircle(2.0, FreeCAD.Vector(0, 0, 0), FreeCAD.Vector(0, 0, 1))

        zlevel_tool_params = {
            "radius": radius,
            "c_rad": corner_radius,
            "profile": shape_type,
            "is_threeD": is_3d,
        }

        # Boundary preparation — extend the stock bbox outward, then trim
        # using the user's Base selection (or the stock face if no Base).
        buffer = radius * 10.0
        border_poly = surface_zlevel.extendedBoundBox(job.Stock.Shape.BoundBox, buffer, 0.0)
        border_face = Part.makeFace(border_poly)

        # Build the boundary-mask face (Base selection or stock projection).
        cutting_faces, avoid_faces = surface_pattern.split_selected_features(
            getattr(obj, "Base", None), 0
        )
        bb_face, _bbox = self._buildBoundary(obj, job, radius, cutting_faces, avoid_faces)
        if bb_face is None:
            bb_face = border_face

        trim_face = surface_zlevel.getTrimFace(border_face, bb_face, wpc)

        cat_steps = surface_zlevel.categorize_floor_steps(
            model_shape,
            float(obj.StartDepth.Value),
            float(obj.FinalDepth.Value),
            float(obj.StepDown.Value),
            bool(obj.ClearPlanarOnly),
        )

        accuracy_val = getattr(obj, "SamplingAccuracy", "4")

        wl_data = surface_zlevel.zlevel_hybrid_stack(
            model_shape,
            cat_steps,
            border_face,
            trim_face,
            zlevel_tool_params,
            float(obj.StockToLeave.Value),
            accuracy_val,
            float(obj.DepthOffset.Value),
            wpc,
            start_z=float(obj.StartDepth.Value),
        )

        pattern_options = {
            "cut_climb": obj.CutMode == "Climb",
            "cut_pattern": getattr(obj, "CutPatternZLevel", "None"),
            "pattern_angle": float(obj.CutPatternAngle),
            "reverse_pattern": bool(obj.CutPatternReversed),
        }
        height_params = {
            "safe_hght": float(obj.SafeHeight.Value),
            "clearance_hght": float(obj.ClearanceHeight.Value),
        }
        feed_params = {
            "horizFeed": self.horizFeed,
            "vertFeed": self.vertFeed,
            "horizRapid": self.horizRapid,
            "vertRapid": self.vertRapid,
        }
        step_over = float(obj.StepOver.Value)

        return surface_zlevel.zlevel_hybrid_to_gcode(
            wl_data,
            feed_params,
            height_params,
            pattern_options,
            bool(obj.IgnoreOuter),
            bool(obj.ClearPlanarOnly),
            step_over,
            radius,
        )

    # ------------------------------------------------------------------
    # Execute
    # ------------------------------------------------------------------

    def opExecute(self, obj):
        Path.Log.track()
        job = PathUtils.findParentJob(obj)
        if job is None:
            Path.Log.error("Surface3D: no parent Job.")
            return

        tool_params = self._extractToolParams(obj)
        diameter = tool_params["diameter"]
        if diameter <= 0:
            Path.Log.error("Surface3D: tool diameter must be > 0.")
            return
        tool_radius = diameter / 2.0

        # Build the model shape (fuse multiple models if the Job has them).
        models = job.Model.Group if hasattr(job.Model, "Group") else []
        if not models:
            Path.Log.error("Surface3D: Job has no model objects.")
            return

        valid_shapes = [m.Shape for m in models if m.Shape and not m.Shape.isNull()]
        if not valid_shapes:
            Path.Log.error("Surface3D: Job models have no valid shapes.")
            return

        if len(valid_shapes) > 1:
            try:
                model_shape = valid_shapes[0].fuse(valid_shapes[1:])
                if hasattr(model_shape, "removeSplitter"):
                    model_shape = model_shape.removeSplitter()
            except Exception as e:
                Path.Log.error("Surface3D: failed to fuse model shapes: {}".format(e))
                return
        else:
            model_shape = valid_shapes[0]

        strategy = obj.Strategy

        # ZLevelHybrid is no-OCL — skip cutter/STL setup.
        if strategy == "ZLevelHybrid":
            cmds = self._executeZLevelHybrid(obj, job, model_shape, tool_params)
            if cmds:
                self.commandlist.extend(cmds)
            return

        # SurfacePattern and Waterline both need OCL + STL + cutter.
        if ocl is None:
            Path.Log.error(
                "Surface3D '{}' strategy requires OpenCamLib (OCL); not "
                "available.  Use ZLevelHybrid for a no-OCL alternative.".format(strategy)
            )
            return

        cutter = surface_common.make_ocl_cutter(
            tool_params["tool_type"],
            diameter,
            corner_radius=tool_params["corner_radius"],
            flat_radius=tool_params["flat_radius"],
            edge_height=tool_params["edge_height"],
            edge_angle=tool_params["edge_angle"],
            length_offset=tool_params["length_offset"],
        )
        if cutter is None:
            Path.Log.error("Surface3D: could not build OCL cutter for tool.")
            return

        # Sanity-check depths before tessellation.  generate_stl pre-clips
        # the model to FinalDepth and a non-empty result requires
        # FinalDepth strictly below the model's top.
        model_zmax = model_shape.BoundBox.ZMax
        model_zmin = model_shape.BoundBox.ZMin
        final_depth = float(obj.FinalDepth.Value)
        if final_depth >= model_zmax - 1e-6:
            Path.Log.error(
                "Surface3D: FinalDepth ({:.3f}) is at or above the top of the "
                "model ({:.3f}). Lower FinalDepth to the floor of the region "
                "you want to finish (the model bottom is {:.3f}).".format(
                    final_depth, model_zmax, model_zmin
                )
            )
            return

        _yield_to_gui()
        stl, _safe_stl = surface_mesh.generate_stl(
            model_shape,
            models,
            avoid_faces=[],
            tool_radius=tool_radius,
            needs_safe_stl=False,
            start_depth=float(obj.StartDepth.Value),
            final_depth=final_depth,
            linear_deflection=float(obj.LinearDeflection.Value),
            angular_deflection=float(obj.AngularDeflection.Value),
            mesh_simplification=int(obj.MeshSimplification),
            use_cpp=True,
        )
        if stl is None:
            Path.Log.error("Surface3D: STL tessellation failed.")
            return
        _yield_to_gui()

        if strategy == "SurfacePattern":
            cmds = self._executeSurfacePattern(obj, job, stl, cutter, tool_radius)
        elif strategy == "Waterline":
            cmds = self._executeWaterline(obj, job, stl, cutter)
        else:
            Path.Log.error("Surface3D: strategy '{}' not implemented.".format(strategy))
            return
        _yield_to_gui()

        if cmds:
            self.commandlist.extend(cmds)


def SetupProperties():
    proxy = ObjectSurface3D.__new__(ObjectSurface3D)
    return [name for _, name, _, _ in proxy.opPropertyDefinitions()]


def Create(name, obj=None, parentJob=None):
    """Create(name, obj=None, parentJob=None) ... create a Surface3D operation."""
    if obj is None:
        obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", name)
    obj.Proxy = ObjectSurface3D(obj, name, parentJob)
    return obj
