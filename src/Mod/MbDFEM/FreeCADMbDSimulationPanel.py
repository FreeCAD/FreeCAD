# SPDX-License-Identifier: LGPL-2.1-or-later

"""Task panel for MbDFEM simulation parameters and ASMT operations."""

import FreeCAD as App
import FreeCADGui as Gui


def owning_assembly(simulation_parameters):
    """Return the MbDAssembly that owns *simulation_parameters*."""
    document = getattr(simulation_parameters, "Document", None)
    if document is None:
        return None

    for obj in document.Objects:
        try:
            if obj.isDerivedFrom("MbDFEM::MbDAssembly") and obj.getSimulationParameters() == simulation_parameters:
                return obj
        except Exception:
            pass
    return None


def is_simulation_parameters(obj):
    try:
        return obj is not None and obj.isDerivedFrom("MbDFEM::MbDSimulationParameters")
    except Exception:
        return False


def _format_property_float(value):
    mantissa, exponent = f"{float(value):.16e}".split("e")
    mantissa = mantissa.rstrip("0").rstrip(".")
    if "." not in mantissa:
        mantissa += ".0"
    return f"{mantissa}e{exponent}"


def _float_from_text(field):
    text = field.text().strip()
    decimal_point = field.locale().decimalPoint()
    if decimal_point != ".":
        text = text.replace(decimal_point, ".")
    return float(text)


class SimulationTaskPanel:
    """Task-tab controls for MbD simulation parameters and ASMT operations."""

    def __init__(self, assembly):
        import FreeCADMbDBackend
        from PySide import QtWidgets

        self.assembly = assembly
        self.parameters = assembly.ensureSimulationParameters()
        self._original_parameters = self._parameter_values()
        self.form = QtWidgets.QWidget()
        self.form.setWindowTitle("Solve MbDAssembly")

        asmt_path = FreeCADMbDBackend.default_asmt_path(assembly)
        solved_path = asmt_path.with_suffix(".solved.asmt")

        layout = QtWidgets.QVBoxLayout(self.form)

        parameters_group = QtWidgets.QGroupBox("Simulation Parameters")
        form_layout = QtWidgets.QFormLayout(parameters_group)

        self.start_time = self._double_line_edit(self.parameters.startTime, -1.0e12, 1.0e12)
        self.end_time = self._double_line_edit(self.parameters.endTime, -1.0e12, 1.0e12)
        self.min_step_size = self._double_line_edit(self.parameters.minStepSize, 0.0, 1.0e12)
        self.max_step_size = self._double_line_edit(self.parameters.maxStepSize, 1.0e-12, 1.0e12)
        self.output_interval = self._double_line_edit(self.parameters.outputInterval, 0.0, 1.0e12)
        self.significant_digits = self._integer_spinbox(self.parameters.significantDigits, 1, 16)
        self.max_iterations = self._integer_spinbox(self.parameters.maxIterations, 1, 1000000)
        self._parameter_fields = [
            self.start_time,
            self.end_time,
            self.output_interval,
            self.min_step_size,
            self.max_step_size,
            self.significant_digits,
            self.max_iterations,
        ]

        form_layout.addRow("startTime", self.start_time)
        form_layout.addRow("endTime", self.end_time)
        form_layout.addRow("outputInterval", self.output_interval)
        form_layout.addRow("minStepSize", self.min_step_size)
        form_layout.addRow("maxStepSize", self.max_step_size)
        form_layout.addRow("significantDigits", self.significant_digits)
        form_layout.addRow("maxIterations", self.max_iterations)
        layout.addWidget(parameters_group)

        files_group = QtWidgets.QGroupBox("ASMT Files")
        files_layout = QtWidgets.QFormLayout(files_group)
        self.asmt_file = QtWidgets.QLineEdit(str(asmt_path))
        self.solved_asmt_file = QtWidgets.QLineEdit(str(solved_path))
        files_layout.addRow("ASMT", self.asmt_file)
        files_layout.addRow("Solved ASMT", self.solved_asmt_file)
        layout.addWidget(files_group)

        actions_layout = QtWidgets.QHBoxLayout()
        self.export_button = QtWidgets.QPushButton("Export ASMT")
        self.simulate_button = QtWidgets.QPushButton("Simulate ASMT")
        self.import_button = QtWidgets.QPushButton("Import ASMT")
        actions_layout.addWidget(self.export_button)
        actions_layout.addWidget(self.simulate_button)
        actions_layout.addWidget(self.import_button)
        layout.addLayout(actions_layout)

        solve_layout = QtWidgets.QHBoxLayout()
        self.solve_button = QtWidgets.QPushButton("Solve MbDAssembly")
        solve_layout.addStretch()
        solve_layout.addWidget(self.solve_button)
        solve_layout.addStretch()
        layout.addLayout(solve_layout)

        self.status_label = QtWidgets.QLabel()
        self.status_label.setWordWrap(True)
        layout.addWidget(self.status_label)

        self.export_button.clicked.connect(self.export_asmt)
        self.simulate_button.clicked.connect(self.simulate_asmt)
        self.import_button.clicked.connect(self.import_asmt)
        self.solve_button.clicked.connect(self.solve_assembly)

    @staticmethod
    def _double_line_edit(value, minimum, maximum):
        from PySide import QtGui, QtWidgets

        field = QtWidgets.QLineEdit(_format_property_float(value))
        validator = QtGui.QDoubleValidator(minimum, maximum, 16, field)
        validator.setNotation(QtGui.QDoubleValidator.ScientificNotation)
        field.setValidator(validator)
        return field

    @staticmethod
    def _integer_spinbox(value, minimum, maximum):
        from PySide import QtWidgets

        field = QtWidgets.QSpinBox()
        field.setRange(minimum, maximum)
        field.setValue(int(value))
        return field

    def getStandardButtons(self):
        from PySide import QtGui

        return QtGui.QDialogButtonBox.Ok | QtGui.QDialogButtonBox.Cancel

    def accept(self):
        return self._apply_parameters()

    def reject(self):
        self._restore_parameters(self._original_parameters)
        return True

    def export_asmt(self):
        import FreeCADMbDExporter
        from pathlib import Path

        try:
            if not self._apply_parameters():
                return False
            asmt_path = Path(self.asmt_file.text())
            FreeCADMbDExporter.export_assembly(self.assembly, asmt_path)
            self._set_status(f"Exported ASMT: {asmt_path}")
            App.Console.PrintMessage(f"Exported FreeCADMbD input: {asmt_path}\n")
            return True
        except Exception as exc:
            self._report_error("ASMT export failed", exc)
            return False

    def simulate_asmt(self):
        import FreeCADMbDBackend
        from pathlib import Path

        try:
            if not self._apply_parameters():
                return False
            asmt_path = Path(self.asmt_file.text())
            solved_path = Path(self.solved_asmt_file.text())
            backend = FreeCADMbDBackend.FreeCADMbDProcessBackend()
            solved_path, completed = backend.simulate_asmt(asmt_path, solved_path)
            self.solved_asmt_file.setText(str(solved_path))
            self._set_status(f"Simulated ASMT: {solved_path}")
            App.Console.PrintMessage(f"Wrote FreeCADMbD solved assembly: {solved_path}\n")
            if completed.stdout:
                App.Console.PrintMessage(completed.stdout)
            if completed.stderr:
                App.Console.PrintWarning(completed.stderr)
            return True
        except Exception as exc:
            self._report_error("ASMT simulation failed", exc)
            return False

    def import_asmt(self):
        import FreeCADMbDResults
        from pathlib import Path

        try:
            if not self._apply_parameters():
                return False
            solved_path = Path(self.solved_asmt_file.text())
            imported = FreeCADMbDResults.import_results(self.assembly, solved_path)
            self._set_status(f"Imported ASMT results: {solved_path}")
            App.Console.PrintMessage(f"Imported FreeCADMbD results from: {solved_path}\n")
            Gui.Selection.clearSelection()
            Gui.Selection.addSelection(self.assembly)
            self.assembly.Document.recompute()
            App.Console.PrintMessage(f"Updated {len(imported)} result objects.\n")
            return True
        except Exception as exc:
            self._report_error("ASMT import failed", exc)
            return False

    def solve_assembly(self):
        if self.export_asmt() and self.simulate_asmt():
            self.import_asmt()

    def _apply_parameters(self):
        invalid_fields = [field for field in self._parameter_fields if not field.hasAcceptableInput()]
        if invalid_fields:
            self._set_status("Enter valid simulation parameter values.")
            return False

        self.parameters.startTime = _float_from_text(self.start_time)
        self.parameters.endTime = _float_from_text(self.end_time)
        self.parameters.maxStepSize = _float_from_text(self.max_step_size)
        self.parameters.minStepSize = _float_from_text(self.min_step_size)
        self.parameters.significantDigits = self.significant_digits.value()
        self.parameters.maxIterations = self.max_iterations.value()
        self.parameters.outputInterval = _float_from_text(self.output_interval)
        return True

    def _parameter_values(self):
        return {
            "startTime": self.parameters.startTime,
            "endTime": self.parameters.endTime,
            "maxStepSize": self.parameters.maxStepSize,
            "minStepSize": self.parameters.minStepSize,
            "significantDigits": self.parameters.significantDigits,
            "maxIterations": self.parameters.maxIterations,
            "outputInterval": self.parameters.outputInterval,
        }

    def _restore_parameters(self, values):
        self.parameters.startTime = values["startTime"]
        self.parameters.endTime = values["endTime"]
        self.parameters.maxStepSize = values["maxStepSize"]
        self.parameters.minStepSize = values["minStepSize"]
        self.parameters.significantDigits = values["significantDigits"]
        self.parameters.maxIterations = values["maxIterations"]
        self.parameters.outputInterval = values["outputInterval"]

    def _set_status(self, message):
        self.status_label.setText(message)
        App.Console.PrintMessage(message + "\n")

    def _report_error(self, action, exc):
        message = f"{action}: {exc}"
        self.status_label.setText(message)
        App.Console.PrintError(message + "\n")


class SimulationParametersSelectionObserver:
    """Open the simulation Task panel when a SimulationParameters object is selected."""

    def addSelection(self, document_name, object_name, sub_name, mouse_position):
        obj = self._selected_object(document_name, object_name, sub_name)
        if not is_simulation_parameters(obj):
            return
        show_simulation_task_panel(obj)

    @staticmethod
    def _selected_object(document_name, object_name, sub_name=""):
        document = App.getDocument(document_name)
        root = document.getObject(object_name) if document is not None else None
        if root is None:
            return None
        if is_simulation_parameters(root):
            return root

        if sub_name:
            try:
                resolved = root.getSubObject(sub_name)
                if is_simulation_parameters(resolved):
                    return resolved
            except Exception:
                pass

            object_from_subname = document.getObject(sub_name.rstrip(".").rsplit(".", 1)[-1])
            if is_simulation_parameters(object_from_subname):
                return object_from_subname

        return root


def show_assembly_task_panel(assembly):
    return _show_task_panel(assembly)


def show_simulation_task_panel(simulation_parameters):
    assembly = owning_assembly(simulation_parameters)
    if assembly is None:
        App.Console.PrintError("No owning MbDAssembly found for SimulationParameters.\n")
        return None
    return _show_task_panel(assembly)


def _show_task_panel(assembly):
    active = Gui.Control.activeDialog()
    if isinstance(active, SimulationTaskPanel):
        if active.assembly == assembly:
            return active
        Gui.Control.closeDialog()
    elif active is not None:
        Gui.Control.closeDialog()

    panel = SimulationTaskPanel(assembly)
    Gui.Control.showDialog(panel)
    return panel
