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
import FreeCAD
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
class TestSurfaceDropCutter(PathTestUtils.PathTestBase):
    """Tests for surface_dropcutter: batch, path, and adaptive drop-cutter algorithms."""

    def setUp(self):
        """Create common test geometry: a flat box and a curved hemisphere."""
        from Path.Base.Generator.surface_mesh import _shape_to_stl
        from Path.Base.Generator.surface_common import make_ocl_cutter

        # A flat box for simple projection tests
        box = Part.makeBox(100, 100, 10)
        self.flat_stl = _shape_to_stl(box, 0.1, 0.5)

        # A hemisphere for testing adaptive sampling on curved surfaces
        sphere = Part.makeSphere(25.0)
        clipper = Part.makeBox(60, 60, 30, FreeCAD.Vector(-30, -30, -30))
        hemisphere = sphere.cut(clipper)
        self.hemisphere_stl = _shape_to_stl(hemisphere, 0.1, 0.2)

        # A standard endmill and a ballnose for testing
        self.endmill = make_ocl_cutter("endmill", 6.0, edge_height=20.0)
        self.ballnose = make_ocl_cutter("ballend", 6.0, edge_height=10.0)

    # -- batch_dropcutter tests --

    def test00_batch_dropcutter_on_flat_surface(self):
        """
        Calculates cutter contact points for a grid of locations on a flat box surface.

        INPUT:
        - Function: batch_dropcutter()
        - Parameters: stl=100x100x10mm box, cutter=6mm endmill.
        - Input data: A list of polylines forming a simple grid inside the box boundaries.

        EXPECTED OUTPUT:
        - Returns a flat list of (x, y, z) tuples.
        - The number of output points should match the total number of input points.
        - All points should be projected to the surface height of Z=10.0mm.
        """
        from Path.Base.Generator.surface_dropcutter import batch_dropcutter

        # A simple 2-line grid
        polylines = [[(10, 10, -5), (90, 10, -5)], [(10, 20, -5), (90, 20, -5)]]
        total_input_points = sum(len(line) for line in polylines)

        results = batch_dropcutter(self.flat_stl, self.endmill, polylines, min_z=-5.0)

        self.assertEqual(len(results), total_input_points)
        for pt in results:
            self.assertAlmostEqual(pt[2], 10.0, delta=0.01)

    # -- path_dropcutter tests --

    def test10_path_dropcutter_on_flat_surface(self):
        """
        Calculates cutter contact points along a straight path across a box surface.

        INPUT:
        - Function: path_dropcutter()
        - Parameters: stl=100x100x10mm box, cutter=6mm endmill, sampling=5.0mm.
        - Input data: A single polyline from (5, 50) to (95, 50).

        EXPECTED OUTPUT:
        - Returns a list of sampled contact points along the path.
        - For a 90mm path with 5mm sampling, we expect 20 points (90/5 + 2).
        - All points should be at the surface height of Z=10.0mm.
        """
        from Path.Base.Generator.surface_dropcutter import path_dropcutter

        polyline = [[(5, 50, -5), (95, 50, -5)]]
        results = path_dropcutter(self.flat_stl, self.endmill, polyline, min_z=-5.0, sampling=5.0)

        self.assertEqual(len(results), 20)
        for pt in results:
            self.assertAlmostEqual(pt[2], 10.0, delta=0.01)

    # -- adaptive_path_dropcutter tests --

    def test20_adaptive_dropcutter_on_flat_surface(self):
        """
        Tests adaptive drop-cutter on a flat surface, where it should behave like standard.

        INPUT:
        - Function: adaptive_path_dropcutter()
        - Parameters: stl=100x100x10mm box, sampling=5.0mm.
        - Input data: The same straight path as the standard path_dropcutter test.

        EXPECTED OUTPUT:
        - The exact number of points is an implementation detail of OCL's adaptive algorithm.
          We verify that it produces a reasonable number of points (at least as many as standard)
          and that they are all at the correct Z-height.
        """
        from Path.Base.Generator.surface_dropcutter import adaptive_path_dropcutter

        polyline = [[(5, 50, -5), (95, 50, -5)]]
        results = adaptive_path_dropcutter(
            self.flat_stl, self.endmill, polyline, min_z=-5.0, sampling=5.0
        )

        self.assertGreaterEqual(len(results), 20)
        for pt in results:
            self.assertAlmostEqual(pt[2], 10.0, delta=0.01)

    def test21_adaptive_dropcutter_on_curved_surface(self):
        """
        Verifies that adaptive sampling adds more points on a curved surface.

        INPUT:
        - Function: adaptive_path_dropcutter() vs path_dropcutter()
        - Input data: A path that travels over the top of a hemisphere.

        EXPECTED OUTPUT:
        - The `adaptive_path_dropcutter` should produce significantly more points
          than the standard `path_dropcutter` with the same base sampling rate.
        - This confirms the refinement logic is working correctly in high-curvature areas.
        """
        from Path.Base.Generator.surface_dropcutter import path_dropcutter, adaptive_path_dropcutter

        # A path going over the top of the 25mm-radius hemisphere
        polyline = [[(-20, 0, -5), (20, 0, -5)]]

        # Run standard fixed-interval drop-cutter
        standard_results = path_dropcutter(
            self.hemisphere_stl, self.ballnose, polyline, min_z=-5.0, sampling=2.0
        )

        # Run adaptive drop-cutter with the same base sampling
        adaptive_results = adaptive_path_dropcutter(
            self.hemisphere_stl, self.ballnose, polyline, min_z=-5.0, sampling=2.0, min_sampling=0.2
        )

        self.assertGreater(
            len(adaptive_results),
            len(standard_results),
            "Adaptive sampling should add points on a curved surface",
        )
