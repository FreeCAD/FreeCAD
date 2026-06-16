# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 <shopinthewoods@gmail.com>
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


__title__ = "CAM Rotary Surface Operation"
__author__ = "sliptonic (Brad Collette)"
__url__ = "https://www.freecad.org"
__doc__ = "Class and implementation of the Rotary Surface operation."

import math

import FreeCAD
from PySide import QtCore

import Path
import Path.Op.Base as PathOp
import Path.Op.SurfaceSupport as SurfaceSupport
import Path.Base.Generator.rotary_dropcutter as rotary_dropcutter
import Path.Base.Generator.rotary_parallel as rotary_parallel
import Path.Base.Generator.rotary_rings as rotary_rings
import Path.Base.Generator.rotary_spiral as rotary_spiral
import Path.Base.Generator.rotary_wrap as rotary_wrap
import Path.Main.Stock as PathStock
import PathScripts.PathUtils as PathUtils

# OCL is loaded lazily in opExecute so the module can be imported in
# environments without OCL (for non-execute access like loading saved
# documents).
try:
    import ocl  # noqa: F401  (preferred name on some packagings)
except ImportError:  # pragma: no cover
    try:
        import opencamlib as ocl  # noqa: F401
    except ImportError:  # pragma: no cover
        ocl = None


translate = FreeCAD.Qt.translate


def _wrap_strategy_value(axis_obj):
    """Extract the rotary axis's wrap strategy as a plain string.

    Tolerates older RotaryAxis instances that predate the field
    (returns "unwound") and accepts either a WrapStrategy enum or its
    string form.
    """
    val = getattr(axis_obj, "wrap_strategy", "unwound")
    return getattr(val, "value", val)


if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


class ObjectRotarySurface(PathOp.ObjectOp):
    """Continuous 4-axis rotary surfacing operation."""

    def opFeatures(self, obj):
        return (
            PathOp.FeatureTool
            | PathOp.FeatureDepths
            | PathOp.FeatureHeights
            | PathOp.FeatureStepDown
            | PathOp.FeatureBaseFaces
            | PathOp.FeatureCoolant
        )

    def initOperation(self, obj):
        self.propertiesReady = False
        self.initOpProperties(obj)

    def opOnDocumentRestored(self, obj):
        pass

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
                msg = translate("CAM_RotarySurface", "New property added to")
                msg += ' "{}": {}'.format(obj.Label, self.addNewProps) + ". "
                msg += translate("CAM_RotarySurface", "Check default value(s).")
                FreeCAD.Console.PrintWarning(msg + "\n")

        self.propertiesReady = True

    def opPropertyDefinitions(self):
        return [
            (
                "App::PropertyDistance",
                "StartX",
                "Rotary",
                QtCore.QT_TRANSLATE_NOOP(
                    "App::Property", "Axial start position along the rotary axis."
                ),
            ),
            (
                "App::PropertyDistance",
                "StopX",
                "Rotary",
                QtCore.QT_TRANSLATE_NOOP(
                    "App::Property", "Axial stop position along the rotary axis."
                ),
            ),
            (
                "App::PropertyAngle",
                "StartAngle",
                "Rotary",
                QtCore.QT_TRANSLATE_NOOP("App::Property", "Angular start position (degrees)."),
            ),
            (
                "App::PropertyAngle",
                "StopAngle",
                "Rotary",
                QtCore.QT_TRANSLATE_NOOP("App::Property", "Angular stop position (degrees)."),
            ),
            (
                "App::PropertyDistance",
                "StepOver",
                "Rotary",
                QtCore.QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Axial advance per full revolution of the rotary axis " "(spiral pitch).",
                ),
            ),
            (
                "App::PropertyAngle",
                "AngularResolution",
                "Rotary",
                QtCore.QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Angular spacing between sampled toolpath points (degrees).",
                ),
            ),
            (
                "App::PropertyDistance",
                "RadialStockToLeave",
                "Rotary",
                QtCore.QT_TRANSLATE_NOOP(
                    "App::Property", "Radial finish allowance left on the surface."
                ),
            ),
            (
                "App::PropertyEnumeration",
                "CutMode",
                "Rotary",
                QtCore.QT_TRANSLATE_NOOP(
                    "App::Property", "Climb or Conventional cutting direction."
                ),
            ),
            (
                "App::PropertyEnumeration",
                "CutPattern",
                "Rotary",
                QtCore.QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Toolpath pattern. Supports Spiral, Parallel, Rings.",
                ),
            ),
            (
                "App::PropertyEnumeration",
                "FeedMode",
                "Rotary",
                QtCore.QT_TRANSLATE_NOOP("App::Property", "Feed-rate strategy."),
            ),
            (
                "App::PropertyFloat",
                "MaxFeed",
                "Rotary",
                QtCore.QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Effective feed clamp (mm/min); used to handle the "
                    "centerline singularity as r approaches zero. 0 means "
                    "fall back to the tool controller rapid rate.",
                ),
            ),
            (
                "App::PropertyBool",
                "BoundaryFromFaces",
                "Rotary",
                QtCore.QT_TRANSLATE_NOOP(
                    "App::Property",
                    "If true and Base is populated, restrict the toolpath "
                    "to the projected (axial, angular) extents of selected faces.",
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
                "App::PropertyDistance",
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
            "CutMode": [
                (translate("CAM_RotarySurface", "Climb"), "Climb"),
                (translate("CAM_RotarySurface", "Conventional"), "Conventional"),
            ],
            "CutPattern": [
                (translate("CAM_RotarySurface", "Spiral"), "Spiral"),
                (translate("CAM_RotarySurface", "Parallel"), "Parallel"),
                (translate("CAM_RotarySurface", "Rings"), "Rings"),
            ],
            "FeedMode": [
                (translate("CAM_RotarySurface", "Surface Speed"), "SurfaceSpeed"),
                (translate("CAM_RotarySurface", "Axial Only"), "AxialOnly"),
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
            "StepOver": 1.0,
            "AngularResolution": 5.0,
            "RadialStockToLeave": 0.0,
            "CutMode": "Climb",
            "CutPattern": "Spiral",
            "FeedMode": "AxialOnly",
            "BoundaryFromFaces": False,
            "LinearDeflection": 0.1,
            "AngularDeflection": 0.524,
        }

    def opSetDefaultValues(self, obj, job):
        Path.Log.track()
        defaults = self.opPropertyDefaults(obj, job)
        for name, value in defaults.items():
            try:
                setattr(obj, name, value)
            except Exception as e:
                Path.Log.warning("RotarySurface: failed to set default for {}: {}".format(name, e))

        # Initialize axial extents from stock if cylindrical.
        stock = job.Stock if job else None
        x_min, x_max = self._stock_axial_extents(stock)
        if x_min is not None and x_max is not None:
            obj.StartX = x_min
            obj.StopX = x_max

        obj.StartAngle = 0.0
        obj.StopAngle = 360.0
        obj.MaxFeed = 0.0  # 0 == unset; will fall back to tool-controller rapid

    @staticmethod
    def _stock_axial_extents(stock):
        """Best-effort axial bounds from a cylindrical stock object."""
        if stock is None:
            return None, None
        if not hasattr(stock, "Shape") or stock.Shape.isNull():
            return None, None
        bb = stock.Shape.BoundBox
        # We don't know the rotary axis here — use whichever world span is
        # largest (stock cylinders are typically aligned with their
        # longest dimension).
        spans = [
            (bb.XMax - bb.XMin, "X", bb.XMin, bb.XMax),
            (bb.YMax - bb.YMin, "Y", bb.YMin, bb.YMax),
            (bb.ZMax - bb.ZMin, "Z", bb.ZMin, bb.ZMax),
        ]
        spans.sort(reverse=True)
        return spans[0][2], spans[0][3]

    # ------------------------------------------------------------------
    # Machine + stock validation
    # ------------------------------------------------------------------

    def _resolve_rotary(self, obj, job):
        """Read the rotary axis from the Job's Machine.

        Returns (axis_label, rotary_letter, rotation_vector_world,
                 axis_min_deg, axis_max_deg, wrap_strategy).

        Raises ValueError if the Job has no Machine, or the Machine
        declares no rotary axes.
        """
        machine = None
        if job is not None and hasattr(job, "Proxy") and hasattr(job.Proxy, "getMachine"):
            try:
                machine = job.Proxy.getMachine()
            except Exception as e:  # pragma: no cover - defensive
                Path.Log.debug("getMachine failed: {}".format(e))
                machine = None

        if machine is None:
            raise ValueError(
                "no machine configured on the Job. Configure a machine "
                "with at least one rotary axis."
            )
        if not getattr(machine, "rotary_axes", None):
            raise ValueError("the configured machine declares no rotary axes.")

        # Take the first rotary axis the machine declares.
        axis_obj = next(iter(machine.rotary_axes.values()))
        rv = axis_obj.rotation_vector
        rot_vec = FreeCAD.Vector(rv.x, rv.y, rv.z)
        rotary_letter = axis_obj.name
        amin = float(getattr(axis_obj, "min_limit", -360.0))
        amax = float(getattr(axis_obj, "max_limit", 360.0))
        wrap_strategy = _wrap_strategy_value(axis_obj)
        axis_label = self._axis_label_from_vec(rot_vec)

        return axis_label, rotary_letter, rot_vec, amin, amax, wrap_strategy

    @staticmethod
    def _axis_label_from_vec(v):
        """Classify a rotation vector as 'X' or 'Y'; raise for others."""
        ax = abs(v.x)
        ay = abs(v.y)
        az = abs(v.z)
        if ax > 0.99 and ay < 0.05 and az < 0.05:
            return "X"
        if ay > 0.99 and ax < 0.05 and az < 0.05:
            return "Y"
        if az > 0.99 and ax < 0.05 and ay < 0.05:
            raise ValueError("Rotary axis along world Z is not supported in v1.")
        raise ValueError(
            "Rotary axis must be aligned with world X or Y in v1; got {}".format((v.x, v.y, v.z))
        )

    def _face_bounds(self, base_list, axis_label, linear_deflection):
        """Project selected faces onto (axial, theta_grid) and return bbox.

        Returns (axial_min, axial_max, theta_min_deg, theta_max_deg) or None
        if no faces could be sampled. theta_grid matches the convention of
        `rotary_dropcutter.sample`: theta=0 means the cutter at +Z is
        sampling the part feature originally at world +Z.
        """
        if not base_list:
            return None
        ax_min = float("inf")
        ax_max = float("-inf")
        th_min = float("inf")
        th_max = float("-inf")
        found = False
        for base, subs in base_list:
            if not hasattr(base, "Shape"):
                continue
            for sub in subs:
                try:
                    shape = base.Shape.getElement(sub)
                except Exception:
                    continue
                if not hasattr(shape, "tessellate"):
                    continue
                try:
                    verts, _ = shape.tessellate(float(linear_deflection))
                except Exception:
                    continue
                for v in verts:
                    found = True
                    if axis_label == "X":
                        ax = v.x
                        th = math.degrees(math.atan2(-v.y, v.z))
                    else:
                        ax = v.y
                        th = math.degrees(math.atan2(v.x, v.z))
                    if th < 0:
                        th += 360.0
                    if ax < ax_min:
                        ax_min = ax
                    if ax > ax_max:
                        ax_max = ax
                    if th < th_min:
                        th_min = th
                    if th > th_max:
                        th_max = th
        if not found:
            return None
        return ax_min, ax_max, th_min, th_max

    def _validate_stock(self, stock, axis_label):
        """Validate stock is a cylinder with axis along axis_label."""
        if stock is None:
            raise ValueError("Job has no Stock.")
        stock_type = PathStock.StockType.FromStock(stock)
        if stock_type != PathStock.StockType.CreateCylinder:
            raise ValueError(
                "Rotary Surface requires a Cylinder stock (got {}).".format(stock_type)
            )
        # Cylinder primitive built around +Z by default; rotated by Placement.
        rot = stock.Placement.Rotation
        cyl_axis = rot.multVec(FreeCAD.Vector(0, 0, 1))
        cyl_axis.normalize()

        target = FreeCAD.Vector(1, 0, 0) if axis_label == "X" else FreeCAD.Vector(0, 1, 0)
        # Accept both polarities (cylinder axis sign convention).
        dot = abs(cyl_axis.dot(target))
        # ~0.1 deg tolerance: cos(0.1deg) ~= 0.9999985
        if dot < 0.9999985:
            raise ValueError(
                "Stock cylinder axis is not aligned with world {} (dot={:.6f}).".format(
                    axis_label, dot
                )
            )

    # ------------------------------------------------------------------
    # Execution
    # ------------------------------------------------------------------

    def opExecute(self, obj):
        Path.Log.track()
        if ocl is None:
            Path.Log.error("Rotary Surface requires opencamlib (OCL); not available.")
            return

        job = PathUtils.findParentJob(obj)
        if job is None:
            Path.Log.error("Rotary Surface: no parent Job.")
            return

        # Resolve rotary axis + letter from the Job's Machine.
        try:
            axis_label, rotary_letter, rot_vec, amin, amax, wrap_strategy = self._resolve_rotary(
                obj, job
            )
        except ValueError as e:
            Path.Log.error("Rotary Surface: {}".format(e))
            return

        # Validate stock.
        try:
            self._validate_stock(job.Stock, axis_label)
        except ValueError as e:
            Path.Log.error("Rotary Surface: {}".format(e))
            return

        # Validate angular extents against machine limits (warn only).
        start_deg = float(obj.StartAngle.Value)
        stop_deg = float(obj.StopAngle.Value)
        if not (amin <= start_deg <= amax) or not (amin <= stop_deg <= amax):
            Path.Log.warning(
                "Rotary Surface: angular extents [{}..{}] exceed "
                "machine rotary limits [{}..{}].".format(start_deg, stop_deg, amin, amax)
            )

        # Build STL from job.Model (after sanity-check we have at least one).
        models = job.Model.Group if hasattr(job.Model, "Group") else []
        if not models:
            Path.Log.error("Rotary Surface: Job has no model objects.")
            return
        model = models[0]

        # Warn when the part centroid sits noticeably off the rotary
        # axis: rotary surfacing assumes the cutter follows a part
        # whose center of mass is on (or very near) the spin axis.
        # BoundBox.Center is a cheap proxy for centroid that catches
        # the gross mis-centering case without needing CoM computation.
        shp = getattr(model, "Shape", None)
        if shp is not None and hasattr(shp, "BoundBox"):
            # Apply the model's Placement to the shape's local bbox
            # center so the offset is measured in world coordinates,
            # not shape-local coordinates.
            local_c = shp.BoundBox.Center
            placement = getattr(model, "Placement", None)
            if placement is not None and hasattr(placement, "multVec"):
                c = placement.multVec(local_c)
            else:
                c = local_c
            if axis_label == "X":
                offset = math.hypot(c.y, c.z)
            else:
                # axis_label == "Y"
                offset = math.hypot(c.x, c.z)
            tol = float(obj.LinearDeflection.Value)
            if offset > tol:
                Path.Log.warning(
                    "Rotary Surface: part centroid is {:.3f}mm off the rotary "
                    "axis (> LinearDeflection {:.3f}mm). Re-center the part "
                    "on the spin axis or expect uneven cuts.".format(offset, tol)
                )
        try:
            stl = SurfaceSupport._makeSTL(model, obj, ocl)
        except Exception as e:
            Path.Log.error("Rotary Surface: failed to tessellate model: {}".format(e))
            return

        # Tool-shape advisory: flat cutters give poor results on curved
        # surfaces. Match Surface.py's behavior with a warning.
        tool_type = None
        if hasattr(self.tool, "ShapeType"):
            tool_type = str(self.tool.ShapeType).lower()
        elif hasattr(self.tool, "ShapeName"):
            tool_type = str(self.tool.ShapeName).lower()
        if tool_type and tool_type not in (
            "ballend",
            "bullnose",
            "taperedballnose",
        ):
            Path.Log.warning(
                "Rotary Surface: tool shape '{}' is not ideal for "
                "rotary surfacing; consider a ball-end or bull-nose tool.".format(tool_type)
            )

        # Build OCL cutter from the tool controller.
        oclt = SurfaceSupport.OCL_Tool(ocl, obj, safe=False)
        oclt.setFromTool(self.tool) if hasattr(oclt, "setFromTool") else None
        cutter = oclt.getOclTool() if hasattr(oclt, "getOclTool") else None
        if not cutter:
            # Fallback to a small flat cutter sized to the tool diameter.
            cutter = ocl.CylCutter(max(self.radius * 2.0, 0.5), 50.0)

        # Build sampling grid. Density along x at AxialStepover/4; thetas at
        # AngularResolution.
        x_min = float(obj.StartX.Value)
        x_max = float(obj.StopX.Value)
        if x_max <= x_min:
            Path.Log.error("Rotary Surface: StopX must exceed StartX.")
            return

        ang_res_rad = math.radians(float(obj.AngularResolution.Value))
        if ang_res_rad <= 0:
            Path.Log.error("Rotary Surface: AngularResolution must be positive.")
            return

        step_over = float(obj.StepOver.Value)
        if step_over <= 0:
            Path.Log.error("Rotary Surface: Step Over must be positive.")
            return

        step_down = float(obj.StepDown.Value) if hasattr(obj, "StepDown") else 0.0
        stock_to_leave = float(obj.RadialStockToLeave.Value)

        # Optional restriction to user-selected faces. Tightens axial and
        # angular extents to the (axial, theta_grid) projection of Base.
        if getattr(obj, "BoundaryFromFaces", False) and getattr(obj, "Base", None):
            bounds = self._face_bounds(obj.Base, axis_label, float(obj.LinearDeflection.Value))
            if bounds is None:
                Path.Log.warning(
                    "Rotary Surface: BoundaryFromFaces is on but no faces "
                    "could be sampled from Base; ignoring."
                )
            else:
                ax_lo, ax_hi, th_lo, th_hi = bounds
                x_min = max(x_min, ax_lo)
                x_max = min(x_max, ax_hi)
                # Skip angular tightening when the face spans (effectively)
                # a full revolution, or when it likely wraps the 0/360
                # boundary (v1 limitation: wrap-aware bounds are future work).
                if (th_hi - th_lo) < 350.0:
                    start_deg = max(start_deg, th_lo)
                    stop_deg = min(stop_deg, th_hi)
                if x_max <= x_min:
                    Path.Log.error(
                        "Rotary Surface: BoundaryFromFaces produced an empty "
                        "axial range; widen StartX/StopX or pick different faces."
                    )
                    return
                if stop_deg <= start_deg:
                    Path.Log.error(
                        "Rotary Surface: BoundaryFromFaces produced an empty "
                        "angular range; widen StartAngle/StopAngle or pick "
                        "different faces."
                    )
                    return
                Path.Log.info(
                    "Rotary Surface: restricted to selected faces — axial "
                    "[{:.2f}..{:.2f}], angular [{:.2f}..{:.2f}].".format(
                        x_min, x_max, start_deg, stop_deg
                    )
                )

        # x grid: dense enough to interpolate well across the spiral.
        x_step = min(step_over * 0.25, (x_max - x_min) / 8.0)
        x_step = max(x_step, 1e-3)
        n_x = max(2, int(math.ceil((x_max - x_min) / x_step)) + 1)
        xs = [x_min + (x_max - x_min) * i / (n_x - 1) for i in range(n_x)]

        # theta grid: full revolution at AngularResolution.
        n_t = max(8, int(math.ceil(2.0 * math.pi / ang_res_rad)) + 1)
        thetas = [2.0 * math.pi * j / (n_t - 1) for j in range(n_t)]

        axis_vec = (rot_vec.x, rot_vec.y, rot_vec.z)
        try:
            radii = rotary_dropcutter.sample(stl, axis_vec, xs, thetas, cutter)
        except Exception as e:
            Path.Log.error("Rotary Surface: dropcutter sampling failed: {}".format(e))
            return

        # Compute MaxFeed default if user left it at 0.
        max_feed = float(obj.MaxFeed)
        if max_feed <= 0:
            max_feed = max(self.horizRapid, self.vertRapid, 1000.0)

        # Determine number of radial passes from stock outer radius.
        # Each pass targets a specific radial *floor* (cutter Z clamp) so the
        # outermost pass cuts just inside the stock surface rather than at
        # surface_r + offset (which would lift the cutter into open air when
        # the part has tall ridges).
        stock_radius = (
            float(getattr(job.Stock, "Radius", 0).Value) if hasattr(job.Stock, "Radius") else 0.0
        )
        valid_radii = [
            r
            for row in (radii.tolist() if hasattr(radii, "tolist") else radii)
            for r in row
            if not (r is None or (r != r))  # NaN check
        ]
        min_surface_r = min(valid_radii) if valid_radii else 0.0
        gap = max(0.0, stock_radius - (min_surface_r + stock_to_leave))
        if step_down > 0.0 and gap > 0.0:
            n_layers = int(math.ceil(gap / step_down))
        else:
            n_layers = 1

        for layer_index in range(n_layers):
            # Layer 0 is outermost (closest to stock OD). Final layer's floor
            # is at or below min_surface_r + stock_to_leave, so the surface
            # follow naturally takes over for the finish pass.
            if step_down > 0.0 and n_layers > 1:
                cutter_z_floor = stock_radius - (layer_index + 1) * step_down
            else:
                cutter_z_floor = None

            pattern = str(obj.CutPattern)
            generator_kwargs = dict(
                radii=radii,
                xs=xs,
                thetas=thetas,
                rotary_letter=rotary_letter,
                rotary_axis=axis_label,
                x_min=x_min,
                x_max=x_max,
                theta_start=math.radians(start_deg),
                theta_end=math.radians(stop_deg),
                axial_stepover=step_over,
                angular_resolution=ang_res_rad,
                radial_stock_to_leave=stock_to_leave,
                cut_mode=str(obj.CutMode),
                safe_height=float(obj.SafeHeight.Value),
                clearance_height=float(obj.ClearanceHeight.Value),
                horiz_feed=self.horizFeed,
                vert_feed=self.vertFeed,
                horiz_rapid=self.horizRapid,
                vert_rapid=self.vertRapid,
                max_feed=max_feed,
                cutter_z_floor=cutter_z_floor,
                feed_mode=str(obj.FeedMode),
            )
            if pattern == "Rings":
                raw_commands = rotary_rings.generate(**generator_kwargs)
            elif pattern == "Parallel":
                raw_commands = rotary_parallel.generate(**generator_kwargs)
            else:
                raw_commands = rotary_spiral.generate(**generator_kwargs)

            commands = rotary_wrap.apply_wrap_strategy(raw_commands, rotary_letter, wrap_strategy)

            for cmd in commands:
                self.commandlist.append(cmd)


def SetupProperties():
    """Property names the Setup Sheet may persist defaults for."""
    return [
        "StepOver",
        "AngularResolution",
        "RadialStockToLeave",
        "CutMode",
        "CutPattern",
        "FeedMode",
        "MaxFeed",
        "BoundaryFromFaces",
        "LinearDeflection",
        "AngularDeflection",
    ]


def Create(name, obj=None, parentJob=None):
    """Factory used by the Op-Gui SetupOperation."""
    if obj is None:
        obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", name)
    obj.Proxy = ObjectRotarySurface(obj, name, parentJob)
    return obj
