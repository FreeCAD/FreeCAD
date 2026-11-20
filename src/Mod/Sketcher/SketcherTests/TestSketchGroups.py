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
        groups = [
            self.group(self.handles[0], [self.circle]),
            self.group(self.handles[1], [self.handles[0], self.other]),
            self.group(self.handles[2], [self.handles[1]]),
        ]
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

    def ungroup(self, ids):
        # Deleting a constraint never deletes geometry: like the GUI, collect the handles of
        # the groups first, delete the constraints, then the handles.
        s = self.sketch
        handles = [s.Constraints[i].First for i in ids]
        s.delConstraints(ids)
        s.delGeometries(handles)

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
                    self.assertVector(
                        s.Geometry[geo].StartPoint, target - 2 * (before[geo].StartPoint - origin)
                    )
                    self.assertVector(
                        s.Geometry[geo].EndPoint, target - 2 * (before[geo].EndPoint - origin)
                    )
                self.assertVector(
                    s.Geometry[self.circle].Center,
                    target - 2 * (before[self.circle].Center - origin),
                )
                self.assertAlmostEqual(s.Geometry[self.circle].Radius, 4)

    def testUngroupOuterPreservesInner(self):
        self.nested()
        s = self.sketch
        self.ungroup([2])
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
        self.ungroup([0, 1])
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
        self.assertVector(
            s.Geometry[self.circle].Center, origin + 1.5 * App.Vector(-delta.y, delta.x, 0)
        )
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

    def testDeleteGroupWithOtherElements(self):
        # Deleting a Group constraint deleted its handle and the constraints on it as well, so
        # the indices a caller had collected for the rest of its selection pointed elsewhere.
        # This replays what the 3D view sends for "Group + Horizontal + line B", in its order.
        s = self.sketch
        handle = self.line(0)
        member = s.addGeometry(Part.LineSegment(App.Vector(0, 1, 0), App.Vector(10, 5, 0)))
        lineB = s.addGeometry(Part.LineSegment(App.Vector(20, 0, 0), App.Vector(30, 3, 0)))
        lineC = s.addGeometry(Part.LineSegment(App.Vector(40, 0, 0), App.Vector(50, 1, 0)))
        s.addConstraint(Sketcher.Constraint("Distance", handle, 10.0))
        horizontal = s.addConstraint(Sketcher.Constraint("Horizontal", lineC))
        s.addConstraint(Sketcher.Constraint("Distance", lineC, 10.0))
        group = s.addConstraint(self.group(handle, [member]))
        self.assertEqual(s.solve(), 0)

        s.delConstraint(group, True)
        self.assertEqual(s.GeometryCount, 4)
        self.assertEqual([c.Type for c in s.Constraints], ["Distance", "Horizontal", "Distance"])
        s.delConstraint(horizontal, True)
        s.delGeometries([handle, lineB], True)

        self.assertEqual(s.GeometryCount, 2)
        self.assertVector(s.Geometry[0].StartPoint, App.Vector(0, 1, 0))
        self.assertVector(s.Geometry[1].StartPoint, App.Vector(40, 0, 0))
        self.assertEqual([(c.Type, c.First) for c in s.Constraints], [("Distance", 1)])
        self.assertEqual(s.solve(), 0)

    def testDeleteEverythingWithGroup(self):
        # Select all, Delete: the constraints go first, then every geometry by its GeoId.
        s = self.sketch
        handle = self.line(0)
        circle = s.addGeometry(Part.Circle(App.Vector(3, 3, 0), App.Vector(0, 0, 1), 2))
        s.addGeometry(Part.LineSegment(App.Vector(20, 0, 0), App.Vector(30, 5, 0)))
        s.addGeometry(Part.LineSegment(App.Vector(40, 0, 0), App.Vector(50, 5, 0)))
        s.addConstraint(self.group(handle, [circle]))
        self.assertEqual(s.solve(), 0)

        s.delConstraint(0, True)
        s.delGeometries([0, 1, 2, 3], True)
        self.assertEqual((s.GeometryCount, s.ConstraintCount), (0, 0))

    def flatGroup(self):
        # A group with more than two members: the handle and the first two members fit in
        # the First/Second/Third slots, the last member only exists in the elements list.
        s = self.sketch
        handle = self.line(10)
        members = [
            s.addGeometry(Part.LineSegment(App.Vector(12, 0, 0), App.Vector(15, 3, 0))),
            s.addGeometry(Part.Circle(App.Vector(13, 6, 0), App.Vector(0, 0, 1), 1)),
            s.addGeometry(Part.LineSegment(App.Vector(16, 0, 0), App.Vector(17, 3, 0))),
        ]
        s.addConstraint(self.group(handle, members))
        self.assertEqual(s.solve(), 0)
        return [handle] + members

    def testCopyKeepsWholeGroup(self):
        # Issue #32374: copying a group must give a second group made of the copies.
        s = self.sketch
        geos = self.flatGroup()
        s.addCopy(geos, App.Vector(30, 0, 0), False)
        self.assertEqual(sum(c.Type == "Group" for c in s.Constraints), 2)
        self.assertEqual(s.solve(), 0)

        # A copy that leaves the handle out does not create a group.
        s.addCopy(geos[1:], App.Vector(0, 30, 0), False)
        self.assertEqual(sum(c.Type == "Group" for c in s.Constraints), 2)
        self.assertEqual(s.solve(), 0)

    def testMirrorKeepsWholeGroup(self):
        # Issue #32374: mirroring a group must give a second group made of the mirrored
        # geometries, and not share the original members with it.
        s = self.sketch
        geos = self.flatGroup()
        mirrored = s.addSymmetric(geos, -2)
        self.assertEqual(sum(c.Type == "Group" for c in s.Constraints), 2)
        self.assertEqual(s.solve(), 0)

        last, mirroredLast = geos[-1], mirrored[-1]
        before = s.Geometry[last].StartPoint
        mirroredBefore = s.Geometry[mirroredLast].StartPoint
        # Keep the mirrored handle rigid so dragging it translates its group.
        s.addConstraint(Sketcher.Constraint("Vertical", mirrored[0]))
        s.addConstraint(Sketcher.Constraint("Distance", mirrored[0], 10.0))
        self.assertEqual(s.solve(), 0)
        delta = App.Vector(-5, 4, 0)
        handleStart = s.Geometry[mirrored[0]].StartPoint
        s.moveGeometry(mirrored[0], 1, handleStart + delta, 0)
        self.assertEqual(s.solve(), 0)
        self.assertVector(s.Geometry[mirroredLast].StartPoint, mirroredBefore + delta)
        self.assertVector(s.Geometry[last].StartPoint, before)

    def testMirrorWithSymmetryConstraintsKeepsWholeGroup(self):
        # The Mirror tool asks for symmetry constraints by default: the mirrored geometries
        # must be a group then too, and follow the original through the handles.
        s = self.sketch
        geos = self.flatGroup()
        mirrored = s.addSymmetric(geos, -2, 0, True)
        self.assertEqual(sum(c.Type == "Group" for c in s.Constraints), 2)
        self.assertEqual(s.solve(), 0)
        self.assertEqual(s.MalformedConstraints, [])

        last, mirroredLast = geos[-1], mirrored[-1]
        before = s.Geometry[last].StartPoint
        mirroredBefore = s.Geometry[mirroredLast].StartPoint
        self.assertVector(mirroredBefore, App.Vector(-before.x, before.y, 0))
        # Keep the handle rigid so dragging it translates its group.
        s.addConstraint(Sketcher.Constraint("Vertical", geos[0]))
        s.addConstraint(Sketcher.Constraint("Distance", geos[0], 10.0))
        self.assertEqual(s.solve(), 0)
        delta = App.Vector(3, 4, 0)
        s.moveGeometry(geos[0], 1, s.Geometry[geos[0]].StartPoint + delta, 0)
        self.assertEqual(s.solve(), 0)
        self.assertVector(s.Geometry[last].StartPoint, before + delta)
        self.assertVector(
            s.Geometry[mirroredLast].StartPoint,
            mirroredBefore + App.Vector(-delta.x, delta.y, 0),
        )

    def assertValidIndices(self):
        count = self.sketch.GeometryCount
        for c in self.sketch.Constraints:
            for geo in (c.First, c.Second, c.Third):
                self.assertTrue(geo == -2000 or -2 <= geo < count, (c.Type, geo))

    def assertGroupFollowsHandle(self, handle, geos):
        # Keep the handle rigid so dragging it translates the whole group.
        s = self.sketch
        s.addConstraint(Sketcher.Constraint("Vertical", handle))
        s.addConstraint(Sketcher.Constraint("Distance", handle, s.Geometry[handle].length()))
        self.assertEqual(s.solve(), 0)
        before = [s.Geometry[g].StartPoint for g in geos]
        delta = App.Vector(-5, 4, 0)
        s.moveGeometry(handle, 1, s.Geometry[handle].StartPoint + delta, 0)
        self.assertEqual(s.solve(), 0)
        for geo, point in zip(geos, before):
            self.assertVector(s.Geometry[geo].StartPoint, point + delta)

    def testSplitMemberKeepsGroup(self):
        # Splitting a member deleted the group and its handle in the middle of the operation,
        # so the split geometry was replaced at a stale index.
        s = self.sketch
        handle, first, circle, last = self.flatGroup()
        s.split(last, App.Vector(16.5, 1.5, 0))
        self.assertEqual(s.GeometryCount, 5)
        self.assertValidIndices()
        self.assertEqual(sum(c.Type == "Group" for c in s.Constraints), 1)
        self.assertVector(s.Geometry[handle].StartPoint, App.Vector(10, 0, 0))
        self.assertVector(s.Geometry[first].StartPoint, App.Vector(12, 0, 0))
        self.assertAlmostEqual(s.Geometry[circle].Radius, 1)
        self.assertVector(s.Geometry[last].StartPoint, App.Vector(16, 0, 0))
        self.assertVector(s.Geometry[4].EndPoint, App.Vector(17, 3, 0))
        self.assertEqual(s.solve(), 0)
        self.assertGroupFollowsHandle(handle, [first, last, 4])

    def testSplitHandleKeepsMembers(self):
        # A click on a group member selects the handle, so splitting the handle must not delete
        # a member in its place.
        s = self.sketch
        handle, first, circle, last = self.flatGroup()
        s.split(handle, App.Vector(10, 5, 0))
        self.assertEqual(s.GeometryCount, 5)
        self.assertValidIndices()
        self.assertEqual(sum(c.Type == "Group" for c in s.Constraints), 1)
        self.assertVector(s.Geometry[handle].StartPoint, App.Vector(10, 0, 0))
        self.assertVector(s.Geometry[first].StartPoint, App.Vector(12, 0, 0))
        self.assertAlmostEqual(s.Geometry[circle].Radius, 1)
        self.assertVector(s.Geometry[last].StartPoint, App.Vector(16, 0, 0))
        self.assertEqual(s.solve(), 0)

    def testTrimMemberKeepsGroup(self):
        s = self.sketch
        handle, first, circle, last = self.flatGroup()
        cutter = s.addGeometry(Part.LineSegment(App.Vector(15, 1.5, 0), App.Vector(20, 1.5, 0)))
        s.trim(last, App.Vector(16.2, 0.6, 0))
        self.assertEqual(s.GeometryCount, 5)
        self.assertValidIndices()
        self.assertEqual(sum(c.Type == "Group" for c in s.Constraints), 1)
        self.assertVector(s.Geometry[handle].StartPoint, App.Vector(10, 0, 0))
        self.assertVector(s.Geometry[first].StartPoint, App.Vector(12, 0, 0))
        self.assertAlmostEqual(s.Geometry[circle].Radius, 1)
        self.assertVector(s.Geometry[last].StartPoint, App.Vector(16.5, 1.5, 0))
        self.assertVector(s.Geometry[cutter].StartPoint, App.Vector(15, 1.5, 0))

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
