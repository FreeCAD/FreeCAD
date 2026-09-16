# SPDX-License-Identifier: LGPL-2.1-or-later

import FreeCAD as App
import Part
import Sketcher
from pivy import coin

from SketcherTests.GuiTestCase import FreeCADGui as Gui, SketcherGuiTestCase


class TestSolverUpdateGui(SketcherGuiTestCase):
    def setUp(self):
        super().setUp()
        Gui.activateWorkbench("SketcherWorkbench")
        self.doc = App.newDocument("TestSolverUpdateGui")
        self.sketch = self.doc.addObject("Sketcher::SketchObject", "Sketch")
        self.sketch.addGeometry(Part.LineSegment(App.Vector(0, 0, 0), App.Vector(40, 0, 0)), False)
        self.sketch.addConstraint(Sketcher.Constraint("Coincident", 0, 1, -1, 1))
        self.sketch.addConstraint(Sketcher.Constraint("Horizontal", 0))
        self.length = self.sketch.addConstraint(Sketcher.Constraint("Distance", 0, 40.0))
        self.doc.recompute()
        self.view = Gui.activeDocument().activeView()

    def assert_drawn_length(self, length):
        search = coin.SoSearchAction()
        search.setName("CurvesCoordinate0")
        search.setSearchingAll(True)
        search.apply(self.view.getSceneGraph())
        path = search.getPath()
        self.assertIsNotNone(path, "The initial solve must create the edit geometry")
        points = path.getTail().point.getValues()
        self.assertEqual(len(points), 2)
        self.assertAlmostEqual(points[0][0], 0.0, places=5)
        self.assertAlmostEqual(points[1][0], length, places=5)

    def test_open_sketch_in_hidden_body(self):
        # A hidden body keeps the edit root out of scene-graph searches until
        # setEditViewer(), after the initial solver notification (#32720).
        body = self.doc.addObject("PartDesign::Body", "Body")
        body.addObject(self.sketch)
        self.sketch.Placement.Rotation = App.Rotation(App.Vector(1, 0, 0), 90)
        self.doc.recompute()
        self.sketch.Visibility = False
        body.Visibility = False

        self.assertTrue(Gui.activeDocument().setEdit(self.sketch.Name))
        # Check immediately: a later camera change/recompute can mask the bug.
        self.assert_drawn_length(40.0)

    def test_spreadsheet_updates_inactive_sketch_view(self):
        try:
            import Spreadsheet  # noqa: F401
        except ImportError:
            self.skipTest("Spreadsheet module is not available")

        sheet = self.doc.addObject("Spreadsheet::Sheet", "Sheet")
        sheet.set("A1", "40 mm")
        self.sketch.setExpression(f"Constraints[{self.length}]", "Sheet.A1")
        self.doc.recompute()
        self.assertTrue(Gui.activeDocument().setEdit(self.sketch.Name))
        self.assert_drawn_length(40.0)

        sheet.ViewObject.doubleClicked()
        self.flush_gui(50)
        self.assertNotEqual(Gui.activeDocument().activeView(), self.view)
        sheet.set("A1", "60 mm")
        self.doc.recompute()

        self.assertAlmostEqual(self.sketch.Geometry[0].EndPoint.x, 60.0)
        self.assert_drawn_length(60.0)
