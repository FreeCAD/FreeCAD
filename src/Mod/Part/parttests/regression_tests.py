from FreeCAD import Vector, newDocument, closeDocument
import Part

import unittest

class RegressionTests(unittest.TestCase):

    def test_issue_4456(self):
        """
        0004456: Regression : Part.Plane.Intersect does not accept plane as argument
        """
        p1 = Part.Plane()
        p2 = Part.Plane(Vector(0, 0, 0), Vector(1, 0, 0))

        result = p1.intersect(p2)
        line = result.pop()
        self.assertEqual(line.Location, Vector(0, 0, 0))
        self.assertEqual(line.Direction, Vector(0, 1, 0))
        # We should now have empty list...
        with self.assertRaises(IndexError):
            result.pop()

    def test_OptimalBox(self):
        box = Part.makeBox(1, 1, 1)
        self.assertTrue(box.optimalBoundingBox(True, False).isValid())

    def test_CircularReference(self):
        # put me in def setup(self): ?
        self.Doc = newDocument("TestSketchExpr")

        cube = self.Doc.addObject("Part::Box","Cube")
        cube.setExpression('Length', 'Width + 10mm')
        with self.assertRaises(RuntimeError) as context:
            cube.setExpression('Width', 'Length + 10mm')
        assert "Width reference creates a cyclic dependency." in str(context.exception)

        cube.setExpression('.Placement.Base.x', '.Placement.Base.y + 10mm')
        with self.assertRaises(RuntimeError) as context:
            cube.setExpression('.Placement.Base.y', '.Placement.Base.x + 10mm')
        assert ".Placement.Base.y reference creates a cyclic dependency." in str(context.exception)

        cube.recompute()
        v1 = cube.Placement.Base
        cube.recompute()
        assert cube.Placement.Base.isEqual(v1,1e-6)

        # Put me in def tearDown(self): ?
        closeDocument(self.Doc.Name)

