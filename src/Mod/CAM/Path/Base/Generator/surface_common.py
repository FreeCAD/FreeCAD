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
# STL mesh conversion
# ---------------------------------------------------------------------------


def shape_to_stl(shape, linear_deflection, angular_deflection, timer=None):
    """Tessellate a Part.Shape into an ``ocl.STLSurf``.

    This is a thin bridge that tessellates via OCCT
    (``MeshPart.meshFromShape``) then copies triangles into OCL's
    ``STLSurf`` via ``addTriangle()``.

    The per-triangle Python loop may become a bottleneck for
    high-resolution meshes (100 k+ triangles).  The API is designed so
    the implementation can be swapped to a C++ extension later without
    changing callers.

    Args:
        shape: A ``Part.Shape`` (already transformed if needed).
        linear_deflection: Linear deflection for tessellation (mm).
        angular_deflection: Angular deflection for tessellation (degrees).
        timer: Optional callable ``timer(stage_name, elapsed_seconds)``
               for performance instrumentation.

    Returns:
        An ``ocl.STLSurf`` object.

    Note:  This method is a major bottleneck to performance.  It is a critical path to implement a
    C++ helper to speed this up.
    """
    import MeshPart as _MeshPart

    ocl = _get_ocl()

    t0 = time.time()

    mesh = _MeshPart.meshFromShape(
        Shape=shape,
        LinearDeflection=linear_deflection,
        AngularDeflection=angular_deflection,
    )

    t1 = time.time()
    if timer:
        timer("tessellate", t1 - t0)

    vertices = [pt.Vector for pt in mesh.Points]
    facet_indices = [f.PointIndices for f in mesh.Facets]

    stl = ocl.STLSurf()
    for idx in facet_indices:
        v0, v1, v2 = vertices[idx[0]], vertices[idx[1]], vertices[idx[2]]
        t = ocl.Triangle(
            ocl.Point(v0[0], v0[1], v0[2]),
            ocl.Point(v1[0], v1[1], v1[2]),
            ocl.Point(v2[0], v2[1], v2[2]),
        )
        stl.addTriangle(t)

    t2 = time.time()
    if timer:
        timer("stl_copy", t2 - t1)

    tri_count = len(facet_indices)
    Path.Log.debug(
        "shape_to_stl: {} triangles, tessellate {:.3f}s, copy {:.3f}s".format(
            tri_count, t1 - t0, t2 - t1
        )
    )

    return stl


def shape_to_stl_arrays(shape, linear_deflection, angular_deflection, timer=None):
    """Tessellate a Part.Shape into raw vertex and facet arrays.

    Args:
        shape: A Part.Shape-like object (TopoShapePy).
        linear_deflection: Linear deflection for tessellation (mm).
        angular_deflection: Angular deflection for tessellation (degrees).
        timer: Optional callable timer(stage_name, elapsed_seconds).

    Returns:
        Tuple of (vertices, faces) where vertices is list of [x,y,z] and faces is list of [i0,i1,i2].
    """
    Path.Log.debug(
        f"surface_common.shape_to_stl_arrays: shape type={type(shape)}, ShapeType={getattr(shape, 'ShapeType', 'N/A')}"
    )
    Path.Log.debug(
        f"surface_common.shape_to_stl_arrays: deflection params linear={linear_deflection}, angular={angular_deflection}"
    )

    _MeshPart = _get_meshpart()

    t0 = time.time()

    mesh = _MeshPart.meshFromShape(
        Shape=shape,
        LinearDeflection=linear_deflection,
        AngularDeflection=angular_deflection,
    )

    t1 = time.time()
    if timer:
        timer("tessellate", t1 - t0)

    vertices = [pt.Vector for pt in mesh.Points]
    facet_indices = [f.PointIndices for f in mesh.Facets]

    Path.Log.debug(
        f"surface_common.shape_to_stl_arrays: mesh has {len(mesh.Points)} points, {len(mesh.Facets)} facets"
    )
    Path.Log.debug(
        f"surface_common.shape_to_stl_arrays: extracted {len(vertices)} vertices, {len(facet_indices)} faces"
    )

    # Sample first few vertices and faces for debug
    if vertices:
        Path.Log.debug(f"surface_common.shape_to_stl_arrays: first vertex = {vertices[0]}")
    if facet_indices:
        Path.Log.debug(f"surface_common.shape_to_stl_arrays: first face = {facet_indices[0]}")

    t2 = time.time()
    if timer:
        timer("extract_triangles", t2 - t1)

    tri_count = len(facet_indices)
    Path.Log.debug(
        f"surface_common.shape_to_stl_arrays: {tri_count} triangles, tessellate {t1 - t0:.3f}s, copy {t2 - t1:.3f}s"
    )

    return vertices, facet_indices


def mesh_to_stl(mesh_points, mesh_facets, timer=None):
    """Convert raw mesh data (points + facet indices) to ``ocl.STLSurf``.

    Useful when the caller already has mesh data (e.g. from a Mesh
    object rather than a Part.Shape).

    Args:
        mesh_points: Sequence of point-like objects (each with x,y,z or [x,y,z]).
        mesh_facets: Sequence of facet index triples (each [i0,i1,i2]).
        timer: Optional callable timer(stage_name, elapsed_seconds).

    Returns:
        An ``ocl.STLSurf`` object.
    """
    Path.Log.debug(
        f"surface_common.mesh_to_stl: input {len(mesh_points)} points, {len(mesh_facets)} facets"
    )

    ocl = _get_ocl()
    stl = ocl.STLSurf()
    addTriangle = stl.addTriangle
    Point = ocl.Point
    Triangle = ocl.Triangle

    t0 = time.time()

    for facet in mesh_facets:
        # Handle different point formats
        if len(facet) != 3:
            Path.Log.warning(f"surface_common.mesh_to_stl: skipping invalid facet {facet}")
            continue

        i0, i1, i2 = facet

        # Extract points - handle various formats
        p0 = mesh_points[i0]
        p1 = mesh_points[i1]
        p2 = mesh_points[i2]

        # Convert to coordinate tuples
        if hasattr(p0, "x"):
            v0 = (p0.x, p0.y, p0.z)
            v1 = (p1.x, p1.y, p1.z)
            v2 = (p2.x, p2.y, p2.z)
        else:
            v0 = (p0[0], p0[1], p0[2])
            v1 = (p1[0], p1[1], p1[2])
            v2 = (p2[0], p2[1], p2[2])

        addTriangle(Triangle(Point(*v0), Point(*v1), Point(*v2)))

    t1 = time.time()
    if timer:
        timer("mesh_to_stl", t1 - t0)

    Path.Log.debug(
        f"surface_common.mesh_to_stl: created STL with {stl.size()} triangles in {t1 - t0:.3f}s"
    )

    return stl


# ---------------------------------------------------------------------------
# Boundary creation utilities
# ---------------------------------------------------------------------------


def make_boundary_face(selected_faces, tool_radius, boundary_adjustment=0.0):
    """Create a flat XY boundary face from selected faces for point containment testing.

    Projects the outer wires of selected faces onto the XY plane (Z=0),
    offsets outward by tool_radius + boundary_adjustment, and returns
    a flat Part.Face for isInside() testing.

    Args:
        selected_faces: List of Part.Face objects to create boundary from
        tool_radius: Tool radius for offset calculation
        boundary_adjustment: Additional boundary adjustment offset

    Returns:
        Part.Face object for boundary testing, or None if creation fails
    """
    if not selected_faces:
        return None

    try:
        # Collect outer wires from all selected faces using TechDraw outline method
        wires = []

        # Create a compound of all selected faces
        face_compound = Part.Compound(selected_faces)

        # Use TechDraw.findShapeOutline to get the true 2D outline
        import TechDraw

        # Project onto XY plane (Z=1 direction)
        outer_wire = TechDraw.findShapeOutline(face_compound, 1, FreeCAD.Vector(0, 0, 1))
        Path.Log.debug(f"Boundary: TechDraw outline found with {len(outer_wire.Edges)} edges")

        # Discretize the outer wire to get clean points
        discretized_wire = []
        for edge in outer_wire.Edges:
            pts = edge.discretize(Deflection=0.1)
            flat_pts = [FreeCAD.Vector(p.x, p.y, 0.0) for p in pts]
            discretized_wire.extend(flat_pts)

        # Remove duplicate points
        unique_pts = []
        seen = set()
        for pt in discretized_wire:
            pt_key = (round(pt.x, 6), round(pt.y, 6))
            if pt_key not in seen:
                seen.add(pt_key)
                unique_pts.append(pt)

        # Create a clean wire from the unique points
        if len(unique_pts) >= 3:
            # Close the polygon by adding the first point at the end
            if unique_pts[0] != unique_pts[-1]:
                unique_pts.append(unique_pts[0])

            clean_wire = Part.makePolygon(unique_pts)
            wires.append(clean_wire)
            Path.Log.debug(f"Boundary: Created clean wire with {len(unique_pts)} points")
        else:
            Path.Log.warning("Boundary: Not enough points to create wire from TechDraw outline")
            return None

        if not wires:
            return None

        # Make a compound of all wires and create a face
        compound = Part.Compound(wires)
        offset = tool_radius + boundary_adjustment

        # Debug the original compound bounds
        original_bb = compound.BoundBox
        Path.Log.debug(
            f"Boundary: Original compound bounds: X[{original_bb.XMin:.3f}, {original_bb.XMax:.3f}], Y[{original_bb.YMin:.3f}, {original_bb.YMax:.3f}]"
        )

        # Use libarea-based offsetting
        import PathScripts.PathUtils as PathUtils

        Path.Log.debug(
            f"Boundary: Calling getOffsetArea with offset={offset}, compound type={type(compound)}"
        )

        offset_shape = PathUtils.getOffsetArea(
            compound,
            offset,
            removeHoles=False,  # Keep holes for boundary testing
            tolerance=0.01,  # Small tolerance for clean geometry
            plane=compound,  # Use the compound itself as the plane (like Profile.py)
        )

        if offset_shape is None or offset_shape is False:
            Path.Log.error("Boundary: getOffsetArea failed - returned None or False")
            return None

        Path.Log.debug(
            f"Boundary: getOffsetArea returned {type(offset_shape)} with {len(offset_shape.Edges)} edges"
        )
        Path.Log.debug("Boundary: Used libarea getOffsetArea() for offsetting")

        # Debug the offset shape bounds
        offset_bb = offset_shape.BoundBox
        Path.Log.debug(
            f"Boundary: Offset shape bounds: X[{offset_bb.XMin:.3f}, {offset_bb.XMax:.3f}], Y[{offset_bb.YMin:.3f}, {offset_bb.YMax:.3f}], offset={offset:.3f}"
        )
        Path.Log.debug(
            f"Boundary: Offset shape type: {type(offset_shape)}, ShapeType: {getattr(offset_shape, 'ShapeType', 'N/A')}"
        )

        # Create a face from the offset shape
        boundary_face = Part.makeFace(offset_shape)
        Path.Log.debug("Boundary: Created face using makeFace()")
        Path.Log.info(
            "Boundary face created: {} edges, offset={:.3f}".format(len(offset_shape.Edges), offset)
        )

        return boundary_face

    except Exception as e:
        Path.Log.warning("Failed to create boundary face: {}".format(e))
        return None


def generate_boundary_aware_scan_lines(boundary_face, sample_interval, stepover, bounds):
    """Generate scan lines that already respect boundary (legacy strategy adapted).

    This function creates scan lines that are clipped to the boundary during generation,
    avoiding the need for expensive post-processing filtering. It adapts the legacy
    PathGeometryGenerator approach to work with the new Surface architecture.

    Args:
        boundary_face: Part.Face defining the boundary
        sample_interval: Distance between sample points along scan lines
        stepover: Distance between scan lines
        bounds: Bounding box (XMin, XMax, YMin, YMax)

    Returns:
        List of scan lines (each line is a list of (x,y,z) tuples)
    """
    if boundary_face is None:
        return []

    # Get boundary bounds for scan line generation
    bb = boundary_face.BoundBox
    x_min, x_max = bb.XMin, bb.XMax
    y_min, y_max = bb.YMin, bb.YMax

    # Generate scan lines using legacy approach
    scan_lines = []

    # Calculate number of scan lines based on stepover
    num_lines = int((y_max - y_min) / stepover) + 1

    Path.Log.debug(f"Boundary-aware generation: {num_lines} lines, stepover={stepover}")

    for line_idx in range(num_lines):
        y = y_min + line_idx * stepover

        # Create scan line from left to right across boundary bounds
        start_pt = (x_min, y, 0.0)
        end_pt = (x_max, y, 0.0)

        # Create line geometry
        line_geom = Part.makeLine(
            FreeCAD.Vector(start_pt[0], start_pt[1], start_pt[2]),
            FreeCAD.Vector(end_pt[0], end_pt[1], end_pt[2]),
        )

        # Intersect with boundary to clip the line
        try:
            # Create intersection compound
            line_compound = Part.Compound([line_geom])

            # Find intersection with boundary
            intersection = boundary_face.common(line_compound)

            if intersection and hasattr(intersection, "Edges") and len(intersection.Edges) > 0:
                # Extract points from intersected edges
                line_points = []

                for edge in intersection.Edges:
                    # Sample points along the edge at sample_interval
                    edge_length = edge.Length  # Property, not method
                    num_samples = max(2, int(edge_length / sample_interval) + 1)

                    for i in range(num_samples):
                        param = i / (num_samples - 1)  # 0.0 to 1.0
                        # Use valueAt with correct parameter calculation
                        pt = edge.valueAt(
                            edge.FirstParameter + param * (edge.LastParameter - edge.FirstParameter)
                        )
                        line_points.append((pt.x, pt.y, pt.z))

                # Add line if we have valid points
                if len(line_points) >= 2:
                    scan_lines.append(line_points)

        except Exception as e:
            Path.Log.debug(f"Boundary-aware generation: Failed to process line {line_idx}: {e}")
            continue

    Path.Log.info(f"Boundary-aware generation: {len(scan_lines)} scan lines generated")
    return scan_lines


def _make_safe_pdc(safe_stl, cutter, safe_z):
    """Create a reusable PathDropCutter for transition probing."""
    ocl = _get_ocl()
    pdc = ocl.PathDropCutter()
    pdc.setSTL(safe_stl)
    pdc.setCutter(cutter)
    pdc.setZ(safe_z)
    pdc.setSampling(0.5)
    return pdc


def optimize_travel(
    last_point,
    next_point,
    safe_z,
    clearance_z,
    horiz_feed,
    horiz_rapid,
    vert_rapid,
    safe_pdc=None,
    cutter=None,
):
    """Find the shortest safe path between two scan line endpoints.

    For short transitions (≤ 2× cutter diameter), emits a direct G1
    feed move to the next point — the tool stays down without probing.
    Longer transitions fall back to a full ``safe_z`` retract.

    Long-distance optimization is deferred to a future iteration.

    Args:
        last_point: (x, y, z) end of previous scan line.
        next_point: (x, y, z) start of next scan line.
        safe_z: Safe retract height.
        clearance_z: Clearance height.
        horiz_feed: Horizontal cutting feed rate.
        horiz_rapid: Horizontal rapid rate.
        vert_rapid: Vertical rapid rate.
        safe_pdc: Pre-built ``ocl.PathDropCutter`` (from
                  :func:`_make_safe_pdc`).  Reused across transitions.
        cutter: OCL cutter (needed for diameter threshold).

    Returns:
        List of ``Path.Command``.
    """
    if safe_pdc is not None and cutter is not None:
        cutter_diam = cutter.getDiameter()
        dx = next_point[0] - last_point[0]
        dy = next_point[1] - last_point[1]
        xy_dist_sqrd = dx * dx + dy * dy

        if xy_dist_sqrd <= (cutter_diam * 2.0) ** 2:
            # Short transition: direct feed move, no probing needed.
            return [
                Path.Command(
                    "G1",
                    {
                        "X": next_point[0],
                        "Y": next_point[1],
                        "Z": next_point[2],
                        "F": horiz_feed,
                    },
                )
            ]

    # Fallback: full retract
    return [
        Path.Command("G0", {"Z": safe_z, "F": vert_rapid}),
        Path.Command("G0", {"X": next_point[0], "Y": next_point[1], "F": horiz_rapid}),
    ]


def _dropcutter_transition(start, end, pdc, cutter, safe_z, horiz_feed):
    """Probe a short transition path using a pre-built PDC and return G1
    commands along the surface if safe, or ``None`` if the tool must
    retract.

    Conditions for surface-following (all must be true):
    - Every probed CL point contacts the surface (none at ``safe_z``).
    - Z variation along the path is less than the cutter diameter.
    """
    ocl = _get_ocl()

    path = ocl.Path()
    p1 = ocl.Point(start[0], start[1], safe_z)
    p2 = ocl.Point(end[0], end[1], safe_z)
    path.append(ocl.Line(p1, p2))
    pdc.setPath(path)
    pdc.run()

    cl_points = pdc.getCLPoints()
    if not cl_points:
        return None

    zs = [pt.z for pt in cl_points]
    min_z = min(zs)
    max_z = max(zs)

    # Reject if any point is at safe_z (outside STL boundary)
    z_tol = 0.1
    if max_z >= safe_z - z_tol:
        return None

    # Reject if Z variation is too large
    if abs(max_z - min_z) >= cutter.getDiameter():
        return None

    # Safe to cut along the surface
    z_floor = min(start[2], end[2])
    commands = []
    for pt in cl_points[1:]:
        z = max(pt.z, z_floor)
        commands.append(Path.Command("G1", {"X": pt.x, "Y": pt.y, "Z": z, "F": horiz_feed}))
    return commands
