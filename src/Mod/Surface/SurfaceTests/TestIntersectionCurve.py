# SPDX-License-Identifier: LGPL-2.1-or-later

"""Regression tests for extruded-profile intersection curves."""

import importlib
import os
import tempfile
import unittest

import FreeCAD as App
import Part

# Register the native document object types used by these tests.
importlib.import_module("Sketcher")
importlib.import_module("Surface")


class TestIntersectionCurve(unittest.TestCase):
    """Exercise geometry, recompute errors, persistence and element naming."""

    def setUp(self):
        self.doc = App.newDocument("TestIntersectionCurve")

    def tearDown(self):
        App.closeDocument(self.doc.Name)

    def profiles(self):
        """Create two perpendicular profiles and their intersection."""
        first = self.doc.addObject("Sketcher::SketchObject", "First")
        first.addGeometry(Part.LineSegment(App.Vector(0, 0, 0), App.Vector(10, 10, 0)))
        second = self.doc.addObject("Sketcher::SketchObject", "Second")
        second.addGeometry(Part.LineSegment(App.Vector(0, 0, 0), App.Vector(10, 20, 0)))
        second.Placement.Rotation = App.Rotation(App.Vector(1, 0, 0), 90)
        curve = self.doc.addObject("Surface::IntersectionCurve", "IntersectionCurve")
        curve.Curve1 = first
        curve.Curve2 = second
        self.doc.recompute()
        return first, second, curve

    def assertCurve(self, curve):
        """Check that the result is a valid wire without faces."""
        self.assertNotIn("Invalid", curve.State)
        self.assertFalse(curve.Shape.isNull())
        self.assertTrue(curve.Shape.isValid())
        self.assertEqual(len(curve.Shape.Wires), 1)
        self.assertEqual(len(curve.Shape.Faces), 0)

    def test_sketch_normals_and_recompute(self):
        """Infer sketch normals and update the intersection after moving a profile."""
        _, second, curve = self.profiles()
        self.assertCurve(curve)
        self.assertAlmostEqual(curve.Shape.Length, App.Vector(10, 10, 20).Length, places=6)
        second.Placement.Base = App.Vector(0, 0, 1000)
        self.doc.recompute()
        self.assertCurve(curve)
        self.assertAlmostEqual(curve.Shape.BoundBox.ZMin, 1000, places=5)
        self.assertAlmostEqual(curve.Shape.BoundBox.ZMax, 1020, places=5)

    def test_element_names_survive_recompute(self):
        """Preserve mapped edge and vertex names through the complete shape pipeline."""
        _, second, curve = self.profiles()
        element_map = curve.Shape.ElementMap
        self.assertEqual(set(element_map.values()), {"Edge1", "Vertex1", "Vertex2"})
        second.Placement.Base = App.Vector(0, 0, 5)
        self.doc.recompute()
        self.assertCurve(curve)
        self.assertEqual(curve.Shape.ElementMap, element_map)

    def test_explicit_directions_for_edges(self):
        """Use explicit extrusion directions for straight edges."""
        first = self.doc.addObject("Part::Feature", "First")
        first.Shape = Part.makeLine(App.Vector(), App.Vector(10, 10, 0))
        second = self.doc.addObject("Part::Feature", "Second")
        second.Shape = Part.makeLine(App.Vector(), App.Vector(10, 0, 20))
        curve = self.doc.addObject("Surface::IntersectionCurve", "IntersectionCurve")
        curve.Curve1, curve.Curve2 = first, second
        curve.Direction1 = App.Vector(0, 0, 1)
        curve.Direction2 = App.Vector(0, 1, 0)
        self.doc.recompute()
        self.assertCurve(curve)
        self.assertAlmostEqual(curve.Shape.Length, App.Vector(10, 10, 20).Length, places=6)

    def test_bspline_wires(self):
        """Keep the intersection on both extruded spline profiles."""
        profiles = []
        for points in (
            [App.Vector(0, 0, 0), App.Vector(5, 4, 0), App.Vector(10, 0, 0)],
            [App.Vector(0, 0, 1), App.Vector(5, 0, 6), App.Vector(10, 0, 2)],
        ):
            spline = Part.BSplineCurve()
            spline.interpolate(points)
            profile = self.doc.addObject("Part::Feature", "Profile")
            profile.Shape = Part.Wire(spline.toShape())
            profiles.append(profile)
        curve = self.doc.addObject("Surface::IntersectionCurve", "IntersectionCurve")
        curve.Curve1 = profiles[0]
        curve.Curve2 = profiles[1]
        self.doc.recompute()
        self.assertCurve(curve)
        for point in curve.Shape.discretize(20):
            xy = Part.Vertex(App.Vector(point.x, point.y, 0))
            xz = Part.Vertex(App.Vector(point.x, 0, point.z))
            self.assertLess(xy.distToShape(profiles[0].Shape)[0], 1e-5)
            self.assertLess(xz.distToShape(profiles[1].Shape)[0], 1e-5)

    def test_invalid_inputs_clear_result(self):
        """Clear stale geometry when profiles become invalid."""
        first, second, curve = self.profiles()
        second.Placement.Rotation = App.Rotation()
        self.doc.recompute()
        self.assertIn("Invalid", curve.State)
        self.assertTrue(curve.Shape.isNull())
        curve.Curve2 = first
        self.doc.recompute()
        self.assertIn("Invalid", curve.State)
        curve.Curve2 = None
        self.doc.recompute()
        self.assertIn("Invalid", curve.State)

    def test_no_intersection(self):
        """Report disjoint extrusions without retaining an old shape."""
        _, second, curve = self.profiles()
        second.Placement.Base = App.Vector(100, 0, 0)
        self.doc.recompute()
        self.assertIn("Invalid", curve.State)
        self.assertTrue(curve.Shape.isNull())

    def test_oblique_directions(self):
        """Extend extrusions far enough for nearly parallel directions."""
        _, second, curve = self.profiles()
        second.Placement = App.Placement(App.Vector(0, 10, 0), App.Rotation())
        curve.Direction1 = App.Vector(0, 0, 1)
        curve.Direction2 = App.Vector(0, 0.01, 1)
        self.doc.recompute()
        self.assertCurve(curve)
        self.assertAlmostEqual(curve.Shape.BoundBox.ZMin, -2000, places=5)
        self.assertAlmostEqual(curve.Shape.BoundBox.ZMax, -1000, places=5)

    def test_disconnected_intersections(self):
        """Preserve all disconnected intersection branches."""
        first, _, curve = self.profiles()
        first.delGeometry(0)
        for start, end in ((0, 4), (6, 10)):
            first.addGeometry(
                Part.LineSegment(App.Vector(start, start, 0), App.Vector(end, end, 0))
            )
        self.doc.recompute()
        self.assertNotIn("Invalid", curve.State)
        self.assertTrue(curve.Shape.isValid())
        self.assertEqual(len(curve.Shape.Wires), 2)

    def test_empty_and_face_inputs(self):
        """Reject empty profiles and profiles containing faces."""
        _, _, curve = self.profiles()
        invalid = self.doc.addObject("Part::Feature", "InvalidProfile")
        curve.Curve1 = invalid
        self.doc.recompute()
        self.assertIn("Invalid", curve.State)
        self.assertTrue(curve.Shape.isNull())
        invalid.Shape = Part.makePlane(10, 10)
        self.doc.recompute()
        self.assertIn("Invalid", curve.State)
        self.assertTrue(curve.Shape.isNull())

    def test_save_restore(self):
        """Restore linked profiles and recompute a saved intersection."""
        self.profiles()
        element_map = self.doc.IntersectionCurve.Shape.ElementMap
        with tempfile.TemporaryDirectory() as directory:
            path = os.path.join(directory, "Intersection.FCStd")
            self.doc.saveAs(path)
            App.closeDocument(self.doc.Name)
            self.doc = App.openDocument(path)
            self.doc.Second.Placement.Base = App.Vector(0, 0, 5)
            self.doc.recompute()
            self.assertCurve(self.doc.IntersectionCurve)
            self.assertEqual(self.doc.IntersectionCurve.Shape.ElementMap, element_map)
            self.assertAlmostEqual(self.doc.IntersectionCurve.Shape.BoundBox.ZMin, 5, places=5)
            # Release the document's file handles before removing the temporary directory.
            App.closeDocument(self.doc.Name)
            self.doc = App.newDocument("TestIntersectionCurve")
