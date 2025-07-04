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
from PySide import QtGui, QtCore
import FreeCAD
import FreeCADGui
from ..models.axis import AngularAxis

translate = FreeCAD.Qt.translate


class PrerequisitesPage(QtGui.QWizardPage):
    def __init__(self):
        super().__init__()
        self.setTitle(translate("CAM", "Prerequisites"))
        layout = QtGui.QGridLayout()
        layout.setContentsMargins(0, 0, 0, 0)
        layout.setSpacing(10)

        # Header
        header_font = QtGui.QFont()
        header_font.setBold(True)
        header_label = QtGui.QLabel(translate("CAM", "You will need:"))
        header_label.setFont(header_font)
        layout.addWidget(header_label, 0, 0, 1, 2)  # Span two columns

        # Prerequisites items
        row = 1
        prerequisites = [
            translate("CAM", "A consistent load applicator (e.g., luggage scale)"),
            translate("CAM", "A micrometer for deflection measurements"),
            translate("CAM", "An inclinometer for angular measurements"),
        ]

        for item_text in prerequisites:
            checkmark_label = QtGui.QLabel("✔")  # Placeholder for checkmark icon
            checkmark_label.setStyleSheet("color: green;")
            item_label = QtGui.QLabel(item_text)
            item_label.setWordWrap(True)
            layout.addWidget(checkmark_label, row, 0, QtCore.Qt.AlignTop)
            layout.addWidget(item_label, row, 1)
            row += 1

        layout.setColumnStretch(0, 0)
        layout.setColumnStretch(1, 1)

        main_layout = QtGui.QVBoxLayout()
        main_layout.addLayout(layout)
        main_layout.addStretch()
        self.setLayout(main_layout)


class TimelineVisualItem(QtGui.QWidget):
    def __init__(self, step_index, total_steps, parent=None):
        super().__init__(parent)
        self.step_index = step_index
        self.total_steps = total_steps
        self.setFixedWidth(20)
        self.setSizePolicy(QtGui.QSizePolicy.Fixed, QtGui.QSizePolicy.Expanding)

    def paintEvent(self, event):
        painter = QtGui.QPainter(self)
        painter.setRenderHint(QtGui.QPainter.Antialiasing)
        line_color = self.palette().highlight().color()
        circle_color = self.palette().highlight().color()
        line_pen = QtGui.QPen(line_color, 2)
        painter.setPen(line_pen)
        circle_brush = QtGui.QBrush(circle_color)
        timeline_x = self.width() // 2
        circle_y = self.height() // 2
        circle_radius = 6

        if self.step_index == 0:  # First step
            painter.drawLine(timeline_x, circle_y, timeline_x, self.height())
        elif self.step_index == self.total_steps - 1:  # Last step
            painter.drawLine(timeline_x, 0, timeline_x, circle_y)
        else:  # Middle steps
            painter.drawLine(timeline_x, 0, timeline_x, self.height())

        painter.setBrush(circle_brush)
        painter.drawEllipse(QtCore.QPoint(timeline_x, circle_y), circle_radius, circle_radius)


class StepContainer(QtGui.QWidget):
    def __init__(self, step_data, input_layout, parent=None):
        super().__init__(parent)
        main_h_layout = QtGui.QHBoxLayout(self)
        main_h_layout.setContentsMargins(0, 0, 0, 0)
        main_h_layout.setSpacing(10)

        # Left side: Timeline visual
        timeline_visual_v_layout = QtGui.QVBoxLayout()
        timeline_visual_v_layout.setContentsMargins(0, 0, 0, 0)
        timeline_visual_v_layout.setSpacing(0)
        timeline_visual_v_layout.addWidget(QtGui.QWidget())  # Add a dummy widget for now
        main_h_layout.addLayout(timeline_visual_v_layout)

        # Right side: Text and Input widgets with margin
        right_v_layout = QtGui.QVBoxLayout()
        right_v_layout.setContentsMargins(10, 10, 10, 10)

        # Title and Subtitle
        text_v_layout = QtGui.QVBoxLayout()
        text_v_layout.setContentsMargins(0, 0, 0, 0)
        text_v_layout.setSpacing(2)

        title_label = QtGui.QLabel(step_data["title"])
        title_font = QtGui.QFont()
        title_font.setPointSize(10)
        title_font.setBold(True)
        title_label.setFont(title_font)
        title_label.setWordWrap(True)
        text_v_layout.addWidget(title_label)

        subtitle_label = QtGui.QLabel(step_data["subtitle"])
        subtitle_font = QtGui.QFont()
        subtitle_font.setPointSize(9)
        subtitle_label.setFont(subtitle_font)
        subtitle_label.setWordWrap(True)
        text_v_layout.addWidget(subtitle_label)

        right_v_layout.addStretch()
        right_v_layout.addLayout(text_v_layout)

        # Input widgets
        if input_layout:
            right_v_layout.addLayout(input_layout)
        right_v_layout.addStretch()

        main_h_layout.addLayout(right_v_layout)

        # Ensure the widget can expand vertically
        self.setSizePolicy(QtGui.QSizePolicy.Preferred, QtGui.QSizePolicy.Preferred)


class TimelineWidget(QtGui.QWidget):
    def __init__(self, step_containers: list[StepContainer], parent=None):
        super().__init__(parent)
        main_layout = QtGui.QVBoxLayout(self)
        main_layout.setContentsMargins(0, 0, 0, 0)
        main_layout.setSpacing(0)

        total_steps = len(step_containers)
        for i, container in enumerate(step_containers):
            timeline_visual_item = TimelineVisualItem(i, total_steps)
            # Replace the dummy widget with the actual TimelineVisualItem
            container.layout().itemAt(0).layout().replaceWidget(
                container.layout().itemAt(0).layout().itemAt(0).widget(), timeline_visual_item
            )
            main_layout.addWidget(container)

        self.setLayout(main_layout)


class LinearRigidityPage(QtGui.QWizardPage):
    def __init__(self, angular_axis: AngularAxis):
        super().__init__()
        self.angular_axis = angular_axis
        self.setTitle(translate("CAM", "Linear Rigidity Measurements"))
        main_layout = QtGui.QVBoxLayout()
        intro = QtGui.QLabel(
            translate("CAM", "Enter measurements for the linear rigidity of the axis.")
        )
        main_layout.addWidget(intro)

        self.inputs = {}
        ui = FreeCADGui.UiLoader()

        # Create input widgets with minimum height
        force_x_edit = ui.createWidget("Gui::QuantitySpinBox")
        force_x_edit.setProperty("unit", FreeCAD.Units.Unit("N"))
        force_x_edit.setProperty("minimum", 0.0)
        force_x_edit.setProperty("value", FreeCAD.Units.Quantity("10.0 N"))
        force_x_edit.setMinimumHeight(30)
        force_x_edit.valueChanged.connect(lambda: self.enforce_unit(force_x_edit, "N"))
        force_x_edit.valueChanged.connect(self.check_completeness)

        deflection_x_edit = ui.createWidget("Gui::QuantitySpinBox")
        deflection_x_edit.setProperty("unit", "mm")
        deflection_x_edit.setProperty("minimum", 0.0)
        deflection_x_edit.setProperty("value", FreeCAD.Units.Quantity("0 mm"))
        deflection_x_edit.setMinimumHeight(30)
        deflection_x_edit.valueChanged.connect(self.check_completeness)

        force_y_edit = ui.createWidget("Gui::QuantitySpinBox")
        force_y_edit.setProperty("unit", FreeCAD.Units.Unit("N"))
        force_y_edit.setMinimumHeight(30)
        force_y_edit.setProperty("value", FreeCAD.Units.Quantity("10.0 N"))
        force_y_edit.valueChanged.connect(lambda: self.enforce_unit(force_y_edit, "N"))
        force_y_edit.valueChanged.connect(self.check_completeness)

        deflection_y_edit = ui.createWidget("Gui::QuantitySpinBox")
        deflection_y_edit.setProperty("unit", "mm")
        deflection_y_edit.setProperty("minimum", 0.0)
        deflection_y_edit.setProperty("value", FreeCAD.Units.Quantity("0 mm"))
        deflection_y_edit.setMinimumHeight(30)
        deflection_y_edit.valueChanged.connect(self.check_completeness)

        # Create grid layouts for X and Y inputs
        x_input_grid = QtGui.QGridLayout()
        x_input_grid.setSpacing(3)
        x_input_grid.addWidget(QtGui.QLabel(translate("CAM", "Force")), 0, 0)
        x_input_grid.addWidget(QtGui.QLabel(translate("CAM", "Deflection")), 0, 1)
        x_input_grid.addWidget(force_x_edit, 1, 0)
        x_input_grid.addWidget(deflection_x_edit, 1, 1)

        y_input_grid = QtGui.QGridLayout()
        y_input_grid.setSpacing(3)
        y_input_grid.addWidget(QtGui.QLabel(translate("CAM", "Force")), 0, 0)
        y_input_grid.addWidget(QtGui.QLabel(translate("CAM", "Deflection")), 0, 1)
        y_input_grid.addWidget(force_y_edit, 1, 0)
        y_input_grid.addWidget(deflection_y_edit, 1, 1)

        # Define steps directly with StepContainer
        step_containers = [
            StepContainer(
                {
                    "title": translate("CAM", "Step 1: Prepare Spindle"),
                    "subtitle": translate("CAM", "Remove tool and collet."),
                },
                None,
            ),
            StepContainer(
                {
                    "title": translate("CAM", "Step 2: Position Machine"),
                    "subtitle": translate(
                        "CAM",
                        "Least rigid configuration (e.g., Z-axis fully extended, X/Y at mid-range).",
                    ),
                },
                None,
            ),
            StepContainer(
                {
                    "title": translate("CAM", "Step 3: Secure Indicator"),
                    "subtitle": translate(
                        "CAM",
                        "Attach dial indicator to the machine table, contacting the spindle nose.",
                    ),
                },
                None,
            ),
            StepContainer(
                {
                    "title": translate("CAM", "Step 4: Measure X-Deflection"),
                    "subtitle": translate(
                        "CAM",
                        "Apply a known force (e.g., 10 N) in the X direction and record the deflection.",
                    ),
                },
                x_input_grid,
            ),
            StepContainer(
                {
                    "title": translate("CAM", "Step 5: Measure Y-Deflection"),
                    "subtitle": translate(
                        "CAM",
                        "Apply a known force (e.g., 10 N) in the Y direction and record the deflection.",
                    ),
                },
                y_input_grid,
            ),
        ]
        timeline_widget = TimelineWidget(step_containers)
        main_layout.addWidget(timeline_widget)

        self.inputs[self.angular_axis.name] = (
            force_x_edit,
            deflection_x_edit,
            force_y_edit,
            deflection_y_edit,
        )

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
        main_layout = QtGui.QVBoxLayout()
        self.setTitle(translate("CAM", "Angular Rigidity Measurements"))
        intro = QtGui.QLabel(
            translate("CAM", "Enter measurements for the angular rigidity of the axis.")
        )
        main_layout.addWidget(intro)

        self.inputs = {}
        ui = FreeCADGui.UiLoader()

        force_edit = ui.createWidget("Gui::QuantitySpinBox")
        force_edit.setProperty("unit", FreeCAD.Units.Unit("N"))
        force_edit.setProperty("minimum", 0.0)
        force_edit.setProperty("value", FreeCAD.Units.Quantity("10.0 N"))
        force_edit.setMinimumHeight(30)
        force_edit.valueChanged.connect(lambda: self.enforce_unit(force_edit, "N"))
        force_edit.valueChanged.connect(self.check_completeness)

        deflection_edit = ui.createWidget("Gui::QuantitySpinBox")
        deflection_edit.setProperty("unit", "deg")
        deflection_edit.setProperty("minimum", 0.0)
        deflection_edit.setProperty("value", FreeCAD.Units.Quantity("0 deg"))
        deflection_edit.setMinimumHeight(30)
        deflection_edit.valueChanged.connect(self.check_completeness)

        input_grid = QtGui.QGridLayout()
        input_grid.setSpacing(3)
        input_grid.addWidget(QtGui.QLabel(translate("CAM", "Force")), 0, 0)
        input_grid.addWidget(QtGui.QLabel(translate("CAM", "Deflection")), 0, 1)
        input_grid.addWidget(force_edit, 1, 0)
        input_grid.addWidget(deflection_edit, 1, 1)

        step_containers = [
            StepContainer(
                {
                    "title": translate("CAM", "Step 1: Prepare Spindle"),
                    "subtitle": translate("CAM", "Remove tool and collet."),
                },
                None,
            ),
            StepContainer(
                {
                    "title": translate("CAM", "Step 2: Attach Inclinometer"),
                    "subtitle": translate("CAM", "Secure an inclinometer to the spindle."),
                },
                None,
            ),
            StepContainer(
                {
                    "title": translate("CAM", "Step 3: Apply Force and Measure Deflection"),
                    "subtitle": translate(
                        "CAM", "Apply a known force (e.g., 10 N) and record the angular deflection."
                    ),
                },
                input_grid,
            ),
        ]
        timeline_widget = TimelineWidget(step_containers)
        main_layout.addWidget(timeline_widget)

        self.inputs[self.angular_axis.name] = (force_edit, deflection_edit)

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
        self.setTitle(translate("CAM", "Review calculated rigidities"))
        main_layout = QtGui.QVBoxLayout()
        self.setButtonText(QtGui.QWizard.FinishButton, translate("CAM", "Apply and Close"))

        self.summary_label = QtGui.QLabel()
        main_layout.addWidget(self.summary_label)
        self.setLayout(main_layout)

    def initializePage(self):
        super().initializePage()
        self.wizard().calculate_rigidities()
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
        self.setMinimumWidth(600)

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
