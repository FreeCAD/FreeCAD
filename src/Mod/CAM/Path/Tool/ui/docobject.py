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

"""Widget for editing a list of properties of a DocumentObject."""

import re
from PySide import QtGui, QtCore
from .property import BasePropertyEditorWidget


def _get_label_text(prop_name):
    """Generate a human-readable label from a property name."""
    # Add space before capital letters (CamelCase splitting)
    s1 = re.sub(r"([A-Z][a-z]+)", r" \1", prop_name)
    # Add space before sequences of capitals (e.g., ID) followed by lowercase
    s2 = re.sub(r"([A-Z]+)([A-Z][a-z])", r"\1 \2", s1)
    # Add space before sequences of capitals followed by end of string
    s3 = re.sub(r"([A-Z]+)$", r" \1", s2)
    # Remove leading/trailing spaces and capitalize
    return s3.strip().capitalize()


class DocumentObjectEditorWidget(QtGui.QWidget):
    """
    A widget that displays a user friendly form for editing properties of a
    FreeCAD DocumentObject.
    """

    # Signal emitted when any underlying property value might have changed
    propertyChanged = QtCore.Signal()

    def __init__(self, obj=None, properties_to_show=None, property_suffixes=None, parent=None):
        """
        Initialize the editor widget.

        Args:
            obj (App.DocumentObject, optional): The object to edit. Defaults to None.
            properties_to_show (list[str], optional): List of property names to display.
                                                     Defaults to None (shows nothing).
            property_suffixes (dict[str, str], optional): Dictionary mapping property names
                                                          to suffixes for their labels.
                                                          Defaults to None.
            parent (QWidget, optional): The parent widget. Defaults to None.
        """
        super().__init__(parent)
        self._obj = obj
        self._properties_to_show = properties_to_show if properties_to_show else []
        self._property_suffixes = property_suffixes if property_suffixes else {}
        self._property_editors = {}  # Store {prop_name: editor_widget}

        self._layout = QtGui.QFormLayout(self)
        self._layout.setContentsMargins(0, 0, 0, 0)
        self._layout.setFieldGrowthPolicy(QtGui.QFormLayout.FieldGrowthPolicy.ExpandingFieldsGrow)

        self._populate_form()

    def _clear_form(self):
        """Remove all rows from the form layout."""
        while self._layout.rowCount() > 0:
            self._layout.removeRow(0)
        self._property_editors.clear()

    def _populate_form(self):
        """Create and add property editors to the form."""
        self._clear_form()
        if not self._obj:
            return

        for prop_name in self._properties_to_show:
            # Only create an editor if the property exists on the object
            if not hasattr(self._obj, prop_name):
                continue

            editor_widget = BasePropertyEditorWidget.for_property(self._obj, prop_name, self)
            label_text = _get_label_text(prop_name)
            suffix = self._property_suffixes.get(prop_name)
            if suffix:
                label_text = f"{label_text} ({suffix}):"
            else:
                label_text = f"{label_text}:"

            label = QtGui.QLabel(label_text)
            self._layout.addRow(label, editor_widget)
            self._property_editors[prop_name] = editor_widget

            # Connect the editor's signal to our own signal
            editor_widget.propertyChanged.connect(self.propertyChanged)

    def setObject(self, obj):
        """Set or change the DocumentObject being edited."""
        if obj != self._obj:
            self._obj = obj
            # Re-populate might be too slow if only object changes,
            # better to just re-attach existing editors.
            # self._populate_form()
            for prop_name, editor in self._property_editors.items():
                editor.attachTo(self._obj, prop_name)

    def setPropertiesToShow(self, properties_to_show, property_suffixes=None):
        """Set or change the list of properties to display."""
        self._properties_to_show = properties_to_show if properties_to_show else []
        self._property_suffixes = property_suffixes if property_suffixes else {}
        self._populate_form()  # Rebuild the form completely

    def updateUI(self):
        """Update all child editor widgets from the object's properties."""
        for editor in self._property_editors.values():
            editor.updateWidget()

    def updateObject(self):
        """Update the object's properties from all child editor widgets."""
        # This might not be strictly necessary if signals are connected,
        # but can be useful for explicit save actions.
        for editor in self._property_editors.values():
            editor.updateProperty()
