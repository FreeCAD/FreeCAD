# SPDX-License-Identifier: LGPL-2.1-or-later

import math
import unittest

import FreeCAD as App
import Part


class TestProjectOnSurface(unittest.TestCase):
    """Face projection regressions, including GitHub issues #27000 and #20203."""

    def setUp(self):
        self.doc = App.newDocument("PartProjectOnSurfaceTest")

    def tearDown(self):
        App.closeDocument(self.doc.Name)

    def test_face_reconstruction_across_sphere_singularity(self):
        # make sphere
        source = self.doc.addObject("Part::Feature", "SourceFace")
        source.Shape = Part.makePlane(4, 4, App.Vector(-2, -2, 10))

        sphere = self.doc.addObject("Part::Sphere", "Sphere")
        sphere.Radius = 5

        projection = self.doc.addObject("Part::ProjectOnSurface", "Projection")
        projection.Projection = [(source, ["Face1"])]
        projection.SupportFace = (sphere, ["Face1"])
        projection.Direction = App.Vector(0, 0, -1)
        self.doc.recompute()

        self.assertTrue(projection.isValid(), projection.getStatusString())
        self.assertTrue(projection.Shape.isValid())
        self.assertEqual(len(projection.Shape.Faces), 1)

        # should be about 17 square units
        self.assertGreater(projection.Shape.Area, 15)
        self.assertLess(projection.Shape.Area, 20)

    def _project_face(self, source_shape, target_shape, direction):
        source = self.doc.addObject("Part::Feature", "SourceFace")
        source.Shape = source_shape
        target = self.doc.addObject("Part::Feature", "TargetFace")
        target.Shape = target_shape
        projection = self.doc.addObject("Part::ProjectOnSurface", "Projection")
        projection.Projection = [(source, ["Face1"])]
        projection.SupportFace = (target, ["Face1"])
        projection.Direction = direction
        self.doc.recompute()

        self.assertTrue(projection.isValid(), projection.getStatusString())
        self.assertFalse(projection.Shape.isNull())
        self.assertTrue(projection.Shape.isValid())
        self.assertEqual(len(projection.Shape.Faces), 1)
        self.assertEqual(len(projection.Shape.Solids), 0)
        return projection.Shape

    def _assert_bounds(self, shape, expected):
        bounds = shape.optimalBoundingBox(False)
        for name, value in zip(("XMin", "XMax", "YMin", "YMax", "ZMin", "ZMax"), expected):
            with self.subTest(bound=name):
                self.assertAlmostEqual(getattr(bounds, name), value, delta=1e-5)

    def _check_cylindrical_projection(self, seam_rotation):
        source = Part.Face(
            Part.makePolygon(
                [
                    App.Vector(10, -2, 2),
                    App.Vector(10, 2, 2),
                    App.Vector(10, 2, 6),
                    App.Vector(10, -2, 6),
                    App.Vector(10, -2, 2),
                ]
            )
        )
        cylinder = Part.makeCylinder(5, 8)
        cylinder.rotate(App.Vector(0, 0, 0), App.Vector(0, 0, 1), seam_rotation)
        target = next(face for face in cylinder.Faces if isinstance(face.Surface, Part.Cylinder))
        shape = self._project_face(source, target, App.Vector(-1, 0, 0))

        # Require the whole near-side patch, not one seam fragment or the far-side hit.
        self.assertAlmostEqual(shape.Area, 40 * math.asin(2 / 5), delta=1e-5)
        self._assert_bounds(shape, (math.sqrt(21), 5, -2, 2, 2, 6))

    def test_face_projection_across_cylindrical_seam(self):
        self._check_cylindrical_projection(0)

    def test_face_projection_away_from_cylindrical_seam(self):
        self._check_cylindrical_projection(90)

    def test_source_face_hole_is_preserved(self):
        outer = Part.makePlane(6, 6, App.Vector(-3, -3, 10))
        inner = Part.makePlane(2, 2, App.Vector(-1, -1, 10))
        source = outer.cut(inner).Faces[0]
        target = Part.makePlane(10, 10, App.Vector(-5, -5, 0))
        shape = self._project_face(source, target, App.Vector(0, 0, -1))

        self.assertAlmostEqual(shape.Area, 32, delta=1e-5)
        self.assertEqual(len(shape.Faces[0].Wires), 2)
        self._assert_bounds(shape, (-3, 3, -3, 3, 0, 0))
        hole = Part.makePlane(2, 2, App.Vector(-1, -1, 0))
        self.assertAlmostEqual(shape.common(hole).Area, 0, delta=1e-5)

    def test_target_face_hole_is_preserved(self):
        source = Part.makePlane(6, 6, App.Vector(-3, -3, 10))
        outer = Part.makePlane(10, 10, App.Vector(-5, -5, 0))
        hole = Part.makePlane(2, 2, App.Vector(0, -1, 0))
        target = outer.cut(hole).Faces[0]
        shape = self._project_face(source, target, App.Vector(0, 0, -1))

        self.assertAlmostEqual(shape.Area, 32, delta=1e-5)
        self.assertEqual(len(shape.Faces[0].Wires), 2)
        self._assert_bounds(shape, (-3, 3, -3, 3, 0, 0))
        self.assertAlmostEqual(shape.common(hole).Area, 0, delta=1e-5)

    def test_partial_overlap_with_bounded_target(self):
        source = Part.makePlane(4, 4, App.Vector(0, 0, 10))
        target = Part.makePlane(4, 4, App.Vector(2, 1, 0))
        shape = self._project_face(source, target, App.Vector(0, 0, -1))

        self.assertAlmostEqual(shape.Area, 6, delta=1e-5)
        self.assertEqual(len(shape.Faces[0].Wires), 1)
        self._assert_bounds(shape, (2, 4, 1, 4, 0, 0))


if __name__ == "__main__":
    unittest.main()
