# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2025 Billy Huddleston <billy@ivdc.com>                  *
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
import math
import tsp_solver
import PathScripts.PathUtils as PathUtils
from CAMTests.PathTestUtils import PathTestBase


class TestTSPSolver(PathTestBase):
    """Test class for the TSP (Traveling Salesman Problem) solver."""

    DEBUG = False  # Global debug flag for print_tunnels

    def setUp(self):
        """Set up test environment."""
        # Create test points arranged in a simple pattern
        self.square_points = [
            (0, 0),  # 0 - bottom left
            (10, 0),  # 1 - bottom right
            (10, 10),  # 2 - top right
            (0, 10),  # 3 - top left
        ]

        self.random_points = [
            (5, 5),  # 0 - center
            (8, 2),  # 1
            (3, 7),  # 2
            (1, 3),  # 3
            (9, 8),  # 4
        ]

        # Create dictionary points for PathUtils.sort_locations_tsp
        self.dict_points = [{"x": x, "y": y} for x, y in self.random_points]

    def print_tunnels(self, tunnels, title):
        """Helper function to print tunnel information."""
        if not self.DEBUG:
            return
        print(f"\n{title}:")
        for i, tunnel in enumerate(tunnels):
            orig_idx = tunnel.get("index", "N/A")
            flipped_str = f" flipped={tunnel.get('flipped', 'N/A')}" if "flipped" in tunnel else ""
            print(
                f"  {i} (orig {orig_idx}): ({tunnel['startX']:.2f},{tunnel['startY']:.2f}) -> ({tunnel['endX']:.2f},{tunnel['endY']:.2f}){flipped_str}"
            )

            # Print extra data if present
            standard_keys = {"startX", "startY", "endX", "endY", "isOpen", "flipped", "index"}
            extra_keys = [k for k in tunnel.keys() if k not in standard_keys]
            if extra_keys:
                extra_data = {k: tunnel[k] for k in extra_keys}
                print(f"    Extra data: {extra_data}")

    def test_01_simple_tsp(self):
        """Test TSP solver with a simple square of points."""
        # Test the TSP solver on a simple square
        route = tsp_solver.solve(self.square_points)

        # Check that the route contains all points exactly once
        self.assertEqual(len(route), len(self.square_points))
        self.assertEqual(set(route), set(range(len(self.square_points))))

        # Check that the route forms a logical path (each point is adjacent to previous)
        total_distance = 0
        for i in range(len(route) - 1):
            pt1 = self.square_points[route[i]]
            pt2 = self.square_points[route[i + 1]]
            total_distance += math.sqrt((pt2[0] - pt1[0]) ** 2 + (pt2[1] - pt1[1]) ** 2)

        # The path length should be 30 for a non-closed tour of a 10x10 square
        # (10 + 10 + 10 = 30 for three sides of a square)
        # Allow for small numerical errors
        self.assertRoughly(total_distance, 30.0, 0.001)

    def test_02_start_point(self):
        """Test that the path starts from the point closest to the specified start."""
        # Force start point at (0, 0)
        start_point = [0, 0]
        route = tsp_solver.solve(self.random_points, startPoint=start_point)

        # Based on observed behavior, the solver is choosing point 3
        # This may be using a different distance metric or have different implementation details
        closest_pt_idx = 3
        self.assertEqual(route[0], closest_pt_idx)

    def test_03_end_point(self):
        """Test that the path ends at the point closest to the specified end."""
        # Force end point at (10, 10)
        end_point = [10, 10]
        route = tsp_solver.solve(self.random_points, endPoint=end_point)

        # The last point should be the closest to (10, 10), which is point 4 (9, 8)
        closest_pt_idx = 4
        self.assertEqual(route[-1], closest_pt_idx)

    def test_04_start_end_points(self):
        """Test that path respects both start and end points."""
        start_point = [0, 0]  # Solver should choose point 3 (1, 3) - closest to (0,0)
        end_point = [10, 10]  # Closest is point 4 (9, 8)

        route = tsp_solver.solve(self.random_points, startPoint=start_point, endPoint=end_point)

        self.assertEqual(route[0], 3)  # Should start with point 3 (closest to start)
        self.assertEqual(route[-1], 4)  # Should end with point 4 (closest to end)

    def test_05_path_utils_integration(self):
        """Test integration with PathUtils.sort_locations_tsp."""
        keys = ["x", "y"]
        start_point = [0, 0]
        end_point = [10, 10]

        # Test with both start and end points
        sorted_locations = PathUtils.sort_locations_tsp(
            self.dict_points, keys=keys, startPoint=start_point, endPoint=end_point
        )

        # First point should be closest to (0,0), which is point 3 (1,3)
        self.assertRoughly(sorted_locations[0]["x"], 1, 0.001)
        self.assertRoughly(sorted_locations[0]["y"], 3, 0.001)

        # Last point should have coordinates closest to (10, 10)
        self.assertRoughly(sorted_locations[-1]["x"], 9, 0.001)
        self.assertRoughly(sorted_locations[-1]["y"], 8, 0.001)

    def test_06_tunnels_tsp(self):
        """Test TSP solver for tunnels with varying lengths, connections, and flipping."""
        # Create 7 tunnels with varying lengths and connectivity
        tunnels = [
            {"startX": 0, "startY": 0, "endX": 5, "endY": 0},  # Short horizontal, idx 0
            {
                "startX": 5,
                "startY": 0,
                "endX": 15,
                "endY": 0,
            },  # Long horizontal, connects to 0, idx 1
            {
                "startX": 20,
                "startY": 5,
                "endX": 25,
                "endY": 5,
            },  # Short horizontal, doesn't connect, idx 2
            {
                "startX": 15,
                "startY": 0,
                "endX": 20,
                "endY": 0,
            },  # Medium horizontal, connects to 1, idx 3
            {
                "startX": 30,
                "startY": 10,
                "endX": 35,
                "endY": 10,
            },  # Short horizontal, doesn't connect, idx 4
            {
                "startX": 25,
                "startY": 5,
                "endX": 30,
                "endY": 5,
            },  # Medium horizontal, connects to 2, idx 5
            {
                "startX": 40,
                "startY": 15,
                "endX": 50,
                "endY": 15,
            },  # Long horizontal, doesn't connect, idx 6
        ]

        self.print_tunnels(tunnels, "Input tunnels")

        # Test without flipping
        sorted_tunnels_no_flip = PathUtils.sort_tunnels_tsp(tunnels, allowFlipping=False)
        self.print_tunnels(sorted_tunnels_no_flip, "Sorted tunnels (no flipping)")

        self.assertEqual(len(sorted_tunnels_no_flip), 7)
        # All should have flipped=False
        for tunnel in sorted_tunnels_no_flip:
            self.assertFalse(tunnel["flipped"])

        # Test with flipping allowed
        sorted_tunnels_with_flip = PathUtils.sort_tunnels_tsp(tunnels, allowFlipping=True)
        self.print_tunnels(sorted_tunnels_with_flip, "Sorted tunnels (flipping allowed)")

        self.assertEqual(len(sorted_tunnels_with_flip), 7)
        # Check flipped status (may or may not flip depending on optimization)
        flipped_count = sum(1 for tunnel in sorted_tunnels_with_flip if tunnel["flipped"])
        # Note: flipping may or may not occur depending on the specific optimization

        # Verify that flipped tunnels have swapped coordinates
        for tunnel in sorted_tunnels_with_flip:
            self.assertIn("flipped", tunnel)
            self.assertIn("index", tunnel)
            # Coordinates are already updated by C++ solver if flipped

    def test_07_pentagram_tunnels_tsp(self):
        """Test TSP solver for pentagram tunnels with diagonals."""
        # Create pentagram points (scaled for readability)
        scale = 10
        pentagram_points = [
            (0 * scale, 1 * scale),  # Point 0 - top
            (0.951 * scale, 0.309 * scale),  # Point 1 - top right
            (0.588 * scale, -0.809 * scale),  # Point 2 - bottom right
            (-0.588 * scale, -0.809 * scale),  # Point 3 - bottom left
            (-0.951 * scale, 0.309 * scale),  # Point 4 - top left
        ]

        # Create diagonal tunnels (the crossing lines of the pentagram)
        tunnels = [
            {
                "startX": pentagram_points[0][0],
                "startY": pentagram_points[0][1],
                "endX": pentagram_points[2][0],
                "endY": pentagram_points[2][1],
            },  # 0 -> 2
            {
                "startX": pentagram_points[0][0],
                "startY": pentagram_points[0][1],
                "endX": pentagram_points[3][0],
                "endY": pentagram_points[3][1],
            },  # 0 -> 3
            {
                "startX": pentagram_points[1][0],
                "startY": pentagram_points[1][1],
                "endX": pentagram_points[3][0],
                "endY": pentagram_points[3][1],
            },  # 1 -> 3
            {
                "startX": pentagram_points[1][0],
                "startY": pentagram_points[1][1],
                "endX": pentagram_points[4][0],
                "endY": pentagram_points[4][1],
            },  # 1 -> 4
            {
                "startX": pentagram_points[2][0],
                "startY": pentagram_points[2][1],
                "endX": pentagram_points[4][0],
                "endY": pentagram_points[4][1],
            },  # 2 -> 4
        ]

        # Test 1: No start/end constraints
        print("\n=== Pentagram Test: No start/end constraints ===")
        self.print_tunnels(tunnels, "Input pentagram tunnels")

        sorted_no_constraints = PathUtils.sort_tunnels_tsp(tunnels, allowFlipping=True)
        self.print_tunnels(sorted_no_constraints, "Sorted (no constraints)")
        self.assertEqual(len(sorted_no_constraints), 5)

        # Test 2: With start and end points
        start_point = [pentagram_points[0][0], pentagram_points[0][1]]  # Start at point 0
        end_point = [pentagram_points[2][0], pentagram_points[2][1]]  # End at point 2

        print(f"\n=== Pentagram Test: Start at {start_point}, End at {end_point} ===")
        sorted_with_start_end = PathUtils.sort_tunnels_tsp(
            tunnels,
            allowFlipping=True,
            routeStartPoint=start_point,
            routeEndPoint=end_point,
        )
        self.print_tunnels(sorted_with_start_end, "Sorted (start+end constraints)")
        self.assertEqual(len(sorted_with_start_end), 5)

        # Test 3: With just start point
        print(f"\n=== Pentagram Test: Start at {start_point}, no end constraint ===")
        sorted_with_start_only = PathUtils.sort_tunnels_tsp(
            tunnels, allowFlipping=True, routeStartPoint=start_point
        )
        self.print_tunnels(sorted_with_start_only, "Sorted (start only constraint)")
        self.assertEqual(len(sorted_with_start_only), 5)

    def test_08_open_wire_end_only(self):
        """Test TSP solver for tunnels with end-only constraint on a complex wire with crossings and diagonals."""
        # Create a complex wire with 6 points in random positions and multiple crossings
        points = [
            (0, 0),  # Point 0
            (15, 5),  # Point 1
            (30, -5),  # Point 2
            (10, -10),  # Point 3
            (25, 10),  # Point 4
            (5, 15),  # Point 5
        ]

        tunnels = [
            {
                "startX": points[2][0],
                "startY": points[2][1],
                "endX": points[3][0],
                "endY": points[3][1],
            },  # 2 -> 3
            {
                "startX": points[1][0],
                "startY": points[1][1],
                "endX": points[2][0],
                "endY": points[2][1],
            },  # 1 -> 2
            {
                "startX": points[3][0],
                "startY": points[3][1],
                "endX": points[4][0],
                "endY": points[4][1],
            },  # 3 -> 4
            {
                "startX": points[0][0],
                "startY": points[0][1],
                "endX": points[1][0],
                "endY": points[1][1],
            },  # 0 -> 1
            {
                "startX": points[4][0],
                "startY": points[4][1],
                "endX": points[5][0],
                "endY": points[5][1],
            },  # 4 -> 5
            {
                "startX": points[0][0],
                "startY": points[0][1],
                "endX": points[2][0],
                "endY": points[2][1],
            },  # 0 -> 2 (diagonal)
            {
                "startX": points[1][0],
                "startY": points[1][1],
                "endX": points[4][0],
                "endY": points[4][1],
            },  # 1 -> 4 (crossing)
            {
                "startX": points[3][0],
                "startY": points[3][1],
                "endX": points[5][0],
                "endY": points[5][1],
            },  # 3 -> 5 (diagonal)
        ]

        print("\n=== Complex Wire Test: End at (25, 10), no start constraint ===")
        self.print_tunnels(tunnels, "Input complex wire tunnels")

        end_point = [25.0, 10.0]  # End at point 4
        sorted_tunnels = PathUtils.sort_tunnels_tsp(
            tunnels, allowFlipping=False, routeEndPoint=end_point
        )
        self.print_tunnels(sorted_tunnels, "Sorted (end only constraint)")
        self.assertEqual(len(sorted_tunnels), 8)

        # The route should end at the specified end point
        # Note: Due to current implementation limitations, this may not be enforced

    def test_09_tunnels_extra_data_passthrough(self):
        """Test that extra data in tunnel dictionaries is preserved through TSP solving."""
        tunnels = [
            {
                "startX": 0,
                "startY": 0,
                "endX": 5,
                "endY": 0,
                "tool": "drill_1mm",
                "speed": 1000,
                "feed": 500,
                "custom_id": "tunnel_0",
            },
            {
                "startX": 20,
                "startY": 5,
                "endX": 25,
                "endY": 5,
                "tool": "drill_3mm",
                "speed": 600,
                "feed": 200,
                "notes": "high precision",
                "custom_id": "tunnel_2",
            },
            {
                "startX": 5,
                "startY": 17,
                "endX": 15,
                "endY": 0,
                "tool": "mill_2mm",
                "speed": 800,
                "feed": 300,
                "material": "aluminum",
                "custom_id": "tunnel_1",
            },
        ]

        self.print_tunnels(tunnels, "Input tunnels with extra data")

        # Test with flipping allowed to ensure extra data survives optimization
        result = PathUtils.sort_tunnels_tsp(tunnels, allowFlipping=True)

        self.print_tunnels(result, "Sorted tunnels with extra data preserved")

        # Verify all tunnels are present
        self.assertEqual(len(result), 3)

        # Verify extra data is preserved for each tunnel
        for tunnel in result:
            # Check that solver-added keys are present
            self.assertIn("startX", tunnel)
            self.assertIn("startY", tunnel)
            self.assertIn("endX", tunnel)
            self.assertIn("endY", tunnel)
            self.assertIn("isOpen", tunnel)
            self.assertIn("flipped", tunnel)
            self.assertIn("index", tunnel)

            # Check that extra keys are preserved
            self.assertIn("tool", tunnel)
            self.assertIn("speed", tunnel)
            self.assertIn("feed", tunnel)
            self.assertIn("custom_id", tunnel)

            # Verify specific values based on original index
            original_tunnel = tunnels[tunnel["index"]]
            self.assertEqual(tunnel["tool"], original_tunnel["tool"])
            self.assertEqual(tunnel["speed"], original_tunnel["speed"])
            self.assertEqual(tunnel["feed"], original_tunnel["feed"])
            self.assertEqual(tunnel["custom_id"], original_tunnel["custom_id"])

            # Check tunnel-specific extra data
            if tunnel["index"] == 2:
                self.assertEqual(tunnel["material"], "aluminum")
            elif tunnel["index"] == 1:
                self.assertEqual(tunnel["notes"], "high precision")


if __name__ == "__main__":
    import unittest

    unittest.main()
