# SPDX-License-Identifier: LGPL-2.1-or-later

import FreeCAD as App
import Part
import Sketcher
from SketcherTests.GuiTestCase import FreeCADGui, SketcherGuiTestCase


class TestDeferredConstraints(SketcherGuiTestCase):
    """
    Test that adding a constraint with an expression in a transaction
    correctly binds the expression to the newly added constraint index.
    """

    def test_add_constraint_with_expression_buffered(self):
        if not FreeCADGui:
            self.skipTest("GUI not available")

        self.doc = App.newDocument("TestDeferredConstraintsDoc")
        self.sketch = self.doc.addObject("Sketcher::SketchObject", "Sketch")
        self.sketch.addGeometry(
            Part.LineSegment(App.Vector(0, 0, 0), App.Vector(10, 10, 0)),
            False,
        )
        self.doc.recompute()

        # 1. Add initial constraint so Constraints[0] exists
        self.sketch.addConstraint(Sketcher.Constraint("DistanceX", 0, 1, 0, 2, 10.0))
        self.doc.recompute()
        self.assertEqual(len(self.sketch.Constraints), 1)

        # 2. Simulate transaction buffering
        FreeCADGui.doCommand("App.setActiveDocument('TestDeferredConstraintsDoc')")

        self.doc.openTransaction("Add DistanceY with Expression")
        self.sketch.addConstraint(Sketcher.Constraint("DistanceY", 0, 1, 0, 2, 10.0))
        self.sketch.setExpression("Constraints[1]", "10.0mm * 2")
        self.doc.commitTransaction()
        self.doc.recompute()

        # 3. Assertions
        self.assertEqual(len(self.sketch.Constraints), 2)

        expressions = self.sketch.ExpressionEngine
        self.assertTrue(
            any("Constraints[1]" in expr[0] for expr in expressions),
            "Expression was not bound to Constraints[1]",
        )
