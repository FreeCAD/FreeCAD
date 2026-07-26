# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2026 Dimitris75 <dimitriospana75@gmail.com>               *
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

"""Z-Level Hybrid (constant-Z contour) generation using native geometry.

Implements a high-performance, geometric-only alternative to OCL-based operations.
Utilizes FreeCAD's native slicing kernel combined with the Path.Area (ClipperLib)
C++ engine for precise tool radius compensation, linear radius sub-sampling,
and robust layer-wise masking. Automatically detects and reconciles CAD floors
to provide a complete hybrid finishing strategy for both steep walls and flat areas.
"""

import math
import FreeCAD
import Path
import Part

if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


# ---------------------------------------------------------------------------
# Boundary preparation
# ---------------------------------------------------------------------------


def extendedBoundBox(wBB, bbBfr, zDep):
    """
    Creates a large, oversized rectangular wire from a given bounding box.

    This wire serves as the absolute outermost boundary or "canvas" for the Z-Level
    strategy. It is intentionally made much larger than the stock to ensure that
    any boolean subtractions near the model's edge have a clean, unambiguous
    area to cut from.

    Args:
        wBB (FreeCAD.BoundBox): The source bounding box (typically from the stock or model).
        bbBfr (float): The buffer or margin distance to expand the box by in X and Y.
        zDep (float): The Z-height at which to create the 2D wire.

    Returns:
        Part.Wire: A closed, rectangular Part.Wire object.
    """
    p1 = FreeCAD.Vector(wBB.XMin - bbBfr, wBB.YMin - bbBfr, zDep)
    p2 = FreeCAD.Vector(wBB.XMax + bbBfr, wBB.YMin - bbBfr, zDep)
    p3 = FreeCAD.Vector(wBB.XMax + bbBfr, wBB.YMax + bbBfr, zDep)
    p4 = FreeCAD.Vector(wBB.XMin - bbBfr, wBB.YMax + bbBfr, zDep)
    return Part.makePolygon([p1, p2, p3, p4, p1])


def getTrimFace(borderFace, bbFace, wpc):
    """
    Calculates the 'Outside World' mask used to clip the toolpath.

    This function takes a giant outer boundary (`borderFace`) and subtracts the
    model's actual 2D silhouette (`bbFace`) from it. The result is a face with a
    hole in it, representing everything *outside* the area to be machined. This
    "trim face" is used in later boolean operations to ensure the toolpath does
    not extend beyond the model's perimeter.

    Args:
        borderFace (Part.Face): The oversized outer boundary created by extendedBoundBox.
        bbFace (Part.Face): The 2D silhouette of the model or stock.
        wpc (Part.Wire): The workplane context for the Path.Area (ClipperLib) engine.

    Returns:
        Part.Shape: The final trim face shape, or an empty shape on failure.
    """
    trim_engine = Path.Area()
    trim_engine.setPlane(wpc)
    trim_engine.add(borderFace)

    if bbFace:
        bb_copy = bbFace.copy()
        bb_copy.translate(FreeCAD.Vector(0, 0, -bb_copy.BoundBox.ZMin))
        trim_engine.add(bb_copy, op=1)

    trim_face = trim_engine.getShape()

    try:
        if hasattr(trim_face, "removeSplitter"):
            trim_face = trim_face.removeSplitter()
    except Exception as e:
        Path.Log.debug(
            f"surface_zlevel.getTrimFace: Removing splitter on trim face failed: {str(e)}"
        )

    return trim_face


# ---------------------------------------------------------------------------
# Depth categorization
# ---------------------------------------------------------------------------


def categorize_floor_steps(shape, start_z, final_z, step_down, clear_planar_only):
    """Reconciles physical model floors with calculated step-down heights.

    This function generates a top-down list of Z-depths starting from start_z
    to final_z. It then analyzes the model geometry to find horizontal faces
    (floors) and categorizes each depth as 'Pure' (standard step), 'Mixed'
    (step lands on a floor), or 'Extra' (floor exists between standard steps).

    Args:
        shape: The manifold Part.Shape of the model to analyze.
        start_z: The absolute Z-height where machining begins (mm).
        final_z: The absolute target Z-depth (mm).
        step_down: The desired vertical distance between passes (mm).

    Returns:
        A list of tuples: (z_height, status, floor_geometry_at_Z0).
        Statuses are strings: "Pure", "Mixed", or "Extra".
    """
    # 1. Generate standard Z-heights list top-down
    z_heights = []
    curr_z = start_z - step_down
    while curr_z > (final_z + 0.0001):
        z_heights.append(round(curr_z, 5))
        curr_z -= step_down
    z_heights.append(round(final_z, 5))

    # 2. Get physical floors from model geometry
    fused_geometry = _get_fused_floor_geometry(shape, start_z, final_z)

    final_depth_logic = []
    accounted_floors = set()

    # 3. Match standard steps to physical floors
    for z_std in z_heights:
        match_z = None
        for floor_z in fused_geometry.keys():
            if abs(floor_z - z_std) < 0.0005:
                match_z = floor_z
                break

        if match_z is not None:
            if clear_planar_only:
                # We are in "Clear Planar Only" mode. Split the "Mixed" step
                # into two separate virtual passes for the main loop to process.
                final_depth_logic.append(
                    (z_std + 0.0001, "Pure", None)
                )  # Set higher to be processed first
                final_depth_logic.append((z_std, "Extra", fused_geometry[match_z]))
            else:
                final_depth_logic.append((z_std, "Mixed", fused_geometry[match_z]))
            accounted_floors.add(match_z)
        else:
            final_depth_logic.append((z_std, "Pure", None))

    # 4. Add intermediate floors as 'Extra' steps
    for z_f, geo in fused_geometry.items():
        if z_f not in accounted_floors:
            final_depth_logic.append((z_f, "Extra", geo))

    final_depth_logic.sort(key=lambda x: x[0], reverse=True)
    return final_depth_logic


def _get_fused_floor_geometry(shape, start_z, final_z, tolerance=0.001):
    """Identifies and fuses upward-facing horizontal faces within the machining range.

    Iterates through all faces of the shape, filtering for planar surfaces
    whose normal vector points strictly toward the tool (+Z). It performs
    an accessibility check to ensure the floor is not occluded by geometry
    above it and fuses coincident faces at the same height into single regions.

    Args:
        shape: The Part.Shape to analyze.
        start_z: Upper vertical bound for floor detection (mm).
        final_z: Lower vertical bound for floor detection (mm).
        tolerance: Distance threshold for considering faces coplanar (mm).

    Returns:
        A dictionary: {z_height: fused_face_at_Z0}.
    """

    def is_upward(face):
        if not (hasattr(face.Surface, "TypeId") and "Plane" in face.Surface.TypeId):
            return False
        u1, u2, v1, v2 = face.ParameterRange
        norm = face.normalAt((u1 + u2) / 2.0, (v1 + v2) / 2.0)
        if face.Orientation == "Reversed":
            norm = norm.multiply(-1)
        return norm.z > 0.99

    def isAccessibleFromTop(face, shape, abs_top):
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

    FACE_COUNT_THRESHOLD = 250
    if len(shape.Faces) > FACE_COUNT_THRESHOLD:
        Path.Log.info(
            f"Shape has >{FACE_COUNT_THRESHOLD} faces. Skipping slow planar floor detection for performance."
        )
        return {}

    floor_accumulator = {}
    abs_top = shape.BoundBox.ZMax
    z_min, z_max = min(start_z, final_z), max(start_z, final_z)

    for face in shape.Faces:
        if is_upward(face):
            if isAccessibleFromTop(face, shape, abs_top):
                z = round(face.Vertexes[0].Z, 5)
                if (z >= z_min - tolerance) and (z < z_max):
                    f_copy = face.copy()
                    f_copy.translate(FreeCAD.Vector(0, 0, -f_copy.BoundBox.ZMin))

                    if z not in floor_accumulator:
                        floor_accumulator[z] = []
                    floor_accumulator[z].append(f_copy)

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


# ---------------------------------------------------------------------------
# Z-Level Hybrid layer generation
# ---------------------------------------------------------------------------


def zlevel_hybrid_stack(
    shape,
    categorizedSteps,
    borderFace,
    trimFace,
    tool_params,
    stock_to_leave,
    accuracy_val,
    z_offset,
    wpc,
    start_z,
):
    """Calculates a stack of 2D clearing areas using geometric slicing and Clipper Booleans.

    This function processes the 3D model layer-by-layer. For each layer, it generates
    a composite silhouette by sub-sampling the model curvature, applies tool radius compensation,
    and resolves the final machining area using a persistent C++ masking engine.
    Uses a dual Squeeze-and-Snap strategy: Pack samples at the tool tip to handle
    high-curvature contact, and snap samples to model floors for precise transitions.
    Linear radius sampling is performed equator-first to enable geometric
    caching on vertical walls.

    Args:
        shape: The source Part.Shape to be machined.
        categorizedSteps: List of tuples (z_target, status, floor_geo) from categorization.
        borderFace: A Part.Face representing the stock or boundary footprint.
        trimFace: A Part.Face representing the 'Outside World' ( forbidden zone).
        tool_params: Dict containing 'radius', 'c_rad', 'profile', 'is_threeD'.
        stock_to_leave: Horizontal (XY) distance to keep from the model (mm).
        accuracy_val: Integer or string representing the number of sub-slices.
        z_offset: Vertical (Axial) distance to shift the final paths (mm).
        wpc: The Part.Circle workplane defining the 2D calculation plane.
        start_z: The Start Depth of the operation.

    Returns:
        A list of tuples: (z_target, cutAreaShape, status).
    """
    Path.Log.debug("surface_zlevel.zlevel_hybrid_stack: Starting geometric stack generation.")

    # 1. Initialization
    stack = []
    sub_face = None
    allPrevComp = None
    tol = 0.0001

    c_rad = tool_params["c_rad"]
    is_3d = tool_params["is_threeD"]
    try:
        num_slices = int(accuracy_val) if is_3d else 1
    except (ValueError, TypeError):
        Path.Log.warning(f"Invalid accuracy_val '{accuracy_val}', defaulting to 4 slices.")
        num_slices = 4 if is_3d else 1
    # 2. Pre-load C++ engine
    area_engine = Path.Area()
    area_engine.setPlane(wpc)

    # Ensure we work on a clean copy
    proc_shape = shape.copy()
    area_engine.add(proc_shape)

    # Configure C++ engine parameters
    params = area_engine.getParams()
    params["SectionTolerance"] = 0.0001

    # 3. Identify critical snapping depths (Top and floors)
    model_bottom, model_top = proc_shape.BoundBox.ZMin, proc_shape.BoundBox.ZMax
    critical_heights = {
        round(h, 6) for h, status, _ in categorizedSteps if status in ["Mixed", "Extra"]
    }

    critical_heights.add(round(model_top, 6))
    if abs(start_z - model_top) > tol:
        critical_heights.add(start_z - tol)

    # 4. Main layer loop
    for z_target, status, floor_geo in categorizedSteps:

        # Reject steps strictly above the model top
        if z_target > (model_top - tol):
            Path.Log.warning(
                f"Skipping step at Z={z_target:.3f}mm as it is above the model top (max Z: {model_top:.3f}mm)."
            )
            continue

        # Determine the Slice Height (Model Footprint)
        z_slice = z_target
        if z_slice < model_bottom:
            z_slice = model_bottom

        # The depth at which the tool has submerged from the model_top
        dist_submerged = max(0, model_top - z_slice)
        # Nudge slice height based on whether we are clearing a floor or a wall
        # Standard layers nudge up (+); Floors nudge down (-) to stay inside material
        slice_bias = 0.0002 if status in ["Mixed", "Extra"] else -0.0002

        # A. Generate sampling plan (Height, Radius pairs)
        unique_steps = _generate_sampling_plan(
            z_slice, dist_submerged, tol, critical_heights, num_slices, tool_params
        )

        # B. Generate all 2D slices for this layer
        layer_slices = _generate_layer_slices(
            area_engine,
            params,
            unique_steps,
            z_target,
            slice_bias,
            stock_to_leave,
            model_top,
            model_bottom,
        )

        if not layer_slices:
            continue

        # C. Fuse all generated slices into a single silhouette for this layer
        fusion = Path.Area()
        fusion.setPlane(wpc)
        for s in layer_slices:
            fusion.add(s)

        # C. Boolean resolution
        currentSilhouette = None
        try:
            # currentSilhouette is the union of all 3D contact points at this depth
            currentSilhouette = fusion.getShape()

            # if hasattr(currentSilhouette, "removeSplitter"):
            #    currentSilhouette = currentSilhouette.removeSplitter()
        except Exception as e:
            Path.Log.error(f"Silhouette fusion failed at Z={round(z_target, 3)}. Error: {str(e)}")
            continue

        # D: Calculate the final cutting area using the new helper
        cutArea = _calculate_cut_area(
            wpc, status, currentSilhouette, floor_geo, borderFace, trimFace, allPrevComp, z_target
        )

        # E: Finalize and store the result for this layer
        if cutArea:
            total_shift = z_target + z_offset

            final_cut = cutArea.copy()
            final_cut.translate(FreeCAD.Vector(0, 0, total_shift))

            # Store target G-code depth, calculated geometry, and metadata
            stack.append((total_shift, final_cut, status))

            # Update Persistent Mask (strictly model silhouette to keep pockets open)
            allPrevComp = _update_machining_mask(
                wpc, allPrevComp, currentSilhouette, status, floor_geo
            )

    return stack


def _generate_layer_slices(
    area_engine,
    params,
    unique_steps,
    z_target,
    slice_bias,
    stock_to_leave,
    model_top,
    model_bottom,
):
    """
    For a single Z-level, generates all the necessary 2D slices based on the tool's 3D profile.

    This function iterates through the provided sampling plan (unique_steps), calculates
    the precise slice height and tool-compensated offset for each sample, and calls the
    C++ Area engine to produce the raw 2D geometry.

    Args:
        area_engine (Path.Area): The pre-configured C++ slicing engine.
        params (dict): The parameter dictionary for the area_engine.
        unique_steps (set): A set of (height, radius) tuples from the sampling plan.
        z_target (float): The base Z-height for the current machining layer.
        slice_bias (float): A small nudge value for the slice height.
        stock_to_leave (float): The horizontal stock to leave.
        model_top (float): The absolute maximum Z of the model.
        model_bottom (float): The absolute minimum Z of the model.

    Returns:
        list: A list of normalized Part.Shape objects representing the slices at Z=0.
    """
    slices = []
    for h, r_theo in unique_steps:
        r_comp = r_theo + stock_to_leave

        # Synchronized Slicing: Calculate the precise Z for this sample
        slice_z = max(model_bottom + 1e-5, min(z_target + h + slice_bias, model_top - 1e-5))

        # Trigger C++ Slicing with the dynamic offset for this sample
        params["Offset"] = r_comp
        area_engine.setParams(**params)

        sections = area_engine.makeSections(mode=0, project=False, heights=[slice_z])

        if not sections:
            # Note: A fall back strategy can be added here.
            continue

        sub_face = sections[0].getShape()

        # Move results to the machine plane (Z=0) for consistent fusion
        sub_face.translate(FreeCAD.Vector(0, 0, -sub_face.BoundBox.ZMin))
        slices.append(sub_face)

    return slices


def _calculate_cut_area(
    wpc,
    status,
    currentSilhouette,
    floor_geo,
    borderFace,
    trimFace,
    allPrevComp,
    z_target,
):
    """
    Calculates the final 2D machining area for a single layer by performing
    a series of boolean (Clipper) operations.

    This function encapsulates the core "what to cut" logic, handling the differences
    between a standard clearing pass and a surgical "Extra" pass for detected floors.

    Args:
        wpc (Part.Circle): The workplane for the Path.Area engine.
        status (str): The status of the layer ("Pure", "Mixed", "Extra").
        currentSilhouette (Part.Shape): The tool-compensated model silhouette at Z=0.
        floor_geo (Part.Shape): The detected physical floor geometry at Z=0.
        borderFace (Part.Shape): The main stock boundary at Z=0.
        trimFace (Part.Shape): The "outside world" mask to clip the toolpath.
        allPrevComp (Part.Shape): The cumulative mask of areas already machined in upper layers.
        z_target (float): The current Z-height, used for logging errors.

    Returns:
        Part.Shape: The final, calculated cutting area for the layer, or None on failure.
    """
    layer_engine = Path.Area()
    layer_engine.setPlane(wpc)

    if status == "Extra":
        # Surgical Floor Mode: The area to machine is defined only by the
        # physical floor geometry minus the current model silhouette.
        # Stock = Floor; Material = Stock - Model
        if floor_geo:
            layer_engine.add(floor_geo)
            layer_engine.add(currentSilhouette, op=1)  # Subtract model

    else:
        # Standard Mode: The area to machine is the stock boundary, minus the model,
        # minus any areas we've already cleared, and clipped by the trim mask.
        # Stock = Border; Material = (Stock - Model - PreviouslyCleared) - TrimMask
        layer_engine.add(borderFace)
        layer_engine.add(currentSilhouette, op=1)

        # Rest Machining: subtract material cleared in layers above
        if allPrevComp:
            layer_engine.add(allPrevComp, op=1)

        # Apply the 'outside world' mask only in standard mode
        if trimFace:
            layer_engine.add(trimFace, op=1)

    try:
        cutArea = layer_engine.getShape()
    except Exception as e:
        Path.Log.error(f"Layer engine failed at Z={round(z_target, 3)}. Error: {str(e)}")
        cutArea = None

    return cutArea


def _generate_sampling_plan(
    z_target, dist_submerged, tol, critical_heights, num_slices, tool_params
):
    """Generates a sorted, unique list of (height, radius) sampling pairs for 3D tool compensation.

    This function implements the core 'Squeeze-and-Snap' strategy. It calculates a
    distribution of sample points along the tool's corner profile to ensure the generated
    silhouette accurately reflects the tool's 3D shape at a given depth.

    Args:
        z_target (float): The target machining depth for the current layer.
        dist_submerged (float): Vertical distance from the tool tip to the model top.
        tol (float): Geometric tolerance for floating point comparisons.
        critical_heights (set): Absolute Z-heights of physical model floors/top.
        num_slices (int): Base number of samples to generate along the tool profile.
        tool_params (dict): Tool geometry containing 'radius', 'c_rad', 'profile', 'is_threeD'.

    Returns:
        set: A unique set of (height, radius) tuples representing the points to sample.
    """

    # 1. Setup & Geometry normalization
    # Extract core tool geometry parameters
    R = tool_params["radius"]
    c_rad = tool_params["c_rad"]
    profile = tool_params["profile"]
    is_3d = tool_params["is_threeD"]

    # A Ball Endmill is mathematically equivalent to a Bullnose tool where the corner radius
    # is equal to the tool radius. Normalizing c_rad here allows us to use
    # the same 'bullnose' formulas for both tool types, simplifying the math.

    if "ballend" in profile:
        c_rad = R

    # 2. Internal math helpers
    # These functions calculate the 3D profile of the tool using the Pythagorean theorem

    def _get_h_from_r(r_target):
        """Inverse Math: For a given horizontal radius (r_target), find the vertical height (h) on the tool's corner."""
        flat_radius = R - c_rad
        # If the target radius is on the flat bottom part of the tool, the height is 0
        if r_target <= flat_radius + 1e-7:
            return 0.0
        # Otherwise, calculate height on the curve using the equation for a circle
        return c_rad - math.sqrt(max(0, c_rad**2 - (r_target - flat_radius) ** 2))

    def _get_r_from_h(h_target):
        """Forward Math: For a given vertical height (h_target), find the horizontal radius (r) on the tool's corner."""
        if not is_3d:
            return R

        # If the height is within the curved portion, calculate the radius
        if h_target < c_rad:
            flat_radius = R - c_rad
            return flat_radius + math.sqrt(max(0, c_rad**2 - (c_rad - h_target) ** 2))
        # If the height is above the corner radius, the tool is at its maximum radius
        return R

    # 3. Generate the Sampling plan
    plan = []

    if is_3d:  # This block handles 3D tools (Ballnose, Bullnose)
        # Determine the widest radius of the tool currently in contact with the model
        max_r = _get_r_from_h(dist_submerged) if dist_submerged < c_rad else R

        # Calculate the vertical 'ceiling' of the tool's 3D profile that is in contact
        h_ceiling = min(c_rad, dist_submerged - tol)

        # A) Squeeze Logic: Generate evenly spaced samples along the tool's contact radius
        min_r = R - c_rad if "bullnose" in profile else 0.0
        squeeze_range = max_r - min_r

        for i in range(num_slices):
            # Linearly interpolate between the minimum contact radius and the maximum
            r_theo = min_r + (squeeze_range / (num_slices - 1)) * i
            plan.append((_get_h_from_r(r_theo), r_theo))

        # B) Snap Logic: Add extra, precise samples that land exactly on physical model floors
        for ch in critical_heights:
            rel_h = ch - z_target
            # Only snap if the floor is within the tool's active 3D contact zone for this layer
            if 0.001 < rel_h < (h_ceiling - 0.001):
                plan.append((rel_h, _get_r_from_h(rel_h)))

        # D) Finalize
        # Convert the plan to a set to automatically remove any duplicate sample points
        # that may have been generated by the squeeze and snap logic. Rounding prevents
        # minor floating-point noise from creating unnecessary extra samples.
        unique_steps = {(round(h, 6), round(r, 6)) for h, r in plan}

    else:  # This block handles simple 2D tools (Flat Endmills)
        # A flat endmill only needs one sample point at its maximum contact radius
        unique_steps = {(0.0, R)}

    return unique_steps


def _update_machining_mask(wpc, allPrevComp, currentSilhouette, status, floor_geo):
    """Updates the persistent cumulative mask with new cleared areas.

    This function maintains a 'shadow' of all material processed in layers
    above the current depth. It performs a C++ union of the previous mask,
    the current model footprint, and any detected physical floors. This
    prevents the tool from air-cutting previously finished surfaces.

    Args:
        wpc (Part.Circle): The workplane for Path.Area operations.
        allPrevComp (Part.Shape): The cumulative mask from previous layers.
        currentSilhouette (Part.Shape): The tool-compensated footprint of
            the current model slice.
        status (str): The layer status ("Pure", "Mixed", or "Extra").
        floor_geo (Part.Face): The physical floor geometry detected at this depth.

    Returns:
        Part.Shape: The updated, dissolved cumulative mask at Z=0.
    """

    mask_engine = Path.Area()
    mask_engine.setPlane(wpc)

    # 1. Add the mask from all layers above
    if allPrevComp and not allPrevComp.isNull():
        mask_engine.add(allPrevComp)

    # 2. Add the current model silhouette (the walls/islands)
    mask_engine.add(currentSilhouette)

    # 3. Add physical floors (Mixed or Extra)
    # This treats 'shelves' as solid barriers for all layers below
    if status in ["Mixed", "Extra"] and floor_geo:
        # floor_geo was normalized to Z=0 during categorization
        mask_engine.add(floor_geo)

    # 4. Extract the dissolved result
    try:
        allPrevComp = mask_engine.getShape()
    except Exception as e:
        Path.Log.error(f"Machining mask update failed: {str(e)}")
        raise

    return allPrevComp


# ---------------------------------------------------------------------------
# G-code generation
# ---------------------------------------------------------------------------


def zlevel_hybrid_to_gcode(
    stack,
    feed_params,
    height_params,
    pattern_options,
    ignore_outer,
    clear_planar_only,
    step_over,
    radius,
):
    """Converts the geometry stack into G-code Path Commands.

    This function iterates through a pre-calculated stack of geometric slices,
    generating perimeter (Waterline) paths and optional floor-clearing patterns.
    It manages tool engagement directions, safety transitions to safe heights,
    and progress reporting.

    Args:
        stack: A list of tuples (z_target, cutArea, status) representing layers.
        feed_params: Dict containing 'horizFeed', 'vertFeed', 'horizRapid', 'vertRapid'.
        height_params: Dict containing 'safe_hght' and 'clearance_hght'.
        pattern_options: Dict containing 'cut_climb', 'cut_pattern', 'pattern_angle',
            'reverse_pattern'.
        ignore_outer: Boolean. If True, skips the outermost boundary (stock edge).
        clear_planar_only: Boolean. If True, only clears floors detected as
            Mixed or Extra.
        step_over: The horizontal step-over distance for clearing patterns (mm).
        radius: The tool radius (mm).

    Returns:
        A list of Path.Command objects (G-code).
    """
    Path.Log.debug("surface_zlevel.zlevel_hybrid_to_gcode: Starting G-code generation.")

    # 1. Initialization
    commands = []

    # Extract feeds and speeds
    v_rapid = feed_params.get("vertRapid", 0.0)

    # Extract heights
    safe_hght = height_params.get("safe_hght", 3.0)
    clear_hght = height_params.get("clearance_hght", 5.0)

    # Extract pattern logic
    cut_climb = pattern_options.get("cut_climb", True)
    pattern_name = pattern_options.get("cut_pattern", "None")
    pattern_angle = pattern_options.get("pattern_angle", 0.0)
    reverse_pattern = pattern_options.get("reverse_pattern", False)

    # 2. Main Layer Processing
    for z_target, cutArea, status in stack:

        if not cutArea or cutArea.isNull() or not cutArea.Wires:
            continue

        # Cut pattern reversed
        working_area = cutArea.reversed() if reverse_pattern else cutArea

        # Determine start index (0 = machine stock edge, 1 = ignore stock edge)
        start_w_idx = 1 if ignore_outer else 0

        # A: Perimeters (Waterline Walls)
        if start_w_idx < len(working_area.Wires):
            for w_idx in range(start_w_idx, len(working_area.Wires)):
                wire = working_area.Wires[w_idx]
                if not wire.isClosed():
                    continue

                # Geometry cleanup
                if hasattr(wire, "removeSplitter"):
                    wire = wire.removeSplitter()
                wire.fix(1e-6, 1e-6, 1e-4)

                # Determine start point coordinates
                V = wire.Vertexes
                lv = len(V) - 1
                # Start at the end vertex for Climb to move backward through CCW wire
                start_p = (
                    FreeCAD.Vector(V[lv].X, V[lv].Y, V[lv].Z)
                    if cut_climb
                    else FreeCAD.Vector(V[0].X, V[0].Y, V[0].Z)
                )

                # Generate the wire-following path
                commands.extend(
                    _generate_wire_path(wire, z_target, safe_hght, start_p, feed_params)
                )

        # B: Cut pattern
        should_clear = False
        if pattern_name != "None":
            if clear_planar_only:
                # Targeted mode: only clear physical model floors
                if status in ["Mixed", "Extra"]:
                    should_clear = True
            else:
                # Global mode: clear every depth level
                should_clear = True

        if should_clear:
            # Ensure tool is at a safe level before moving into the pattern
            commands.append(Path.Command("G0", {"Z": safe_hght, "F": v_rapid}))

            # Dispatch to the high-speed Path.Area pattern engine
            pattern_cmds = _generatePattern(
                cutArea,
                pattern_name,
                pattern_angle,
                cut_climb,
                reverse_pattern,
                z_target,
                step_over,
                radius,
                feed_params,
                safe_hght,
            )
            commands.extend(pattern_cmds)

    # 3. Finalize Operation

    # Return to clearance height
    commands.append(Path.Command("G0", {"Z": clear_hght, "F": v_rapid}))

    return commands


def _generatePattern(
    cutArea,
    cut_pattern,
    pattern_angle,
    cut_climb,
    reverse_pattern,
    z_target,
    step_over,
    radius,
    feed_params,
    safe_hght,
):
    """Generates high-speed infill patterns using the native C++ Path.Area engine.

    This function utilizes the Clipper-based C++ kernel to calculate 2D clearing
    patterns (ZigZag, Offset, Line, Grid) within a provided boundary. It handles
    tool radius compensation, pattern rotation, and machining sequence.

    Args:
        cutArea: A Part.Face or Part.Shape representing the boundary to clear.
        cut_pattern: String identifier for the pattern (ZigZag, Offset, Line, Grid).
        pattern_angle: Float representing the yaw angle (degrees) for scanline patterns.
        cut_climb: Boolean. If True, uses Climb milling; otherwise, Conventional.
        reverse_pattern: Boolean. If True, reverses the clearing order (e.g., Inside-Out).
        z_target: The target Z-coordinate for the toolpath (machining depth).
        step_over: The horizontal distance between consecutive passes (mm).
        radius: The tool radius used for extra offset calculation (mm).
        feed_params: Dictionary containing 'horizFeed' and 'vertFeed' values.
        safe_hght: The Z-height for rapid transitions between segments.

    Returns:
        A list of Path.Command objects representing the clearing G-code.
    """
    Path.Log.debug(
        f"surface_zlevel._generatePattern: Generating {cut_pattern} pattern at Z={z_target}"
    )
    commands = []
    should_reverse = True

    # 1. Validation Guards
    if not cutArea or cutArea.isNull():
        Path.Log.warning("Pattern generation skipped - Empty cutting area.")
        return []

    if cutArea.Area < 1e-7:
        return []

    # 2. Engine Setup
    v_rapid = feed_params.get("vertRapid", 0.0)

    engine = Path.Area()
    engine.add(cutArea)

    # 3. Map UI Strategy to C++ PocketMode
    if cut_pattern == "ZigZag":
        pattern_mode = 1
    elif cut_pattern == "Offset":
        pattern_mode = 2
        # Specific logic for Offset: Climb vs Reverse
        if cut_climb != reverse_pattern:
            should_reverse = False
    elif cut_pattern == "Line":
        pattern_mode = 5
    elif cut_pattern == "Grid":
        pattern_mode = 6
    else:
        Path.Log.error(f"Unsupported pattern type '{cut_pattern}'")
        return []

    # 4. Configure C++ Solver Parameters
    extra_offset = radius - step_over
    params = engine.getParams()
    params["PocketMode"] = pattern_mode
    params["PocketStepover"] = step_over
    params["PocketExtraOffset"] = -extra_offset
    params["Angle"] = float(pattern_angle)
    params["ToolRadius"] = radius
    params["FromCenter"] = reverse_pattern

    engine.setParams(**params)

    # 5. Execute Native Solver
    try:
        engine.makePocket()
        res_area = engine.getShape()
    except Exception as e:
        Path.Log.error(f"Pattern G-code generation failed: {str(e)}")
        return []

    if not res_area or res_area.isNull():
        return []

    # Apply topological reversal for Climb milling on Offset rings if needed
    if should_reverse:
        res_area = res_area.reversed()

    # 6. G-Code Generation Loop
    for wire in res_area.Wires:
        if not wire.isClosed() and pattern_mode == 2:  # Offsets should be closed
            continue

        start_p = wire.Vertexes[0].Point

        # Generate the wire-following path
        commands.extend(_generate_wire_path(wire, z_target, safe_hght, start_p, feed_params))

        # Safety Retract after each segment (island or ring)
        commands.append(Path.Command("G0", {"Z": safe_hght, "F": v_rapid}))

    return commands


def _generate_wire_path(wire, z_target, safe_hght, start_p, feed_params):
    """Standardizes G-code generation for a single wire segment.

    Args:
        wire (Part.Wire): The geometric path to follow.
        z_target (float): The target machining depth.
        safe_hght (float): The height for safe transitions.
        start_p (Vector): The optimized starting point.
        feed_params: Dictionary containing 'horizFeed' and 'vertFeed' values.

    Returns:
        list: A list of Path.Command objects.
    """

    commands = []

    # Extract feeds and speeds
    h_feed = feed_params.get("horizFeed", 0.0)
    v_feed = feed_params.get("vertFeed", 0.0)
    h_rapid = feed_params.get("horizRapid", 0.0)
    v_rapid = feed_params.get("vertRapid", 0.0)

    # Safety transition: Rapid to SafeHeight, then to start position
    commands.append(Path.Command("G0", {"Z": safe_hght, "F": v_rapid}))
    commands.append(Path.Command("G0", {"X": start_p.x, "Y": start_p.y, "F": h_rapid}))
    # Move to depth (plunge)
    commands.append(Path.Command("G1", {"Z": z_target, "F": v_feed}))

    path_params = {
        "shapes": [wire],
        "feedrate": h_feed,
        "start": start_p,
        "preamble": False,
        "verbose": True,
        "retraction": safe_hght,
        "resume_height": safe_hght,
    }

    try:
        pp = Path.fromShapes(**path_params)
    except Exception as e:
        Path.Log.error(f"Path.fromShapes failed at Z={z_target}: {str(e)}")
        return []

    # Extend Commands list
    commands.extend(pp.Commands)
    # Return commands
    return commands
