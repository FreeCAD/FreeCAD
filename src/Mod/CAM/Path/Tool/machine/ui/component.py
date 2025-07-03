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
import FreeCAD
from PySide import QtGui, QtCore
from typing import List, Tuple, Union

from ..models.component import MachineComponent, AttributeConfig
from .spinbox import (
    VelocitySpinBox,
    AngularRigiditySpinBox,
    MmPerNewtonSpinBox,
    PowerSpinBox,
    RPMSpinBox,
    TorqueSpinBox,
)


class MachineComponentWidget(QtGui.QWidget):
    """
    A generic UI widget for displaying and editing MachineComponent attributes.
    Dynamically creates input fields based on AttributeConfig.
    """

    labelChanged = QtCore.Signal(str)

    def __init__(
        self, component: MachineComponent, attribute_configs: List[AttributeConfig], parent=None
    ):
        super().__init__(parent)
        self.component = component
        self.attribute_configs = attribute_configs
        self.widgets = {}  # Stores references to created UI widgets

        self.setLayout(QtGui.QFormLayout())

        self._setup_ui()

    def _setup_ui(self):
        """Dynamically creates UI widgets based on attribute configurations."""
        layout = self.layout()
        for config in self.attribute_configs:
            widget = self._create_widget_for_attribute(config)
            if widget:
                layout.addRow(config.label, widget)
                self.widgets[config.name] = widget
                self._set_initial_value(widget, config)
                self._connect_widget_signals(widget, config.name)

    def _create_widget_for_attribute(self, config: AttributeConfig):
        """Creates and returns the appropriate widget for a given attribute config."""
        if config.readonly:
            label = QtGui.QLabel()
            # Set initial value for the label
            value = getattr(self.component, config.name, None)
            if value is not None:
                if isinstance(value, FreeCAD.Units.Quantity):
                    label.setText(str(value.UserString))
                else:
                    label.setText(str(value))
            return label
        elif config.property_type == "App::PropertyVelocity":
            spinbox = VelocitySpinBox()
            spinbox.setMinimum(config.min_value if config.min_value is not None else -1e9)
            spinbox.setMaximum(config.max_value if config.max_value is not None else 1e9)
            spinbox.setDecimals(config.decimals if config.decimals is not None else 3)
            return spinbox
        elif config.property_type == "App::PropertyAngularRigidity":
            spinbox = AngularRigiditySpinBox()
            spinbox.setMinimum(config.min_value if config.min_value is not None else 0.0)
            spinbox.setMaximum(config.max_value if config.max_value is not None else 1e9)
            spinbox.setDecimals(config.decimals if config.decimals is not None else 3)
            return spinbox
        elif config.property_type == "App::PropertyRigidity":
            spinbox = MmPerNewtonSpinBox()
            spinbox.setMinimum(config.min_value if config.min_value is not None else 0.0)
            spinbox.setMaximum(config.max_value if config.max_value is not None else 1e9)
            spinbox.setDecimals(config.decimals if config.decimals is not None else 3)
            return spinbox
        elif config.property_type == "App::PropertyPower":
            spinbox = PowerSpinBox()
            spinbox.setMinimum(config.min_value if config.min_value is not None else 0.0)
            spinbox.setMaximum(config.max_value if config.max_value is not None else 1e9)
            spinbox.setDecimals(config.decimals if config.decimals is not None else 3)
            return spinbox
        elif config.property_type == "App::PropertyAngularSpeed":
            spinbox = RPMSpinBox()
            spinbox.setMinimum(int(config.min_value) if config.min_value is not None else 0)
            spinbox.setMaximum(int(config.max_value) if config.max_value is not None else 200000)
            return spinbox
        elif config.property_type == "App::PropertyTorque":
            spinbox = TorqueSpinBox()
            spinbox.setMinimum(config.min_value if config.min_value is not None else 0.0)
            spinbox.setMaximum(config.max_value if config.max_value is not None else 1e9)
            spinbox.setDecimals(config.decimals if config.decimals is not None else 3)
            return spinbox
        elif config.property_type == "App::PropertyFloat":
            spinbox = QtGui.QDoubleSpinBox()
            spinbox.setMinimum(config.min_value if config.min_value is not None else -1e9)
            spinbox.setMaximum(config.max_value if config.max_value is not None else 1e9)
            spinbox.setDecimals(config.decimals if config.decimals is not None else 3)
            return spinbox
        elif config.property_type == "App::PropertyInteger":
            spinbox = QtGui.QSpinBox()
            spinbox.setMinimum(int(config.min_value) if config.min_value is not None else -1e9)
            spinbox.setMaximum(int(config.max_value) if config.max_value is not None else 1e9)
            return spinbox
        elif config.property_type == "App::PropertyString":
            line_edit = QtGui.QLineEdit()
            if config.name == "label":
                line_edit.textChanged.connect(self.labelChanged.emit)
            return line_edit
        elif config.property_type == "App::PropertyBool":
            checkbox = QtGui.QCheckBox()
            return checkbox
        # Add more property types as needed
        return None

    def _set_initial_value(self, widget, config: AttributeConfig):
        """Sets the initial value of the widget from the component's attribute."""
        value = getattr(self.component, config.name, None)
        if value is not None:
            if isinstance(widget, (QtGui.QDoubleSpinBox, QtGui.QSpinBox)):
                if isinstance(value, FreeCAD.Units.Quantity):
                    # For Quantity objects, set the value directly if the spinbox
                    # supports Quantity (like custom spinboxes), otherwise convert
                    # to a float for generic QDoubleSpinBox.
                    try:
                        widget.setValue(value)
                    except TypeError:
                        # Fallback for generic QDoubleSpinBox if custom setValue
                        # is not implemented for Quantity
                        widget.setValue(value.getValueAs(value.Unit))
                else:
                    widget.setValue(value)
            elif isinstance(widget, QtGui.QLineEdit):
                widget.setText(str(value))
            elif isinstance(widget, QtGui.QCheckBox):
                widget.setChecked(bool(value))

    def _connect_widget_signals(self, widget, attribute_name):
        """Connects the appropriate signal of the widget to update the component."""
        if isinstance(widget, (QtGui.QDoubleSpinBox, QtGui.QSpinBox)):
            widget.valueChanged.connect(
                lambda: setattr(self.component, attribute_name, widget.value())
            )
        elif isinstance(widget, QtGui.QLineEdit):
            widget.textChanged.connect(
                lambda: setattr(self.component, attribute_name, widget.text())
            )
        elif isinstance(widget, QtGui.QCheckBox):
            widget.toggled.connect(
                lambda: setattr(self.component, attribute_name, widget.isChecked())
            )

    def update_ui_from_component(self):
        """
        Updates the UI widgets with the current values from the component.
        """
        for config in self.attribute_configs:
            widget = self.widgets.get(config.name)
            if widget:
                self._set_initial_value(widget, config)

    def set_visibility(self, property_type: Union[str, List[str], Tuple[str, ...]], visible: bool):
        """
        Sets the visibility of the widgets with the given property type.
        """
        if isinstance(property_type, str):
            property_type = [property_type]  # Convert single string to list
        for config in self.attribute_configs:
            if config.property_type not in property_type:
                continue

            widget = self.widgets.get(config.name)
            if not widget:
                continue

            # Hide the widget and its label in the form layout
            widget.setVisible(visible)
            layout = self.layout()
            for i in range(layout.rowCount()):
                if layout.itemAt(i, QtGui.QFormLayout.LabelRole) and layout.itemAt(
                    i, QtGui.QFormLayout.FieldRole
                ) == layout.itemAt(layout.indexOf(widget)):
                    label_widget = layout.itemAt(i, QtGui.QFormLayout.LabelRole).widget()
                    if label_widget:
                        label_widget.setVisible(visible)
                    break

    def validate_inputs(self) -> bool:
        """
        Performs basic validation on input values.
        Returns True if all inputs are valid, False otherwise.
        Displays QMessageBox for failures.
        """
        for config in self.attribute_configs:
            widget = self.widgets.get(config.name)
            if widget:
                current_value = None
                if isinstance(widget, (QtGui.QDoubleSpinBox, QtGui.QSpinBox)):
                    current_value = widget.value()
                    if isinstance(current_value, FreeCAD.Units.Quantity):
                        current_value = current_value.getValueAs(current_value.Unit)

                    if config.min_value is not None and current_value < config.min_value:
                        QtGui.QMessageBox.warning(
                            self,
                            "Validation Error",
                            f"Value for '{config.label}' cannot be less than "
                            f"{config.min_value}.",
                        )
                        return False
                    if config.max_value is not None and current_value > config.max_value:
                        QtGui.QMessageBox.warning(
                            self,
                            "Validation Error",
                            f"Value for '{config.label}' cannot be greater than "
                            f"{config.max_value}.",
                        )
                        return False
                # Add validation for other widget types if necessary
        return True
