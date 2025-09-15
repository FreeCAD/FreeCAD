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
        if not hasattr(sp, 'Report'):
            sp.addProperty('App::PropertyLink', 'Report', 'Arch', QT_TRANSLATE_NOOP('App::Property', 'The BIM Report that uses this spreadsheet'))
        sp.Report = obj

    def getSpreadSheet(self, obj, force=False):
        """Return or create the spreadsheet used by this report.

        Behaviour intentionally mirrors ArchSchedule.getSpreadSheet.
        """
        # If we cached a spreadsheet, check it still belongs to this report.
        # If the attribute access raises, let the exception propagate so the
        # caller can handle it (don't silently swallow deletions).
        if getattr(self, 'spreadsheet', None) is not None and getattr(self.spreadsheet, 'Report', None) == obj:
            return self.spreadsheet

        for o in FreeCAD.ActiveDocument.Objects:
            if o.TypeId == 'Spreadsheet::Sheet' and getattr(o, 'Report', None) == obj:
                self.spreadsheet = o
                return self.spreadsheet

        if force:
            self.spreadsheet = FreeCAD.ActiveDocument.addObject('Spreadsheet::Sheet', 'ReportResult')
            self.setReportPropertySpreadsheet(self.spreadsheet, obj)
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

    def setSpreadsheetData(self, obj, labels, force=False):
        """Write header and rows into the spreadsheet, then recompute and purgeTouched."""
        if not hasattr(self, 'data'):
            self.data = labels
        if not self.data:
            return
        if not (getattr(obj, 'Target', None) or force):
            return
        sp = self.getSpreadSheet(obj, force=True)
        # preserve widths
        widths = [sp.getColumnWidth(col) for col in ('A', 'B', 'C')]
        sp.clearAll()
        for col, width in zip(('A', 'B', 'C'), widths):
            sp.setColumnWidth(col, width)
        sp.set('A1', 'Object Label')
        sp.setStyle('A1', 'bold', 'add')
        for k, v in enumerate(self.data, start=2):
            sp.set('A%d' % k, v)
        sp.recompute()
        sp.purgeTouched()

    def execute(self, obj):
        if not getattr(obj, 'Query', None):
            return
        results = ArchSql.run_query_for_objects(obj.Query)
        if results is None:
            FreeCAD.Console.PrintError("Report '%s': Error executing query.\n" % getattr(obj, 'Label', ''))
            return
        labels = []
        for o in results:
            if o is None:
                continue
            if getattr(o, 'TypeId', '') == 'Spreadsheet::Sheet':
                continue
            if hasattr(o, 'Proxy') and getattr(o.Proxy, 'Type', None) in ('Schedule', 'ArchReport'):
                continue
            labels.append(getattr(o, 'Label', getattr(o, 'Name', '')))
        self.setSpreadsheetData(obj, labels, force=True)


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

    def claimChildren(self):
        vobj = getattr(self, 'vobj', None)
        if vobj is None:
            return []
        obj = getattr(vobj, 'Object', None)
        if obj is None:
            return []
        target = getattr(obj, 'Target', None)
        if target is not None and getattr(target, 'TypeId', '') == 'Spreadsheet::Sheet':
            return [target]
        doc = getattr(obj, 'Document', FreeCAD.ActiveDocument)
        children = []
        for o in getattr(doc, 'Objects', []):
            if getattr(o, 'TypeId', '') == 'Spreadsheet::Sheet' and getattr(o, 'Report', None) == obj:
                children.append(o)
        return children

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