#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-2.1-or-later

import unittest

import FreeCAD as App
import Part
import PartDesign  # noqa: F401
import Sketcher  # noqa: F401


class TestProjectOnSurface(unittest.TestCase):
    def setUp(self):
        self.doc = App.newDocument("PartDesignProjectOnSurfaceTest")

    def tearDown(self):
        App.closeDocument(self.doc.Name)

    def make_source_sketch(self):
        body = self.doc.addObject("PartDesign::Body", "Body")
        sketch = body.newObject("Sketcher::SketchObject", "SourceSketch")
        sketch.addGeometry(
            [
                Part.LineSegment(App.Vector(-2, -2), App.Vector(2, -2)),
                Part.LineSegment(App.Vector(2, -2), App.Vector(2, 2)),
                Part.LineSegment(App.Vector(2, 2), App.Vector(-2, 2)),
                Part.LineSegment(App.Vector(-2, 2), App.Vector(-2, -2)),
            ],
            False,
        )
        # Rotate the sketch plane so its normal points along the global X axis.
        sketch.Placement = App.Placement(
            App.Vector(10, 0, 5), App.Rotation(App.Vector(0, 1, 0), 90)
        )
        return body, sketch

    def test_sketch_normal_projects_onto_curved_face(self):
        body, sketch = self.make_source_sketch()
        cylinder = body.newObject("PartDesign::AdditiveCylinder", "Cylinder")
        cylinder.Radius = 5
        cylinder.Height = 10

        projection = body.newObject("PartDesign::ProjectOnSurface", "Projection")
        # An empty sub-name deliberately passes the entire Sketch to Part. This
        # exercises Part's compound traversal as well as its sketch-plane normal.
        projection.Projection = [(sketch, [""])]
        projection.SupportFaces = [(cylinder, ["Face1"])]
        self.doc.recompute()

        self.assertEqual(projection.Mode, "All")
        self.assertTrue(projection.AutoDirection)
        self.assertTrue(projection.isValid(), projection.getStatusString())
        self.assertGreater(len(projection.Shape.Edges), 0)
        self.assertIn(projection, body.Group)
        self.assertEqual(body.Tip, cylinder)

    def test_multiple_curved_target_faces(self):
        body, sketch = self.make_source_sketch()
        cylinders = []
        for index, x_position in enumerate((0, -15), start=1):
            cylinder = self.doc.addObject("Part::Cylinder", f"Cylinder{index}")
            cylinder.Radius = 5
            cylinder.Height = 10
            cylinder.Placement.Base.x = x_position
            cylinders.append(cylinder)

        projection = body.newObject("PartDesign::ProjectOnSurface", "Projection")
        projection.Projection = [(sketch, [""])]
        projection.SupportFaces = [(cylinder, ["Face1"]) for cylinder in cylinders]
        self.doc.recompute()

        self.assertEqual(projection.Mode, "All")
        self.assertTrue(projection.AutoDirection)
        self.assertTrue(projection.isValid(), projection.getStatusString())
        self.assertEqual(len(projection.SupportFaces), 2)
        self.assertGreaterEqual(len(projection.Shape.Edges), 8)


if __name__ == "__main__":
    unittest.main()
