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
                "CylCutter edge_height <= 0 ({}), using diameter as fallback".format(edge_height)
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
                "BullCutter edge_height <= 0 ({}), using diameter as fallback".format(edge_height)
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


def create_boundary_face(model_faces, offset, tolerance=0.005):
    """
    Creates a 2D boundary mask from the silhouette of an entire model.

    This function uses the high-speed TechDraw module to find the 2D projection
    of a full model shape, which is very efficient for this specific task.

    Args:
        model_faces (list): A list of all faces from a Part.Compound of the model.
        offset (float): The expansion or contraction distance for the boundary.
        tolerance (float): The deflection tolerance for discretizing curves.

    Returns:
        Part.Face: The final 2D boundary face, or None on failure.
    """
    import TechDraw
    import PathScripts.PathUtils as PathUtils

    if not model_faces:
        return None

    outer_wire, offset_shape = None, None

    # Create a single compound of the 3D faces to find the global silhouette
    compound = Part.Compound(model_faces)

    # Extract the exact 2D projection outline looking down the Z-axis
    try:
        outer_wire = TechDraw.findShapeOutline(compound, 1, FreeCAD.Vector(0, 0, 1))
    except Exception as e:
        Path.Log.error(f"TechDraw failed to extract the 2D projection outline from Model: {e}")
        return None

    # Let PathUtils (ClipperLib) handle the offsetting natively.
    offset_shape = PathUtils.getOffsetArea(
        outer_wire,
        offset,
        removeHoles=False,
        tolerance=tolerance,
        plane=Part.makeCircle(2.0),
    )

    if not offset_shape or not hasattr(offset_shape, "Edges") or len(offset_shape.Edges) == 0:
        Path.Log.warning(
            "Offsetting the Model faces resulted in an empty shape. "
            "Extend the boundary if the selected faces are too small."
        )
        return None

    if offset_shape.BoundBox.ZMin != 0.0:
        offset_shape.translate(FreeCAD.Vector(0, 0, -offset_shape.BoundBox.ZMin))

    return offset_shape


def generate_pattern_mask(
    is_whole_model_job, bb_face, cutting_faces, avoid_faces, tool_radius, boundary_adj, tolerance
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
        avoid_faces (list): A list of Part.Face objects to be cut out from the main boundary.
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

    if is_whole_model_job:
        # Use TechDraw.findShapeOutline for whole model silhouette
        main_boundary = bb_face
    else:
        main_boundary = build_optimized_boundary([cutting_faces], outer_offset, tolerance)

    if not main_boundary:
        Path.Log.warning("Could not determine geometry for main boundary mask.")
        return None

    # Create the "Keep-Out" Zones from Avoid Faces
    if not avoid_faces:
        return main_boundary

    # For avoid zones, we want to keep the tool center away, so we expand the boundary
    epsilon = tolerance + 0.001  # Allow some extra room to avoid "path spikes" on vertical walls
    avoid_boundary = build_optimized_boundary([avoid_faces], tool_radius + epsilon, tolerance)

    if not avoid_boundary:
        Path.Log.warning("Failed to generate boundary for avoid_faces.")
        return main_boundary
    # Punch the holes
    try:
        final_mask = main_boundary.cut(avoid_boundary)
        if final_mask.isNull():
            Path.Log.warning("Boolean cut for avoid_faces failed.")
            return main_boundary
        return final_mask
    except Exception as e:
        Path.Log.error(f"Failed to cut avoid_faces from boundary mask: {e}")
        return main_boundary


def build_optimized_boundary(faces, offset, tolerance=0.005):
    """
    Acts as a middleman to optimize boundary creation.
    Separates faces into touching and isolated groups. Passes touching faces
    as a single batch, and isolated faces one by one to prevent TechDraw/ClipperLib artifacts.
    """
    if not faces:
        return None

    # Get our separated lists
    touching_faces, isolated_faces = _separate_touching_faces(faces, tolerance)

    Path.Log.debug(
        f"Boundary Optimization: Processing {len(touching_faces)} touching faces and {len(isolated_faces)} isolated faces."
    )

    generated_boundaries = []

    # 1. Process all touching faces at once
    if touching_faces:
        # Wrapped in a list so create_boundary_face treats them as one group
        touching_bnd = create_boundary_face(touching_faces, offset, tolerance)
        if touching_bnd and not touching_bnd.isNull():
            generated_boundaries.append(touching_bnd)

    # 2. Process isolated faces one by one
    if isolated_faces:
        for face in isolated_faces:
            # Wrapped in a double list so create_boundary_face treats it as a single group of 1 face
            isolated_bnd = create_boundary_face([face], offset, tolerance)
            if isolated_bnd and not isolated_bnd.isNull():
                generated_boundaries.append(isolated_bnd)

    if not generated_boundaries:
        return None

    # 3. Combine all successfully generated boundaries into one final mask
    if len(generated_boundaries) > 1:
        final_boundary = generated_boundaries[0].fuse(generated_boundaries[1:])
        # Clean up any seams left over from fusing
        if hasattr(final_boundary, "removeSplitter"):
            final_boundary = final_boundary.removeSplitter()
    else:
        final_boundary = generated_boundaries[0]

    return final_boundary


def _separate_touching_faces(faces, tolerance=0.005):
    """
    Separate Touching vs. Isolated Faces

    TechDraw.findShapeOutline and PathUtils.getOffsetArea often fail or produce
    artifacts when processing faces that are disjoint (far apart from each other).

    This function evaluates a list of faces and separates them into two groups:
    1. touching_faces: Faces that physically touch at least one other face.
    2. isolated_faces: Faces that are completely alone and touch nothing.

    By separating them, isolated faces can be processed individually and
    compounded safely at the end, completely bypassing OpenCASCADE's
    sensitivity to disjoint bodies.

    Args:
        faces (list): A list of Part.Face objects (or a nested list of faces).
        tolerance (float): Maximum distance to be considered "touching".

    Returns:
        tuple: (touching_faces, isolated_faces) as lists of Part.Face objects.
    """
    touching_faces = []
    isolated_faces = []

    if not faces:
        return touching_faces, isolated_faces

    # 1. Flatten the input list (safely handles both[Face, Face] and [[Face], [Face]])
    flat_faces = []
    for item in faces:
        if isinstance(item, (list, tuple)):
            flat_faces.extend(item)
        else:
            flat_faces.append(item)

    if not flat_faces:
        return touching_faces, isolated_faces

    # Dictionary to keep track of which flat_faces indices have touched another face
    touches = {i: False for i in range(len(flat_faces))}

    # Helper function for a safe, fast Bounding Box intersection check
    def bb_overlap(bb1, bb2, tol):
        if bb1.XMax < bb2.XMin - tol or bb1.XMin > bb2.XMax + tol:
            return False
        if bb1.YMax < bb2.YMin - tol or bb1.YMin > bb2.YMax + tol:
            return False
        if bb1.ZMax < bb2.ZMin - tol or bb1.ZMin > bb2.ZMax + tol:
            return False
        return True

    # 2. Compare every face against every other face
    for i in range(len(flat_faces)):
        face_i = flat_faces[i]
        bb_i = face_i.BoundBox

        for j in range(i + 1, len(flat_faces)):
            face_j = flat_faces[j]
            bb_j = face_j.BoundBox

            # Fast pre-check: Do their bounding boxes overlap?
            if bb_overlap(bb_i, bb_j, tolerance):

                # Precise check: Are the physical geometries actually touching?
                dist = face_i.distToShape(face_j)[0]

                if dist <= tolerance:
                    touches[i] = True
                    touches[j] = True

    # 3. Separate the results based on our tracking dictionary
    for i, face in enumerate(flat_faces):
        if touches[i]:
            touching_faces.append(face)
        else:
            isolated_faces.append(face)

    return touching_faces, isolated_faces
