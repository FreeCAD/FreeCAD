import FreeCAD
from FreeCAD import Vector, Base, newDocument, closeDocument
import Part
if "BUILD_SKETCHER" in FreeCAD.__cmake__:
    import Sketcher

import unittest

class RegressionTests(unittest.TestCase):

    # pylint: disable=attribute-defined-outside-init

    def setUp(self):
        """Create a document for each test in the test suite"""
        self.Doc = newDocument("PartRegressionTest." + self._testMethodName)
        self.KeepTestDoc = False

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

    def test_issue_15735(self):
        """
        15735: Point in sketch as loft profile won't work in dev, but works in stable
        The following test is a simplified version of the issue, but the outcome is the same
        """

        if "BUILD_SKETCHER" in FreeCAD.__cmake__:
            # Arrange
            ArcSketch = self.Doc.addObject("Sketcher::SketchObject", "ArcSketch")
            ArcSketch.Placement = Base.Placement(
                Base.Vector(0.000000, 0.000000, 0.000000),
                Base.Rotation(0.500000, 0.500000, 0.500000, 0.500000),
            )
            ArcSketch.MapMode = "Deactivated"

            geoList = []
            geoList.append(
                Part.ArcOfCircle(
                    Part.Circle(
                        Base.Vector(0.000000, 0.000000, 0.000000),
                        Base.Vector(0.000000, 0.000000, 1.000000),
                        10.000000,
                    ),
                    3.141593,
                    6.283185,
                )
            )
            ArcSketch.addGeometry(geoList, False)
            del geoList

            constraintList = []
            ArcSketch.addConstraint(Sketcher.Constraint("Radius", 0, 10.000000))
            constraintList.append(Sketcher.Constraint("Coincident", 0, 3, -1, 1))
            constraintList.append(Sketcher.Constraint("PointOnObject", 0, 2, -1))
            constraintList.append(Sketcher.Constraint("PointOnObject", 0, 1, -1))
            ArcSketch.addConstraint(constraintList)
            del constraintList

            self.Doc.recompute()

            PointSketch = self.Doc.addObject("Sketcher::SketchObject", "PointSketch")
            PointSketch.Placement = Base.Placement(
                Base.Vector(-10.000000, 0.000000, 0.000000),
                Base.Rotation(0.500000, 0.500000, 0.500000, 0.500000),
            )
            PointSketch.MapMode = "Deactivated"

            PointSketch.addGeometry(Part.Point(Base.Vector(0.000000, 0.000000, 0)))

            PointSketch.addConstraint(Sketcher.Constraint("Coincident", 0, 1, -1, 1))

            self.Doc.recompute()

            Loft = self.Doc.addObject("Part::Loft", "Loft")
            Loft.Sections = [
                ArcSketch,
                PointSketch,
            ]
            Loft.Solid = False
            Loft.Ruled = False
            Loft.Closed = False

            # Act
            self.Doc.recompute()

            # Assert
            self.assertTrue(Loft.isValid())
            self.KeepTestDoc = not Loft.isValid()

    def test_OptimalBox(self):
        box = Part.makeBox(1, 1, 1)
        self.assertTrue(box.optimalBoundingBox(True, False).isValid())

    def test_CircularReference(self):

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

    def tearDown(self):
        """Clean up our test, optionally preserving the test document"""
        # This flag allows doing something like this:
        #   self.KeepTestDoc = True
        #   import TestApp
        #   TestApp.Test("TestPartApp.RegressionTests.test_issue_15735")
        # to leave the test document(s) around for further examination in an interactive setting.
        if hasattr(self, "KeepTestDoc") and self.KeepTestDoc:
            return
        closeDocument(self.Doc.Name)
