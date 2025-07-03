# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2025 Samuel Abels <knipknap@gmail.com>                  *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************
from PySide import QtGui
import FreeCAD
import FreeCADGui
from ..models.axis import AngularAxis
from ..models.machine import Machine

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


class LinearRigidityPage(QtGui.QWizardPage):
    def __init__(self, machine: Machine):
        super().__init__()
        self.machine = machine
        self.setTitle(translate("CAM", "Linear Rigidity Measurements"))
        self.setSubTitle(
            translate("CAM", "Enter measurements for linear rigidity (X and Y directions)")
        )
        main_layout = QtGui.QVBoxLayout()

        steps = translate(
            "CAM",
            """Measurement Procedure for Linear Rigidity (X and Y directions):
1. Remove tool and collet from the spindle.
2. Position the machine at its least rigid configuration (e.g., for gantry-style machines: Z-axis fully extended, X/Y at mid-range).
3. Secure a dial indicator to the machine table, contacting the spindle nose.
4. Apply a known force (e.g., 10 N) in the X direction and record the deflection.
5. Apply a known force (e.g., 10 N) in the Y direction and record the deflection.
6. Enter the force and deflection values for each angular axis below.
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
        grid_layout.addWidget(QtGui.QLabel(translate("CAM", "Force X")), 0, 1)
        grid_layout.addWidget(QtGui.QLabel(translate("CAM", "Deflection X")), 0, 2)
        grid_layout.addWidget(QtGui.QLabel(translate("CAM", "Force Y")), 0, 3)
        grid_layout.addWidget(QtGui.QLabel(translate("CAM", "Deflection Y")), 0, 4)

        self.inputs = {}
        ui = FreeCADGui.UiLoader()
        row = 1  # Start from row 1 for data
        for axis in sorted(machine.find_children_by_type(AngularAxis), key=lambda a: a.name):
            force_x_edit = ui.createWidget("Gui::QuantitySpinBox")
            force_x_edit.setProperty("unit", FreeCAD.Units.Unit("N"))
            force_x_edit.setProperty("minimum", 0.0)
            force_x_edit.setProperty("value", FreeCAD.Units.Quantity("10.0 N"))
            force_x_edit.valueChanged.connect(lambda: self.enforce_unit(force_x_edit, "N"))
            force_x_edit.valueChanged.connect(self.check_completeness)

            deflection_x_edit = ui.createWidget("Gui::QuantitySpinBox")
            deflection_x_edit.setProperty("unit", "mm")
            deflection_x_edit.setProperty("minimum", 0.0)
            deflection_x_edit.setProperty("value", FreeCAD.Units.Quantity("0 mm"))
            deflection_x_edit.valueChanged.connect(self.check_completeness)

            force_y_edit = ui.createWidget("Gui::QuantitySpinBox")
            force_y_edit.setProperty("unit", FreeCAD.Units.Unit("N"))
            force_y_edit.setProperty("minimum", 0.0)
            force_y_edit.setProperty("value", FreeCAD.Units.Quantity("10.0 N"))
            force_y_edit.valueChanged.connect(lambda: self.enforce_unit(force_y_edit, "N"))
            force_y_edit.valueChanged.connect(self.check_completeness)

            deflection_y_edit = ui.createWidget("Gui::QuantitySpinBox")
            deflection_y_edit.setProperty("unit", "mm")
            deflection_y_edit.setProperty("minimum", 0.0)
            deflection_y_edit.setProperty("value", FreeCAD.Units.Quantity("0 mm"))
            deflection_y_edit.valueChanged.connect(self.check_completeness)

            grid_layout.addWidget(
                QtGui.QLabel(axis.label),
                row,
                0,
            )
            grid_layout.addWidget(force_x_edit, row, 1)
            grid_layout.addWidget(deflection_x_edit, row, 2)
            grid_layout.addWidget(force_y_edit, row, 3)
            grid_layout.addWidget(deflection_y_edit, row, 4)
            self.inputs[axis.name] = (
                force_x_edit,
                deflection_x_edit,
                force_y_edit,
                deflection_y_edit,
            )
            row += 1

        if not self.inputs:
            grid_layout.addWidget(
                QtGui.QLabel(translate("CAM", "No angular axes to configure linear rigidity.")),
                row,
                0,
                1,
                5,
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


class AngularRigidityPage(QtGui.QWizardPage):
    def __init__(self, machine: Machine):
        super().__init__()
        self.machine = machine
        self.setTitle(translate("CAM", "Angular Rigidity Measurements"))
        self.setSubTitle(translate("CAM", "Enter measurements for angular rigidity"))

        main_layout = QtGui.QVBoxLayout()
        instructions = QtGui.QLabel(
            translate(
                "CAM",
                """Measurement Procedure for Angular Rigidity:
1. Attach an inclinometer to the spindle.
2. Apply a known force and measure the angular deflection.
3. Enter the force and deflection values for each angular axis below.
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
        grid_layout.addWidget(QtGui.QLabel(translate("CAM", "Axis")), 0, 0)
        grid_layout.addWidget(QtGui.QLabel(translate("CAM", "Force")), 0, 1)
        grid_layout.addWidget(QtGui.QLabel(translate("CAM", "Deflection")), 0, 2)

        self.inputs = {}
        ui = FreeCADGui.UiLoader()
        row = 1  # Start from row 1 for data
        for axis in sorted(machine.find_children_by_type(AngularAxis), key=lambda a: a.name):
            force_edit = ui.createWidget("Gui::QuantitySpinBox")
            force_edit.setProperty("unit", FreeCAD.Units.Unit("N"))
            force_edit.setProperty("minimum", 0.0)
            force_edit.setProperty("value", FreeCAD.Units.Quantity("10.0 N"))
            force_edit.valueChanged.connect(lambda: self.enforce_unit(force_edit, "N"))
            force_edit.valueChanged.connect(self.check_completeness)

            deflection_edit = ui.createWidget("Gui::QuantitySpinBox")
            deflection_edit.setProperty("unit", "deg")
            deflection_edit.setProperty("minimum", 0.0)
            deflection_edit.setProperty("value", FreeCAD.Units.Quantity("0 deg"))
            deflection_edit.valueChanged.connect(self.check_completeness)

            grid_layout.addWidget(
                QtGui.QLabel(axis.label),
                row,
                0,
            )
            grid_layout.addWidget(force_edit, row, 1)
            grid_layout.addWidget(deflection_edit, row, 2)
            self.inputs[axis.name] = (force_edit, deflection_edit)
            row += 1

        if not self.inputs:
            grid_layout.addWidget(
                QtGui.QLabel(translate("CAM", "No angular axes to configure angular rigidity.")),
                row,
                0,
                1,
                3,
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
            for axis, (angular_rigidity, rigidity_x, rigidity_y) in rigidities.items():
                summary_text += f"{axis.capitalize()}:\n"
                summary_text += f"  Angular Rigidity: {angular_rigidity.Value:.6f} deg/N\n"
                summary_text += f"  Rigidity X: {rigidity_x.Value:.6f} mm/N\n"
                summary_text += f"  Rigidity Y: {rigidity_y.Value:.6f} mm/N\n"

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
    def __init__(self, machine: Machine, parent=None):
        super().__init__(parent)
        self.machine = machine
        self.setWindowTitle(translate("CAM", "Configure Rigidities"))

        self.setOption(QtGui.QWizard.HaveCustomButton1, True)
        self.setButtonText(QtGui.QWizard.CustomButton1, translate("CAM", "Skip"))

        self.prerequisites_page = PrerequisitesPage()
        self.linear_rigidity_page = LinearRigidityPage(machine)
        self.angular_rigidity_page = AngularRigidityPage(machine)
        self.summary_page = SummaryPage(self)

        self.addPage(self.prerequisites_page)
        self.addPage(self.linear_rigidity_page)
        self.addPage(self.angular_rigidity_page)
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
        linear_rigidity_page_id = self.pageIds()[1]
        angular_rigidity_page_id = self.pageIds()[2]
        is_visible = page_id in [linear_rigidity_page_id, angular_rigidity_page_id]
        self.button(QtGui.QWizard.CustomButton1).setVisible(is_visible)

    def handle_skip(self):
        current_id = self.currentId()
        linear_rigidity_page_id = self.pageIds()[1]
        angular_rigidity_page_id = self.pageIds()[2]

        if current_id == linear_rigidity_page_id:
            self.skip_linear = True
            self.next()
        elif current_id == angular_rigidity_page_id:
            self.skip_angular = True
            self.next()

    def nextId(self):
        current_id = self.currentId()
        linear_rigidity_page_id = self.pageIds()[1]
        angular_rigidity_page_id = self.pageIds()[2]
        summary_page_id = self.pageIds()[3]

        if current_id == linear_rigidity_page_id and self.skip_linear:
            return angular_rigidity_page_id
        elif current_id == angular_rigidity_page_id and self.skip_angular:
            return summary_page_id
        return super().nextId()

    def calculate_rigidities(self):
        self.rigidities = {}

        if not self.skip_linear:
            for axis_name, (
                force_x_edit,
                deflection_x_edit,
                force_y_edit,
                deflection_y_edit,
            ) in self.linear_rigidity_page.inputs.items():
                force_x = force_x_edit.property("value")
                deflection_x = deflection_x_edit.property("value")
                force_y = force_y_edit.property("value")
                deflection_y = deflection_y_edit.property("value")

                rigidity_x = FreeCAD.Units.Quantity("0 mm/N")
                rigidity_y = FreeCAD.Units.Quantity("0 mm/N")

                if force_x.Value > 0:
                    rigidity_x = deflection_x / force_x * 1000

                if force_y.Value > 0:
                    rigidity_y = deflection_y / force_y * 1000

                # For linear rigidity, we return both X and Y values
                # Angular rigidity is calculated on the next page
                self.rigidities[axis_name] = (None, rigidity_x, rigidity_y)

        if not self.skip_angular:
            for axis_name, (
                force_edit,
                deflection_edit,
            ) in self.angular_rigidity_page.inputs.items():
                force = force_edit.property("value")
                deflection = deflection_edit.property("value")

                angular_rigidity = FreeCAD.Units.Quantity("0 deg/N")

                if force.Value > 0:
                    angular_rigidity = deflection / force * 1000

                # Retrieve existing rigidity_x and rigidity_y if they were calculated
                _, existing_rigidity_x, existing_rigidity_y = self.rigidities.get(
                    axis_name, (None, None, None)
                )
                rigidity_x = (
                    existing_rigidity_x if existing_rigidity_x else FreeCAD.Units.Quantity("0 mm/N")
                )
                rigidity_y = (
                    existing_rigidity_y if existing_rigidity_y else FreeCAD.Units.Quantity("0 mm/N")
                )

                self.rigidities[axis_name] = (angular_rigidity, rigidity_x, rigidity_y)

        return self.rigidities

    def accept(self):
        self.calculate_rigidities()
        super().accept()

    def get_rigidities(self):
        return self.rigidities
