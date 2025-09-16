# SPDX-License-Identifier: LGPL-2.1-or-later
#
# Copyright (c) 2025 The FreeCAD Project

import FreeCAD

from draftutils import params

if FreeCAD.GuiUp:
    from PySide import QtCore, QtWidgets
    from PySide.QtCore import QT_TRANSLATE_NOOP
    import FreeCADGui
    from draftutils.translate import translate
else:
    def translate(ctxt, txt):
        return txt
    def QT_TRANSLATE_NOOP(ctxt, txt):
        return txt

import ArchSql


class _ArchReportDocObserver:
    """Document observer that triggers report execution on recompute."""

    def __init__(self, doc, report):
        self.doc = doc
        self.report = report

    def slotRecomputedDocument(self, doc):
        if doc != self.doc:
            return
        # Execute the report when the document is recomputed. Let exceptions
        # propagate so test failures and real errors are visible to the caller.
        self.report.Proxy.execute(self.report)


class _ArchReport:

    def __init__(self, obj):
        self.setProperties(obj)
        obj.Proxy = self
        self.Type = 'ArchReport'
        # --- FIX: Initialize self.spreadsheet ---
        self.spreadsheet = None
        self.docObserver = None # It will be re-attached by onChanged

    def onDocumentRestored(self, obj):
        self.setProperties(obj)

    def setProperties(self, obj):
        if not 'Query' in obj.PropertiesList:
            obj.addProperty('App::PropertyString', 'Query', 'Report', QT_TRANSLATE_NOOP('App::Property', 'The SQL query to execute'))
        if not 'Target' in obj.PropertiesList:
            obj.addProperty('App::PropertyLink', 'Target', 'Report', QT_TRANSLATE_NOOP('App::Property', 'The spreadsheet for the results'))
        if not 'AutoUpdate' in obj.PropertiesList:
            obj.addProperty('App::PropertyBool', 'AutoUpdate', 'Report', QT_TRANSLATE_NOOP('App::Property', 'If True, update report when document recomputes'))
            obj.AutoUpdate = True

        # Attach observer state
        self.onChanged(obj, 'AutoUpdate')

    def setReportPropertySpreadsheet(self, sp, obj):
        """Associate a spreadsheet with a report.

        Ensures the spreadsheet has a non-dependent string property
        ``ReportName`` with the report's object name, and sets the
        report's ``Target`` link to the spreadsheet for future writes.

        Parameters
        - sp: the Spreadsheet::Sheet object to associate
        - obj: the report object (proxy owner)
        """
        if not hasattr(sp, 'ReportName'):
            sp.addProperty('App::PropertyString', 'ReportName', 'Arch', QT_TRANSLATE_NOOP('App::Property', 'The name of the BIM Report that uses this spreadsheet'))
        sp.ReportName = obj.Name
        obj.Target = sp

    def getSpreadSheet(self, obj, force=False):
        """Find or (optionally) create the spreadsheet associated with a report.

        The association is persisted via the sheet's ``ReportName`` string.

        Parameters
        - obj: the report object
        - force: if True, create a new spreadsheet when none is found
        """
        # Return cached sheet when it matches the report name
        sp = getattr(self, 'spreadsheet', None)
        if sp and getattr(sp, 'ReportName', None) == obj.Name:
            return sp

        # Search document for a sheet whose ReportName matches the report
        for o in FreeCAD.ActiveDocument.Objects:
            if o.TypeId == 'Spreadsheet::Sheet' and getattr(o, 'ReportName', None) == obj.Name:
                self.spreadsheet = o
                return self.spreadsheet

        if force:
            sheet = FreeCAD.ActiveDocument.addObject('Spreadsheet::Sheet', 'ReportResult')
            self.setReportPropertySpreadsheet(sheet, obj)
            self.spreadsheet = sheet
            return self.spreadsheet
        else:
            return None

    def onChanged(self, obj, prop):
        if prop == 'AutoUpdate':
            if obj.AutoUpdate:
                if getattr(self, 'docObserver', None) is None:
                    self.docObserver = _ArchReportDocObserver(FreeCAD.ActiveDocument, obj)
                    FreeCAD.addDocumentObserver(self.docObserver)
            else:
                if getattr(self, 'docObserver', None) is not None:
                    FreeCAD.removeDocumentObserver(self.docObserver)
                    self.docObserver = None

    def setSpreadsheetData(self, obj, headers, data_rows, force=False):
        """Write headers and rows into the report's spreadsheet.

        The method prefers an explicit ``obj.Target`` if present. If no valid
        target exists, it will look up a spreadsheet by the sheet's
        ``ReportName`` (matching ``obj.Name``) and, when ``force=True``, create
        a new sheet and associate it.

        Parameters
        - obj: report object
        - headers: list of column header strings
        - data_rows: list of row lists (each row is a list of cell values)
        - force: if True, create the spreadsheet when none is found
        """
        # Prefer the explicit Target spreadsheet selected by the user.
        # If no valid Target is provided, fall back to the named spreadsheet
        # (ReportName) or create one when forced.
        target = getattr(obj, 'Target', None)
        sp = None
        if target and getattr(target, 'TypeId', '') == 'Spreadsheet::Sheet':
            sp = target
        else:
            if not (getattr(obj, 'Target', None) or force):
                return
            sp = self.getSpreadSheet(obj, force=force)
        if not sp:
            return

        widths = [sp.getColumnWidth(col) for col in ('A', 'B', 'C', 'D', 'E', 'F', 'G', 'H')]
        sp.clearAll()
        for i, width in enumerate(widths):
            sp.setColumnWidth(chr(ord('A') + i), width)

        for col_idx, header_text in enumerate(headers):
            sp.set(f"{chr(ord('A') + col_idx)}1", f"'{header_text}'")
        sp.setStyle('A1:%s1' % chr(ord('A') + len(headers) - 1), 'bold', 'add')

        for row_idx, row_data in enumerate(data_rows, start=2):
            for col_idx, cell_value in enumerate(row_data):
                sp.set(f"{chr(ord('A') + col_idx)}{row_idx}", f"'{cell_value}'")

        sp.recompute()
        sp.purgeTouched()

    def execute(self, obj):
        if not getattr(obj, 'Query', None):
            return

        # --- FIX: run_query_for_objects now consistently returns (headers, data_rows) ---
        headers, results_data = ArchSql.run_query_for_objects(obj.Query)

        if results_data is None: # This should now be an empty list, not None, if no results
            FreeCAD.Console.PrintError("Report '%s': Error executing query.\n" % getattr(obj, 'Label', ''))
            return

        # setSpreadsheetData expects (headers, list_of_lists_of_values)
        self.setSpreadsheetData(obj, headers, results_data, force=True)

    def onDelete(self, obj, subelements):
        """
        Clean up the associated spreadsheet on deletion
        This method is called by FreeCAD when the Report object is about to be deleted.
        It ensures the linked spreadsheet is also removed, preventing orphaned objects.
        """
        if obj.Target:
            try:
                # Remove the linked spreadsheet from the document.
                FreeCAD.ActiveDocument.removeObject(obj.Target.Name)
            except Exception as e:
                FreeCAD.Console.PrintWarning(f"Failed to delete spreadsheet '{obj.Target.Label}' linked to Report '{obj.Label}': {e}\n")
        return True # Return True to allow the Report object itself to be deleted.

    def dumps(self):
        # Explicitly *omits self.docObserver
        # Only return 'self.Type', which is a simple string and serializable.
        if hasattr(self,"Type"):
            return self.Type

    def loads(self,state):
        if state:
            self.Type = state
        # The docObserver is NOT restored here; it's handled by onDocumentRestored -> setProperties -> onChanged.

class Report(_ArchReport):
    """Public alias class so Arch._initializeArchObject can find 'Report'.

    This mirrors the naming pattern used by ArchSchedule where the public
    base class is available at module level. It simply inherits from
    the implementation class and does not change behaviour.
    """
    def __init__(self, obj):
        super().__init__(obj)


class ViewProviderReport:
    """The ViewProvider for the ArchReport object."""

    def __init__(self, vobj):
        vobj.Proxy = self
        self.vobj = vobj

    def getIcon(self):
        return ":/icons/Arch_Schedule.svg"

    def doubleClicked(self, vobj):
        return self.setEdit(vobj, 0)

    def setEdit(self, vobj, mode):
        if mode == 0:
            if not hasattr(vobj.Object, "Query"):
                return False
            if FreeCAD.GuiUp:
                panel = ReportTaskPanel(vobj.Object)
                try:
                    FreeCADGui.Control.showDialog(panel)
                except RuntimeError as e:
                    # Avoid raising into the caller (e.g., double click handler)
                    FreeCAD.Console.PrintError(f"Could not open Report editor: {e}\n")
                    return False
            return True
        return False

    def attach(self, vobj):
        # Called by the C++ loader when the view provider is rehydrated.
        # Ensure we keep a reference to the view object for later callbacks.
        self.vobj = vobj

    def claimChildren(self):
        """
        Makes the Target spreadsheet appear as a child in the Tree view,
        by relying on the proxy's getSpreadSheet method for robust lookup.
        """
        obj = self.vobj.Object
        spreadsheet = obj.Proxy.getSpreadSheet(obj)
        return [spreadsheet] if spreadsheet else []

    def dumps(self):
        return None

    def loads(self, state):
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
            if error == "INCOMPLETE":
                return

            if error:
                self.status_label.setText(f"❌ {error}")
                self.status_label.setStyleSheet("color: red;")
                return

            if count == 0:
                message = "⚠️ Query is valid, but found 0 objects."
                color = "orange"
            else:
                message = f"✅ Found {count} objects."
                color = "green"

            self.status_label.setText(message)
            self.status_label.setStyleSheet(f"color: {color};")

        except Exception:
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
