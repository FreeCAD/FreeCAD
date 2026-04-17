# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 sliptonic <shopinthewoods@gmail.com>

################################################################################
#                                                                              #
#   FreeCAD is free software: you can redistribute it and/or modify            #
#   it under the terms of the GNU Lesser General Public License as             #
#   published by the Free Software Foundation, either version 2.1              #
#   of the License, or (at your option) any later version.                     #
#                                                                              #
#   FreeCAD is distributed in the hope that it will be useful,                 #
#   but WITHOUT ANY WARRANTY; without even the implied warranty                #
#   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                    #
#   See the GNU Lesser General Public License for more details.                #
#                                                                              #
#   You should have received a copy of the GNU Lesser General Public           #
#   License along with FreeCAD. If not, see https://www.gnu.org/licenses       #
#                                                                              #
################################################################################


"""Unified Post-Processing dialog.

Shows job information, operation selection, output settings, and sanity
warnings in a single dialog before committing to G-code generation.

Phase 1 - skeleton: populates all widgets from the job object and returns
a config dict on accept.  No functional post-processing logic lives here yet.
"""

import FreeCAD
import FreeCADGui
import Path
import Path.Preferences as PathPref
from PySide import QtCore, QtGui

translate = FreeCAD.Qt.translate

debug = True
if debug:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


_TAB_OVERVIEW = 0
_TAB_OPERATIONS = 1
_TAB_OPTIONS = 2
_TAB_GCODE = 3
_TAB_WARNINGS = 4


def _parse_cycle_time(ct_str):
    """Convert 'HH:MM:SS' string to total seconds, or None if unparseable."""
    try:
        parts = ct_str.strip().split(":")
        if len(parts) == 3:
            return int(parts[0]) * 3600 + int(parts[1]) * 60 + int(parts[2])
        if len(parts) == 2:
            return int(parts[0]) * 60 + int(parts[1])
    except Exception:
        pass
    return None


def _format_seconds(total_secs):
    """Format total seconds as 'H:MM:SS' or 'M:SS'."""
    h = total_secs // 3600
    m = (total_secs % 3600) // 60
    s = total_secs % 60
    if h:
        return f"{h}:{m:02d}:{s:02d}"
    return f"{m}:{s:02d}"


class PostProcessDialog:
    """Unified post-processing dialog.

    Parameters
    ----------
    job : FreeCAD DocumentObject
        The CAM Job to post-process.

    Usage
    -----
    dlg = PostProcessDialog(job)
    if dlg.exec_() == QtGui.QDialog.Accepted:
        config = dlg.config()
    """

    def __init__(self, job):
        self.job = job
        self.dialog = FreeCADGui.PySideUic.loadUi(":/panels/DlgPostProcess.ui")
        if self.dialog is None:
            raise RuntimeError(
                "Failed to load DlgPostProcess.ui — rebuild may be needed to update Qt resources"
            )
        # Tracks dynamically added machine-output QGroupBoxes and their widgets
        self._dynamic_output_groups = []
        self._machine_output_field_widgets = {}  # section_name -> {field_name: widget}
        # Post-processor parameter widgets (from get_property_schema)
        self._post_param_widgets = (
            {}
        )  # runtime params on Overview: {param_name: (widget, schema_entry)}
        self._post_config_widgets = (
            {}
        )  # non-runtime params on Options: {param_name: (widget, schema_entry)}
        # Stores generated G-code: {full_path_filename: gcode_string}
        self._generated_outputs = {}
        # Original (subpart, gcode) sections — used to regenerate filenames
        self._output_sections = []

        self._setup_dialog()
        self._populate()

    # ------------------------------------------------------------------
    # Setup
    # ------------------------------------------------------------------

    def _setup_dialog(self):
        dlg = self.dialog
        self._btn_generate = dlg.buttonBox.addButton(
            translate("CAM_Post", "Generate Output"),
            QtGui.QDialogButtonBox.ButtonRole.ActionRole,
        )
        self._btn_generate.setDefault(True)
        self._btn_generate.clicked.connect(self._generate_output)
        dlg.buttonSelectAllOps.clicked.connect(self._select_all_ops)
        dlg.buttonSelectNoneOps.clicked.connect(self._select_none_ops)
        dlg.buttonWorkplan.clicked.connect(self._show_workplan)
        dlg.treeWidgetOperations.itemChanged.connect(self._on_ops_changed)
        dlg.comboBoxMachine.currentIndexChanged.connect(self._on_machine_changed)
        dlg.buttonBrowseOutputLocation.clicked.connect(self._browse_output_location)
        dlg.buttonApplyTemplate.clicked.connect(self._regenerate_filenames)
        dlg.listWidgetOutputFiles.currentItemChanged.connect(self._on_output_file_selected)
        dlg.listWidgetOutputFiles.itemChanged.connect(self._on_output_file_renamed)
        dlg.listWidgetOutputFiles.setContextMenuPolicy(
            QtCore.Qt.ContextMenuPolicy.CustomContextMenu
        )
        dlg.listWidgetOutputFiles.customContextMenuRequested.connect(
            self._on_output_files_context_menu
        )
        dlg.buttonSaveOutput.clicked.connect(self._save_output_files)

    # ------------------------------------------------------------------
    # Population
    # ------------------------------------------------------------------

    def _populate(self):
        self._populate_title()
        self._populate_machine()  # also triggers _rebuild_machine_output_section
        self._populate_fixtures()
        self._populate_job_details()
        self._populate_operations()
        self._populate_warnings()  # must be last — updates tab badge

    def _populate_title(self):
        self.dialog.labelJobTitle.setText(
            translate("CAM_Post", "Post Processing - Job: {}").format(self.job.Label)
        )

    def _populate_machine(self):
        dlg = self.dialog
        dlg.comboBoxMachine.blockSignals(True)
        dlg.comboBoxMachine.clear()
        dlg.comboBoxMachine.addItem(translate("CAM_Post", "(none)"), userData=None)

        try:
            from Machine.models.machine import MachineFactory

            # User-saved machines
            for name, filename in MachineFactory.list_configuration_files():
                if filename:
                    full_path = str(MachineFactory.get_config_directory() / filename)
                    dlg.comboBoxMachine.addItem(name, userData=full_path)

        except Exception as e:
            Path.Log.warning(f"Could not enumerate machines: {e}")

        current = getattr(self.job, "Machine", None)
        if current:
            idx = dlg.comboBoxMachine.findText(current)
            if idx >= 0:
                dlg.comboBoxMachine.setCurrentIndex(idx)

        dlg.comboBoxMachine.blockSignals(False)
        self._rebuild_machine_output_section()

    def _on_machine_changed(self, _index):
        self._rebuild_machine_output_section()

    def _rebuild_machine_output_section(self):
        """Clear and rebuild the dynamic machine-output option groups in the Output tab.

        Uses the shared ``build_output_options`` helper so the layout stays
        consistent with the Machine Editor dialog.
        """
        dlg = self.dialog
        scroll_layout = dlg.scrollContentsOutput.layout()

        # Remove previously added dynamic groups
        for group in self._dynamic_output_groups:
            scroll_layout.removeWidget(group)
            group.deleteLater()
        self._dynamic_output_groups.clear()
        self._machine_output_field_widgets.clear()

        machine_path = dlg.comboBoxMachine.currentData()

        if not machine_path:
            dlg.labelNoMachineOutput.setVisible(True)
            self._rebuild_post_params_section(None)
            return

        dlg.labelNoMachineOutput.setVisible(False)

        machine = None
        try:
            from Machine.models.machine import MachineFactory

            machine = MachineFactory.load_configuration(machine_path)
        except Exception as e:
            Path.Log.warning(f"Could not load machine for output options: {e}")
            self._rebuild_post_params_section(None)
            return

        try:
            from Machine.ui.editor.output_options_layout import build_output_options
        except Exception as e:
            Path.Log.warning(f"Could not import build_output_options: {e}")
            self._rebuild_post_params_section(machine)
            return

        # Build all output + processing widgets into a temporary container,
        # then transplant each top-level group into the scroll layout.
        container = QtGui.QWidget()
        container_layout = QtGui.QVBoxLayout(container)
        container_layout.setContentsMargins(0, 0, 0, 0)

        section_widgets = build_output_options(machine, container_layout, context="CAM_Post")

        # Flatten section_widgets into _machine_output_field_widgets
        self._machine_output_field_widgets = dict(section_widgets)

        # Move the generated group boxes from the container into the real
        # scroll layout (inserted before the trailing spacer).
        insert_idx = max(0, scroll_layout.count() - 1)
        while container_layout.count():
            item = container_layout.takeAt(0)
            widget = item.widget()
            if widget is not None:
                scroll_layout.insertWidget(insert_idx, widget)
                self._dynamic_output_groups.append(widget)
                insert_idx += 1

        container.deleteLater()

        # Populate runtime post-processor params on Overview tab
        self._rebuild_post_params_section(machine)

    def _rebuild_post_config_section(self, machine, scroll_layout, insert_idx):
        """Build non-runtime postprocessor property widgets on the Options tab.

        These mirror the same properties shown in the machine editor and allow
        per-run overrides of machine-config values like pierce_delay, cooling_delay, etc.
        """
        # Clear previous non-runtime config widgets
        for name, (widget, _schema) in self._post_config_widgets.items():
            # The widget is inside a group box tracked by _dynamic_output_groups
            pass
        self._post_config_widgets.clear()

        if machine is None:
            return

        # Resolve postprocessor class
        post_class = None
        postprocessor_name = getattr(machine, "postprocessor_file_name", None)
        if postprocessor_name:
            try:
                from Path.Post.Processor import PostProcessorFactory

                post_obj = PostProcessorFactory.get_post_processor(self.job, postprocessor_name)
                if post_obj is not None:
                    post_class = type(post_obj)
            except Exception as e:
                Path.Log.debug(f"Could not resolve postprocessor class for config: {e}")

        if post_class is None:
            return

        try:
            schema = post_class.get_property_schema()
        except Exception:
            return

        non_runtime = [e for e in schema if not e.get("runtime", False)] if schema else []
        if not non_runtime:
            return

        # Build the configuration bundle to get values with job overrides applied
        bundle = {}
        if post_obj is not None and hasattr(post_obj, "build_configuration_bundle"):
            bundle = post_obj.build_configuration_bundle()
            Path.Log.debug(f"Post config bundle for dialog: {bundle}")

        pp_props = bundle if bundle else (getattr(machine, "postprocessor_properties", {}) or {})

        group = QtGui.QGroupBox(translate("CAM_Post", "Postprocessor Properties"))
        form = QtGui.QFormLayout(group)

        for entry in non_runtime:
            name = entry.get("name", "")
            param_type = entry.get("type", "string")
            label_text = entry.get("label", name)
            default = entry.get("default")
            help_text = entry.get("help", "")
            current = pp_props.get(name, default)

            widget = None
            if param_type == "bool":
                widget = QtGui.QCheckBox()
                widget.setChecked(bool(current))
                widget.setToolTip(help_text)
                widget.value_getter = lambda w=widget: w.isChecked()
            elif param_type in ("integer", "int"):
                widget = QtGui.QSpinBox()
                widget.setMinimum(entry.get("min", 0))
                widget.setMaximum(entry.get("max", 99999))
                widget.setValue(int(current) if current is not None else 0)
                widget.setToolTip(help_text)
                widget.value_getter = lambda w=widget: w.value()
            elif param_type == "float":
                widget = QtGui.QDoubleSpinBox()
                widget.setMinimum(entry.get("min", 0.0))
                widget.setMaximum(entry.get("max", 99999.0))
                widget.setDecimals(entry.get("decimals", 3))
                widget.setValue(float(current) if current is not None else 0.0)
                widget.setToolTip(help_text)
                widget.value_getter = lambda w=widget: w.value()
            elif param_type == "choice":
                widget = QtGui.QComboBox()
                for choice in entry.get("choices", []):
                    if isinstance(choice, tuple):
                        widget.addItem(str(choice[0]), choice[1])
                    else:
                        widget.addItem(str(choice), choice)
                if current is not None:
                    idx = widget.findData(current)
                    if idx >= 0:
                        widget.setCurrentIndex(idx)
                widget.setToolTip(help_text)
                widget.value_getter = lambda w=widget: w.currentData()
            elif param_type == "text":
                widget = QtGui.QPlainTextEdit()
                widget.setPlainText(str(current) if current else "")
                widget.setMaximumHeight(80)
                widget.setToolTip(help_text)
                widget.value_getter = lambda w=widget: w.toPlainText()
            else:
                widget = QtGui.QLineEdit()
                widget.setText(str(current) if current else "")
                widget.setToolTip(help_text)
                widget.value_getter = lambda w=widget: w.text()

            # if widget is not None:
            #     # Connect signal to recompute warnings when value changes
            #     if isinstance(widget, (QtGui.QCheckBox, QtGui.QLineEdit, QtGui.QPlainTextEdit)):
            #         (
            #             widget.textChanged.connect(self._recompute_warnings)
            #             if hasattr(widget, "textChanged")
            #             else widget.stateChanged.connect(self._recompute_warnings)
            #         )
            #     elif isinstance(widget, (QtGui.QSpinBox, QtGui.QDoubleSpinBox)):
            #         widget.valueChanged.connect(self._recompute_warnings)
            #     elif isinstance(widget, QtGui.QComboBox):
            #         widget.currentIndexChanged.connect(self._recompute_warnings)

            #     form.addRow(label_text, widget)
            #     self._post_config_widgets[name] = (widget, entry)

        scroll_layout.insertWidget(insert_idx, group)
        self._dynamic_output_groups.append(group)

    def _rebuild_post_params_section(self, machine=None):
        """Populate groupBoxPostParams with widgets from the postprocessor's get_property_schema().

        Resolves the postprocessor class from the machine config, calls
        get_property_schema(), and creates appropriate widgets for each entry.
        Machine-config values override schema defaults when available.
        """
        dlg = self.dialog
        layout = dlg.layoutPostParams
        placeholder = dlg.labelPostParamsPlaceholder

        # Clear previously created post-param widgets
        for name, (widget, _schema) in self._post_param_widgets.items():
            # QFormLayout.labelForField returns the label widget for a given field
            label_widget = layout.labelForField(widget)
            if label_widget:
                layout.removeWidget(label_widget)
                label_widget.deleteLater()
            layout.removeWidget(widget)
            widget.deleteLater()
        self._post_param_widgets.clear()

        # Resolve postprocessor class from machine
        post_class = None
        if machine is not None:
            postprocessor_name = getattr(machine, "postprocessor_file_name", None)
            if postprocessor_name:
                try:
                    from Path.Post.Processor import PostProcessorFactory

                    post_obj = PostProcessorFactory.get_post_processor(self.job, postprocessor_name)
                    if post_obj is not None:
                        post_class = type(post_obj)
                except Exception as e:
                    Path.Log.debug(f"Could not resolve postprocessor class: {e}")

        if post_class is None:
            placeholder.setVisible(True)
            return

        # Get the property schema
        try:
            schema = post_class.get_property_schema()
        except Exception as e:
            Path.Log.warning(f"Could not get property schema: {e}")
            placeholder.setVisible(True)
            return

        # Only runtime parameters are shown on the Overview tab
        runtime_schema = [e for e in schema if e.get("runtime", False)] if schema else []

        if not runtime_schema:
            placeholder.setVisible(True)
            return

        placeholder.setVisible(False)

        # Get current machine postprocessor_properties for initial values
        pp_props = getattr(machine, "postprocessor_properties", {}) or {}

        for entry in runtime_schema:

            name = entry.get("name", "")
            param_type = entry.get("type", "string")
            label = entry.get("label", name)
            default = entry.get("default")
            help_text = entry.get("help", "")

            # Current value: machine config overrides schema default
            current = pp_props.get(name, default)

            widget = None
            if param_type == "bool":
                widget = QtGui.QCheckBox()
                widget.setChecked(bool(current))
                widget.setToolTip(help_text)
                widget.value_getter = lambda w=widget: w.isChecked()

            elif param_type in ("integer", "int"):
                widget = QtGui.QSpinBox()
                widget.setMinimum(entry.get("min", 0))
                widget.setMaximum(entry.get("max", 99999))
                widget.setValue(int(current) if current is not None else 0)
                widget.setToolTip(help_text)
                widget.value_getter = lambda w=widget: w.value()

            elif param_type == "float":
                widget = QtGui.QDoubleSpinBox()
                widget.setMinimum(entry.get("min", 0.0))
                widget.setMaximum(entry.get("max", 99999.0))
                widget.setDecimals(entry.get("decimals", 3))
                widget.setValue(float(current) if current is not None else 0.0)
                widget.setToolTip(help_text)
                widget.value_getter = lambda w=widget: w.value()

            elif param_type == "choice":
                widget = QtGui.QComboBox()
                for choice in entry.get("choices", []):
                    if isinstance(choice, tuple):
                        widget.addItem(str(choice[0]), choice[1])
                    else:
                        widget.addItem(str(choice), choice)
                if current is not None:
                    idx = widget.findData(current)
                    if idx >= 0:
                        widget.setCurrentIndex(idx)
                widget.setToolTip(help_text)
                widget.value_getter = lambda w=widget: w.currentData()

            elif param_type == "text":
                widget = QtGui.QPlainTextEdit()
                widget.setPlainText(str(current) if current else "")
                widget.setMaximumHeight(80)
                widget.setToolTip(help_text)
                widget.value_getter = lambda w=widget: w.toPlainText()

            else:
                # Default: string / unknown type -> QLineEdit
                widget = QtGui.QLineEdit()
                widget.setText(str(current) if current else "")
                widget.setToolTip(help_text)
                widget.value_getter = lambda w=widget: w.text()

            if widget is not None:
                layout.addRow(label, widget)
                self._post_param_widgets[name] = (widget, entry)

    def _collect_post_param_values(self):
        """Read current values from all post-parameter widgets (runtime + non-runtime).

        Returns:
            dict: {param_name: value} for all post-processor parameters.
        """
        values = {}
        # Non-runtime config from Options tab
        for name, (widget, _schema) in self._post_config_widgets.items():
            if hasattr(widget, "value_getter"):
                values[name] = widget.value_getter()
        # Runtime params from Overview tab (override if same key exists)
        for name, (widget, _schema) in self._post_param_widgets.items():
            if hasattr(widget, "value_getter"):
                values[name] = widget.value_getter()
        return values

    def _populate_fixtures(self):
        dlg = self.dialog
        lw = dlg.listWidgetFixtures
        lw.clear()

        fixtures = getattr(self.job, "Fixtures", None) or []
        if not fixtures:
            placeholder = QtGui.QListWidgetItem(translate("CAM_Post", "(no fixtures defined)"))
            placeholder.setFlags(QtCore.Qt.ItemFlag.ItemIsEnabled)
            lw.addItem(placeholder)
            return

        for fix in fixtures:
            label = fix.Label if hasattr(fix, "Label") else str(fix)
            item = QtGui.QListWidgetItem(label)
            item.setFlags(
                QtCore.Qt.ItemFlag.ItemIsEnabled
                | QtCore.Qt.ItemFlag.ItemIsUserCheckable
                | QtCore.Qt.ItemFlag.ItemIsSelectable
            )
            item.setCheckState(QtCore.Qt.CheckState.Checked)
            lw.addItem(item)

    def _populate_job_details(self):
        dlg = self.dialog
        dlg.lineEditJobAuthor.setText(getattr(self.job.Document, "CreatedBy", "") or "")
        dlg.plainTextEditComment.setPlainText(getattr(self.job, "Description", "") or "")

    def _populate_operations(self):
        dlg = self.dialog
        tree = dlg.treeWidgetOperations
        tree.blockSignals(True)
        tree.clear()
        tree.setHeaderHidden(True)

        for op in self._get_active_operations():
            item = QtGui.QTreeWidgetItem(tree)
            item.setText(0, op.Label)
            item.setCheckState(0, QtCore.Qt.CheckState.Checked)
            item.setFlags(
                item.flags()
                | QtCore.Qt.ItemFlag.ItemIsUserCheckable
                | QtCore.Qt.ItemFlag.ItemIsEnabled
            )

        tree.resizeColumnToContents(0)
        tree.blockSignals(False)
        self._update_ops_tab_label()

    def _get_dialog_overrides(self):
        """Collect current dialog widget values as an overrides dict.

        Delegates to _collect_post_param_values() so both sanity checks
        and output generation use exactly the same values.

        Returns:
            dict: Property overrides from all postprocessor config widgets.
        """
        return self._collect_post_param_values()

    def _recompute_warnings(self):
        """Recompute warnings using current dialog values instead of saved job state."""
        # Show temporary feedback
        dlg = self.dialog
        original_text = dlg.labelWarningsStatus.text()
        dlg.labelWarningsStatus.setText("Recomputing...")
        dlg.labelWarningsStatus.setStyleSheet("color: blue; font-weight: bold;")

        # Process events to update UI
        QtGui.QApplication.processEvents()

        # Recompute warnings
        self._populate_warnings(use_dialog_values=True)

        # Brief delay to show the recomputation happened
        QtCore.QTimer.singleShot(200, lambda: None)

    def _populate_warnings(self, use_dialog_values=False):
        dlg = self.dialog
        try:
            from Path.Main.Sanity.Sanity import CAMSanity

            overrides = self._get_dialog_overrides() if use_dialog_values else None
            all_squawks, critical_squawks = CAMSanity.validate_job(self.job, overrides=overrides)
        except Exception as e:
            Path.Log.warning(f"Sanity check failed: {e}")
            all_squawks = []
            critical_squawks = []

        table = dlg.tableWidgetWarnings

        if not all_squawks:
            dlg.labelWarningsStatus.setText(translate("CAM_Post", "No issues found."))
            dlg.labelWarningsStatus.setStyleSheet("color: green; font-weight: bold;")
            table.setVisible(False)
            self._update_warnings_tab_label(0, 0)
            return

        table.setVisible(True)
        table.setRowCount(0)

        severity_colors = {
            "WARNING": QtGui.QColor(200, 40, 40),
            "CAUTION": QtGui.QColor(200, 110, 0),
            "NOTE": QtGui.QColor(30, 100, 200),
            "TIP": QtGui.QColor(110, 110, 110),
        }

        for sq in all_squawks:
            row = table.rowCount()
            table.insertRow(row)
            sev = sq.get("squawkType", "NOTE")
            table.setItem(row, 0, QtGui.QTableWidgetItem(sev))
            table.setItem(row, 1, QtGui.QTableWidgetItem(sq.get("Note", "")))
            color = severity_colors.get(sev, QtGui.QColor(110, 110, 110))
            for col in range(2):
                table.item(row, col).setForeground(color)

        table.resizeColumnToContents(0)

        n_critical = len(critical_squawks)
        n_total = len(all_squawks)

        if n_critical:
            dlg.labelWarningsStatus.setText(
                translate("CAM_Post", "{} critical issue(s) — review before machining.").format(
                    n_critical
                )
            )
            dlg.labelWarningsStatus.setStyleSheet("color: red; font-weight: bold;")
        else:
            dlg.labelWarningsStatus.setText(
                translate("CAM_Post", "{} advisory notice(s) found.").format(n_total)
            )
            dlg.labelWarningsStatus.setStyleSheet("color: darkorange; font-weight: bold;")

        self._update_warnings_tab_label(n_total, n_critical)

    # ------------------------------------------------------------------
    # Tab label badges
    # ------------------------------------------------------------------

    def _update_ops_tab_label(self):
        tree = self.dialog.treeWidgetOperations
        total = tree.topLevelItemCount()
        checked = sum(
            1
            for i in range(total)
            if tree.topLevelItem(i).checkState(0) == QtCore.Qt.CheckState.Checked
        )
        self.dialog.tabWidget.setTabText(
            _TAB_OPERATIONS,
            translate("CAM_Post", "Operations ({}/{})").format(checked, total),
        )

    def _update_warnings_tab_label(self, n_total, n_critical):
        tab = self.dialog.tabWidget
        if n_critical:
            tab.setTabText(_TAB_WARNINGS, translate("CAM_Post", "Warnings (!) {}").format(n_total))
            tab.tabBar().setTabTextColor(_TAB_WARNINGS, QtGui.QColor("red"))
        elif n_total:
            tab.setTabText(_TAB_WARNINGS, translate("CAM_Post", "Warnings {}").format(n_total))
            tab.tabBar().setTabTextColor(_TAB_WARNINGS, QtGui.QColor("darkorange"))
        else:
            tab.setTabText(_TAB_WARNINGS, translate("CAM_Post", "Warnings"))
            tab.tabBar().setTabTextColor(_TAB_WARNINGS, QtGui.QColor("green"))

    # ------------------------------------------------------------------
    # Operations helpers
    # ------------------------------------------------------------------

    def _get_operations(self):
        ops = getattr(self.job, "Operations", None)
        if ops and hasattr(ops, "Group"):
            return ops.Group
        return []

    def _get_active_operations(self):
        """Return only active operations, matching what the tree widget displays."""
        return [op for op in self._get_operations() if getattr(op, "Active", True)]

    def _on_ops_changed(self, _item, _col=None):
        self._update_ops_tab_label()

    def _select_all_ops(self):
        tree = self.dialog.treeWidgetOperations
        tree.blockSignals(True)
        for i in range(tree.topLevelItemCount()):
            tree.topLevelItem(i).setCheckState(0, QtCore.Qt.CheckState.Checked)
        tree.blockSignals(False)
        self._on_ops_changed(None)

    def _select_none_ops(self):
        tree = self.dialog.treeWidgetOperations
        tree.blockSignals(True)
        for i in range(tree.topLevelItemCount()):
            tree.topLevelItem(i).setCheckState(0, QtCore.Qt.CheckState.Unchecked)
        tree.blockSignals(False)
        self._on_ops_changed(None)

    def _show_workplan(self):
        """Display the workplan (postable items structure) in a dialog."""
        from Path.Post.PostList import buildPostList
        import Path.Base.Util as PathUtil

        # Create a minimal processor-like object for building the postlist
        class TempProcessor:
            def __init__(self, job, operations):
                self._job = job
                self._operations = operations

        # Create temporary processor with active operations
        temp_processor = TempProcessor(self.job, self._get_active_operations())

        # Build the workplan text
        workplan_text = self._format_workplan(temp_processor)

        # Create and show the dialog
        dlg = QtGui.QDialog(self.dialog)
        dlg.setWindowTitle(translate("CAM_Post", "Workplan"))
        dlg.resize(800, 600)

        layout = QtGui.QVBoxLayout(dlg)

        # Add text display
        text_edit = QtGui.QTextEdit()
        text_edit.setPlainText(workplan_text)
        text_edit.setReadOnly(True)
        text_edit.setFont(QtGui.QFont("Courier", 9))
        layout.addWidget(text_edit)

        # Add close button
        button_box = QtGui.QDialogButtonBox(QtGui.QDialogButtonBox.Close)
        button_box.rejected.connect(dlg.reject)
        layout.addWidget(button_box)

        dlg.exec_()

    def _format_workplan(self, processor):
        """Format the workplan text with per-item cycle times and a grand total."""
        from Path.Post.PostList import buildPostList

        lines = []
        lines.append("=" * 80)
        lines.append("WORKPLAN")
        lines.append("=" * 80)
        lines.append("")
        lines.append(f"Job: {processor._job.Label}")
        lines.append(f"SplitOutput: {getattr(processor._job, 'SplitOutput', 'N/A')}")
        lines.append(f"OrderOutputBy: {getattr(processor._job, 'OrderOutputBy', 'N/A')}")
        lines.append(f"Fixtures: {getattr(processor._job, 'Fixtures', 'N/A')}")
        lines.append("")

        postables = buildPostList(processor)
        total_secs = 0
        has_any_time = False

        for idx, postable in enumerate(postables, 1):
            group_key = postable[0]
            objects = postable[1]

            # Format the group key display
            if group_key == "":
                display_key = "(empty string)"
            else:
                display_key = f'"{group_key}"'

            group_secs = 0
            group_has_time = False

            lines.append(f"[{idx}] Postable Group: {display_key}")
            lines.append(f"    Objects: {len(objects)}")
            lines.append("")

            for obj_idx, obj in enumerate(objects, 1):
                lines.append(f"    [{obj_idx}] {obj.Label}")

                # Determine object type/role and extract cycle time
                obj_type = None
                cycle_time_str = None
                if hasattr(obj, "item_type"):
                    # Postable object
                    obj_type = obj.item_type.title()
                    if obj.item_type == "fixture":
                        if hasattr(obj, "path") and obj.path and len(obj.path.Commands) > 0:
                            fixture_cmd = obj.path.Commands[0]
                            lines.append(f"        Fixture: {fixture_cmd.Name}")
                    elif obj.item_type == "tool_controller":
                        if hasattr(obj, "data") and "tool_number" in obj.data:
                            lines.append(f"        Tool Number: {obj.data['tool_number']}")
                        lines.append(f"        Tool: {obj.Label}")
                    elif obj.item_type == "operation":
                        if hasattr(obj, "data") and "tool_controller" in obj.data:
                            tc = obj.data["tool_controller"]
                            lines.append(
                                f"        ToolController: {tc.Label} (T{tc.data.get('tool_number', '?')})"
                            )
                        else:
                            lines.append("        ToolController: None")
                        # Cycle time from postable operation data
                        src = getattr(obj, "source", None)
                        if src is not None:
                            cycle_time_str = getattr(src, "CycleTime", None)
                        if cycle_time_str is None and hasattr(obj, "data"):
                            cycle_time_str = obj.data.get("cycle_time")
                else:
                    # Legacy object
                    if type(obj).__name__ == "_TempObject":
                        obj_type = "Fixture Setup"
                        if hasattr(obj, "Path") and obj.Path and len(obj.Path.Commands) > 0:
                            fixture_cmd = obj.Path.Commands[0]
                            lines.append(f"        Fixture: {fixture_cmd.Name}")
                    elif hasattr(obj, "TypeId"):
                        # Check if it's a tool controller
                        if "ToolController" in obj.TypeId or obj.Name.startswith("TC"):
                            obj_type = "Tool Change"
                            if hasattr(obj, "ToolNumber"):
                                lines.append(f"        Tool Number: {obj.ToolNumber}")
                            if hasattr(obj, "Label"):
                                lines.append(f"        Tool: {obj.Label}")
                        else:
                            # It's an operation
                            obj_type = "Operation"
                            if hasattr(obj, "ToolController") and obj.ToolController:
                                tc = obj.ToolController
                                lines.append(
                                    f"        ToolController: {tc.Label} (T{tc.ToolNumber})"
                                )
                            elif hasattr(obj, "ToolController"):
                                lines.append("        ToolController: None")
                            cycle_time_str = getattr(obj, "CycleTime", None)
                    else:
                        obj_type = type(obj).__name__

                if obj_type:
                    lines.append(f"        Type: {obj_type}")

                # Show cycle time for this item
                if cycle_time_str:
                    lines.append(f"        Cycle Time: {cycle_time_str}")
                    secs = _parse_cycle_time(cycle_time_str)
                    if secs is not None:
                        group_secs += secs
                        group_has_time = True

            if group_has_time:
                total_secs += group_secs
                has_any_time = True
                lines.append(f"    Group Time: {_format_seconds(group_secs)}")

            lines.append("")

        lines.append("=" * 80)
        lines.append(f"Total Groups: {len(postables)}")
        total_objects = sum(len(p[1]) for p in postables)
        lines.append(f"Total Objects: {total_objects}")
        if has_any_time:
            lines.append(f"Total Estimated Time: {_format_seconds(total_secs)}")
        else:
            lines.append("Total Estimated Time: -")
        lines.append("=" * 80)

        return "\n".join(lines)

    # ------------------------------------------------------------------
    # G-code output generation / preview / save
    # ------------------------------------------------------------------

    def _apply_options_to_machine(self, machine):
        """Write Options-tab widget values back to the correct machine attributes.

        _merge_machine_config() inside export2() reads from machine.output.* to
        populate self.values (e.g. OUTPUT_HEADER, OUTPUT_COMMENTS, etc.).
        This method must be called after postprocessor._machine is set but before
        export2() so that any changes the user made in the Options tab take effect.
        """
        try:
            from Machine.models.machine import OutputUnits
        except Exception:
            OutputUnits = None

        for section, widgets in self._machine_output_field_widgets.items():
            if section == "main":
                for field_name, widget in widgets.items():
                    if not hasattr(widget, "value_getter"):
                        continue
                    value = widget.value_getter()
                    if field_name == "units":
                        if OutputUnits is not None:
                            try:
                                machine.output.units = OutputUnits(value)
                            except (ValueError, KeyError):
                                pass
                    elif field_name == "output_header":
                        # _merge_machine_config() reads header.include_date as the
                        # OUTPUT_HEADER master switch, so mirror the value there too.
                        machine.output.output_header = value
                        if hasattr(machine.output, "header") and hasattr(
                            machine.output.header, "include_date"
                        ):
                            machine.output.header.include_date = value
                    elif hasattr(machine.output, field_name):
                        setattr(machine.output, field_name, value)

            elif section in ("header", "comments", "formatting", "precision", "duplicates"):
                sub = getattr(machine.output, section, None)
                if sub is None:
                    continue
                for field_name, widget in widgets.items():
                    if hasattr(widget, "value_getter") and hasattr(sub, field_name):
                        setattr(sub, field_name, widget.value_getter())

            elif section == "processing":
                sub = getattr(machine, "processing", None)
                if sub is None:
                    continue
                for field_name, widget in widgets.items():
                    if hasattr(widget, "value_getter") and hasattr(sub, field_name):
                        setattr(sub, field_name, widget.value_getter())

    def _reset_output_tab(self):
        """Clear all generated output and reset the Output tab to its initial state."""
        self._generated_outputs = {}
        self._output_sections = []
        dlg = self.dialog
        dlg.listWidgetOutputFiles.blockSignals(True)
        dlg.listWidgetOutputFiles.clear()
        dlg.listWidgetOutputFiles.blockSignals(False)
        dlg.plainTextEditGcode.setPlainText("")
        dlg.labelOutputStatus.setVisible(True)
        dlg.buttonSaveOutput.setEnabled(False)
        dlg.buttonApplyTemplate.setEnabled(False)

    def _generate_output(self):
        """Run the post-processor in-memory and show the result in the Output tab."""
        import os
        from Path.Post.Processor import PostProcessorFactory
        from Path.Post.Utils import FilenameGenerator
        from Machine.models.machine import MachineFactory

        self._reset_output_tab()

        job = self.job
        dlg = self.dialog

        # Build filtered operations list from the Operations tab checkboxes.
        # Use _get_active_operations() because the tree skips inactive ops,
        # so tree indices align with the active-only list, not the full list.
        active_ops = self._get_active_operations()
        tree = dlg.treeWidgetOperations
        selected_ops = [
            active_ops[i]
            for i in range(tree.topLevelItemCount())
            if tree.topLevelItem(i).checkState(0) == QtCore.Qt.CheckState.Checked
            and i < len(active_ops)
        ]
        job_arg = {"job": job, "operations": selected_ops}

        try:
            # Determine which machine and post-processor to use.
            # Priority: machine selected in the dialog combo > job.Machine > legacy post-processor.
            combo_machine_path = dlg.comboBoxMachine.currentData()
            loaded_machine = None
            use_new_flow = False

            if combo_machine_path:
                # Load machine directly from the combo's stored file path
                from Machine.models.machine import Machine

                machine_data = MachineFactory.load_configuration(combo_machine_path)
                if isinstance(machine_data, dict):
                    loaded_machine = Machine.from_dict(machine_data)
                else:
                    loaded_machine = machine_data
                postprocessor_name = getattr(loaded_machine, "postprocessor_file_name", None)
                if not postprocessor_name:
                    QtGui.QMessageBox.warning(
                        self.dialog,
                        translate("CAM_Post", "Generate Output"),
                        translate(
                            "CAM_Post", "The selected machine has no post-processor configured."
                        ),
                    )
                    return
                use_new_flow = True
            elif hasattr(job, "Machine") and job.Machine:
                loaded_machine = MachineFactory.get_machine(job.Machine)
                postprocessor_name = getattr(loaded_machine, "postprocessor_file_name", None)
                if not postprocessor_name:
                    QtGui.QMessageBox.warning(
                        self.dialog,
                        translate("CAM_Post", "Generate Output"),
                        translate(
                            "CAM_Post", "The selected machine has no post-processor configured."
                        ),
                    )
                    return
                use_new_flow = True
            else:
                postprocessor_name = (
                    getattr(job, "PostProcessor", None) or Path.Preferences.defaultPostProcessor()
                )
                if not postprocessor_name:
                    QtGui.QMessageBox.warning(
                        self.dialog,
                        translate("CAM_Post", "Generate Output"),
                        translate("CAM_Post", "No post-processor configured for this job."),
                    )
                    return

            postprocessor = PostProcessorFactory.get_post_processor(job_arg, postprocessor_name)

            if postprocessor is None:
                QtGui.QMessageBox.warning(
                    self.dialog,
                    translate("CAM_Post", "Generate Output"),
                    translate(
                        "CAM_Post",
                        "Could not load post-processor '{}'.".format(postprocessor_name),
                    ),
                )
                return

            # Override the postprocessor's machine with the dialog's selection, then
            # write back any changes the user made in the Options tab before export2()
            # calls _merge_machine_config().
            if loaded_machine is not None:
                postprocessor._machine = loaded_machine
                self._apply_options_to_machine(postprocessor._machine)

            # Build the configuration bundle using dialog widget values as
            # overrides.  This gives dialog values highest priority (above
            # job-level overrides) and writes them into self.values so that
            # export2 and sanity checks all read from the same source.
            dialog_overrides = self._collect_post_param_values()

            # Add job_author, comment, and selected_fixtures from dialog fields
            dialog_config = self.config()
            dialog_overrides["job_author"] = dialog_config["job_author"]
            dialog_overrides["comment"] = dialog_config["comment"]
            dialog_overrides["selected_fixtures"] = dialog_config["selected_fixtures"]

            if hasattr(postprocessor, "apply_configuration_bundle"):
                postprocessor.apply_configuration_bundle(overrides=dialog_overrides)

            # Signal that the unified dialog handled user interaction so
            # pre_processing_dialog() is a no-op during export2().
            postprocessor._dialog_handled = True
            postprocessor._bundle_applied = True

            post_data = postprocessor.export2() if use_new_flow else postprocessor.export()

            if not post_data:
                QtGui.QMessageBox.warning(
                    self.dialog,
                    translate("CAM_Post", "Generate Output"),
                    translate("CAM_Post", "Post-processor returned no output."),
                )
                return

            file_ext = postprocessor.get_file_extension() if use_new_flow else None

            # Populate template field from job if user hasn't set one yet
            if not dlg.lineEditFilenameTemplate.text().strip():
                default_template = getattr(job, "PostProcessorOutputFile", "") or ""
                dlg.lineEditFilenameTemplate.setText(default_template)

            generator = FilenameGenerator(job=job, file_extension=file_ext)
            gen_filenames = generator.generate_filenames()

            # Use output folder from the Output tab if already set by user
            output_folder = dlg.lineEditOutputLocation.text().strip() or None

            self._output_sections = list(post_data)
            self._generated_outputs = {}
            for subpart, gcode in self._output_sections:
                subpart_clean = "" if subpart == "allitems" else subpart
                generator.set_subpartname(subpart_clean)
                fname = next(gen_filenames)
                if output_folder:
                    fname = os.path.join(output_folder, os.path.basename(fname))
                if gcode is not None:
                    self._generated_outputs[fname] = gcode

            # Show the resolved output directory in the location field (if not already set)
            if self._generated_outputs and not output_folder:
                first_fname = next(iter(self._generated_outputs))
                resolved_dir = os.path.dirname(first_fname) or os.getcwd()
                dlg.lineEditOutputLocation.setText(resolved_dir)

        except Exception as e:
            QtGui.QMessageBox.critical(
                self.dialog,
                translate("CAM_Post", "Generate Output"),
                translate("CAM_Post", "Error during generation:\n{}").format(str(e)),
            )
            Path.Log.error(f"Generate output failed: {e}")
            return

        self._populate_output_tab()
        self.dialog.tabWidget.setCurrentIndex(_TAB_GCODE)

    def _populate_output_tab(self):
        """Fill the Output tab with the generated file list and first file's content."""
        dlg = self.dialog
        lw = dlg.listWidgetOutputFiles
        lw.blockSignals(True)
        lw.clear()

        for fname in self._generated_outputs:
            import os

            item = QtGui.QListWidgetItem(os.path.basename(fname))
            item.setData(QtCore.Qt.ItemDataRole.UserRole, fname)
            item.setToolTip(fname)
            item.setFlags(
                item.flags()
                | QtCore.Qt.ItemFlag.ItemIsEditable
                | QtCore.Qt.ItemFlag.ItemIsEnabled
                | QtCore.Qt.ItemFlag.ItemIsSelectable
            )
            lw.addItem(item)

        lw.blockSignals(False)

        if lw.count() > 0:
            lw.setCurrentRow(0)  # triggers _on_output_file_selected
        else:
            dlg.plainTextEditGcode.setPlainText("")

        n = len(self._generated_outputs)
        dlg.labelOutputStatus.setVisible(n == 0)
        dlg.buttonSaveOutput.setEnabled(n > 0)
        dlg.buttonApplyTemplate.setEnabled(n > 0)

    def _on_output_file_selected(self, current, previous):
        """Persist edits from the previously viewed file; load the newly selected one."""
        dlg = self.dialog
        if previous is not None:
            prev_fname = previous.data(QtCore.Qt.ItemDataRole.UserRole)
            if prev_fname in self._generated_outputs:
                self._generated_outputs[prev_fname] = dlg.plainTextEditGcode.toPlainText()

        if current is not None:
            fname = current.data(QtCore.Qt.ItemDataRole.UserRole)
            dlg.plainTextEditGcode.setPlainText(self._generated_outputs.get(fname, ""))

    def _on_output_files_context_menu(self, pos):
        """Show rename context menu on right-click."""
        lw = self.dialog.listWidgetOutputFiles
        item = lw.itemAt(pos)
        if item is None:
            return
        menu = QtGui.QMenu(self.dialog)
        rename_action = menu.addAction(translate("CAM_Post", "Rename"))
        if menu.exec_(lw.mapToGlobal(pos)) == rename_action:
            lw.editItem(item)

    def _on_output_file_renamed(self, item):
        """Update _generated_outputs key when a file item is renamed by the user."""
        import os

        new_basename = item.text()
        old_fname = item.data(QtCore.Qt.ItemDataRole.UserRole)
        if not old_fname:
            return
        if new_basename == os.path.basename(old_fname):
            return  # Nothing changed

        old_dir = os.path.dirname(old_fname)
        new_fname = os.path.join(old_dir, new_basename) if old_dir else new_basename

        if old_fname in self._generated_outputs:
            gcode = self._generated_outputs.pop(old_fname)
            self._generated_outputs[new_fname] = gcode

        lw = self.dialog.listWidgetOutputFiles
        lw.blockSignals(True)
        item.setData(QtCore.Qt.ItemDataRole.UserRole, new_fname)
        item.setToolTip(new_fname)
        lw.blockSignals(False)

    def _flush_editor_to_outputs(self):
        """Save any pending edits from the editor back to _generated_outputs."""
        dlg = self.dialog
        current = dlg.listWidgetOutputFiles.currentItem()
        if current is not None:
            fname = current.data(QtCore.Qt.ItemDataRole.UserRole)
            if fname in self._generated_outputs:
                self._generated_outputs[fname] = dlg.plainTextEditGcode.toPlainText()

    def _regenerate_filenames(self):
        """Re-apply the filename template to rename all items in the output list."""
        import os
        from Path.Post.Utils import FilenameGenerator

        if not self._output_sections:
            return

        self._flush_editor_to_outputs()

        dlg = self.dialog
        template = dlg.lineEditFilenameTemplate.text().strip() or None
        output_dir = dlg.lineEditOutputLocation.text().strip() or None

        # Temporarily override the job's output file template
        old_template = getattr(self.job, "PostProcessorOutputFile", "")
        try:
            if template:
                self.job.PostProcessorOutputFile = template

            generator = FilenameGenerator(job=self.job)
            gen_filenames = generator.generate_filenames()

            new_fnames = []
            for subpart, gcode in self._output_sections:
                subpart_clean = "" if subpart == "allitems" else subpart
                generator.set_subpartname(subpart_clean)
                fname = next(gen_filenames)
                if output_dir:
                    fname = os.path.join(output_dir, os.path.basename(fname))
                if gcode is not None:
                    new_fnames.append(fname)
        finally:
            if template:
                self.job.PostProcessorOutputFile = old_template

        # Map new names onto existing gcode (preserving user edits) by position
        old_gcodes = list(self._generated_outputs.values())
        self._generated_outputs = {}
        for i, fname in enumerate(new_fnames):
            self._generated_outputs[fname] = old_gcodes[i] if i < len(old_gcodes) else ""

        self._populate_output_tab()

    def _browse_output_location(self):
        dlg = self.dialog
        current = dlg.lineEditOutputLocation.text().strip() or ""
        folder = QtGui.QFileDialog.getExistingDirectory(
            dlg,
            translate("CAM_Post", "Select Output Folder"),
            current,
        )
        if folder:
            dlg.lineEditOutputLocation.setText(folder)

    def _save_output_files(self):
        """Write all generated (and possibly edited) outputs to disk, then close."""
        import os

        self._flush_editor_to_outputs()

        if not self._generated_outputs:
            return

        output_dir = self.dialog.lineEditOutputLocation.text().strip() or None

        written = []
        errors = []
        for fname, gcode in self._generated_outputs.items():
            try:
                save_path = (
                    os.path.join(output_dir, os.path.basename(fname)) if output_dir else fname
                )
                os.makedirs(os.path.dirname(os.path.abspath(save_path)), exist_ok=True)
                with open(save_path, "w", encoding="utf-8") as f:
                    f.write(gcode)
                written.append(save_path)
                FreeCAD.Console.PrintMessage(f"File written to {save_path}\n")
            except Exception as e:
                errors.append(f"{os.path.basename(fname)}: {e}")

        dlg = self.dialog
        if errors:
            dlg.labelOutputStatus.setText(
                translate("CAM_Post", "{} error(s) while saving:\n{}").format(
                    len(errors), "\n".join(errors)
                )
            )
            dlg.labelOutputStatus.setStyleSheet("color: red;")
            dlg.labelOutputStatus.setVisible(True)
        else:
            dlg.accept()

    # ------------------------------------------------------------------
    # Public interface
    # ------------------------------------------------------------------

    def exec_(self):
        return self.dialog.exec_()

    def config(self):
        """Return a dict of the user's choices. Call after exec_() returns Accepted."""
        dlg = self.dialog
        tree = dlg.treeWidgetOperations
        ops = self._get_active_operations()

        # Only active operations that the user kept checked
        selected_ops = [
            ops[i]
            for i in range(tree.topLevelItemCount())
            if tree.topLevelItem(i).checkState(0) == QtCore.Qt.CheckState.Checked and i < len(ops)
        ]

        lw = dlg.listWidgetFixtures
        fixtures = getattr(self.job, "Fixtures", None) or []
        selected_fixtures = [
            fixtures[i]
            for i in range(lw.count())
            if lw.item(i).checkState() == QtCore.Qt.CheckState.Checked and i < len(fixtures)
        ]

        # Collect dynamic machine output field values
        machine_output_overrides = {}
        for section, widgets in self._machine_output_field_widgets.items():
            for field_name, widget in widgets.items():
                if hasattr(widget, "value_getter"):
                    machine_output_overrides[field_name] = widget.value_getter()

        self._flush_editor_to_outputs()

        return {
            "machine": dlg.comboBoxMachine.currentData(),
            "selected_operations": selected_ops,
            "selected_fixtures": selected_fixtures,
            "job_author": dlg.lineEditJobAuthor.text().strip(),
            "comment": dlg.plainTextEditComment.toPlainText().strip(),
            "generate_sanity_report": dlg.checkBoxSanityReport.isChecked(),
            "machine_output_overrides": machine_output_overrides,
            "generated_outputs": dict(self._generated_outputs),
        }
