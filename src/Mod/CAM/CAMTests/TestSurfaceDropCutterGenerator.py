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


def _make_test_stl_and_cutter():
    """Create a simple box STL and endmill cutter for testing."""
    from Path.Base.Generator.surface_common import shape_to_stl, make_ocl_cutter

    box = Part.makeBox(100, 100, 10)
    stl = shape_to_stl(box, 0.5, 0.5)
    cutter = make_ocl_cutter("endmill", 6.0, edge_height=20.0)
    return stl, cutter


@unittest.skipUnless(_ocl_available, "OpenCamLib not available")
class TestSurfaceDropCutterGenerator(PathTestUtils.PathTestBase):
    """Tests for surface_dropcutter: batch, path, adaptive, filter, gcode."""

    # -- batch_dropcutter tests --

    def test00(self):
        """
        Calculates cutter contact points for a grid of locations on a flat box surface.

        INPUT:
        - Function: batch_dropcutter()
        - Parameters: stl=100x100x10mm box, cutter=6mm endmill, grid=11×11 points, min_z=-5.0
        - Input data: Flat surface at Z=10mm with cutter starting below surface

        EXPECTED OUTPUT:
        - Returns Z-height for each grid point (same number as input points)
        - All interior points should return Z=10.0mm (surface height)
        - Batch processing is efficient for calculating many points at once
        """
        from Path.Base.Generator.surface_dropcutter import batch_dropcutter
        from Path.Base.Generator.surface_scan import BBox, generate_grid_points

        stl, cutter = _make_test_stl_and_cutter()
        bb = BBox(3, 97, 3, 97)
        grid, (nx, ny) = generate_grid_points(bb, 10.0, 10.0, min_z=-5.0)

        results = batch_dropcutter(stl, cutter, grid, -5.0)

        self.assertEqual(len(results), len(grid))
        for pt in results:
            self.assertRoughly(pt[2], 10.0, error=0.5)

    def test01(self):
        """batch_dropcutter timer callback is invoked."""
        from Path.Base.Generator.surface_dropcutter import batch_dropcutter
        from Path.Base.Generator.surface_scan import BBox, generate_grid_points

        stl, cutter = _make_test_stl_and_cutter()
        bb = BBox(10, 90, 10, 90)
        grid, _ = generate_grid_points(bb, 20.0, 20.0, min_z=-5.0)

        timings = {}

        def timer(stage, elapsed):
            timings[stage] = elapsed

        batch_dropcutter(stl, cutter, grid, -5.0, timer=timer)
        self.assertIn("batch_dropcutter", timings)

    # -- path_dropcutter tests --

    def test10(self):
        """
        Calculates cutter contact points along a straight path across a box surface.

        INPUT:
        - Function: path_dropcutter()
        - Parameters: stl=100x100x10mm box, cutter=6mm endmill, path=horizontal line, sampling=1.0mm
        - Input data: Line from (5,50) to (95,50) crossing the box surface

        EXPECTED OUTPUT:
        - Returns multiple contact points along the path
        - Interior points should be at Z=10.0mm (surface height)
        - Used for calculating toolpath along specific routes
        """
        from Path.Base.Generator.surface_dropcutter import path_dropcutter

        stl, cutter = _make_test_stl_and_cutter()

        path = ocl.Path()
        p1 = ocl.Point(5, 50, -5)
        p2 = ocl.Point(95, 50, -5)
        path.append(ocl.Line(p1, p2))

        results = path_dropcutter(stl, cutter, path, -5.0, 1.0)

        self.assertTrue(len(results) > 5, "Should produce multiple CL-points")
        for pt in results:
            if 10 < pt[0] < 90:
                self.assertRoughly(pt[2], 10.0, error=0.5)

    # -- adaptive_path_dropcutter tests --

    def test20(self):
        """adaptive_path_dropcutter returns CL-points along a line."""
        from Path.Base.Generator.surface_dropcutter import adaptive_path_dropcutter

        stl, cutter = _make_test_stl_and_cutter()

        path = ocl.Path()
        p1 = ocl.Point(5, 50, -5)
        p2 = ocl.Point(95, 50, -5)
        path.append(ocl.Line(p1, p2))

        results = adaptive_path_dropcutter(stl, cutter, path, -5.0, 2.0)

        self.assertTrue(len(results) > 5, "Should produce multiple CL-points")
        for pt in results:
            if 10 < pt[0] < 90:
                self.assertRoughly(pt[2], 10.0, error=0.5)

    # -- filter_cl_points tests --

    def test30(self):
        """filter_cl_points removes collinear points from a straight line."""
        from Path.Base.Generator.surface_dropcutter import filter_cl_points

        points = [(float(i), 50.0, 10.0) for i in range(0, 101, 10)]
        filtered = filter_cl_points(points, 0.001)

        self.assertTrue(
            len(filtered) < len(points),
            "Filtered ({}) should have fewer points than input ({})".format(
                len(filtered), len(points)
            ),
        )
        self.assertTrue(len(filtered) >= 2)

    def test31(self):
        """filter_cl_points preserves non-collinear points."""
        from Path.Base.Generator.surface_dropcutter import filter_cl_points

        points = [
            (0.0, 0.0, 0.0),
            (10.0, 0.0, 5.0),
            (20.0, 0.0, 0.0),
            (30.0, 0.0, 5.0),
            (40.0, 0.0, 0.0),
        ]
        filtered = filter_cl_points(points, 0.001)
        self.assertEqual(len(filtered), len(points))

    def test32(self):
        """filter_cl_points handles fewer than 3 points gracefully."""
        from Path.Base.Generator.surface_dropcutter import filter_cl_points

        self.assertEqual(len(filter_cl_points([], 0.001)), 0)
        self.assertEqual(len(filter_cl_points([(0, 0, 0)], 0.001)), 1)
        self.assertEqual(len(filter_cl_points([(0, 0, 0), (1, 0, 0)], 0.001)), 2)

    # -- points_to_gcode tests --

    def test40(self):
        """
        Converts cutter contact points into G-code commands with proper feeds and heights.

        INPUT:
        - Function: points_to_gcode()
        - Parameters: points=[3 contact points], horiz_feed=500, vert_rapid=1000, horiz_rapid=2000, safe_z=50, clearance_z=60
        - Input data: Three points at Z=10mm representing cutter positions

        EXPECTED OUTPUT:
        - Should start with G0 rapid move to clearance height
        - Should contain G1 cutting moves for each point
        - Proper feed rates should be applied to different move types
        """
        from Path.Base.Generator.surface_dropcutter import points_to_gcode

        points = [(0.0, 0.0, 10.0), (10.0, 0.0, 10.0), (20.0, 0.0, 10.0)]
        cmds = points_to_gcode(points, 500.0, 1000.0, 2000.0, 50.0, 60.0)

        self.assertTrue(len(cmds) > 0)
        self.assertEqual(cmds[0].Name, "G0")
        g1_cmds = [c for c in cmds if c.Name == "G1"]
        self.assertEqual(len(g1_cmds), 3)

    def test41(self):
        """points_to_gcode applies depth_offset."""
        from Path.Base.Generator.surface_dropcutter import points_to_gcode

        points = [(0.0, 0.0, 10.0)]
        cmds = points_to_gcode(points, 500.0, 1000.0, 2000.0, 50.0, 60.0, depth_offset=-0.5)
        g1_cmds = [c for c in cmds if c.Name == "G1"]
        self.assertEqual(len(g1_cmds), 1)
        self.assertRoughly(g1_cmds[0].Parameters["Z"], 9.5)

    def test42(self):
        """points_to_gcode returns empty list for empty input."""
        from Path.Base.Generator.surface_dropcutter import points_to_gcode

        self.assertEqual(len(points_to_gcode([], 500, 1000, 2000, 50, 60)), 0)

    # -- scan_lines_to_gcode tests --

    def test50(self):
        """scan_lines_to_gcode inserts travel moves between lines."""
        from Path.Base.Generator.surface_dropcutter import scan_lines_to_gcode

        line1 = [(0.0, 0.0, 10.0), (10.0, 0.0, 10.0)]
        line2 = [(0.0, 10.0, 10.0), (10.0, 10.0, 10.0)]
        cmds = scan_lines_to_gcode([line1, line2], 500.0, 1000.0, 2000.0, 50.0, 60.0)

        g0_cmds = [c for c in cmds if c.Name == "G0"]
        self.assertTrue(len(g0_cmds) >= 3)

    def test51(self):
        """Short ZigZag transition stays on the surface instead of retracting.

        INPUT:
        - Two scan lines on a 100×100×10 box in ZigZag order.
          line1 ends at (90, 47, 10), line2 starts at (90, 53, 10).
          Distance = 6 mm, which is < 2× cutter diameter (12 mm).
        - optimize_transitions=True with the box STL and a 6 mm cutter.

        EXPECTED OUTPUT:
        - The transition between lines uses G1 moves along the surface
          (the tool stays down) instead of a G0 retract to safe_z.
        - Without optimization, a G0 Z retract to safe_z appears between
          the two scan lines.
        """
        from Path.Base.Generator.surface_dropcutter import scan_lines_to_gcode

        stl, cutter = _make_test_stl_and_cutter()

        line1 = [(10.0, 47.0, 10.0), (90.0, 47.0, 10.0)]
        line2 = [(90.0, 53.0, 10.0), (10.0, 53.0, 10.0)]
        safe_z = 50.0
        clearance_z = 60.0

        cmds_no_opt = scan_lines_to_gcode(
            [line1, line2],
            500.0,
            1000.0,
            2000.0,
            safe_z,
            clearance_z,
        )
        cmds_opt = scan_lines_to_gcode(
            [line1, line2],
            500.0,
            1000.0,
            2000.0,
            safe_z,
            clearance_z,
            optimize_transitions=True,
            safe_stl=stl,
            cutter=cutter,
        )

        # Non-optimized: must have a G0 Z retract to safe_z between lines
        g0_z_no_opt = [
            c
            for c in cmds_no_opt
            if c.Name == "G0"
            and "Z" in c.Parameters
            and c.Parameters["Z"] >= safe_z
            and "X" not in c.Parameters
        ]
        self.assertTrue(
            len(g0_z_no_opt) >= 1,
            "Non-optimized should retract to safe_z between lines",
        )

        # Optimized: no G0 Z retract to safe_z between lines — the
        # transition is G1 moves along the surface.  The only G0 Z
        # should be the initial clearance_z move.
        transition_retracts = [
            c
            for c in cmds_opt
            if c.Name == "G0"
            and "Z" in c.Parameters
            and c.Parameters["Z"] >= safe_z
            and "X" not in c.Parameters
            and c.Parameters["Z"] != clearance_z
        ]
        self.assertEqual(
            len(transition_retracts),
            0,
            "Optimized short transition should not retract to safe_z, "
            "got {} retract(s): {}".format(
                len(transition_retracts),
                [c.Parameters["Z"] for c in transition_retracts],
            ),
        )

    def test52(self):
        """scan_lines_to_gcode with optimize_transitions=False ignores stl/cutter."""
        from Path.Base.Generator.surface_dropcutter import scan_lines_to_gcode

        line1 = [(0.0, 0.0, 10.0), (10.0, 0.0, 10.0)]
        line2 = [(0.0, 10.0, 10.0), (10.0, 10.0, 10.0)]

        # Should not raise even though stl/cutter are None
        cmds = scan_lines_to_gcode(
            [line1, line2],
            500.0,
            1000.0,
            2000.0,
            50.0,
            60.0,
            optimize_transitions=False,
            safe_stl=None,
            cutter=None,
        )
        self.assertTrue(len(cmds) > 0)

    def test53(self):
        """Long-distance transition falls back to safe_z retract.

        INPUT:
        - Two scan lines 50 mm apart in Y (far > 2× cutter diam = 12 mm).
        - optimize_transitions=True with box STL and 6 mm cutter.

        EXPECTED OUTPUT:
        - The transition still retracts to safe_z because long-distance
          optimization is not yet implemented.  The output should match
          the non-optimized pattern (G0 Z safe, G0 XY).
        """
        from Path.Base.Generator.surface_dropcutter import scan_lines_to_gcode

        stl, cutter = _make_test_stl_and_cutter()

        line1 = [(10.0, 10.0, 10.0), (90.0, 10.0, 10.0)]
        line2 = [(10.0, 60.0, 10.0), (90.0, 60.0, 10.0)]
        safe_z = 50.0
        clearance_z = 60.0

        cmds_no_opt = scan_lines_to_gcode(
            [line1, line2],
            500.0,
            1000.0,
            2000.0,
            safe_z,
            clearance_z,
        )
        cmds_opt = scan_lines_to_gcode(
            [line1, line2],
            500.0,
            1000.0,
            2000.0,
            safe_z,
            clearance_z,
            optimize_transitions=True,
            safe_stl=stl,
            cutter=cutter,
        )

        # Both should retract to safe_z — long-distance optimization is deferred
        g0_z_no_opt = [
            c
            for c in cmds_no_opt
            if c.Name == "G0"
            and "Z" in c.Parameters
            and c.Parameters["Z"] >= safe_z
            and "X" not in c.Parameters
            and c.Parameters["Z"] != clearance_z
        ]
        g0_z_opt = [
            c
            for c in cmds_opt
            if c.Name == "G0"
            and "Z" in c.Parameters
            and c.Parameters["Z"] >= safe_z
            and "X" not in c.Parameters
            and c.Parameters["Z"] != clearance_z
        ]
        self.assertEqual(
            len(g0_z_no_opt),
            len(g0_z_opt),
            "Long transition should retract to safe_z same as non-optimized",
        )

    # -- grid_to_scan_lines tests --

    def test60(self):
        """grid_to_scan_lines reshapes flat grid into rows."""
        from Path.Base.Generator.surface_dropcutter import grid_to_scan_lines

        grid = [
            (0, 0, 10),
            (5, 0, 10),
            (10, 0, 10),
            (0, 5, 10),
            (5, 5, 10),
            (10, 5, 10),
        ]
        lines = grid_to_scan_lines(grid, nx=3, ny=2)

        self.assertEqual(len(lines), 2)
        self.assertEqual(len(lines[0]), 3)
        self.assertEqual(len(lines[1]), 3)
        self.assertEqual(lines[0][0], (0, 0, 10))
        self.assertEqual(lines[1][2], (10, 5, 10))
