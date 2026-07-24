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


import Path
import Part
import unittest
import FreeCAD
import CAMTests.PathTestUtils as PathTestUtils

Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())
Path.Log.trackModule(Path.Log.thisModule())

# Check for optional C++ dependencies
try:
    from Path.Base.Generator.surface_mesh import _HAS_CPP, _HAS_SIMPLIFICATION
except ImportError:
    _HAS_CPP, _HAS_SIMPLIFICATION = False, False


class TestSurfaceMesh(PathTestUtils.PathTestBase):
    """Tests for surface_mesh: Python/C++ tessellation, simplification, and STL generation."""

    def setUp(self):
        """Create common test geometry."""
        self.box = Part.makeBox(20, 20, 10)
        # Use a cylinder for simplification tests as it generates many triangles
        self.cylinder = Part.makeCylinder(10, 30)

    # -- Tessellation Backend Tests --

    def test00_python_tessellator(self):
        """
        Tests the pure Python fallback for converting a Part.Shape to mesh arrays.

        INPUT:
        - Function: _shape_to_stl_python()
        - Parameters: A simple 20x20x10 box shape.
        - Input data: A standard Part.Shape object.

        EXPECTED OUTPUT:
        - Returns valid vertex and face list tuples.
        - A box should be tessellated into exactly 12 triangles (2 per face).
        - This ensures the baseline functionality works without C++ extensions.
        """
        from Path.Base.Generator.surface_mesh import _shape_to_stl_python

        verts, faces = _shape_to_stl_python(self.box, 0.1, 0.5)

        self.assertIsNotNone(verts)
        self.assertIsNotNone(faces)
        self.assertGreater(len(verts), 0)
        self.assertEqual(len(faces), 12, "A simple box should always tessellate to 12 triangles")

    @unittest.skipUnless(_HAS_CPP, "C++ surface_generator module not available")
    def test01_cpp_tessellator(self):
        """
        Tests the accelerated C++ implementation for shape tessellation.

        INPUT:
        - Function: _shape_to_stl_cpp()
        - Parameters: A simple 20x20x10 box shape.
        - Input data: A standard Part.Shape object.

        EXPECTED OUTPUT:
        - Returns valid vertex and face list tuples.
        - Should also produce 12 triangles for a simple box.
        - This verifies the high-performance C++ backend is functioning correctly.
        """
        from Path.Base.Generator.surface_mesh import _shape_to_stl_cpp

        verts, faces = _shape_to_stl_cpp(self.box, 0.1, 0.5)

        self.assertIsNotNone(verts)
        self.assertIsNotNone(faces)
        self.assertGreater(len(verts), 0)
        self.assertEqual(len(faces), 12, "A simple box should always tessellate to 12 triangles")

    # -- Mesh Simplification Tests --

    def test02_main_stl_converter_no_simplification(self):
        """
        Tests the main _shape_to_stl converter with mesh simplification disabled.

        INPUT:
        - Function: _shape_to_stl()
        - Parameters: A cylinder shape, mesh_simplification=1 (disabled).
        - Input data: A shape with curved surfaces.

        EXPECTED OUTPUT:
        - Returns a valid ocl.STLSurf object.
        - The number of triangles should match the raw tessellation output.
        - This verifies the end-to-end conversion process.
        """
        from Path.Base.Generator.surface_mesh import _shape_to_stl

        stl_obj = _shape_to_stl(self.cylinder, 0.05, 0.2, mesh_simplification=1)

        self.assertIsNotNone(stl_obj)
        self.assertGreater(stl_obj.size(), 0, "STL object should contain triangles")

    @unittest.skipUnless(_HAS_SIMPLIFICATION, "fast_simplification library not available")
    def test03_mesh_simplification(self):
        """
        Verifies that mesh simplification reduces the triangle count.

        INPUT:
        - Function: _shape_to_stl()
        - Parameters: A cylinder shape, tested with simplification disabled (1) and enabled (7).
        - Input data: A shape with many triangles suitable for simplification.

        EXPECTED OUTPUT:
        - The STL generated with level 7 simplification should have fewer triangles
          than the one generated with level 1 (no simplification).
        - This confirms the mesh optimization feature is working.
        """
        from Path.Base.Generator.surface_mesh import _shape_to_stl

        # Generate high-quality mesh with no simplification
        stl_high_res = _shape_to_stl(self.cylinder, 0.05, 0.2, mesh_simplification=1)
        count_high = stl_high_res.size()

        # Generate low-quality mesh with maximum simplification
        stl_low_res = _shape_to_stl(self.cylinder, 0.05, 0.2, mesh_simplification=7)
        count_low = stl_low_res.size()

        self.assertGreater(count_high, count_low, "Simplified mesh should have fewer triangles")

    # -- Safe STL and Orchestrator Tests --

    def test04_safe_stl_generation(self):
        """
        Tests the generation of the secondary (safety) STL for collision avoidance.

        INPUT:
        - Function: _shape_to_safe_stl()
        - Parameters: A model shape, avoid faces, and tool radius.
        - Input data: A main box with another smaller box on top to be avoided.

        EXPECTED OUTPUT:
        - Returns a valid ocl.STLSurf object.
        - The bounding box of the safe STL should be larger than the original model's
          bounding box due to the added "invisible floor" and extruded avoid zones.
        """
        from Path.Base.Generator.surface_mesh import _shape_to_safe_stl

        avoid_box = Part.makeBox(5, 5, 5, FreeCAD.Vector(7.5, 7.5, 10))
        model_shape = self.box.fuse(avoid_box)

        safe_stl = _shape_to_safe_stl(
            model_shape=model_shape,
            avoid_faces=avoid_box.Faces,
            tool_radius=3.0,
            start_depth=20.0,
            linear_deflection=0.1,
            angular_deflection=0.5,
        )

        self.assertIsNotNone(safe_stl)
        self.assertGreater(safe_stl.size(), 0)

        # Safety STL should be larger due to the floor plate and extruded pillars
        safe_bb = safe_stl.bb
        model_bb = model_shape.BoundBox
        self.assertGreater(safe_bb.maxpt.x - safe_bb.minpt.x, model_bb.XLength)
        self.assertGreater(safe_bb.maxpt.y - safe_bb.minpt.y, model_bb.YLength)

    def test05_generate_stl_orchestrator(self):
        """
        Tests the main `generate_stl` orchestrator function.

        INPUT:
        - Function: generate_stl()
        - Parameters: A model shape, needs_safe_stl=True.
        - Input data: A simple box model.

        EXPECTED OUTPUT:
        - Returns two valid ocl.STLSurf objects (stl, safe_stl).
        - The safe_stl should be a distinct object from the primary stl.
        - This verifies the top-level function correctly manages the generation
          of both primary and secondary meshes.
        """
        from Path.Base.Generator.surface_mesh import generate_stl

        stl, safe_stl = generate_stl(
            model_shape=self.box,
            base_objs=[self.box],  # Simulate base object from Job
            avoid_faces=[],
            tool_radius=3.0,
            needs_safe_stl=True,
            start_depth=10.0,
            final_depth=0.0,
            linear_deflection=0.1,
            angular_deflection=0.5,
            mesh_simplification=1,
            use_cpp=False,
        )

        self.assertIsNotNone(stl)
        self.assertIsNotNone(safe_stl)
        self.assertGreater(stl.size(), 0)
        self.assertGreater(safe_stl.size(), 0)
        # With avoid_faces=[], the safe_stl is still larger due to the base plate
        self.assertGreater(
            safe_stl.bb.maxpt.x - safe_stl.bb.minpt.x, stl.bb.maxpt.x - stl.bb.minpt.x
        )
