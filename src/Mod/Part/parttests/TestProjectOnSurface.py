# SPDX-License-Identifier: LGPL-2.1-or-later

import unittest

import FreeCAD as App
import Part


class TestProjectOnSurface(unittest.TestCase):
    """
    Regression test for github issues #27000 and #20203"""

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


if __name__ == "__main__":
    unittest.main()
