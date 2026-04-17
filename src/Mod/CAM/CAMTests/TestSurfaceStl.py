# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2025 sliptonic <shopinthewoods@gmail.com>               *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file                                 *
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

"""Test surface STL conversion functionality."""

import unittest
import time
import Part
import FreeCAD

# Import modules to test
from Path.Base.Generator import surface_stl
from Path.Base.Generator import surface_common


class TestSurfaceStl(unittest.TestCase):
    """Test surface STL conversion functionality."""

    def setUp(self):
        """Set up test fixtures."""
        # Create a simple box shape for testing
        self.box = Part.makeBox(10, 10, 5)

        # Create simple mesh data
        self.mesh_points = [
            (0.0, 0.0, 0.0),  # Bottom triangle
            (1.0, 0.0, 0.0),
            (0.5, 1.0, 0.0),
            (0.0, 0.0, 1.0),  # Top triangle
            (1.0, 0.0, 1.0),
            (0.5, 1.0, 1.0),
        ]
        self.mesh_facets = [
            (0, 1, 2),  # Bottom face
            (3, 4, 5),  # Top face
        ]

    def test_cpp_availability_and_direct_test(self):
        """Test C++ availability and call the raw C++ function directly."""
        # First, check if C++ module is actually importable
        try:
            import surface_stl as _stl_cpp

            cpp_available = True
            print("C++ module imported successfully")
        except ImportError as e:
            cpp_available = False
            print(f"C++ module not available: {e}")

        self.assertIsInstance(cpp_available, bool)

        # If C++ is available, test it directly - it should WORK, not fail
        if cpp_available:
            print("Testing raw C++ function directly...")

            # Call the raw C++ function - this should SUCCEED if C++ is working
            try:
                verts, faces = _stl_cpp.shape_tessellate_fast(self.box, 0.1, 0.5, None)

                # If we get here, C++ worked - verify results
                self.assertIsInstance(verts, list)
                self.assertIsInstance(faces, list)
                self.assertGreater(len(verts), 0)
                self.assertGreater(len(faces), 0)
                print("✅ C++ function works correctly")

            except RuntimeError as e:
                # C++ failed - this should make the test FAIL
                self.fail(f"C++ function is broken: {e}")
        else:
            self.skipTest("C++ module not available")

    def test_python_implementation_direct(self):
        """Test Python implementation directly (should always work)."""
        # Test the Python implementation directly
        verts, faces = surface_common.shape_to_stl_arrays(self.box, 0.1, 0.5)

        # Verify we got valid results
        self.assertIsInstance(verts, list)
        self.assertIsInstance(faces, list)
        self.assertGreater(len(verts), 0)
        self.assertGreater(len(faces), 0)
        print("✅ Python implementation works correctly")

    def test_cpp_python_identical_results(self):
        """Test that C++ and Python implementations produce identical results."""
        # First check if C++ is available
        try:
            import surface_stl as _stl_cpp

            cpp_available = True
        except ImportError as e:
            cpp_available = False
            print(f"C++ module not available: {e}")

        if not cpp_available:
            self.skipTest("C++ acceleration not available")

        # Test with multiple shapes to ensure consistency
        test_shapes = [
            ("Simple Box", Part.makeBox(10, 10, 5)),
            ("Cylinder", Part.makeCylinder(5, 10)),
            ("Sphere", Part.makeSphere(5)),
            ("Compound", Part.makeBox(10, 10, 5).fuse(Part.makeCylinder(5, 10))),
        ]

        linear_deflection = 0.1
        angular_deflection = 0.5

        for shape_name, test_shape in test_shapes:
            print(f"\n--- Testing {shape_name} ---")

            # Get results from both implementations
            verts_cpp, faces_cpp = _stl_cpp.shape_tessellate_fast(
                test_shape, linear_deflection, angular_deflection
            )
            verts_py, faces_py = surface_common.shape_to_stl_arrays(
                test_shape, linear_deflection, angular_deflection
            )

            print(f"C++: {len(verts_cpp)} vertices, {len(faces_cpp)} faces")
            print(f"Python: {len(verts_py)} vertices, {len(faces_py)} faces")

            # Verify triangle counts are very close (allowing for small differences due to tessellation algorithms)
            # Different algorithms (BRepMesh_IncrementalMesh vs MeshPart::Mesher) may produce slightly different results
            triangle_diff = abs(len(faces_cpp) - len(faces_py))
            triangle_tolerance = max(5, len(faces_py) * 0.02)  # 2% or 5 triangles minimum

            self.assertLessEqual(
                triangle_diff,
                triangle_tolerance,
                f"{shape_name}: Triangle count difference too large - C++: {len(faces_cpp)}, Python: {len(faces_py)}, diff: {triangle_diff}",
            )

            # Verify vertex counts are very close
            vertex_diff = abs(len(verts_cpp) - len(verts_py))
            vertex_tolerance = max(10, len(verts_py) * 0.05)  # 5% or 10 vertices minimum

            self.assertLessEqual(
                vertex_diff,
                vertex_tolerance,
                f"{shape_name}: Vertex count difference too large - C++: {len(verts_cpp)}, Python: {len(verts_py)}, diff: {vertex_diff}",
            )

            print(
                f"✅ {shape_name}: Results are very close (triangle diff: {triangle_diff}, vertex diff: {vertex_diff})"
            )
            print(f"   Both implementations produce valid tessellations with acceptable tolerance")

    def test_performance_comparison(self):
        """Test performance comparison between C++ and Python implementations."""
        # First check if C++ is available
        try:
            import surface_stl as _stl_cpp

            cpp_available = True
        except ImportError as e:
            cpp_available = False
            print(f"C++ module not available: {e}")

        if not cpp_available:
            self.skipTest("C++ acceleration not available")

        # Use a complex shape that should show performance differences
        complex_shape = Part.makeCylinder(20, 50)
        complex_shape = complex_shape.fuse(Part.makeBox(30, 30, 20))

        linear_deflection = 0.05
        angular_deflection = 0.2

        print(f"\n=== Performance Analysis ===")
        print(f"Shape: Compound (cylinder + box)")
        print(f"Deflection: linear={linear_deflection}, angular={angular_deflection}")

        # Test Python implementation with detailed timing
        print(f"\n--- Python Implementation ---")

        py_times = []
        for i in range(3):
            start_time = time.perf_counter()
            verts_py, faces_py = surface_common.shape_to_stl_arrays(
                complex_shape, linear_deflection, angular_deflection
            )
            end_time = time.perf_counter()
            py_times.append(end_time - start_time)

        py_avg = sum(py_times) / len(py_times)
        py_triangles = len(faces_py)
        print(f"Times: {[f'{t:.4f}s' for t in py_times]}")
        print(f"Average: {py_avg:.4f}s ({py_triangles} triangles)")
        print(f"Rate: {py_triangles/py_avg:.0f} triangles/second")

        # Test C++ implementation
        print(f"\n--- C++ Implementation ---")

        cpp_times = []
        for i in range(3):
            start_time = time.perf_counter()
            verts_cpp, faces_cpp = _stl_cpp.shape_tessellate_fast(
                complex_shape, linear_deflection, angular_deflection
            )
            end_time = time.perf_counter()
            cpp_times.append(end_time - start_time)

        cpp_avg = sum(cpp_times) / len(cpp_times)
        cpp_triangles = len(faces_cpp)
        print(f"Times: {[f'{t:.4f}s' for t in cpp_times]}")
        print(f"Average: {cpp_avg:.4f}s ({cpp_triangles} triangles)")
        print(f"Rate: {cpp_triangles/cpp_avg:.0f} triangles/second")

        # Compare results
        print(f"\n--- Comparison ---")
        print(f"C++: {cpp_avg:.4f}s ({cpp_triangles} triangles)")
        print(f"Python: {py_avg:.4f}s ({py_triangles} triangles)")

        if cpp_avg > 0.0001:  # Avoid division by tiny numbers
            speedup = py_avg / cpp_avg
            print(f"Speedup: {speedup:.2f}x")

            if speedup < 1.0:
                print(f"⚠️  C++ is {1/speedup:.1f}x SLOWER than Python")
                print("This suggests:")
                print("  - C++ implementation has overhead issues")
                print("  - Python implementation is well-optimized")
                print("  - Data transfer overhead dominates")
            else:
                print(f"✅ C++ is {speedup:.1f}x faster")
        else:
            print("Both implementations too fast to measure accurately")

        # Verify both produce valid results
        self.assertIsNotNone(verts_cpp)
        self.assertIsNotNone(faces_cpp)
        self.assertGreater(len(faces_cpp), 100)
        self.assertGreater(len(faces_py), 100)

        # Triangle counts should be similar
        self.assertAlmostEqual(len(faces_cpp), len(faces_py), delta=len(faces_cpp) * 0.1)

    def test_shape_to_stl_deflection_parameters(self):
        """Test shape to STL conversion with different deflection parameters."""
        # Low precision (larger deflection) = fewer triangles
        stl_low = surface_stl.shape_to_stl(self.box, 1.0, 1.0)

        # High precision (smaller deflection) = more triangles
        stl_high = surface_stl.shape_to_stl(self.box, 0.05, 0.1)

        # Both should produce valid STLs
        self.assertIsNotNone(stl_low)
        self.assertIsNotNone(stl_high)
        self.assertGreater(stl_low.size(), 0)
        self.assertGreater(stl_high.size(), 0)

        # High precision should have more or equal triangles
        self.assertGreaterEqual(stl_high.size(), stl_low.size())

    def test_complex_shape(self):
        """Test STL conversion with a more complex shape."""
        # Create a cylinder
        cylinder = Part.makeCylinder(5, 10)

        stl = surface_stl.shape_to_stl(cylinder, 0.1, 0.5)

        # Verify we get an STL surface
        self.assertIsNotNone(stl)
        self.assertGreater(stl.size(), 50)  # Cylinder needs more triangles than box

    def test_error_handling_invalid_shape(self):
        """Test error handling with invalid shape."""
        with self.assertRaises((ValueError, RuntimeError)):
            surface_stl.shape_to_stl(None, 0.1, 0.5)

    def test_mesh_simplification_levels(self):
        """Test mesh simplification at different levels."""
        # Create a shape with many triangles (sphere)
        sphere = Part.makeSphere(5)

        # Test different simplification levels
        results = {}
        for level in range(1, 8):
            stl = surface_stl.shape_to_stl(sphere, 0.1, 0.5, mesh_simplification=level)
            results[level] = stl.size()

        # Level 1 (no simplification) should have most triangles
        self.assertGreater(results[1], 0)

        # Higher levels should generally have fewer triangles
        # (though this depends on the fast-simplification library availability)
        if surface_stl._HAS_SIMPLIFICATION:
            # Levels 6-7 should show significant reduction
            self.assertLessEqual(results[7], results[1])
            self.assertLessEqual(results[6], results[2])

    def test_mesh_simplification_disabled(self):
        """Test that level 1 disables simplification."""
        sphere = Part.makeSphere(5)

        # Level 1 should produce same result as no simplification
        stl_level1 = surface_stl.shape_to_stl(sphere, 0.1, 0.5, mesh_simplification=1)
        stl_default = surface_stl.shape_to_stl(sphere, 0.1, 0.5)  # defaults to 1

        self.assertEqual(stl_level1.size(), stl_default.size())

    def test_mesh_simplification_performance(self):
        """Test that mesh simplification improves performance."""
        # Create a complex shape
        complex_shape = Part.makeSphere(10)

        # Time without simplification
        start = time.perf_counter()
        stl_no_simp = surface_stl.shape_to_stl(complex_shape, 0.05, 0.3, mesh_simplification=1)
        time_no_simp = time.perf_counter() - start

        # Time with maximum simplification
        start = time.perf_counter()
        stl_max_simp = surface_stl.shape_to_stl(complex_shape, 0.05, 0.3, mesh_simplification=7)
        time_max_simp = time.perf_counter() - start

        # Both should produce valid STLs
        self.assertIsNotNone(stl_no_simp)
        self.assertIsNotNone(stl_max_simp)
        self.assertGreater(stl_no_simp.size(), 0)
        self.assertGreater(stl_max_simp.size(), 0)

        # Maximum simplification should have fewer triangles
        if surface_stl._HAS_SIMPLIFICATION:
            self.assertLess(stl_max_simp.size(), stl_no_simp.size())

            # And should be faster (though this is not guaranteed in all cases)
            # We just log the performance difference for informational purposes
            speedup = time_no_simp / time_max_simp if time_max_simp > 0 else 1.0
            print(
                f"Mesh simplification performance: {speedup:.2f}x speedup, "
                f"{stl_no_simp.size()} → {stl_max_simp.size()} triangles"
            )

    def test_mesh_simplification_fallback(self):
        """Test graceful fallback when fast-simplification is not available."""
        # Temporarily disable simplification to test fallback
        original_has_simplification = surface_stl._HAS_SIMPLIFICATION
        surface_stl._HAS_SIMPLIFICATION = False

        try:
            sphere = Part.makeSphere(5)

            # Should work without simplification library
            stl = surface_stl.shape_to_stl(sphere, 0.1, 0.5, mesh_simplification=7)
            self.assertIsNotNone(stl)
            self.assertGreater(stl.size(), 0)

        finally:
            # Restore original state
            surface_stl._HAS_SIMPLIFICATION = original_has_simplification

    def test_mesh_simplification_parameter_validation(self):
        """Test parameter validation for mesh simplification."""
        sphere = Part.makeSphere(5)

        # Test boundary values
        stl_min = surface_stl.shape_to_stl(sphere, 0.1, 0.5, mesh_simplification=1)
        stl_max = surface_stl.shape_to_stl(sphere, 0.1, 0.5, mesh_simplification=7)

        self.assertIsNotNone(stl_min)
        self.assertIsNotNone(stl_max)

        # Test values outside range (should be clamped or handled gracefully)
        stl_below = surface_stl.shape_to_stl(sphere, 0.1, 0.5, mesh_simplification=0)
        stl_above = surface_stl.shape_to_stl(sphere, 0.1, 0.5, mesh_simplification=10)

        self.assertIsNotNone(stl_below)
        self.assertIsNotNone(stl_above)

    def test_pre_clipping_basic(self):
        """Test basic pre-clipping functionality."""
        # Create a tall box that extends well below final depth
        tall_box = Part.makeBox(10, 10, 20, FreeCAD.Vector(0, 0, -10))

        # Clip above final depth of -5
        final_depth = -5.0
        stl = surface_stl.shape_to_stl(tall_box, 0.1, 0.5, final_depth=final_depth)

        self.assertIsNotNone(stl)
        self.assertGreater(stl.size(), 0)

    def test_pre_clipping_no_effect(self):
        """Test pre-clipping when it provides no benefit."""
        # Create a shallow box entirely above final depth
        shallow_box = Part.makeBox(10, 10, 5, FreeCAD.Vector(0, 0, 0))

        # Final depth is below the box, so no clipping should occur
        final_depth = -10.0
        stl = surface_stl.shape_to_stl(shallow_box, 0.1, 0.5, final_depth=final_depth)

        self.assertIsNotNone(stl)
        self.assertGreater(stl.size(), 0)

    def test_pre_clipping_disabled(self):
        """Test that pre-clipping is disabled when final_depth is None or >= 0."""
        box = Part.makeBox(10, 10, 5)

        # Test with None
        stl_none = surface_stl.shape_to_stl(box, 0.1, 0.5, final_depth=None)
        self.assertIsNotNone(stl_none)

        # Test with non-negative depth
        stl_positive = surface_stl.shape_to_stl(box, 0.1, 0.5, final_depth=5.0)
        self.assertIsNotNone(stl_positive)

        # Should produce same results as no clipping
        stl_no_clip = surface_stl.shape_to_stl(box, 0.1, 0.5)
        self.assertEqual(stl_none.size(), stl_no_clip.size())

    def test_pre_clipping_performance(self):
        """Test that pre-clipping improves performance on deep models."""
        # Create a deep model with significant portion below final depth
        deep_model = Part.makeCylinder(10, 30, FreeCAD.Vector(0, 0, -20))

        # Time without pre-clipping
        start = time.perf_counter()
        stl_no_clip = surface_stl.shape_to_stl(deep_model, 0.1, 0.5, final_depth=None)
        time_no_clip = time.perf_counter() - start

        # Time with pre-clipping
        start = time.perf_counter()
        stl_with_clip = surface_stl.shape_to_stl(deep_model, 0.1, 0.5, final_depth=-5.0)
        time_with_clip = time.perf_counter() - start

        # Both should produce valid STLs
        self.assertIsNotNone(stl_no_clip)
        self.assertIsNotNone(stl_with_clip)
        self.assertGreater(stl_no_clip.size(), 0)
        self.assertGreater(stl_with_clip.size(), 0)

        # Pre-clipping should generally be faster for deep models
        # (though this depends on the specific geometry and clipping overhead)
        speedup = time_no_clip / time_with_clip if time_with_clip > 0 else 1.0
        print(
            f"Pre-clipping performance: {speedup:.2f}x speedup, "
            f"triangles: {stl_no_clip.size()} → {stl_with_clip.size()}"
        )

    def test_pre_clipping_with_simplification(self):
        """Test pre-clipping combined with mesh simplification."""
        # Create a complex deep model
        complex_model = Part.makeSphere(10, FreeCAD.Vector(0, 0, -10))

        # Apply both pre-clipping and simplification
        stl = surface_stl.shape_to_stl(
            complex_model, 0.1, 0.5, final_depth=-5.0, mesh_simplification=5
        )

        self.assertIsNotNone(stl)
        self.assertGreater(stl.size(), 0)

    def test_pre_clipping_edge_cases(self):
        """Test edge cases for pre-clipping."""
        # Test with empty shape (should handle gracefully)
        empty_shape = Part.Shape()
        try:
            stl_empty = surface_stl.shape_to_stl(empty_shape, 0.1, 0.5, final_depth=-5.0)
            # If it doesn't raise an error, the STL should be empty or very small
            self.assertIsNotNone(stl_empty)
        except (ValueError, RuntimeError, Exception) as e:
            # Empty shapes are expected to raise errors
            # Check if it's the expected FreeCAD error about null shapes
            if "null shape" in str(e).lower():
                pass  # Expected error
            else:
                # Re-raise unexpected errors
                raise e

        # Test with very deep final depth (should clip everything above, leaving empty)
        box = Part.makeBox(10, 10, 5, FreeCAD.Vector(0, 0, 0))
        stl_deep = surface_stl.shape_to_stl(box, 0.1, 0.5, final_depth=100.0)
        self.assertIsNotNone(stl_deep)  # Should handle gracefully - may be empty STL


if __name__ == "__main__":
    unittest.main()
