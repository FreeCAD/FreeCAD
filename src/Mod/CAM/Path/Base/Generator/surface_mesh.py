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

# Try to import PyVista for high-quality CAD mesh decimation
try:
    import pyvista as pv
    import numpy as np

    _HAS_SIMPLIFICATION = True
    Path.Log.info("Using PyVista for high-quality mesh optimization")
except ImportError:
    _HAS_SIMPLIFICATION = False
    Path.Log.info("PyVista not available, mesh optimization disabled. (pip install pyvista)")

# Try to import C++ implementation
try:
    # Import the compiled C++ extension module
    import surface_generator as _stl_cpp

    _HAS_CPP = True
    Path.Log.info("surface_mesh: Using C++ accelerated implementation")
except ImportError as e:
    _HAS_CPP = False
    Path.Log.info(f"C++ not available ({e}), using Python fallback")
except Exception as e:
    _HAS_CPP = False
    Path.Log.info(f"C++ import error ({e}), using Python fallback")

if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


def _apply_mesh_simplification(vertices, facets, simplification_level, silence):
    """Apply topology-preserving mesh simplification to reduce triangle count.

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
    reduction = 0.75  # 75% High reduction ratio at low angle to terget only flat surfaces
    angle_level = {
        1: 0.0,  # No reduction (Highest accuracy)
        2: 1.0,  # 1 Degree (Extremely safe: merges ONLY perfectly flat surfaces)
        3: 2.0,  # 2 Degrees
        4: 3.0,  # 3 Degrees
        5: 4.0,  # 4 Degrees
        6: 5.0,  # 5 Degrees
        7: 6.0,  # 6 Degrees (Fastest processing, might flatten very shallow curves)
    }

    target_angle = angle_level.get(simplification_level, 0.0)
    if target_angle < 1.0:
        return vertices, facets

    start_time = time.perf_counter()
    original_triangles = len(facets)

    try:
        # PyVista expects faces as a flat array: [3, v0, v1, v2, 3, v0, v1, v2...]
        faces_pv = np.empty((len(facets), 4), dtype=int)
        faces_pv[:, 0] = 3
        faces_pv[:, 1:] = facets
        faces_flat = faces_pv.flatten()

        # Build the PyVista PolyData object
        mesh = pv.PolyData(np.array(vertices), faces_flat)

        # Apply decimate_pro: This is the VTK algorithm that respects CAD topology (preserve details)
        simplified_mesh = mesh.decimate_pro(
            reduction,
            preserve_topology=True,
            feature_angle=target_angle,  # Protects sharp corners (like walls and chamfers)
            splitting=False,
            boundary_vertex_deletion=False,
        )

        # Extract the results back into standard Python lists
        simplified_vertices = simplified_mesh.points.tolist()

        # Unpack from VTK's [3, v1, v2, v3] format back to [[v1, v2, v3]]
        sf = simplified_mesh.faces
        simplified_facets = sf.reshape(-1, 4)[:, 1:].tolist()

        simplification_time = time.perf_counter() - start_time
        final_triangles = len(simplified_facets)
        actual_reduction = (original_triangles - final_triangles) / original_triangles * 100

        if not silence:
            Path.Log.info(
                f"PyVista simplification level {simplification_level}: "
                f"{original_triangles} → {final_triangles} triangles "
                f"({actual_reduction:.1f}% reduction, {simplification_time:.3f}s)"
            )

        return simplified_vertices, simplified_facets

    except Exception as e:
        Path.Log.warning(f"PyVista mesh simplification failed: {e}, using original mesh")
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
    shape,
    linear_deflection,
    angular_deflection,
    mesh_simplification=1,
    silence=False,
):
    """Convert a Part.Shape / Compound to ocl.STLSurf using raw arrays.

    Uses C++ implementation when available for 5-10x performance improvement,
    otherwise falls back to pure Python implementation.

    Args:
        shape: A Part.Shape-like object (TopoShapePy).
        linear_deflection: Linear deflection for tessellation (mm).
        angular_deflection: Angular deflection for tessellation (degrees).
        mesh_simplification: Integer 1-7 for mesh simplification (1=highest accuracy, 7=fastest).

    Returns:
        An ocl.STLSurf object.
    """

    total_start = time.perf_counter()

    # Validate Shape
    if not hasattr(shape, "ShapeType"):
        if hasattr(shape, "Shape"):
            shape = shape.Shape
        else:
            raise ValueError("Expected Part.Shape-like object or object with Shape property")

    Path.Log.debug(
        f"surface_mesh._shape_to_stl: shape type={type(shape)}, ShapeType={getattr(shape, 'ShapeType', 'N/A')}"
    )
    Path.Log.debug(
        f"surface_mesh._shape_to_stl: deflection params linear={linear_deflection}, angular={angular_deflection}"
    )

    # Tessellation phase
    tess_start = time.perf_counter()
    if _HAS_CPP:
        try:
            verts, faces = _shape_to_stl_cpp(shape, linear_deflection, angular_deflection)
        except RuntimeError as e:
            Path.Log.warning(
                f"High-speed mesh generation failed. Falling back to the slower standard method. (Error: {e})"
            )
            verts, faces = _shape_to_stl_python(shape, linear_deflection, angular_deflection)
    else:
        verts, faces = _shape_to_stl_python(shape, linear_deflection, angular_deflection)

    raw_tess_time = time.perf_counter() - tess_start
    Path.Log.debug(
        f"surface_mesh._shape_to_stl: Raw tessellation time: {raw_tess_time:.4f}s ({len(faces)} triangles)"
    )

    # Mesh simplification phase (if enabled)
    simp_start = time.perf_counter()
    verts, faces = _apply_mesh_simplification(verts, faces, mesh_simplification, silence)
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
    from . import surface_common

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


def _clip_model(model_shape, bbox, final_depth, clip_buffer=0.1):
    """
    Clips the Job's model geometry below the operation's final depth to reduce
    the computational load during mesh generation.

    Args:
        model_shape (Part.Shape): The mathematically fused solid of the entire Job model.
        bbox (Base.BoundBox): The bounding box of the model.
        final_depth (float): The lower Z-bound of the operation.
        clip_buffer (float): A safety buffer added to the clipping plane.

    Returns:
        Part.Shape: The clipped model shape, or the original model if clipping fails.
    """
    clipped_shape = None
    padding = 1.0

    try:
        clipper_box = Part.makeBox(
            bbox.XLength + padding * 2,
            bbox.YLength + padding * 2,
            bbox.ZMax - (final_depth - clip_buffer) + padding,
            FreeCAD.Vector(bbox.XMin - padding, bbox.YMin - padding, final_depth - clip_buffer),
        )

        clipped_shape = model_shape.common(clipper_box)
    except Exception as e:
        Path.Log.warning(
            f"Failed to create clipping boundary. Check your Job Origin and Depths. "
            f"Using original full model. (Error: {e})"
        )
        return model_shape

    if not clipped_shape or clipped_shape.isNull():
        Path.Log.warning(
            "Pre-clipping the machining shape resulted in an empty shape. Using original full model."
        )
        return model_shape

    return clipped_shape


def _face_fingerprint(face, precision=4):
    """
    Builds a lightweight, hashable fingerprint for a Part.Face, used to
    match faces across two independently-obtained shapes (e.g. a face
    selection made before a boolean fuse, checked against the fused
    result afterward) where Python object identity can't be relied on --
    FreeCAD's `.Faces` property returns freshly-built wrapper objects on
    every access, and a fuse additionally produces a genuinely new shape
    with no guaranteed object correspondence to its inputs.

    Returns:
        tuple: (rounded_area, rounded_x, rounded_y, rounded_z)
    """
    com = face.CenterOfMass
    return (
        round(face.Area, precision),
        round(com.x, precision),
        round(com.y, precision),
        round(com.z, precision),
    )


def _model_optimization(
    strategy,
    shape,
    bb_face=None,
    exempt_faces=None,
    stl_filter_adj=0.0,
    tool_diam=0.0,
    final_depth=0.0,
    normal_tolerance=0.01,
):
    """
    Filters the model's faces based on specific criteria to minimize the
    computational load during mesh generation.

    Args:
        strategy (str): The selected operation strategy ('SurfaceScan' or 'Waterline').
        shape (Part.Shape): The mathematically fused solid of the entire Job model.
        bb_face (Part.Face): The bounding box of the selected faces.
        exempt_faces (list): A list of Part.Face objects explicitly selected to be machined.
        stl_filter_adj (float): A positive offset value for the boundary adjustment of the face filter.
        tool_diam (float): The diameter of the active tool.
        final_depth (float): The lower Z-bound of the operation.
        normal_tolerance (float): Tolerance for filtering vertical faces.

    Returns:
        Part.Compound or Part.Shape: A compound of the filtered faces, or the original shape if no faces are filtered.
    """
    from . import surface_common

    # Detect pre-triangulated models and skip optimization
    if not exempt_faces and surface_common._is_triangulated_mesh(shape.Faces):
        Path.Log.debug(
            "surface_mesh._model_optimization: Pre-triangulated model detected. Skipping face optimization."
        )
        return shape

    filtered = []
    rejected = 0
    clip_bb = None
    exempt_set = None

    # Build exempt set by geometric fingerprint, not object identity
    if exempt_faces:
        exempt_set = {_face_fingerprint(f) for f in exempt_faces}

        if bb_face is not None and tool_diam > 0 and strategy == "SurfaceScan":
            ba = stl_filter_adj
            bb = bb_face.BoundBox
            clip_bb = {
                "XMin": bb.XMin - ba,
                "XMax": bb.XMax + ba,
                "YMin": bb.YMin - ba,
                "YMax": bb.YMax + ba,
            }

    for face in shape.Faces:
        try:
            # SurfaceScan strategy with face selection
            # Exempt faces are always kept
            if exempt_set and _face_fingerprint(face) in exempt_set:
                filtered.append(face)
                continue

            # Reject faces below final depth
            if face.BoundBox.ZMax < final_depth - 0.1:  # Plus a small buffer
                rejected += 1
                continue

            # Reject faces outside of the selection boundary
            if clip_bb:
                bb = face.BoundBox
                if (
                    bb.XMax < clip_bb["XMin"]
                    or bb.XMin > clip_bb["XMax"]
                    or bb.YMax < clip_bb["YMin"]
                    or bb.YMin > clip_bb["YMax"]
                ):
                    rejected += 1
                    continue

            u1, u2, v1, v2 = face.ParameterRange
            norm = face.normalAt((u1 + u2) / 2.0, (v1 + v2) / 2.0)
            if face.Orientation == "Reversed":
                norm = norm.multiply(-1)

            normal_z = abs(norm.z)

            # Reject truly vertical faces
            if normal_z < normal_tolerance:
                rejected += 1
                continue

            filtered.append(face)

        except Exception as e:
            Path.Log.debug(
                f"surface_mesh._filter_selected_faces: Face check failed — keeping it. {e}"
            )
            filtered.append(face)

    # Nothing filtered, return original
    if not filtered:
        return shape

    Path.Log.debug(
        f"surface_mesh._filter_selected_faces: "
        f"Kept {len(filtered)} faces, rejected {rejected} "
        f"(vertical or outside boundary)."
    )

    # All filtered! Return original
    if len(filtered) == len(shape.Faces):
        return shape

    return Part.makeCompound(filtered)


def _shape_to_safe_stl(
    model_shape,
    bb_safe,
    pad_buffer,
    final_depth,
    avoid_boundary,
    start_depth,
    linear_deflection,
    angular_deflection,
    mesh_simplification,
):
    """
    Generates the secondary (safety) STL mesh for collision avoidance.

    This function creates a robust "keep-out zones" for any avoided faces.

    Args:
        model_shape (Part.Shape): The complete, un-clipped model geometry.
        bb_safe (Part.Face): The bounding box of the model or selected faces.
        pad_buffer (float): The calculated outward offset for the safety pad.
        final_depth (float): The lower Z-bound of the operation.
        avoid_boundary (Part.Shape, optional): Pre-built Avoid Faces "keep-out" boundary.
        start_depth (float): The upper Z-bound of the operation.
        linear_deflection (float): The base linear deflection for calculating a coarse mesh.
        angular_deflection (float): The base angular deflection for calculating a coarse mesh.
        mesh_simplification (int): The user-set simplification level for the primary mesh.

    Returns:
        ocl.STLSurf: The generated safety mesh, or None on failure.
    """
    fused_shapes = []
    bb = bb_safe.BoundBox

    fused_shapes.append(model_shape)

    # Create a pad face at the bottom of the original bounding box
    try:
        p1 = FreeCAD.Vector(bb.XMin - pad_buffer, bb.YMin - pad_buffer, final_depth)
        p2 = FreeCAD.Vector(bb.XMax + pad_buffer, bb.YMin - pad_buffer, final_depth)
        p3 = FreeCAD.Vector(bb.XMax + pad_buffer, bb.YMax + pad_buffer, final_depth)
        p4 = FreeCAD.Vector(bb.XMin - pad_buffer, bb.YMax + pad_buffer, final_depth)

        pad_wire = Part.makePolygon([p1, p2, p3, p4, p1])
        pad_face = Part.Face(pad_wire)
        fused_shapes.append(pad_face)
        Path.Log.debug("surface_mesh._shape_to_safe_stl: Appended bottom pad face to safe STL.")
    except Exception as e:
        Path.Log.warning(f"Failed to create bottom pad face for safe STL: {e}")

    # Fuse in the "Keep-Out Pillar" for the pre-built avoid boundary, if any
    if avoid_boundary:
        Path.Log.debug(
            "surface_mesh._shape_to_safe_stl: Fusing precomputed avoid-zone boundary into safe STL."
        )
        try:
            avoid = Part.makeFace(avoid_boundary)
            avoid.translate(FreeCAD.Vector(0, 0, start_depth + 0.1))
            fused_shapes.append(avoid)
        except Exception as e:
            Path.Log.error(f"Generating avoid zones for avoided faces failed: {e}")

    # Fuse and create a coarse mesh
    safe_compound = Part.Compound(fused_shapes)

    if _HAS_SIMPLIFICATION:
        # PyVista will intelligently decimate flat areas, so we can afford
        # a higher resolution base mesh to capture perfect boundary edges.
        safe_lin_def = linear_deflection
        safe_ang_def = angular_deflection
    else:
        # No PyVista: We must manually loosen the deflection to reduce
        # triangle count and prevent the boundary calculations from lagging.
        safe_lin_def = linear_deflection + 0.02
        safe_ang_def = angular_deflection + 0.1

    try:
        safe_stl = _shape_to_stl(
            safe_compound,
            safe_lin_def,
            safe_ang_def,
            max(mesh_simplification, 2),
            silence=True,
        )
        Path.Log.debug("surface_mesh._shape_to_safe_stl: Safe STL generated successfully.")
    except Exception as e:
        Path.Log.error(
            "Could not generate the safety collision mesh."
            f"WARNING: Rapid transitions and Smart Lead-In/Out moves may crash into the part! (Error: {e})"
        )
        return None

    return safe_stl


def generate_stl(
    model_shape,
    base_objs,
    optimize_stl,
    strategy,
    stl_faces,
    stl_filter_adj,
    bb_face,
    avoid_boundary,
    tool_diam,
    needs_safe_stl,
    boundary_adjustment,
    start_depth,
    final_depth,
    linear_deflection,
    angular_deflection,
    mesh_simplification,
):
    """
    Orchestrates the creation of the primary (machining) and secondary (safety) STL meshes.

    This function acts as a high-level controller. It generates the primary STL from
    the appropriate geometry (selected faces or full model) and then, if required,
    delegates the creation of the complex safety STL to the _shape_to_safe_stl helper.

    Args:
        model_shape (Part.Shape): The mathematically fused solid of the entire Job model.
        base_objs (list): The source geometric objects from the Job (can be Part or Mesh).
        optimize_stl (bool): Flag indicating if STL optimization is enabled.
        strategy (str): The selected strategy of the operation, SurfaceScan or Waterline.
        stl_faces (list): A list of Part.Face objects to be machined.
        stl_filter_adj (float): A positive offset value for the boundary adjustment of the STL face filter.
        bb_face: (Part.Face): The BoundBox of the selected faces.
        avoid_boundary (Part.Shape, optional): Pre-built Avoid Faces "keep-out" boundary.
        tool_diam (float): The diameter of the active tool.
        needs_safe_stl (bool): Flag indicating if the safety model is required.
        boundary_adjustment (float): A positive or negative value of the boundary adjustment.
        start_depth (float): The upper Z-bound of the operation.
        final_depth (float): The lower Z-bound of the operation.
        linear_deflection (float): The user-set linear deflection for the primary mesh.
        angular_deflection (float): The user-set angular deflection for the primary mesh.
        mesh_simplification (int): The user-set simplification level for the primary mesh.

    Returns:
        tuple: (stl, safe_stl), where stl is the primary mesh and safe_stl is the
               collision mesh (or a copy of stl if generation failed or wasn't needed).
    """
    stl = safe_stl = clipped_shape = optimized_shape = None

    if not base_objs:
        Path.Log.error(
            "No 3D models were found in the Job. Please add a base model to the Job setup."
        )
        return None, None

    # Dispatch based on geometry type
    is_mesh_op = hasattr(base_objs[0], "TypeId") and base_objs[0].TypeId.startswith("Mesh")

    if is_mesh_op:
        Path.Log.debug(
            "surface_mesh.generate_stl. Mesh object detected as Base. Using direct mesh conversion."
        )
        stl = _mesh_to_stl(base_objs[0])
        if stl is None:
            Path.Log.error("Could not create a valid shape for primary STL generation.")
            return None, None

        return stl, stl
    else:
        # Generate the primary machining STL
        if not model_shape or model_shape.isNull():
            Path.Log.error("Could not create a valid shape for primary STL generation.")
            return None, None

        # STL optimization - pre-process Model
        if optimize_stl:
            optimized_shape = _model_optimization(
                strategy,
                model_shape,
                bb_face,
                stl_faces,
                stl_filter_adj,
                tool_diam,
                final_depth,
            )
            if optimized_shape and not optimized_shape.isNull():
                model_shape = optimized_shape

        # Pre-clip the full model shape to the final depth
        clip_buffer = 0.1
        bbox = model_shape.BoundBox

        if final_depth > bbox.ZMin + clip_buffer and not optimize_stl:
            clipped_shape = _clip_model(model_shape, bbox, final_depth, clip_buffer)

        if clipped_shape and not clipped_shape.isNull():
            model_shape = clipped_shape

        # Generate the primary STL
        stl = _shape_to_stl(
            model_shape,
            linear_deflection,
            angular_deflection,
            mesh_simplification,
            silence=False,
        )

        # Check if the STL object is None OR if it contains zero triangles.
        if stl is None or stl.size() == 0:
            Path.Log.debug(
                "surface_mesh.generate_stl.Failed to create a valid STL from the model (mesh is empty)."
            )
            return None, None

        # Generate the Safe STL
        if needs_safe_stl:

            if optimize_stl and stl_faces:
                bb_safe = bb_face
                pad_buffer = stl_filter_adj + 0.1
            else:
                bb_safe = model_shape
                pad_buffer = boundary_adjustment + 0.1

            safe_stl = _shape_to_safe_stl(
                model_shape,
                bb_safe,
                pad_buffer,
                final_depth,
                avoid_boundary,
                start_depth,
                linear_deflection,
                angular_deflection,
                mesh_simplification=max(mesh_simplification, 2),
            )

        if safe_stl is None:
            safe_stl = stl

        return stl, safe_stl
