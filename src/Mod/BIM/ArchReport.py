# SPDX-License-Identifier: LGPL-2.1-or-later
#
# Copyright (c) 2025 The FreeCAD Project

import FreeCAD
import FreeCADGui
from PySide import QtCore, QtWidgets
import ArchSql

class Report:
    """The proxy class for the ArchReport FeaturePython object."""

    def __init__(self, obj):
        obj.Proxy = self
        self.Type = "ArchReport"
        self.set_properties(obj)

    def set_properties(self, obj):
        if not hasattr(obj, "Query"):
            obj.addProperty("App::PropertyString", "Query", "Report", "The SQL query to execute")
        if not hasattr(obj, "Target"):
            obj.addProperty("App::PropertyLink", "Target", "Report", "The spreadsheet for the results")

    def execute(self, fp):
        """Called on document recompute."""
        if not fp.Target or not fp.Query:
            return

        results = ArchSql.run_query_for_objects(fp.Query)

        if results is None:
            FreeCAD.Console.PrintError(f"Report '{fp.Label}': Error executing query.\n")
            return

        sheet = fp.Target
        sheet.clearAll()
        if results:
            sheet.set("A1", "'Object Label'")
            for i, res_obj in enumerate(results, 2):
                sheet.set(f"A{i}", f"'{res_obj.Label}'")


class ViewProviderReport:
    """The ViewProvider for the ArchReport object."""

    def __init__(self, vobj):
        vobj.Proxy = self
        self.vobj = vobj

    def getIcon(self):
        return ":/icons/Arch_Schedule.svg"

    def doubleClicked(self, vobj):
        # Delegate to setEdit, which is the primary and standard entry point.
        return self.setEdit(vobj, 0)

    def setEdit(self, vobj, mode):
        # This is the primary entry point for all edit operations.
        # Mode 0 is the default edit mode (double-click, F2, context menu).
        if mode == 0:
            if not hasattr(vobj.Object, "Query"):
                return False
            if FreeCAD.GuiUp:
                panel = ReportTaskPanel(vobj.Object)
                FreeCADGui.Control.showDialog(panel)
            return True
        return False

    def dumps(self):
        """Tells FreeCAD not to save any state for this view provider."""
        return None

    def loads(self, state):
        """Tells FreeCAD not to restore any state for this view provider."""
        return None

class ReportTaskPanel:
    """The Task Panel UI for editing an ArchReport object."""

    def __init__(self, report_obj):
        self.obj = report_obj
        self.form = QtWidgets.QDialog()
        self.form.setWindowTitle("BIM Report Editor")

        self.query_edit = QtWidgets.QPlainTextEdit()
        self.status_label = QtWidgets.QLabel("Ready")

        self.status_label.setTextInteractionFlags(QtCore.Qt.TextSelectableByMouse)

        layout = QtWidgets.QVBoxLayout()
        layout.addWidget(QtWidgets.QLabel("Enter SQL Query:"))
        layout.addWidget(self.query_edit)
        layout.addWidget(self.status_label)

        button_box = QtWidgets.QDialogButtonBox(QtWidgets.QDialogButtonBox.Ok | QtWidgets.QDialogButtonBox.Cancel)
        button_box.accepted.connect(self.accept)
        button_box.rejected.connect(self.form.reject)
        layout.addWidget(button_box)

        self.form.setLayout(layout)

        self.validation_timer = QtCore.QTimer()
        self.validation_timer.setSingleShot(True)
        self.validation_timer.timeout.connect(self.run_validation)

        self.query_edit.textChanged.connect(self.on_query_changed)

        self.query_edit.setPlainText(self.obj.Query)
        self.run_validation()

    def on_query_changed(self):
        self.status_label.setText("<i>Typing...</i>")
        self.status_label.setStyleSheet("color: gray;")
        self.validation_timer.start(500)

    def run_validation(self):
        query = self.query_edit.toPlainText()
        if not query.strip():
            self.status_label.setText("Ready")
            self.status_label.setStyleSheet("color: black;")
            return

        count, error = ArchSql.run_query_for_count(query)

        try:
            # Handle the "incomplete" case first and exit.
            if error == "INCOMPLETE":
                return  # Do nothing and wait for the user to finish typing.

            # Handle any actual syntax errors and exit.
            if error:
                self.status_label.setText(f"❌ {error}")
                self.status_label.setStyleSheet("color: red;")
                return

            # If we reach this point, the query is valid (error is None).
            # We now only need to distinguish between zero and non-zero results.
            if count == 0:
                message = "⚠️ Query is valid, but found 0 objects."
                color = "orange"
            else:
                message = f"✅ Found {count} objects."
                color = "green"

            self.status_label.setText(message)
            self.status_label.setStyleSheet(f"color: {color};")

        except Exception:
            # If any other unexpected error happens, catch it and print the full traceback
            import traceback
            self.status_label.setText("❌ An unexpected error occurred. See Report View.")
            self.status_label.setStyleSheet("color: red;")
            FreeCAD.Console.PrintError("--- BIM Report Unexpected Error ---\n")
            FreeCAD.Console.PrintError(traceback.format_exc())
            FreeCAD.Console.PrintError("----------------------------------\n")

    def accept(self):
        self.obj.Query = self.query_edit.toPlainText()
        FreeCAD.ActiveDocument.recompute()
        self.form.accept()