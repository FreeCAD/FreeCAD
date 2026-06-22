# SPDX-License-Identifier: LGPL-2.1-or-later

"""Regression test for the Sketcher edit-mode axis-cross out-of-bounds crash.

Entering a sketch's edit mode builds the edit-mode scene graph. The axis cross
(``RootCrossLineSet``) declares ``numVertices = {2, 2}`` (i.e. four coordinates
for the two axis lines), but its coordinate node was only populated later by
``updateAxesLength()`` -- which runs from a camera-update handler, not before the
first render. A freshly created ``SoCoordinate3`` holds a single default point, so
the first render read past the coordinate array in ``SoLineSet::GLRender`` (a heap
out-of-bounds read that crashed FreeCAD, often surfacing at an unrelated allocation).

This test enters edit mode and asserts the cross line set never claims more
vertices than its coordinate node provides.
"""

import unittest

import FreeCAD
import FreeCADGui
import Part
import Sketcher

try:
    from pivy import coin

    PIVY_AVAILABLE = True
except ImportError:
    PIVY_AVAILABLE = False


def _find_node_by_name(root, type_id, name):
    sa = coin.SoSearchAction()
    sa.setType(type_id)
    sa.setInterest(coin.SoSearchAction.ALL)
    sa.apply(root)
    paths = sa.getPaths()
    for i in range(paths.getLength()):
        node = paths[i].getTail()
        node_name = node.getName().getString() if node.getName() else ""
        if node_name == name:
            return node
    return None


class SketchEditModeAxisCrossTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if not FreeCAD.GuiUp:
            raise unittest.SkipTest("Cannot run GUI tests in a CLI environment.")
        if not PIVY_AVAILABLE:
            raise unittest.SkipTest("pivy (coin) not available.")

    def setUp(self):
        self.doc = FreeCAD.newDocument("AxisCrossEditModeTest")

    def tearDown(self):
        try:
            FreeCADGui.ActiveDocument.resetEdit()
        except Exception:
            pass
        FreeCAD.closeDocument(self.doc.Name)

    def test_axis_cross_coordinate_buffer_is_sufficient(self):
        """Entering edit mode must not leave the axis cross with fewer
        coordinates than its numVertices claims (would read out of bounds)."""
        sketch = self.doc.addObject("Sketcher::SketchObject", "Sketch")
        line0 = sketch.addGeometry(
            Part.LineSegment(FreeCAD.Vector(0, 0, 0), FreeCAD.Vector(10, 0, 0)), False
        )
        line1 = sketch.addGeometry(
            Part.LineSegment(FreeCAD.Vector(0, 5, 0), FreeCAD.Vector(10, 5, 0)), False
        )
        # Point-to-line distance: the configuration that triggered the crash.
        sketch.addConstraint(Sketcher.Constraint("Distance", line1, 1, line0, 0, 5.0))
        self.doc.recompute()

        FreeCADGui.ActiveDocument.setEdit(sketch.Name)
        FreeCADGui.updateGui()

        view = FreeCADGui.ActiveDocument.ActiveView
        root = view.getViewer().getSceneGraph()

        line_set = _find_node_by_name(root, coin.SoLineSet.getClassTypeId(), "RootCrossLineSet")
        coordinate = _find_node_by_name(
            root, coin.SoCoordinate3.getClassTypeId(), "RootCrossCoordinate"
        )
        self.assertIsNotNone(line_set, "RootCrossLineSet not found in edit scene graph")
        self.assertIsNotNone(coordinate, "RootCrossCoordinate not found in edit scene graph")

        num_vertices = line_set.numVertices
        claimed = sum(num_vertices[i] for i in range(num_vertices.getNum()))
        available = coordinate.point.getNum()

        self.assertLessEqual(
            claimed,
            available,
            f"RootCrossLineSet claims {claimed} vertices but RootCrossCoordinate "
            f"provides only {available} points (out-of-bounds read in GLRender).",
        )


if __name__ == "__main__":
    unittest.main()
