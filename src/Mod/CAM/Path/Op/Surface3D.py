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
import Path.Op.Base as PathOp
import Path.Base.Generator.surface_common as surface_common
import Path.Base.Generator.surface_dropcutter as surface_dropcutter
import Path.Base.Generator.surface_mesh as surface_mesh
import Path.Base.Generator.surface_pattern as surface_pattern
import Path.Base.Generator.surface_postprocess as surface_postprocess
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
                msg += translate("CAM_Surface3D", "Check default value(s).")
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
                    "Top-level finishing strategy. SurfacePattern projects a 2D pattern "
                    "onto the surface via drop-cutter; Waterline and ZLevelHybrid arrive "
                    "in later slices.",
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
                    "Distance between sampled points along a scan line (mm).",
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
        # Only SurfacePattern is wired in this slice; later slices add the
        # Waterline and ZLevelHybrid values.
        enums = {
            "Strategy": [
                (translate("CAM_Surface3D", "Surface Pattern"), "SurfacePattern"),
            ],
            "CutPattern": [
                (translate("CAM_Surface3D", "Line"), "Line"),
                (translate("CAM_Surface3D", "ZigZag"), "ZigZag"),
                (translate("CAM_Surface3D", "Circular"), "Circular"),
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
            "CutPatternAngle": 0.0,
            "CutPatternReversed": False,
            "CutMode": "Climb",
            "StepOver": 1.0,
            "SampleInterval": 0.4,
            "DepthOffset": 0.0,
            "BoundaryAdjustment": 0.0,
            "HandleMultipleFeatures": "Collectively",
            "OptimizeLinearPaths": True,
            "LinearDeflection": 0.1,
            "AngularDeflection": 0.524,
        }

    def opSetDefaultValues(self, obj, job):
        Path.Log.track()
        defaults = self.opPropertyDefaults(obj, job)
        for name, value in defaults.items():
            if hasattr(obj, name):
                setattr(obj, name, value)

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
            cut_climb,
            boundary_face,
        )
        if not raw_scan_lines:
            Path.Log.warning("Surface3D: pattern generator produced no scan lines.")
            return []

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
    # Execute
    # ------------------------------------------------------------------

    def opExecute(self, obj):
        Path.Log.track()
        if ocl is None:
            Path.Log.error("Surface3D requires OpenCamLib (OCL); not available.")
            return

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

        stl, _safe_stl = surface_mesh.generate_stl(
            model_shape,
            models,
            avoid_faces=[],
            tool_radius=tool_radius,
            needs_safe_stl=False,
            start_depth=float(obj.StartDepth.Value),
            final_depth=float(obj.FinalDepth.Value),
            linear_deflection=float(obj.LinearDeflection.Value),
            angular_deflection=float(obj.AngularDeflection.Value),
            mesh_simplification=1,
            use_cpp=True,
        )
        if stl is None:
            Path.Log.error("Surface3D: STL tessellation failed.")
            return

        strategy = obj.Strategy
        if strategy == "SurfacePattern":
            cmds = self._executeSurfacePattern(obj, job, stl, cutter, tool_radius)
        else:
            Path.Log.error("Surface3D: strategy '{}' not implemented yet.".format(strategy))
            return

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
