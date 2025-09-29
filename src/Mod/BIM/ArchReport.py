# SPDX-License-Identifier: LGPL-2.1-or-later
#
# Copyright (c) 2025 The FreeCAD Project

import FreeCAD
import os
import json

if FreeCAD.GuiUp:
    from PySide import QtCore, QtWidgets, QtGui
    from PySide.QtCore import QT_TRANSLATE_NOOP
    import FreeCADGui
    from draftutils.translate import translate
else:
    def translate(ctxt, txt):
        return txt
    def QT_TRANSLATE_NOOP(ctxt, txt):
        return txt

import Arch

if FreeCAD.GuiUp:
    ICON_STATUS_OK = FreeCADGui.getIcon(":/icons/edit_OK.svg")
    ICON_STATUS_WARN = FreeCADGui.getIcon(":/icons/Warning.svg")
    ICON_STATUS_ERROR = FreeCADGui.getIcon(":/icons/delete.svg")
    ICON_STATUS_INCOMPLETE = FreeCADGui.getIcon(":/icons/button_invalid.svg")
    ICON_EDIT = FreeCADGui.getIcon(":/icons/Draft_Edit.svg")
    ICON_ADD = FreeCADGui.getIcon(":/icons/list-add.svg")
    ICON_REMOVE = FreeCADGui.getIcon(":/icons/list-remove.svg")
    ICON_DUPLICATE = FreeCADGui.getIcon(":/icons/edit-copy.svg")


def _get_preset_paths(preset_type):
    """Gets the file paths for bundled and user presets."""
    if preset_type == 'query':
        filename = "query_presets.json"
    elif preset_type == 'template':
        filename = "report_templates.json"
    else:
        return None, None

    resource_path = os.path.join(FreeCAD.getResourceDir(), "Mod", "BIM", "Presets", "ArchReport", filename)
    user_path = os.path.join(FreeCAD.getUserAppDataDir(), "BIM", "ArchReport", f"user_{filename}")
    return resource_path, user_path

def _get_presets(preset_type):
    """Loads bundled and user presets, with user presets overriding bundled ones."""
    resource_path, user_path = _get_preset_paths(preset_type)
    presets = {}

    # 1. Load bundled presets first
    if resource_path and os.path.exists(resource_path):
        try:
            with open(resource_path, 'r', encoding='utf8') as f:
                presets.update(json.load(f))
        except Exception as e:
            FreeCAD.Console.PrintError(f"BIM Report: Could not load bundled presets from {resource_path}: {e}\n")

    # 2. Load user presets, which will override any bundled presets with the same name
    if user_path and os.path.exists(user_path):
        try:
            with open(user_path, 'r', encoding='utf8') as f:
                presets.update(json.load(f))
        except Exception as e:
            FreeCAD.Console.PrintError(f"BIM Report: Could not load user presets from {user_path}: {e}\n")

    return presets

def _save_preset(preset_type, name, data):
    """Saves a single preset or template to the user's preset file."""
    _, user_path = _get_preset_paths(preset_type)
    if not user_path:
        return

    user_dir = os.path.dirname(user_path)
    os.makedirs(user_dir, exist_ok=True)

    # Load existing user presets to avoid overwriting them
    user_presets = {}
    if os.path.exists(user_path):
        try:
            with open(user_path, 'r', encoding='utf8') as f:
                # Handle case where file might be empty
                content = f.read()
                if content:
                    user_presets = json.loads(content)
        except Exception as e:
            FreeCAD.Console.PrintError(f"BIM Report: Could not read user presets file at {user_path}: {e}\n")

    # Add or update the new preset
    user_presets[name] = data

    # Write the entire collection back to the file
    try:
        with open(user_path, 'w', encoding='utf8') as f:
            json.dump(user_presets, f, indent=2)
        FreeCAD.Console.PrintMessage(f"BIM Report: Preset '{name}' saved successfully.\n")
    except Exception as e:
        FreeCAD.Console.PrintError(f"BIM Report: Could not save preset to {user_path}: {e}\n")


if FreeCAD.GuiUp:

    class SqlQueryEditor(QtWidgets.QPlainTextEdit):
        """
        A custom QPlainTextEdit that provides autocompletion features.

        This class integrates QCompleter and handles key events to provide
        content-based sizing for the popup and a better user experience,
        such as accepting completions with the Tab key.
        """
        def __init__(self, parent=None):
            super().__init__(parent)
            self._completer = None
            self.setMouseTracking(True) # Required to receive mouseMoveEvents
            self.api_docs = {}
            self.clauses = set()
            self.functions = {}

        def set_api_documentation(self, api_docs: dict):
            """Receives the API documentation from the panel and caches it."""
            self.api_docs = api_docs
            self.clauses = set(api_docs.get('clauses', []))
            # Create a flat lookup dictionary for fast access
            for category, func_list in api_docs.get('functions', {}).items():
                for func_data in func_list:
                    self.functions[func_data['name']] = {
                        'category': category,
                        'signature': func_data['signature'],
                        'description': func_data['description']
                    }

        def mouseMoveEvent(self, event: QtGui.QMouseEvent):
            """Overrides the mouse move event to show tooltips."""
            cursor = self.cursorForPosition(event.pos())
            cursor.select(QtGui.QTextCursor.WordUnderCursor)
            word = cursor.selectedText().upper()

            tooltip_text = self._get_tooltip_for_word(word)

            if tooltip_text:
                QtWidgets.QToolTip.showText(event.globalPos(), tooltip_text, self)
            else:
                QtWidgets.QToolTip.hideText()

            super().mouseMoveEvent(event)

        def _get_tooltip_for_word(self, word: str) -> str:
            """Builds the HTML-formatted tooltip string for a given word."""
            if not word:
                return ""

            # Check if the word is a function
            if word in self.functions:
                func_data = self.functions[word]
                # Format a rich HTML tooltip for functions
                return (f"<p style='white-space:nowrap'><code><b>{func_data['signature']}</b></code><br>"
                        f"<i>{func_data['category']}</i><br>"
                        f"{func_data['description']}</p>")

            # Check if the word is a clause
            if word in self.clauses:
                # Format a simple, translatable tooltip for clauses
                # The string itself is marked for translation here.
                return f"<i>{translate('Arch', 'SQL Clause')}</i>"

            return ""

        def setCompleter(self, completer):
            if self._completer:
                self._completer.activated.disconnect(self.insertCompletion)

            self._completer = completer
            if not self._completer:
                return

            self._completer.setWidget(self)
            self._completer.setCompletionMode(QtWidgets.QCompleter.PopupCompletion)
            self._completer.activated.connect(self.insertCompletion)

        def completer(self):
            return self._completer

        def insertCompletion(self, completion):
            if self._completer.widget() is not self:
                return

            tc = self.textCursor()
            tc.select(QtGui.QTextCursor.WordUnderCursor)
            tc.insertText(completion)
            self.setTextCursor(tc)

        def textUnderCursor(self):
            tc = self.textCursor()
            tc.select(QtGui.QTextCursor.WordUnderCursor)
            return tc.selectedText()

        def keyPressEvent(self, event):
            # Pass key events to the completer first if its popup is visible.
            if self._completer and self._completer.popup().isVisible():
                if event.key() in (QtCore.Qt.Key_Enter, QtCore.Qt.Key_Return,
                                   QtCore.Qt.Key_Escape, QtCore.Qt.Key_Tab,
                                   QtCore.Qt.Key_Backtab):
                    event.ignore()
                    return

            # Let the parent handle the key press to ensure normal typing works.
            super().keyPressEvent(event)

            # --- Autocompletion Trigger Logic ---

            # A Ctrl+Space shortcut can also be used to trigger completion.
            is_shortcut = (event.modifiers() & QtCore.Qt.ControlModifier and
                           event.key() == QtCore.Qt.Key_Space)

            completion_prefix = self.textUnderCursor()

            # Don't show completer for very short prefixes unless forced by shortcut.
            if not is_shortcut and len(completion_prefix) < 2:
                self._completer.popup().hide()
                return

            # Show the completer if the prefix has changed.
            if completion_prefix != self._completer.completionPrefix():
                self._completer.setCompletionPrefix(completion_prefix)
                # Select the first item by default for a better UX.
                self._completer.popup().setCurrentIndex(
                    self._completer.completionModel().index(0, 0))

            # --- Sizing and Positioning Logic (The critical fix) ---
            cursor_rect = self.cursorRect()

            # Calculate the required width based on the content of the popup.
            popup_width = (self._completer.popup().sizeHintForColumn(0) +
                           self._completer.popup().verticalScrollBar().sizeHint().width())
            cursor_rect.setWidth(popup_width)

            # Show the completer.
            self._completer.complete(cursor_rect)


class ReportStatement:
    """Encapsulates a single SQL query statement and its display options."""

    def __init__(self, description="", query_string="",
                 use_description_as_header=False, include_column_names=True,
                 add_empty_row_after=False, print_results_in_bold=False,
                 is_pipelined=False):
        self.description = description
        self.query_string = query_string
        self.use_description_as_header = use_description_as_header
        self.include_column_names = include_column_names
        self.add_empty_row_after = add_empty_row_after
        self.print_results_in_bold = print_results_in_bold
        self.is_pipelined = is_pipelined

        # Internal validation state (transient, not serialized)
        self._validation_status = "Ready" # e.g., "OK", "0_RESULTS", "ERROR", "INCOMPLETE"
        self._validation_message = translate("Arch", "Ready") # e.g., "Found 5 objects.", "Syntax Error: ..."
        self._validation_count = 0

    def dumps(self):
        """Returns the internal state for serialization."""
        return {
            'description': self.description,
            'query_string': self.query_string,
            'use_description_as_header': self.use_description_as_header,
            'include_column_names': self.include_column_names,
            'add_empty_row_after': self.add_empty_row_after,
            'print_results_in_bold': self.print_results_in_bold,
            'is_pipelined': self.is_pipelined,
        }

    def loads(self, state):
        """Restores the internal state from serialized data."""
        self.description = state.get('description', '')
        self.query_string = state.get('query_string', '')
        self.use_description_as_header = state.get('use_description_as_header', False)
        self.include_column_names = state.get('include_column_names', True)
        self.add_empty_row_after = state.get('add_empty_row_after', False)
        self.print_results_in_bold = state.get('print_results_in_bold', False)
        self.is_pipelined = state.get('is_pipelined', False)

        # Validation state is transient and re-calculated on UI load/edit
        self._validation_status = "Ready"
        self._validation_message = translate("Arch", "Ready")
        self._validation_count = 0

    def validate_and_update_status(self):
        """Runs validation for this statement's query and updates its internal status."""
        if not self.query_string.strip():
            self._validation_status = "OK" # Empty query is valid, no error
            self._validation_message = translate("Arch", "Ready")
            self._validation_count = 0
            return

        count, error = Arch.count(self.query_string)

        if error == "INCOMPLETE":
            self._validation_status = "INCOMPLETE"
            self._validation_message = translate("Arch", "Typing...")
            self._validation_count = -1
        elif error:
            self._validation_status = "ERROR"
            self._validation_message = error
            self._validation_count = -1
        elif count == 0:
            self._validation_status = "0_RESULTS"
            self._validation_message = translate("Arch", "Query is valid, but found 0 objects.")
            self._validation_count = 0
        else:
            self._validation_status = "OK"
            self._validation_message = f"{translate('Arch', 'Found')} {count} {translate('Arch', 'objects')}."
            self._validation_count = count


class _ArchReportDocObserver:
    """Document observer that triggers report execution on recompute."""

    def __init__(self, doc, report):
        self.doc = doc
        self.report = report

    def slotRecomputedDocument(self, doc):
        if doc != self.doc:
            return
        self.report.Proxy.execute(self.report)


class _ArchReport:

    def __init__(self, obj):
        self.setProperties(obj)
        # Keep a reference to the host object so helper methods can persist data
        self.obj = obj
        obj.Proxy = self
        self.Type = 'ArchReport'
        self.spreadsheet = None
        self.docObserver = None
        self.spreadsheet_current_row = 1 # Internal state for multi-statement reports
        # This list holds the "live" ReportStatement objects for runtime use (UI, execute)
        self.live_statements = []
        # On creation, immediately hydrate the live list from the persistent property
        self.hydrate_live_statements(obj)
        # If no persisted statements were present, create one default statement
        # so the UI shows a starter entry (matching previous behavior).
        if not self.live_statements:
            default_stmt = ReportStatement(description=translate("Arch", "New Statement"))
            self.live_statements.append(default_stmt)
            # Persist the default starter statement so future loads see it
            try:
                self.commit_statements()
            except Exception:
                # Be resilient during early initialization when document context
                # may not be fully available; ignore commit failure.
                pass

    def onDocumentRestored(self, obj):
        """Called after the object properties are restored from a file."""
        # Rebuild the live list of objects from the newly loaded persistent data
        self.hydrate_live_statements(obj)
        self.setProperties(obj) # This will ensure observer is re-attached

    def hydrate_live_statements(self, obj):
        """(Re)builds the live list of objects from the stored list of dicts."""
        self.live_statements = []
        if hasattr(obj, 'Statements') and obj.Statements:
            for s_data in obj.Statements:
                statement = ReportStatement()
                statement.loads(s_data) # Use existing loads method
                self.live_statements.append(statement)

    def commit_statements(self):
        """
        Persists the live statements to the document object.

        This method serializes the list of live ReportStatement objects
        (self.live_statements) into a list of dictionaries and saves it
        to the persistent obj.Statements property. This is the official
        programmatic way to commit changes.
        """
        self.obj.Statements = [s.dumps() for s in self.live_statements]

    def setProperties(self, obj):
        # Ensure the `Statements` property exists (list of ReportStatement objects)
        if not 'Statements' in obj.PropertiesList:
            obj.addProperty('App::PropertyPythonObject', 'Statements', 'Report', QT_TRANSLATE_NOOP('App::Property', 'The list of SQL statements to execute (managed by the Task Panel)'), locked=True)
            obj.Statements = [] # Initialize with an empty list

        if not 'Target' in obj.PropertiesList:
            obj.addProperty('App::PropertyLink', 'Target', 'Report', QT_TRANSLATE_NOOP('App::Property', 'The spreadsheet for the results'))
        if not 'AutoUpdate' in obj.PropertiesList:
            obj.addProperty('App::PropertyBool', 'AutoUpdate', 'Report', QT_TRANSLATE_NOOP('App::Property', 'If True, update report when document recomputes'))
            obj.AutoUpdate = True

        self.onChanged(obj, 'AutoUpdate')
        # Make the Statements property read-only in the GUI to guide users to the TaskPanel.
        # Mode 1: Read-Only. It does not affect scripting access.
        if FreeCAD.GuiUp:
            obj.setEditorMode("Statements", 1)

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
            sp.addProperty('App::PropertyString', 'ReportName', 'Report', QT_TRANSLATE_NOOP('App::Property', 'The name of the BIM Report that uses this spreadsheet'))
        sp.ReportName = obj.Name
        obj.Target = sp

    def getSpreadSheet(self, obj, force=False):
        """Find or (optionally) create the spreadsheet associated with a report.

        The association is persisted via the sheet's ``ReportName`` string.

        Parameters
        - obj: the report object
        - force: if True, create a new spreadsheet when none is found
        """
        sp = getattr(self, 'spreadsheet', None)
        if sp and getattr(sp, 'ReportName', None) == obj.Name:
            return sp

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

            if prop == 'Statements':
                # If the persistent data is changed externally (e.g., by a script),
                # re-hydrate the live list to ensure consistency.
                self.hydrate_live_statements(obj)

    def __getstate__(self):
        """Returns minimal internal state of the proxy for serialization."""
        # The main 'Statements' data is persisted on the obj property, not here.
        return {
            'Type': self.Type,
        }

    def __setstate__(self, state):
        """Restores minimal internal state of the proxy from serialized data."""
        self.Type = state.get('Type', 'ArchReport')
        self.spreadsheet = None
        self.docObserver = None

    def _write_cell(self, spreadsheet, cell_address, value):
        """Intelligently writes a value to a spreadsheet cell based on its type."""
        # Handle FreeCAD Quantity objects by extracting their raw numerical value.
        if isinstance(value, FreeCAD.Units.Quantity):
            spreadsheet.set(cell_address, str(value.Value))
        elif isinstance(value, (int, float)):
            # Write other numbers directly without quotes for calculations.
            spreadsheet.set(cell_address, str(value))
        elif value is None:
            # Write an empty literal string for None.
            spreadsheet.set(cell_address, "''")
        else:
            # Write all other types (e.g., strings) as literal strings.
            spreadsheet.set(cell_address, f"'{value}")

    def setSpreadsheetData(self, obj, headers, data_rows, start_row,
                           use_description_as_header=False, description_text="",
                           include_column_names=True,
                           add_empty_row_after=False, print_results_in_bold=False,
                           force=False):
        """Write headers and rows into the report's spreadsheet, starting from a specific row."""
        sp = obj.Target # Always use obj.Target directly as it's the explicit link
        if not sp: # ensure spreadsheet exists, this is an error condition
            FreeCAD.Console.PrintError(f"Report '{getattr(obj, 'Label', '')}': No target spreadsheet found.\n")
            return start_row # Return current row unchanged

        # Determine the effective starting row for this block of data
        current_row = start_row

        # --- "Analyst-First" Header Generation ---
        # Pre-scan the first data row to find the common unit for each column.
        unit_map = {} # e.g., {1: 'mm', 2: 'mm'}

        if data_rows:
            for i, cell_value in enumerate(data_rows[0]):
                if isinstance(cell_value, FreeCAD.Units.Quantity):
                    unit_map[i] = cell_value.getUserPreferred()[2]

        # Create the final headers, appending units where found.
        final_headers = []
        for i, header_text in enumerate(headers):
            if i in unit_map:
                final_headers.append(f"{header_text} ({unit_map[i]})")
            else:
                final_headers.append(header_text)

        # Add header for this statement if requested
        if use_description_as_header and description_text.strip():
            # Merging the header across columns (A to last data column)
            last_col_char = chr(ord('A') + len(final_headers) - 1) if final_headers else 'A'
            sp.set(f"A{current_row}", f"'{description_text}")
            sp.mergeCells(f"A{current_row}:{last_col_char}{current_row}")
            sp.setStyle(f"A{current_row}", 'bold', 'add')
            current_row += 1 # Advance row for data or column names

        # Write column names if requested
        if include_column_names and final_headers:
            for col_idx, header_text in enumerate(final_headers):
                sp.set(f"{chr(ord('A') + col_idx)}{current_row}", f"'{header_text}")
            sp.setStyle(f'A{current_row}:{chr(ord("A") + len(final_headers) - 1)}{current_row}', 'bold', 'add')
            current_row += 1 # Advance row for data

        # Write data rows
        for row_data in data_rows:
            for col_idx, cell_value in enumerate(row_data):
                cell_address = f"{chr(ord('A') + col_idx)}{current_row}"
                self._write_cell(sp, cell_address, cell_value)
                if print_results_in_bold:
                    sp.setStyle(cell_address, 'bold', 'add')
            current_row += 1 # Advance row for next data row

        # Add empty row if specified
        if add_empty_row_after:
            current_row += 1 # Just increment row, leave it blank

        return current_row # Return the next available row

    def execute(self, obj):
        # Execute using the live list of objects, managed by the proxy
        if not self.live_statements:
            return

        sp = obj.Target # Ensure we use the explicitly linked Target
        if not sp: # ensure spreadsheet exists, this is an error condition
            FreeCAD.Console.PrintError(f"Report '{getattr(obj, 'Label', '')}': No target spreadsheet found.\n")
            return
        sp.clearAll() # Clear the spreadsheet for a new report

        self.spreadsheet_current_row = 1 # Reset row counter for a new report build

        for statement in self.live_statements:
            # Skip empty queries (user may have an empty placeholder statement)
            if not statement.query_string or not statement.query_string.strip():
                continue

            try:
                # Arch.select will log detailed errors and then re-raise an exception.
                # We catch it here to write a user-friendly message to the spreadsheet.
                headers, results_data = Arch.select(statement.query_string)
            except Arch.SqlEngineError as e:
                # On failure, record the error in the spreadsheet.
                self.spreadsheet_current_row = self.setSpreadsheetData(
                    obj,
                    ["Error"],
                    [[str(e)]],
                    self.spreadsheet_current_row,
                    use_description_as_header=True, # Give context to the error
                    description_text=statement.description,
                    include_column_names=False, # Don't print "Error" twice
                    print_results_in_bold=True
                )
                self.spreadsheet_current_row += 1 # Add a space after the error row
                continue # Move to the next statement

            self.spreadsheet_current_row = self.setSpreadsheetData(
                obj,
                headers,
                results_data,
                self.spreadsheet_current_row,
                use_description_as_header=statement.use_description_as_header,
                description_text=statement.description,
                include_column_names=statement.include_column_names,
                add_empty_row_after=statement.add_empty_row_after,
                print_results_in_bold=statement.print_results_in_bold,
                force=False # Spreadsheet already exists
            )

        sp.recompute()
        sp.purgeTouched()


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
        """Called by the C++ loader when the view provider is rehydrated."""
        self.vobj = vobj # Ensure self.vobj is set for consistent access

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
    """Multi-statement task panel for editing a Report.

    Exposes `self.form` as a QWidget so it works with FreeCADGui.Control.showDialog(panel).
    Implements accept() and reject() to save or discard changes.
    """

    # A static blocklist of common, non-queryable properties to exclude
    # from the autocompletion list to reduce noise.
    PROPERTY_BLOCKLIST = {
        "ExpressionEngine", "Label2", "Proxy", "ShapeColor", "Visibility",
        "LineColor", "LineWidth", "PointColor", "PointSize"
    }

    def __init__(self, report_obj):
        # Create two top-level widgets so FreeCAD will wrap each into a TaskBox.
        # Box 1 (overview) contains the statements table and management buttons.
        # Box 2 (editor) contains the query editor and options.
        self.obj = report_obj
        self.current_edited_statement_index = -1  # To track which statement is in editor
        self.is_dirty = False # To track uncommitted changes

        # Overview widget (TaskBox 1)
        self.overview_widget = QtWidgets.QWidget()
        self.overview_widget.setWindowTitle(translate("Arch", "Report Statements"))
        self.statements_overview_widget = self.overview_widget  # preserve older name
        self.statements_overview_layout = QtWidgets.QVBoxLayout(self.statements_overview_widget)

        # Table for statements: Description | Header | Cols | Status
        self.table_statements = QtWidgets.QTableWidget()
        self.table_statements.setColumnCount(4)  # Description, Header, Cols, Status
        self.table_statements.setHorizontalHeaderLabels([
            translate("Arch", "Description"),
            translate("Arch", "Header"),
            translate("Arch", "Cols"),
            translate("Arch", "Status"),
        ])

        # Add informative tooltips to the headers
        self.table_statements.horizontalHeaderItem(0).setToolTip(translate("Arch", "A user-defined description for this statement."))
        self.table_statements.horizontalHeaderItem(1).setToolTip(translate("Arch", "If checked, the Description will be used as a section header in the report."))
        self.table_statements.horizontalHeaderItem(2).setToolTip(translate("Arch", "If checked, the column names (e.g., 'Label', 'Area') will be included in the report."))
        self.table_statements.horizontalHeaderItem(3).setToolTip(translate("Arch", "Indicates the status of the SQL query."))

        # Description stretches, others sized to contents
        self.table_statements.horizontalHeader().setSectionResizeMode(0, QtWidgets.QHeaderView.Stretch)
        self.table_statements.horizontalHeader().setSectionResizeMode(1, QtWidgets.QHeaderView.ResizeToContents)
        self.table_statements.horizontalHeader().setSectionResizeMode(2, QtWidgets.QHeaderView.ResizeToContents)
        self.table_statements.horizontalHeader().setSectionResizeMode(3, QtWidgets.QHeaderView.ResizeToContents)
        self.table_statements.setSelectionBehavior(QtWidgets.QAbstractItemView.SelectRows)
        self.table_statements.setSelectionMode(QtWidgets.QAbstractItemView.SingleSelection)
        self.table_statements.setDragDropMode(QtWidgets.QAbstractItemView.InternalMove)
        self.statements_overview_layout.addWidget(self.table_statements)

        # Template controls for full reports
        self.template_layout = QtWidgets.QHBoxLayout()
        self.template_dropdown = NoScrollHijackComboBox()
        self.template_dropdown.setToolTip(translate("Arch", "Load a full report template, replacing all current statements."))
        self.btn_save_template = QtWidgets.QPushButton(translate("Arch", "Save as Template..."))
        self.btn_save_template.setToolTip(translate("Arch", "Save the current set of statements as a new report template."))
        self.btn_save_template.setIcon(FreeCADGui.getIcon(":/icons/document-save.svg"))
        self.template_layout.addWidget(self.template_dropdown)
        self.template_layout.addWidget(self.btn_save_template)
        template_label = QtWidgets.QLabel(translate("Arch", "Report Templates:"))
        self.statements_overview_layout.addWidget(template_label)
        self.statements_overview_layout.addLayout(self.template_layout)

        # Statement Management Buttons
        self.statement_buttons_layout = QtWidgets.QHBoxLayout()
        self.btn_add_statement = QtWidgets.QPushButton(ICON_ADD, translate("Arch", "Add Statement"))
        self.btn_add_statement.setToolTip(translate("Arch", "Add a new blank statement to the report."))
        self.btn_remove_statement = QtWidgets.QPushButton(ICON_REMOVE, translate("Arch", "Remove Selected"))
        self.btn_remove_statement.setToolTip(translate("Arch", "Remove the selected statement from the report."))
        self.btn_duplicate_statement = QtWidgets.QPushButton(ICON_DUPLICATE, translate("Arch", "Duplicate Selected"))
        self.btn_duplicate_statement.setToolTip(translate("Arch", "Create a copy of the selected statement."))

        self.statement_buttons_layout.addWidget(self.btn_add_statement)
        self.statement_buttons_layout.addWidget(self.btn_remove_statement)
        self.statement_buttons_layout.addWidget(self.btn_duplicate_statement)
        self.statements_overview_layout.addLayout(self.statement_buttons_layout)

        # Editor widget (TaskBox 2) -- starts collapsed until a statement is selected
        self.editor_widget = QtWidgets.QWidget()
        # Use a neutral title; the specific statement is displayed when selected
        self.editor_widget.setWindowTitle(translate("Arch", "Edit Statement"))
        # Keep compatibility name used elsewhere
        self.editor_box = self.editor_widget
        self.editor_layout = QtWidgets.QVBoxLayout(self.editor_box)

        # --- Form Layout for Aligned Inputs ---
        self.form_layout = QtWidgets.QFormLayout()
        self.form_layout.setContentsMargins(0, 0, 0, 0) # Use the main layout's margins

        # Description Row
        self.description_edit = QtWidgets.QLineEdit()
        self.form_layout.addRow(translate("Arch", "Description:"), self.description_edit)

        # Preset Controls Row (widgets are placed in a QHBoxLayout for the second column)
        self.preset_controls_layout = QtWidgets.QHBoxLayout()
        self.query_preset_dropdown = NoScrollHijackComboBox()
        self.query_preset_dropdown.setToolTip(translate("Arch", "Load a saved query preset into the editor."))
        self.btn_save_query_preset = QtWidgets.QPushButton(translate("Arch", "Save..."))
        self.btn_save_query_preset.setToolTip(translate("Arch", "Save the current query as a new preset."))
        self.preset_controls_layout.addWidget(self.query_preset_dropdown)
        self.preset_controls_layout.addWidget(self.btn_save_query_preset)
        self.form_layout.addRow(translate("Arch", "Query Presets:"), self.preset_controls_layout)

        self.editor_layout.addLayout(self.form_layout)

        # SQL Query editor
        self.sql_label = QtWidgets.QLabel(translate("Arch", "SQL Query:"))
        self.sql_query_edit = SqlQueryEditor()
        self.sql_query_status_label = QtWidgets.QLabel(translate("Arch", "Ready"))
        self.sql_query_status_label.setTextInteractionFlags(QtCore.Qt.TextSelectableByMouse)
        # Enable word wrapping to prevent long error messages from expanding the panel.
        self.sql_query_status_label.setWordWrap(True)
        # Set a dynamic minimum height of 2 lines to prevent layout shifting
        # when the label's content changes from 1 to 2 lines.
        font_metrics = QtGui.QFontMetrics(self.sql_query_status_label.font())
        two_lines_height = 2.5 * font_metrics.height()
        self.sql_query_status_label.setMinimumHeight(two_lines_height)

        # --- Attach Syntax Highlighter ---
        self.sql_highlighter = SqlSyntaxHighlighter(self.sql_query_edit.document())

        # --- Setup Autocompletion ---
        self.completer = QtWidgets.QCompleter(self.sql_query_edit)
        self.completion_model = self._build_completion_model()
        self.completer.setModel(self.completion_model)
        self.completer.setCaseSensitivity(QtCore.Qt.CaseInsensitive)
        # We use a custom keyPressEvent in SqlQueryEditor to handle Tab/Enter
        self.sql_query_edit.setCompleter(self.completer)

        self.editor_layout.addWidget(self.sql_label)
        self.editor_layout.addWidget(self.sql_query_edit)
        self.editor_layout.addWidget(self.sql_query_status_label)

        # --- On-Demand Preview ---
        self.preview_layout = QtWidgets.QHBoxLayout()
        self.btn_preview_results = QtWidgets.QPushButton(translate("Arch", "Preview Results"))
        self.btn_preview_results.setIcon(FreeCADGui.getIcon(":/icons/Std_ToggleVisibility.svg"))
        self.btn_preview_results.setToolTip(translate("Arch", "Execute the current query and show the results in a table below."))
        self.btn_show_cheatsheet = QtWidgets.QPushButton(translate("Arch", "Help"))
        self.btn_show_cheatsheet.setIcon(FreeCADGui.getIcon(":/icons/help-browser.svg"))
        self.btn_show_cheatsheet.setToolTip(translate("Arch", "Show a cheatsheet of the supported SQL syntax."))
        self.preview_layout.addStretch()
        self.preview_layout.addWidget(self.btn_preview_results)
        #self.preview_layout.addStretch()
        self.preview_layout.addWidget(self.btn_show_cheatsheet)
        self.editor_layout.addLayout(self.preview_layout)

        self.table_preview_results = QtWidgets.QTableWidget()
        self.table_preview_results.setVisible(False) # Start hidden
        self.table_preview_results.setMinimumHeight(150)
        self.table_preview_results.setEditTriggers(QtWidgets.QAbstractItemView.NoEditTriggers) # Make read-only
        self.table_preview_results.setSelectionBehavior(QtWidgets.QAbstractItemView.SelectRows)
        self.editor_layout.addWidget(self.table_preview_results)

        # Display Options GroupBox
        self.display_options_group = QtWidgets.QGroupBox(translate("Arch", "Display Options"))
        self.display_options_layout = QtWidgets.QVBoxLayout(self.display_options_group)

        self.chk_use_description_as_header = QtWidgets.QCheckBox(translate("Arch", "Use Description as Section Header"))
        self.chk_use_description_as_header.setToolTip(translate("Arch", "When checked, the statement's description will be written as a merged header row before its results."))
        self.chk_include_column_names = QtWidgets.QCheckBox(translate("Arch", "Include Column Names"))
        self.chk_include_column_names.setToolTip(translate("Arch", "Include the column headers (Label, IfcType, ...) in the spreadsheet output."))
        self.chk_add_empty_row_after = QtWidgets.QCheckBox(translate("Arch", "Add Empty Row After"))
        self.chk_add_empty_row_after.setToolTip(translate("Arch", "Insert one empty row after this statement's results."))
        self.chk_print_results_in_bold = QtWidgets.QCheckBox(translate("Arch", "Print Results in Bold"))
        self.chk_print_results_in_bold.setToolTip(translate("Arch", "Render the result cells in bold font for emphasis."))
        self.display_options_layout.addWidget(self.chk_use_description_as_header)
        self.display_options_layout.addWidget(self.chk_include_column_names)
        self.display_options_layout.addWidget(self.chk_add_empty_row_after)
        self.display_options_layout.addWidget(self.chk_print_results_in_bold)
        self.editor_layout.addWidget(self.display_options_group)

        # Expose form as a list of the two top-level widgets so FreeCAD creates
        # two built-in TaskBox sections. The overview goes first, editor second.
        self.form = [self.overview_widget, self.editor_widget]

        # --- Connections ---
        self.btn_add_statement.clicked.connect(self._add_statement)
        self.btn_remove_statement.clicked.connect(self._remove_selected_statement)
        self.btn_duplicate_statement.clicked.connect(self._duplicate_selected_statement)
        self.table_statements.itemSelectionChanged.connect(self._on_table_selection_changed)
        self.template_dropdown.activated.connect(self._on_load_report_template)
        self.btn_save_template.clicked.connect(self._on_save_report_template)
        # Keep table edits in sync with the runtime statements
        self.table_statements.itemChanged.connect(self._on_table_item_changed)
        # Connect all editor fields to a single handler to manage dirty state and UI sync.
        self.description_edit.textChanged.connect(self._on_editor_field_changed)
        self.sql_query_edit.textChanged.connect(self._on_editor_sql_changed)
        for checkbox in self.display_options_group.findChildren(QtWidgets.QCheckBox):
            checkbox.stateChanged.connect(self._on_editor_field_changed)
        self.query_preset_dropdown.activated.connect(self._on_load_query_preset)
        self.btn_save_query_preset.clicked.connect(self._on_save_query_preset)
        self.btn_show_cheatsheet.clicked.connect(self._show_cheatsheet_dialog)
        self.btn_preview_results.clicked.connect(self._on_preview_results_clicked)

        # Validation Timer for live SQL preview
        # Timer doesn't need a specific QWidget parent here; use no parent.
        self.validation_timer = QtCore.QTimer()
        self.validation_timer.setSingleShot(True)
        self.validation_timer.timeout.connect(self._run_live_validation_for_editor)

        # Initial UI setup
        self._load_and_populate_presets()
        self._populate_table_from_statements()
        # Pass the documentation data to the editor for its tooltips
        api_docs = Arch.getSqlApiDocumentation()
        self.sql_query_edit.set_api_documentation(api_docs)
        self._update_editor_visibility(False)  # Start with editor collapsed/hidden
        # Do not auto-select the first statement; leave editor collapsed until user selects

    def _load_and_populate_presets(self):
        """Loads all presets from disk and populates the UI dropdowns."""
        self.query_presets = _get_presets('query')
        self.query_preset_dropdown.clear()
        self.query_preset_dropdown.addItem(translate("Arch", "--- Select a Query Preset ---"))
        self.query_preset_dropdown.addItems(sorted(self.query_presets.keys()))

        self.report_templates = _get_presets('template')
        self.template_dropdown.clear()
        self.template_dropdown.addItem(translate("Arch", "--- Load a Report Template ---"))
        self.template_dropdown.addItems(sorted(self.report_templates.keys()))

    # --- Statement Management (Buttons and Table Interaction) ---
    def _populate_table_from_statements(self):
        # Avoid emitting itemChanged while we repopulate programmatically
        self.table_statements.blockSignals(True)
        self.table_statements.setRowCount(0) # Clear existing rows
        # The UI always interacts with the live list of objects from the proxy
        for row_idx, statement in enumerate(self.obj.Proxy.live_statements):
            self.table_statements.insertRow(row_idx)
            # Description (editable text)
            desc_item = QtWidgets.QTableWidgetItem(statement.description)
            desc_item.setFlags(desc_item.flags() | QtCore.Qt.ItemIsEditable | QtCore.Qt.ItemIsSelectable | QtCore.Qt.ItemIsEnabled)
            desc_item.setToolTip(translate("Arch", "Double-click to edit description in place."))
            self.table_statements.setItem(row_idx, 0, desc_item)

            # Header checkbox
            header_item = QtWidgets.QTableWidgetItem()
            header_item.setFlags(QtCore.Qt.ItemIsUserCheckable | QtCore.Qt.ItemIsEnabled | QtCore.Qt.ItemIsSelectable)
            header_item.setCheckState(QtCore.Qt.Checked if statement.use_description_as_header else QtCore.Qt.Unchecked)
            header_item.setToolTip(translate("Arch", "Toggle whether to use this statement's Description as a section header."))
            self.table_statements.setItem(row_idx, 1, header_item)

            # Cols checkbox (Include Column Names)
            cols_item = QtWidgets.QTableWidgetItem()
            cols_item.setFlags(QtCore.Qt.ItemIsUserCheckable | QtCore.Qt.ItemIsEnabled | QtCore.Qt.ItemIsSelectable)
            cols_item.setCheckState(QtCore.Qt.Checked if statement.include_column_names else QtCore.Qt.Unchecked)
            cols_item.setToolTip(translate("Arch", "Toggle whether to include this statement's column names in the report."))
            self.table_statements.setItem(row_idx, 2, cols_item)

            # Status Item (Icon + Tooltip) - read-only
            status_icon, status_tooltip = self._get_status_icon_and_tooltip(statement)
            status_item = QtWidgets.QTableWidgetItem()
            status_item.setIcon(status_icon)
            status_item.setToolTip(status_tooltip)
            status_item.setFlags(status_item.flags() & ~QtCore.Qt.ItemIsEditable) # Make read-only
            self.table_statements.setItem(row_idx, 3, status_item)

            # Recalculate status for each statement (important on load)
            statement.validate_and_update_status()
        # Re-enable signals after population so user edits are handled
        self.table_statements.blockSignals(False)

    def _on_table_item_changed(self, item):
        # Synchronize direct table edits (description and checkboxes) back into the runtime statement
        row = item.row()
        col = item.column()
        if row < 0 or row >= len(self.obj.Proxy.live_statements):
            return
        stmt = self.obj.Proxy.live_statements[row]

        if col == 0:
            # Description text edited
            new_text = item.text()
            stmt.description = new_text
            stmt.validate_and_update_status()
            self._update_table_row_status(row, stmt)
            if row == self.current_edited_statement_index:
                # Prevent echo loops by blocking editor signals briefly
                try:
                    self.description_edit.blockSignals(True)
                    self.description_edit.setText(new_text)
                finally:
                    self.description_edit.blockSignals(False)
        elif col == 1:
            # Header checkbox toggled
            stmt.use_description_as_header = (item.checkState() == QtCore.Qt.Checked)
            stmt.validate_and_update_status()
            self._update_table_row_status(row, stmt)
            # If this row is currently edited, update the editor checkbox to match
            if row == self.current_edited_statement_index:
                self.chk_use_description_as_header.setChecked(stmt.use_description_as_header)
        elif col == 2:
            # Cols checkbox toggled -> include_column_names
            stmt.include_column_names = (item.checkState() == QtCore.Qt.Checked)
            stmt.validate_and_update_status()
            self._update_table_row_status(row, stmt)
            if row == self.current_edited_statement_index:
                self.chk_include_column_names.setChecked(stmt.include_column_names)

        self._set_dirty(True)

    def _add_statement(self):
        # Modify the live list; changes will be committed to the object property on accept()
        new_statement = ReportStatement(description=translate("Arch", f"New Statement {len(self.obj.Proxy.live_statements) + 1}"))
        self.obj.Proxy.live_statements.append(new_statement)
        self._populate_table_from_statements() # Refresh the table
        new_statement.validate_and_update_status() # Validate immediately
        self._select_statement_in_table(len(self.obj.Proxy.live_statements) - 1)
        self._set_dirty(True)

    def _remove_selected_statement(self):
        selected_rows = self.table_statements.selectionModel().selectedRows()
        if not selected_rows:
            return

        row_to_remove = selected_rows[0].row()
        description_to_remove = self.table_statements.item(row_to_remove, 0).text()

        if QtWidgets.QMessageBox.question(None, translate("Arch", "Remove Statement"),
                                          translate("Arch", f"Are you sure you want to remove statement '{description_to_remove}'?"),
                                          QtWidgets.QMessageBox.Yes | QtWidgets.QMessageBox.No) == QtWidgets.QMessageBox.Yes:
            self.obj.Proxy.live_statements.pop(row_to_remove)
            self._set_dirty(True)
            self._populate_table_from_statements()
            self._select_first_statement_if_available()

    def _duplicate_selected_statement(self):
        selected_rows = self.table_statements.selectionModel().selectedRows()
        if not selected_rows:
            return

        row_to_duplicate = selected_rows[0].row()
        original_statement = self.obj.Proxy.live_statements[row_to_duplicate]

        duplicated_statement = ReportStatement()
        duplicated_statement.loads(original_statement.dumps()) # Use serialization to deep copy
        duplicated_statement.description = translate("Arch", f"Copy of {original_statement.description}")

        self.obj.Proxy.live_statements.insert(row_to_duplicate + 1, duplicated_statement)
        self._set_dirty(True)
        self._populate_table_from_statements()
        duplicated_statement.validate_and_update_status()
        self._select_statement_in_table(row_to_duplicate + 1)

    def _select_first_statement_if_available(self):
        if self.obj.Proxy.live_statements:
            self._select_statement_in_table(0)
        else:
            self._update_editor_visibility(False) # Hide editor if no statements

    def _select_statement_in_table(self, row_idx):
        # Select the row visually and trigger _on_table_selection_changed
        self.table_statements.selectRow(row_idx)
        # Ensure the editor is expanded if a statement is selected explicitly
        self._update_editor_visibility(True)


    # --- Editor (Box 2) Management ---
    def _on_table_selection_changed(self):
        selected_rows = self.table_statements.selectionModel().selectedRows()
        if selected_rows:
            new_index = selected_rows[0].row()
            if new_index != self.current_edited_statement_index:
                self._save_current_editor_state_to_statement() # Save old state
                self.current_edited_statement_index = new_index
                self._load_statement_to_editor(self.obj.Proxy.live_statements[new_index])
                self._update_editor_title(self.obj.Proxy.live_statements[new_index].description)
                self._run_live_validation_for_editor() # Validate the loaded query
                self._update_editor_visibility(True) # Ensure editor is visible
        else:
            self._save_current_editor_state_to_statement() # Save last edited state
            self.current_edited_statement_index = -1
            self._update_editor_visibility(False) # Hide editor if nothing selected

    def _load_statement_to_editor(self, statement: ReportStatement):
        self.description_edit.setText(statement.description)
        self.sql_query_edit.setPlainText(statement.query_string)
        self.chk_use_description_as_header.setChecked(statement.use_description_as_header)
        self.chk_include_column_names.setChecked(statement.include_column_names)
        self.chk_add_empty_row_after.setChecked(statement.add_empty_row_after)
        self.chk_print_results_in_bold.setChecked(statement.print_results_in_bold)
        # Update validation status label for loaded query
        self._update_editor_status_display(statement)

    def _save_current_editor_state_to_statement(self):
        if self.current_edited_statement_index != -1 and self.current_edited_statement_index < len(self.obj.Proxy.live_statements):
            statement = self.obj.Proxy.live_statements[self.current_edited_statement_index]
            statement.description = self.description_edit.text()
            statement.query_string = self.sql_query_edit.toPlainText()
            statement.use_description_as_header = self.chk_use_description_as_header.isChecked()
            statement.include_column_names = self.chk_include_column_names.isChecked()
            statement.add_empty_row_after = self.chk_add_empty_row_after.isChecked()
            statement.print_results_in_bold = self.chk_print_results_in_bold.isChecked()
            statement.validate_and_update_status() # Update status in the statement object
            self._update_table_row_status(self.current_edited_statement_index, statement) # Refresh table status

    def _on_editor_sql_changed(self):
        self._set_dirty(True)
        self.sql_query_status_label.setText(translate("Arch", "<i>Typing...</i>"))
        self.sql_query_status_label.setStyleSheet("color: gray;")
        self.validation_timer.start(500) # (Re)start timer for live validation

    def _on_editor_field_changed(self):
        """A generic slot that handles any change in an editor field."""
        self._set_dirty(True)
        # --- Specific logic for description sync ---
        if self.current_edited_statement_index != -1:
            # This provides the real-time two-way data binding for the description.
            item = self.table_statements.item(self.current_edited_statement_index, 0)
            if item and item.text() != self.description_edit.text():
                item.setText(self.description_edit.text())
            self._update_editor_title(self.description_edit.text())

        # --- Specific logic for checkboxes ---
        # If a statement is currently selected in the table, mirror the editor state to the table
        if self.current_edited_statement_index != -1:
            row = self.current_edited_statement_index
            header_item = self.table_statements.item(row, 1)
            if header_item:
                header_item.setCheckState(QtCore.Qt.Checked if self.chk_use_description_as_header.isChecked() else QtCore.Qt.Unchecked)
            cols_item = self.table_statements.item(row, 2)
            if cols_item:
                cols_item.setCheckState(QtCore.Qt.Checked if self.chk_include_column_names.isChecked() else QtCore.Qt.Unchecked)
            # Also update the underlying statement object to keep model consistent
            stmt = self.obj.Proxy.live_statements[row]
            stmt.use_description_as_header = self.chk_use_description_as_header.isChecked()
            stmt.include_column_names = self.chk_include_column_names.isChecked()
            stmt.validate_and_update_status()
            self._update_table_row_status(row, stmt)
        self._set_dirty(True)

    def _on_load_query_preset(self, index):
        """Handles the selection of a query preset from the dropdown."""
        if index == 0: # Ignore the placeholder item
            return

        preset_name = self.query_preset_dropdown.itemText(index)

        # Confirm before overwriting existing text
        if self.sql_query_edit.toPlainText().strip():
            reply = QtWidgets.QMessageBox.question(None, translate("Arch", "Overwrite Query?"),
                                                   translate("Arch", "Loading a preset will overwrite the current text in the query editor. Continue?"),
                                                   QtWidgets.QMessageBox.Yes | QtWidgets.QMessageBox.No, QtWidgets.QMessageBox.No)
            if reply == QtWidgets.QMessageBox.No:
                self.query_preset_dropdown.setCurrentIndex(0) # Reset dropdown
                return

        preset_data = self.query_presets.get(preset_name)
        if preset_data and 'query' in preset_data:
            self.sql_query_edit.setPlainText(preset_data['query'])

        self.query_preset_dropdown.setCurrentIndex(0) # Reset dropdown to act as a one-shot button

    def _on_save_query_preset(self):
        """Saves the current query text as a new user preset."""
        current_query = self.sql_query_edit.toPlainText().strip()
        if not current_query:
            QtWidgets.QMessageBox.warning(None, translate("Arch", "Empty Query"), translate("Arch", "Cannot save an empty query as a preset."))
            return

        preset_name, ok = QtWidgets.QInputDialog.getText(None, translate("Arch", "Save Query Preset"), translate("Arch", "Preset Name:"))
        if ok and preset_name:
            preset_data = {"description": "User-defined preset.", "query": current_query}
            _save_preset('query', preset_name, preset_data)
            self._load_and_populate_presets() # Refresh the dropdown with the new preset

    def _on_load_report_template(self, index):
        """Handles the selection of a full report template from the dropdown."""
        if index == 0:
            return

        template_name = self.template_dropdown.itemText(index)

        if self.obj.Proxy.live_statements:
            reply = QtWidgets.QMessageBox.question(None, translate("Arch", "Overwrite Report?"),
                                                   translate("Arch", "Loading a template will replace all current statements in this report. Continue?"),
                                                   QtWidgets.QMessageBox.Yes | QtWidgets.QMessageBox.No, QtWidgets.QMessageBox.No)
            if reply == QtWidgets.QMessageBox.No:
                self.template_dropdown.setCurrentIndex(0)
                return

        template_data = self.report_templates.get(template_name)
        if template_data and 'statements' in template_data:
            # Rebuild the live list from the template data
            self.obj.Proxy.live_statements = []
            for s_data in template_data['statements']:
                statement = ReportStatement()
                statement.loads(s_data)
                self.obj.Proxy.live_statements.append(statement)

            self._populate_table_from_statements()
            self._update_editor_visibility(False)
            self._set_dirty(True)

        self.template_dropdown.setCurrentIndex(0)

    def _on_save_report_template(self):
        """Saves the current set of statements as a new report template."""
        template_name, ok = QtWidgets.QInputDialog.getText(None, translate("Arch", "Save Report Template"), translate("Arch", "Template Name:"))
        if ok and template_name:
            template_data = {"description": "User-defined report template.", "statements": [s.dumps() for s in self.obj.Proxy.live_statements]}
            _save_preset('template', template_name, template_data)
            self._load_and_populate_presets() # Refresh the template dropdown

    def _run_live_validation_for_editor(self):
        # Validate the query currently in the editor's text area
        temp_statement = ReportStatement(query_string=self.sql_query_edit.toPlainText())
        temp_statement.validate_and_update_status()
        self._update_editor_status_display(temp_statement) # Update the status label in the editor UI

        # Also update table if this is the currently edited statement
        if self.current_edited_statement_index != -1:
            statement_obj = self.obj.Proxy.live_statements[self.current_edited_statement_index]
            statement_obj.query_string = temp_statement.query_string # Keep statement object's query updated for validation.
            statement_obj.validate_and_update_status()
            self._update_table_row_status(self.current_edited_statement_index, statement_obj)


    def _update_editor_status_display(self, statement: ReportStatement):
        # Update the status label (below SQL editor) in Box 2
        if statement._validation_status == "INCOMPLETE":
            self.sql_query_status_label.setText(translate("Arch", "<i>Typing...</i>"))
            self.sql_query_status_label.setStyleSheet("color: gray;")
        elif statement._validation_status == "ERROR":
            self.sql_query_status_label.setText(f"❌ {statement._validation_message}")
            self.sql_query_status_label.setStyleSheet("color: red;")
        elif statement._validation_status == "0_RESULTS":
            self.sql_query_status_label.setText(f"⚠️ {statement._validation_message}")
            self.sql_query_status_label.setStyleSheet("color: orange;")
        else: # OK or Ready
            self.sql_query_status_label.setText(f"✅ {statement._validation_message}")
            self.sql_query_status_label.setStyleSheet("color: green;")

    def _update_table_row_status(self, row_idx, statement: ReportStatement):
        # Update the status icon/tooltip in the QTableWidget (Box 1)
        # Status column moved to index 3
        status_item = self.table_statements.item(row_idx, 3)
        if status_item:
            status_icon, status_tooltip = self._get_status_icon_and_tooltip(statement)
            status_item.setIcon(status_icon)
            status_item.setToolTip(status_tooltip)

        # Update description in table in case it was changed via editor
        desc_item = self.table_statements.item(row_idx, 0)
        if desc_item:
            desc_item.setText(statement.description)

        # Update checkbox columns to reflect current statement state
        header_item = self.table_statements.item(row_idx, 1)
        if header_item:
            header_item.setCheckState(QtCore.Qt.Checked if statement.use_description_as_header else QtCore.Qt.Unchecked)
        cols_item = self.table_statements.item(row_idx, 2)
        if cols_item:
            cols_item.setCheckState(QtCore.Qt.Checked if statement.include_column_names else QtCore.Qt.Unchecked)


    def _get_status_icon_and_tooltip(self, statement: ReportStatement):
        # Helper to get appropriate icon and tooltip for table status column
        status = statement._validation_status
        message = statement._validation_message

        if status == "OK":
            return ICON_STATUS_OK, message
        elif status == "0_RESULTS":
            return ICON_STATUS_WARN, message
        elif status == "ERROR":
            return ICON_STATUS_ERROR, message
        elif status == "INCOMPLETE":
            return ICON_STATUS_INCOMPLETE, translate("Arch", "Query incomplete or typing...")
        return QtGui.QIcon(), translate("Arch", "Ready") # Default/initial state

    def _update_editor_title(self, description):
        # Updates the title of the collapsible editor box (Box 2)
        if description.strip():
            self.editor_box.setWindowTitle(translate("Arch", f"Edit Statement: {description}"))
        else:
            self.editor_box.setWindowTitle(translate("Arch", "Edit Statement: (No Description)"))

    def _update_editor_visibility(self, visible: bool):
        # Controls the visibility/collapsing of the editor box
        self.editor_box.setVisible(visible)
        # Change arrow indicator (Qt handles this if QGroupBox is collapsible,
        # but we control visibility directly here.)

    def _set_dirty(self, dirty_state):
        """Updates the UI to show if there are uncommitted changes."""
        if self.is_dirty == dirty_state:
            return
        self.is_dirty = dirty_state
        title = translate("Arch", "Report Statements")
        if self.is_dirty:
            title += " *"
        self.overview_widget.setWindowTitle(title)

    def _show_cheatsheet_dialog(self):
        """Gets the API documentation and displays it in a dialog."""
        api_data = Arch.getSqlApiDocumentation()
        dialog = CheatsheetDialog(api_data, parent=self.editor_widget)
        dialog.exec_()

    def _on_preview_results_clicked(self):
        """Handles the 'Preview Results' button click."""
        query = self.sql_query_edit.toPlainText().strip()

        if not query:
            # If query is empty, just clear and hide the table
            self.table_preview_results.clear()
            self.table_preview_results.setRowCount(0)
            self.table_preview_results.setColumnCount(0)
            self.table_preview_results.setVisible(False)
            return

        try:
            # Execute the query using the Arch.select API, which raises on failure
            headers, data_rows = Arch.select(query)
            # --- Success Case: Populate the table with results ---
            self.table_preview_results.clear()
            self.table_preview_results.setColumnCount(len(headers))
            self.table_preview_results.setHorizontalHeaderLabels(headers)
            self.table_preview_results.setRowCount(len(data_rows))

            for row_idx, row_data in enumerate(data_rows):
                for col_idx, cell_value in enumerate(row_data):
                    item = QtWidgets.QTableWidgetItem(str(cell_value))
                    self.table_preview_results.setItem(row_idx, col_idx, item)

            # Restore default resize mode in case it was changed for error display
            self.table_preview_results.horizontalHeader().setSectionResizeMode(QtWidgets.QHeaderView.Interactive)

        except Arch.SqlEngineError as e:
            # --- Failure Case: Display the error in the preview table ---
            self.table_preview_results.clear()
            self.table_preview_results.setRowCount(1)
            self.table_preview_results.setColumnCount(1)
            self.table_preview_results.setHorizontalHeaderLabels(["Query Error"])
            error_item = QtWidgets.QTableWidgetItem(f"❌ {str(e)}")
            error_item.setForeground(QtGui.QColor("red"))
            self.table_preview_results.setItem(0, 0, error_item)
            # Make the single error column stretch to fill the available space
            self.table_preview_results.horizontalHeader().setSectionResizeMode(0, QtWidgets.QHeaderView.Stretch)

        self.table_preview_results.setVisible(True) # Always show the table for results or errors

    def _build_completion_model(self):
        """
        Builds the master list of words for the autocompleter.

        This method scans for all SQL keywords and all unique, queryable
        property names across all objects in the active document.
        """
        # 1. Start with the static SQL keywords and functions.
        all_words = set(Arch.getSqlKeywords())

        # 2. Add all unique property names from all objects in the document.
        if FreeCAD.ActiveDocument:
            property_names = set()
            for obj in FreeCAD.ActiveDocument.Objects:
                for prop_name in obj.PropertiesList:
                    if prop_name not in self.PROPERTY_BLOCKLIST:
                        property_names.add(prop_name)
            all_words.update(property_names)

        # 3. Return a sorted model for the completer.
        return QtCore.QStringListModel(sorted(list(all_words)))

    # --- Dialog Acceptance / Rejection ---
    def accept(self):
        """Saves changes from UI to Report object and triggers recompute."""
        # Ensure the currently edited statement's state is saved before final accept
        self._save_current_editor_state_to_statement()
        # First, ensure the currently edited statement's state is saved to the live list

        # This is the "commit" step: convert the live list of objects into a
        # Delegate to proxy helper to persist the live statements
        self.obj.Proxy.commit_statements()

        # Trigger a recompute to run the report and mark the document as modified
        FreeCAD.ActiveDocument.recompute()

        # Quality of life: open the target spreadsheet to show the results upon manual accept
        spreadsheet = self.obj.Target
        if spreadsheet:
            FreeCADGui.ActiveDocument.setEdit(spreadsheet.Name, 0)

        # Close the task panel via FreeCADGui when GUI is available
        try:
            FreeCADGui.Control.closeDialog()
        except Exception as e:
            # This is a defensive catch. If closing the dialog fails for any reason
            # (e.g., it was already closed), we log the error but do not crash.
            FreeCAD.Console.PrintLog(f"Could not close Report Task Panel: {e}\n")
        self._set_dirty(False)

    def reject(self):
        """Closes dialog without saving changes to the Report object."""
        # Revert changes by not writing to self.obj.Statements
        # Discard live changes by re-hydrating from the persisted property
        self.obj.Proxy.hydrate_live_statements(self.obj)
        self._set_dirty(False)
        # Close the task panel when GUI is available
        try:
            FreeCADGui.Control.closeDialog()
        except Exception as e:
            # This is a defensive catch. If closing the dialog fails for any reason
            # (e.g., it was already closed), we log the error but do not crash.
            FreeCAD.Console.PrintLog(f"Could not close Report Task Panel: {e}\n")

if FreeCAD.GuiUp:
    class NoScrollHijackComboBox(QtWidgets.QComboBox):
        """
        A custom QComboBox that only processes wheel events when its popup view is visible.
        This prevents it from "hijacking" the scroll wheel from a parent QScrollArea.
        """
        def wheelEvent(self, event):
            if self.view().isVisible():
                # If the widget has focus, perform the default scrolling action.
                super().wheelEvent(event)
            else:
                # If the popup is not visible, ignore the event. This allows
                # the event to propagate to the parent widget (the scroll area).
                event.ignore()

    class SqlSyntaxHighlighter(QtGui.QSyntaxHighlighter):
        """
        Custom QSyntaxHighlighter for SQL syntax.
        """
        def __init__(self, parent_text_document):
            super().__init__(parent_text_document)

            # --- Define Formatting Rules ---
            keyword_format = QtGui.QTextCharFormat()
            keyword_format.setForeground(QtGui.QColor("#0070C0")) # Dark Blue
            keyword_format.setFontWeight(QtGui.QFont.Bold)

            function_format = QtGui.QTextCharFormat()
            function_format.setForeground(QtGui.QColor("#800080")) # Purple
            function_format.setFontItalic(True)

            string_format = QtGui.QTextCharFormat()
            string_format.setForeground(QtGui.QColor("#A31515")) # Dark Red

            comment_format = QtGui.QTextCharFormat()
            comment_format.setForeground(QtGui.QColor("#008000")) # Green
            comment_format.setFontItalic(True)

            # --- Build Rules List ---
            self.highlighting_rules = []

            # Keywords (case-insensitive regex)
            # Get the list of keywords from the SQL engine.
            for word in Arch.getSqlKeywords():
                pattern = QtCore.QRegExp(r"\b" + word + r"\b", QtCore.Qt.CaseInsensitive)
                rule = {"pattern": pattern, "format": keyword_format}
                self.highlighting_rules.append(rule)

            # Aggregate Functions (case-insensitive regex)
            functions = ["COUNT", "SUM", "MIN", "MAX"]
            for word in functions:
                pattern = QtCore.QRegExp(r"\b" + word + r"\b", QtCore.Qt.CaseInsensitive)
                rule = {"pattern": pattern, "format": function_format}
                self.highlighting_rules.append(rule)

            # String Literals (single quotes)
            # This regex captures everything between single quotes, allowing for escaped quotes
            string_pattern = QtCore.QRegExp(r"'[^'\\]*(\\.[^'\\]*)*'")
            self.highlighting_rules.append({"pattern": string_pattern, "format": string_format})
            # Also support double-quoted string literals (some SQL dialects use double quotes)
            double_string_pattern = QtCore.QRegExp(r'"[^"\\]*(\\.[^"\\]*)*"')
            self.highlighting_rules.append({"pattern": double_string_pattern, "format": string_format})

            # Single-line comments (starting with -- or #)
            comment_single_line_pattern = QtCore.QRegExp(r"--[^\n]*|\#[^\n]*")
            self.highlighting_rules.append({"pattern": comment_single_line_pattern, "format": comment_format})

            # Multi-line comments (/* ... */) - requires special handling in highlightBlock
            self.multi_line_comment_start_pattern = QtCore.QRegExp(r"/\*")
            self.multi_line_comment_end_pattern = QtCore.QRegExp(r"\*/")
            self.multi_line_comment_format = comment_format

        def highlightBlock(self, text):
            """
            Applies highlighting rules to the given text block.
            This method is called automatically by Qt for each visible text block.
            """
            # Reset format for the current block
            for rule in self.highlighting_rules:
                pattern = rule["pattern"]
                format = rule["format"]
                index = pattern.indexIn(text)
                while index >= 0:
                    length = pattern.matchedLength()
                    self.setFormat(index, length, format)
                    index = pattern.indexIn(text, index + length)

            # Handle multi-line comments
            self.setCurrentBlockState(0) # Default state (no comment)

            # Start from the correct index depending on whether the previous block
            # ended inside a multi-line comment. If the previous block was inside
            # a comment we continue scanning from the start of this block (0).
            # Otherwise, search for the opening comment marker in this block.
            if self.previousBlockState() == 1:  # State 1 means "inside multi-line comment"
                start_index = 0
            else:
                start_index = self.multi_line_comment_start_pattern.indexIn(text)

            while start_index >= 0:
                end_index = self.multi_line_comment_end_pattern.indexIn(text, start_index)
                comment_length = 0
                if end_index == -1: # No end tag found, so comment continues to end of block
                    self.setCurrentBlockState(1)
                    comment_length = len(text) - start_index
                else: # End tag found
                    comment_length = end_index - start_index + self.multi_line_comment_end_pattern.matchedLength()

                self.setFormat(start_index, comment_length, self.multi_line_comment_format)
                start_index = self.multi_line_comment_start_pattern.indexIn(text, start_index + comment_length)


    class CheatsheetDialog(QtWidgets.QDialog):
        """A simple dialog to display the HTML cheatsheet."""
        def __init__(self, api_data, parent=None):
            super().__init__(parent)
            self.setWindowTitle(translate("Arch", "BIM SQL Cheatsheet"))
            self.setMinimumSize(750, 600)
            layout = QtWidgets.QVBoxLayout(self)
            html = self._format_as_html(api_data)
            text_edit = QtWidgets.QTextEdit()
            text_edit.setReadOnly(True)
            text_edit.setHtml(html)
            layout.addWidget(text_edit)
            button_box = QtWidgets.QDialogButtonBox(QtWidgets.QDialogButtonBox.Ok)
            button_box.accepted.connect(self.accept)
            layout.addWidget(button_box)
            self.setLayout(layout)

        def _format_as_html(self, api_data: dict) -> str:
            """
            Takes the structured data from the API and builds the final HTML string.
            All presentation logic and translatable strings are contained here.
            """
            html = f"<h1>{translate('Arch', 'BIM SQL Cheatsheet')}</h1>"
            html += f"<h2>{translate('Arch', 'Clauses')}</h2>"
            html += f"<code>{', '.join(sorted(api_data.get('clauses', [])))}</code>"
            html += f"<h2>{translate('Arch', 'Key Functions')}</h2>"
            # Sort categories for a consistent display order
            for category_name in sorted(api_data.get('functions', {}).keys()):
                functions = api_data['functions'][category_name]
                html += f"<b>{category_name}:</b><ul>"
                # Sort functions within a category alphabetically
                for func_data in sorted(functions, key=lambda x: x['name']):
                    # Add a bottom margin to the list item for clear visual separation.
                    html += f"<li style='margin-bottom: 10px;'><code>{func_data['signature']}</code><br>{func_data['description']}"                    # Add the example snippet if it exists
                    if func_data.get('snippet'):
                        snippet_html = func_data['snippet'].replace("\n", "<br>")
                        # No <br> before the snippet. Added styling to make the snippet stand out.
                        html += f"<pre style='margin-top: 4px; padding: 5px; background-color: #f0f0f0; border: 1px solid #ccc;'><code>{snippet_html}</code></pre></li>"
                    else:
                        html += "</li>"
                html += "</ul>"
            return html
else:
    # In headless mode, we don't need the GUI classes.
    pass
