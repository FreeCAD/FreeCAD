# SPDX-License-Identifier: LGPL-2.1-or-later
#
# Copyright (c) 2025 The FreeCAD Project

import FreeCAD
import json # For ReportStatement serialization/deserialization

from draftutils import params

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

import ArchSql

# --- New: Icons for Status and Edit buttons ---
ICON_STATUS_OK = FreeCADGui.getIcon(":/icons/Task_Ok.svg")
ICON_STATUS_WARN = FreeCADGui.getIcon(":/icons/Task_Warning.svg")
ICON_STATUS_ERROR = FreeCADGui.getIcon(":/icons/Task_Error.svg")
ICON_STATUS_INCOMPLETE = FreeCADGui.getIcon(":/icons/Task_Incomplete.svg")
ICON_EDIT = FreeCADGui.getIcon(":/icons/Draft_Edit.svg")
ICON_ADD = FreeCADGui.getIcon(":/icons/list-add.svg")
ICON_REMOVE = FreeCADGui.getIcon(":/icons/list-remove.svg")
ICON_DUPLICATE = FreeCADGui.getIcon(":/icons/copy.svg")


class ReportStatement:
    """Encapsulates a single SQL query statement and its display options."""

    def __init__(self, description="", query_string="",
                 use_description_as_header=False, include_column_names=True,
                 add_empty_row_after=False, print_results_in_bold=False):
        self.description = description
        self.query_string = query_string
        self.use_description_as_header = use_description_as_header
        self.include_column_names = include_column_names
        self.add_empty_row_after = add_empty_row_after
        self.print_results_in_bold = print_results_in_bold

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
        }

    def loads(self, state):
        """Restores the internal state from serialized data."""
        self.description = state.get('description', '')
        self.query_string = state.get('query_string', '')
        self.use_description_as_header = state.get('use_description_as_header', False)
        self.include_column_names = state.get('include_column_names', True)
        self.add_empty_row_after = state.get('add_empty_row_after', False)
        self.print_results_in_bold = state.get('print_results_in_bold', False)

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

        count, error = ArchSql.run_query_for_count(self.query_string)

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
        obj.Proxy = self
        self.Type = 'ArchReport'
        self.spreadsheet = None
        self.docObserver = None
        self.spreadsheet_current_row = 1 # Internal state for multi-statement reports

    def onDocumentRestored(self, obj):
        # This is called AFTER properties are set.
        # Now we need to properly deserialize the Statements list.
        # The temporary list from __setstate__ needs to be converted.
        if hasattr(self, '_temp_statements_on_load') and self._temp_statements_on_load:
            statements = []
            for s_data in self._temp_statements_on_load:
                statement = ReportStatement()
                statement.loads(s_data)
                statements.append(statement)
            obj.Statements = statements
            del self._temp_statements_on_load # Clean up temporary attribute

        self.setProperties(obj) # This will ensure observer is re-attached

    def setProperties(self, obj):
        # Ensure the `Statements` property exists (list of ReportStatement objects)
        if not 'Statements' in obj.PropertiesList:
            obj.addProperty('App::PropertyPythonObject', 'Statements', 'Report', QT_TRANSLATE_NOOP('App::Property', 'The list of SQL statements to execute'), locked=True)
            obj.Statements = [] # Initialize with an empty list

        if not 'Target' in obj.PropertiesList:
            obj.addProperty('App::PropertyLink', 'Target', 'Report', QT_TRANSLATE_NOOP('App::Property', 'The spreadsheet for the results'))
        if not 'AutoUpdate' in obj.PropertiesList:
            obj.addProperty('App::PropertyBool', 'AutoUpdate', 'Report', QT_TRANSLATE_NOOP('App::Property', 'If True, update report when document recomputes'))
            obj.AutoUpdate = True

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

    def __getstate__(self):
        """Returns the internal state of the _ArchReport object for serialization."""
        state = {
            'Type': self.Type,
            # --- PHASE 1.5 FIX: Serialize Statements list ---
            'Statements': [s.dumps() for s in getattr(self.obj, 'Statements', [])],
            'spreadsheet_name': getattr(getattr(self, 'spreadsheet', None), 'Name', None),
        }
        return state

    def __setstate__(self, state):
        """Restores the internal state of the _ArchReport object from serialized data."""
        self.Type = state.get('Type', 'ArchReport')
        # Store statements data temporarily as obj is not available yet
        self._temp_statements_on_load = []
        for s_data in state.get('Statements', []):
            statement = ReportStatement()
            statement.loads(s_data)
            self._temp_statements_on_load.append(statement)

        self.spreadsheet = None
        self.docObserver = None

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

        # Add header for this statement if requested
        if use_description_as_header and description_text.strip():
            # Merging the header across columns (A to last data column)
            last_col_char = chr(ord('A') + len(headers) - 1) if headers else 'A'
            sp.set(f"A{current_row}", f"'{description_text}'")
            sp.mergeCells(f"A{current_row}:{last_col_char}{current_row}")
            sp.setStyle(f"A{current_row}", 'bold', 'add')
            current_row += 1 # Advance row for data or column names

        # Write column names if requested
        if include_column_names and headers:
            for col_idx, header_text in enumerate(headers):
                sp.set(f"{chr(ord('A') + col_idx)}{current_row}", f"'{header_text}'")
            sp.setStyle(f'A{current_row}:{chr(ord("A") + len(headers) - 1)}{current_row}', 'bold', 'add')
            current_row += 1 # Advance row for data

        # Write data rows
        for row_data in data_rows:
            for col_idx, cell_value in enumerate(row_data):
                cell_address = f"{chr(ord('A') + col_idx)}{current_row}"
                sp.set(cell_address, f"'{cell_value}'")
                if print_results_in_bold:
                    sp.setStyle(cell_address, 'bold', 'add')
            current_row += 1 # Advance row for next data row

        # Add empty row if specified
        if add_empty_row_after:
            current_row += 1 # Just increment row, leave it blank

        return current_row # Return the next available row

    def execute(self, obj):
        # --- PHASE 1.5 FIX: Handle Statements list ---
        if not hasattr(obj, 'Statements') or not obj.Statements:
            return

        sp = obj.Target # Ensure we use the explicitly linked Target
        if not sp: # ensure spreadsheet exists, this is an error condition
            FreeCAD.Console.PrintError(f"Report '{getattr(obj, 'Label', '')}': No target spreadsheet found.\n")
            return
        sp.clearAll() # Clear the spreadsheet for a new report

        self.spreadsheet_current_row = 1 # Reset row counter for a new report build

        for statement in obj.Statements:
            headers, results_data = ArchSql.run_query_for_objects(statement.query_string)

            if results_data is None:
                FreeCAD.Console.PrintError("Report '%s': Error executing query '%s'.\n" % (getattr(obj, 'Label', ''), statement.query_string))
                # Add an error message to the spreadsheet
                sp.set(f"A{self.spreadsheet_current_row}", f"❌ Error executing query: {statement.query_string}")
                sp.setStyle(f"A{self.spreadsheet_current_row}", 'color:red', 'add')
                self.spreadsheet_current_row += 1
                continue # Skip to next statement

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

    def __init__(self, report_obj):
        # Create two top-level widgets so FreeCAD will wrap each into a TaskBox.
        # Box 1 (overview) contains the statements table and management buttons.
        # Box 2 (editor) contains the query editor and options.
        self.obj = report_obj
        self.current_edited_statement_index = -1  # To track which statement is in editor

        # Overview widget (TaskBox 1)
        self.overview_widget = QtWidgets.QWidget()
        self.overview_widget.setWindowTitle(translate("Arch", "Report Statements"))
        self.statements_overview_widget = self.overview_widget  # preserve older name
        self.statements_overview_layout = QtWidgets.QVBoxLayout(self.statements_overview_widget)

        # Table for statements
        self.table_statements = QtWidgets.QTableWidget()
        self.table_statements.setColumnCount(3)  # Description, Status, Edit
        self.table_statements.setHorizontalHeaderLabels([
            translate("Arch", "Description"),
            translate("Arch", "Status"),
            translate("Arch", "Edit"),
        ])
        self.table_statements.horizontalHeader().setSectionResizeMode(0, QtWidgets.QHeaderView.Stretch)
        self.table_statements.horizontalHeader().setSectionResizeMode(1, QtWidgets.QHeaderView.ResizeToContents)
        self.table_statements.horizontalHeader().setSectionResizeMode(2, QtWidgets.QHeaderView.ResizeToContents)
        self.table_statements.setSelectionBehavior(QtWidgets.QAbstractItemView.SelectRows)
        self.table_statements.setSelectionMode(QtWidgets.QAbstractItemView.SingleSelection)
        self.table_statements.setDragDropMode(QtWidgets.QAbstractItemView.InternalMove)
        self.statements_overview_layout.addWidget(self.table_statements)

        # Statement Management Buttons
        self.statement_buttons_layout = QtWidgets.QHBoxLayout()
        self.btn_add_statement = QtWidgets.QPushButton(ICON_ADD, translate("Arch", "Add Statement"))
        self.btn_remove_statement = QtWidgets.QPushButton(ICON_REMOVE, translate("Arch", "Remove Selected"))
        self.btn_duplicate_statement = QtWidgets.QPushButton(ICON_DUPLICATE, translate("Arch", "Duplicate Selected"))

        self.statement_buttons_layout.addWidget(self.btn_add_statement)
        self.statement_buttons_layout.addWidget(self.btn_remove_statement)
        self.statement_buttons_layout.addWidget(self.btn_duplicate_statement)
        self.statements_overview_layout.addLayout(self.statement_buttons_layout)

        # Editor widget (TaskBox 2) -- starts collapsed until a statement is selected
        self.editor_widget = QtWidgets.QWidget()
        self.editor_widget.setWindowTitle(translate("Arch", "Edit Statement: (None Selected)"))
        # Keep compatibility name used elsewhere
        self.editor_box = self.editor_widget
        self.editor_layout = QtWidgets.QVBoxLayout(self.editor_box)

        # Description for edited statement
        self.description_label = QtWidgets.QLabel(translate("Arch", "Description:"))
        self.description_edit = QtWidgets.QLineEdit()
        self.editor_layout.addWidget(self.description_label)
        self.editor_layout.addWidget(self.description_edit)

        # Use Description as Section Header checkbox
        self.chk_use_description_as_header = QtWidgets.QCheckBox(translate("Arch", "Use Description as Section Header"))
        self.editor_layout.addWidget(self.chk_use_description_as_header)

        # SQL Query editor
        self.sql_label = QtWidgets.QLabel(translate("Arch", "SQL Query:"))
        self.sql_query_edit = QtWidgets.QPlainTextEdit()
        self.sql_query_status_label = QtWidgets.QLabel(translate("Arch", "Ready"))
        self.sql_query_status_label.setTextInteractionFlags(QtCore.Qt.TextSelectableByMouse)

        self.editor_layout.addWidget(self.sql_label)
        self.editor_layout.addWidget(self.sql_query_edit)
        self.editor_layout.addWidget(self.sql_query_status_label)

        # Display Options for edited statement
        self.chk_include_column_names = QtWidgets.QCheckBox(translate("Arch", "Include Column Names"))
        self.chk_add_empty_row_after = QtWidgets.QCheckBox(translate("Arch", "Add Empty Row After"))
        self.chk_print_results_in_bold = QtWidgets.QCheckBox(translate("Arch", "Print Results in Bold"))

        self.editor_layout.addWidget(self.chk_include_column_names)
        self.editor_layout.addWidget(self.chk_add_empty_row_after)
        self.editor_layout.addWidget(self.chk_print_results_in_bold)

        # Expose form as a list of the two top-level widgets so FreeCAD creates
        # two built-in TaskBox sections. The overview goes first, editor second.
        self.form = [self.overview_widget, self.editor_widget]

        # --- Connections ---
        self.btn_add_statement.clicked.connect(self._add_statement)
        self.btn_remove_statement.clicked.connect(self._remove_selected_statement)
        self.btn_duplicate_statement.clicked.connect(self._duplicate_selected_statement)
        self.table_statements.itemSelectionChanged.connect(self._on_table_selection_changed)
        self.description_edit.textChanged.connect(self._on_editor_description_changed)
        self.sql_query_edit.textChanged.connect(self._on_editor_sql_changed)
        self.chk_use_description_as_header.stateChanged.connect(self._on_editor_checkbox_changed)
        self.chk_include_column_names.stateChanged.connect(self._on_editor_checkbox_changed)
        self.chk_add_empty_row_after.stateChanged.connect(self._on_editor_checkbox_changed)
        self.chk_print_results_in_bold.stateChanged.connect(self._on_editor_checkbox_changed)

        # Validation Timer for live SQL preview
        # Timer doesn't need a specific QWidget parent here; use no parent.
        self.validation_timer = QtCore.QTimer()
        self.validation_timer.setSingleShot(True)
        self.validation_timer.timeout.connect(self._run_live_validation_for_editor)

        # Initial UI setup
        self._populate_table_from_statements()
        self._update_editor_visibility(False)  # Start with editor collapsed/hidden
        # Do not auto-select the first statement; leave editor collapsed until user selects



    # --- Statement Management (Buttons and Table Interaction) ---
    def _populate_table_from_statements(self):
        self.table_statements.setRowCount(0) # Clear existing rows
        statements = self.obj.Statements
        for row_idx, statement in enumerate(statements):
            self.table_statements.insertRow(row_idx)
            self.table_statements.setItem(row_idx, 0, QtWidgets.QTableWidgetItem(statement.description))

            # Status Item (Icon + Tooltip)
            status_icon, status_tooltip = self._get_status_icon_and_tooltip(statement)
            status_item = QtWidgets.QTableWidgetItem()
            status_item.setIcon(status_icon)
            status_item.setToolTip(status_tooltip)
            status_item.setFlags(status_item.flags() & ~QtCore.Qt.ItemIsEditable) # Make read-only
            self.table_statements.setItem(row_idx, 1, status_item)

            # Edit Button Item
            edit_button = QtWidgets.QToolButton()
            edit_button.setIcon(ICON_EDIT)
            edit_button.setToolTip(translate("Arch", "Edit statement details"))
            edit_button.clicked.connect(lambda _, r=row_idx: self._select_statement_in_table(r)) # Use lambda to pass row_idx
            self.table_statements.setCellWidget(row_idx, 2, edit_button)

            # Make description editable directly in table
            self.table_statements.item(row_idx, 0).setFlags(self.table_statements.item(row_idx, 0).flags() | QtCore.Qt.ItemIsEditable)

            # Recalculate status for each statement (important on load)
            statement.validate_and_update_status()

    def _add_statement(self):
        new_statement = ReportStatement(description=translate("Arch", f"New Statement {len(self.obj.Statements) + 1}"))
        self.obj.Statements.append(new_statement)
        self._populate_table_from_statements() # Refresh the table
        new_statement.validate_and_update_status() # Validate immediately
        self._select_statement_in_table(len(self.obj.Statements) - 1) # Select and open new statement

    def _remove_selected_statement(self):
        selected_rows = self.table_statements.selectionModel().selectedRows()
        if not selected_rows:
            return

        row_to_remove = selected_rows[0].row()
        description_to_remove = self.table_statements.item(row_to_remove, 0).text()

        if QtWidgets.QMessageBox.question(None, translate("Arch", "Remove Statement"),
                                          translate("Arch", f"Are you sure you want to remove statement '{description_to_remove}'?"),
                                          QtWidgets.QMessageBox.Yes | QtWidgets.QMessageBox.No) == QtWidgets.QMessageBox.Yes:
            self.obj.Statements.pop(row_to_remove)
            self._populate_table_from_statements()
            self._select_first_statement_if_available()

    def _duplicate_selected_statement(self):
        selected_rows = self.table_statements.selectionModel().selectedRows()
        if not selected_rows:
            return

        row_to_duplicate = selected_rows[0].row()
        original_statement = self.obj.Statements[row_to_duplicate]

        duplicated_statement = ReportStatement()
        duplicated_statement.loads(original_statement.dumps()) # Use serialization to deep copy
        duplicated_statement.description = translate("Arch", f"Copy of {original_statement.description}")

        self.obj.Statements.insert(row_to_duplicate + 1, duplicated_statement)
        self._populate_table_from_statements()
        duplicated_statement.validate_and_update_status()
        self._select_statement_in_table(row_to_duplicate + 1)

    def _select_first_statement_if_available(self):
        if self.obj.Statements:
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
                self._load_statement_to_editor(self.obj.Statements[new_index])
                self._update_editor_title(self.obj.Statements[new_index].description)
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
        if self.current_edited_statement_index != -1 and self.current_edited_statement_index < len(self.obj.Statements):
            statement = self.obj.Statements[self.current_edited_statement_index]
            statement.description = self.description_edit.text()
            statement.query_string = self.sql_query_edit.toPlainText()
            statement.use_description_as_header = self.chk_use_description_as_header.isChecked()
            statement.include_column_names = self.chk_include_column_names.isChecked()
            statement.add_empty_row_after = self.chk_add_empty_row_after.isChecked()
            statement.print_results_in_bold = self.chk_print_results_in_bold.isChecked()
            statement.validate_and_update_status() # Update status in the statement object
            self._update_table_row_status(self.current_edited_statement_index, statement) # Refresh table status

    def _on_editor_description_changed(self):
        # Update table description live as user types in editor
        if self.current_edited_statement_index != -1:
            item = self.table_statements.item(self.current_edited_statement_index, 0)
            if item:
                item.setText(self.description_edit.text())
            self._update_editor_title(self.description_edit.text())
        # Re-trigger validation for other fields potentially (e.g. if query uses description)
        self.validation_timer.start(500) # (Re)start timer for live validation

    def _on_editor_sql_changed(self):
        self.sql_query_status_label.setText(translate("Arch", "<i>Typing...</i>"))
        self.sql_query_status_label.setStyleSheet("color: gray;")
        self.validation_timer.start(500) # (Re)start timer for live validation

    def _on_editor_checkbox_changed(self):
        # Checkboxes are saved when editor state is saved, but ensure live validation for status icon
        self.validation_timer.start(500) # (Re)start timer for live validation


    def _run_live_validation_for_editor(self):
        # Validate the query currently in the editor's text area
        temp_statement = ReportStatement(query_string=self.sql_query_edit.toPlainText())
        temp_statement.validate_and_update_status()
        self._update_editor_status_display(temp_statement) # Update the status label in the editor UI

        # Also update table if this is the currently edited statement
        if self.current_edited_statement_index != -1:
            statement_obj = self.obj.Statements[self.current_edited_statement_index]
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
        status_item = self.table_statements.item(row_idx, 1)
        if status_item:
            status_icon, status_tooltip = self._get_status_icon_and_tooltip(statement)
            status_item.setIcon(status_icon)
            status_item.setToolTip(status_tooltip)

        # Update description in table in case it was changed via editor
        desc_item = self.table_statements.item(row_idx, 0)
        if desc_item:
            desc_item.setText(statement.description)


    def _get_status_icon_and_tooltip(self, statement: ReportStatement):
        # Helper to get appropriate icon and tooltip for table status column
        status = statement._validation_status
        message = statement._validation_message
        count = statement._validation_count

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


    # --- Dialog Acceptance / Rejection ---
    def accept(self):
        """Saves changes from UI to Report object and triggers recompute."""
        # Ensure the currently edited statement's state is saved before final accept
        self._save_current_editor_state_to_statement()
        # Trigger a recompute to persist changes and refresh report
        try:
            FreeCAD.ActiveDocument.recompute()
        except Exception:
            # In headless/static checks FreeCAD.ActiveDocument may be unavailable
            pass
        # Close the task panel via FreeCADGui if available
        try:
            FreeCADGui.Control.closeDialog()
        except Exception:
            pass

    def reject(self):
        """Closes dialog without saving changes to the Report object."""
        # Revert changes by not writing to self.obj.Statements
        # Close the task panel without saving
        try:
            FreeCADGui.Control.closeDialog()
        except Exception:
            pass
