# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 Dimitrios Pana <dimitriospana75@gmail.com>
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
# Fill selected holes
# ---------------------------------------------------------------------------


def _apply_fill_hole_masks(
    wpc,
    fill_holes_masks,
    fill_mask_idx,
    current_silhouette,
    status,
    floor_geo,
    all_prev_comp,
    z_target,
    loose_tol,
):
    """
    Processes and applies active fill-hole masks for the current layer.

    Masks are activated top-down: once z_target passes a mask's threshold height,
    it becomes active. Processed masks are ultimately discarded from the pending list
    because their geometry is permanently baked into the cumulative keep-out mask.

    Application logic:
    1. Permanent Retention: Every activated mask is unconditionally injected into
       `all_prev_comp`. This creates a permanent keep-out zone, ensuring the tool
       will not fall into the hole on this layer or any subsequent lower layers.
    2. Immediate Fusion: If the current layer is "Mixed" or "Extra" (a floor pass),
       the mask is simultaneously fused into `floor_geo`. This allows the clearing
       pattern for the current floor to smoothly machine across the capped void
       without leaving uncut gaps.

    Args:
        wpc (Part.Circle): Workplane for Path.Area engine.
        fill_holes_masks (list): Sorted list of (max_z, Part.Face) tuples.
        fill_mask_idx (int): Current index into fill_holes_masks.
        current_silhouette (Part.Shape): Model silhouette at current depth.
        status (str): Current layer status — "Pure", "Mixed", or "Extra".
        floor_geo (Part.Shape): Floor geometry for this layer, or None.
        all_prev_comp (Part.Shape): Cumulative mask of previously cleared areas.
        z_target (float): Current layer Z height.
        loose_tol (float): Tolerance for mask activation threshold.

    Returns:
        tuple: (fill_mask_idx, fill_holes_masks, floor_geo, all_prev_comp) — updated values
               for all four mutable state variables.
    """
    if status in ("Mixed", "Extra") and floor_geo is not None:
        merge_engine = Path.Area()
        merge_engine.setPlane(wpc)

    while (
        fill_mask_idx < len(fill_holes_masks)
        and fill_holes_masks[fill_mask_idx][0] >= z_target - loose_tol
    ):

        mask = fill_holes_masks[fill_mask_idx][1]

        # 1.Permanent Retention
        all_prev_comp = _update_machining_mask(
            wpc, all_prev_comp, mask, status="Pure", floor_geo=None
        )

        # 2.Immediate Fusion
        if status in ("Mixed", "Extra") and floor_geo is not None:
            try:
                merge_engine.add(floor_geo)
                merge_engine.add(mask, op=0)
                merged = merge_engine.getShape()
                if merged and not merged.isNull():
                    floor_geo = merged
            except Exception as e:
                Path.Log.warning(f"Failed to fuse hole mask into floor geometry: {e}")

        fill_mask_idx += 1

    # Discard processed masks safely because their geometry is now permanently baked into all_prev_comp
    fill_holes_masks = fill_holes_masks[fill_mask_idx:]
    fill_mask_idx = 0

    return fill_mask_idx, fill_holes_masks, floor_geo, all_prev_comp


def _fuse_coplanar_masks(fill_holes_masks):
    """
    Groups fill-hole masks by Z height and fuses any faces sharing the
    same height into a single shape.

    Args:
        fill_holes_masks (list): List of (max_z, Part.Face) tuples.

    Returns:
        list: List of (max_z, Part.Shape) tuples with co-planar faces fused,
              sorted by Z descending.
    """
    from itertools import groupby

    sorted_list = sorted(fill_holes_masks, key=lambda x: x[0], reverse=True)
    fused_list = []

    for max_z, group in groupby(sorted_list, key=lambda x: x[0]):
        faces = [item[1] for item in group]

        if len(faces) == 1:
            fused_list.append((max_z, faces[0]))
            continue
        try:
            fused = Part.makeCompound(faces)
            if hasattr(fused, "removeSplitter"):
                fused = fused.removeSplitter()
            fused_list.append((max_z, fused))
            Path.Log.debug(f"_fuse_coplanar_masks: Fused {len(faces)} masks at Z={max_z:.4f}.")
        except Exception as e:
            Path.Log.warning(
                f" Failed to compound {len(faces)} fill selected holes "
                f"at Z={max_z:.4f}: {e}. Using first face only."
            )
            fused_list.append((max_z, faces[0]))

    return fused_list


def _get_selected_faces(base_property):
    """
    Parses the Path operation's 'Base' property to extract all selected Part.Face objects.

    Args:
        base_property (list): The operation's `obj.Base` list of (object, subnames) tuples.

    Returns:
        list: A flat list of all valid Part.Face objects found in the selection.
    """
    extracted_faces = []
    if not base_property:
        return extracted_faces

    for base, subs in base_property:
        for sub in subs:
            if not sub:
                continue
            try:
                shape = base.Shape.getElement(sub)
                if shape and isinstance(shape, Part.Face):
                    extracted_faces.append(shape)
            except Exception as e:
                Path.Log.debug(
                    f"_get_selected_faces: Bypassed invalid sub-element '{sub}' on '{base.Label}': {str(e)}"
                )
    return extracted_faces


def fill_selected(base_property):
    """
    Creates flat 2D keep-out masks for user-selected geometry to prevent the
    tool from plunging into specified holes or pockets.

    The generation process follows a strict topographical hierarchy:
    1. Extraction: Parses the operation's Base property for valid Part.Face objects.
    2. Z-Grouping: Groups all faces strictly by their absolute maximum Z-height
       to prevent cross-elevation merging.
    3. Separation: At each Z-level, separates faces into touching groups
       (processed collectively) and isolated faces.
    4. Topographical Routing: Delegates isolated faces to specific capping
       strategies (Flat floors vs. 3D sloped/curved walls) based on their
       geometric properties.
    5. Fusion: Fuses all co-planar masks at each Z-height into a single,
       unified Part.Shape.

    Args:
        base_property (list): The operation's `obj.Base` property containing
            the selected geometry tuples.

    Returns:
        list: A list of tuples `(max_z, mask_shape)`, sorted by Z-height in
            descending order. Returns an empty list on failure or if no valid
            faces are found.
    """
    from . import surface_common

    selected_faces = _get_selected_faces(base_property)
    if not selected_faces:
        Path.Log.warning("Fill Selected Holes enabled, but no faces selected. Skipping.")
        return []

    # Group faces STRICTLY by their maximum Z-height
    faces_by_z = {}
    for face in selected_faces:
        z_key = round(face.BoundBox.ZMax, 4)
        if z_key not in faces_by_z:
            faces_by_z[z_key] = []
        faces_by_z[z_key].append(face)

    fill_holes_masks = []

    # Process faces at each Z-level
    for z_level, faces_at_z in faces_by_z.items():
        if len(faces_at_z) > 1:
            touching_groups, isolated_faces = surface_common._separate_touching_faces([faces_at_z])
        else:
            touching_groups = []
            isolated_faces = faces_at_z

        # 1. Process touching group faces collectively
        for group in touching_groups:
            cap_face = surface_common.create_boundary_face(group, offset=0.0)
            if cap_face and not cap_face.isNull():
                cap_face.translate(FreeCAD.Vector(0, 0, -cap_face.BoundBox.ZMin))
                fill_holes_masks.append((z_level, cap_face))
            else:
                Path.Log.warning(f"Failed to process touching face group at Z={z_level}")

        # 2. Process isolated faces individually based on Topography
        for face in isolated_faces:
            if not face.Wires:
                continue

            # Route the face to the correct capping strategy
            fill_holes_masks.extend(_process_isolated_face(face, z_level))

    if not fill_holes_masks:
        Path.Log.warning("Failed to generate any caps for selected holes.")
        return []

    Path.Log.debug(f"Generated {len(fill_holes_masks)} mask(s).")
    return _fuse_coplanar_masks(fill_holes_masks)


def _make_flat_cap(wire, cap_z):
    """
    Discretizes a wire, flattens all edges to cap_z, and returns a
    closed Part.Face translated to Z=0 for use as a 2D mask.
    Returns None on failure.
    """
    flat_edges = []
    for edge in wire.Edges:
        try:
            points = edge.discretize(Distance=0.1)
            flat_pts = [FreeCAD.Vector(p.x, p.y, cap_z) for p in points]
            flat_edge = Part.makePolygon(flat_pts)
            flat_edges.extend(flat_edge.Edges)
        except Exception as e:
            Path.Log.debug(f"surface_zlevel.fill_selected: Edge flatten failed: {e}")
            continue

    if not flat_edges:
        return None

    try:
        sorted_edges = Part.__sortEdges__(flat_edges)
        flat_wire = Part.Wire(sorted_edges)
        cap_face = Part.Face(flat_wire)
        cap_face.translate(FreeCAD.Vector(0, 0, -cap_face.BoundBox.ZMin))
        return cap_face
    except Exception as e:
        Path.Log.warning(f"surface_zlevel.fill_selected: Failed to build cap face: {e}")
        return None


def _process_isolated_face(face, z_level):
    """
    Routes an isolated face to the appropriate capping strategy based on
    its topography (Flat vs 3D Wall) and wire count.

    Args:
        face (Part.Face): The isolated face to process.
        z_level (float): The current Z-height grouping.

    Returns:
        list: A list of (z_level, mask_face) tuples.
    """
    from . import surface_common

    masks = []
    is_flat = face.BoundBox.ZLength < 1e-5

    if is_flat:
        if len(face.Wires) == 1:
            # Simple single flat cap (e.g., flat pocket floor)
            cap_face = surface_common.create_boundary_face([face], offset=0.0)
            if cap_face and not cap_face.isNull():
                cap_face.translate(FreeCAD.Vector(0, 0, -cap_face.BoundBox.ZMin))
                masks.append((z_level, cap_face))
        else:
            # Flat face with inner holes (Scenario A)
            masks.extend(_cap_flat_face_holes(face, z_level))
    else:
        # 3D Wall, such as a pipe, tapered, or sloped hole (Scenario B)
        mask_tuple = _cap_3d_wall(face)
        if mask_tuple:
            masks.append(mask_tuple)

    return masks


def _cap_flat_face_holes(face, cap_z):
    """
    Scenario A: Capping inner holes on a mathematically flat surface.
    Sorts wires by size; the largest is the outer boundary, the rest are holes.
    """
    masks = []
    # BoundingBox DiagonalLength is a fast, crash-proof way to sort wire size
    sorted_wires = sorted(face.Wires, key=lambda w: w.BoundBox.DiagonalLength)

    # The largest wire is the outer boundary. The rest are the inner holes.
    inner_wires = sorted_wires[:-1]

    for wire in inner_wires:
        if not wire.isClosed():
            continue
        cap_face = _make_flat_cap(wire, cap_z)
        if cap_face:
            masks.append((cap_z, cap_face))

    return masks


def _cap_3d_wall(face):
    """
    Scenario B: Extracting and capping the top rim of a 3D wall (e.g., sloped holes).
    Safely ignores vertical seams and crushes 3D splines into flat 2D polygons.
    """
    face_zmax = round(face.BoundBox.ZMax, 4)
    face_zmin = round(face.BoundBox.ZMin, 4)

    top_edges = []
    for edge in face.Edges:
        # 1. Identify and skip "seam" edges that run vertically down the walls
        is_seam = (round(edge.BoundBox.ZMin, 4) <= face_zmin + 1e-3) and (
            round(edge.BoundBox.ZMax, 4) >= face_zmax - 1e-3
        )
        if is_seam:
            continue

        # 2. Keep only the edges that form the upper rim
        if round(edge.BoundBox.ZMax, 4) >= face_zmax - 1e-3:
            top_edges.append(edge)

    if not top_edges:
        return None

    try:
        sorted_edges = Part.__sortEdges__(top_edges)
        top_wire = Part.Wire(sorted_edges)

        cap_face = _make_flat_cap(top_wire, face_zmax)
        if cap_face:
            return (face_zmax, cap_face)
    except Exception as e:
        Path.Log.debug(f"Failed to cap complex 3D wall top rim: {e}")

    return None


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


def getTrimFace(border_face, bbFace, wpc):
    """
    Calculates the 'Outside World' mask used to clip the toolpath.

    This function takes a giant outer boundary (`border_face`) and subtracts the
    model's actual 2D silhouette (`bbFace`) from it. The result is a face with a
    hole in it, representing everything *outside* the area to be machined. This
    "trim face" is used in later boolean operations to ensure the toolpath does
    not extend beyond the model's perimeter.

    Args:
        border_face (Part.Face): The oversized outer boundary created by extendedBoundBox.
        bbFace (Part.Face): The 2D silhouette of the model or stock.
        wpc (Part.Wire): The workplane context for the Path.Area (ClipperLib) engine.

    Returns:
        Part.Shape: The final trim face shape, or an empty shape on failure.
    """
    trim_engine = Path.Area()
    trim_engine.setPlane(wpc)

    # We expand the canvas of the trim_face by 1.0mm so it completely engulfs
    # the border_face during the subtraction in _calculate_cut_area.
    bb = border_face.BoundBox
    p1 = FreeCAD.Vector(bb.XMin - 1.0, bb.YMin - 1.0, 0)
    p2 = FreeCAD.Vector(bb.XMax + 1.0, bb.YMin - 1.0, 0)
    p3 = FreeCAD.Vector(bb.XMax + 1.0, bb.YMax + 1.0, 0)
    p4 = FreeCAD.Vector(bb.XMin - 1.0, bb.YMax + 1.0, 0)
    expanded_canvas = Part.makePolygon([p1, p2, p3, p4, p1])

    trim_engine.add(expanded_canvas)

    if bbFace:
        # Use a copy to avoid mutating the original reference
        bbFace_copy = bbFace.copy()
        bbFace_copy.translate(FreeCAD.Vector(0, 0, -bbFace_copy.BoundBox.ZMin))
        trim_engine.add(bbFace_copy, op=1)

    try:
        trim_face = trim_engine.getShape()

        if hasattr(trim_face, "removeSplitter"):
            trim_face = trim_face.removeSplitter()
    except Exception as e:
        Path.Log.debug(
            f"surface_zlevel.getTrimFace: Removing splitter on trim face failed: {str(e)}"
        )
        return None

    return trim_face


# ---------------------------------------------------------------------------
# Depth categorization
# ---------------------------------------------------------------------------


def categorize_floor_steps(shape, start_z, final_z, step_down, clear_planar_only, tolerance=0.0001):
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

    floor_match_tol = 0.0005
    curr_z = start_z - step_down

    while curr_z > (final_z + tolerance):
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
        for floor_z in fused_geometry:
            if abs(floor_z - z_std) < floor_match_tol:
                match_z = floor_z
                break

        if match_z is not None:
            if clear_planar_only:
                # We are in "Clear Planar Only" mode. Split the "Mixed" step
                # into two separate virtual passes, "Pure"/"Extra", to avoid
                # clearing areas around floor_geo in the main loop process.
                # Set "Extra" higher to be processed first (required by fill selected holes).
                final_depth_logic.append((z_std + tolerance, "Extra", fused_geometry[match_z]))
                final_depth_logic.append((z_std, "Pure", None))
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
    """Identifies and fuses horizontal faces within the machining range.

    Iterates through all faces of the shape, filtering for planar surfaces
    It performs an accessibility check to ensure the floor is not occluded by geometry
    above it and fuses coincident faces at the same height into single regions.

    Args:
        shape: The Part.Shape to analyze.
        start_z: Upper vertical bound for floor detection (mm).
        final_z: Lower vertical bound for floor detection (mm).
        tolerance: Distance threshold for considering faces coplanar (mm).

    Returns:
        A dictionary: {z_height: fused_face_at_Z0}.
    """

    def fuse_faces(faces):
        fuse_engine = Path.Area()
        for i in range(len(faces)):
            fuse_engine.add(faces[i])
        try:
            result = fuse_engine.getShape()
        except:
            result = faces[0].multiFuse(faces[1:])
        return result

    def is_planar(face):
        # If you ever have issues with Planar surfaces (if face.BoundBox.ZLength < 1e-5:)
        if not (hasattr(face.Surface, "TypeId") and "Plane" in face.Surface.TypeId):
            return False

        u1, u2, v1, v2 = face.ParameterRange
        norm = face.normalAt((u1 + u2) / 2.0, (v1 + v2) / 2.0)

        return abs(norm.z > 0.99)

    def isAccessibleFromTop(face, shape, abs_top):
        # Accessibility Check: Solid Projection (Shadow Test)
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

    # Detect pre-triangulated models and skip floor detection
    from . import surface_common

    is_triangulated = surface_common._is_triangulated_mesh(shape.Faces)
    if is_triangulated:
        Path.Log.warning(
            "Pre-triangulated model detected. Automatic floor detection disabled for performance. 'Clear Planar Only' disabled."
        )
        return {}

    floor_accumulator = {}

    abs_top = shape.BoundBox.ZMax
    z_min, z_max = min(start_z, final_z), max(start_z, final_z)

    for face in shape.Faces:
        if is_planar(face):
            z = round(face.Vertexes[0].Z, 5)
            if (z >= z_min - tolerance) and (z < z_max):
                if isAccessibleFromTop(face, shape, abs_top):
                    f_copy = face.copy()
                    f_copy.translate(FreeCAD.Vector(0, 0, -f_copy.BoundBox.ZMin))

                    if z not in floor_accumulator:
                        floor_accumulator[z] = []
                    floor_accumulator[z].append(f_copy)

    fused = {}

    for z, faces in floor_accumulator.items():
        if len(faces) > 1:
            res = fuse_faces(faces)
        else:
            res = faces[0]
        if hasattr(res, "removeSplitter"):
            res = res.removeSplitter()
        fused[z] = res

    return fused


# ---------------------------------------------------------------------------
# Z-Level Hybrid layer generation
# ---------------------------------------------------------------------------


def zlevel_hybrid_stack(
    shape,
    categorized_steps,
    border_face,
    trim_face,
    fill_holes_masks,
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
        categorized_steps: List of tuples (z_target, status, floor_geo) from categorization.
        border_face: A Part.Face representing the stock or boundary footprint.
        trim_face: A Part.Face representing the 'Outside World' (forbidden zone).
        fill_holes_masks: A list of tuples, where each tuple is (max_z, mask_face_at_z0).
        tool_params: Dict containing 'radius', 'c_rad', 'profile', 'is_threeD'.
        stock_to_leave: Horizontal (XY) distance to keep from the model (mm).
        accuracy_val: Integer or string representing the number of sub-slices.
        z_offset: Vertical (Axial) distance to shift the final paths (mm).
        wpc: The Part.Circle workplane defining the 2D calculation plane.
        start_z: The Start Depth of the operation.

    Returns:
        A list of tuples: (z_target, cut_area_shape, status).
    """
    Path.Log.debug("surface_zlevel.zlevel_hybrid_stack: Starting geometric stack generation.")

    # 1. Initialization
    stack = []

    all_prev_comp = None
    tol = 0.0001
    loose_tol = 0.0002
    fill_mask_idx = 0  # Fill holes masks list pointer

    is_3d = tool_params["is_threeD"]
    num_slices = int(accuracy_val) if is_3d else 1

    # 2. Pre-load C++ engine
    area_engine = Path.Area()
    area_engine.setPlane(wpc)
    area_engine.add(shape)
    # Configure C++ engine parameters
    params = area_engine.getDefaultParams()
    params["SectionTolerance"] = 0.0001

    # 3. Identify critical snapping depths (Top and floors)
    model_bottom, model_top = shape.BoundBox.ZMin, shape.BoundBox.ZMax
    critical_heights = {
        round(h, 6) for h, status, _ in categorized_steps if status in ["Mixed", "Extra"]
    }

    critical_heights.add(round(model_top, 6))
    if abs(start_z - model_top) > tol:
        critical_heights.add(start_z - tol)

    # 4. Main layer loop
    for z_target, status, floor_geo in categorized_steps:

        # Reject steps strictly above the model top
        if z_target > (model_top - tol):
            Path.Log.warning(
                f"Skipping step at Z={z_target:.3f}mm as it is above the model top (max Z: {model_top:.3f}mm)."
            )
            continue

        # Determine the Slice Height (Model Footprint)
        z_slice = max(z_target, model_bottom)

        # The depth at which the tool has submerged from the model_top
        dist_submerged = max(0, model_top - z_slice)
        # Nudge slice height based on whether we are clearing a floor or a wall
        # Standard layers nudge up (+); Floors nudge down (-) to stay inside material
        slice_bias = loose_tol if status in ["Mixed", "Extra"] else -loose_tol

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

        if not layer_slices or not len(layer_slices) > 0:
            continue

        # C. Fuse all generated slices into a single silhouette for this layer
        fusion = Path.Area()
        fusion.setPlane(wpc)
        for s in layer_slices:
            if not s.isNull():
                fusion.add(s)

        # D. Boolean resolution
        current_silhouette = None
        try:
            # current_silhouette is the union of all 3D contact points at this depth
            current_silhouette = fusion.getShape()
        except Exception as e:
            Path.Log.error(f"Silhouette fusion failed at Z={round(z_target, 3)}. Error: {str(e)}")
            continue

        # E. Process and apply active fill hole masks
        if fill_holes_masks:
            fill_mask_idx, fill_holes_masks, floor_geo, all_prev_comp = _apply_fill_hole_masks(
                wpc,
                fill_holes_masks,
                fill_mask_idx,
                current_silhouette,
                status,
                floor_geo,
                all_prev_comp,
                z_target,
                loose_tol,
            )

        # F: Calculate the final cutting area using the new helper
        cut_area = _calculate_cut_area(
            wpc,
            status,
            current_silhouette,
            floor_geo,
            border_face,
            trim_face,
            all_prev_comp,
            z_target,
        )

        # G: Finalize and store the result for this layer
        if cut_area:
            total_shift = z_target + z_offset

            final_cut = cut_area.copy()
            final_cut.translate(FreeCAD.Vector(0, 0, total_shift))

            # Store target G-code depth, calculated geometry, and metadata
            stack.append((total_shift, final_cut, status))

            # Update Persistent Mask (strictly model silhouette to keep pockets open)
            all_prev_comp = _update_machining_mask(
                wpc, all_prev_comp, current_silhouette, status, floor_geo
            )

    return stack


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
    sections = []
    tol = 1e-5

    for h, r_theo in unique_steps:
        r_comp = r_theo + stock_to_leave

        # Synchronized Slicing: Calculate the precise Z for this sample
        slice_z = max(model_bottom + tol, min(z_target + h + slice_bias, model_top - tol))

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
        sections = []

    return slices


def _calculate_cut_area(
    wpc,
    status,
    current_silhouette,
    floor_geo,
    border_face,
    trim_face,
    all_prev_comp,
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
        current_silhouette (Part.Shape): The tool-compensated model silhouette at Z=0.
        floor_geo (Part.Shape): The detected physical floor geometry at Z=0.
        border_face (Part.Shape): The main stock boundary at Z=0.
        trim_face (Part.Shape): The "outside world" mask to clip the toolpath.
        all_prev_comp (Part.Shape): The cumulative mask of areas already machined in upper layers.
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
            layer_engine.add(current_silhouette, op=1)  # Subtract model

    else:
        # Standard Mode: The area to machine is the stock boundary, minus the model,
        # minus any areas we've already cleared, and clipped by the trim mask.
        # Stock = Border; Material = (Stock - Model - PreviouslyCleared) - TrimMask
        layer_engine.add(border_face)
        layer_engine.add(current_silhouette, op=1)

        # Rest Machining: subtract material cleared in layers above
        if all_prev_comp:
            layer_engine.add(all_prev_comp, op=1)

    # Apply the 'outside world' mask
    if trim_face:
        layer_engine.add(trim_face, op=1)

    try:
        cut_area = layer_engine.getShape()
    except Exception as e:
        Path.Log.error(f"Layer engine failed at Z={round(z_target, 3)}. Error: {str(e)}")
        cut_area = None

    return cut_area


def _update_machining_mask(wpc, all_prev_comp, current_silhouette, status, floor_geo):
    """Updates the persistent cumulative mask with new cleared areas.

    This function maintains a 'shadow' of all material processed in layers
    above the current depth. It performs a C++ union of the previous mask,
    the current model footprint, and any detected physical floors. This
    prevents the tool from air-cutting previously finished surfaces.

    Args:
        wpc (Part.Circle): The workplane for Path.Area operations.
        all_prev_comp (Part.Shape): The cumulative mask from previous layers.
        current_silhouette (Part.Shape): The tool-compensated footprint of
            the current model slice.
        status (str): The layer status ("Pure", "Mixed", or "Extra").
        floor_geo (Part.Face): The physical floor geometry detected at this depth.

    Returns:
        Part.Shape: The updated, dissolved cumulative mask at Z=0.
    """

    mask_engine = Path.Area()
    mask_engine.setPlane(wpc)

    # 1. Add the mask from all layers above
    if all_prev_comp and not all_prev_comp.isNull():
        mask_engine.add(all_prev_comp)

    # 2. Add the current model silhouette (the walls/islands)
    mask_engine.add(current_silhouette)

    # 3. Add physical floors (Mixed or Extra)
    # This treats 'shelves' as solid barriers for all layers below
    if status in ["Mixed", "Extra"] and floor_geo:
        # floor_geo was normalized to Z=0 during categorization
        mask_engine.add(floor_geo)

    # 4. Extract the dissolved result
    try:
        all_prev_comp = mask_engine.getShape()
    except Exception as e:
        Path.Log.error(f"Machining mask update failed: {str(e)}")

    return all_prev_comp


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
    start_point,
    radius,
    is_adaptive,
    adaptive_params,
    bb_face,
    enforce_geofence,
):
    """Converts the geometry stack into G-code Path Commands.

    This function iterates through a pre-calculated stack of geometric slices,
    generating perimeter (Waterline) paths and optional floor-clearing patterns.
    It manages tool engagement directions, safety transitions to safe heights,
    and progress reporting.

    Args:
        stack: A list of tuples (z_target, cut_area, status) representing layers.
        feed_params: Dict containing 'horizFeed', 'vertFeed', 'horizRapid', 'vertRapid'.
        height_params: Dict containing 'safe_hght' and 'clearance_hght'.
        pattern_options: Dict containing 'cut_climb', 'cut_pattern', 'pattern_angle',
            'reverse_pattern', 'keep_tool_down', 'keep_down_ratio'.
        ignore_outer: Boolean. If True, skips the outermost boundary (stock edge).
        clear_planar_only: Boolean. If True, only clears floors detected as
            Mixed or Extra.
        step_over: The horizontal step-over distance for clearing patterns (mm).
        start_point: A user-defined Start Point or 'None'.
        radius: The tool radius (mm).
        is_adaptive: True if the CutPattern is Adaptive.
        adaptive_params: Dict containing 'op_type', adaptive_accuracy', 'stock_to_leave',
            'force_insideout', 'finishing_profile', 'lift_distance', 'keep_tool_down',
            'helix_angle', 'helix_diameter', 'helix_min_diameter'
        bb_face: A Part.Face representing the stock or boundary footprint.

    Returns:
        A list of Path.Command objects (G-code).
    """
    Path.Log.debug("surface_zlevel.zlevel_hybrid_to_gcode: Starting G-code generation.")

    # 1. Initialization
    commands = []

    tool_diam = radius * 2
    vert_rapid = feed_params.get("horizRapid", 0.0)
    min_path_length = tool_diam
    min_adaptive_area = math.pi * (radius**2)

    # Extract heights
    safe_hght = height_params.get("safe_hght", 3.0)
    clear_hght = height_params.get("clearance_hght", 5.0)
    prev_z = height_params.get("start_hght", safe_hght) + 0.1  # Plus 0.1 for safety

    # Extract pattern logic
    cut_climb = pattern_options.get("cut_climb", True)
    pattern_name = pattern_options.get("cut_pattern", "None")
    pattern_angle = pattern_options.get("pattern_angle", 0.0)
    reverse_pattern = pattern_options.get("reverse_pattern", False)
    keep_tool_down = pattern_options.get("keep_tool_down", True)
    keep_down_ratio = pattern_options.get("keep_down_ratio", 2.0) * tool_diam

    # 2. Main Layer Processing
    for z_target, cut_area, status in stack:

        if not cut_area or cut_area.isNull() or not cut_area.Wires:
            continue

        # Determine start index (0 = machine stock edge, 1 = ignore stock edge)
        start_w_idx = 1 if ignore_outer and not status in ["Extra"] else 0

        # A: Adaptive Cut Pattern
        if is_adaptive:
            from . import adaptive_common as _adaptive

            # This single call checks the topology, sets up the offsets,
            # handles the finishing_profile override, and prints a warning!
            geofence, bb_offset = _setup_adaptive_geofence(
                cut_area, bb_face, adaptive_params, radius, z_target, enforce_geofence
            )

            pattern_cmds = _adaptive.generate(
                adaptive_params,
                feed_params,
                radius,
                step_over,
                z_target,
                safe_hght,
                prev_z,
                cut_area,
                min_adaptive_area,
                bb_face,
                enforce_geofence=geofence,
                cut_area_offset=radius,
                bb_face_offset=bb_offset,
            )

            commands.extend(pattern_cmds)
            if status not in ["Extra"]:
                prev_z = z_target + 0.1  # Plus 0.1 for safety
            continue

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
            commands.append(Path.Command("G0", {"Z": safe_hght, "F": vert_rapid}))

            # Dispatch to the high-speed Path.Area pattern engine
            pattern_cmds = _generatePattern(
                cut_area,
                pattern_name,
                pattern_angle,
                cut_climb,
                reverse_pattern,
                z_target,
                step_over,
                keep_tool_down,
                keep_down_ratio,
                start_point,
                radius,
                feed_params,
                safe_hght,
                min_path_length,
            )

            commands.extend(pattern_cmds)

        # C: Perimeters (Waterline Walls)
        if start_w_idx < len(cut_area.Wires):
            # Dynamic magnet point for the perimeter traveling salesperson
            current_peri_start = start_point
            for w_idx in range(start_w_idx, len(cut_area.Wires)):
                wire = cut_area.Wires[w_idx]
                if not wire.isClosed() or wire.Length < min_path_length:
                    continue

                # Geometry cleanup
                if hasattr(wire, "removeSplitter"):
                    wire = wire.removeSplitter()
                wire.fix(1e-6, 1e-6, 1e-4)

                # Rotate the perimeter wire to align with our start point
                if current_peri_start:
                    wire = _reorient_wire_start(wire, current_peri_start)
                    # Update magnet for the next perimeter loop
                    if wire.Vertexes:
                        current_peri_start = wire.Vertexes[0].Point

                # Grab the exact first vertex of the rotated wire
                start_p = wire.Vertexes[0].Point if wire.Vertexes else current_peri_start

                # Generate the wire-following path
                commands.extend(
                    _generate_wire_path(
                        wire,
                        z_target,
                        safe_hght,
                        start_p,
                        feed_params,
                        keep_tool_down,
                        keep_down_ratio,
                        reverse_pattern,
                        cut_climb,
                        sort_mode=0,
                    )
                )

    if not commands:
        Path.Log.warning(
            "No toolpath generated. The tool may not fit within the defined machining area. "
            "Try increasing the 'Boundary adjustment' or checking your tool diameter."
        )

    # Return to clearance height
    commands.append(Path.Command("G0", {"Z": clear_hght, "F": vert_rapid}))

    return commands


def _setup_adaptive_geofence(
    cut_area, bb_face, adaptive_params, radius, z_target, enforce_geofence
):
    """
    Analyzes the geometric relationship between the cut area and the stock boundary
    to detect open pockets, and configures safety overrides for the Adaptive2d algorithm.

    The libarea Adaptive2d algorithm is optimized for closed pockets and can produce
    erratic toolpaths when encountering open boundaries. This function detects those
    breaches using a two-pass check (AABB followed by topological intersection) and
    applies geofencing and parameter overrides to ensure safe machining.

    Args:
        cut_area (Part.Shape): The 2D boundary of the area to be machined on this layer.
        bb_face (Part.Shape): The 2D stock boundary (geofence limit).
        adaptive_params (dict): The dictionary of adaptive routing parameters.
                                Modified in-place if overrides are required.
        radius (float): The tool radius in millimeters.
        z_target (float): The current Z-depth (used for contextual logging).
        enforce_geofence (bool): The user's preference from the operation's Data tab.
                                      If False, respects the power-user's choice to disable
                                      geofence clipping on open pockets. Defaults to True.

    Returns:
        tuple: (geofence_active (bool), bb_offset (float))
               - geofence_active: True if transit moves should be strictly clipped.
               - bb_offset: The boundary offset applied for the Adaptive2d algorithm.
    """
    # Defaults for closed pockets
    force_insideout = adaptive_params.get("force_insideout", False)
    geofence = False
    bb_offset = radius - 0.01

    if not cut_area or cut_area.isNull() or not bb_face or bb_face.isNull():
        return geofence, bb_offset

    # Open Pocket Geometric Detection
    c_bb = cut_area.BoundBox
    s_bb = bb_face.BoundBox
    tol = 0.01
    is_open = False

    # Fast AABB Check
    if (
        c_bb.XMin <= s_bb.XMin + tol
        or c_bb.XMax >= s_bb.XMax - tol
        or c_bb.YMin <= s_bb.YMin + tol
        or c_bb.YMax >= s_bb.YMax - tol
    ):
        is_open = True
    else:
        # Irregular Stock Check
        try:
            intersection = cut_area.common(bb_face)
            if intersection and not intersection.isNull():
                if abs(cut_area.Area - intersection.Area) > 0.01:
                    is_open = True
                elif abs(intersection.Length - cut_area.Length) > 0.01:
                    is_open = True
        except Exception:
            is_open = True

    # Apply Safety Overrides
    if is_open and not force_insideout:
        # Respect the power-user toggle
        geofence = bool(enforce_geofence)
        bb_offset = -0.01
        adaptive_params["finishing_profile"] = False

        status_text = "ENABLED" if geofence else "DISABLED (by user override)"

        Path.Log.warning(
            f"Z={round(z_target, 3)}: Outside adaptive cut detected.\n"
            f"Geofence clipping is {status_text}.\n"
            "The Adaptive2d algorithm can be unpredictable in open regions. For safest results:\n"
            " - Inspect the toolpath closely for any anomalies.\n"
            " - Set your Boundary Box property to 'Stock' instead of 'BoundingBox'.\n"
            " - Adjust your 'Boundary Extension' manually if the tool overextends.\n"
            "(Note: 'Finishing Profile' was automatically disabled for this layer to prevent edge artifacts.)"
        )

    return geofence, bb_offset


def _find_start_point(wire, start_point, cut_climb):
    """
    Calculates the starting vertex based on a user-provided start point..

    Args:
        wire (Part.Wire): The input wire to process.
        start_point (FreeCAD.Vector or None): A user-defined preference for the
            start location, or None if no custom start point is requested.
        cut_climb (bool): The cut direction. (Currently bypassed to allow the
            C++ engine to natively calculate natural entry points).

    Returns:
        tuple: A tuple containing (
            FreeCAD.Vector: The calculated starting point or 'None'.
        )
    """
    # Determine start point
    V = wire.Vertexes
    lv = len(V) - 1

    if start_point:
        # Find the vertex closest to the user-defined start point
        start_p = min(
            [FreeCAD.Vector(v.X, v.Y, v.Z) for v in V],
            key=lambda v: math.hypot(v.x - start_point.x, v.y - start_point.y),
        )
    else:
        # Default — climb starts at last vertex (end of CCW wire for inside profile)
        start_p = (
            FreeCAD.Vector(V[lv].X, V[lv].Y, V[lv].Z)
            if cut_climb
            else FreeCAD.Vector(V[0].X, V[0].Y, V[0].Z)
        )

    return start_p


def _reorient_wire_start(wire, start_point):
    """
    Rebuilds a closed wire so that its first edge begins at the vertex
    closest to the provided start_point.
    """
    if not wire.isClosed():
        return wire

    edges = wire.Edges
    if not edges:
        return wire

    closest_idx = 0
    min_dist = float("inf")

    for i, edge in enumerate(edges):
        if not edge.Vertexes:
            continue

        v_start = edge.Vertexes[0].Point
        dist = v_start.distanceToPoint(start_point)

        if dist < min_dist:
            min_dist = dist
            closest_idx = i

    # If it is already oriented correctly, skip rebuilding
    if closest_idx == 0:
        return wire

    # Cycle the edge array
    reordered_edges = edges[closest_idx:] + edges[:closest_idx]

    try:
        new_wire = Part.Wire(reordered_edges)
        return new_wire
    except Exception as e:
        Path.Log.debug(f"Failed to reorient wire: {e}")
        return wire


def _generate_wire_path(
    wire,
    z_target,
    safe_hght,
    start_p,
    feed_params,
    keep_tool_down=False,
    keep_down_ratio=0.0,
    reverse_pattern=False,
    cut_climb=False,
    sort_mode=0,
):
    """Standardizes G-code generation for a single wire segment.

    Args:
            wire (Part.Wire or list): The geometric path or list of paths to follow.
            z_target (float): The target machining depth.
            safe_hght (float): The height for safe rapid transitions.
            start_p (FreeCAD.Vector or None): The calculated starting point, or None.
            feed_params (dict): Dictionary containing 'horizFeed' and 'vertFeed' values.
            keep_tool_down (bool): If True, minimizes Z-retractions between disconnected paths.
            keep_down_ratio (float): The threshold distance for keeping the tool down.
            reverse_pattern (bool): If True, reverses the toolpath orientation (e.g., Inside-Out).
            cut_climb (bool): If True, uses Climb milling; otherwise, Conventional.
            sort_mode (int): The native FreeCAD sorting mode (0 = None, 1 = Inside-Out,
                2 = Outside-In, 3 = Nearest Neighbor).

        Returns:
            list: A list of Path.Command objects.
    """
    commands = []
    # if reverse_pattern and cut_climb:
    orientation = cut_climb if cut_climb and reverse_pattern else not cut_climb

    # Extract feeds and speeds
    h_feed = feed_params.get("horizFeed", 0.0)
    v_feed = feed_params.get("vertFeed", 0.0)

    path_params = {
        "sort_mode": int(sort_mode),
        "shapes": wire,
        "orientation": bool(orientation),
        "feedrate": float(h_feed),
        "feedrate_v": float(v_feed),
        "preamble": False,
        "retraction": float(safe_hght),
        "resume_height": float(safe_hght),
        "arc_plane": 2,  # 2 = XY Plane (Silences the "Sort mode 'None'" warning)
    }

    # Apply the Zero-Bug Fix right at the gate (taken from Area.py)
    if start_p:
        safe_x = start_p.x
        safe_y = start_p.y
        if Path.Geom.isRoughly(safe_x, 0.0):
            safe_x = 0.00001
        if Path.Geom.isRoughly(safe_y, 0.0):
            safe_y = 0.00001

        path_params["start"] = FreeCAD.Vector(safe_x, safe_y, start_p.z)

    if keep_tool_down and keep_down_ratio > 0:
        path_params["threshold"] = keep_down_ratio

    try:
        pp = Path.fromShapes(**path_params)
    except Exception as e:
        Path.Log.error(f"Path.fromShapes failed at Z={z_target}: {str(e)}")
        return []

    # Extend Commands list
    commands.extend(pp.Commands)
    # Return commands
    return commands


def _generatePattern(
    cut_area,
    cut_pattern,
    pattern_angle,
    cut_climb,
    reverse_pattern,
    z_target,
    step_over,
    keep_tool_down,
    keep_down_ratio,
    start_point,
    radius,
    feed_params,
    safe_hght,
    min_path_length,
):
    """
    Orchestrates the generation of high-speed 2D infill patterns for a given layer.

    This function utilizes the Clipper-based C++ Path.Area engine to calculate
    clearing geometry (ZigZag, Offset, Line, Grid) within the provided boundaries.
    It applies tool radius compensation, stepover, and angle adjustments.

    Advanced Routing:
    - For 'Offset' patterns, it acts as a Traveling Salesperson optimizer. It
      evaluates the resulting closed loops and dynamically rotates their geometric
      seams (vertices) to align with the tool's current position, eliminating
      unnecessary G0 rapid moves between passes.
    - Start coordinates are passed down raw and are sanitized for engine-specific
      zero-coordinate bugs within the downstream `_generate_wire_path` function.

    Args:
        cut_area (Part.Shape): The boundary face or shape to clear.
        cut_pattern (str): The infill strategy ("ZigZag", "Offset", "Line", "Grid").
        pattern_angle (float): The yaw angle (degrees) for scanline patterns.
        cut_climb (bool): If True, uses Climb milling; otherwise Conventional.
        reverse_pattern (bool): If True, reverses sequence (e.g., Inside-Out)
            or applies a 90-degree orthogonal shift to linear patterns.
        z_target (float): The absolute Z-coordinate for this machining pass.
        step_over (float): The horizontal distance between consecutive passes (mm).
        keep_tool_down (bool): If True, minimizes Z-retractions between cuts.
        keep_down_ratio (float): The threshold distance ratio for keeping the tool down.
        start_point (FreeCAD.Vector or None): A user-defined origin preference.
        radius (float): The tool radius for offset calculation (mm).
        feed_params (dict): Feed rates ('horizFeed', 'vertFeed').
        safe_hght (float): Z-height for safe rapid XY transitions.
        min_path_length (float): The minimum length to keep a generated wire (mm).

    Returns:
        list: A flat list of Path.Command objects representing the pattern G-code.
    """
    Path.Log.debug(
        f"surface_zlevel._generatePattern: Generating {cut_pattern} pattern at Z={z_target}"
    )

    commands = []
    sort_mode = 3

    # 1. Validation Guards
    if not cut_area or cut_area.isNull():
        Path.Log.warning("Pattern generation skipped - Empty cutting area.")
        return []

    # Map UI Strategy to C++ PocketMode
    if cut_pattern == "ZigZag":
        pattern_mode = 1
    elif cut_pattern == "Offset":
        pattern_mode = 2
        sort_mode = 0 if reverse_pattern else 3
    elif cut_pattern == "Line":
        pattern_mode = 5
    elif cut_pattern == "Grid":
        pattern_mode = 6
    else:
        Path.Log.error(f"Unsupported pattern type '{cut_pattern}'")
        return []

    adjusted_angle = float(pattern_angle)
    if reverse_pattern and cut_pattern in ["ZigZag", "Line"]:
        adjusted_angle -= 90.0

    extra_offset = radius - step_over

    for face in cut_area.Faces:
        if face.Area < 1e-7:
            continue

        # Spin up a fresh engine for just this island
        engine = Path.Area()
        engine.add(face)

        params = engine.getParams()
        params["PocketMode"] = pattern_mode
        params["PocketStepover"] = step_over
        params["PocketExtraOffset"] = -extra_offset
        params["Angle"] = adjusted_angle
        params["ToolRadius"] = radius
        params["FromCenter"] = reverse_pattern

        engine.setParams(**params)

        try:
            engine.makePocket()
            res_area = engine.getShape()
        except Exception as e:
            Path.Log.error(f"Pattern G-code generation failed for island at Z={z_target}: {str(e)}")
            continue

        if not res_area or res_area.isNull():
            continue

        # 2. Filter and Reorient Wires (The Traveling Salesperson Optimizer)
        filtered_wires = []
        current_start_pt = start_point

        if pattern_mode == 2:  # Offset pattern usually outputs closed loops
            for wire in res_area.Wires:
                if not wire.isClosed() or wire.Length < min_path_length:
                    continue

                # Apply reorientation if a start point exists and the wire is significant
                if current_start_pt and wire.BoundBox.DiagonalLength > 2.0:
                    rotated_wire = _reorient_wire_start(wire, current_start_pt)
                    filtered_wires.append(rotated_wire)

                    # Update magnet point to the end of this wire for the next loop
                    if rotated_wire.Vertexes:
                        current_start_pt = rotated_wire.Vertexes[0].Point
                else:
                    filtered_wires.append(wire)
        else:
            # ZigZag, Line, and Grid patterns don't need reorientation as they are open
            filtered_wires = res_area.Wires

        # 3. Generate G-code for this specific island
        if pattern_mode == 5:  # Line pattern
            for wire in filtered_wires:
                # Find start point using the raw start_point (it will be sanitized downstream)
                start_p = _find_start_point(wire, start_point, False)
                commands.extend(
                    _generate_wire_path(
                        wire,
                        z_target,
                        safe_hght,
                        start_p,
                        feed_params,
                        keep_tool_down,
                        keep_down_ratio,
                        reverse_pattern,
                        cut_climb,
                        sort_mode=0,
                    )
                )
        else:  # ZigZag, Offset, Grid patterns
            # Pass the raw current_start_pt down (it will be sanitized downstream)
            commands.extend(
                _generate_wire_path(
                    filtered_wires,
                    z_target,
                    safe_hght,
                    current_start_pt,
                    feed_params,
                    keep_tool_down,
                    keep_down_ratio,
                    reverse_pattern,
                    cut_climb,
                    sort_mode,
                )
            )

    return commands
