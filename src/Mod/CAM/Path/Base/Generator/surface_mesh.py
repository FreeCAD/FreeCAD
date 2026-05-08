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

"""
Fast STL operations for surface generation.

This module provides a C++ accelerated implementation of shape_to_stl
when available, with fallback to the legacy pure Python implementation.

Tests in TestSurfaceStl verify results and provide a performance comparison.
C++ version is about 2.1x faster.
Python version and tests can be removed if deemed unnecessary. (Waterline needs Python for now)
"""

import Path
import Part
import time
import FreeCAD

# Try to import mesh simplification library
try:
    import fast_simplification

    _HAS_SIMPLIFICATION = True
    Path.Log.info("surface_mesh: Using fast-simplification library for mesh optimization")
except ImportError:
    _HAS_SIMPLIFICATION = False
    Path.Log.info("surface_mesh: fast-simplification not available, mesh optimization disabled")

# Try to import C++ implementation
try:
    # Import the compiled C++ extension module
    import surface_generator as _stl_cpp

    _HAS_CPP = True
    Path.Log.info("surface_mesh: Using C++ accelerated implementation")
except ImportError as e:
    _HAS_CPP = False
    Path.Log.info(f"surface_mesh: C++ not available ({e}), using Python fallback")
except Exception as e:
    _HAS_CPP = False
    Path.Log.info(f"surface_mesh: C++ import error ({e}), using Python fallback")


def _apply_mesh_simplification(vertices, facets, simplification_level):
    """Apply mesh simplification to reduce triangle count.

    Args:
        vertices: List of vertex coordinates [[x,y,z], ...]
        facets: List of facet indices [[v1,v2,v3], ...]
        simplification_level: Integer from 1-7 (1=highest accuracy, 7=fastest)

    Returns:
        Tuple of (simplified_vertices, simplified_facets)
    """
    if not _HAS_SIMPLIFICATION or simplification_level <= 1:
        Path.Log.debug("surface_mesh: Mesh simplification disabled or level 1 (no reduction)")
        return vertices, facets

    # Map simplification level (1-7) to reduction ratios
    # Based on PR #24450 performance analysis
    reduction_ratios = {
        1: 0.0,  # No reduction (highest accuracy)
        2: 0.1,  # 10% reduction
        3: 0.2,  # 20% reduction
        4: 0.3,  # 30% reduction
        5: 0.5,  # 50% reduction (moderate)
        6: 0.7,  # 70% reduction (aggressive)
        7: 0.9,  # 90% reduction (maximum speed)
    }

    reduction = reduction_ratios.get(simplification_level, 0.0)
    if reduction <= 0.0:
        return vertices, facets

    start_time = time.perf_counter()
    original_triangles = len(facets)

    if original_triangles == 0:
        return vertices, facets

    try:
        # Apply fast quadratic mesh simplification
        simplified_vertices, simplified_facets = fast_simplification.simplify(
            vertices, facets, reduction
        )

        simplification_time = time.perf_counter() - start_time
        final_triangles = len(simplified_facets)
        actual_reduction = (original_triangles - final_triangles) / original_triangles * 100
        Path.Log.info(
            f"surface_mesh: Mesh simplification level {simplification_level}: "
            f"{original_triangles} → {final_triangles} triangles "
            f"({actual_reduction:.1f}% reduction, {simplification_time:.3f}s)"
        )

        return simplified_vertices, simplified_facets

    except Exception as e:
        Path.Log.warning(f"Mesh simplification failed: {e}, using original mesh")
        return vertices, facets


def _shape_to_stl_cpp(shape, linear_deflection, angular_deflection):
    """C++ accelerated shape to STL conversion.

    Args:
        shape: A Part.Shape-like object (TopoShapePy).
        linear_deflection: Linear deflection for tessellation (mm).
        angular_deflection: Angular deflection for tessellation (degrees).

    Returns:
        Tuple of (vertices, faces) for OCL conversion.

    Raises:
        RuntimeError: If C++ shape extraction fails.
    """

    start_time = time.perf_counter()

    if not hasattr(shape, "ShapeType"):
        if hasattr(shape, "Shape"):
            shape = shape.Shape
        else:
            raise ValueError("Expected Part.Shape-like object or object with Shape property")

    Path.Log.debug(
        f"surface_mesh._shape_to_stl_cpp: shape type={type(shape)}, ShapeType={getattr(shape, 'ShapeType', 'N/A')}"
    )
    Path.Log.debug(
        f"surface_mesh._shape_to_stl_cpp: deflection params linear={linear_deflection}, angular={angular_deflection}"
    )

    # C++ tessellation
    cpp_start = time.perf_counter()
    verts, faces = _stl_cpp.shape_tessellate_fast(shape, linear_deflection, angular_deflection)
    cpp_time = time.perf_counter() - cpp_start

    total_time = time.perf_counter() - start_time
    Path.Log.debug(f"surface_mesh._shape_to_stl_cpp: C++ tessellation time: {cpp_time:.4f}s")
    Path.Log.debug(
        f"surface_mesh._shape_to_stl_cpp: Total C++ shape_to_stl_cpp time: {total_time:.4f}s"
    )
    Path.Log.debug(f"surface_mesh._shape_to_stl_cpp: got {len(verts)} vertices, {len(faces)} faces")

    return verts, faces


def _shape_to_stl_python(shape, linear_deflection, angular_deflection):
    """Python fallback shape to STL conversion.

    Args:
        shape: A Part.Shape-like object (TopoShapePy).
        linear_deflection: Linear deflection for tessellation (mm).
        angular_deflection: Angular deflection for tessellation (degrees).

    Returns:
        Tuple of (vertices, faces) for OCL conversion.
    """

    start_time = time.perf_counter()

    if not hasattr(shape, "ShapeType"):
        if hasattr(shape, "Shape"):
            shape = shape.Shape
        else:
            raise ValueError("Expected Part.Shape-like object or object with Shape property")

    Path.Log.debug(
        f"surface_mesh._shape_to_stl_python: shape type={type(shape)}, ShapeType={getattr(shape, 'ShapeType', 'N/A')}"
    )
    Path.Log.debug(
        f"surface_mesh._shape_to_stl_python: deflection params linear={linear_deflection}, angular={angular_deflection}"
    )

    # Python tessellation
    py_start = time.perf_counter()
    verts, faces = _shape_to_stl_arrays(shape, linear_deflection, angular_deflection)
    py_time = time.perf_counter() - py_start

    total_time = time.perf_counter() - start_time
    Path.Log.debug(f"surface_mesh._shape_to_stl_python: Python tessellation time: {py_time:.4f}s")
    Path.Log.debug(
        f"surface_mesh._shape_to_stl_python: Total Python shape_to_stl_python time: {total_time:.4f}s"
    )
    Path.Log.debug(
        f"surface_mesh._shape_to_stl_python: got {len(verts)} vertices, {len(faces)} faces"
    )

    return verts, faces


def _shape_to_stl_arrays(shape, linear_deflection, angular_deflection):
    """Tessellate a Part.Shape into raw vertex and facet arrays.

    Args:
        shape: A Part.Shape-like object (TopoShapePy).
        linear_deflection: Linear deflection for tessellation (mm).
        angular_deflection: Angular deflection for tessellation (degrees).

    Returns:
        Tuple of (vertices, faces) where vertices is list of [x,y,z] and faces is list of [i0,i1,i2].
    """
    Path.Log.debug(
        f"surface_mesh.shape_to_stl_arrays: shape type={type(shape)}, ShapeType={getattr(shape, 'ShapeType', 'N/A')}"
    )
    Path.Log.debug(
        f"surface_mesh.shape_to_stl_arrays: deflection params linear={linear_deflection}, angular={angular_deflection}"
    )

    import MeshPart as _MeshPart

    mesh_start = time.perf_counter()

    mesh = _MeshPart.meshFromShape(
        Shape=shape,
        LinearDeflection=linear_deflection,
        AngularDeflection=angular_deflection,
    )
    mesh_time = time.perf_counter() - mesh_start

    copy_start = time.perf_counter()
    vertices = [pt.Vector for pt in mesh.Points]
    facet_indices = [f.PointIndices for f in mesh.Facets]

    Path.Log.debug(
        f"surface_mesh.shape_to_stl_arrays: mesh has {len(mesh.Points)} points, {len(mesh.Facets)} facets"
    )
    Path.Log.debug(
        f"surface_mesh.shape_to_stl_arrays: extracted {len(vertices)} vertices, {len(facet_indices)} faces"
    )

    # Sample first few vertices and faces for debug
    if vertices:
        Path.Log.debug(f"surface_mesh.shape_to_stl_arrays: first vertex = {vertices[0]}")
    if facet_indices:
        Path.Log.debug(f"surface_mesh.shape_to_stl_arrays: first face = {facet_indices[0]}")

    copy_time = time.perf_counter() - copy_start

    tri_count = len(facet_indices)
    Path.Log.debug(
        f"surface_mesh.shape_to_stl_arrays: {tri_count} triangles, tessellate {mesh_time:.3f}s, copy {copy_time:.3f}s"
    )

    return vertices, facet_indices


def _shape_to_stl(
    shape, linear_deflection, angular_deflection, mesh_simplification=1, use_cpp=False
):
    """Convert a Part.Shape / Compound to ocl.STLSurf using raw arrays.

    Uses C++ implementation when available for 5-10x performance improvement,
    otherwise falls back to pure Python implementation.

    Args:
        shape: A Part.Shape-like object (TopoShapePy).
        linear_deflection: Linear deflection for tessellation (mm).
        angular_deflection: Angular deflection for tessellation (degrees).
        mesh_simplification: Integer 1-7 for mesh simplification (1=highest accuracy, 7=fastest).
        use_cpp: Use C++ accelerated shape to STL conversion.

    Returns:
        An ocl.STLSurf object.
    """

    total_start = time.perf_counter()

    # Tessellation phase
    tess_start = time.perf_counter()
    if _HAS_CPP and use_cpp:  # use_cpp - Waterline and shape_to_stl_cpp issue not yet solved
        try:
            verts, faces = _shape_to_stl_cpp(shape, linear_deflection, angular_deflection)
        except RuntimeError as e:
            Path.Log.warning(f"C++ shape extraction failed: {e}, falling back to Python")
            verts, faces = _shape_to_stl_python(shape, linear_deflection, angular_deflection)
    else:
        verts, faces = _shape_to_stl_python(shape, linear_deflection, angular_deflection)

    raw_tess_time = time.perf_counter() - tess_start
    Path.Log.debug(
        f"surface_mesh._shape_to_stl: Raw tessellation time: {raw_tess_time:.4f}s ({len(faces)} triangles)"
    )

    # Mesh simplification phase (if enabled)
    simp_start = time.perf_counter()
    verts, faces = _apply_mesh_simplification(verts, faces, mesh_simplification)
    simp_time = time.perf_counter() - simp_start
    Path.Log.debug(f"surface_mesh._shape_to_stl: Mesh simplification time: {simp_time:.4f}s")

    total_tess_time = time.perf_counter() - tess_start
    Path.Log.debug(
        f"surface_mesh._shape_to_stl: Total tessellation time: {total_tess_time:.4f}s (including simplification)"
    )

    # OCL conversion phase (Python only)
    ocl_start = time.perf_counter()
    from . import surface_common

    ocl = surface_common._get_ocl()

    stl = ocl.STLSurf()
    addTriangle = stl.addTriangle
    Point = ocl.Point
    Triangle = ocl.Triangle

    for f0, f1, f2 in faces:
        addTriangle(
            Triangle(
                Point(verts[f0][0], verts[f0][1], verts[f0][2]),
                Point(verts[f1][0], verts[f1][1], verts[f1][2]),
                Point(verts[f2][0], verts[f2][1], verts[f2][2]),
            )
        )

    ocl_time = time.perf_counter() - ocl_start
    total_time = time.perf_counter() - total_start

    Path.Log.debug(f"surface_mesh._shape_to_stl: OCL conversion time: {ocl_time:.4f}s")
    Path.Log.debug(f"surface_mesh._shape_to_stl: Total shape_to_stl time: {total_time:.4f}s")

    return stl


def _mesh_to_stl(mesh_obj):
    """
    Converts a FreeCAD Mesh object directly to an ocl.STLSurf.

    This function handles the entire conversion process, from extracting raw
    point and facet data to constructing the final OCL mesh object.

    Args:
        mesh_obj (Mesh::Feature): The source FreeCAD Mesh object.
        timer (callable, optional): A callback for performance instrumentation.

    Returns:
        ocl.STLSurf: The generated OCL mesh object, or None on failure.
    """
    if not hasattr(mesh_obj, "Mesh") or not mesh_obj.Mesh.Facets:
        Path.Log.error("The provided object is not a valid mesh or is empty.")
        return None

    mesh_start = time.perf_counter()
    mesh_data = mesh_obj.Mesh
    mesh_points = [tuple(p) for p in mesh_data.Points]
    mesh_facets = [tuple(f.PointIndices) for f in mesh_data.Facets]

    Path.Log.debug(
        f"surface_mesh.mesh_to_stl: input {len(mesh_points)} points, {len(mesh_facets)} facets"
    )

    ocl = surface_common._get_ocl()
    stl = ocl.STLSurf()
    addTriangle = stl.addTriangle
    Point = ocl.Point
    Triangle = ocl.Triangle

    for facet in mesh_facets:
        # Handle different point formats
        if len(facet) != 3:
            Path.Log.warning(f"Mesh_to_STL: skipping invalid facet {facet}")
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

    mesh_time = time.perf_counter() - mesh_start

    Path.Log.debug(
        f"surface_mesh.mesh_to_stl: created STL with {stl.size()} triangles in {mesh_time:.3f}s"
    )

    return stl


def _shape_to_safe_stl(
    model_shape,
    avoid_faces,
    tool_radius,
    start_depth,
    linear_deflection,
    angular_deflection,
):
    """
    Generates the secondary (safety) STL mesh for collision avoidance.

    This function creates a robust collision model by fusing the model with an
    "invisible floor" and extruded "keep-out zones" for any avoided faces. It
    applies hollowing and generates a coarse mesh for maximum performance.

    Args:
        model_shape (Part.Shape): The complete, un-clipped model geometry.
        avoid_faces (list): A list of Part.Face objects to be avoided.
        tool_radius (float): The radius of the active tool.
        start_depth (float): The upper Z-bound of the operation.
        linear_deflection (float): The base linear deflection for calculating a coarse mesh.
        angular_deflection (float): The base angular deflection for calculating a coarse mesh.

    Returns:
        ocl.STLSurf: The generated safety mesh, or None on failure.
    """
    fused_shapes = []
    boundary_face = None

    fused_shapes.append(model_shape)

    # Add the "Invisible Floor" base plate
    bb = model_shape.BoundBox
    plate_padding = tool_radius * 2
    base_plate = Part.makeBox(
        bb.XLength + plate_padding * 2,
        bb.YLength + plate_padding * 2,
        0.1,
        FreeCAD.Vector(bb.XMin - plate_padding, bb.YMin - plate_padding, bb.ZMin - 0.1),
    )
    fused_shapes.append(base_plate)

    # Create and add the extruded "Keep-Out Pillars" for avoided faces
    if avoid_faces:
        Path.Log.debug(
            f"surface_mesh._shape_to_safe_stl: Generating extruded envelope for {len(avoid_faces)} avoided faces."
        )
        from . import surface_common

        boundary_face = surface_common.build_optimized_boundary(
            [avoid_faces], tool_radius, linear_deflection
        )

        if not boundary_face:
            Path.Log.error("Failed to generate Safe STL. Transitions may not be collision-safe.")
            return None

        height = abs(start_depth - bb.ZMin) + 0.1  # Plus 0.1 for safety
        avoid_solid = boundary_face.extrude(FreeCAD.Vector(0, 0, -height))
        avoid_solid.translate(FreeCAD.Vector(0, 0, start_depth + 0.1))
        fused_shapes.append(avoid_solid)

    # Fuse, Hollow, and create a coarse mesh
    safe_compound = Part.Compound(fused_shapes)
    hollow_shape = safe_compound
    if safe_compound.Shells:
        hollow_shape = Part.makeCompound(safe_compound.Shells)

    try:
        safe_stl = _shape_to_stl(
            hollow_shape, linear_deflection, angular_deflection, mesh_simplification=5, use_cpp=True
        )

        Path.Log.debug("surface_mesh._shape_to_safe_stl: Safe STL generated successfully.")
    except Exception as e:
        Path.Log.error(
            f"Failed to generate Safe STL. Transitions may not be collision-safe. Error: {e}"
        )
        return None

    return safe_stl


def generate_stl(
    model_shape,
    base_objs,
    avoid_faces,
    tool_radius,
    needs_safe_stl,
    start_depth,
    final_depth,
    linear_deflection,
    angular_deflection,
    mesh_simplification,
    use_cpp,
):
    """
    Orchestrates the creation of the primary (machining) and secondary (safety) STL meshes.

    This function acts as a high-level controller. It generates the primary STL from
    the appropriate geometry (selected faces or full model) and then, if required,
    delegates the creation of the complex safety STL to the _shape_to_safe_stl helper.

    Args:
        model_shape (Part.Shape): The mathematically fused solid of the entire Job model.
        base_objs (list): The source geometric objects from the Job (can be Part or Mesh).
        avoid_faces (list): A list of Part.Face objects to be avoided.
        tool_radius (float): The radius of the active tool.
        needs_safe_stl (bool): Flag indicating if the safety model is required.
        start_depth (float): The upper Z-bound of the operation.
        final_depth (float): The lower Z-bound of the operation.
        linear_deflection (float): The user-set linear deflection for the primary mesh.
        angular_deflection (float): The user-set angular deflection for the primary mesh.
        mesh_simplification (int): The user-set simplification level for the primary mesh.
        use_cpp: (bool): Use C++ accelerated shape to STL conversion.

    Returns:
        tuple: (stl, safe_stl), where stl is the primary mesh and safe_stl is the
               collision mesh (or a copy of stl if generation failed or wasn't needed).
    """
    stl, safe_stl = None, None

    if not base_objs:
        Path.Log.error("No base models provided for STL generation.")
        return None, None

    # Dispatch based on geometry type
    is_mesh_op = hasattr(base_objs[0], "TypeId") and base_objs[0].TypeId.startswith("Mesh")

    if is_mesh_op:
        Path.Log.debug(
            "surface_mesh.generate_stl. Mesh object detected as Base. Using direct mesh conversion."
        )
        stl = mesh_to_stl(base_objs[0])
        if stl is None:
            Path.Log.error("Could not create a valid shape for primary STL generation.")
            return None, None

        return stl, stl
    else:
        # Generate the primary machining STL
        if not model_shape or model_shape.isNull():
            Path.Log.error("Could not create a valid shape for primary STL generation.")
            return None, None

        # Pre-clip the full model shape to the final depth
        bbox = model_shape.BoundBox
        padding = 1.0
        clip_z = min(final_depth, bbox.ZMax)
        clip_height = bbox.ZMax - clip_z + padding

        try:
            clipper_box = Part.makeBox(
                bbox.XLength + padding * 2,
                bbox.YLength + padding * 2,
                clip_height,
                FreeCAD.Vector(bbox.XMin - padding, bbox.YMin - padding, clip_z),
            )

            clipped_shape = model_shape.common(clipper_box)
        except Exception as e:
            # Catch any other OpenCASCADE topology errors gracefully
            clipped_shape = model_shape
            Path.Log.warning(
                f"Failed to create clipping boundary. Check your Job Origin and Depths. "
                f"Using original full model. (Error: {e})"
            )

        if clipped_shape.isNull():
            Path.Log.warning(
                "Pre-clipping the machining shape resulted in an empty shape. Using original full model."
            )
            clipped_shape = model_shape

        # Generate the primary STL
        stl = _shape_to_stl(
            clipped_shape,
            linear_deflection,
            angular_deflection,
            mesh_simplification,
            use_cpp,
        )

        # Check if the STL object is None OR if it contains zero triangles.
        if stl is None or stl.size() == 0:
            Path.Log.debug(
                "surface_mesh.generate_stl.Failed to create a valid STL from the model (mesh is empty)."
            )
            return None, None

        # Generate the Safe STL
        if needs_safe_stl:
            safe_stl = _shape_to_safe_stl(
                model_shape,
                avoid_faces,
                tool_radius,
                start_depth,
                linear_deflection,
                angular_deflection,
            )

        if safe_stl is None:
            safe_stl = stl

        return stl, safe_stl
