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
    def __init__(self, angular_axis: AngularAxis):
        super().__init__()
        self.angular_axis = angular_axis
        self.setTitle(translate("CAM", "Linear Rigidity Measurements"))
        self.setSubTitle(
            translate("CAM", f"Enter measurements for linear rigidity of {angular_axis.label}")
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
6. Enter the force and deflection values for the angular axis below.
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
        grid_layout.addWidget(QtGui.QLabel(translate("CAM", "Force X")), 0, 0)
        grid_layout.addWidget(QtGui.QLabel(translate("CAM", "Deflection X")), 0, 1)
        grid_layout.addWidget(QtGui.QLabel(translate("CAM", "Force Y")), 0, 2)
        grid_layout.addWidget(QtGui.QLabel(translate("CAM", "Deflection Y")), 0, 3)

        self.inputs = {}
        ui = FreeCADGui.UiLoader()
        row = 1  # Start from row 1 for data

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

        grid_layout.addWidget(force_x_edit, row, 0)
        grid_layout.addWidget(deflection_x_edit, row, 1)
        grid_layout.addWidget(force_y_edit, row, 2)
        grid_layout.addWidget(deflection_y_edit, row, 3)
        self.inputs[self.angular_axis.name] = (
            force_x_edit,
            deflection_x_edit,
            force_y_edit,
            deflection_y_edit,
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
    def __init__(self, angular_axis: AngularAxis):
        super().__init__()
        self.angular_axis = angular_axis
        self.setTitle(translate("CAM", "Angular Rigidity Measurements"))
        self.setSubTitle(
            translate("CAM", f"Enter measurements for angular rigidity of {angular_axis.label}")
        )

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

        grid_layout.addWidget(force_edit, row, 1)
        grid_layout.addWidget(deflection_edit, row, 2)
        self.inputs[self.angular_axis.name] = (force_edit, deflection_edit)

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
        self.wizard().calculate_rigidities()  # Calculate rigidities before displaying
        rigidities = self.wizard().get_rigidities()
        summary_text = "Calculated Rigidities:\n\n"

        if "rigidity_x" in rigidities:
            summary_text += f"  Rigidity X: {rigidities['rigidity_x'].getValueAs('mm/N')} mm/N\n"
        if "rigidity_y" in rigidities:
            summary_text += f"  Rigidity Y: {rigidities['rigidity_y'].getValueAs('mm/N')} mm/N\n"
        if "angular_rigidity" in rigidities:
            summary_text += (
                f"  Angular Rigidity: {rigidities['angular_rigidity'].Value:.6f} deg/N\n"
            )

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
    def __init__(self, angular_axis: AngularAxis, parent=None):
        super().__init__(parent)
        self.angular_axis = angular_axis
        self.setWindowTitle(translate("CAM", f"Configure Rigidity for {angular_axis.label}"))

        self.setOption(QtGui.QWizard.HaveCustomButton1, True)
        self.setButtonText(QtGui.QWizard.CustomButton1, translate("CAM", "Skip"))

        self.prerequisites_page = PrerequisitesPage()
        self.linear_rigidity_page = LinearRigidityPage(angular_axis)
        self.angular_rigidity_page = AngularRigidityPage(angular_axis)
        self.summary_page = SummaryPage(self)

        self.addPage(self.prerequisites_page)
        self.addPage(self.linear_rigidity_page)
        self.addPage(self.angular_rigidity_page)
        self.addPage(self.summary_page)

        self.calculated_rigidities = {}

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
        axis_name = self.angular_axis.name
        self.calculated_rigidities = {}

        if not self.skip_linear:
            force_x_edit, deflection_x_edit, force_y_edit, deflection_y_edit = (
                self.linear_rigidity_page.inputs[axis_name]
            )

            force_x = force_x_edit.property("value")
            deflection_x = deflection_x_edit.property("value")
            force_y = force_y_edit.property("value")
            deflection_y = deflection_y_edit.property("value")

            if force_x.Value > 0:
                self.calculated_rigidities["rigidity_x"] = deflection_x / force_x

            if force_y.Value > 0:
                self.calculated_rigidities["rigidity_y"] = deflection_y / force_y

        if not self.skip_angular:
            force_edit, deflection_edit = self.angular_rigidity_page.inputs[axis_name]

            force = force_edit.property("value")
            deflection = deflection_edit.property("value")

            if force.Value > 0:
                self.calculated_rigidities["angular_rigidity"] = deflection / force * 1000

        return self.calculated_rigidities

    def accept(self):
        self.calculate_rigidities()
        super().accept()

    def get_rigidities(self):
        return self.calculated_rigidities
