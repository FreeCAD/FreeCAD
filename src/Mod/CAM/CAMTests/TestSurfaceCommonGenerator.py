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
import unittest
import CAMTests.PathTestUtils as PathTestUtils

Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())
Path.Log.trackModule(Path.Log.thisModule())

# Check if OCL is available
_ocl_available = False
try:
    try:
        import ocl

        _ocl_available = True
    except ImportError:
        import opencamlib as ocl

        _ocl_available = True
except ImportError:
    pass


@unittest.skipUnless(_ocl_available, "OpenCamLib not available")
class TestSurfaceCommonGenerator(PathTestUtils.PathTestBase):
    """Tests for surface_common: make_ocl_cutter, shape_to_stl, mesh_to_stl."""

    # -- make_ocl_cutter tests --

    def test00(self):
        """
        Creates a cylindrical end mill cutter from basic tool parameters.

        INPUT:
        - Function: make_ocl_cutter()
        - Parameters: tool_type="endmill", diameter=10.0, edge_height=20.0
        - Input data: Standard end mill specifications for a 10mm cutter

        EXPECTED OUTPUT:
        - Returns an OCL CylCutter object (not None)
        - Cutter diameter should be exactly 10.0mm
        - This is the most common cutter type used in CAM operations
        """
        from Path.Base.Generator.surface_common import make_ocl_cutter

        cutter = make_ocl_cutter("endmill", 10.0, edge_height=20.0)
        self.assertIsNotNone(cutter)
        self.assertRoughly(cutter.getDiameter(), 10.0)

    def test01(self):
        """
        Creates a ball-nose cutter from ball end tool parameters.

        INPUT:
        - Function: make_ocl_cutter()
        - Parameters: tool_type="ballend", diameter=10.0, edge_height=10.0
        - Input data: Ball nose cutter with radius equal to half diameter

        EXPECTED OUTPUT:
        - Returns an OCL BallCutter object (not None)
        - Cutter diameter should be exactly 10.0mm
        - Ball nose cutters are used for smooth 3D surface finishing
        """
        from Path.Base.Generator.surface_common import make_ocl_cutter

        cutter = make_ocl_cutter("ballend", 10.0, edge_height=10.0)
        self.assertIsNotNone(cutter)
        self.assertRoughly(cutter.getDiameter(), 10.0)

    def test02(self):
        """
        Tests that ball cutter automatically sets correct edge height when zero is provided.

        INPUT:
        - Function: make_ocl_cutter()
        - Parameters: tool_type="ballend", diameter=10.0, edge_height=0.0
        - Input data: Ball nose cutter with zero edge height (should auto-correct)

        EXPECTED OUTPUT:
        - Returns an OCL BallCutter object (not None)
        - Function should handle zero edge_height gracefully
        - Ball nose radius should default to diameter/2 = 5.0mm
        """
        from Path.Base.Generator.surface_common import make_ocl_cutter

        cutter = make_ocl_cutter("ballend", 10.0, edge_height=0.0)
        self.assertIsNotNone(cutter)
        self.assertRoughly(cutter.getDiameter(), 10.0)

    def test03(self):
        """
        Creates a bullnose cutter (end mill with corner radius) from tool parameters.

        INPUT:
        - Function: make_ocl_cutter()
        - Parameters: tool_type="bullnose", diameter=10.0, flat_radius=3.0, edge_height=20.0
        - Input data: Bullnose cutter with 3mm corner radius on 10mm diameter

        EXPECTED OUTPUT:
        - Returns an OCL BullCutter object (not None)
        - Cutter diameter should be exactly 10.0mm
        - Bullnose cutters provide stronger corners than sharp end mills
        """
        from Path.Base.Generator.surface_common import make_ocl_cutter

        cutter = make_ocl_cutter("bullnose", 10.0, flat_radius=3.0, edge_height=20.0)
        self.assertIsNotNone(cutter)
        self.assertRoughly(cutter.getDiameter(), 10.0)

    def test04(self):
        """
        Creates a cone-shaped V-bit cutter from engraving tool parameters.

        INPUT:
        - Function: make_ocl_cutter()
        - Parameters: tool_type="v-bit", diameter=10.0, edge_angle=90.0, length_offset=5.0
        - Input data: 90-degree V-bit commonly used for engraving and chamfering

        EXPECTED OUTPUT:
        - Returns an OCL ConeCutter object (not None)
        - V-bit cutters create angled cuts for engraving letters and decorative edges
        """
        from Path.Base.Generator.surface_common import make_ocl_cutter

        cutter = make_ocl_cutter("v-bit", 10.0, edge_angle=90.0, length_offset=5.0)
        self.assertIsNotNone(cutter)

    def test05(self):
        """
        Tests that unsupported tool types are handled gracefully by returning None.

        INPUT:
        - Function: make_ocl_cutter()
        - Parameters: tool_type="chamfer", diameter=10.0, edge_height=20.0
        - Input data: "chamfer" is not a supported OCL cutter type

        EXPECTED OUTPUT:
        - Returns None (not a cutter object)
        - Function should fail gracefully for unsupported tool types
        - Prevents crashes when users specify invalid tool types
        """
        import Path
        from Path.Base.Generator.surface_common import make_ocl_cutter

        # Suppress error logging during this test
        old_level = Path.Log.getLevel()
        Path.Log.setLevel(Path.Log.Level.WARNING)

        try:
            cutter = make_ocl_cutter("chamfer", 10.0, edge_height=20.0)
            self.assertIsNone(cutter)
        finally:
            Path.Log.setLevel(old_level)

    def test06(self):
        """
        Tests that invalid cutter diameters (zero or negative) are rejected.

        INPUT:
        - Function: make_ocl_cutter()
        - Parameters: tool_type="endmill", diameter=0.0 and diameter=-5.0
        - Input data: Physically impossible cutter sizes

        EXPECTED OUTPUT:
        - Returns None for both zero and negative diameters
        - Prevents creation of invalid cutter objects
        - Ensures data validation catches impossible tool dimensions
        """
        import Path
        from Path.Base.Generator.surface_common import make_ocl_cutter

        # Suppress error logging during this test
        old_level = Path.Log.getLevel()
        Path.Log.setLevel(Path.Log.Level.WARNING)

        try:
            cutter = make_ocl_cutter("endmill", 0.0, edge_height=20.0)
            self.assertIsNone(cutter)

            cutter = make_ocl_cutter("endmill", -5.0, edge_height=20.0)
            self.assertIsNone(cutter)
        finally:
            Path.Log.setLevel(old_level)

    def test07(self):
        """
        Tests that V-bit cutters require a valid edge angle to be created.

        INPUT:
        - Function: make_ocl_cutter()
        - Parameters: tool_type="v-bit", diameter=10.0, edge_angle=0.0
        - Input data: V-bit with zero edge angle (physically impossible)

        EXPECTED OUTPUT:
        - Returns None (not a cutter object)
        - V-bit cutters must have a positive angle to cut properly
        - Prevents creation of degenerate cone cutters
        """
        import Path
        from Path.Base.Generator.surface_common import make_ocl_cutter

        # Suppress error logging during this test
        old_level = Path.Log.getLevel()
        Path.Log.setLevel(Path.Log.Level.WARNING)

        try:
            cutter = make_ocl_cutter("v-bit", 10.0, edge_angle=0.0)
            self.assertIsNone(cutter)
        finally:
            Path.Log.setLevel(old_level)

    def test08(self):
        """
        Tests that tool type names are case-insensitive for user convenience.

        INPUT:
        - Function: make_ocl_cutter()
        - Parameters: tool_type="EndMill" and "BALLEND" (mixed case)
        - Input data: Same tool types with different capitalization

        EXPECTED OUTPUT:
        - Returns valid cutter objects for both cases
        - Users shouldn't have to worry about exact capitalization
        - Makes the function more user-friendly and forgiving
        """
        from Path.Base.Generator.surface_common import make_ocl_cutter

        cutter = make_ocl_cutter("EndMill", 10.0, edge_height=20.0)
        self.assertIsNotNone(cutter)

        cutter = make_ocl_cutter("BALLEND", 10.0, edge_height=10.0)
        self.assertIsNotNone(cutter)

    def test09(self):
        """
        Creates a safety oversized cutter for collision detection and clearance.

        INPUT:
        - Functions: make_ocl_cutter() and make_safe_cutter()
        - Parameters: tool_type="endmill", diameter=10.0, edge_height=20.0
        - Input data: Standard end mill specifications

        EXPECTED OUTPUT:
        - Normal cutter: 10.0mm diameter
        - Safe cutter: larger diameter than normal cutter
        - Safe cutters are used to check for collisions without actual cutting
        """
        from Path.Base.Generator.surface_common import make_ocl_cutter, make_safe_cutter

        normal = make_ocl_cutter("endmill", 10.0, edge_height=20.0)
        safe = make_safe_cutter("endmill", 10.0, edge_height=20.0)

        self.assertIsNotNone(normal)
        self.assertIsNotNone(safe)
        self.assertTrue(
            safe.getDiameter() > normal.getDiameter(),
            "Safe cutter should be larger than normal cutter",
        )

    # -- shape_to_stl tests --

    def test10(self):
        """
        Converts a FreeCAD box shape into an OCL STL surface for CAM processing.

        INPUT:
        - Function: shape_to_stl()
        - Parameters: shape=100x100x10mm box, linear_deflection=0.5, angular_deflection=0.5
        - Input data: Simple rectangular solid geometry

        EXPECTED OUTPUT:
        - Returns an OCL STLSurf object (not None)
        - STL should have non-degenerate bounding box
        - STL surfaces are used by OCL algorithms for 3D machining calculations
        """
        import Part
        from Path.Base.Generator.surface_common import shape_to_stl

        box = Part.makeBox(100, 100, 10)
        stl = shape_to_stl(box, 0.5, 0.5)

        self.assertIsNotNone(stl)
        bb = stl.bb
        self.assertTrue(bb.maxpt.x > bb.minpt.x)
        self.assertTrue(bb.maxpt.y > bb.minpt.y)

    def test11(self):
        """
        Tests that shape-to-STL conversion provides timing information for performance monitoring.

        INPUT:
        - Function: shape_to_stl()
        - Parameters: shape=50x50x10mm box, linear_deflection=0.5, angular_deflection=0.5, timer=callback function
        - Input data: Small box with timing callback function

        EXPECTED OUTPUT:
        - Timer callback should be called with "tessellate" and "stl_copy" stages
        - Both timing values should be non-negative numbers
        - Timing information helps users understand performance bottlenecks
        """
        import Part
        from Path.Base.Generator.surface_common import shape_to_stl

        box = Part.makeBox(50, 50, 10)
        timings = {}

        def timer(stage, elapsed):
            timings[stage] = elapsed

        shape_to_stl(box, 0.5, 0.5, timer=timer)

        self.assertIn("tessellate", timings)
        self.assertIn("stl_copy", timings)
        self.assertTrue(timings["tessellate"] >= 0)
        self.assertTrue(timings["stl_copy"] >= 0)

    # -- mesh_to_stl tests --

    def test20(self):
        """
        Converts raw mesh vertex and face data into an OCL STL surface.

        INPUT:
        - Function: mesh_to_stl()
        - Parameters: points=[4 corner vertices], facets=[2 triangular faces]
        - Input data: Simple square mesh made of two triangles

        EXPECTED OUTPUT:
        - Returns an OCL STLSurf object (not None)
        - Allows direct mesh-to-STL conversion without FreeCAD shapes
        - Useful for importing mesh data from external sources
        """
        from Path.Base.Generator.surface_common import mesh_to_stl

        points = [
            (0, 0, 0),
            (10, 0, 0),
            (10, 10, 0),
            (0, 10, 0),
        ]
        facets = [
            (0, 1, 2),
            (0, 2, 3),
        ]

        stl = mesh_to_stl(points, facets)
        self.assertIsNotNone(stl)
