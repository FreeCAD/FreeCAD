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
import time

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


def make_boundary_face(cutting_faces, offset, tolerance=0.005):
    """
    Creates a mathematically precise 2D boundary face (mask) on the XY plane.

    This function takes an array of 3D faces, projects their absolute silhouette
    onto the Z=0 plane, and cleanly offsets that silhouette using ClipperLib.
    It guarantees a contiguous, non-crisscrossing polygon that smoothly matches
    the user's exact accuracy settings.

    Args:
        faces_to_mask (list): A list of Part.Face objects to derive the silhouette from.
        offset (float): The expansion (positive) or contraction (negative) distance.
        tolerance (float): The deflection tolerance used for drawing smooth arcs.
                           Derived directly from the user's LinearDeflection property.

    Returns:
        Part.Face: A single, flat 2D face on the XY plane representing the clipping boundary.
                   Returns None if geometry extraction or offsetting fails.
    """
    import TechDraw
    import PathScripts.PathUtils as PathUtils

    if not cutting_faces:
        return None

    offset_face = None
    boundary_face = None

    # Create a single compound of the 3D faces to find the global silhouette
    compound = Part.Compound(cutting_faces)

    # Extract the exact 2D projection outline looking down the Z-axis
    outer_wire = TechDraw.findShapeOutline(compound, 1, FreeCAD.Vector(0, 0, 1))

    # Fallback if the TechDraw projection module fails on complex geometry
    if not outer_wire or not hasattr(outer_wire, "Edges") or len(outer_wire.Edges) == 0:
        Path.Log.warning(
            "surface_common.make_boundary_face: TechDraw could not find a valid outline. Attempting fallback."
        )
        outer_wire = compound

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
            "surface_common.make_boundary_face: Offsetting the selected faces resulted in an empty shape."
        )
        return None

    # Convert the offset 2D wire into a solid masking face
    boundary_face = Part.makeFace(offset_shape)
    if not boundary_face:
        Path.Log.warning(f"surface_common.make_boundary_face: Failed to create smooth boundary face: {e}")
        return None

    if boundary_face.BoundBox.ZMin != 0.0:
        boundary_face.translate(FreeCAD.Vector(0, 0, -boundary_face.BoundBox.ZMin))
        
    return boundary_face


def generate_pattern_mask(cutting_faces, avoid_faces, tool_radius, boundary_adj, tolerance):
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
        Path.Log.warning(
            "surface_common.generate_pattern_mask:Could not determine geometry for main boundary mask."
        )
        return None

    # 1. Create the Main Outer Boundary
    outer_offset = -tool_radius + boundary_adj
    main_boundary = make_boundary_face(cutting_faces, outer_offset, tolerance)

    if not main_boundary:
        Path.Log.warning(
            "surface_common.generate_pattern_mask: Could not determine geometry for main boundary mask."
        )
        return None

    # 2. Create the "Keep-Out" Zones from Avoid Faces
    if not avoid_faces:
        return main_boundary

    Path.Log.info(
        f"surface_common.generate_pattern_mask: Punching {len(avoid_faces)} holes for avoided faces."
    )

    # For avoid zones, we want to keep the tool center away, so we expand the boundary
    epsilon = tolerance + 0.001  # Allow some extra room to avoid "path spikes" on vertical walls
    avoid_boundary = make_boundary_face(avoid_faces, tool_radius+epsilon, tolerance)

    if not avoid_boundary:
        Path.Log.warning(
            "surface_common.generate_pattern_mask: Failed to generate boundary for avoid_faces."
        )
        return main_boundary

    # 3. Punch the holes
    try:
        final_mask = main_boundary.cut(avoid_boundary)
        if final_mask.isNull():
            Path.Log.warning(
                "surface_common.generate_pattern_mask: Boolean cut for avoid_faces failed."
            )
            return main_boundary
        return final_mask
    except Exception as e:
        Path.Log.error(
            f"surface_common.generate_pattern_mask: Failed to cut avoid_faces from boundary mask: {e}"
        )
        return main_boundary

