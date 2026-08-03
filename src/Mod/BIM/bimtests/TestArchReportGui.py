"""GUI tests for ArchReport features that require a running FreeCAD GUI.

These tests inherit from the GUI test base `TestArchBaseGui` which skips
the whole class when `FreeCAD.GuiUp` is False.
"""

import FreeCAD
import Arch
import FreeCADGui
import ArchReport

from bimtests.TestArchBaseGui import TestArchBaseGui


class TestArchReportGui(TestArchBaseGui):
    """GUI-enabled tests ported from TestArchReport.

    These tests rely on Qt widgets and the BIM workbench UI panels.
    """

    def setUp(self):
        super().setUp()
        self.doc = self.document
        self.panel = None

        # Recreate the same minimal scene that TestArchReport.setUp creates
        self.wall_ext = Arch.makeWall(length=1000, name="Exterior Wall")
        self.wall_ext.IfcType = "Wall"
        self.wall_ext.Height = FreeCAD.Units.Quantity(3000, "mm")

        self.wall_int = Arch.makeWall(length=500, name="Interior partition wall")
        self.wall_int.IfcType = "Wall"
        self.wall_int.Height = FreeCAD.Units.Quantity(2500, "mm")

        self.column = Arch.makeStructure(length=300, width=330, height=2000, name="Main Column")
        self.column.IfcType = "Column"

        self.beam = Arch.makeStructure(length=2000, width=200, height=400, name="Main Beam")
        self.beam.IfcType = "Beam"

        self.window = Arch.makeWindow(name="Living Room Window")
        self.window.IfcType = "Window"

        self.part_box = self.doc.addObject("Part::Box", "Generic Box")

        # Spreadsheet used by some report features
        self.spreadsheet = self.doc.addObject("Spreadsheet::Sheet", "ReportTarget")
        self.doc.recompute()

    def tearDown(self):
        # This method is automatically called after EACH test function.
        if self.panel:
            # If a panel was created, ensure it is closed.
            FreeCADGui.Control.closeDialog()
            self.panel = None  # Clear the reference
        super().tearDown()

    def test_cheatsheet_dialog_creation(self):
        """Tests that the Cheatsheet dialog can be created without errors."""
        api_data = Arch.getSqlApiDocumentation()
        dialog = ArchReport.CheatsheetDialog(api_data)
        self.assertIsNotNone(dialog)

    def DISABLED_test_preview_pane_toggle_and_refresh(self):
        """
        Tests the user workflow for the preview pane: toggling visibility,
        refreshing with a valid query, and checking the results.
        This replaces the obsolete test_task_panel_on_demand_preview.
        """
        # 1. Arrange: Create a report object and the task panel.
        report_obj = Arch.makeReport(name="PreviewToggleTestReport")
        self.panel = ArchReport.ReportTaskPanel(report_obj)
        # Open the editor for the first (default) statement.
        self.panel._start_edit_session(row_index=0)

        # 2. Assert Initial State: The preview pane should be hidden.
        self.assertFalse(
            self.panel.preview_pane.isVisible(), "Preview pane should be hidden by default."
        )

        # 3. Act: Toggle the preview pane to show it.
        # A user click on a checkable button toggles its checked state.
        self.panel.btn_toggle_preview.setChecked(True)
        self.pump_gui_events()

        # 4. Assert Visibility: The pane and its contents should now be visible.
        self.assertTrue(
            self.panel.preview_pane.isVisible(),
            "Preview pane should be visible after toggling it on.",
        )
        self.assertEqual(
            self.panel.btn_toggle_preview.text(),
            "Hide Preview",
            "Button text should update to 'Hide Preview'.",
        )
        self.assertTrue(
            self.panel.btn_refresh_preview.isVisible(),
            "Refresh button should be visible when pane is open.",
        )

        # 5. Act: Set a valid query and refresh the preview.
        query = "SELECT Label, IfcType FROM document WHERE IfcType = 'Wall' ORDER BY Label"
        self.panel.sql_query_edit.setPlainText(query)
        self.panel.btn_refresh_preview.click()
        self.pump_gui_events()

        # 6. Assert Correctness: The preview table should be populated correctly.
        self.assertEqual(
            self.panel.table_preview_results.columnCount(),
            2,
            "Preview table should have 2 columns for the valid query.",
        )
        self.assertEqual(
            self.panel.table_preview_results.rowCount(),
            2,
            "Preview table should have 2 rows for the two wall objects.",
        )
        # Check cell content to confirm the query ran correctly.
        self.assertEqual(self.panel.table_preview_results.item(0, 0).text(), self.wall_ext.Label)
        self.assertEqual(self.panel.table_preview_results.item(1, 1).text(), self.wall_int.IfcType)

        # 7. Act: Toggle the preview pane to hide it again.
        self.panel.btn_toggle_preview.setChecked(False)
        self.pump_gui_events()

        # 8. Assert Final State: The pane should be hidden.
        self.assertFalse(
            self.panel.preview_pane.isVisible(),
            "Preview pane should be hidden after toggling it off.",
        )
        self.assertEqual(
            self.panel.btn_toggle_preview.text(),
            "Show Preview",
            "Button text should revert to 'Show Preview'.",
        )

    def DISABLED_test_preview_pane_displays_errors_gracefully(self):
        """
        Tests that the preview pane displays a user-friendly error message when
        the query is invalid, instead of raising an exception.
        This replaces the obsolete test_preview_button_handles_errors_gracefully_in_ui.
        """
        # 1. Arrange: Create the report and panel, then open the editor and preview.
        report_obj = Arch.makeReport(name="PreviewErrorTestReport")
        self.panel = ArchReport.ReportTaskPanel(report_obj)
        self.panel._start_edit_session(row_index=0)
        self.panel.btn_toggle_preview.setChecked(True)
        self.pump_gui_events()

        # 2. Act: Set an invalid query and click the refresh button.
        invalid_query = "SELECT Label FRM document"  # Deliberate syntax error
        self.panel.sql_query_edit.setPlainText(invalid_query)
        self.panel.btn_refresh_preview.click()
        self.pump_gui_events()

        # 3. Assert: The preview table should be visible and display the error.
        self.assertTrue(
            self.panel.table_preview_results.isVisible(),
            "Preview table should remain visible to display the error.",
        )
        self.assertEqual(
            self.panel.table_preview_results.rowCount(),
            1,
            "Error display should occupy a single row.",
        )
        self.assertEqual(
            self.panel.table_preview_results.columnCount(),
            1,
            "Error display should occupy a single column.",
        )

        error_item = self.panel.table_preview_results.item(0, 0)
        self.assertIsNotNone(error_item, "An error item should have been placed in the table.")
        # Check for keywords that indicate a graceful error message.
        self.assertIn(
            "Syntax Error", error_item.text(), "The error message should indicate a syntax error."
        )
        self.assertIn(
            "❌", error_item.text(), "The error message should contain a visual error indicator."
        )

    def test_hover_tooltips(self):
        """Tests that the SQL editor can generate tooltips."""
        editor = ArchReport.SqlQueryEditor()
        api_docs = Arch.getSqlApiDocumentation()
        editor.set_api_documentation(api_docs)

        func_tooltip = editor._get_tooltip_for_word("CONVERT")
        self.assertIn("CONVERT(quantity, 'unit')", func_tooltip)
        self.assertIn("Utility", func_tooltip)

        clause_tooltip = editor._get_tooltip_for_word("SELECT")
        self.assertIn("SQL Clause", clause_tooltip)
