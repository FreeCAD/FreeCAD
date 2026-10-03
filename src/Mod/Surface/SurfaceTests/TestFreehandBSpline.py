# SPDX-License-Identifier: LGPL-2.1-or-later

import os
import tempfile
import unittest

import FreeCAD as App
import Part
import Surface


class TestFreehandBSpline(unittest.TestCase):
    def setUp(self):
        self.doc = App.newDocument("CurveNetworkTest")

    def tearDown(self):
        App.closeDocument(self.doc.Name)

    def curve(self, points):
        obj = self.doc.addObject("Surface::FreehandBSpline", "Curve")
        obj.Points = [App.Vector(*p) for p in points]
        self.doc.recompute()
        self.assertTrue(obj.isValid(), obj.getStatusString())
        return obj

    def test_interpolation_and_placement(self):
        curve = self.curve([(0, 0, 0), (3, 4, 1), (8, 1, 4), (10, 0, 0)])
        self.assertEqual(len(curve.Shape.Edges), 1)
        for p in curve.Points:
            self.assertLess(curve.Shape.distToShape(Part.Vertex(p))[0], 1e-6)
        placement = App.Placement(App.Vector(4, 6, 8), App.Rotation(App.Vector(1, 1, 0), 33))
        curve.Placement = placement
        points = curve.Points
        points[1] = App.Vector(3, 5, 1)
        curve.Points = points
        self.doc.recompute()
        for p in points:
            self.assertLess(curve.Shape.distToShape(Part.Vertex(placement.multVec(p)))[0], 1e-6)

    def test_persistent_point_supports(self):
        curve = self.curve([(0, 0, 0), (5, 8, 0), (10, 0, 0), (15, -4, 0)])
        support = self.doc.addObject("Part::Feature", "Support")
        support.Shape = Part.makeLine(App.Vector(0, 2, 0), App.Vector(20, 2, 0))
        curve.Support = [(support, ("Vertex1",)), (support, ("Edge1",))]
        curve.SupportPointIndices = [0, 1]
        curve.SupportParameters = [App.Vector(), App.Vector(0.25, 0, 0)]
        self.doc.recompute()
        self.assertEqual(curve.Points[0], App.Vector(0, 2, 0))
        self.assertEqual(curve.Points[1], App.Vector(5, 2, 0))
        support.Shape = Part.makeLine(App.Vector(0, 2, 0), App.Vector(40, 2, 0))
        support.Placement.Base = App.Vector(3, 4, 5)
        self.doc.recompute()
        self.assertEqual(curve.Points[0], App.Vector(3, 6, 5))
        self.assertEqual(curve.Points[1], App.Vector(13, 6, 5))
        with tempfile.TemporaryDirectory() as directory:
            path = os.path.join(directory, "LockedSpline.FCStd")
            self.doc.saveAs(path)
            App.closeDocument(self.doc.Name)
            self.doc = App.openDocument(path)
            restored = self.doc
            try:
                restored.Support.Placement.Base = App.Vector(3, 9, 5)
                restored.recompute()
                self.assertEqual(restored.Curve.Points[1], App.Vector(13, 11, 5))
                self.assertTrue(restored.Curve.isValid())
            finally:
                App.closeDocument(restored.Name)
                self.doc = App.newDocument("CurveNetworkTest")

    def test_point_support_inside_body(self):
        body = self.doc.addObject("PartDesign::Body", "Body")
        sketch = self.doc.addObject("Sketcher::SketchObject", "Sketch")
        body.addObject(sketch)
        corners = [
            App.Vector(0, 0, 0),
            App.Vector(10, 0, 0),
            App.Vector(10, 10, 0),
            App.Vector(0, 10, 0),
        ]
        for i in range(4):
            sketch.addGeometry(Part.LineSegment(corners[i], corners[(i + 1) % 4]), False)
        pad = body.newObject("PartDesign::Pad", "Pad")
        pad.Profile = sketch
        pad.Length = 5
        self.doc.recompute()
        vertex = next(i for i, v in enumerate(pad.Shape.Vertexes) if v.Point.z > 4)
        curve = self.curve([(0, 0, 0), (15, 15, 10), (20, 0, 0)])
        curve.Support = [(pad, ("Vertex%d" % (vertex + 1),))]
        curve.SupportPointIndices = [0]
        curve.SupportParameters = [App.Vector()]
        self.doc.recompute()
        self.assertEqual(curve.Points[0], pad.Shape.Vertexes[vertex].Point)
        self.assertTrue(curve.isValid(), curve.getStatusString())

    def test_reference_tangents(self):
        curve = self.curve([(0, 0, 0), (5, 8, 3), (15, 0, 0)])
        support = self.doc.addObject("Part::Feature", "TangentEdge")
        support.Shape = Part.makeLine(App.Vector(), App.Vector(10, 0, 0))
        curve.Support = [(support, ("Vertex1",))]
        curve.SupportPointIndices = [0]
        curve.SupportParameters = [App.Vector()]
        curve.TangentSupport = [(support, ("Edge1",))]
        curve.TangentPointIndices = [0]
        self.doc.recompute()
        self.assertTrue(curve.isValid(), curve.getStatusString())
        edge = curve.Shape.Edges[0]
        tangent = edge.tangentAt(edge.FirstParameter)
        self.assertLess(tangent.cross(App.Vector(1, 0, 0)).Length, 1e-7)
        # Two interpolation points must also honor the requested endpoint direction.
        curve.Points = [App.Vector(), App.Vector(10, 10, 0)]
        self.doc.recompute()
        edge = curve.Shape.Edges[0]
        self.assertLess(edge.tangentAt(edge.FirstParameter).cross(App.Vector(1, 0, 0)).Length, 1e-7)
        self.assertGreater(edge.Length, (curve.Points[1] - curve.Points[0]).Length)
        with tempfile.TemporaryDirectory() as directory:
            path = os.path.join(directory, "TangentSpline.FCStd")
            self.doc.saveAs(path)
            App.closeDocument(self.doc.Name)
            self.doc = App.openDocument(path)
            self.doc.recompute()
            self.assertEqual(self.doc.Curve.TangentPointIndices, [0])
            edge = self.doc.Curve.Shape.Edges[0]
            self.assertLess(
                edge.tangentAt(edge.FirstParameter).cross(App.Vector(1, 0, 0)).Length, 1e-7
            )

    def test_face_tangent_plane(self):
        curve = self.curve([(2, 2, 0), (5, 8, 3), (15, 0, 6)])
        face = self.doc.addObject("Part::Feature", "TangentFace")
        face.Shape = Part.makePlane(20, 20)
        curve.Support = [(face, ("Face1",))]
        curve.SupportPointIndices = [0]
        curve.SupportParameters = [App.Vector(2, 2, 0)]
        curve.TangentSupport = [(face, ("Face1",))]
        curve.TangentPointIndices = [0]
        self.doc.recompute()
        self.assertTrue(curve.isValid(), curve.getStatusString())
        edge = curve.Shape.Edges[0]
        self.assertAlmostEqual(edge.tangentAt(edge.FirstParameter).z, 0, places=7)
        curve.LinearSegments = [True, False]
        self.doc.recompute()
        self.assertFalse(curve.isValid())

    def test_deleted_reference_unlocks_only_affected_points(self):
        curve = self.curve([(0, 0, 0), (5, 8, 0), (10, 0, 0)])
        first = self.doc.addObject("Part::Feature", "FirstSupport")
        first.Shape = Part.makeLine(App.Vector(), App.Vector(0, 10, 0))
        second = self.doc.addObject("Part::Feature", "SecondSupport")
        second.Shape = Part.makeLine(App.Vector(10, 0, 0), App.Vector(20, 0, 0))
        curve.Support = [(first, ("Vertex1",)), (second, ("Vertex1",))]
        curve.SupportPointIndices = [0, 2]
        curve.SupportParameters = [App.Vector(), App.Vector()]
        curve.TangentSupport = [(first, ("Edge1",)), (second, ("Edge1",))]
        curve.TangentPointIndices = [0, 2]
        self.doc.recompute()
        original = list(curve.Points)
        self.doc.UndoMode = 1
        self.doc.openTransaction("Delete support")
        self.doc.removeObject(first.Name)
        self.doc.recompute()
        self.doc.commitTransaction()
        self.assertEqual(curve.SupportPointIndices, [2])
        self.assertEqual(curve.TangentPointIndices, [2])
        self.assertEqual(curve.Points, original)
        self.assertTrue(curve.isValid(), curve.getStatusString())
        self.doc.undo()
        self.doc.recompute()
        self.assertEqual(curve.SupportPointIndices, [0, 2])
        self.assertEqual(curve.TangentPointIndices, [0, 2])
        self.doc.redo()
        self.doc.recompute()
        self.assertEqual(curve.SupportPointIndices, [2])
        second.Placement.Base = App.Vector(0, 0, 4)
        self.doc.recompute()
        self.assertEqual(curve.Points[0], original[0])
        self.assertEqual(curve.Points[2], original[2] + App.Vector(0, 0, 4))

    def test_missing_subelement_unlocks(self):
        curve = self.curve([(0, 0, 0), (5, 8, 0), (10, 0, 0)])
        support = self.doc.addObject("Part::Feature", "Support")
        support.Shape = Part.makeLine(App.Vector(), App.Vector(0, 10, 0))
        curve.Support = [(support, ("Vertex999",))]
        curve.SupportPointIndices = [0]
        curve.SupportParameters = [App.Vector()]
        curve.TangentSupport = [(support, ("Edge1",))]
        curve.TangentPointIndices = [0]
        before = list(curve.Points)
        self.doc.recompute()
        self.assertTrue(curve.isValid(), curve.getStatusString())
        self.assertEqual(curve.SupportPointIndices, [])
        self.assertEqual(curve.TangentPointIndices, [])
        self.assertEqual(curve.Points, before)

    def test_periodic_and_linear(self):
        curve = self.curve([(0, 0, 0), (5, 8, 0), (10, 0, 0), (5, -4, 0)])
        curve.Periodic = True
        self.doc.recompute()
        self.assertTrue(curve.Shape.isClosed())
        self.assertTrue(curve.Shape.Edges[0].Curve.isPeriodic())
        curve.LinearSegments = [True, False, True, False]
        self.doc.recompute()
        self.assertTrue(curve.isValid(), curve.getStatusString())
        self.assertTrue(curve.Shape.isClosed())
        for i in (0, 2):
            for step in range(11):
                p = curve.Points[i] + (curve.Points[i + 1] - curve.Points[i]) * step / 10
                self.assertLess(curve.Shape.distToShape(Part.Vertex(p))[0], 1e-6)

    def test_linear_corner(self):
        curve = self.curve([(0, 0, 0), (10, 0, 0), (10, 10, 0), (20, 10, 0)])
        curve.LinearSegments = [True, True, True]
        self.doc.recompute()
        self.assertTrue(curve.isValid(), curve.getStatusString())
        self.assertAlmostEqual(curve.Shape.Length, 30, places=6)

    def test_duplicate_points_are_rejected(self):
        curve = self.curve([(0, 0, 0), (10, 0, 0)])
        curve.Points = [App.Vector(), App.Vector()]
        self.doc.recompute()
        self.assertFalse(curve.isValid())
