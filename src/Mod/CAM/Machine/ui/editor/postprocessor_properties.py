# SPDX-License-Identifier: LGPL-2.1-or-later
# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2025 sliptonic <shopinthewoods@gmail.com>               *
# *                                                                         *
# *   This file is part of FreeCAD.                                         *
# *                                                                         *
# *   FreeCAD is free software: you can redistribute it and/or modify it    *
# *   under the terms of the GNU Lesser General Public License as           *
# *   published by the Free Software Foundation, either version 2.1 of the  *
# *   License, or (at your option) any later version.                       *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful, but        *
# *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
# *   Lesser General Public License for more details.                       *
# *                                                                         *
# *   You should have received a copy of the GNU Lesser General Public      *
# *   License along with FreeCAD. If not, see                               *
# *   <https://www.gnu.org/licenses/>.                                      *
# *                                                                         *
# ***************************************************************************
"""
Helper module for managing postprocessor properties in the machine editor.
"""
from PySide import QtGui
from typing import Dict, Any


class PostProcessorPropertyManager:
    """Manages dynamic creation and handling of postprocessor property widgets."""

    @staticmethod
    def create_property_widget(prop: Dict[str, Any], current_value: Any) -> QtGui.QWidget:
        """Create a widget for a postprocessor property based on its type.
        
        Args:
            prop: Property schema dictionary with keys: name, type, label, default, help, etc.
            current_value: Current value of the property
            
        Returns:
            QWidget configured for the property type, with a value_getter() method
        """
        prop_type = prop.get('type')
        
        if prop_type == 'bool':
            widget = QtGui.QCheckBox()
            widget.setChecked(current_value if current_value is not None else False)
            widget.value_getter = lambda: widget.isChecked()
            return widget
        
        elif prop_type == 'int':
            widget = QtGui.QSpinBox()
            widget.setRange(prop.get('min', -999999), prop.get('max', 999999))
            widget.setValue(current_value if current_value is not None else 0)
            widget.value_getter = lambda: widget.value()
            return widget
        
        elif prop_type == 'float':
            widget = QtGui.QDoubleSpinBox()
            widget.setRange(prop.get('min', -999999.0), prop.get('max', 999999.0))
            widget.setDecimals(prop.get('decimals', 4))
            widget.setValue(current_value if current_value is not None else 0.0)
            widget.value_getter = lambda: widget.value()
            return widget
        
        elif prop_type == 'choice':
            widget = QtGui.QComboBox()
            choices = prop.get('choices', [])
            for choice in choices:
                widget.addItem(str(choice), choice)
            if current_value:
                index = widget.findData(current_value)
                if index >= 0:
                    widget.setCurrentIndex(index)
            widget.value_getter = lambda: widget.itemData(widget.currentIndex())
            return widget
        
        elif prop_type == 'text':
            widget = QtGui.QPlainTextEdit()
            widget.setMaximumHeight(100)
            widget.setPlainText(current_value if current_value else "")
            widget.value_getter = lambda: widget.toPlainText()
            return widget
        
        elif prop_type == 'file':
            # File picker widget
            container = QtGui.QWidget()
            layout = QtGui.QHBoxLayout(container)
            layout.setContentsMargins(0, 0, 0, 0)
            
            line_edit = QtGui.QLineEdit()
            line_edit.setText(current_value if current_value else "")
            
            browse_button = QtGui.QPushButton("...")
            browse_button.setMaximumWidth(30)
            
            def browse_file():
                filters = prop.get('filters', "All Files (*)")
                filename, _ = QtGui.QFileDialog.getOpenFileName(container, "Select File", "", filters)
                if filename:
                    line_edit.setText(filename)
            
            browse_button.clicked.connect(browse_file)
            
            layout.addWidget(line_edit)
            layout.addWidget(browse_button)
            
            container.value_getter = lambda: line_edit.text()
            return container
        
        else:  # Default to string
            widget = QtGui.QLineEdit()
            widget.setText(str(current_value) if current_value else "")
            widget.value_getter = lambda: widget.text()
            return widget

    @staticmethod
    def connect_property_widget(widget: QtGui.QWidget, prop_name: str, prop_type: str, 
                                machine, callback=None):
        """Connect a property widget to update the machine configuration.
        
        Args:
            widget: The widget to connect
            prop_name: Name of the property
            prop_type: Type of the property
            machine: Machine object to update
            callback: Optional callback function to call after update
        """
        def update_property():
            if hasattr(widget, 'value_getter'):
                value = widget.value_getter()
                machine.postprocessor_properties[prop_name] = value
                if callback:
                    callback(prop_name, value)
        
        # Connect appropriate signal based on widget type
        if isinstance(widget, QtGui.QCheckBox):
            widget.stateChanged.connect(update_property)
        elif isinstance(widget, QtGui.QSpinBox) or isinstance(widget, QtGui.QDoubleSpinBox):
            widget.valueChanged.connect(update_property)
        elif isinstance(widget, QtGui.QComboBox):
            widget.currentIndexChanged.connect(update_property)
        elif isinstance(widget, QtGui.QPlainTextEdit):
            widget.textChanged.connect(update_property)
        elif isinstance(widget, QtGui.QLineEdit):
            widget.textChanged.connect(update_property)
        elif isinstance(widget, QtGui.QWidget):
            # For composite widgets (like file picker), find the line edit
            line_edit = widget.findChild(QtGui.QLineEdit)
            if line_edit:
                line_edit.textChanged.connect(update_property)
