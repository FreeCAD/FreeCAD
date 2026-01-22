# SPDX-License-Identifier: LGPL-2.1-or-later
#
# Copyright (c) 2026 Furgo
#
# This file is part of the FreeCAD Arch workbench.
# You can find the full license text in the LICENSE file in the root directory.

import Draft
import ArchCovering
from bimtests import TestArchBaseGui


class TestArchCoveringGui(TestArchBaseGui.TestArchBaseGui):
    """GUI-side tests for Arch Covering Task Panel logic and workflows."""

    def setUp(self):
        super().setUp()
        self.box = self.document.addObject("Part::Box", "BaseBox")
        self.document.recompute()
        self.panel = None

    def tearDown(self):
        # Ensure any open task panel is closed and the phantom is removed
        if self.panel:
            self.panel.reject()
        self.pump_gui_events()
        super().tearDown()

    def test_phantom_expression_transfer(self):
        """Verify that expressions on the phantom object are transferred to real objects."""
        self.printTestMessage("expression transfer from phantom...")
        # Open panel in creation mode (no obj passed)
        self.panel = ArchCovering.ArchCoveringTaskPanel()

        # Set an expression on the phantom property (simulating user entering f(x) in UI)
        expression = "100mm + 200mm"
        self.panel.phantom.setExpression("TileLength", expression)

        # Assign a target face
        self.panel.selection_list = [(self.box, ["Face6"])]

        # Accept (create the real object)
        self.panel.accept()
        self.pump_gui_events()

        # Assert
        new_obj = self.document.getObject("Covering")
        self.assertIsNotNone(new_obj)

        # Verify the expression by inspecting the ExpressionEngine
        found_expr = False
        if hasattr(new_obj, "ExpressionEngine"):
            for path, expr_val in new_obj.ExpressionEngine:
                if path == "TileLength":
                    # FreeCAD normalizes expressions (e.g. adding spaces).
                    # We compare stripped strings to avoid false negatives.
                    self.assertEqual(expr_val.replace(" ", ""), expression.replace(" ", ""))
                    found_expr = True
                    break

        self.assertTrue(found_expr, "Expression was not found on the generated object.")

    def test_selection_ui_labels(self):
        """Test the smart labeling logic for the 'Base' field."""
        self.printTestMessage("selection UI labels...")
        self.panel = ArchCovering.ArchCoveringTaskPanel()
        # TODO: this test needs to be reworked to be locale-independent

        # Scenario A: single face
        self.panel.selection_list = [(self.box, ["Face1"])]
        self.panel._updateSelectionUI()
        self.assertEqual(self.panel.le_selection.text(), "BaseBox.Face1")

        # Scenario B: multiple faces of same object
        self.panel.selection_list = [(self.box, ["Face1"]), (self.box, ["Face2"])]
        self.panel._updateSelectionUI()
        self.assertIn("(2 faces)", self.panel.le_selection.text())

        # Scenario C: multiple distinct objects
        box2 = self.document.addObject("Part::Box", "SecondBox")
        self.panel.selection_list = [self.box, box2]
        self.panel._updateSelectionUI()
        self.assertEqual(self.panel.le_selection.text(), "2 objects selected")

    def test_batch_creation_logic(self):
        """Verify that selecting multiple faces creates multiple covering objects."""
        self.printTestMessage("batch creation logic...")
        initial_count = len(self.document.Objects)

        # Provide a selection list with 3 targets
        selection = [(self.box, ["Face1"]), (self.box, ["Face2"]), (self.box, ["Face3"])]
        self.panel = ArchCovering.ArchCoveringTaskPanel(selection=selection)

        # Manipulate the widget to ensure the binding and the underlying phantom object are updated
        # correctly.
        target_width = 450.0
        self.panel.sb_width.setProperty("rawValue", target_width)
        # Let the Qt event loop process the change and update the phantom via binding
        self.pump_gui_events()

        self.panel.accept()
        self.pump_gui_events()

        # Verify 3 new objects created + 1 (the initial box) - 1 (the phantom was deleted)
        self.assertEqual(len(self.document.Objects), initial_count + 3)

        # Safe filtering using Draft utility to avoid 'PrimitivePy' attribute errors
        coverings = [o for o in self.document.Objects if Draft.get_type(o) == "Covering"]
        self.assertEqual(len(coverings), 3)
        for c in coverings:
            self.assertAlmostEqual(c.TileWidth.Value, target_width, places=5)

    def test_continue_mode_workflow(self):
        """Test the soft-reset behavior of Continue mode."""
        self.printTestMessage("continue mode workflow...")
        self.panel = ArchCovering.ArchCoveringTaskPanel(selection=[(self.box, ["Face6"])])
        self.panel.chk_continue.setChecked(True)

        # Accept first operation (should return False to keep dialog open)
        result = self.panel.accept()
        self.assertFalse(result, "Panel should not close when Continue is checked.")

        # Verify state reset
        self.assertEqual(len(self.panel.selection_list), 0)
        self.assertEqual(self.panel.le_selection.text(), "No selection")
        self.assertTrue(self.panel.isPicking(), "Picking should be re-armed automatically.")

        # Ensure phantom still exists for the next round
        self.assertIsNotNone(self.document.getObject(self.panel.phantom.Name))

    def test_mode_switching_ux(self):
        """Verify that thickness is disabled when entering pattern modes."""
        self.printTestMessage("mode switching UX...")
        self.panel = ArchCovering.ArchCoveringTaskPanel()

        # Initial state: Solid Tiles (index 0)
        self.panel.combo_mode.setCurrentIndex(0)
        self.assertTrue(self.panel.sb_thick.isEnabled())

        # Switch to Parametric Pattern (index 1)
        self.panel.combo_mode.setCurrentIndex(1)
        self.assertFalse(self.panel.sb_thick.isEnabled())
        self.assertEqual(self.panel.sb_thick.property("rawValue"), 0.0)

        # Switch back to Solid Tiles
        self.panel.combo_mode.setCurrentIndex(0)
        self.assertTrue(self.panel.sb_thick.isEnabled())

        # Should restore the previous default thickness
        self.assertGreater(self.panel.sb_thick.property("rawValue"), 0.0)

    def test_cleanup_removes_phantom(self):
        """Ensure the phantom object is deleted on close/reject."""
        self.printTestMessage("phantom cleanup on reject...")
        self.panel = ArchCovering.ArchCoveringTaskPanel()
        phantom_name = self.panel.phantom.Name
        self.assertIsNotNone(self.document.getObject(phantom_name))

        # Close the panel
        self.panel.reject()
        self.pump_gui_events()

        # Verify the object is gone from the document
        self.assertIsNone(
            self.document.getObject(phantom_name), "Phantom object was not cleaned up."
        )
