# SPDX-License-Identifier: LGPL-2.1-or-later

import FreeCAD
import Part
import Sketcher

from SketcherTests.GuiTestCase import SketcherGuiTestCase, GUI_MODULE_AVAILABLE

if GUI_MODULE_AVAILABLE:
    import FreeCADGui

TOL = 1e-3


class TestGroupConstraintGui(SketcherGuiTestCase):
    """Tests for the Group constraint command, in particular that the generated
    frame line allows centering the group horizontally, vertically or both."""

    def setUp(self):
        super().setUp()
        self.doc = FreeCAD.newDocument("TestGroupConstraint")

    def make_sketch(self, name):
        sketch = self.doc.addObject("Sketcher::SketchObject", name)
        # bounding box x in [8, 12], y in [3, 7]
        sketch.addGeometry(Part.Circle(FreeCAD.Vector(10, 5, 0), FreeCAD.Vector(0, 0, 1), 2), False)
        # bounding box x in [17, 23], y in [12, 18]
        sketch.addGeometry(
            Part.Circle(FreeCAD.Vector(20, 15, 0), FreeCAD.Vector(0, 0, 1), 3), False
        )
        # total bounding box x in [8, 23], y in [3, 18]
        self.doc.recompute()
        return sketch

    def group_circles(self, sketch):
        FreeCADGui.getDocument(self.doc.Name).setEdit(sketch.Name)
        self.flush_gui(80)
        FreeCADGui.Selection.clearSelection()
        FreeCADGui.Selection.addSelection(self.doc.Name, sketch.Name, "Edge1")
        FreeCADGui.Selection.addSelection(self.doc.Name, sketch.Name, "Edge2")
        FreeCADGui.runCommand("Sketcher_ConstrainGroup")
        self.flush_gui(80)
        self.doc.recompute()

    def bbox_center(self, sketch):
        c1, c2 = sketch.Geometry[0], sketch.Geometry[1]
        xs = [
            c1.Center.x - c1.Radius,
            c1.Center.x + c1.Radius,
            c2.Center.x - c2.Radius,
            c2.Center.x + c2.Radius,
        ]
        ys = [
            c1.Center.y - c1.Radius,
            c1.Center.y + c1.Radius,
            c2.Center.y - c2.Radius,
            c2.Center.y + c2.Radius,
        ]
        return (min(xs) + max(xs)) / 2.0, (min(ys) + max(ys)) / 2.0

    def assert_group_rigid(self, sketch):
        """The group must keep its size and orientation: centering is a pure
        translation, never a scaling or rotation of the grouped geometries."""
        c1, c2 = sketch.Geometry[0], sketch.Geometry[1]
        self.assertAlmostEqual(c1.Radius, 2, delta=TOL)
        self.assertAlmostEqual(c2.Radius, 3, delta=TOL)
        self.assertAlmostEqual(c2.Center.x - c1.Center.x, 10, delta=TOL)
        self.assertAlmostEqual(c2.Center.y - c1.Center.y, 10, delta=TOL)

    def test_frame_is_vertical_line_through_group_center(self):
        sketch = self.make_sketch("SketchFrame")
        self.group_circles(sketch)

        self.assertEqual([c.Type for c in sketch.Constraints], ["Group"])
        self.assertEqual(len(sketch.Geometry), 3)
        frame = sketch.Geometry[2]
        self.assertEqual(frame.TypeId, "Part::GeomLineSegment")
        self.assertTrue(sketch.GeometryFacadeList[2].Construction)

        # vertical line at the horizontal center of the bounding box,
        # spanning its full height
        self.assertAlmostEqual(frame.StartPoint.x, 15.5, delta=TOL)
        self.assertAlmostEqual(frame.EndPoint.x, 15.5, delta=TOL)
        self.assertAlmostEqual(min(frame.StartPoint.y, frame.EndPoint.y), 3.0, delta=TOL)
        self.assertAlmostEqual(max(frame.StartPoint.y, frame.EndPoint.y), 18.0, delta=TOL)

    def fix_group_size(self, sketch):
        """Dimension the frame length: the frame is the group's scale handle, so
        pinning its length keeps the group size fixed during further solves."""
        sketch.addConstraint(Sketcher.Constraint("Distance", 2, 15.0))
        self.doc.recompute()

    def test_center_group_horizontally(self):
        sketch = self.make_sketch("SketchHorizontal")
        self.group_circles(sketch)
        self.fix_group_size(sketch)

        # frame collinear with the vertical axis -> group centered horizontally
        # (collinearity already implies the frame stays vertical)
        sketch.addConstraint(Sketcher.Constraint("Tangent", 2, -2))
        self.doc.recompute()

        # the vertical position remains a free degree of freedom
        cx, _ = self.bbox_center(sketch)
        self.assertAlmostEqual(cx, 0, delta=TOL)
        self.assert_group_rigid(sketch)

    def test_center_group_vertically(self):
        sketch = self.make_sketch("SketchVertical")
        self.group_circles(sketch)
        self.fix_group_size(sketch)

        # frame endpoints symmetric about the horizontal axis -> group centered
        # vertically
        sketch.addConstraint(Sketcher.Constraint("Symmetric", 2, 1, 2, 2, -1))
        self.doc.recompute()

        # the horizontal position remains a free degree of freedom
        _, cy = self.bbox_center(sketch)
        self.assertAlmostEqual(cy, 0, delta=TOL)
        self.assert_group_rigid(sketch)

    def test_center_group_both_directions(self):
        sketch = self.make_sketch("SketchBoth")
        self.group_circles(sketch)
        self.fix_group_size(sketch)

        # vertical frame with endpoints symmetric about the origin -> group
        # centered both ways, since the line's midpoint is the center of the group
        sketch.addConstraint(Sketcher.Constraint("Vertical", 2))
        sketch.addConstraint(Sketcher.Constraint("Symmetric", 2, 1, 2, 2, -1, 1))
        self.doc.recompute()

        cx, cy = self.bbox_center(sketch)
        self.assertAlmostEqual(cx, 0, delta=TOL)
        self.assertAlmostEqual(cy, 0, delta=TOL)
        self.assert_group_rigid(sketch)
