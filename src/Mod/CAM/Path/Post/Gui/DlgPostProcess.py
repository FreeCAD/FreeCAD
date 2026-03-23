# SPDX-License-Identifier: LGPL-2.1-or-later

"""Unified Post-Processing dialog.

Shows job information, operation selection, output settings, and sanity
warnings in a single dialog before committing to G-code generation.

Phase 1 - skeleton: populates all widgets from the job object and returns
a config dict on accept.  No functional post-processing logic lives here yet.
"""

import FreeCAD
import FreeCADGui
import Path
import Path.Preferences as PathPrefs
from PySide import QtCore, QtGui

translate = FreeCAD.Qt.translate

LOG_MODULE = Path.Log.thisModule()

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
        dlg.buttonSelectByType.clicked.connect(self._select_by_type)
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
        """Clear and rebuild the dynamic machine-output option groups in the Output tab."""
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
            return

        dlg.labelNoMachineOutput.setVisible(False)

        machine = None
        try:
            from Machine.models.machine import MachineFactory

            machine = MachineFactory.load_configuration(machine_path)
        except Exception as e:
            Path.Log.warning(f"Could not load machine for output options: {e}")
            return

        try:
            from Machine.ui.editor.machine_editor import DataclassGUIGenerator
        except Exception as e:
            Path.Log.warning(f"Could not import DataclassGUIGenerator: {e}")
            return

        # Insertion point: just before groupBoxPostParams
        insert_idx = scroll_layout.indexOf(dlg.groupBoxPostParams)

        # --- Main output options (units + top-level booleans) ---
        main_group = QtGui.QGroupBox(translate("CAM_Post", "Main Options"))
        main_layout = QtGui.QFormLayout(main_group)

        units_combo = QtGui.QComboBox()
        units_combo.addItem(translate("CAM_Post", "Metric"), "metric")
        units_combo.addItem(translate("CAM_Post", "Imperial"), "imperial")
        try:
            units_val = machine.output.units.value
            idx = units_combo.findData(units_val)
            if idx >= 0:
                units_combo.setCurrentIndex(idx)
        except Exception:
            pass
        main_layout.addRow(translate("CAM_Post", "Units"), units_combo)
        units_combo.value_getter = lambda: units_combo.itemData(units_combo.currentIndex())

        main_widgets = {"units": units_combo}
        for field_name, label in [
            ("output_header", translate("CAM_Post", "Output Header")),
            ("output_tool_length_offset", translate("CAM_Post", "Output Tool Length Offset (G43)")),
            ("remote_post", translate("CAM_Post", "Enable Remote Posting")),
        ]:
            cb = QtGui.QCheckBox()
            cb.setChecked(bool(getattr(getattr(machine, "output", None), field_name, False)))
            cb.value_getter = lambda w=cb: w.isChecked()
            main_layout.addRow(label, cb)
            main_widgets[field_name] = cb

        scroll_layout.insertWidget(insert_idx, main_group)
        self._dynamic_output_groups.append(main_group)
        self._machine_output_field_widgets["main"] = main_widgets
        insert_idx += 1

        # --- Sub-dataclass groups ---
        output = getattr(machine, "output", None)
        if output is None:
            return

        sub_sections = [
            ("header", translate("CAM_Post", "Header Options")),
            ("comments", translate("CAM_Post", "Comment Options")),
            ("formatting", translate("CAM_Post", "Formatting Options")),
            ("precision", translate("CAM_Post", "Precision Options")),
            ("duplicates", translate("CAM_Post", "Duplicate Output Options")),
        ]
        for attr_name, title in sub_sections:
            dc_instance = getattr(output, attr_name, None)
            if dc_instance is None:
                continue
            try:
                group, widgets = DataclassGUIGenerator.create_group_for_dataclass(
                    dc_instance, title
                )
            except Exception as e:
                Path.Log.warning(f"Could not build group for {attr_name}: {e}")
                continue
            scroll_layout.insertWidget(insert_idx, group)
            self._dynamic_output_groups.append(group)
            self._machine_output_field_widgets[attr_name] = widgets
            insert_idx += 1

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
        dlg.lineEditJobAuthor.setText(getattr(self.job, "Author", "") or "")
        dlg.plainTextEditComment.setPlainText("")

    def _populate_operations(self):
        dlg = self.dialog
        tree = dlg.treeWidgetOperations
        tree.blockSignals(True)
        tree.clear()

        for op in self._get_operations():
            # Skip operations that are toggled inactive
            if not getattr(op, "Active", True):
                continue
            item = QtGui.QTreeWidgetItem(tree)
            item.setText(0, op.Label)
            ct = getattr(op, "CycleTime", None)
            item.setText(1, ct if ct else "-")
            item.setCheckState(0, QtCore.Qt.CheckState.Checked)
            item.setFlags(
                item.flags()
                | QtCore.Qt.ItemFlag.ItemIsUserCheckable
                | QtCore.Qt.ItemFlag.ItemIsEnabled
            )

        tree.resizeColumnToContents(0)
        tree.header().setStretchLastSection(True)
        tree.blockSignals(False)
        self._update_ops_tab_label()
        self._update_total_time()

    def _populate_warnings(self):
        dlg = self.dialog
        try:
            from Path.Main.Sanity.Sanity import CAMSanity

            all_squawks, critical_squawks = CAMSanity.validate_job(self.job)
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

    def _update_total_time(self):
        tree = self.dialog.treeWidgetOperations
        total_secs = 0
        has_any = False
        for i in range(tree.topLevelItemCount()):
            item = tree.topLevelItem(i)
            if item.checkState(0) != QtCore.Qt.CheckState.Checked:
                continue
            secs = _parse_cycle_time(item.text(1))
            if secs is not None:
                total_secs += secs
                has_any = True
        self.dialog.labelTotalTime.setText(
            translate("CAM_Post", "Total: {}").format(
                _format_seconds(total_secs) if has_any else "-"
            )
        )

    def _on_ops_changed(self, _item, _col=None):
        self._update_ops_tab_label()
        self._update_total_time()

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

    def _select_by_type(self):
        """Pop a menu of distinct operation types; selecting one checks only those ops."""
        ops = self._get_operations()
        tree = self.dialog.treeWidgetOperations
        types = {}
        for i in range(tree.topLevelItemCount()):
            if i < len(ops):
                t = ops[i].TypeId.split("::")[-1]
                types.setdefault(t, []).append(i)

        if not types:
            return

        menu = QtGui.QMenu(self.dialog)
        for t in sorted(types):
            action = menu.addAction(t)
            action.setData(t)

        btn = self.dialog.buttonSelectByType
        chosen = menu.exec_(btn.mapToGlobal(btn.rect().bottomLeft()))
        if not chosen:
            return

        selected_type = chosen.data()
        tree.blockSignals(True)
        for i in range(tree.topLevelItemCount()):
            op_type = ops[i].TypeId.split("::")[-1] if i < len(ops) else ""
            state = (
                QtCore.Qt.CheckState.Checked
                if op_type == selected_type
                else QtCore.Qt.CheckState.Unchecked
            )
            tree.topLevelItem(i).setCheckState(0, state)
        tree.blockSignals(False)
        self._on_ops_changed(None)

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

        # Build filtered operations list from the Operations tab checkboxes
        all_ops = self._get_operations()
        tree = dlg.treeWidgetOperations
        selected_ops = [
            all_ops[i]
            for i in range(tree.topLevelItemCount())
            if tree.topLevelItem(i).checkState(0) == QtCore.Qt.CheckState.Checked
            and i < len(all_ops)
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
                    getattr(job, "PostProcessor", None) or PathPrefs.defaultPostProcessor()
                )
                if not postprocessor_name:
                    QtGui.QMessageBox.warning(
                        self.dialog,
                        translate("CAM_Post", "Generate Output"),
                        translate("CAM_Post", "No post-processor configured for this job."),
                    )
                    return

            postprocessor = PostProcessorFactory.get_post_processor(job_arg, postprocessor_name)

            # Override the postprocessor's machine with the dialog's selection, then
            # write back any changes the user made in the Options tab before export2()
            # calls _merge_machine_config().
            if loaded_machine is not None:
                postprocessor._machine = loaded_machine
                self._apply_options_to_machine(postprocessor._machine)

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
        ops = self._get_operations()

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
