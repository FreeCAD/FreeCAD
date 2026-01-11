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

    # Create an alias for the Slot decorator for use within the GUI-only classes.
    Slot = QtCore.Slot
else:

    def translate(ctxt, txt):
        return txt

    def QT_TRANSLATE_NOOP(ctxt, txt):
        return txt

    # In headless mode, create a dummy decorator named 'Slot'. This allows the
    # Python interpreter to parse the @Slot syntax in GUI-only classes without
    # raising a NameError because QtCore is not imported.
    def Slot(*args, **kwargs):
        def decorator(func):
            return func

        return decorator


import ArchSql
from ArchSql import ReportStatement

if FreeCAD.GuiUp:
    ICON_STATUS_OK = FreeCADGui.getIcon(":/icons/edit_OK.svg")
    ICON_STATUS_WARN = FreeCADGui.getIcon(":/icons/Warning.svg")
    ICON_STATUS_ERROR = FreeCADGui.getIcon(":/icons/delete.svg")
    ICON_STATUS_INCOMPLETE = FreeCADGui.getIcon(":/icons/button_invalid.svg")
    ICON_EDIT = FreeCADGui.getIcon(":/icons/edit-edit.svg")
    ICON_ADD = FreeCADGui.getIcon(":/icons/list-add.svg")
    ICON_REMOVE = FreeCADGui.getIcon(":/icons/list-remove.svg")
    ICON_DUPLICATE = FreeCADGui.getIcon(":/icons/edit-copy.svg")


def _get_preset_paths(preset_type):
    """
    Gets the file paths for bundled (system) and user preset directories.

    Parameters
    ----------
    preset_type : str
        The type of preset, either 'query' or 'report'.

    Returns
    -------
    tuple
        A tuple containing (system_preset_dir, user_preset_dir).
    """
    if preset_type == "query":
        subdir = "QueryPresets"
    elif preset_type == "report":
        subdir = "ReportPresets"
    else:
        return None, None

    # Path to the bundled presets installed with FreeCAD
    system_path = os.path.join(
        FreeCAD.getResourceDir(), "Mod", "BIM", "Presets", "ArchReport", subdir
    )
    # Path to the user's custom presets in their AppData directory
    user_path = os.path.join(FreeCAD.getUserAppDataDir(), "BIM", "Presets", "ArchReport", subdir)

    return system_path, user_path


def _get_presets(preset_type):
    """
    Loads all bundled and user presets from the filesystem.

    This function scans the mirrored system and user directories, loading each
    valid .json file. It is resilient to errors in user-created files.

    Parameters
    ----------
    preset_type : str
        The type of preset to load, either 'query' or 'report'.

    Returns
    -------
    dict
        A dictionary mapping the preset's filename (its stable ID) to a
        dictionary containing its display name, data, and origin.
        Example:
        {
          "room-schedule.json": {"name": "Room Schedule", "data": {...}, "is_user": False},
          "c2f5b1a0...json": {"name": "My Custom Report", "data": {...}, "is_user": True}
        }
    """
    system_dir, user_dir = _get_preset_paths(preset_type)
    presets = {}

    def scan_directory(directory, is_user_preset):
        if not os.path.isdir(directory):
            return

        for filename in os.listdir(directory):
            if not filename.endswith(".json"):
                continue

            file_path = os.path.join(directory, filename)
            try:
                with open(file_path, "r", encoding="utf8") as f:
                    data = json.load(f)

                if "name" not in data:
                    # Graceful handling: use filename as fallback, log a warning
                    display_name = os.path.splitext(filename)[0]
                    FreeCAD.Console.PrintWarning(
                        f"BIM Report: Preset file '{file_path}' is missing a 'name' key. Using filename as fallback.\n"
                    )
                else:
                    display_name = data["name"]

                # Apply translation only to bundled system presets
                if not is_user_preset:
                    display_name = translate("Arch", display_name)

                presets[filename] = {"name": display_name, "data": data, "is_user": is_user_preset}

            except json.JSONDecodeError:
                # Graceful handling: skip malformed file, log a detailed error
                FreeCAD.Console.PrintError(
                    f"BIM Report: Could not parse preset file at '{file_path}'. It may contain a syntax error.\n"
                )
            except Exception as e:
                FreeCAD.Console.PrintError(
                    f"BIM Report: An unexpected error occurred while loading preset '{file_path}': {e}\n"
                )

    # Scan system presets first, then user presets. User presets will not
    # overwrite system presets as their filenames (UUIDs) are unique.
    scan_directory(system_dir, is_user_preset=False)
    scan_directory(user_dir, is_user_preset=True)

    return presets


def _save_preset(preset_type, name, data):
    """
    Saves a preset to a new, individual .json file with a UUID-based filename.

    This function handles name collision checks and ensures the user's preset
    is saved in their personal AppData directory.

    Parameters
    ----------
    preset_type : str
        The type of preset, either 'query' or 'report'.
    name : str
        The desired human-readable display name for the preset.
    data : dict
        The dictionary of preset data to be saved as JSON.
    """
    import uuid

    _, user_path = _get_preset_paths(preset_type)
    if not user_path:
        return

    os.makedirs(user_path, exist_ok=True)

    # --- Name Collision Handling ---
    existing_presets = _get_presets(preset_type)
    existing_display_names = {p["name"] for p in existing_presets.values() if p["is_user"]}

    final_name = name
    counter = 1
    while final_name in existing_display_names:
        final_name = f"{name} ({counter:03d})"
        counter += 1

    # The display name is stored inside the JSON content
    data_to_save = data.copy()
    data_to_save["name"] = final_name

    # The filename is a stable, unique identifier
    filename = f"{uuid.uuid4()}.json"
    file_path = os.path.join(user_path, filename)

    try:
        with open(file_path, "w", encoding="utf8") as f:
            json.dump(data_to_save, f, indent=2)
        FreeCAD.Console.PrintMessage(
            f"BIM Report: Preset '{final_name}' saved successfully to '{file_path}'.\n"
        )
    except Exception as e:
        FreeCAD.Console.PrintError(f"BIM Report: Could not save preset to '{file_path}': {e}\n")


def _rename_preset(preset_type, filename, new_name):
    """Renames a user preset by updating the 'name' key in its JSON file."""
    _, user_path = _get_preset_paths(preset_type)
    file_path = os.path.join(user_path, filename)

    if not os.path.exists(file_path):
        FreeCAD.Console.PrintError(
            f"BIM Report: Cannot rename preset. File not found: {file_path}\n"
        )
        return

    try:
        with open(file_path, "r", encoding="utf8") as f:
            data = json.load(f)

        data["name"] = new_name

        with open(file_path, "w", encoding="utf8") as f:
            json.dump(data, f, indent=2)
    except Exception as e:
        FreeCAD.Console.PrintError(f"BIM Report: Failed to rename preset file '{file_path}': {e}\n")


def _delete_preset(preset_type, filename):
    """Deletes a user preset file from disk."""
    _, user_path = _get_preset_paths(preset_type)
    file_path = os.path.join(user_path, filename)

    if not os.path.exists(file_path):
        FreeCAD.Console.PrintError(
            f"BIM Report: Cannot delete preset. File not found: {file_path}\n"
        )
        return

    try:
        os.remove(file_path)
    except Exception as e:
        FreeCAD.Console.PrintError(f"BIM Report: Failed to delete preset file '{file_path}': {e}\n")


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
            self.setMouseTracking(True)  # Required to receive mouseMoveEvents
            self.api_docs = {}
            self.clauses = set()
            self.functions = {}

        def set_api_documentation(self, api_docs: dict):
            """Receives the API documentation from the panel and caches it."""
            self.api_docs = api_docs
            self.clauses = set(api_docs.get("clauses", []))
            # Create a flat lookup dictionary for fast access
            for category, func_list in api_docs.get("functions", {}).items():
                for func_data in func_list:
                    self.functions[func_data["name"]] = {
                        "category": category,
                        "signature": func_data["signature"],
                        "description": func_data["description"],
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
                return (
                    f"<p style='white-space:nowrap'><code><b>{func_data['signature']}</b></code><br>"
                    f"<i>{func_data['category']}</i><br>"
                    f"{func_data['description']}</p>"
                )

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
                if event.key() in (
                    QtCore.Qt.Key_Enter,
                    QtCore.Qt.Key_Return,
                    QtCore.Qt.Key_Escape,
                    QtCore.Qt.Key_Tab,
                    QtCore.Qt.Key_Backtab,
                ):
                    event.ignore()
                    return

            # Let the parent handle the key press to ensure normal typing works.
            super().keyPressEvent(event)

            # --- Autocompletion Trigger Logic ---

            # A Ctrl+Space shortcut can also be used to trigger completion.
            is_shortcut = (
                event.modifiers() & QtCore.Qt.ControlModifier and event.key() == QtCore.Qt.Key_Space
            )

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
                    self._completer.completionModel().index(0, 0)
                )

            # --- Sizing and Positioning Logic (The critical fix) ---
            cursor_rect = self.cursorRect()

            # Calculate the required width based on the content of the popup.
            popup_width = (
                self._completer.popup().sizeHintForColumn(0)
                + self._completer.popup().verticalScrollBar().sizeHint().width()
            )
            cursor_rect.setWidth(popup_width)

            # Show the completer.
            self._completer.complete(cursor_rect)


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
        self.Type = "ArchReport"
        self.spreadsheet = None
        self.docObserver = None
        self.spreadsheet_current_row = 1  # Internal state for multi-statement reports
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
        self.obj = obj
        self.hydrate_live_statements(obj)
        self.setProperties(obj)  # This will ensure observer is re-attached

    def hydrate_live_statements(self, obj):
        """(Re)builds the live list of objects from the stored list of dicts."""
        self.live_statements = []
        if hasattr(obj, "Statements") and obj.Statements:
            for s_data in obj.Statements:
                statement = ReportStatement()
                statement.loads(s_data)  # Use existing loads method
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
        if not "Statements" in obj.PropertiesList:
            obj.addProperty(
                "App::PropertyPythonObject",
                "Statements",
                "Report",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "The list of SQL statements to execute (managed by the Task Panel)",
                ),
                locked=True,
            )
            obj.Statements = []  # Initialize with an empty list

        if not "Target" in obj.PropertiesList:
            obj.addProperty(
                "App::PropertyLink",
                "Target",
                "Report",
                QT_TRANSLATE_NOOP("App::Property", "The spreadsheet for the results"),
            )
        if not "AutoUpdate" in obj.PropertiesList:
            obj.addProperty(
                "App::PropertyBool",
                "AutoUpdate",
                "Report",
                QT_TRANSLATE_NOOP(
                    "App::Property", "If True, update report when document recomputes"
                ),
            )
            obj.AutoUpdate = True

        self.onChanged(obj, "AutoUpdate")
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
        if not hasattr(sp, "ReportName"):
            sp.addProperty(
                "App::PropertyString",
                "ReportName",
                "Report",
                QT_TRANSLATE_NOOP(
                    "App::Property", "The name of the BIM Report that uses this spreadsheet"
                ),
            )
        sp.ReportName = obj.Name
        obj.Target = sp

    def getSpreadSheet(self, obj, force=False):
        """Find or (optionally) create the spreadsheet associated with a report.

        The association is persisted via the sheet's ``ReportName`` string.

        Parameters
        - obj: the report object
        - force: if True, create a new spreadsheet when none is found
        """
        sp = getattr(self, "spreadsheet", None)
        if sp and getattr(sp, "ReportName", None) == obj.Name:
            return sp

        for o in FreeCAD.ActiveDocument.Objects:
            if o.TypeId == "Spreadsheet::Sheet" and getattr(o, "ReportName", None) == obj.Name:
                self.spreadsheet = o
                return self.spreadsheet

        if force:
            sheet = FreeCAD.ActiveDocument.addObject("Spreadsheet::Sheet", "ReportResult")
            self.setReportPropertySpreadsheet(sheet, obj)
            self.spreadsheet = sheet
            return self.spreadsheet
        else:
            return None

    def onChanged(self, obj, prop):
        if prop == "AutoUpdate":
            if obj.AutoUpdate:
                if getattr(self, "docObserver", None) is None:
                    self.docObserver = _ArchReportDocObserver(FreeCAD.ActiveDocument, obj)
                    FreeCAD.addDocumentObserver(self.docObserver)
            else:
                if getattr(self, "docObserver", None) is not None:
                    FreeCAD.removeDocumentObserver(self.docObserver)
                    self.docObserver = None

            if prop == "Statements":
                # If the persistent data is changed externally (e.g., by a script),
                # re-hydrate the live list to ensure consistency.
                self.hydrate_live_statements(obj)

    def __getstate__(self):
        """Returns minimal internal state of the proxy for serialization."""
        # The main 'Statements' data is persisted on the obj property, not here.
        return {
            "Type": self.Type,
        }

    def __setstate__(self, state):
        """Restores minimal internal state of the proxy from serialized data."""
        self.Type = state.get("Type", "ArchReport")
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

    def setSpreadsheetData(
        self,
        obj,
        headers,
        data_rows,
        start_row,
        use_description_as_header=False,
        description_text="",
        include_column_names=True,
        add_empty_row_after=False,
        print_results_in_bold=False,
        force=False,
    ):
        """Write headers and rows into the report's spreadsheet, starting from a specific row."""
        sp = obj.Target  # Always use obj.Target directly as it's the explicit link
        if not sp:  # ensure spreadsheet exists, this is an error condition
            FreeCAD.Console.PrintError(
                f"Report '{getattr(obj, 'Label', '')}': No target spreadsheet found.\n"
            )
            return start_row  # Return current row unchanged

        # Determine the effective starting row for this block of data
        current_row = start_row

        # --- "Analyst-First" Header Generation ---
        # Pre-scan the first data row to find the common unit for each column.
        unit_map = {}  # e.g., {1: 'mm', 2: 'mm'}

        if data_rows:
            for i, cell_value in enumerate(data_rows[0]):
                if isinstance(cell_value, FreeCAD.Units.Quantity):
                    # TODO: Replace this with a direct API call when available. The C++ Base::Unit
                    # class has a `getString()` method that returns the simple unit symbol (e.g.,
                    # "mm^2"), but it is not exposed to the Python API. The most reliable workaround
                    # is to stringify the entire Quantity (e.g., "1500.0 mm") and parse the unit
                    # from that string.
                    quantity_str = str(cell_value)
                    parts = quantity_str.split(" ", 1)
                    if len(parts) > 1:
                        unit_map[i] = parts[1]

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
            last_col_char = chr(ord("A") + len(final_headers) - 1) if final_headers else "A"
            sp.set(f"A{current_row}", f"'{description_text}")
            sp.mergeCells(f"A{current_row}:{last_col_char}{current_row}")
            sp.setStyle(f"A{current_row}", "bold", "add")
            current_row += 1  # Advance row for data or column names

        # Write column names if requested
        if include_column_names and final_headers:
            for col_idx, header_text in enumerate(final_headers):
                sp.set(f"{chr(ord('A') + col_idx)}{current_row}", f"'{header_text}")
            sp.setStyle(
                f'A{current_row}:{chr(ord("A") + len(final_headers) - 1)}{current_row}',
                "bold",
                "add",
            )
            current_row += 1  # Advance row for data

        # Write data rows
        for row_data in data_rows:
            for col_idx, cell_value in enumerate(row_data):
                cell_address = f"{chr(ord('A') + col_idx)}{current_row}"
                self._write_cell(sp, cell_address, cell_value)
                if print_results_in_bold:
                    sp.setStyle(cell_address, "bold", "add")
            current_row += 1  # Advance row for next data row

        # Add empty row if specified
        if add_empty_row_after:
            current_row += 1  # Just increment row, leave it blank

        return current_row  # Return the next available row

    def execute(self, obj):
        """Executes all statements and writes the results to the target spreadsheet."""
        if not self.live_statements:
            return

        sp = self.getSpreadSheet(obj, force=True)
        if not sp:
            FreeCAD.Console.PrintError(
                f"Report '{getattr(obj, 'Label', '')}': No target spreadsheet found.\n"
            )
            return
        sp.clearAll()

        # Reset the row counter for a new report build.
        self.spreadsheet_current_row = 1

        # The execute_pipeline function is a generator that yields the results
        # of each standalone statement or the final result of a pipeline chain.
        for statement, headers, results_data in ArchSql.execute_pipeline(self.live_statements):
            # For each yielded result block, write it to the spreadsheet.
            # The setSpreadsheetData helper already handles all the formatting.
            self.spreadsheet_current_row = self.setSpreadsheetData(
                obj,
                headers,
                results_data,
                start_row=self.spreadsheet_current_row,
                use_description_as_header=statement.use_description_as_header,
                description_text=statement.description,
                include_column_names=statement.include_column_names,
                add_empty_row_after=statement.add_empty_row_after,
                print_results_in_bold=statement.print_results_in_bold,
            )

        sp.recompute()
        sp.purgeTouched()

    def __repr__(self):
        """Provides an unambiguous representation for developers."""
        return f"<BIM Report Label='{self.obj.Label}' Statements={len(self.live_statements)}>"

    def __str__(self):
        """
        Provides a detailed, human-readable string representation of the report,
        including the full SQL query for each statement.
        """
        num_statements = len(self.live_statements)
        header = f"BIM Report: '{self.obj.Label}' ({num_statements} statements)"

        lines = [header]
        if not self.live_statements:
            return header

        for i, stmt in enumerate(self.live_statements, 1):
            lines.append("")  # Add a blank line for spacing

            # Build the flag string for the statement header
            flags = []
            if stmt.is_pipelined:
                flags.append("Pipelined")
            if stmt.use_description_as_header:
                flags.append("Header")
            flag_str = f" ({', '.join(flags)})" if flags else ""

            # Add the statement header
            lines.append(f"=== Statement [{i}]: {stmt.description}{flag_str} ===")

            # Add the formatted SQL query
            if stmt.query_string.strip():
                query_lines = stmt.query_string.strip().split("\n")
                for line in query_lines:
                    lines.append(f"    {line}")
            else:
                lines.append("    (No query defined)")

        return "\n".join(lines)


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
        self.vobj = vobj  # Ensure self.vobj is set for consistent access

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
        "ExpressionEngine",
        "Label2",
        "Proxy",
        "ShapeColor",
        "Visibility",
        "LineColor",
        "LineWidth",
        "PointColor",
        "PointSize",
    }

    def __init__(self, report_obj):
        # Create two top-level widgets so FreeCAD will wrap each into a TaskBox.
        # Box 1 (overview) contains the statements table and management buttons.
        # Box 2 (editor) contains the query editor and options.
        self.obj = report_obj
        self.current_edited_statement_index = -1  # To track which statement is in editor
        self.is_dirty = False  # To track uncommitted changes

        # Overview widget (TaskBox 1)
        self.overview_widget = QtWidgets.QWidget()
        self.overview_widget.setWindowTitle(translate("Arch", "Report Statements"))
        self.statements_overview_widget = self.overview_widget  # preserve older name
        self.statements_overview_layout = QtWidgets.QVBoxLayout(self.statements_overview_widget)

        # Table for statements: Description | Header | Cols | Status
        self.table_statements = QtWidgets.QTableWidget()
        self.table_statements.setColumnCount(5)  # Description, Pipe, Header, Cols, Status
        self.table_statements.setHorizontalHeaderLabels(
            [
                translate("Arch", "Description"),
                translate("Arch", "Pipe"),
                translate("Arch", "Header"),
                translate("Arch", "Cols"),
                translate("Arch", "Status"),
            ]
        )

        # Add informative tooltips to the headers
        self.table_statements.horizontalHeaderItem(2).setToolTip(
            translate("Arch", "A user-defined description for this statement.")
        )
        self.table_statements.horizontalHeaderItem(1).setToolTip(
            translate(
                "Arch",
                "If checked, this statement will use the results of the previous statement as its data source.",
            )
        )
        self.table_statements.horizontalHeaderItem(2).setToolTip(
            translate(
                "Arch",
                "If checked, the Description will be used as a section header in the report.",
            )
        )
        self.table_statements.horizontalHeaderItem(3).setToolTip(
            translate(
                "Arch",
                "If checked, the column names (e.g., 'Label', 'Area') will be included in the report.",
            )
        )
        self.table_statements.horizontalHeaderItem(4).setToolTip(
            translate("Arch", "Indicates the status of the SQL query.")
        )

        # Description stretches, others sized to contents
        self.table_statements.horizontalHeader().setSectionResizeMode(
            0, QtWidgets.QHeaderView.Stretch
        )  # Description
        self.table_statements.horizontalHeader().setSectionResizeMode(
            1, QtWidgets.QHeaderView.ResizeToContents
        )  # Pipe
        self.table_statements.horizontalHeader().setSectionResizeMode(
            2, QtWidgets.QHeaderView.ResizeToContents
        )
        self.table_statements.horizontalHeader().setSectionResizeMode(
            3, QtWidgets.QHeaderView.ResizeToContents
        )
        self.table_statements.horizontalHeader().setSectionResizeMode(
            4, QtWidgets.QHeaderView.ResizeToContents
        )
        self.table_statements.setSelectionBehavior(QtWidgets.QAbstractItemView.SelectRows)
        self.table_statements.setSelectionMode(QtWidgets.QAbstractItemView.SingleSelection)
        self.table_statements.setDragDropMode(QtWidgets.QAbstractItemView.InternalMove)
        self.table_statements.setDragDropOverwriteMode(False)
        # Allow in-place editing of the description with F2, but disable the
        # default double-click editing so we can repurpose it.
        self.table_statements.setEditTriggers(QtWidgets.QAbstractItemView.EditKeyPressed)
        self.table_statements.verticalHeader().sectionMoved.connect(self._on_row_moved)
        self.statements_overview_layout.addWidget(self.table_statements)

        # Template controls for full reports
        self.template_layout = QtWidgets.QHBoxLayout()
        self.template_dropdown = NoScrollHijackComboBox()
        self.template_dropdown.setToolTip(
            translate("Arch", "Load a full report template, replacing all current statements.")
        )
        # Enable per-item tooltips in the dropdown view
        self.template_dropdown.view().setToolTip("")
        self.btn_manage_templates = QtWidgets.QPushButton(translate("Arch", "Manage..."))
        self.btn_manage_templates.setToolTip(
            translate("Arch", "Rename, delete, or edit saved report templates.")
        )
        self.btn_save_template = QtWidgets.QPushButton(translate("Arch", "Save as Template..."))
        self.btn_save_template.setToolTip(
            translate("Arch", "Save the current set of statements as a new report template.")
        )
        self.btn_save_template.setIcon(FreeCADGui.getIcon(":/icons/document-save.svg"))
        self.template_layout.addWidget(self.template_dropdown)
        self.template_layout.addWidget(self.btn_manage_templates)
        self.template_layout.addWidget(self.btn_save_template)
        template_label = QtWidgets.QLabel(translate("Arch", "Report Templates:"))
        self.statements_overview_layout.addWidget(template_label)
        self.statements_overview_layout.addLayout(self.template_layout)

        # Statement Management Buttons
        self.statement_buttons_layout = QtWidgets.QHBoxLayout()
        self.btn_add_statement = QtWidgets.QPushButton(ICON_ADD, translate("Arch", "Add Statement"))
        self.btn_add_statement.setToolTip(
            translate("Arch", "Add a new blank statement to the report.")
        )
        self.btn_remove_statement = QtWidgets.QPushButton(
            ICON_REMOVE, translate("Arch", "Remove Selected")
        )
        self.btn_remove_statement.setToolTip(
            translate("Arch", "Remove the selected statement from the report.")
        )
        self.btn_duplicate_statement = QtWidgets.QPushButton(
            ICON_DUPLICATE, translate("Arch", "Duplicate Selected")
        )
        self.btn_duplicate_statement.setToolTip(
            translate("Arch", "Create a copy of the selected statement.")
        )
        self.btn_edit_selected = QtWidgets.QPushButton(
            ICON_EDIT, translate("Arch", "Edit Selected")
        )
        self.btn_edit_selected.setToolTip(
            translate("Arch", "Load the selected statement into the editor below.")
        )

        self.statement_buttons_layout.addWidget(self.btn_add_statement)
        self.statement_buttons_layout.addWidget(self.btn_remove_statement)
        self.statement_buttons_layout.addWidget(self.btn_duplicate_statement)
        self.statement_buttons_layout.addStretch()
        self.statement_buttons_layout.addWidget(self.btn_edit_selected)
        self.statements_overview_layout.addLayout(self.statement_buttons_layout)

        # Editor widget (TaskBox 2) -- starts collapsed until a statement is selected
        self.editor_widget = QtWidgets.QWidget()
        self.editor_widget.setWindowTitle(translate("Arch", "Statement Editor"))
        # Keep compatibility name used elsewhere
        self.editor_box = self.editor_widget
        self.editor_layout = QtWidgets.QVBoxLayout(self.editor_box)

        # --- Form Layout for Aligned Inputs ---
        self.form_layout = QtWidgets.QFormLayout()
        self.form_layout.setContentsMargins(0, 0, 0, 0)  # Use the main layout's margins

        # Description Row
        self.description_edit = QtWidgets.QLineEdit()
        self.form_layout.addRow(translate("Arch", "Description:"), self.description_edit)

        # Preset Controls Row (widgets are placed in a QHBoxLayout for the second column)
        self.preset_controls_layout = QtWidgets.QHBoxLayout()
        self.query_preset_dropdown = NoScrollHijackComboBox()
        self.query_preset_dropdown.setToolTip(
            translate("Arch", "Load a saved query preset into the editor.")
        )
        # Enable per-item tooltips in the dropdown view
        self.query_preset_dropdown.view().setToolTip("")
        self.btn_manage_queries = QtWidgets.QPushButton(translate("Arch", "Manage..."))
        self.btn_manage_queries.setToolTip(
            translate("Arch", "Rename, delete, or edit your saved query presets.")
        )
        self.btn_save_query_preset = QtWidgets.QPushButton(translate("Arch", "Save..."))
        self.btn_save_query_preset.setToolTip(
            translate("Arch", "Save the current query as a new preset.")
        )
        self.preset_controls_layout.addWidget(self.query_preset_dropdown)
        self.preset_controls_layout.addWidget(self.btn_manage_queries)
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

        # --- Debugging Actions (Show Preview, Help) ---
        self.debugging_actions_layout = QtWidgets.QHBoxLayout()

        self.btn_toggle_preview = QtWidgets.QPushButton(translate("Arch", "Show Preview"))
        self.btn_toggle_preview.setIcon(FreeCADGui.getIcon(":/icons/Std_ToggleVisibility.svg"))
        self.btn_toggle_preview.setToolTip(
            translate("Arch", "Show a preview pane to test the current query in isolation.")
        )
        self.btn_toggle_preview.setCheckable(True)  # Make it a toggle button

        self.btn_show_cheatsheet = QtWidgets.QPushButton(translate("Arch", "SQL Cheatsheet"))
        self.btn_show_cheatsheet.setIcon(FreeCADGui.getIcon(":/icons/help-browser.svg"))
        self.btn_show_cheatsheet.setToolTip(
            translate("Arch", "Show a cheatsheet of the supported SQL syntax.")
        )

        self.editor_layout.addLayout(self.debugging_actions_layout)
        self.debugging_actions_layout.addStretch()  # Add stretch first for right-alignment
        self.debugging_actions_layout.addWidget(self.btn_show_cheatsheet)
        self.debugging_actions_layout.addWidget(self.btn_toggle_preview)

        # --- Self-Contained Preview Pane ---
        self.preview_pane = QtWidgets.QWidget()
        preview_pane_layout = QtWidgets.QVBoxLayout(self.preview_pane)
        preview_pane_layout.setContentsMargins(0, 5, 0, 0)  # Add a small top margin

        preview_toolbar_layout = QtWidgets.QHBoxLayout()
        self.btn_refresh_preview = QtWidgets.QPushButton(translate("Arch", "Refresh"))
        self.btn_refresh_preview.setIcon(FreeCADGui.getIcon(":/icons/view-refresh.svg"))
        self.btn_refresh_preview.setToolTip(
            translate("Arch", "Re-run the query and update the preview table.")
        )
        preview_toolbar_layout.addWidget(
            QtWidgets.QLabel(translate("Arch", "<b>Query Results Preview</b>"))
        )
        preview_toolbar_layout.addStretch()
        preview_toolbar_layout.addWidget(self.btn_refresh_preview)

        self.table_preview_results = QtWidgets.QTableWidget()
        self.table_preview_results.setMinimumHeight(150)
        self.table_preview_results.setEditTriggers(
            QtWidgets.QAbstractItemView.NoEditTriggers
        )  # Make read-only
        self.table_preview_results.setSelectionBehavior(QtWidgets.QAbstractItemView.SelectRows)
        preview_pane_layout.addLayout(preview_toolbar_layout)
        preview_pane_layout.addWidget(self.table_preview_results)
        self.editor_layout.addWidget(self.preview_pane)

        # Display Options GroupBox
        self.display_options_group = QtWidgets.QGroupBox(translate("Arch", "Display Options"))
        self.display_options_layout = QtWidgets.QVBoxLayout(self.display_options_group)

        self.chk_is_pipelined = QtWidgets.QCheckBox(translate("Arch", "Use as Pipeline Step"))
        self.chk_is_pipelined.setToolTip(
            translate(
                "Arch",
                "When checked, this statement will use the results of the previous statement as its data source.",
            )
        )
        self.chk_use_description_as_header = QtWidgets.QCheckBox(
            translate("Arch", "Use Description as Section Header")
        )
        self.chk_use_description_as_header.setToolTip(
            translate(
                "Arch",
                "When checked, the statement's description will be written as a merged header row before its results.",
            )
        )
        self.chk_include_column_names = QtWidgets.QCheckBox(
            translate("Arch", "Include Column Names as Headers")
        )
        self.chk_include_column_names.setToolTip(
            translate(
                "Arch",
                "Include the column headers (Label, IfcType, ...) in the spreadsheet output.",
            )
        )
        self.chk_add_empty_row_after = QtWidgets.QCheckBox(translate("Arch", "Add Empty Row After"))
        self.chk_add_empty_row_after.setToolTip(
            translate("Arch", "Insert one empty row after this statement's results.")
        )
        self.chk_print_results_in_bold = QtWidgets.QCheckBox(
            translate("Arch", "Print Results in Bold")
        )
        self.chk_print_results_in_bold.setToolTip(
            translate("Arch", "Render the result cells in bold font for emphasis.")
        )
        self.display_options_layout.addWidget(self.chk_is_pipelined)
        self.display_options_layout.addWidget(self.chk_use_description_as_header)
        self.display_options_layout.addWidget(self.chk_include_column_names)
        self.display_options_layout.addWidget(self.chk_add_empty_row_after)
        self.display_options_layout.addWidget(self.chk_print_results_in_bold)
        self.editor_layout.addWidget(self.display_options_group)

        # --- Commit Actions (Apply, Discard) ---
        self.commit_actions_layout = QtWidgets.QHBoxLayout()
        self.chk_save_and_next = QtWidgets.QCheckBox(translate("Arch", "Save and Next"))
        self.chk_save_and_next.setToolTip(
            translate(
                "Arch",
                "If checked, clicking 'Save' will automatically load the next statement for editing.",
            )
        )
        self.btn_save = QtWidgets.QPushButton(translate("Arch", "Save"))
        self.btn_save.setIcon(FreeCADGui.getIcon(":/icons/document-save.svg"))
        self.btn_save.setToolTip(
            translate("Arch", "Save changes to this statement and close the statement editor.")
        )
        self.btn_discard = QtWidgets.QPushButton(translate("Arch", "Discard"))
        self.btn_discard.setIcon(FreeCADGui.getIcon(":/icons/delete.svg"))
        self.btn_discard.setToolTip(
            translate("Arch", "Discard all changes made in the statement editor.")
        )
        self.commit_actions_layout.addStretch()
        self.commit_actions_layout.addWidget(self.chk_save_and_next)
        self.commit_actions_layout.addWidget(self.btn_discard)
        self.commit_actions_layout.addWidget(self.btn_save)
        self.editor_layout.addLayout(self.commit_actions_layout)

        # Expose form as a list of the two top-level widgets so FreeCAD creates
        # two built-in TaskBox sections. The overview goes first, editor second.
        self.form = [self.overview_widget, self.editor_widget]

        # --- Connections ---
        # Use explicit slots instead of lambda wrappers so Qt's meta-object
        # system can see the call targets and avoid creating anonymous functions.
        self.btn_add_statement.clicked.connect(self._on_add_statement_clicked)
        self.btn_remove_statement.clicked.connect(self._on_remove_selected_statement_clicked)
        self.btn_duplicate_statement.clicked.connect(self._on_duplicate_selected_statement_clicked)  # type: ignore
        self.btn_edit_selected.clicked.connect(self._on_edit_selected_clicked)
        self.table_statements.itemSelectionChanged.connect(self._on_table_selection_changed)
        self.table_statements.itemDoubleClicked.connect(self._on_item_double_clicked)
        self.template_dropdown.activated.connect(self._on_load_report_template)

        # Keep table edits in sync with the runtime statements
        self.table_statements.itemChanged.connect(self._on_table_item_changed)
        self.btn_save_template.clicked.connect(self._on_save_report_template)

        # Enable and connect the preset management buttons
        self.btn_manage_templates.setEnabled(True)
        self.btn_manage_queries.setEnabled(True)
        self.btn_manage_templates.clicked.connect(lambda: self._on_manage_presets("report"))
        self.btn_manage_queries.clicked.connect(lambda: self._on_manage_presets("query"))

        # Connect all editor fields to a generic handler to manage the dirty state.
        self.description_edit.textChanged.connect(self._on_editor_field_changed)
        self.sql_query_edit.textChanged.connect(self._on_editor_sql_changed)
        for checkbox in self.display_options_group.findChildren(QtWidgets.QCheckBox):
            checkbox.stateChanged.connect(self._on_editor_field_changed)
        self.query_preset_dropdown.activated.connect(self._on_load_query_preset)
        self.chk_is_pipelined.stateChanged.connect(self._on_editor_sql_changed)
        self.btn_save_query_preset.clicked.connect(self._on_save_query_preset)

        # Preview and Commit connections
        self.btn_toggle_preview.toggled.connect(self._on_preview_toggled)
        self.btn_refresh_preview.clicked.connect(self._run_and_display_preview)
        self.btn_save.clicked.connect(self.on_save_clicked)
        self.btn_discard.clicked.connect(self.on_discard_clicked)
        self.btn_show_cheatsheet.clicked.connect(self._show_cheatsheet_dialog)

        # Validation Timer for live SQL preview
        # Timer doesn't need a specific QWidget parent here; use no parent.
        self.validation_timer = QtCore.QTimer()
        self.validation_timer.setSingleShot(True)
        self.validation_timer.timeout.connect(self._run_live_validation_for_editor)

        # Store icons for dynamic button changes
        self.icon_show_preview = FreeCADGui.getIcon(":/icons/Std_ToggleVisibility.svg")
        self.icon_hide_preview = FreeCADGui.getIcon(":/icons/Invisible.svg")

        # Initial UI setup
        self._load_and_populate_presets()
        self._populate_table_from_statements()
        # Pass the documentation data to the editor for its tooltips
        api_docs = ArchSql.getSqlApiDocumentation()
        self.sql_query_edit.set_api_documentation(api_docs)
        self.editor_widget.setVisible(False)  # Start with editor hidden
        self._update_ui_for_mode("overview")  # Set initial button states

    def _load_and_populate_presets(self):
        """Loads all presets and populates the UI dropdowns, including tooltips."""

        def _populate_combobox(combobox, preset_type, placeholder_text):
            """Internal helper to load presets and populate a QComboBox."""
            # Load the raw preset data from the backend
            presets = _get_presets(preset_type)

            # Prepare the UI widget
            combobox.clear()
            # The placeholder_text is already translated by the caller
            combobox.addItem(placeholder_text)

            model = combobox.model()

            sorted_presets = sorted(presets.items(), key=lambda item: item[1]["name"])

            # Populate the combobox with the sorted presets
            for filename, preset in sorted_presets:
                # Add the item with its display name and stable filename (as userData)
                combobox.addItem(preset["name"], userData=filename)

                # Get the index of the item that was just added
                index = combobox.count() - 1

                # Access the description from the nested "data" dictionary.
                description = preset["data"].get("description", "").strip()

                if description:
                    item = model.item(index)
                    if item:
                        item.setToolTip(description)

            return presets

        # Use the helper function to populate both dropdowns,
        # ensuring the placeholder strings are translatable.
        self.query_presets = _populate_combobox(
            self.query_preset_dropdown, "query", translate("Arch", "--- Select a Query Preset ---")
        )
        self.report_templates = _populate_combobox(
            self.template_dropdown, "report", translate("Arch", "--- Load a Report Template ---")
        )

    def _on_manage_presets(self, mode):
        """
        Launches the ManagePresetsDialog and refreshes the dropdowns
        when the dialog is closed.
        """
        dialog = ManagePresetsDialog(mode, parent=self.form[0])
        dialog.exec_()

        # Refresh the dropdowns to reflect any changes made
        self._load_and_populate_presets()

    @Slot("QTableWidgetItem")
    def _on_item_double_clicked(self, item):
        """Handles a double-click on an item in the statements table."""
        if item:
            # A double-click is a shortcut for editing the full statement.
            self._start_edit_session(row_index=item.row())

    # --- Statement Management (Buttons and Table Interaction) ---
    def _populate_table_from_statements(self):
        # Avoid emitting itemChanged while we repopulate programmatically
        self.table_statements.blockSignals(True)
        self.table_statements.setRowCount(0)  # Clear existing rows
        # The UI always interacts with the live list of objects from the proxy
        for row_idx, statement in enumerate(self.obj.Proxy.live_statements):
            self.table_statements.insertRow(row_idx)
            # Description (editable text)
            desc_item = QtWidgets.QTableWidgetItem(statement.description)
            desc_item.setFlags(
                desc_item.flags()
                | QtCore.Qt.ItemIsEditable
                | QtCore.Qt.ItemIsSelectable
                | QtCore.Qt.ItemIsEnabled
            )
            desc_item.setToolTip(translate("Arch", "Double-click to edit description in place."))
            self.table_statements.setItem(row_idx, 0, desc_item)

            # Pipe checkbox
            pipe_item = QtWidgets.QTableWidgetItem()
            pipe_item.setFlags(
                QtCore.Qt.ItemIsUserCheckable | QtCore.Qt.ItemIsEnabled | QtCore.Qt.ItemIsSelectable
            )
            pipe_item.setCheckState(
                QtCore.Qt.Checked if statement.is_pipelined else QtCore.Qt.Unchecked
            )
            if row_idx == 0:
                pipe_item.setFlags(
                    pipe_item.flags() & ~QtCore.Qt.ItemIsEnabled
                )  # Disable for first row
                pipe_item.setToolTip(translate("Arch", "The first statement cannot be pipelined."))
            else:
                pipe_item.setToolTip(
                    translate(
                        "Arch", "Toggle whether to use the previous statement's results as input."
                    )
                )
            self.table_statements.setItem(row_idx, 1, pipe_item)

            # Header checkbox
            header_item = QtWidgets.QTableWidgetItem()
            header_item.setFlags(
                QtCore.Qt.ItemIsUserCheckable | QtCore.Qt.ItemIsEnabled | QtCore.Qt.ItemIsSelectable
            )
            header_item.setCheckState(
                QtCore.Qt.Checked if statement.use_description_as_header else QtCore.Qt.Unchecked
            )
            header_item.setToolTip(
                translate(
                    "Arch",
                    "Toggle whether to use this statement's Description as a section header.",
                )
            )
            self.table_statements.setItem(row_idx, 2, header_item)

            # Cols checkbox (Include Column Names)
            cols_item = QtWidgets.QTableWidgetItem()
            cols_item.setFlags(
                QtCore.Qt.ItemIsUserCheckable | QtCore.Qt.ItemIsEnabled | QtCore.Qt.ItemIsSelectable
            )
            cols_item.setCheckState(
                QtCore.Qt.Checked if statement.include_column_names else QtCore.Qt.Unchecked
            )
            cols_item.setToolTip(
                translate(
                    "Arch", "Toggle whether to include this statement's column names in the report."
                )
            )
            self.table_statements.setItem(row_idx, 3, cols_item)

            # Status Item (Icon + Tooltip) - read-only
            status_icon, status_tooltip = self._get_status_icon_and_tooltip(statement)
            status_item = QtWidgets.QTableWidgetItem()
            status_item.setIcon(status_icon)
            status_item.setToolTip(status_tooltip)
            # Display the object count next to the icon for valid queries.
            if statement._validation_status in ("OK", "0_RESULTS"):
                status_item.setText(str(statement._validation_count))
                # Align the text to the right for better visual separation.
                status_item.setTextAlignment(QtCore.Qt.AlignRight | QtCore.Qt.AlignVCenter)
            status_item.setFlags(status_item.flags() & ~QtCore.Qt.ItemIsEditable)  # Make read-only
            self.table_statements.setItem(row_idx, 4, status_item)

        # After populating all rows, trigger a validation for all statements.
        # This ensures the counts and statuses are up-to-date when the panel opens.
        for statement in self.obj.Proxy.live_statements:
            statement.validate_and_update_status()
            self._update_table_row_status(
                self.obj.Proxy.live_statements.index(statement), statement
            )

        # Re-enable signals after population so user edits are handled
        self.table_statements.blockSignals(False)

    # --- Explicit Qt Slot Wrappers ---
    @Slot()
    def _on_add_statement_clicked(self):
        """Slot wrapper for the Add button (clicked)."""
        # Default behavior: create a new statement but do not open editor.
        self._add_statement(start_editing=False)

    @Slot()
    def _on_remove_selected_statement_clicked(self):
        """Slot wrapper for the Remove button (clicked)."""
        self._remove_selected_statement()

    @Slot()
    def _on_duplicate_selected_statement_clicked(self):
        """Slot wrapper for the Duplicate button (clicked)."""
        self._duplicate_selected_statement()

    @Slot()
    def _on_edit_selected_clicked(self):
        """Slot wrapper for the Edit Selected button (clicked)."""
        # Delegate to _start_edit_session() which will find the selection if no
        # explicit row_index is given.
        self._start_edit_session()

    def _on_table_item_changed(self, item):
        """Synchronize direct table edits (description and checkboxes) back into the runtime statement."""
        row = item.row()
        col = item.column()
        if row < 0 or row >= len(self.obj.Proxy.live_statements):
            return
        stmt = self.obj.Proxy.live_statements[row]

        if col == 0:  # Description
            new_text = item.text()
            if stmt.description != new_text:
                stmt.description = new_text
                self._set_dirty(True)

        elif col == 1:  # Pipe checkbox
            is_checked = item.checkState() == QtCore.Qt.Checked
            if stmt.is_pipelined != is_checked:
                stmt.is_pipelined = is_checked
                self._set_dirty(True)
                # Re-validate the editor if its context has changed
                if self.current_edited_statement_index != -1:
                    self._run_live_validation_for_editor()

        elif col == 2:  # Header checkbox
            is_checked = item.checkState() == QtCore.Qt.Checked
            if stmt.use_description_as_header != is_checked:
                stmt.use_description_as_header = is_checked
                self._set_dirty(True)

        elif col == 3:  # Cols checkbox
            is_checked = item.checkState() == QtCore.Qt.Checked
            if stmt.include_column_names != is_checked:
                stmt.include_column_names = is_checked
                self._set_dirty(True)

    def _on_row_moved(self, logical_index, old_visual_index, new_visual_index):
        """Handles the reordering of statements via drag-and-drop."""
        # The visual index is what the user sees. The logical index is tied to the original sort.
        # When a row is moved, we need to map the visual change back to our data model.

        # Pop the item from its original position in the data model.
        moving_statement = self.obj.Proxy.live_statements.pop(old_visual_index)
        # Insert it into its new position.
        self.obj.Proxy.live_statements.insert(new_visual_index, moving_statement)

        self._set_dirty(True)
        # After reordering the data, we must repopulate the table to ensure
        # everything is visually correct and consistent, especially the disabled
        # "Pipe" checkbox on the new first row.
        self._populate_table_from_statements()
        # Restore the selection to the row that was just moved.
        self.table_statements.selectRow(new_visual_index)

    def _add_statement(self, start_editing=False):
        """Creates a new statement, adds it to the report, and optionally starts editing it."""
        # Create the new statement object and add it to the live list.
        new_statement = ReportStatement(
            description=translate(
                "Arch", f"New Statement {len(self.obj.Proxy.live_statements) + 1}"
            )
        )
        self.obj.Proxy.live_statements.append(new_statement)

        # Refresh the entire overview table to show the new row.
        self._populate_table_from_statements()

        # Validate the new (empty) statement to populate its status.
        new_statement.validate_and_update_status()

        new_row_index = len(self.obj.Proxy.live_statements) - 1
        if start_editing:
            self._start_edit_session(row_index=new_row_index)
        else:
            self.table_statements.selectRow(new_row_index)

        self._set_dirty(True)

    def _remove_selected_statement(self):
        selected_rows = self.table_statements.selectionModel().selectedRows()
        if not selected_rows:
            return

        row_to_remove = selected_rows[0].row()
        description_to_remove = self.table_statements.item(row_to_remove, 0).text()

        if (
            QtWidgets.QMessageBox.question(
                None,
                translate("Arch", "Remove Statement"),
                translate(
                    "Arch", f"Are you sure you want to remove statement '{description_to_remove}'?"
                ),
                QtWidgets.QMessageBox.Yes | QtWidgets.QMessageBox.No,
            )
            == QtWidgets.QMessageBox.Yes
        ):
            self.obj.Proxy.live_statements.pop(row_to_remove)
            self._set_dirty(True)
            self._populate_table_from_statements()
            self._end_edit_session()  # Close editor and reset selection

    def _duplicate_selected_statement(self):
        """Duplicates the selected statement without opening the editor."""
        selected_rows = self.table_statements.selectionModel().selectedRows()
        if not selected_rows:
            return

        row_to_duplicate = selected_rows[0].row()
        original = self.obj.Proxy.live_statements[row_to_duplicate]

        duplicated = ReportStatement()
        duplicated.loads(original.dumps())
        duplicated.description = translate("Arch", f"Copy of {original.description}")

        self.obj.Proxy.live_statements.insert(row_to_duplicate + 1, duplicated)
        self._set_dirty(True)
        self._populate_table_from_statements()
        duplicated.validate_and_update_status()

        # New behavior: Just select the newly created row. Do NOT open the editor.
        self.table_statements.selectRow(row_to_duplicate + 1)

    def _select_statement_in_table(self, row_idx):
        # Select the row visually and trigger the new edit session
        self.table_statements.selectRow(row_idx)
        # This method should ONLY select, not start an edit session.

    # --- Editor (Box 2) Management ---

    def _load_statement_to_editor(self, statement: ReportStatement):
        # Disable/enable the pipeline checkbox based on row index
        is_first_statement = self.current_edited_statement_index == 0
        self.chk_is_pipelined.setEnabled(not is_first_statement)
        if is_first_statement:
            # Ensure the first statement can never be pipelined
            statement.is_pipelined = False

        self.description_edit.setText(statement.description)
        self.sql_query_edit.setPlainText(statement.query_string)
        self.chk_is_pipelined.setChecked(statement.is_pipelined)
        self.chk_use_description_as_header.setChecked(statement.use_description_as_header)
        self.chk_include_column_names.setChecked(statement.include_column_names)
        self.chk_add_empty_row_after.setChecked(statement.add_empty_row_after)
        self.chk_print_results_in_bold.setChecked(statement.print_results_in_bold)

        # We must re-run validation here because the context may have changed
        self._run_live_validation_for_editor()

    def _save_current_editor_state_to_statement(self):
        if self.current_edited_statement_index != -1 and self.current_edited_statement_index < len(
            self.obj.Proxy.live_statements
        ):
            statement = self.obj.Proxy.live_statements[self.current_edited_statement_index]
            statement.description = self.description_edit.text()
            statement.query_string = self.sql_query_edit.toPlainText()
            statement.use_description_as_header = self.chk_use_description_as_header.isChecked()
            statement.include_column_names = self.chk_include_column_names.isChecked()
            statement.add_empty_row_after = self.chk_add_empty_row_after.isChecked()
            statement.print_results_in_bold = self.chk_print_results_in_bold.isChecked()
            statement.validate_and_update_status()  # Update status in the statement object
            self._update_table_row_status(
                self.current_edited_statement_index, statement
            )  # Refresh table status

    def _on_editor_sql_changed(self):
        """Handles text changes in the SQL editor, triggering validation."""
        self._on_editor_field_changed()  # Mark as dirty
        # Immediately switch to a neutral "Typing..." state to provide
        # instant feedback and hide any previous validation messages.
        self.sql_query_status_label.setText(translate("Arch", "<i>Typing...</i>"))
        self.sql_query_status_label.setStyleSheet("color: gray;")
        # Start (or restart) the timer for the full validation.
        self.validation_timer.start(500)

    def _on_editor_field_changed(self, *args):
        """A generic slot that handles any change in an editor field to mark it as dirty.

        This method is connected to multiple signal signatures (textChanged -> str,
        stateChanged -> int). Leaving it undecorated (or accepting *args) keeps it
        flexible so Qt can call it with varying argument lists.
        """
        self._set_dirty(True)

    @Slot(int)
    def _on_load_query_preset(self, index):
        """Handles the selection of a query preset from the dropdown."""
        if index == 0:  # Ignore the placeholder item
            return

        filename = self.query_preset_dropdown.itemData(index)
        preset_data = self.query_presets.get(filename, {}).get("data")

        if not preset_data:
            FreeCAD.Console.PrintError(
                f"BIM Report: Could not load data for query preset with filename '{filename}'.\n"
            )
            self.query_preset_dropdown.setCurrentIndex(0)
            return

        # Confirm before overwriting existing text
        if self.sql_query_edit.toPlainText().strip():
            reply = QtWidgets.QMessageBox.question(
                None,
                translate("Arch", "Overwrite Query?"),
                translate(
                    "Arch",
                    "Loading a preset will overwrite the current text in the query editor. Continue?",
                ),
                QtWidgets.QMessageBox.Yes | QtWidgets.QMessageBox.No,
                QtWidgets.QMessageBox.No,
            )
            if reply == QtWidgets.QMessageBox.No:
                self.query_preset_dropdown.setCurrentIndex(0)  # Reset dropdown
                return

        if "query" in preset_data:
            self.sql_query_edit.setPlainText(preset_data["query"])

        # Reset dropdown to act as a one-shot action button
        self.query_preset_dropdown.setCurrentIndex(0)

    @Slot()
    def _on_save_query_preset(self):
        """Saves the current query text as a new user preset."""
        current_query = self.sql_query_edit.toPlainText().strip()
        if not current_query:
            QtWidgets.QMessageBox.warning(
                None,
                translate("Arch", "Empty Query"),
                translate("Arch", "Cannot save an empty query as a preset."),
            )
            return

        preset_name, ok = QtWidgets.QInputDialog.getText(
            None, translate("Arch", "Save Query Preset"), translate("Arch", "Preset Name:")
        )
        if ok and preset_name:
            # The data payload does not include the 'name' key; _save_preset adds it.
            preset_data = {"description": "User-defined query preset.", "query": current_query}
            _save_preset("query", preset_name, preset_data)
            self._load_and_populate_presets()  # Refresh the dropdown with the new preset

    @Slot(int)
    def _on_load_report_template(self, index):
        """Handles the selection of a full report template from the dropdown."""
        if index == 0:
            return

        filename = self.template_dropdown.itemData(index)
        template_data = self.report_templates.get(filename, {}).get("data")

        if not template_data:
            FreeCAD.Console.PrintError(
                f"BIM Report: Could not load data for template with filename '{filename}'.\n"
            )
            self.template_dropdown.setCurrentIndex(0)
            return

        if self.obj.Proxy.live_statements:
            reply = QtWidgets.QMessageBox.question(
                None,
                translate("Arch", "Overwrite Report?"),
                translate(
                    "Arch",
                    "Loading a template will replace all current statements in this report. Continue?",
                ),
                QtWidgets.QMessageBox.Yes | QtWidgets.QMessageBox.No,
                QtWidgets.QMessageBox.No,
            )
            if reply == QtWidgets.QMessageBox.No:
                self.template_dropdown.setCurrentIndex(0)
                return

        if "statements" in template_data:
            # Rebuild the live list from the template data
            self.obj.Proxy.live_statements = []
            for s_data in template_data["statements"]:
                statement = ReportStatement()
                statement.loads(s_data)
                self.obj.Proxy.live_statements.append(statement)

            self._populate_table_from_statements()

            # Terminate any active editing session, as loading a template invalidates it. This
            # correctly resets the entire UI state.
            self._end_edit_session()

            self._set_dirty(True)

        self.template_dropdown.setCurrentIndex(0)

    @Slot()
    def _on_save_report_template(self):
        """Saves the current set of statements as a new report template."""
        if not self.obj.Proxy.live_statements:
            QtWidgets.QMessageBox.warning(
                None,
                translate("Arch", "Empty Report"),
                translate("Arch", "Cannot save an empty report as a template."),
            )
            return

        template_name, ok = QtWidgets.QInputDialog.getText(
            None, translate("Arch", "Save Report Template"), translate("Arch", "Template Name:")
        )
        if ok and template_name:
            # The data payload does not include the 'name' key.
            template_data = {
                "description": "User-defined report template.",
                "statements": [s.dumps() for s in self.obj.Proxy.live_statements],
            }
            _save_preset("report", template_name, template_data)
            self._load_and_populate_presets()  # Refresh the template dropdown

    def _run_live_validation_for_editor(self):
        """
        Runs live validation for the query in the editor, providing
        contextual feedback if the statement is part of a pipeline.
        This method does NOT modify the underlying statement object.
        """
        if self.current_edited_statement_index == -1:
            return

        current_query = self.sql_query_edit.toPlainText()
        is_pipelined = self.chk_is_pipelined.isChecked()

        # Create a temporary, in-memory statement object for validation.
        # This prevents mutation of the real data model.
        temp_statement = ReportStatement()

        source_objects = None
        input_count_str = ""

        if is_pipelined and self.current_edited_statement_index > 0:
            preceding_statements = self.obj.Proxy.live_statements[
                : self.current_edited_statement_index
            ]
            source_objects = ArchSql._execute_pipeline_for_objects(preceding_statements)
            input_count = len(source_objects)
            input_count_str = translate("Arch", f" (from {input_count} in pipeline)")

        count, error = ArchSql.count(current_query, source_objects=source_objects)

        # --- Update the UI display using the validation results ---
        if not error and count > 0:
            temp_statement._validation_status = "OK"
            temp_statement._validation_message = f"{translate('Arch', 'Found')} {count} {translate('Arch', 'objects')}{input_count_str}."
        elif not error and count == 0:
            temp_statement._validation_status = "0_RESULTS"
            # The message for 0 results is more of a warning than a success.
            temp_statement._validation_message = (
                f"{translate('Arch', 'Query is valid but found 0 objects')}{input_count_str}."
            )
        elif error == "INCOMPLETE":
            temp_statement._validation_status = "INCOMPLETE"
            temp_statement._validation_message = translate("Arch", "Query is incomplete")
        else:  # An actual error occurred
            temp_statement._validation_status = "ERROR"
            temp_statement._validation_message = f"{error}{input_count_str}"

        self._update_editor_status_display(temp_statement)

    def _update_editor_status_display(self, statement: ReportStatement):
        # Update the status label (below SQL editor) in Box 2
        # The "Typing..." state is now handled instantly by _on_editor_sql_changed.
        # This method only handles the final states (Incomplete, Error, 0, OK).
        if statement._validation_status == "INCOMPLETE":
            self.sql_query_status_label.setText(f"⚠️ {statement._validation_message}")
            self.sql_query_status_label.setStyleSheet("color: orange;")
        elif statement._validation_status == "ERROR":
            self.sql_query_status_label.setText(f"❌ {statement._validation_message}")
            self.sql_query_status_label.setStyleSheet("color: red;")
        elif statement._validation_status == "0_RESULTS":
            self.sql_query_status_label.setText(f"⚠️ {statement._validation_message}")
            self.sql_query_status_label.setStyleSheet("color: orange;")
        else:  # OK or Ready
            self.sql_query_status_label.setText(f"✅ {statement._validation_message}")
            self.sql_query_status_label.setStyleSheet("color: green;")

        # The preview button should only be enabled if the query is valid and
        # can be executed (even if it returns 0 results).
        is_executable = statement._validation_status in ("OK", "0_RESULTS")
        self.btn_toggle_preview.setEnabled(is_executable)

    def _update_table_row_status(self, row_idx, statement: ReportStatement):
        """Updates the status icon/tooltip and other data in the QTableWidget for a given row."""
        if row_idx < 0 or row_idx >= self.table_statements.rowCount():
            return

        # Correct Column Mapping:
        # 0: Description
        # 1: Pipe
        # 2: Header
        # 3: Cols
        # 4: Status

        # Update all cells in the row to be in sync with the statement object.
        # This is safer than assuming which property might have changed.
        self.table_statements.item(row_idx, 0).setText(statement.description)
        self.table_statements.item(row_idx, 1).setCheckState(
            QtCore.Qt.Checked if statement.is_pipelined else QtCore.Qt.Unchecked
        )
        self.table_statements.item(row_idx, 2).setCheckState(
            QtCore.Qt.Checked if statement.use_description_as_header else QtCore.Qt.Unchecked
        )
        self.table_statements.item(row_idx, 3).setCheckState(
            QtCore.Qt.Checked if statement.include_column_names else QtCore.Qt.Unchecked
        )

        status_item = self.table_statements.item(row_idx, 4)
        if status_item:
            status_icon, status_tooltip = self._get_status_icon_and_tooltip(statement)
            status_item.setIcon(status_icon)
            status_item.setToolTip(status_tooltip)
            # Update the text as well
            if statement._validation_status in ("OK", "0_RESULTS"):
                status_item.setText(str(statement._validation_count))
            else:
                status_item.setText("")  # Clear the text for error/incomplete states

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
        return QtGui.QIcon(), translate("Arch", "Ready")  # Default/initial state

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
        api_data = ArchSql.getSqlApiDocumentation()
        dialog = CheatsheetDialog(api_data, parent=self.editor_widget)
        dialog.exec_()

    def _build_completion_model(self):
        """
        Builds the master list of words for the autocompleter.

        This method gets raw data from the SQL engine and then applies all
        UI-specific formatting, such as combining keywords into phrases and
        adding trailing spaces for a better user experience.
        """
        # 1. Get the set of keywords that should NOT get a trailing space.
        no_space_keywords = ArchSql.getSqlKeywords(kind="no_space")

        # 2. Get the raw list of all individual keywords.
        raw_keywords = set(ArchSql.getSqlKeywords())

        # 3. Define UI-specific phrases and their components.
        smart_clauses = {"GROUP BY ": ("GROUP", "BY"), "ORDER BY ": ("ORDER", "BY")}

        # 4. Build the final set of completion words.
        all_words = set()

        # Add the smart phrases directly.
        all_words.update(smart_clauses.keys())

        # Get the individual components of the smart phrases to avoid adding them twice.
        words_to_skip = {word for components in smart_clauses.values() for word in components}

        for word in raw_keywords:
            if word in words_to_skip:
                continue

            if word in no_space_keywords:
                all_words.add(word)  # Add without a space
            else:
                all_words.add(word + " ")  # Add with a space by default

        # 5. Add all unique property names from the document (without spaces).
        if FreeCAD.ActiveDocument:
            property_names = set()
            for obj in FreeCAD.ActiveDocument.Objects:
                for prop_name in obj.PropertiesList:
                    if prop_name not in self.PROPERTY_BLOCKLIST:
                        property_names.add(prop_name)
            all_words.update(property_names)

        # 6. Return a sorted model for the completer.
        return QtCore.QStringListModel(sorted(list(all_words)))

    def _update_ui_for_mode(self, mode):
        """Centralizes enabling/disabling of UI controls based on the current mode."""
        if mode == "editing":
            # In edit mode, disable overview actions to prevent conflicts
            self.btn_add_statement.setEnabled(False)
            self.btn_remove_statement.setEnabled(False)
            self.btn_duplicate_statement.setEnabled(False)
            self.btn_edit_selected.setEnabled(False)
            self.template_dropdown.setEnabled(False)
            self.btn_save_template.setEnabled(False)
            self.table_statements.setEnabled(False)
        else:  # "overview" mode
            # In overview mode, re-enable controls
            self.btn_add_statement.setEnabled(True)
            self.btn_remove_statement.setEnabled(True)
            self.btn_duplicate_statement.setEnabled(True)
            self.template_dropdown.setEnabled(True)
            self.btn_save_template.setEnabled(True)
            self.table_statements.setEnabled(True)
            # The "Edit" button state depends on whether a row is selected
            self._on_table_selection_changed()

    def _on_table_selection_changed(self):
        """Slot for selection changes in the overview table."""
        # This method's only job is to enable the "Edit" button if a row is selected.
        has_selection = bool(self.table_statements.selectionModel().selectedRows())
        self.btn_edit_selected.setEnabled(has_selection)

    def _start_edit_session(self, row_index=None):
        """Loads a statement into the editor and displays it."""
        if row_index is None:
            selected_rows = self.table_statements.selectionModel().selectedRows()
            if not selected_rows:
                return
            row_index = selected_rows[0].row()

        # Explicitly hide the preview pane and reset the toggle when starting a new session.
        self.preview_pane.setVisible(False)
        self.btn_toggle_preview.setChecked(False)

        self.current_edited_statement_index = row_index
        statement = self.obj.Proxy.live_statements[row_index]

        # Load data into the editor
        self._load_statement_to_editor(statement)

        # Show editor and set focus
        self.editor_widget.setVisible(True)
        self.sql_query_edit.setFocus()
        self._update_ui_for_mode("editing")

        # Initially disable the preview button until the first validation confirms
        # that the query is executable.
        self.btn_toggle_preview.setEnabled(False)

    def _end_edit_session(self):
        """Hides the editor and restores the overview state."""
        self.editor_widget.setVisible(False)
        self.preview_pane.setVisible(False)  # Also hide preview if it was open
        self.btn_toggle_preview.setChecked(False)  # Ensure toggle is reset
        self.current_edited_statement_index = -1
        self._update_ui_for_mode("overview")
        self.table_statements.setFocus()

    def _commit_changes(self):
        """Saves the data from the editor back to the live statement object."""
        if self.current_edited_statement_index == -1:
            return

        statement = self.obj.Proxy.live_statements[self.current_edited_statement_index]
        statement.description = self.description_edit.text()
        statement.query_string = self.sql_query_edit.toPlainText()
        statement.is_pipelined = self.chk_is_pipelined.isChecked()
        statement.use_description_as_header = self.chk_use_description_as_header.isChecked()
        statement.include_column_names = self.chk_include_column_names.isChecked()
        statement.add_empty_row_after = self.chk_add_empty_row_after.isChecked()
        statement.print_results_in_bold = self.chk_print_results_in_bold.isChecked()

        statement.validate_and_update_status()
        self._update_table_row_status(self.current_edited_statement_index, statement)
        self._set_dirty(True)

    def on_save_clicked(self):
        """Saves changes and either closes the editor or adds a new statement."""
        # First, always commit the changes from the current edit session.
        self._commit_changes()

        if self.chk_save_and_next.isChecked():
            # If the checkbox is checked, the "Next" action is to add a new
            # blank statement. The _add_statement helper already handles
            # creating the statement and opening it in the editor.
            self._add_statement(start_editing=True)
        else:
            # The default action is to simply close the editor.
            self._end_edit_session()

    def on_discard_clicked(self):
        """Discards changes and closes the editor."""
        self._end_edit_session()

    @Slot(bool)
    def _on_preview_toggled(self, checked):
        """Shows or hides the preview pane and updates the toggle button's appearance."""
        if checked:
            self.btn_toggle_preview.setText(translate("Arch", "Hide Preview"))
            self.btn_toggle_preview.setIcon(self.icon_hide_preview)
            self.preview_pane.setVisible(True)
            self.btn_refresh_preview.setVisible(True)
            self._run_and_display_preview()
        else:
            self.btn_toggle_preview.setText(translate("Arch", "Show Preview"))
            self.btn_toggle_preview.setIcon(self.icon_show_preview)
            self.preview_pane.setVisible(False)
            self.btn_refresh_preview.setVisible(False)

    def _run_and_display_preview(self):
        """Executes the query in the editor and populates the preview table, respecting the pipeline context."""
        query = self.sql_query_edit.toPlainText().strip()
        is_pipelined = self.chk_is_pipelined.isChecked()

        if not self.preview_pane.isVisible():
            return
        if not query:
            self.table_preview_results.clear()
            self.table_preview_results.setRowCount(0)
            self.table_preview_results.setColumnCount(0)
            return

        source_objects = None
        if is_pipelined and self.current_edited_statement_index > 0:
            preceding_statements = self.obj.Proxy.live_statements[
                : self.current_edited_statement_index
            ]
            source_objects = ArchSql._execute_pipeline_for_objects(preceding_statements)

        try:
            # Run the preview with the correct context.
            headers, data_rows, _ = ArchSql._run_query(
                query, mode="full_data", source_objects=source_objects
            )

            self.table_preview_results.clear()
            self.table_preview_results.setColumnCount(len(headers))
            self.table_preview_results.setHorizontalHeaderLabels(headers)
            self.table_preview_results.setRowCount(len(data_rows))

            for row_idx, row_data in enumerate(data_rows):
                for col_idx, cell_value in enumerate(row_data):
                    item = QtWidgets.QTableWidgetItem(str(cell_value))
                    self.table_preview_results.setItem(row_idx, col_idx, item)
            self.table_preview_results.horizontalHeader().setSectionResizeMode(
                QtWidgets.QHeaderView.Interactive
            )

        except (ArchSql.SqlEngineError, ArchSql.BimSqlSyntaxError) as e:
            # Error handling remains the same
            self.table_preview_results.clear()
            self.table_preview_results.setRowCount(1)
            self.table_preview_results.setColumnCount(1)
            self.table_preview_results.setHorizontalHeaderLabels(["Query Error"])
            error_item = QtWidgets.QTableWidgetItem(f"❌ {str(e)}")
            error_item.setForeground(QtGui.QColor("red"))
            self.table_preview_results.setItem(0, 0, error_item)
            self.table_preview_results.horizontalHeader().setSectionResizeMode(
                0, QtWidgets.QHeaderView.Stretch
            )

    # --- Dialog Acceptance / Rejection ---

    def accept(self):
        """Saves changes from UI to Report object and triggers recompute."""
        # First, check if there is an active, unsaved edit session.
        if self.current_edited_statement_index != -1:
            reply = QtWidgets.QMessageBox.question(
                None,
                translate("Arch", "Unsaved Changes"),
                translate(
                    "Arch",
                    "You have unsaved changes in the statement editor. Do you want to save them before closing?",
                ),
                QtWidgets.QMessageBox.Save
                | QtWidgets.QMessageBox.Discard
                | QtWidgets.QMessageBox.Cancel,
                QtWidgets.QMessageBox.Save,
            )

            if reply == QtWidgets.QMessageBox.Save:
                self._commit_changes()
            elif reply == QtWidgets.QMessageBox.Cancel:
                return  # Abort the close operation entirely.
            # If Discard, do nothing and proceed with closing.

        # This is the "commit" step: persist the live statements to the document object.
        self.obj.Proxy.commit_statements()

        # Trigger a recompute to run the report and mark the document as modified.
        # This will now run the final, correct pipeline.
        FreeCAD.ActiveDocument.recompute()

        # Quality of life: open the target spreadsheet to show the results.
        spreadsheet = self.obj.Target
        if spreadsheet:
            FreeCADGui.ActiveDocument.setEdit(spreadsheet.Name, 0)

        # Close the task panel.
        try:
            FreeCADGui.Control.closeDialog()
        except Exception as e:
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
    from PySide.QtGui import QDesktopServices
    from PySide.QtCore import QUrl

    class ManagePresetsDialog(QtWidgets.QDialog):
        """A dialog for managing user-created presets (rename, delete, edit source)."""

        def __init__(self, mode, parent=None):
            super().__init__(parent)
            self.mode = mode  # 'query' or 'report'
            self.setWindowTitle(translate("Arch", f"Manage {mode.capitalize()} Presets"))
            self.setMinimumSize(500, 400)

            # --- UI Layout ---
            self.layout = QtWidgets.QVBoxLayout(self)

            self.preset_list = QtWidgets.QListWidget()
            self.layout.addWidget(self.preset_list)

            self.buttons_layout = QtWidgets.QHBoxLayout()
            self.btn_rename = QtWidgets.QPushButton(translate("Arch", "Rename..."))
            self.btn_delete = QtWidgets.QPushButton(translate("Arch", "Delete"))
            self.btn_edit_source = QtWidgets.QPushButton(translate("Arch", "Edit Source..."))
            self.btn_close = QtWidgets.QPushButton(translate("Arch", "Close"))

            self.buttons_layout.addWidget(self.btn_rename)
            self.buttons_layout.addWidget(self.btn_delete)
            self.buttons_layout.addStretch()
            self.buttons_layout.addWidget(self.btn_edit_source)
            self.layout.addLayout(self.buttons_layout)
            self.layout.addWidget(self.btn_close)

            # --- Connections ---
            self.btn_close.clicked.connect(self.accept)
            self.preset_list.itemSelectionChanged.connect(self._on_selection_changed)
            self.btn_rename.clicked.connect(self._on_rename)
            self.btn_delete.clicked.connect(self._on_delete)
            self.btn_edit_source.clicked.connect(self._on_edit_source)

            # --- Initial State ---
            self._populate_list()
            self._on_selection_changed()  # Set initial button states

        def _populate_list(self):
            """Fills the list widget with system and user presets."""
            self.preset_list.clear()
            self.presets = _get_presets(self.mode)

            # Sort by display name for consistent UI order
            sorted_presets = sorted(self.presets.items(), key=lambda item: item[1]["name"])

            for filename, preset_data in sorted_presets:
                item = QtWidgets.QListWidgetItem()
                display_text = preset_data["name"]

                if preset_data["is_user"]:
                    item.setText(f"{display_text} (User)")
                else:
                    item.setText(display_text)
                    # Make system presets visually distinct and non-selectable for modification
                    item.setForeground(QtGui.QColor("gray"))
                    flags = item.flags()
                    flags &= ~QtCore.Qt.ItemIsSelectable
                    item.setFlags(flags)

                # Store the stable filename as data in the item
                item.setData(QtCore.Qt.UserRole, filename)
                self.preset_list.addItem(item)

        def _on_selection_changed(self):
            """Enables/disables buttons based on the current selection."""
            selected_items = self.preset_list.selectedItems()
            is_user_preset_selected = False

            if selected_items:
                filename = selected_items[0].data(QtCore.Qt.UserRole)
                if self.presets[filename]["is_user"]:
                    is_user_preset_selected = True

            self.btn_rename.setEnabled(is_user_preset_selected)
            self.btn_delete.setEnabled(is_user_preset_selected)
            self.btn_edit_source.setEnabled(is_user_preset_selected)

            # --- Add Tooltips for Disabled State (Refinement #2) ---
            tooltip = translate("Arch", "This action is only available for user-created presets.")
            self.btn_rename.setToolTip("" if is_user_preset_selected else tooltip)
            self.btn_delete.setToolTip("" if is_user_preset_selected else tooltip)
            self.btn_edit_source.setToolTip("" if is_user_preset_selected else tooltip)

        def _on_rename(self):
            """Handles the rename action."""
            item = self.preset_list.selectedItems()[0]
            filename = item.data(QtCore.Qt.UserRole)
            current_name = self.presets[filename]["name"]

            # --- Live Name Collision Check (Refinement #2) ---
            existing_names = {p["name"] for f, p in self.presets.items() if f != filename}

            new_name, ok = QtWidgets.QInputDialog.getText(
                self,
                translate("Arch", "Rename Preset"),
                translate("Arch", "New name:"),
                text=current_name,
            )
            if ok and new_name and new_name != current_name:
                if new_name in existing_names:
                    QtWidgets.QMessageBox.warning(
                        self,
                        translate("Arch", "Name Conflict"),
                        translate(
                            "Arch",
                            "A preset with this name already exists. Please choose a different name.",
                        ),
                    )
                    return

                _rename_preset(self.mode, filename, new_name)
                self._populate_list()  # Refresh the list

        def _on_delete(self):
            """Handles the delete action."""
            item = self.preset_list.selectedItems()[0]
            filename = item.data(QtCore.Qt.UserRole)
            name = self.presets[filename]["name"]

            reply = QtWidgets.QMessageBox.question(
                self,
                translate("Arch", "Delete Preset"),
                translate(
                    "Arch", f"Are you sure you want to permanently delete the preset '{name}'?"
                ),
                QtWidgets.QMessageBox.Yes | QtWidgets.QMessageBox.No,
                QtWidgets.QMessageBox.No,
            )

            if reply == QtWidgets.QMessageBox.Yes:
                _delete_preset(self.mode, filename)
                self._populate_list()

        def _on_edit_source(self):
            """Opens the preset's JSON file in an external editor."""
            item = self.preset_list.selectedItems()[0]
            filename = item.data(QtCore.Qt.UserRole)
            _, user_path = _get_preset_paths(self.mode)
            file_path = os.path.join(user_path, filename)

            if not os.path.exists(file_path):
                QtWidgets.QMessageBox.critical(
                    self,
                    translate("Arch", "File Not Found"),
                    translate("Arch", f"Could not find the preset file at:\n{file_path}"),
                )
                return

            # --- Use QDesktopServices for robust, cross-platform opening (Refinement #3) ---
            url = QUrl.fromLocalFile(file_path)
            if not QDesktopServices.openUrl(url):
                QtWidgets.QMessageBox.warning(
                    self,
                    translate("Arch", "Could Not Open File"),
                    translate(
                        "Arch",
                        "FreeCAD could not open the file. Please check if you have a default text editor configured in your operating system.",
                    ),
                )

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
            keyword_format.setForeground(QtGui.QColor("#0070C0"))  # Dark Blue
            keyword_format.setFontWeight(QtGui.QFont.Bold)

            function_format = QtGui.QTextCharFormat()
            function_format.setForeground(QtGui.QColor("#800080"))  # Purple
            function_format.setFontItalic(True)

            string_format = QtGui.QTextCharFormat()
            string_format.setForeground(QtGui.QColor("#A31515"))  # Dark Red

            comment_format = QtGui.QTextCharFormat()
            comment_format.setForeground(QtGui.QColor("#008000"))  # Green
            comment_format.setFontItalic(True)

            # --- Build Rules List ---
            self.highlighting_rules = []

            if hasattr(QtCore.QRegularExpression, "PatternOption"):
                # This is the PySide6/Qt6 structure
                CaseInsensitiveOption = (
                    QtCore.QRegularExpression.PatternOption.CaseInsensitiveOption
                )
            else:
                # This is the PySide2/Qt5 structure
                CaseInsensitiveOption = QtCore.QRegularExpression.CaseInsensitiveOption

            # Keywords (case-insensitive regex)
            # Get the list of keywords from the SQL engine.
            for word in ArchSql.getSqlKeywords():
                pattern = QtCore.QRegularExpression(r"\b" + word + r"\b", CaseInsensitiveOption)
                rule = {"pattern": pattern, "format": keyword_format}
                self.highlighting_rules.append(rule)

            # Aggregate Functions (case-insensitive regex)
            functions = ["COUNT", "SUM", "MIN", "MAX"]
            for word in functions:
                pattern = QtCore.QRegularExpression(r"\b" + word + r"\b", CaseInsensitiveOption)
                rule = {"pattern": pattern, "format": function_format}
                self.highlighting_rules.append(rule)

            # String Literals (single quotes)
            # This regex captures everything between single quotes, allowing for escaped quotes
            string_pattern = QtCore.QRegularExpression(r"'[^'\\]*(\\.[^'\\]*)*'")
            self.highlighting_rules.append({"pattern": string_pattern, "format": string_format})
            # Also support double-quoted string literals (some SQL dialects use double quotes)
            double_string_pattern = QtCore.QRegularExpression(r'"[^"\\]*(\\.[^"\\]*)*"')
            self.highlighting_rules.append(
                {"pattern": double_string_pattern, "format": string_format}
            )

            # Single-line comments (starting with -- or #)
            comment_single_line_pattern = QtCore.QRegularExpression(r"--[^\n]*|\#[^\n]*")
            self.highlighting_rules.append(
                {"pattern": comment_single_line_pattern, "format": comment_format}
            )

            # Multi-line comments (/* ... */) - requires special handling in highlightBlock
            self.multi_line_comment_start_pattern = QtCore.QRegularExpression(r"/\*")
            self.multi_line_comment_end_pattern = QtCore.QRegularExpression(r"\*/")
            self.multi_line_comment_format = comment_format

        def highlightBlock(self, text):
            """
            Applies highlighting rules to the given text block.
            This method is called automatically by Qt for each visible text block.
            """
            # --- Part 1: Handle single-line rules ---
            # Iterate over all the rules defined in the constructor
            for rule in self.highlighting_rules:
                pattern = rule["pattern"]
                format = rule["format"]

                # Get an iterator for all matches
                iterator = pattern.globalMatch(text)
                while iterator.hasNext():
                    match = iterator.next()
                    # Apply the format to the matched text
                    self.setFormat(match.capturedStart(), match.capturedLength(), format)

            # --- Part 2: Handle multi-line comments (which span blocks) ---
            self.setCurrentBlockState(0)

            startIndex = 0
            # Check if the previous block was an unclosed multi-line comment
            if self.previousBlockState() != 1:
                # It wasn't, so find the start of a new comment in the current line
                match = self.multi_line_comment_start_pattern.match(text)
                startIndex = match.capturedStart() if match.hasMatch() else -1
            else:
                # The previous block was an unclosed comment, so this block starts inside a comment
                startIndex = 0

            while startIndex >= 0:
                # Find the end of the comment
                end_match = self.multi_line_comment_end_pattern.match(text, startIndex)
                commentLength = 0

                if not end_match.hasMatch():
                    # The comment doesn't end in this line, so it spans the rest of the block
                    self.setCurrentBlockState(1)
                    commentLength = len(text) - startIndex
                else:
                    # The comment ends in this line
                    commentLength = end_match.capturedEnd() - startIndex

                self.setFormat(startIndex, commentLength, self.multi_line_comment_format)

                # Look for the next multi-line comment in the same line
                next_start_index = startIndex + commentLength
                next_match = self.multi_line_comment_start_pattern.match(text, next_start_index)
                startIndex = next_match.capturedStart() if next_match.hasMatch() else -1

    class CheatsheetDialog(QtWidgets.QDialog):
        """A simple dialog to display the HTML cheatsheet."""

        def __init__(self, api_data, parent=None):
            super().__init__(parent)
            self.setWindowTitle(translate("Arch", "BIM SQL Cheatsheet"))
            self.setMinimumSize(800, 600)
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
            for category_name in sorted(api_data.get("functions", {}).keys()):
                functions = api_data["functions"][category_name]
                html += f"<b>{category_name}:</b><ul>"
                # Sort functions within a category alphabetically
                for func_data in sorted(functions, key=lambda x: x["name"]):
                    # Add a bottom margin to the list item for clear visual separation.
                    html += f"<li style='margin-bottom: 10px;'><code>{func_data['signature']}</code><br>{func_data['description']}"  # Add the example snippet if it exists
                    if func_data.get("snippet"):
                        snippet_html = func_data["snippet"].replace("\n", "<br>")
                        # No <br> before the snippet. Added styling to make the snippet stand out.
                        html += f"<pre style='margin-top: 4px; padding: 5px; background-color: #f0f0f0; border: 1px solid #ccc;'><code>{snippet_html}</code></pre></li>"
                    else:
                        html += "</li>"
                html += "</ul>"
            return html

else:
    # In headless mode, we don't need the GUI classes.
    pass
