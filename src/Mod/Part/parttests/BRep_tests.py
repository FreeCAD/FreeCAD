import FreeCAD as App
import Part

import unittest

class BRepTests(unittest.TestCase):

    def testProject(self):
        """
        This is a unit test for PR #13507
        """
        num = 18
        alt = [0,1] * num
        pts = [App.Vector(i, alt[i], 0) for i in range(num)]

        bsc = Part.BSplineCurve()
        bsc.buildFromPoles(pts, False, 1)
        edge = bsc.toShape()

        rts = Part.RectangularTrimmedSurface(Part.Plane(), -50, 50, -50, 50)
        plane_shape = rts.toShape()

        proj = plane_shape.project([edge])
        self.assertFalse(proj.isNull())
        self.assertEqual(len(proj.Edges), 1)

