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

import FreeCAD
import Part
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


def _make_hemisphere_stl_and_cutter():
    """Create a hemisphere STL and ball-nose cutter for waterline testing.

    A hemisphere provides non-trivial waterline contours at each Z-height.
    """
    from Path.Base.Generator.surface_common import shape_to_stl, make_ocl_cutter

    # Create a hemisphere: sphere cut by a box below Z=0
    sphere = Part.makeSphere(25.0)
    box = Part.makeBox(60, 60, 30, FreeCAD.Vector(-30, -30, -30))
    hemisphere = sphere.cut(box)

    stl = shape_to_stl(hemisphere, 0.2, 0.2)
    cutter = make_ocl_cutter("ballend", 6.0, edge_height=10.0)
    return stl, cutter


class TestSurfaceWaterlineGenerator(PathTestUtils.PathTestBase):
    """Tests for surface_waterline: waterline, adaptive, stack, slice, gcode."""

    # -- waterline tests (OCL required) --

    @unittest.skipUnless(_ocl_available, "OpenCamLib not available")
    def test00(self):
        """
        Calculates waterline contours at a specific Z-height on a hemisphere surface.

        INPUT:
        - Function: waterline()
        - Parameters: stl=25mm radius hemisphere, cutter=6mm ballnose, tolerance=0.5, z_height=10.0
        - Input data: Hemisphere sliced at Z=10mm (below the equator)

        EXPECTED OUTPUT:
        - Returns one or more closed contour loops
        - At least one loop should have more than 3 points (meaningful contour)
        - All points should be approximately at Z=10.0mm
        - Waterline creates horizontal cutting paths for 3D finishing
        """
        from Path.Base.Generator.surface_waterline import waterline

        stl, cutter = _make_hemisphere_stl_and_cutter()
        loops = waterline(stl, cutter, 0.5, 10.0)

        self.assertTrue(len(loops) > 0)
        # Count loops with meaningful number of points
        meaningful_loops = [loop for loop in loops if len(loop) > 3]
        self.assertTrue(len(meaningful_loops) > 0, "Should have at least one loop with >3 points")

        for i, loop in enumerate(loops):
            print(f"Loop {i}: {len(loop)} points")
            if len(loop) > 0:
                print(f"  First point: {loop[0]}")
                print(f"  Last point: {loop[-1]}")
        for loop in loops:
            for pt in loop:
                self.assertRoughly(pt[2], 10.0, error=1.0)

    @unittest.skipUnless(_ocl_available, "OpenCamLib not available")
    def test01(self):
        """waterline at Z above the hemisphere produces no loops."""
        from Path.Base.Generator.surface_waterline import waterline

        stl, cutter = _make_hemisphere_stl_and_cutter()
        loops = waterline(stl, cutter, 0.5, 30.0)
        self.assertEqual(len(loops), 0)

    # -- adaptive_waterline tests (OCL required) --

    @unittest.skipUnless(_ocl_available, "OpenCamLib not available")
    def test10(self):
        """adaptive_waterline produces loops at a Z-height on a hemisphere."""
        from Path.Base.Generator.surface_waterline import adaptive_waterline

        stl, cutter = _make_hemisphere_stl_and_cutter()
        loops = adaptive_waterline(stl, cutter, 0.5, 10.0)

        self.assertTrue(len(loops) > 0)
        # Count loops with meaningful number of points
        meaningful_loops = [loop for loop in loops if len(loop) > 3]
        self.assertTrue(len(meaningful_loops) > 0, "Should have at least one loop with >3 points")

    # -- waterline_stack tests (OCL required) --

    @unittest.skipUnless(_ocl_available, "OpenCamLib not available")
    def test20(self):
        """
        Generates waterline contours at multiple Z-heights for layered 3D finishing.

        INPUT:
        - Function: waterline_stack()
        - Parameters: stl=25mm hemisphere, cutter=6mm ballnose, tolerance=0.5, min_z=2.0, max_z=20.0, step_down=5.0
        - Input data: Hemisphere with 5mm vertical spacing between waterline layers

        EXPECTED OUTPUT:
        - Returns dictionary with Z-height keys and contour loop values
        - Should produce contours at multiple heights (2.0, 7.0, 12.0, 17.0)
        - Z-heights should be in decreasing order (top to bottom)
        - Creates stacked horizontal cutting paths for efficient 3D machining
        """
        from Path.Base.Generator.surface_waterline import waterline_stack

        stl, cutter = _make_hemisphere_stl_and_cutter()
        result = waterline_stack(
            stl,
            cutter,
            0.5,
            min_z=2.0,
            max_z=20.0,
            step_down=5.0,
        )

        self.assertTrue(
            len(result) > 0,
            "waterline_stack should produce results at multiple heights",
        )
        # Z-heights should be decreasing
        heights = list(result.keys())
        for i in range(1, len(heights)):
            self.assertTrue(
                heights[i] < heights[i - 1],
                "Z-heights should be in decreasing order",
            )

    @unittest.skipUnless(_ocl_available, "OpenCamLib not available")
    def test21(self):
        """waterline_stack applies depth_offset."""
        from Path.Base.Generator.surface_waterline import waterline_stack

        stl, cutter = _make_hemisphere_stl_and_cutter()
        offset = -0.5
        result = waterline_stack(
            stl,
            cutter,
            0.5,
            min_z=5.0,
            max_z=15.0,
            step_down=5.0,
            depth_offset=offset,
        )
        for z_height, loops in result.items():
            for loop in loops:
                for pt in loop:
                    self.assertRoughly(pt[2], z_height + offset, error=1.0)

    @unittest.skipUnless(_ocl_available, "OpenCamLib not available")
    def test22(self):
        """waterline_stack with adaptive=True uses AdaptiveWaterline."""
        from Path.Base.Generator.surface_waterline import waterline_stack

        stl, cutter = _make_hemisphere_stl_and_cutter()
        result = waterline_stack(
            stl,
            cutter,
            0.5,
            min_z=5.0,
            max_z=15.0,
            step_down=5.0,
            adaptive=True,
        )
        self.assertTrue(len(result) > 0)

    # -- slice_waterline tests (no OCL required) --

    def test30(self):
        """slice_waterline extracts contours from a hemisphere using shape.slice()."""
        from Path.Base.Generator.surface_waterline import slice_waterline

        sphere = Part.makeSphere(25.0)
        box = Part.makeBox(60, 60, 30, FreeCAD.Vector(-30, -30, -30))
        hemisphere = sphere.cut(box)

        result = slice_waterline(hemisphere, 2.0, 20.0, 5.0)

        self.assertTrue(len(result) > 0)
        for z_height, loops in result.items():
            print(f"Z={z_height}: {len(loops)} loops")
            for i, loop in enumerate(loops):
                print(f"  Loop {i}: {len(loop)} points")
                if len(loop) > 0:
                    print(f"    First point: {loop[0]}")
            self.assertTrue(len(loops) > 0)
            # For a hemisphere, slice_waterline may produce single points or small loops
            # The important thing is that it produces some output at the expected Z heights
            total_points = sum(len(loop) for loop in loops)
            self.assertTrue(total_points > 0, f"Z={z_height} should have some points")

    def test31(self):
        """slice_waterline above the shape produces no results."""
        from Path.Base.Generator.surface_waterline import slice_waterline

        box = Part.makeBox(10, 10, 10)
        result = slice_waterline(box, 15.0, 20.0, 1.0)
        self.assertEqual(len(result), 0)

    # -- waterline_to_gcode tests (OCL required) --

    @unittest.skipUnless(_ocl_available, "OpenCamLib not available")
    def test40(self):
        """
        Converts waterline contour data into G-code with proper feeds and heights.

        INPUT:
        - Function: waterline_to_gcode()
        - Parameters: wl_data=multi-height waterline contours, horiz_feed=500, vert_rapid=1000, horiz_rapid=2000, safe_z=50, clearance_z=60
        - Input data: Waterline contours at Z=5.0, 10.0, 15.0 from hemisphere

        EXPECTED OUTPUT:
        - Should contain both G0 rapid moves and G1 cutting moves
        - G0 moves for positioning between contour levels
        - G1 moves for following the waterline contours
        - Proper feed rates applied to different move types
        """
        from Path.Base.Generator.surface_waterline import waterline_stack, waterline_to_gcode

        stl, cutter = _make_hemisphere_stl_and_cutter()
        wl_data = waterline_stack(
            stl,
            cutter,
            0.5,
            min_z=5.0,
            max_z=15.0,
            step_down=5.0,
        )
        cmds = waterline_to_gcode(wl_data, 500.0, 1000.0, 2000.0, 50.0, 60.0)

        self.assertTrue(len(cmds) > 0)
        g0_cmds = [c for c in cmds if c.Name == "G0"]
        g1_cmds = [c for c in cmds if c.Name == "G1"]
        self.assertTrue(len(g0_cmds) > 0)
        self.assertTrue(len(g1_cmds) > 0)

    @unittest.skipUnless(_ocl_available, "OpenCamLib not available")
    def test41(self):
        """waterline_to_gcode with cut_climb reverses loop direction."""
        from collections import OrderedDict
        from Path.Base.Generator.surface_waterline import waterline_to_gcode

        wl_data = OrderedDict()
        loop = [(0.0, 0.0, 5.0), (10.0, 0.0, 5.0), (10.0, 10.0, 5.0), (0.0, 0.0, 5.0)]
        wl_data[5.0] = [loop]

        cmds_conv = waterline_to_gcode(wl_data, 500, 1000, 2000, 50, 60, cut_climb=False)
        cmds_climb = waterline_to_gcode(wl_data, 500, 1000, 2000, 50, 60, cut_climb=True)

        g1_conv = [c.Parameters.get("X") for c in cmds_conv if c.Name == "G1"]
        g1_climb = [c.Parameters.get("X") for c in cmds_climb if c.Name == "G1"]

        self.assertEqual(len(g1_conv), len(g1_climb))
        self.assertEqual(g1_conv, list(reversed(g1_climb)))
