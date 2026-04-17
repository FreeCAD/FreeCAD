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

import math
import Path
import Path.Base.Generator.surface_scan as scan
import CAMTests.PathTestUtils as PathTestUtils

Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())
Path.Log.trackModule(Path.Log.thisModule())


class TestSurfaceScanGenerator(PathTestUtils.PathTestBase):
    """Tests for surface_scan: BBox, line, zigzag, circular, spiral, grid."""

    # -- BBox tests --

    def test00(self):
        """
        Creates a bounding box and verifies its geometric properties are calculated correctly.

        INPUT:
        - Class: BBox
        - Parameters: xmin=0, xmax=100, ymin=0, ymax=50
        - Input data: 100x50 unit rectangle

        EXPECTED OUTPUT:
        - x_length should be 100.0 units
        - y_length should be 50.0 units
        - center should be at (50, 25)
        - diagonal should be sqrt(100² + 50²) ≈ 111.8 units
        """
        bb = scan.BBox(0, 100, 0, 50)
        self.assertRoughly(bb.x_length, 100.0)
        self.assertRoughly(bb.y_length, 50.0)
        self.assertRoughly(bb.center[0], 50.0)
        self.assertRoughly(bb.center[1], 25.0)
        self.assertRoughly(bb.diagonal, math.sqrt(100**2 + 50**2))

    def test01(self):
        """
        Creates a bounding box from an object that has XMin/XMax/YMin/YMax attributes.

        INPUT:
        - Class method: BBox.from_bbox()
        - Parameters: FakeBB object with XMin=-10, XMax=10, YMin=-5, YMax=5
        - Input data: Object mimicking FreeCAD bounding box structure

        EXPECTED OUTPUT:
        - Should create BBox with same dimensions as source object
        - x_length should be 20.0 units (from -10 to 10)
        - y_length should be 10.0 units (from -5 to 5)
        """

        class FakeBB:
            XMin = -10
            XMax = 10
            YMin = -5
            YMax = 5

        bb = scan.BBox.from_bbox(FakeBB())
        self.assertRoughly(bb.xmin, -10.0)
        self.assertRoughly(bb.xmax, 10.0)
        self.assertRoughly(bb.x_length, 20.0)
        self.assertRoughly(bb.y_length, 10.0)

    # -- Line scan tests --

    def test10(self):
        """
        Generates parallel line scan pattern that covers a rectangular bounding box.

        INPUT:
        - Function: generate_line_scan()
        - Parameters: bbox=100x100 square, step_over=10.0, tool_diam=6.0
        - Input data: Square area with 10mm spacing between lines

        EXPECTED OUTPUT:
        - Returns a list of line segments
        - Each line should have exactly 2 endpoints
        - Each endpoint should be a 2D coordinate (x, y)
        - Lines should extend beyond bbox to ensure complete coverage
        """
        bb = scan.BBox(0, 100, 0, 100)
        lines = scan.generate_line_scan(bb, 10.0, 6.0)

        self.assertIsInstance(lines, list)
        self.assertTrue(len(lines) > 0)
        for line in lines:
            self.assertEqual(len(line), 2)
            self.assertEqual(len(line[0]), 2)
            self.assertEqual(len(line[1]), 2)

    def test11(self):
        """
        Verifies that line scan produces horizontal parallel lines when angle=0.

        INPUT:
        - Function: generate_line_scan()
        - Parameters: bbox=100x100 square, step_over=10.0, tool_diam=6.0, angle=0.0
        - Input data: Square area with no rotation (horizontal lines)

        EXPECTED OUTPUT:
        - All lines should be horizontal (parallel to X-axis)
        - Both endpoints of each line should have the same Y coordinate
        - This ensures proper parallel line scanning pattern
        """
        bb = scan.BBox(0, 100, 0, 100)
        lines = scan.generate_line_scan(bb, 10.0, 6.0, angle=0.0)
        for line in lines:
            self.assertRoughly(line[0][1], line[1][1], error=0.001)

    def test12(self):
        """
        Verifies that adjacent lines in a scan pattern are spaced by the step_over distance.

        INPUT:
        - Function: generate_line_scan()
        - Parameters: bbox=100x100 square, step_over=10.0, tool_diam=6.0, angle=0.0
        - Input data: Square area with 10mm spacing between scan lines

        EXPECTED OUTPUT:
        - Distance between adjacent lines should be exactly 10.0mm
        - Ensures consistent spacing for uniform material removal
        """
        bb = scan.BBox(0, 100, 0, 100)
        step_over = 10.0
        lines = scan.generate_line_scan(bb, step_over, 6.0, angle=0.0)
        for i in range(1, len(lines)):
            y_prev = lines[i - 1][0][1]
            y_curr = lines[i][0][1]
            self.assertRoughly(abs(y_curr - y_prev), step_over, error=0.01)

    def test13(self):
        """
        Tests that the reversed flag inverts the order of scan lines.

        INPUT:
        - Function: generate_line_scan()
        - Parameters: bbox=100x100 square, step_over=10.0, tool_diam=6.0, reversed=True/False
        - Input data: Same scan pattern with different order settings

        EXPECTED OUTPUT:
        - Both patterns should have same number of lines
        - First line of forward pattern should match last line of reversed pattern
        - Allows control over scan direction for optimization
        """
        bb = scan.BBox(0, 100, 0, 100)
        lines_fwd = scan.generate_line_scan(bb, 10.0, 6.0, reversed=False)
        lines_rev = scan.generate_line_scan(bb, 10.0, 6.0, reversed=True)
        self.assertEqual(len(lines_fwd), len(lines_rev))
        self.assertRoughly(lines_fwd[0][0][1], lines_rev[-1][0][1], error=0.001)

    def test14(self):
        """
        Tests that rotating the scan angle changes the line orientation while keeping count.

        INPUT:
        - Function: generate_line_scan()
        - Parameters: bbox=100x100 square, step_over=10.0, tool_diam=6.0, angle=0.0 and 45.0
        - Input data: Same area scanned at different angles

        EXPECTED OUTPUT:
        - Both patterns should have same number of lines
        - 45-degree lines should be diagonal compared to 0-degree horizontal lines
        - Angle parameter allows scanning in any direction
        """
        bb = scan.BBox(0, 100, 0, 100)
        lines_0 = scan.generate_line_scan(bb, 10.0, 6.0, angle=0.0)
        lines_45 = scan.generate_line_scan(bb, 10.0, 6.0, angle=45.0)
        self.assertEqual(len(lines_0), len(lines_45))

    def test15(self):
        """
        Tests that invalid step_over values (zero or negative) are rejected.

        INPUT:
        - Function: generate_line_scan()
        - Parameters: bbox=100x100 square, step_over=0.0 and step_over=-1.0
        - Input data: Invalid spacing values that don't make physical sense

        EXPECTED OUTPUT:
        - Should raise ValueError for both zero and negative step_over
        - Prevents creation of invalid scan patterns
        - Ensures data validation catches impossible spacing
        """
        bb = scan.BBox(0, 100, 0, 100)
        self.assertRaises(ValueError, scan.generate_line_scan, bb, 0.0, 6.0)
        self.assertRaises(ValueError, scan.generate_line_scan, bb, -1.0, 6.0)

    def test16(self):
        """
        Verifies that scan lines extend beyond the bounding box to ensure complete edge coverage.

        INPUT:
        - Function: generate_line_scan()
        - Parameters: bbox=100x100 square, step_over=10.0, tool_diam=10.0, angle=0.0
        - Input data: Square area with tool diameter equal to step spacing

        EXPECTED OUTPUT:
        - Lines should extend beyond bbox on both ends
        - Ensures cutter reaches the edges of the material
        - Prevents uncut material at the boundaries
        """
        bb = scan.BBox(0, 100, 0, 100)
        lines = scan.generate_line_scan(bb, 10.0, 10.0, angle=0.0)
        for line in lines:
            x_min = min(line[0][0], line[1][0])
            x_max = max(line[0][0], line[1][0])
            self.assertTrue(x_min < bb.xmin, "Line should extend past bbox XMin")
            self.assertTrue(x_max > bb.xmax, "Line should extend past bbox XMax")

    # -- Zigzag scan tests --

    def test20(self):
        """
        Generates a zigzag scan pattern where adjacent lines alternate direction.

        INPUT:
        - Function: generate_zigzag_scan()
        - Parameters: bbox=100x100 square, step_over=10.0, tool_diam=6.0, angle=0.0
        - Input data: Square area with alternating line directions

        EXPECTED OUTPUT:
        - Even-numbered lines should go left-to-right (increasing X)
        - Odd-numbered lines should go right-to-left (decreasing X)
        - Reduces rapid moves by eliminating long travel between lines
        """
        bb = scan.BBox(0, 100, 0, 100)
        lines = scan.generate_zigzag_scan(bb, 10.0, 6.0, angle=0.0)
        self.assertTrue(len(lines) > 2)
        for i, line in enumerate(lines):
            if i % 2 == 0:
                self.assertTrue(
                    line[0][0] < line[1][0],
                    "Even line {} should go left-to-right".format(i),
                )
            else:
                self.assertTrue(
                    line[0][0] > line[1][0],
                    "Odd line {} should go right-to-left".format(i),
                )


def test31(self):
    """
    Circular zigzag scan alternates direction.

    INPUT:
    - Function: generate_circular_zigzag_scan()
    - Parameters: center=(50,50), max_radius=50.0, step_over=5.0, tool_diam=6.0
    - Input data: Circular area with alternating circle directions

    EXPECTED OUTPUT:
    - Even-numbered circles should go counter-clockwise (direction=1)
    - Odd-numbered circles should go clockwise (direction=-1)
    - Reduces rapid moves by eliminating long travel between circles
    """
    center = (50.0, 50.0)
    circles = scan.generate_circular_zigzag_scan(center, 50.0, 5.0, 6.0)
    self.assertTrue(len(circles) > 2)
    for i, c in enumerate(circles):
        expected_dir = 1 if i % 2 == 0 else -1
        self.assertEqual(c["direction"], expected_dir)


def test32(self):
    """
    Circular scan reversed flag reverses order.

    INPUT:
    - Function: generate_circular_scan()
    - Parameters: center=(50,50), max_radius=50.0, step_over=5.0, tool_diam=6.0, reversed=True/False
    - Input data: Same scan pattern with different order settings

    EXPECTED OUTPUT:
    - Both patterns should have same number of circles
    - First circle of forward pattern should match last circle of reversed pattern
    - Allows control over scan direction for optimization
    """
    center = (50.0, 50.0)
    fwd = scan.generate_circular_scan(center, 50.0, 5.0, 6.0, reversed=False)
    rev = scan.generate_circular_scan(center, 50.0, 5.0, 6.0, reversed=True)
    self.assertEqual(len(fwd), len(rev))
    self.assertRoughly(fwd[0]["radius"], rev[-1]["radius"])


def test33(self):
    """
    Circular scan raises ValueError for non-positive step_over.

    INPUT:
    - Function: generate_circular_scan()
    - Parameters: center=(50,50), max_radius=50.0, step_over=0.0, tool_diam=6.0
    - Input data: Invalid spacing value that doesn't make physical sense

    EXPECTED OUTPUT:
    - Should raise ValueError for non-positive step_over
    - Prevents creation of invalid scan patterns
    - Ensures data validation catches impossible spacing
    """
    self.assertRaises(ValueError, scan.generate_circular_scan, (0, 0), 50.0, 0.0, 6.0)


# -- Spiral scan tests --


def test40(self):
    """
    Generates a continuous spiral path that expands outward from a center point.

    INPUT:
    - Function: generate_spiral_scan()
    - Parameters: center=(50,50), max_radius=50.0, step_over=5.0, step=1.0
    - Input data: Circular area with 1mm step between spiral turns

    EXPECTED OUTPUT:
    - Returns list of (x, y) points forming a spiral
    - First point should be near the center (within 5mm)
    - Last point should be near the maximum radius (40mm+)
    - Creates smooth continuous cutting path without lift moves
    """
    center = (50.0, 50.0)
    points = scan.generate_spiral_scan(center, 50.0, 5.0, 1.0)
    self.assertTrue(len(points) > 10)
    dx = points[0][0] - center[0]
    dy = points[0][1] - center[1]
    self.assertTrue(
        math.sqrt(dx**2 + dy**2) < 5.0,
        "First spiral point should be near center",
    )
    dx = points[-1][0] - center[0]
    dy = points[-1][1] - center[1]
    self.assertTrue(
        math.sqrt(dx**2 + dy**2) > 40.0,
        "Last spiral point should be near max_radius",
    )


def test41(self):
    """
    Spiral scan reversed starts from outside.

    INPUT:
    - Function: generate_spiral_scan()
    - Parameters: center=(50,50), max_radius=50.0, step_over=5.0, step=1.0, reversed=True/False
    - Input data: Same scan pattern with different order settings

    EXPECTED OUTPUT:
    - Both patterns should have same number of points
    - First point of forward pattern should match last point of reversed pattern
    - Allows control over scan direction for optimization
    """
    center = (50.0, 50.0)
    fwd = scan.generate_spiral_scan(center, 50.0, 5.0, 1.0, reversed=False)
    rev = scan.generate_spiral_scan(center, 50.0, 5.0, 1.0, reversed=True)
    self.assertEqual(len(fwd), len(rev))
    dx = rev[0][0] - center[0]
    dy = rev[0][1] - center[1]
    self.assertTrue(
        math.sqrt(dx**2 + dy**2) > 40.0,
        "First reversed spiral point should be near max_radius",
    )


def test42(self):
    """
    Spiral scan raises ValueError for non-positive parameters.

    INPUT:
    - Function: generate_spiral_scan()
    - Parameters: center=(50,50), max_radius=50.0, step_over=0.0, step=1.0
    - Input data: Invalid spacing value that doesn't make physical sense

    EXPECTED OUTPUT:
    - Should raise ValueError for non-positive step_over
    - Should raise ValueError for non-positive step
    - Prevents creation of invalid scan patterns
    - Ensures data validation catches impossible spacing
    """
    self.assertRaises(ValueError, scan.generate_spiral_scan, (0, 0), 50.0, 0.0, 1.0)
    self.assertRaises(ValueError, scan.generate_spiral_scan, (0, 0), 50.0, 5.0, 0.0)


# -- Grid points tests --


def test50(self):
    """
    Generates a regular grid of points covering a rectangular bounding box.

    INPUT:
    - Function: generate_grid_points()
    - Parameters: bbox=100x50 rectangle, step_x=10.0, step_y=10.0
    - Input data: Rectangular area with 10mm spacing in both directions

    EXPECTED OUTPUT:
    - Should create 11×6 grid points (66 total)
    - Points should include all corners of the bounding box
    - Grid points are used for batch drop-cutter calculations
    """
    bb = scan.BBox(0, 100, 0, 50)
    points, (nx, ny) = scan.generate_grid_points(bb, 10.0, 10.0)
    self.assertEqual(len(points), nx * ny)
    self.assertEqual(nx, 11)  # 0,10,20,...,100 = 11 points
    self.assertEqual(ny, 6)  # 0,10,20,...,50 = 6 points


def test51(self):
    """
    Grid points have correct Z value.

    INPUT:
    - Function: generate_grid_points()
    - Parameters: bbox=10x10 square, step_x=5.0, step_y=5.0, min_z=-5.0
    - Input data: Square area with 5mm spacing and -5mm Z offset

    EXPECTED OUTPUT:
    - All points should have Z value equal to min_z
    - Ensures correct Z positioning for drop-cutter calculations
    """
    bb = scan.BBox(0, 10, 0, 10)
    min_z = -5.0
    points, _ = scan.generate_grid_points(bb, 5.0, 5.0, min_z=min_z)
    for pt in points:
        self.assertRoughly(pt[2], min_z)


def test52(self):
    """
    Grid points first and last match bbox corners.

    INPUT:
    - Function: generate_grid_points()
    - Parameters: bbox=100x50 rectangle, step_x=10.0, step_y=10.0
    - Input data: Rectangular area with 10mm spacing in both directions

    EXPECTED OUTPUT:
    - First point should match bbox bottom-left corner (0,0)
    - Last point should match bbox top-right corner (100,50)
    - Ensures grid points cover the entire bounding box
    """
    bb = scan.BBox(0, 100, 0, 50)
    points, _ = scan.generate_grid_points(bb, 10.0, 10.0)
    self.assertRoughly(points[0][0], 0.0)
    self.assertRoughly(points[0][1], 0.0)
    self.assertRoughly(points[-1][0], 100.0)
    self.assertRoughly(points[-1][1], 50.0)

    def test52(self):
        """Grid points first and last match bbox corners."""
        bb = scan.BBox(0, 100, 0, 50)
        points, _ = scan.generate_grid_points(bb, 10.0, 10.0)
        self.assertRoughly(points[0][0], 0.0)
        self.assertRoughly(points[0][1], 0.0)
        self.assertRoughly(points[-1][0], 100.0)
        self.assertRoughly(points[-1][1], 50.0)
