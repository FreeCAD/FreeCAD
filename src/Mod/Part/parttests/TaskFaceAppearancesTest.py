# SPDX-License-Identifier: LGPL-2.1-or-later

import unittest
import FreeCAD
import FreeCADGui
from PySide import QtWidgets


class TaskFaceAppearancesGuiTest(unittest.TestCase):
    def setUp(self):
        """Set up the test document and a basic shape."""
        self.doc = FreeCAD.newDocument("FaceAppTest")
        self.box = self.doc.addObject("Part::Box", "Box")
        self.doc.recompute()

    def tearDown(self):
        """Clean up the selection and close the document."""
        FreeCADGui.Selection.clearSelection()
        FreeCAD.closeDocument(self.doc.Name)

    def test_face_selection_updates_widget_state(self):
        """Test if the material widget state synchronizes with face selection (#27716)."""
        # This test requires the GUI module to be loaded and active
        if not FreeCAD.GuiUp:
            self.skipTest("This test requires a graphical user interface (GUI).")

        # 1. Open the TaskFaceAppearances panel
        FreeCADGui.Selection.addSelection(self.doc.Name, "Box")
        FreeCADGui.runCommand("Part_ColorPerFace")

        # Get the FreeCAD Main Window to search for the UI elements
        main_window = FreeCADGui.getMainWindow()
        self.assertIsNotNone(main_window, "Could not find the FreeCAD main window.")

        # 2. Find the relevant UI widgets by their object names directly in the Main Window
        btn_custom = main_window.findChild(QtWidgets.QPushButton, "buttonCustomAppearance")
        widget_material = main_window.findChild(QtWidgets.QWidget, "widgetMaterial")
        label_element = main_window.findChild(QtWidgets.QLabel, "labelElement")

        self.assertIsNotNone(btn_custom, "Custom Appearance button not found in the UI.")
        self.assertIsNotNone(widget_material, "Material widget not found in the UI.")
        self.assertIsNotNone(label_element, "Element label not found in the UI.")

        # Scenario A: No face selected -> Widgets should be disabled
        FreeCADGui.Selection.clearSelection()
        self.assertFalse(
            widget_material.isEnabled(), "Widget should be disabled when no face is selected."
        )
        self.assertFalse(
            btn_custom.isEnabled(),
            "Custom Appearance button should be disabled when no face is selected.",
        )

        # Scenario B: Single face selected -> Widgets should be enabled
        FreeCADGui.Selection.addSelection(self.doc.Name, "Box", "Face1")
        self.assertTrue(
            widget_material.isEnabled(), "Widget should be enabled after selecting Face1."
        )
        self.assertEqual(label_element.text(), "[1]", "Label did not update to reflect Face 1.")

        # Scenario C: Multiple faces selected -> Widgets should remain enabled
        FreeCADGui.Selection.addSelection(self.doc.Name, "Box", "Face2")
        self.assertTrue(
            widget_material.isEnabled(), "Widget should be enabled with multiple faces selected."
        )

        label_text = label_element.text().replace(" ", "")
        self.assertIn(
            label_text,
            ["[1,2]", "[2,1]"],
            f"Label did not update correctly for multiple faces. Got: {label_text}",
        )

        # Scenario D: Clear selection -> Widgets should be disabled again
        FreeCADGui.Selection.clearSelection()
        self.assertFalse(
            widget_material.isEnabled(), "Widget should be disabled after clearing selection."
        )

        # 3. Close the Task Panel gracefully
        FreeCADGui.Control.closeDialog()
