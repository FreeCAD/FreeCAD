# SPDX-License-Identifier: LGPL-2.1-or-later

import FreeCAD
import Part
import Sketcher

from SketcherTests.GuiTestCase import FreeCADGui, SketcherGuiTestCase


class TestCoincidentCommandGui(SketcherGuiTestCase):
    def setUp(self):
        super().setUp()

        self.doc = FreeCAD.newDocument("CoincidentCommandGuiTest")
        self.sketch = self.doc.addObject("Sketcher::SketchObject", "Sketch")
        self.sketch.addGeometry(
            Part.LineSegment(FreeCAD.Vector(0, 0, 0), FreeCAD.Vector(10, 0, 0)),
            True,
        )
        self.sketch.addGeometry(
            Part.LineSegment(FreeCAD.Vector(20, 0, 0), FreeCAD.Vector(30, 0, 0)),
            True,
        )
        self.sketch.addConstraint(Sketcher.Constraint("DistanceX", 0, 2, 10.0))
        self.sketch.addConstraint(Sketcher.Constraint("DistanceX", 1, 1, 20.0))

    def assert_direct_coincident_conflicts(self):
        constraint_index = self.sketch.addConstraint(Sketcher.Constraint("Coincident", 0, 2, 1, 1))

        self.assertNotEqual(
            self.sketch.solve(),
            0,
            "Test sketch must repro the solver conflict before testing command rollback.",
        )

        self.sketch.delConstraint(constraint_index)
        self.assertEqual(self.sketch.solve(), 0)

    def test_conflicting_coincident_selection_is_rejected(self):
        self.assertEqual(self.sketch.solve(), 0)
        self.assert_direct_coincident_conflicts()

        constraint_count = len(self.sketch.Constraints)

        FreeCADGui.ActiveDocument.setEdit(self.sketch.Name)
        self.flush_gui()
        FreeCADGui.Selection.clearSelection()
        FreeCADGui.Selection.addSelection(self.doc.Name, self.sketch.Name, "Vertex2")
        FreeCADGui.Selection.addSelection(self.doc.Name, self.sketch.Name, "Vertex3")

        FreeCADGui.runCommand("Sketcher_ConstrainCoincidentUnified", 0)
        self.flush_gui()

        self.assertEqual(len(self.sketch.Constraints), constraint_count)
        self.assertEqual(self.sketch.solve(), 0)
