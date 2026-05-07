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
from collections import OrderedDict
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
    from Path.Base.Generator.surface_mesh import _shape_to_stl
    from Path.Base.Generator.surface_common import make_ocl_cutter

    # Create a hemisphere: sphere with its base at Z=0
    sphere = Part.makeSphere(25.0)
    box = Part.makeBox(60, 60, 30, FreeCAD.Vector(-30, -30, -30))
    hemisphere = sphere.cut(box)

    # Note: Waterline currently requires the Python STL converter
    stl = _shape_to_stl(hemisphere, 0.2, 0.2, use_cpp=False)
    cutter = make_ocl_cutter("ballend", 6.0, edge_height=10.0)
    return stl, cutter


@unittest.skipUnless(_ocl_available, "OpenCamLib not available")
class TestSurfaceWaterline(PathTestUtils.PathTestBase):
    """Tests for surface_waterline: waterline, adaptive, stack, and gcode generation."""

    # -- waterline tests --

    def test00_waterline_single_level(self):
        """
        Calculates waterline contours at a specific Z-height on a hemisphere surface.

        INPUT:
        - Function: waterline()
        - Parameters: stl=25mm radius hemisphere, cutter=6mm ballnose, sampling=1.0, z_height=10.0
        - Input data: Hemisphere sliced at Z=10mm (below the equator)

        EXPECTED OUTPUT:
        - Returns one or more closed contour loops.
        - All points in the loops should be approximately at Z=10.0mm.
        - Waterline creates horizontal cutting paths for 3D finishing.
        """
        from Path.Base.Generator.surface_waterline import waterline

        stl, cutter = _make_hemisphere_stl_and_cutter()
        loops = waterline(stl, cutter, sampling=1.0, z=10.0)

        self.assertGreater(len(loops), 0, "Waterline should produce at least one loop")
        for loop in loops:
            # A valid path segment must have at least 2 points.
            self.assertGreaterEqual(
                len(loop), 2, "A valid contour loop should have at least 2 points"
            )
            for pt in loop:
                self.assertAlmostEqual(pt[2], 10.0, delta=0.01)

    def test01_waterline_above_part(self):
        """
        Verifies that running waterline at a Z-height above the model produces no geometry.

        INPUT:
        - Function: waterline()
        - Parameters: z_height=30.0 (hemisphere top is at Z=25).
        - Input data: A Z-level that does not intersect the model.

        EXPECTED OUTPUT:
        - Returns an empty list of loops.
        - The algorithm correctly handles cases with no intersections.
        """
        from Path.Base.Generator.surface_waterline import waterline

        stl, cutter = _make_hemisphere_stl_and_cutter()
        loops = waterline(stl, cutter, sampling=1.0, z=30.0)
        self.assertEqual(len(loops), 0)

    # -- adaptive_waterline tests --

    def test10_adaptive_waterline_single_level(self):
        """
        Tests the adaptive waterline algorithm at a single Z-height.

        INPUT:
        - Function: adaptive_waterline()
        - Parameters: Hemisphere STL, z_height=15.0.
        - Input data: A curved surface ideal for adaptive sampling.

        EXPECTED OUTPUT:
        - Returns one or more valid contour loops at the correct Z-height.
        - This confirms the adaptive variant of the algorithm is functional.
        """
        from Path.Base.Generator.surface_waterline import adaptive_waterline

        stl, cutter = _make_hemisphere_stl_and_cutter()
        loops = adaptive_waterline(stl, cutter, sampling=2.0, min_sampling=0.5, z=15.0)

        self.assertGreater(len(loops), 0, "Adaptive waterline should produce loops")
        for loop in loops:
            self.assertGreater(len(loop), 3)
            for pt in loop:
                self.assertAlmostEqual(pt[2], 15.0, delta=0.01)

    # -- waterline_stack tests --

    def test20_waterline_stack_multiple_levels(self):
        """
        Generates waterline contours at multiple Z-heights for layered 3D finishing.

        INPUT:
        - Function: waterline_stack()
        - Parameters: min_z=5.0, max_z=20.0, step_down=5.0
        - Input data: Hemisphere with 5mm vertical spacing between waterline layers.

        EXPECTED OUTPUT:
        - Returns a dictionary with Z-height keys and contour loop values.
        - Should produce contours at 4 levels: 20, 15, 10, and 5.
        - Z-heights in the dictionary should be in decreasing order (top to bottom).
        """
        from Path.Base.Generator.surface_waterline import waterline_stack

        stl, cutter = _make_hemisphere_stl_and_cutter()
        result = waterline_stack(
            stl, cutter, sampling=2.0, min_sampling=0.5, min_z=5.0, max_z=20.0, step_down=5.0
        )

        self.assertEqual(len(result), 4, "Expected 4 Z-levels in the stack")
        heights = list(result.keys())
        self.assertAlmostEqual(heights[0], 20.0, delta=0.01)
        self.assertAlmostEqual(heights[-1], 5.0, delta=0.01)
        self.assertTrue(all(heights[i] > heights[i + 1] for i in range(len(heights) - 1)))

    def test21_waterline_stack_with_offset(self):
        """
        Verifies that the `depth_offset` parameter is correctly applied to all points.

        INPUT:
        - Function: waterline_stack()
        - Parameters: depth_offset=-0.2
        - Input data: A single Z-level for simplicity.

        EXPECTED OUTPUT:
        - The Z-coordinate of every point in the output loops should be the
          calculated Z-height plus the specified offset.
        """
        from Path.Base.Generator.surface_waterline import waterline_stack

        stl, cutter = _make_hemisphere_stl_and_cutter()
        offset = -0.2
        result = waterline_stack(
            stl,
            cutter,
            sampling=2.0,
            min_sampling=0.5,
            min_z=10.0,
            max_z=10.0,
            step_down=1.0,
            depth_offset=offset,
        )

        self.assertEqual(len(result), 1)
        z_height = list(result.keys())[0]
        loops = result[z_height]
        for loop in loops:
            for pt in loop:
                self.assertAlmostEqual(pt[2], z_height + offset, delta=0.01)

    # -- waterline_to_gcode tests --

    def test30_gcode_conversion(self):
        """
        Converts multi-level waterline contour data into a G-code command list.

        INPUT:
        - Function: waterline_to_gcode()
        - Input data: A dictionary of waterline loops from `waterline_stack`.

        EXPECTED OUTPUT:
        - Returns a list of Path.Command objects.
        - The list should contain both G0 rapid moves (for transitions between
          loops and layers) and G1 cutting moves (for following the contours).
        """
        from Path.Base.Generator.surface_waterline import waterline_stack, waterline_to_gcode

        stl, cutter = _make_hemisphere_stl_and_cutter()
        wl_data = waterline_stack(
            stl, cutter, sampling=2.0, min_sampling=0.5, min_z=10.0, max_z=15.0, step_down=5.0
        )

        cmds = waterline_to_gcode(
            wl_data,
            horiz_feed=300,
            vert_rapid=1000,
            horiz_rapid=1000,
            safe_z=30.0,
            clearance_z=40.0,
        )

        self.assertGreater(len(cmds), 0)
        cmd_names = {c.Name for c in cmds}
        self.assertIn("G0", cmd_names)
        self.assertIn("G1", cmd_names)

    def test31_gcode_climb_vs_conventional(self):
        """
        Verifies that `cut_climb=True` correctly reverses the toolpath direction.

        INPUT:
        - Function: waterline_to_gcode()
        - Input data: A simple square loop, processed with both `cut_climb=False`
          and `cut_climb=True`.

        EXPECTED OUTPUT:
        - The sequence of G1 cutting moves for the climb path should be the
          reverse of the sequence for the conventional path.
        """
        from Path.Base.Generator.surface_waterline import waterline_to_gcode

        # Create a simple, unambiguous square loop manually
        loop = [(0.0, 0.0, 5.0), (10.0, 0.0, 5.0), (10.0, 10.0, 5.0), (0.0, 10.0, 5.0)]
        wl_data = OrderedDict()
        wl_data[5.0] = [loop]

        cmds_conv = waterline_to_gcode(wl_data, 300, 1000, 1000, 30, 40, cut_climb=False)
        cmds_climb = waterline_to_gcode(wl_data, 300, 1000, 1000, 30, 40, cut_climb=True)

        # Extract just the X,Y coordinates from the G1 commands
        path_conv = [
            (c.Parameters.get("X"), c.Parameters.get("Y")) for c in cmds_conv if c.Name == "G1"
        ]
        path_climb = [
            (c.Parameters.get("X"), c.Parameters.get("Y")) for c in cmds_climb if c.Name == "G1"
        ]

        self.assertEqual(len(path_conv), len(path_climb))

        # The core points of the path (omitting the identical start/end points) should be a perfect reverse of each other.
        self.assertEqual(path_conv[1:-1], list(reversed(path_climb[1:-1])))
