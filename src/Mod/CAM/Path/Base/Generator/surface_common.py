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


# ---------------------------------------------------------------------------
# CL-point filtering via LineCLFilter
# ---------------------------------------------------------------------------


def filter_cl_points(cl_points, tolerance, timer=None):
    """Use OCL LineCLFilter to remove collinear CL-points.

    Replaces the manual ``isOnLineSegment`` checks and
    ``OptimizeLinearPaths`` logic currently in SurfaceSupport.py.

    Args:
        cl_points: List of ``(x, y, z)`` tuples.
        tolerance: Collinearity tolerance (mm).
        timer: Optional callback.

    Returns:
        Filtered list of ``(x, y, z)`` tuples.
    """
    ocl = _get_ocl()

    if len(cl_points) < 3:
        return list(cl_points)

    f = ocl.LineCLFilter()
    f.setTolerance(tolerance)

    for pt in cl_points:
        f.addCLPoint(ocl.CLPoint(pt[0], pt[1], pt[2]))

    t0 = time.time()
    f.run()
    t1 = time.time()

    if timer:
        timer("filter_cl_points", t1 - t0)

    result = [(cl.x, cl.y, cl.z) for cl in f.getCLPoints()]

    Path.Log.debug(
        "filter_cl_points: {} -> {} points in {:.3f}s".format(len(cl_points), len(result), t1 - t0)
    )

    return result


# ---------------------------------------------------------------------------
# Boundary creation utilities
# ---------------------------------------------------------------------------


def make_boundary_face(selected_faces, offset, tolerance=0.005):
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

    if not selected_faces:
        return None

    try:
        # Create a single compound of the 3D faces to find the global silhouette
        compound = Part.Compound(selected_faces)

        # Extract the exact 2D projection outline looking down the Z-axis
        outer_wire = TechDraw.findShapeOutline(compound, 1, FreeCAD.Vector(0, 0, 1))

        # Fallback if the TechDraw projection module fails on complex geometry
        if not outer_wire or not hasattr(outer_wire, "Edges") or len(outer_wire.Edges) == 0:
            outer_wire = compound

        # Let PathUtils (ClipperLib) handle the offsetting natively.
        # This guarantees NO crisscrossing points and uses the exact slider tolerance!
        offset_shape = PathUtils.getOffsetArea(
            outer_wire,
            offset,
            removeHoles=False,
            tolerance=tolerance,
            plane=Part.makeCircle(2.0),  # Flat XY reference plane ensures Z=0 behavior
        )

        if offset_shape is None:
            return None

        # Convert the offset 2D wire into a solid masking face
        boundary_face = Part.makeFace(offset_shape)

        # Ensure the mask is completely flat at Z=0 for boolean intersections
        if boundary_face.BoundBox.ZMin != 0.0:
            boundary_face.translate(FreeCAD.Vector(0, 0, -boundary_face.BoundBox.ZMin))

        return boundary_face

    except Exception as e:
        Path.Log.warning(f"Failed to create smooth boundary face: {e}")
        return None


def generate_pattern_mask(job, obj, selected_faces, tool_radius, boundary_adj):
    """
    Acts as the main router for generating a toolpath containment mask.

    This determines whether to mask against specific selected faces or the entire
    model/stock bounding volume. It enforces "Inside Containment" by mathematically
    shrinking the footprint inwards by the tool's radius so the cutter never gouges
    the unselected exterior geometry.

    Args:
        job (Path.Job): The parent Job object containing the model and stock.
        obj (Path.Operation): The current 3D Surface operation feature.
        selected_faces (list): A list of Part.Face objects specifically selected by the user.
        tool_radius (float): The radius of the active cutter.
        boundary_adj (float): An explicit user-provided offset override.

    Returns:
        Part.Face: The final 2D clipping boundary used by the C++ engine.
    """
    faces_to_mask = []

    if selected_faces:
        faces_to_mask = selected_faces
    else:
        # If no faces are explicitly selected, we assume a "Whole Model" operation.
        # We check if the user wants to limit by Stock bounds, otherwise we use the base Model.
        if obj.BoundBox == "Stock" and job.Stock:
            faces_to_mask = job.Stock.Shape.Faces
        else:
            base_objs = (
                [base for base, subs in obj.Base]
                if hasattr(obj, "Base") and obj.Base
                else job.Model.Group
            )
            for base in base_objs:
                if hasattr(base, "Shape"):
                    faces_to_mask.extend(base.Shape.Faces)

    if not faces_to_mask:
        return None

    # To keep the tool completely inside the boundary, we must shrink the
    # boundary mask inwards by exactly the tool radius.
    offset = -tool_radius + boundary_adj

    # Grab the high-precision linear deflection set by the user's Accuracy slider
    # so the boundary resolution perfectly matches the STL mesh resolution.
    tolerance = obj.LinearDeflection.Value if hasattr(obj, "LinearDeflection") else 0.005

    return make_boundary_face(faces_to_mask, offset, tolerance)
