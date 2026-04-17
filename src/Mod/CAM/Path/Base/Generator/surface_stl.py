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
Python version and tests can be removed if deemed unnecessary.
"""

import Path
import Part
import time
import FreeCAD

# Try to import mesh simplification library
try:
    import fast_simplification

    _HAS_SIMPLIFICATION = True
    Path.Log.info("surface_stl: Using fast-simplification library for mesh optimization")
except ImportError:
    _HAS_SIMPLIFICATION = False
    Path.Log.info("surface_stl: fast-simplification not available, mesh optimization disabled")

# Try to import C++ implementation
try:
    # Import the compiled C++ extension module
    import surface_stl as _stl_cpp

    _HAS_CPP = True
    Path.Log.info("surface_stl: Using C++ accelerated implementation")
except ImportError as e:
    _HAS_CPP = False
    Path.Log.info(f"surface_stl: C++ not available ({e}), using Python fallback")
except Exception as e:
    _HAS_CPP = False
    Path.Log.info(f"surface_stl: C++ import error ({e}), using Python fallback")


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
        Path.Log.debug("surface_stl: Mesh simplification disabled or level 1 (no reduction)")
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

    try:
        # Apply fast quadratic mesh simplification
        simplified_vertices, simplified_facets = fast_simplification.simplify(
            vertices, facets, reduction
        )

        simplification_time = time.perf_counter() - start_time
        final_triangles = len(simplified_facets)
        actual_reduction = (original_triangles - final_triangles) / original_triangles * 100

        Path.Log.info(
            f"surface_stl: Mesh simplification level {simplification_level}: "
            f"{original_triangles} → {final_triangles} triangles "
            f"({actual_reduction:.1f}% reduction, {simplification_time:.3f}s)"
        )

        return simplified_vertices, simplified_facets

    except Exception as e:
        Path.Log.warning(f"surface_stl: Mesh simplification failed: {e}, using original mesh")
        return vertices, facets


def _apply_pre_clipping(shape, final_depth):
    """Apply pre-clipping to only process relevant portion above final depth.

    Args:
        shape: Part.Shape object to clip
        final_depth: Z-coordinate of final cutting depth

    Returns:
        Clipped Part.Shape (or original if clipping fails)
    """
    if final_depth is None or final_depth >= 0:
        Path.Log.debug("surface_stl: Pre-clipping disabled (final_depth is None or >= 0)")
        return shape

    # Check for empty or null shape
    if shape is None or shape.isNull():
        Path.Log.debug("surface_stl: Pre-clipping disabled (shape is null or empty)")
        return shape

    start_time = time.perf_counter()

    try:
        # Create a large box above the final depth
        # Make it significantly larger than the shape bounds
        bounds = shape.BoundBox
        margin = max(bounds.DiagonalLength, 100.0)  # Large margin

        # Box extends from final_depth up to well above the shape
        box_min = FreeCAD.Vector(bounds.XMin - margin, bounds.YMin - margin, final_depth - margin)
        box_max = FreeCAD.Vector(bounds.XMax + margin, bounds.YMax + margin, bounds.ZMax + margin)
        clipping_box = Part.makeBox(
            box_max.x - box_min.x, box_max.y - box_min.y, box_max.z - box_min.z, box_min
        )

        # Perform boolean intersection to get only the portion above final_depth
        clipped_shape = shape.common(clipping_box)

        if clipped_shape is None or clipped_shape.isNull():
            Path.Log.warning("surface_stl: Pre-clipping resulted in empty shape, using original")
            return shape

        # Check if clipping actually reduced the shape
        original_volume = shape.Volume
        clipped_volume = clipped_shape.Volume

        if clipped_volume >= original_volume * 0.95:  # Less than 5% reduction
            Path.Log.debug("surface_stl: Pre-clipping provided minimal benefit, using original")
            return shape

        clipping_time = time.perf_counter() - start_time
        reduction_percent = (1.0 - clipped_volume / original_volume) * 100

        Path.Log.info(
            f"surface_stl: Pre-clipping: removed {reduction_percent:.1f}% volume "
            f"({original_volume:.1f} → {clipped_volume:.1f}, {clipping_time:.3f}s)"
        )

        return clipped_shape

    except Exception as e:
        Path.Log.warning(f"surface_stl: Pre-clipping failed: {e}, using original shape")
        return shape


def _shape_to_stl_cpp(shape, linear_deflection, angular_deflection, timer=None):
    """C++ accelerated shape to STL conversion.

    Args:
        shape: A Part.Shape-like object (TopoShapePy).
        linear_deflection: Linear deflection for tessellation (mm).
        angular_deflection: Angular deflection for tessellation (degrees).
        timer: Optional callable timer(stage_name, elapsed_seconds).

    Returns:
        Tuple of (vertices, faces) for OCL conversion.

    Raises:
        RuntimeError: If C++ shape extraction fails.
    """
    import time

    start_time = time.perf_counter()

    if not hasattr(shape, "ShapeType"):
        if hasattr(shape, "Shape"):
            shape = shape.Shape
        else:
            raise ValueError("Expected Part.Shape-like object or object with Shape property")

    Path.Log.debug(
        f"surface_stl._shape_to_stl_cpp: shape type={type(shape)}, ShapeType={getattr(shape, 'ShapeType', 'N/A')}"
    )
    Path.Log.debug(
        f"surface_stl._shape_to_stl_cpp: deflection params linear={linear_deflection}, angular={angular_deflection}"
    )

    # C++ tessellation
    cpp_start = time.perf_counter()
    verts, faces = _stl_cpp.shape_tessellate_fast(
        shape, linear_deflection, angular_deflection, timer
    )
    cpp_time = time.perf_counter() - cpp_start

    total_time = time.perf_counter() - start_time
    Path.Log.debug(f"surface_stl._shape_to_stl_cpp: C++ tessellation time: {cpp_time:.4f}s")
    Path.Log.debug(
        f"surface_stl._shape_to_stl_cpp: Total C++ shape_to_stl_cpp time: {total_time:.4f}s"
    )
    Path.Log.debug(f"surface_stl._shape_to_stl_cpp: got {len(verts)} vertices, {len(faces)} faces")

    return verts, faces


def _shape_to_stl_python(shape, linear_deflection, angular_deflection, timer=None):
    """Python fallback shape to STL conversion.

    Args:
        shape: A Part.Shape-like object (TopoShapePy).
        linear_deflection: Linear deflection for tessellation (mm).
        angular_deflection: Angular deflection for tessellation (degrees).
        timer: Optional callable timer(stage_name, elapsed_seconds).

    Returns:
        Tuple of (vertices, faces) for OCL conversion.
    """
    import time

    start_time = time.perf_counter()

    if not hasattr(shape, "ShapeType"):
        if hasattr(shape, "Shape"):
            shape = shape.Shape
        else:
            raise ValueError("Expected Part.Shape-like object or object with Shape property")

    Path.Log.debug(
        f"surface_stl._shape_to_stl_python: shape type={type(shape)}, ShapeType={getattr(shape, 'ShapeType', 'N/A')}"
    )
    Path.Log.debug(
        f"surface_stl._shape_to_stl_python: deflection params linear={linear_deflection}, angular={angular_deflection}"
    )

    from . import surface_common

    # Python tessellation
    py_start = time.perf_counter()
    verts, faces = surface_common.shape_to_stl_arrays(
        shape, linear_deflection, angular_deflection, timer
    )
    py_time = time.perf_counter() - py_start

    total_time = time.perf_counter() - start_time
    Path.Log.debug(f"surface_stl._shape_to_stl_python: Python tessellation time: {py_time:.4f}s")
    Path.Log.debug(
        f"surface_stl._shape_to_stl_python: Total Python shape_to_stl_python time: {total_time:.4f}s"
    )
    Path.Log.debug(
        f"surface_stl._shape_to_stl_python: got {len(verts)} vertices, {len(faces)} faces"
    )

    return verts, faces


def shape_to_stl(
    shape,
    linear_deflection,
    angular_deflection,
    timer=None,
    mesh_simplification=1,
    final_depth=None,
):
    """Convert a Part.Shape / Compound to ocl.STLSurf using raw arrays.

    Uses C++ implementation when available for 5-10x performance improvement,
    otherwise falls back to pure Python implementation.

    Args:
        shape: A Part.Shape-like object (TopoShapePy).
        linear_deflection: Linear deflection for tessellation (mm).
        angular_deflection: Angular deflection for tessellation (degrees).
        timer: Optional callable timer(stage_name, elapsed_seconds).
        mesh_simplification: Integer 1-7 for mesh simplification (1=highest accuracy, 7=fastest).
        final_depth: Optional Z-coordinate for pre-clipping above this depth.

    Returns:
        An ocl.STLSurf object.
    """
    import time

    total_start = time.perf_counter()

    # Pre-clipping phase (if enabled)
    clip_start = time.perf_counter()
    original_shape = shape
    shape = _apply_pre_clipping(shape, final_depth)
    clip_time = time.perf_counter() - clip_start

    # Log if clipping was applied
    if shape != original_shape:
        Path.Log.debug(f"Pre-clipping applied: {clip_time:.4f}s")

    # Tessellation phase
    tess_start = time.perf_counter()
    if _HAS_CPP:
        try:
            verts, faces = _shape_to_stl_cpp(shape, linear_deflection, angular_deflection, timer)
        except RuntimeError as e:
            Path.Log.warning(f"C++ shape extraction failed: {e}, falling back to Python")
            verts, faces = _shape_to_stl_python(shape, linear_deflection, angular_deflection, timer)
    else:
        verts, faces = _shape_to_stl_python(shape, linear_deflection, angular_deflection, timer)

    raw_tess_time = time.perf_counter() - tess_start
    Path.Log.debug(f"Raw tessellation time: {raw_tess_time:.4f}s ({len(faces)} triangles)")

    # Mesh simplification phase (if enabled)
    simp_start = time.perf_counter()
    verts, faces = _apply_mesh_simplification(verts, faces, mesh_simplification)
    simp_time = time.perf_counter() - simp_start

    total_tess_time = time.perf_counter() - tess_start
    Path.Log.debug(f"Total tessellation time: {total_tess_time:.4f}s (including simplification)")

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

    Path.Log.debug(f"OCL conversion time: {ocl_time:.4f}s")
    Path.Log.debug(f"Total shape_to_stl time: {total_time:.4f}s")
    Path.Log.debug(
        f"Breakdown: clipping {clip_time/total_time*100:.1f}%, tessellation {total_tess_time/total_time*100:.1f}%, OCL conversion {ocl_time/total_time*100:.1f}%"
    )

    if timer:
        timer("shape_to_stl", total_time)

    return stl


def mesh_to_stl(mesh_points, mesh_facets, timer=None):
    """Convert raw mesh data to ocl.STLSurf.

    This function currently uses the Python implementation as it's typically
    faster when mesh data is already available (no tessellation needed).

    Args:
        mesh_points: Sequence of point-like objects.
        mesh_facets: Sequence of facet index tuples.
        timer: Optional callable timer(stage_name, elapsed_seconds).

    Returns:
        An ocl.STLSurf object.
    """
    from . import surface_common

    return surface_common.mesh_to_stl(mesh_points, mesh_facets, timer)
