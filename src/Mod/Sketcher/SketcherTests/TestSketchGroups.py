# SPDX-License-Identifier: LGPL-2.1-or-later

import math
import os
import tempfile
import unittest

import FreeCAD as App
import Part
import Sketcher


class TestSketchGroups(unittest.TestCase):
    def setUp(self):
        self.doc = App.newDocument("NestedGroupsTest")
        self.sketch = self.doc.addObject("Sketcher::SketchObject", "Sketch")

    def tearDown(self):
        App.closeDocument(self.doc.Name)

    def line(self, x, y=0):
        return self.sketch.addGeometry(
            Part.LineSegment(App.Vector(x, y, 0), App.Vector(x, y + 10, 0)), True
        )

    def group(self, handle, members):
        elements = [handle, 0]
        for member in members:
            elements.extend([member, 0])
        return Sketcher.Constraint("Group", elements)

    def nested(self, reverse=False):
        s = self.sketch
        self.handles = [self.line(x) for x in (10, 20, 30)]
        self.circle = s.addGeometry(Part.Circle(App.Vector(12, 4, 0), App.Vector(0, 0, 1), 2))
        self.other = s.addGeometry(Part.LineSegment(App.Vector(23, 2, 0), App.Vector(27, 5, 0)))
        groups = [self.group(self.handles[0], [self.circle]),
                  self.group(self.handles[1], [self.handles[0], self.other]),
                  self.group(self.handles[2], [self.handles[1]])]
        s.addConstraint(list(reversed(groups)) if reverse else groups)
        self.assertEqual(s.solve(), 0)
        root = self.handles[2]
        self.x = s.addConstraint(Sketcher.Constraint("DistanceX", root, 1, 30.0))
        self.y = s.addConstraint(Sketcher.Constraint("DistanceY", root, 1, 1.0))
        self.angle = s.addConstraint(Sketcher.Constraint("Angle", root, math.pi / 2))
        self.length = s.addConstraint(Sketcher.Constraint("Distance", root, 10.0))
        self.assertEqual(s.solve(), 0)

    def assertVector(self, actual, expected):
        self.assertLess((actual - expected).Length, 1e-6)

    def testTransformAndRepeatedSolve(self):
        for reverse in (False, True):
            if reverse:
                self.sketch = self.doc.addObject("Sketcher::SketchObject", "ReverseOrder")
            self.nested(reverse)
            s = self.sketch
            before = s.Geometry
            origin = before[self.handles[2]].StartPoint
            s.setDatum(self.length, 20.0)
            s.setDatum(self.angle, 3 * math.pi / 2)
            s.setDatum(self.x, 50.0)
            s.setDatum(self.y, 11.0)
            target = App.Vector(50, 11, 0)
            for _ in range(3):
                self.assertEqual(s.solve(), 0)
                for geo in self.handles[:2] + [self.other]:
                    self.assertVector(s.Geometry[geo].StartPoint,
                                      target - 2 * (before[geo].StartPoint - origin))
                    self.assertVector(s.Geometry[geo].EndPoint,
                                      target - 2 * (before[geo].EndPoint - origin))
                self.assertVector(s.Geometry[self.circle].Center,
                                  target - 2 * (before[self.circle].Center - origin))
                self.assertAlmostEqual(s.Geometry[self.circle].Radius, 4)

    def testUngroupOuterPreservesInner(self):
        self.nested()
        s = self.sketch
        s.delConstraint(2)
        self.assertEqual(s.solve(), 0)
        self.assertEqual(sum(c.Type == "Group" for c in s.Constraints), 2)
        self.assertEqual(s.GeometryCount, 4)
        s.addConstraint(Sketcher.Constraint("Distance", self.handles[1], 20.0))
        self.assertEqual(s.solve(), 0)
        self.assertAlmostEqual(s.Geometry[self.circle - 1].Radius, 4)

    def testSaveRestore(self):
        self.nested()
        with tempfile.TemporaryDirectory() as directory:
            path = os.path.join(directory, "Nested.FCStd")
            self.doc.recompute()
            self.doc.saveAs(path)
            App.closeDocument(self.doc.Name)
            self.doc = App.openDocument(path)
            self.sketch = self.doc.getObject("Sketch")
            self.assertEqual(self.sketch.solve(), 0)
            self.sketch.setDatum(self.length, 20.0)
            self.assertEqual(self.sketch.solve(), 0)
            self.assertAlmostEqual(self.sketch.Geometry[self.circle].Radius, 4)

    def testInvalidHierarchy(self):
        for case in ("self", "cycle", "shared", "duplicate_handle", "duplicate_member"):
            s = self.doc.addObject("Sketcher::SketchObject", case)
            self.sketch = s
            a, b, c = [self.line(x) for x in (0, 20, 40)]
            definitions = {
                "self": [self.group(a, [a])],
                "cycle": [self.group(a, [b]), self.group(b, [a])],
                "shared": [self.group(a, [c]), self.group(b, [c])],
                "duplicate_handle": [self.group(a, [b]), self.group(a, [c])],
                "duplicate_member": [self.group(a, [b, b])],
            }
            s.addConstraint(definitions[case])
            self.assertEqual(s.solve(), -5, case)

    def testDeactivateOuter(self):
        self.nested()
        s = self.sketch
        s.toggleActive(2)
        self.assertEqual(s.solve(), 0)
        before = s.Geometry[self.circle].Center
        s.setDatum(self.length, 20.0)
        self.assertVector(s.Geometry[self.circle].Center, before)

    def testUngroupInnerAndUndo(self):
        self.nested()
        s = self.sketch
        self.doc.openTransaction("Ungroup inner levels")
        s.delConstraints([0, 1])
        self.doc.commitTransaction()
        self.assertEqual(s.solve(), 0)
        s.setDatum(self.length - 2, 20.0)
        self.assertEqual(s.GeometryCount, 3)
        self.assertAlmostEqual(s.Geometry[self.circle - 2].Radius, 4)
        self.doc.undo()
        self.assertEqual(s.solve(), 0)
        self.assertEqual(sum(c.Type == "Group" for c in s.Constraints), 3)

    def testExpressionAndQuarterTurn(self):
        self.nested()
        s = self.sketch
        before = s.Geometry[self.circle].Center
        origin = s.Geometry[self.handles[2]].StartPoint
        s.addProperty("App::PropertyLength", "GroupLength")
        s.GroupLength = 15
        s.setExpression("Constraints[{}]".format(self.length), "GroupLength")
        self.doc.recompute()
        s.setDatum(self.angle, math.pi)
        delta = before - origin
        self.assertVector(s.Geometry[self.circle].Center,
                          origin + 1.5 * App.Vector(-delta.y, delta.x, 0))
        self.assertAlmostEqual(s.Geometry[self.circle].Radius, 3)

    def testChildDimensionsAreDormant(self):
        self.nested()
        s = self.sketch
        child_length = s.addConstraint(Sketcher.Constraint("Distance", self.handles[0], 10.0))
        self.assertEqual(s.solve(), 0)
        s.setDatum(self.length, 20.0)
        self.assertAlmostEqual(s.Geometry[self.handles[0]].length(), 20)
        self.assertAlmostEqual(s.Constraints[child_length].Value, 10)
        s.delConstraints([1, 2])
        self.assertEqual(s.solve(), 0)
        self.assertAlmostEqual(s.Geometry[self.handles[0]].length(), 10)

    def testDragOuterHandle(self):
        self.nested()
        s = self.sketch
        s.delConstraints([self.x, self.y])
        self.assertEqual(s.solve(), 0)
        root = self.handles[2]
        before = s.Geometry[self.circle].Center
        origin = s.Geometry[root].StartPoint
        for delta in (App.Vector(15, 15, 0), App.Vector(-10, -10, 0)):
            s.moveGeometry(root, 1, origin + delta, 0)
            self.assertEqual(s.solve(), 0)
            self.assertVector(s.Geometry[self.circle].Center, before + delta)

    def testDeepHierarchy(self):
        s = self.sketch
        circle = s.addGeometry(Part.Circle(App.Vector(1, 2, 0), App.Vector(0, 0, 1), 2))
        child = circle
        constraints = []
        for i in range(100):
            handle = self.line(10 + i)
            constraints.append(self.group(handle, [child]))
            child = handle
        s.addConstraint(list(reversed(constraints)))
        self.assertEqual(s.solve(), 0)
        s.addConstraint(Sketcher.Constraint("Distance", handle, 20.0))
        self.assertEqual(s.solve(), 0)
        self.assertAlmostEqual(s.Geometry[circle].Radius, 4)


if __name__ == "__main__":
    unittest.main()
