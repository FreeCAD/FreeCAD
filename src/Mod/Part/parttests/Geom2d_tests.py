import FreeCAD
vec2 = FreeCAD.Base.Vector2d
import Part

import unittest

class Geom2dTests(unittest.TestCase):

    def test_toShape(self):
        surf = Part.Cylinder()

        p1 = vec2(1.0, -1.0)
        p2 = vec2(2.0, -1.0)

        l12 = Part.Geom2d.Line2dSegment(p1, p2)

        e1 = l12.toShape()
        e2 = l12.toShape(surf)
        self.assertNotEqual(e1.curveOnSurface(0), None)
        self.assertNotEqual(e2.curveOnSurface(0), None)

    def test_insertKnot(self):
        with self.assertRaises(TypeError):
            curve = Part.Geom2d.BSplineCurve2d()
            curve.insertKnot(0.5, 1, 0.01, 2)
