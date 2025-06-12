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

"""
Widgets for editing specific types of DocumentObject properties.
Includes a factory method to create the appropriate widget based on type.
"""

import FreeCAD
import FreeCADGui
from PySide import QtGui, QtCore
from typing import Optional


class BasePropertyEditorWidget(QtGui.QWidget):
    """
    Base class for property editor widgets. Includes a factory method
    to create specific subclasses based on property type.
    """

    # Signal emitted when the underlying property value might have changed
    propertyChanged = QtCore.Signal()

    def __init__(self, obj: FreeCAD.DocumentObject, prop_name: str, parent: QtGui.QWidget = None):
        super().__init__(parent)
        self._obj = obj
        self._prop_name = prop_name
        self._layout = QtGui.QHBoxLayout(self)
        self._layout.setContentsMargins(0, 0, 0, 0)
        self._editor_widget: QtGui.QWidget = None  # The actual input widget (SpinBox, ComboBox)
        self._editor_mode: int = 0  # Default to editable
        self._is_read_only: bool = False
        self._update_editor_mode()
        self.setSizePolicy(QtGui.QSizePolicy.Expanding, QtGui.QSizePolicy.Expanding)

    def attachTo(self, obj: FreeCAD.DocumentObject, prop_name: Optional[str] = None):
        """Attach the editor to a (potentially different) object/property."""
        self._obj = obj
        self._prop_name = prop_name if prop_name else self._prop_name
        self._update_editor_mode()
        self.updateWidget()  # Ensure widget reflects new state

    def _update_editor_mode(self):
        """Fetch and store the current editor mode for the property."""
        if self._obj and self._prop_name:
            self._editor_mode = self._obj.getEditorMode(self._prop_name)
            self._is_read_only = self._editor_mode == 2
            return
        self._editor_mode = 0
        self._is_read_only = False

    def updateWidget(self):
        """Update the editor widget's display from the object property."""
        # Implementation specific to subclasses
        raise NotImplementedError

    def updateProperty(self):
        """Update the object property from the editor widget's value."""
        # Implementation specific to subclasses
        # Should emit propertyChanged signal if value actually changes
        raise NotImplementedError

    @classmethod
    def for_property(
        cls, obj: FreeCAD.DocumentObject, prop_name: str, parent: QtGui.QWidget = None
    ) -> "BasePropertyEditorWidget":
        """
        Factory method to create the appropriate editor widget subclass.
        """
        if not obj or not hasattr(obj, "getPropertyByName"):
            return LabelPropertyEditorWidget(obj, prop_name, parent)

        prop_value = obj.getPropertyByName(prop_name)
        prop_type = obj.getTypeIdOfProperty(prop_name)

        if isinstance(prop_value, FreeCAD.Units.Quantity):
            return QuantityPropertyEditorWidget(obj, prop_name, parent)
        elif isinstance(prop_value, bool):
            return BoolPropertyEditorWidget(obj, prop_name, parent)
        elif isinstance(prop_value, int):
            return IntPropertyEditorWidget(obj, prop_name, parent)
        elif prop_type == "App::PropertyEnumeration":
            return EnumPropertyEditorWidget(obj, prop_name, parent)
        else:
            # Default to a read-only label for other types
            return LabelPropertyEditorWidget(obj, prop_name, parent)


class QuantityPropertyEditorWidget(BasePropertyEditorWidget):
    """Editor widget for Quantity properties."""

    def __init__(self, obj: FreeCAD.DocumentObject, prop_name: str, parent: QtGui.QWidget = None):
        super().__init__(obj, prop_name, parent)
        ui = FreeCADGui.UiLoader()
        self._editor_widget: FreeCADGui.QuantitySpinBox = ui.createWidget("Gui::QuantitySpinBox")
        self._layout.addWidget(self._editor_widget)
        self.updateWidget()  # Set initial value
        # Connect signal after setting initial value to avoid premature update
        self._editor_widget.editingFinished.connect(self.updateProperty)

    def updateWidget(self):
        value: FreeCAD.Units.Quantity = self._obj.getPropertyByName(self._prop_name)
        # Block signals temporarily to prevent feedback loops
        self._editor_widget.blockSignals(True)
        self._editor_widget.setProperty("value", value)
        self._editor_widget.blockSignals(False)
        self._editor_widget.setEnabled(not self._is_read_only)

    def updateProperty(self):
        current_value = self._obj.getPropertyByName(self._prop_name)
        new_value_str: str = self._editor_widget.property("value").UserString
        new_value = FreeCAD.Units.Quantity(new_value_str)
        if new_value_str != current_value:
            self._obj.setPropertyByName(self._prop_name, new_value)
            self.propertyChanged.emit()


class BoolPropertyEditorWidget(BasePropertyEditorWidget):
    """Editor widget for Boolean properties."""

    def __init__(self, obj: FreeCAD.DocumentObject, prop_name: str, parent: QtGui.QWidget = None):
        super().__init__(obj, prop_name, parent)
        self._editor_widget: QtGui.QComboBox = QtGui.QComboBox()
        self._editor_widget.addItems(["False", "True"])
        self._layout.addWidget(self._editor_widget)
        self.updateWidget()
        self._editor_widget.currentIndexChanged.connect(self._on_index_changed)

    def updateWidget(self):
        value: bool = self._obj.getPropertyByName(self._prop_name)
        self._editor_widget.blockSignals(True)
        self._editor_widget.setCurrentIndex(1 if value else 0)
        self._editor_widget.blockSignals(False)
        self._editor_widget.setEnabled(not self._is_read_only)

    def _on_index_changed(self, index: int):
        """Slot connected to currentIndexChanged signal."""
        if self._is_read_only:
            return
        current_value: bool = self._obj.getPropertyByName(self._prop_name)
        new_value: bool = bool(index)
        if new_value != current_value:
            self._obj.setPropertyByName(self._prop_name, new_value)
            self.propertyChanged.emit()

    def updateProperty(self):
        """Update property based on current widget state (for consistency)."""
        self._on_index_changed(self._editor_widget.currentIndex())


class IntPropertyEditorWidget(BasePropertyEditorWidget):
    """Editor widget for Integer properties."""

    def __init__(self, obj: FreeCAD.DocumentObject, prop_name: str, parent: QtGui.QWidget = None):
        super().__init__(obj, prop_name, parent)
        self._editor_widget: QtGui.QSpinBox = QtGui.QSpinBox()
        self._editor_widget.setMinimum(-2147483648)
        self._editor_widget.setMaximum(2147483647)
        self._layout.addWidget(self._editor_widget)
        self.updateWidget()
        self._editor_widget.editingFinished.connect(self.updateProperty)

    def updateWidget(self):
        value = self._obj.getPropertyByName(self._prop_name)
        self._editor_widget.blockSignals(True)
        self._editor_widget.setValue(value or 0)
        self._editor_widget.blockSignals(False)
        self._editor_widget.setEnabled(not self._is_read_only)

    def updateProperty(self):
        current_value: int = self._obj.getPropertyByName(self._prop_name)
        new_value: int = self._editor_widget.value()
        if new_value != current_value:
            self._obj.setPropertyByName(self._prop_name, new_value)
            self.propertyChanged.emit()


class EnumPropertyEditorWidget(BasePropertyEditorWidget):
    """Editor widget for Enumeration properties."""

    def __init__(self, obj: FreeCAD.DocumentObject, prop_name: str, parent: QtGui.QWidget = None):
        super().__init__(obj, prop_name, parent)
        self._editor_widget: QtGui.QComboBox = QtGui.QComboBox()
        self._layout.addWidget(self._editor_widget)
        self._populate_enum()
        self.updateWidget()
        self._editor_widget.currentIndexChanged.connect(self._on_index_changed)

    def _populate_enum(self):
        self._editor_widget.clear()
        enums: list[str] = self._obj.getEnumerationsOfProperty(self._prop_name)
        self._editor_widget.addItems(enums)

    def attachTo(self, obj: FreeCAD.DocumentObject, prop_name: Optional[str] = None):
        """Override attachTo to repopulate enums if object changes."""
        super().attachTo(obj, prop_name)
        self._populate_enum()  # Repopulate in case enums are different

    def updateWidget(self):
        value: str = self._obj.getPropertyByName(self._prop_name)
        self._editor_widget.blockSignals(True)
        index: int = self._editor_widget.findText(value)
        self._editor_widget.setCurrentIndex(index if index >= 0 else 0)
        self._editor_widget.blockSignals(False)
        self._editor_widget.setEnabled(not self._is_read_only)

    def _on_index_changed(self, index: int):
        """Slot connected to currentIndexChanged signal."""
        if self._is_read_only:
            return
        current_value: str = self._obj.getPropertyByName(self._prop_name)
        new_value: str = self._editor_widget.itemText(index)
        if new_value != current_value:
            self._obj.setPropertyByName(self._prop_name, new_value)
            self.propertyChanged.emit()

    def updateProperty(self):
        """Update property based on current widget state (for consistency)."""
        self._on_index_changed(self._editor_widget.currentIndex())


class LabelPropertyEditorWidget(BasePropertyEditorWidget):
    """Read-only label for unsupported or invalid property types."""

    def __init__(self, obj: FreeCAD.DocumentObject, prop_name: str, parent: QtGui.QWidget = None):
        super().__init__(obj, prop_name, parent)
        self._editor_widget: QtGui.QLabel = QtGui.QLabel("N/A")
        self._editor_widget.setTextInteractionFlags(
            QtGui.Qt.TextSelectableByMouse | QtGui.Qt.TextSelectableByKeyboard
        )
        self._layout.addWidget(self._editor_widget)
        self.updateWidget()

    def updateWidget(self):
        text = "N/A"
        text: str = str(self._obj.getPropertyByName(self._prop_name))
        self._editor_widget.setText(text)

    def updateProperty(self):
        # Read-only, no action needed
        pass
