# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2025 Dimitrios Pana <dimitriospana75@gmail.com>
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

"""Shared utilities for 3D surface and waterline generators.

Provides OCL cutter creation, STL mesh conversion, and travel optimization.
These are pure functions with no FreeCAD document access — tool parameters
and geometry are passed in by the operation wrapper.
"""

import Path
import Part
import FreeCAD

__title__ = "Surface Common Utilities"
__author__ = "sliptonic (Brad Collette)"
__url__ = "https://www.freecad.org"

if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


# ---------------------------------------------------------------------------
# OCL import helper
# ---------------------------------------------------------------------------


_ocl = None
_meshpart = None


def _get_ocl():
    """Lazily import OCL, trying both package names."""
    global _ocl
    if _ocl is not None:
        return _ocl
    try:
        import ocl

        _ocl = ocl
    except ImportError:
        try:
            import opencamlib as ocl

            _ocl = ocl
        except ImportError:
            raise ImportError(
                "OpenCamLib (ocl) is required for 3D surface operations. "
                "Install it via your package manager or from "
                "https://github.com/aewallin/opencamlib"
            )
    return _ocl


def _get_meshpart():
    """Lazily import MeshPart."""
    global _meshpart
    if _meshpart is not None:
        return _meshpart
    try:
        import MeshPart as meshpart

        _meshpart = meshpart
    except ImportError:
        raise ImportError("MeshPart is required for shape tessellation")
    return _meshpart


# ---------------------------------------------------------------------------
# OCL Cutter creation
# ---------------------------------------------------------------------------


# Map of FreeCAD ToolBit shape types to OCL cutter factory names
_TOOL_TYPE_MAP = {
    "endmill": "CylCutter",
    "ballend": "BallCutter",
    "bullnose": "BullCutter",
    "taperedballnose": "BallCutter",
    "drill": "ConeCutter",
    "engraver": "ConeCutter",
    "v_bit": "ConeCutter",
    "v-bit": "ConeCutter",
    "vbit": "ConeCutter",
}


def make_ocl_cutter(
    tool_type,
    diameter,
    corner_radius=0.0,
    flat_radius=0.0,
    edge_height=0.0,
    edge_angle=0.0,
    length_offset=0.0,
):
    """Create an OCL cutter from tool parameters.

    Pure function — no FreeCAD document access.  Tool parameters are
    extracted by the operation wrapper before calling this.

    Args:
        tool_type: ToolBit shape type string (e.g. 'endmill', 'ballend',
                   'bullnose', 'drill', 'v-bit', etc.)
        diameter: Tool diameter in mm.
        corner_radius: Corner radius for bull-nose cutters.
        flat_radius: Flat radius at tip (derived from diameter and
                     corner_radius for bull-nose).
        edge_height: Cutting edge height in mm.
        edge_angle: Cutting edge full angle in degrees (for V-bits / drills).
        length_offset: Length offset in mm.

    Returns:
        An ``ocl`` cutter object, or *None* if the tool type is not
        supported.
    """
    ocl = _get_ocl()
    tool_type_lower = tool_type.lower()
    cutter_name = _TOOL_TYPE_MAP.get(tool_type_lower)

    if cutter_name is None:
        Path.Log.error("Unsupported tool type '{}' for OCL cutter creation.".format(tool_type))
        return None

    if diameter <= 0:
        Path.Log.error("Tool diameter must be positive, got {}".format(diameter))
        return None

    if cutter_name == "CylCutter":
        if edge_height <= 0:
            Path.Log.warning(
                f"The cutting edge height for this tool is set to {edge_height}. Using the tool's diameter as a fallback height."
            )
            edge_height = diameter
        return ocl.CylCutter(diameter, edge_height + length_offset)

    elif cutter_name == "BallCutter":
        if edge_height <= 0:
            edge_height = diameter / 2.0
        return ocl.BallCutter(diameter, edge_height + length_offset)

    elif cutter_name == "BullCutter":
        if edge_height <= 0:
            Path.Log.warning(
                f"The cutting edge height for this Bull-nose tool is set to {edge_height}. Using the tool's diameter as a fallback height."
            )
            edge_height = diameter
        # OCL BullCutter(diameter, minor_radius, length)
        # minor_radius = diameter/2 - flat_radius
        minor_radius = diameter / 2.0 - flat_radius
        if minor_radius < 0:
            minor_radius = 0.0
        return ocl.BullCutter(diameter, minor_radius, edge_height + length_offset)

    elif cutter_name == "ConeCutter":
        if edge_angle <= 0:
            Path.Log.error("ConeCutter requires a positive edge_angle, got {}".format(edge_angle))
            return None
        # OCL ConeCutter(diameter, half_angle, length)
        return ocl.ConeCutter(diameter, edge_angle / 2.0, length_offset)

    return None


def make_safe_cutter(
    tool_type,
    diameter,
    corner_radius=0.0,
    flat_radius=0.0,
    edge_height=0.0,
    edge_angle=0.0,
    length_offset=0.0,
    buffer_pct=0.25,
):
    """Create an oversized OCL cutter for safe-travel-height checks.

    Same interface as :func:`make_ocl_cutter` but inflates the diameter
    by *buffer_pct* (default 25 %).
    """
    safe_diam = diameter * (1.0 + buffer_pct)
    safe_flat = flat_radius * (1.0 + buffer_pct) if flat_radius > 0 else safe_diam * buffer_pct
    return make_ocl_cutter(
        tool_type,
        safe_diam,
        corner_radius=corner_radius,
        flat_radius=safe_flat,
        edge_height=edge_height,
        edge_angle=edge_angle,
        length_offset=length_offset,
    )


# ---------------------------------------------------------------------------
# Boundary creation utilities
# ---------------------------------------------------------------------------


def create_boundary_face(
    model_faces, offset=0.0, tolerance=0.005, avoids=False, model_boundary=False
):
    """
    Creates a flat 2D boundary face from a list of 3D faces using
    Path.Area's built-in HLR projection (Outline mode) as primary method,
    falling back to TechDraw.findShapeOutline() if projection fails.

    Path.Area with Outline=True uses OCC's HLRBRep_Algo to project the
    3D shape silhouette onto the XY plane — more robust than TechDraw
    for complex curved and spiral faces where findShapeOutline() struggles.

    Args:
        model_faces (list): List of Part.Face objects to build boundary from.
        offset (float): Offset to apply to the resulting boundary.
        tolerance (float): Tolerance for wire joining.
        avoids (bool): 'True' only from _preprocess_avoid_faces.

    Returns:
        Part.Shape: The 2D boundary face, or None on failure.
    """
    if not model_faces:
        Path.Log.warning(
            "No faces provided. Check that the Base Geometry selection contains valid faces."
        )
        return None

    outline = not avoids
    is_triangulated = _is_triangulated_mesh(model_faces)

    if not is_triangulated:
        if model_boundary:
            model_faces = _filter_vertical(model_faces)
        result = _boundary_via_area(model_faces, offset, outline)
        if result is not None:
            return result

    return _boundary_via_techdraw(model_faces, offset, outline)


def _boundary_via_area(model_faces, offset, outline):
    """
    Primary boundary engine: Path.Area projection/offset.

    Returns:
        Part.Shape: The resulting boundary, or None if Path.Area
        produced an empty/null shape or raised an exception.
    """
    try:
        compound = model_faces[0] if len(model_faces) == 1 else Part.makeCompound(model_faces)

        wpc = Part.makeCircle(2)
        area = Path.Area()
        area.setPlane(wpc)
        area.add(compound)
        area.setParams(
            Outline=outline,
            Offset=offset,
            Coplanar=0,  # CoplanarNone — don't restrict to coplanar
            Fill=2,  # FillFace
        )
        result = area.getShape()

        if not result or result.isNull():
            Path.Log.warning(
                "Offsetting the Model faces resulted in an empty shape. "
                "Extend the boundary if the selected faces are too small."
            )
            return None
        return result

    except Exception as e:
        Path.Log.warning(
            f"Path.Area projection failed: {e} — falling back to TechDraw outline extraction."
        )
        return None


def _boundary_via_techdraw(model_faces, offset, outline):
    """
    Secondary (fallback) boundary engine: TechDraw.findShapeOutline(),
    followed by a second Path.Area pass purely to apply `offset`.

    Note: findShapeOutline() only ever returns the outer envelope — unlike
    _boundary_via_area(), it cannot preserve inner wires (holes) when
    `outline` is False. If a caller needed holes preserved and lands
    here, that guarantee is lost, and a warning is logged.

    Returns:
        Part.Shape: The resulting boundary, or None on failure.
    """
    if not outline:
        Path.Log.warning(
            "Falling back to TechDraw outline extraction, which cannot preserve "
            "inner wires (holes). Any holes in this selection will be lost."
        )
    try:
        compound = model_faces[0] if len(model_faces) == 1 else Part.makeCompound(model_faces)

        import TechDraw

        direction = FreeCAD.Vector(0, 0, 1)
        outline_shape = TechDraw.findShapeOutline(compound, 1.0, direction)

        if not outline_shape or outline_shape.isNull() or not outline_shape.Wires:
            Path.Log.warning(
                "Offsetting the Model faces resulted in an empty shape. "
                "Extend the boundary if the selected faces are too small."
            )
            return None

        boundary = Part.makeFace(outline_shape.Wires, "Part::FaceMakerBullseye")
        if not boundary or boundary.isNull():
            Path.Log.error(
                "Failed to calculate the boundary offset. "
                "Try adjusting the Boundary Adjustment value or checking the selected faces for geometric errors."
            )
            return None

        boundary.translate(FreeCAD.Vector(0, 0, -boundary.BoundBox.ZMin))

        offset_engine = Path.Area()
        offset_engine.add(boundary)
        offset_engine.setParams(Offset=offset)
        return offset_engine.getShape()

    except Exception as e:
        Path.Log.error(f"TechDraw fallback failed offsetting the Model faces: {e}")
        return None


def generate_pattern_mask(
    is_whole_model_job, bb_face, cutting_faces, avoid_boundary, tool_radius, boundary_adj, tolerance
):
    """
    Generates a universal 2D boundary face, punching out
    holes for any user-defined avoid_faces.

    The process follows three main steps:
    1.  It generates the main outer boundary from the 'cutting_faces', shrinking it
        inwards by the tool radius to ensure the tool stays contained.
    2.  It generates "keep-out" zones from the 'avoid_faces', expanding them outwards
        by the tool radius to create a safety buffer.
    3.  It performs a boolean cut, subtracting the keep-out zones from the main
        boundary to create the final, correctly-holed mask.

    Args:
        cutting_faces (list): A list of Part.Face objects to derive the main boundary from.
        avoid_boundary (Part.Shape, optional): Pre-built Avoid Faces "keep-out" boundary.
        tool_radius (float): The radius of the active cutter.
        boundary_adj (float): An explicit user-provided offset override.
        tolerance (float): The deflection tolerance for discretizing curves smoothly.

    Returns:
        Part.Face: The final 2D clipping boundary. Returns None on failure.
    """
    if not cutting_faces:
        Path.Log.warning("Could not determine geometry for main boundary mask.")
        return None

    # Create the Main Outer Boundary
    main_boundary = None
    outer_offset = -tool_radius + boundary_adj

    # Add a small buffer to avoid "path spikes" on vertical walls
    epsilon = max(0.01, tolerance + 0.001)

    if is_whole_model_job:
        # Use TechDraw.findShapeOutline for whole model silhouette
        main_boundary = bb_face
    else:
        main_boundary = build_optimized_boundary([cutting_faces], outer_offset - epsilon, tolerance)

    if not main_boundary:
        Path.Log.warning("Could not determine geometry for main boundary mask.")
        return None

    # Punch the holes for the pre-built Avoid Faces keep-out zone, if any
    if not avoid_boundary:
        return main_boundary

    try:
        final_mask = main_boundary.cut(avoid_boundary)
        if final_mask.isNull():
            Path.Log.warning("Boolean cut for avoid_faces failed.")
            return main_boundary
        return final_mask
    except Exception as e:
        Path.Log.error(f"Failed to cut avoid_faces from boundary mask: {e}")
        return main_boundary


def build_optimized_boundary(faces, offset, tolerance=0.005, avoids=False):
    """
    Acts as a middleman to optimize boundary creation.

    Separates faces into connected groups and isolated faces. Each connected
    group is processed as a single batch — faces that touch transitively are
    guaranteed to be in the same batch, preventing TechDraw/ClipperLib
    artifacts from disjoint geometry. Isolated faces are processed one by one.

    Args:
        faces (list): List of Part.Face objects or nested list of faces.
        offset (float): Offset to apply to each boundary.
        tolerance (float): Maximum distance to be considered touching.
        avoids (bool): Default 'False'. 'True' only from _preprocess_avoid_faces.

    Returns:
        Part.Shape: The combined boundary shape, or None on failure.
    """
    if not faces:
        return None

    touching_groups, isolated_faces = _separate_touching_faces(faces)

    Path.Log.debug(
        f"build_optimized_boundary: {len(touching_groups)} touching group(s), "
        f"{len(isolated_faces)} isolated face(s)."
    )

    generated_boundaries = []

    # Process each connected group as a single batch
    for group in touching_groups:
        bnd = create_boundary_face(group, offset, tolerance, avoids)
        if bnd and not bnd.isNull():
            generated_boundaries.append(bnd)

    # Process isolated faces one by one
    for face in isolated_faces:
        bnd = create_boundary_face([face], offset, tolerance, avoids)
        if bnd and not bnd.isNull():
            generated_boundaries.append(bnd)

    if not generated_boundaries:
        return None

    if len(generated_boundaries) == 1:
        return generated_boundaries[0]

    try:
        final_boundary = generated_boundaries[0].fuse(generated_boundaries[1:])
        if hasattr(final_boundary, "removeSplitter"):
            final_boundary = final_boundary.removeSplitter()
        return final_boundary
    except Exception as e:
        Path.Log.warning(
            f"build_optimized_boundary: Failed to fuse boundaries: {e}. "
            "Returning first boundary only."
        )
        return generated_boundaries[0]


def _separate_touching_faces(faces, tolerance=0.01):
    """
    Separates a list of faces into groups of touching faces and a list of
    isolated faces, based on XY bounding box overlap and physical distance.

    Uses a union-find (disjoint set) algorithm to correctly group transitively
    connected faces — if A touches B and B touches C, all three end up in the
    same group even if A and C don't directly touch.

    The bb_overlap pre-check tests both X and Y independently and only rejects
    when BOTH axes fail to overlap — a face touching only in Y is correctly
    identified as overlapping.

    Args:
        faces (list): A list of Part.Face objects or nested list of faces.
        tolerance (float): Maximum distance to be considered touching.

    Returns:
        tuple: (touching_groups, isolated_faces)
            touching_groups (list of lists): Each inner list is a group of
                mutually connected faces. Groups with a single face that
                touches another group are included here.
            isolated_faces (list): Faces that touch no other face.
    """
    if not faces:
        return [], []

    import math

    # Flatten input — handles both [Face, Face] and [[Face], [Face]]
    flat_faces = []
    for item in faces:
        if isinstance(item, list):
            flat_faces.extend(item)
        else:
            flat_faces.append(item)

    if not flat_faces:
        return [], []

    n = len(flat_faces)

    # XY-only bounding box overlap — Z deliberately excluded
    def bb_overlap(bb1, bb2, tol):
        if bb1.XMax < bb2.XMin - tol or bb1.XMin > bb2.XMax + tol:
            return False
        if bb1.YMax < bb2.YMin - tol or bb1.YMin > bb2.YMax + tol:
            return False
        return True  # Both axes overlap — faces may be touching

    # Union-Find implementation for transitive grouping
    parent = list(range(n))

    def find(i):
        while parent[i] != i:
            parent[i] = parent[parent[i]]  # Path compression
            i = parent[i]
        return i

    def union(i, j):
        ri, rj = find(i), find(j)
        if ri != rj:
            parent[ri] = rj

    # Pre-compute bounding boxes once
    bboxes = [f.BoundBox for f in flat_faces]

    # Compare every pair — union touching faces into the same group
    for i in range(n):
        for j in range(i + 1, n):
            if not bb_overlap(bboxes[i], bboxes[j], tolerance):
                continue
            try:
                dist = flat_faces[i].distToShape(flat_faces[j])[0]
                if dist <= tolerance:
                    union(i, j)
                    continue
            except Exception as e:
                Path.Log.debug(
                    f"_separate_touching_faces: distToShape failed for " f"faces {i},{j}: {e}"
                )
            # Fallback: check if face centroids are within a larger
            # proximity threshold based on average face diagonal.
            try:
                bb_i = bboxes[i]
                bb_j = bboxes[j]
                cx_i = (bb_i.XMin + bb_i.XMax) / 2
                cy_i = (bb_i.YMin + bb_i.YMax) / 2
                cx_j = (bb_j.XMin + bb_j.XMax) / 2
                cy_j = (bb_j.YMin + bb_j.YMax) / 2
                centroid_dist = math.hypot(cx_i - cx_j, cy_i - cy_j)
                avg_diag = (
                    math.hypot(bb_i.XLength, bb_i.YLength) + math.hypot(bb_j.XLength, bb_j.YLength)
                ) / 2
                if centroid_dist < avg_diag * 0.75:
                    union(i, j)
            except Exception as e:
                Path.Log.debug(
                    f"_separate_touching_faces: centroid check failed for " f"faces {i},{j}: {e}"
                )

    # Collect groups by root
    from collections import defaultdict

    groups = defaultdict(list)
    for i in range(n):
        groups[find(i)].append(flat_faces[i])

    touching_groups = []
    isolated_faces = []

    for group in groups.values():
        if len(group) == 1:
            isolated_faces.append(group[0])
        else:
            touching_groups.append(group)

    return touching_groups, isolated_faces


def _is_triangulated_mesh(faces, sample_size=75, threshold=0.50):
    """
    Heuristically detects whether `faces` come from a mesh-to-shape
    conversion, as opposed to a genuine CAD/BRep model.

    A bare "exactly 3 edges" test isn't enough — plenty of legitimate
    BRep faces on curved/circular parts (a conical wedge, a fillet
    blend, a trimmed cylindrical face) are also bounded by 3 edges while
    remaining curved. A genuine mesh-derived triangle is additionally
    *planar*.

    Args:
        faces (list): Part.Face objects to sample.
        sample_size (int): Maximum number of faces to sample.
        threshold (float): Fraction of sampled faces that must look like
            mesh triangles for the whole set to be flagged as triangulated.

    Returns:
        bool: True if the sampled faces look like a mesh-to-shape conversion.
    """
    if not faces:
        return False

    sample = faces[: min(sample_size, len(faces))]

    def _looks_like_mesh_triangle(face):
        if len(face.Edges) == 3:

            try:
                if isinstance(face.Surface, Part.Plane):
                    return True
            except Exception:
                return False

        return False

    triangle_count = sum(1 for f in sample if _looks_like_mesh_triangle(f))
    return len(sample) > 0 and (triangle_count / len(sample)) > threshold


def _filter_vertical(model_faces, tolerance=0.0005):
    """Removes vertical faces from a list of Part.Face objects.

    This function is a performance optimization for BaseBoundBox only.

    The "true" outward-pointing normal is calculated by respecting the
    face's topological orientation.

    Args:
        model_faces (list): A list of Part.Face objects to be filtered.
        tolerance (float, optional): The threshold for the normal vector's
            Z-component. Faces with abs(normal.z) less than this value are
            considered vertical and will be removed. Defaults to 0.0005.

    Returns:
        list: A new list of Part.Face objects with the vertical faces
            excluded. Returns the original list if filtering would result
            in an empty list.
    """
    filtered = []

    for face in model_faces:
        u1, u2, v1, v2 = face.ParameterRange
        norm = face.normalAt((u1 + u2) / 2.0, (v1 + v2) / 2.0)
        if face.Orientation == "Reversed":
            norm = norm.multiply(-1)

        normal_z = abs(norm.z)

        # Reject truly vertical faces
        if normal_z < tolerance:
            continue
        filtered.append(face)

    if filtered:
        return filtered

    return model_faces


# ---------------------------------------------------------------------------
# Avoid Faces Boundary creation
# ---------------------------------------------------------------------------


def build_avoid_boundary(avoid_faces, avoid_overlap, tolerance):
    """
    Builds the 2D "keep-out" boundary for user-selected Avoid Faces.

    Each raw face is first classified and, if needed, capped to a flat
    shape suitable for boundary generation:

      - Planar faces (flat or tilted, including ones with a genuine hole
        such as an annular/donut selection) are used as-is.
      - Non-planar faces (the cylindrical, conical, or otherwise curved
        wall of a hole or pocket) are reduced to a flat cap at their
        topmost rim, since a standard top-down projection would otherwise
        distort a tilted or cylindrical wall into an ellipse.
      - Any face that can't be resolved this way is grouped with the
        others like it and processed together as a fallback, rather than
        being left as raw, unprocessed geometry.

    The resulting faces are then combined and offset by avoid_overlap
    (expanded outward, with a small extra buffer to avoid path spikes on
    vertical walls) to produce the final avoid-zone boundary. This
    boundary is used both to build a collision-safety pillar around each
    Avoid Face and to cut a matching hole out of the machining area.

    Args:
        avoid_faces (list): Raw Part.Face objects selected by the user as
            Avoid Faces.
        avoid_overlap (float): A negative offset value if Avoid Faces
            Overlap is enabled, or the tool radius otherwise.
        tolerance (float): The deflection tolerance for discretizing
            curves smoothly.

    Returns:
        Part.Shape: The offset avoid-zone boundary, or None if
        avoid_faces is empty or boundary generation fails.
    """
    if not avoid_faces:
        return None

    prepared_faces, fallback_faces = _classify_and_cap_faces(avoid_faces)

    if fallback_faces:
        secondary = build_optimized_boundary(fallback_faces, 0.0, 0.001)
        if secondary is not None:
            prepared_faces.append(secondary)
        else:
            Path.Log.warning(
                f"Failed to build a fallback boundary for {len(fallback_faces)} unresolved avoid face(s); they will be dropped."
            )

    if not prepared_faces:
        Path.Log.debug("build_avoid_boundary: Nothing left to build a boundary from.")
        return None

    # Small buffer to avoid "path spikes" on vertical walls
    epsilon = max(0.01, tolerance + 0.001)

    avoid_boundary = build_optimized_boundary(
        prepared_faces,
        avoid_overlap + epsilon,
        tolerance,
        avoids=True,
    )

    if not avoid_boundary:
        Path.Log.warning("Failed to generate boundary for avoid_faces.")
        return None

    return avoid_boundary


def _classify_and_cap_faces(raw_faces):
    """
    Sorts raw Avoid Faces into two buckets: faces already usable as-is,
    and faces that need further, batched processing.

    For each face:
      - Faces with no edges are set aside for the fallback batch.
      - Planar faces (flat or tilted, including ones with a genuine hole
        such as an annular/donut selection) are used as-is.
      - Non-planar faces (a cylindrical, conical, or otherwise curved
        wall) are capped at their topmost rim. If that succeeds, the cap
        is used; if not, the original face is set aside for the fallback
        batch instead.

    Args:
        raw_faces (list): Raw Part.Face objects selected by the user.

    Returns:
        tuple: (prepared_faces, fallback) — prepared_faces is a list of
        faces ready to use directly; fallback is a list of faces that
        couldn't be individually resolved and need further processing.
    """
    prepared_faces = []
    fallback_faces = []

    for raw_face in raw_faces:
        if not hasattr(raw_face, "Edges") or not raw_face.Edges:
            fallback_faces.append(raw_face)
            continue

        # Skip if face is planar
        try:
            is_planar = isinstance(raw_face.Surface, Part.Plane)
        except Exception:
            is_planar = False

        if is_planar:
            prepared_faces.append(raw_face)
            continue

        # Non-Planar 3D Walls (Sloped, Tapered, Curved)
        face_zmax = round(raw_face.BoundBox.ZMax, 4)
        face_zmin = round(raw_face.BoundBox.ZMin, 4)

        top_edges = []
        for edge in raw_face.Edges:
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
            fallback_faces.append(raw_face)
            continue

        try:
            # Reconstruct just the top boundary into a new wire
            sorted_edges = Part.__sortEdges__(top_edges)
            top_wire = Part.Wire(sorted_edges)

            # 3. Discretize and crush the 3D rim to a flat 2D polygon at Z-Max.
            # This safely handles curved 3D splines and perfectly preserves the
            # "egged" ellipse of angled holes without crashing OpenCASCADE.
            flat_edges = []
            for edge in top_wire.Edges:
                points = edge.discretize(Distance=0.1)
                flat_pts = [FreeCAD.Vector(p.x, p.y, face_zmax) for p in points]
                flat_polygon = Part.makePolygon(flat_pts)
                flat_edges.extend(flat_polygon.Edges)

            sorted_flat = Part.__sortEdges__(flat_edges)
            flat_wire = Part.Wire(sorted_flat)
            cap_face = Part.Face(flat_wire)

            if cap_face.isValid() and not cap_face.isNull():
                prepared_faces.append(cap_face)
            else:
                fallback_faces.append(raw_face)

        except Exception as e:
            Path.Log.debug(
                "_preprocess_avoid_faces: Failed to pre-process Avoid Face. "
                f"Fall back to the original Avoid Faces process: {e}"
            )
            fallback_faces.append(raw_face)

    return prepared_faces, fallback_faces
