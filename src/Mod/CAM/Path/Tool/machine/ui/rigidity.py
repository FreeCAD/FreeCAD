from PySide import QtGui, QtCore
import FreeCAD
import FreeCADGui
from ..models.machine import LinearAxis, AngularAxis


translate = FreeCAD.Qt.translate


class PrerequisitesPage(QtGui.QWizardPage):
    def __init__(self):
        super().__init__()
        self.setTitle(translate("CAM", "Prerequisites"))
        label = QtGui.QLabel(
            translate(
                "CAM",
                "You will need:\n"
                "- A consistent load applicator (e.g., luggage scale)\n"
                "- A micrometer for deflection measurements\n"
                "- An inclinometer for angular measurements",
            )
        )
        layout = QtGui.QVBoxLayout()
        layout.addWidget(label)
        layout.addStretch()
        self.setLayout(layout)


class LinearAxesPage(QtGui.QWizardPage):
    def __init__(self, machine):
        super().__init__()
        self.machine = machine
        self.setTitle(translate("CAM", "Linear Axes Measurements"))
        self.setSubTitle(translate("CAM", "Enter measurements for linear axes"))
        main_layout = QtGui.QVBoxLayout()

        steps = translate(
            "CAM",
            """Measurement Procedure:
1. Remove tool and collet from the spindle.
2. Position the machine at its least rigid configuration (e.g., for gantry-style machines: Z-axis fully extended, X/Y at mid-range).
3. Secure a dial indicator to the machine table, contacting the spindle nose.
4. Apply a known force (e.g., 10 N) and record the deflection.
5. Enter the force and deflection values for each axis below.
""",
        )

        instructions = QtGui.QLabel(steps)
        main_layout.addWidget(instructions)

        separator = QtGui.QFrame()
        separator.setFrameShape(QtGui.QFrame.HLine)
        separator.setFrameShadow(QtGui.QFrame.Sunken)
        main_layout.addWidget(separator)

        grid_layout = QtGui.QGridLayout()
        grid_layout.setSpacing(6)

        # Add column headers
        grid_layout.addWidget(QtGui.QLabel(translate("CAM", "Axis")), 0, 0)
        grid_layout.addWidget(QtGui.QLabel(translate("CAM", "Force")), 0, 1)
        grid_layout.addWidget(QtGui.QLabel(translate("CAM", "Deflection")), 0, 2)

        self.inputs = {}
        ui = FreeCADGui.UiLoader()
        row = 1  # Start from row 1 for data
        for axis_name, axis in sorted(machine.axes.items()):
            if isinstance(axis, LinearAxis):
                force_edit = ui.createWidget("Gui::QuantitySpinBox")
                force_edit.setProperty("unit", FreeCAD.Units.Unit("N"))
                force_edit.setProperty("minimum", 0.0)
                force_edit.setProperty("value", FreeCAD.Units.Quantity("10.0 N"))
                force_edit.valueChanged.connect(lambda: self.enforce_unit(force_edit, "N"))
                force_edit.valueChanged.connect(self.check_completeness)

                deflection_edit = ui.createWidget("Gui::QuantitySpinBox")
                deflection_edit.setProperty("unit", "mm")
                deflection_edit.setProperty("minimum", 0.0)
                deflection_edit.setProperty("value", FreeCAD.Units.Quantity("0 mm"))
                deflection_edit.valueChanged.connect(self.check_completeness)

                grid_layout.addWidget(
                    QtGui.QLabel(translate("CAM", axis_name.capitalize())),
                    row,
                    0,
                )
                grid_layout.addWidget(force_edit, row, 1)
                grid_layout.addWidget(deflection_edit, row, 2)
                self.inputs[axis_name] = (force_edit, deflection_edit)
                row += 1

        if not self.inputs:
            grid_layout.addWidget(
                QtGui.QLabel(translate("CAM", "No linear axes to configure.")), row, 0, 1, 3
            )

        main_layout.addLayout(grid_layout)
        main_layout.addStretch()
        self.setLayout(main_layout)

    def initializePage(self):
        super().initializePage()
        self.check_completeness()

    def isComplete(self):
        for force_edit, deflection_edit in self.inputs.values():
            force = force_edit.property("value").Value
            deflection = deflection_edit.property("value").Value
            if force > 0 and deflection <= 0:
                return False
        return True

    def check_completeness(self):
        self.completeChanged.emit()
        self.update_button_states()

    def update_button_states(self):
        wizard = self.wizard()
        wizard.button(QtGui.QWizard.NextButton).setEnabled(self.isComplete())

    def enforce_unit(self, widget, unit):
        value = widget.property("value")
        if value.Unit != FreeCAD.Units.Unit(unit):
            widget.setProperty("value", FreeCAD.Units.Quantity(f"{value.Value} {unit}"))


class AngularAxesPage(QtGui.QWizardPage):
    def __init__(self, machine):
        super().__init__()
        self.machine = machine
        self.setTitle(translate("CAM", "Angular Axes Measurements"))
        self.setSubTitle(translate("CAM", "Enter measurements for angular axes"))

        main_layout = QtGui.QVBoxLayout()
        instructions = QtGui.QLabel(
            translate(
                "CAM",
                """Measurement Procedure:
1. Attach an inclinometer to the spindle.
2. Apply a known force and measure the angular deflection.
3. Record force and deflection values for X and Y directions below.
""",
            )
        )
        main_layout.addWidget(instructions)

        separator = QtGui.QFrame()
        separator.setFrameShape(QtGui.QFrame.HLine)
        separator.setFrameShadow(QtGui.QFrame.Sunken)
        main_layout.addWidget(separator)

        grid_layout = QtGui.QGridLayout()
        grid_layout.setSpacing(6)

        # Add column headers
        grid_layout.addWidget(QtGui.QLabel(translate("CAM", "Force")), 0, 1)
        grid_layout.addWidget(QtGui.QLabel(translate("CAM", "Deflection")), 0, 2)

        self.inputs = {}
        ui = FreeCADGui.UiLoader()
        row = 1  # Start from row 1 for data
        for axis_name, axis in sorted(machine.axes.items()):
            if isinstance(axis, AngularAxis):
                # X-direction inputs
                force_x_edit = ui.createWidget("Gui::QuantitySpinBox")
                force_x_edit.setProperty("unit", FreeCAD.Units.Unit("N"))
                force_x_edit.setProperty("minimum", 0.0)
                force_x_edit.setProperty("value", FreeCAD.Units.Quantity("10.0 N"))
                force_x_edit.valueChanged.connect(lambda: self.enforce_unit(force_x_edit, "N"))
                force_x_edit.valueChanged.connect(self.check_completeness)

                deflection_x_edit = ui.createWidget("Gui::QuantitySpinBox")
                deflection_x_edit.setProperty("unit", "deg")
                deflection_x_edit.setProperty("minimum", 0.0)
                deflection_x_edit.setProperty("value", FreeCAD.Units.Quantity("0 deg"))
                deflection_x_edit.valueChanged.connect(self.check_completeness)

                # Y-direction inputs
                force_y_edit = ui.createWidget("Gui::QuantitySpinBox")
                force_y_edit.setProperty("unit", FreeCAD.Units.Unit("N"))
                force_y_edit.setProperty("minimum", 0.0)
                force_y_edit.setProperty("value", FreeCAD.Units.Quantity("10.0 N"))
                force_y_edit.valueChanged.connect(lambda: self.enforce_unit(force_y_edit, "N"))
                force_y_edit.valueChanged.connect(self.check_completeness)

                deflection_y_edit = ui.createWidget("Gui::QuantitySpinBox")
                deflection_y_edit.setProperty("unit", "deg")
                deflection_y_edit.setProperty("minimum", 0.0)
                deflection_y_edit.setProperty("value", FreeCAD.Units.Quantity("0 deg"))
                deflection_y_edit.valueChanged.connect(self.check_completeness)

                # Add X-direction row
                grid_layout.addWidget(
                    QtGui.QLabel(translate("CAM", f"{axis_name.capitalize()}-axis X")),
                    row,
                    0,
                )
                grid_layout.addWidget(force_x_edit, row, 1)
                grid_layout.addWidget(deflection_x_edit, row, 2)
                row += 1

                # Add Y-direction row
                grid_layout.addWidget(
                    QtGui.QLabel(translate("CAM", f"{axis_name.capitalize()}-axis Y")),
                    row,
                    0,
                )
                grid_layout.addWidget(force_y_edit, row, 1)
                grid_layout.addWidget(deflection_y_edit, row, 2)
                row += 1

                self.inputs[axis_name] = (
                    force_x_edit,
                    deflection_x_edit,
                    force_y_edit,
                    deflection_y_edit,
                )

        if not self.inputs:
            grid_layout.addWidget(
                QtGui.QLabel(translate("CAM", "No angular axes to configure.")), row, 0, 1, 3
            )

        main_layout.addLayout(grid_layout)
        main_layout.addStretch()
        self.setLayout(main_layout)

    def initializePage(self):
        super().initializePage()
        self.check_completeness()

    def isComplete(self):
        for (
            force_x_edit,
            deflection_x_edit,
            force_y_edit,
            deflection_y_edit,
        ) in self.inputs.values():
            force_x = force_x_edit.property("value").Value
            deflection_x = deflection_x_edit.property("value").Value
            force_y = force_y_edit.property("value").Value
            deflection_y = deflection_y_edit.property("value").Value
            if (force_x > 0 and deflection_x <= 0) or (force_y > 0 and deflection_y <= 0):
                return False
        return True

    def check_completeness(self):
        self.completeChanged.emit()
        self.update_button_states()

    def update_button_states(self):
        wizard = self.wizard()
        wizard.button(QtGui.QWizard.NextButton).setEnabled(self.isComplete())

    def enforce_unit(self, widget, unit):
        value = widget.property("value")
        if value.Unit != FreeCAD.Units.Unit(unit):
            widget.setProperty("value", FreeCAD.Units.Quantity(f"{value.Value} {unit}"))


class SummaryPage(QtGui.QWizardPage):
    def __init__(self, wizard):
        super().__init__()
        self.setTitle(translate("CAM", "Summary"))
        self.setSubTitle(translate("CAM", "Review calculated rigidities"))
        self.setButtonText(QtGui.QWizard.FinishButton, translate("CAM", "Apply and Close"))

        layout = QtGui.QVBoxLayout()
        self.summary_label = QtGui.QLabel()
        layout.addWidget(self.summary_label)
        layout.addStretch()
        self.setLayout(layout)

    def initializePage(self):
        super().initializePage()
        rigidities = self.wizard().calculate_rigidities()
        summary_text = "Calculated Rigidities:\n\n"

        if not rigidities:
            summary_text += "No rigidity values calculated."
        else:
            for axis, (rigidity, unit) in rigidities.items():
                value = rigidity.Value
                summary_text += f"{axis.capitalize()}: {value:.6f} {unit}\n"

        self.summary_label.setText(translate("CAM", summary_text))
        self.check_completeness()

    def isComplete(self):
        return True

    def check_completeness(self):
        self.completeChanged.emit()
        self.update_button_states()

    def update_button_states(self):
        wizard = self.wizard()
        wizard.button(QtGui.QWizard.FinishButton).setEnabled(self.isComplete())


class RigidityWizard(QtGui.QWizard):
    def __init__(self, machine, parent=None):
        super().__init__(parent)
        self.machine = machine
        self.setWindowTitle(translate("CAM", "Configure Rigidities"))

        self.setOption(QtGui.QWizard.HaveCustomButton1, True)
        self.setButtonText(QtGui.QWizard.CustomButton1, translate("CAM", "Skip"))

        self.prerequisites_page = PrerequisitesPage()
        self.linear_axes_page = LinearAxesPage(machine)
        self.angular_axes_page = AngularAxesPage(machine)
        self.summary_page = SummaryPage(self)

        self.addPage(self.prerequisites_page)
        self.addPage(self.linear_axes_page)
        self.addPage(self.angular_axes_page)
        self.addPage(self.summary_page)

        self.rigidities = {}

        self.button(QtGui.QWizard.CustomButton1).clicked.connect(self.handle_skip)

        self.skip_linear = False
        self.skip_angular = False

        self.setOption(QtGui.QWizard.NoDefaultButton, True)
        self.setButtonLayout(
            [
                QtGui.QWizard.CancelButton,
                QtGui.QWizard.Stretch,
                QtGui.QWizard.BackButton,
                QtGui.QWizard.CustomButton1,
                QtGui.QWizard.NextButton,
                QtGui.QWizard.FinishButton,
            ]
        )

        # Connect currentIdChanged signal explicitly
        self.currentIdChanged.connect(self.update_skip_button_visibility)
        self.update_skip_button_visibility(self.currentId())

    def update_skip_button_visibility(self, page_id):
        linear_page_id = self.pageIds()[1]
        angular_page_id = self.pageIds()[2]
        is_visible = page_id in [linear_page_id, angular_page_id]
        self.button(QtGui.QWizard.CustomButton1).setVisible(is_visible)

    def handle_skip(self):
        current_id = self.currentId()
        linear_page_id = self.pageIds()[1]
        angular_page_id = self.pageIds()[2]

        if current_id == linear_page_id:
            self.skip_linear = True
            self.next()
        elif current_id == angular_page_id:
            self.skip_angular = True
            self.next()

    def nextId(self):
        current_id = self.currentId()
        linear_page_id = self.pageIds()[1]
        angular_page_id = self.pageIds()[2]
        summary_page_id = self.pageIds()[3]

        if current_id == linear_page_id and self.skip_linear:
            return angular_page_id
        elif current_id == angular_page_id and self.skip_angular:
            return summary_page_id
        return super().nextId()

    def calculate_rigidities(self):
        self.rigidities = {}

        if not self.skip_linear:
            for axis_name, (force_edit, deflection_edit) in self.linear_axes_page.inputs.items():
                force = force_edit.property("value")
                deflection = deflection_edit.property("value")
                if force.Value > 0:
                    # normalize to deflection per 1 Newton. FreeCAD raw value for N is
                    # stored in mN, so factor 1000
                    rigidity = deflection / force * 1000
                    self.rigidities[axis_name] = rigidity, "mm/N"

        if not self.skip_angular:
            for axis_name, (
                force_x_edit,
                deflection_x_edit,
                force_y_edit,
                deflection_y_edit,
            ) in self.angular_axes_page.inputs.items():
                force_x = force_x_edit.property("value")
                deflection_x_deg = deflection_x_edit.property("value")
                force_y = force_y_edit.property("value")
                deflection_y_deg = deflection_y_edit.property("value")

                rigidity_x = FreeCAD.Units.Quantity("0 deg")
                rigidity_y = FreeCAD.Units.Quantity("0 deg")

                if force_x.Value > 0:
                    # normalize to deflection per 1 Newton
                    rigidity_x = deflection_x_deg / force_x * 1000

                if force_y.Value > 0:
                    rigidity_y = deflection_y_deg / force_y * 1000

                # Choose the worst-case rigidity (maximum value)
                rigidity = max(rigidity_x, rigidity_y)
                self.rigidities[axis_name] = rigidity, "deg/N"

        return self.rigidities

    def accept(self):
        self.calculate_rigidities()
        super().accept()

    def get_rigidities(self):
        return self.rigidities
