# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2020 sliptonic <shopinthewoods@gmail.com>               *
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
import Path
import Path.Base.SetupSheetOpPrototype as PathSetupSheetOpPrototype

from PySide import QtCore, QtGui

__title__ = "CAM Property Editor"
__author__ = "sliptonic (Brad Collette)"
__url__ = "https://www.freecad.org"
__doc__ = "Task panel editor for Properties"


if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


class _PropertyEditor(object):
    """Base class of all property editors - just outlines the TableView delegate interface."""

    def __init__(self, obj, prop):
        self.obj = obj
        self.prop = prop

    def widget(self, parent):
        """widget(parent) ... called by the delegate to get a new editor widget.
        Must be implemented by subclasses and return the widget."""
        pass

    def setEditorData(self, widget):
        """setEditorData(widget) ... called by the delegate to initialize the editor.
        The widget is the object returned by widget().
        Must be implemented by subclasses."""
        pass

    def setModelData(self, widget):
        """setModelData(widget) ... called by the delegate to store new values.
        Must be implemented by subclasses."""
        pass

    def propertyValue(self):
        return self.obj.getPropertyByName(self.prop)

    def setProperty(self, value):
        setattr(self.obj, self.prop, value)

    def displayString(self):
        return self.propertyValue()


class _PropertyEditorBool(_PropertyEditor):
    """Editor for boolean values - uses a combo box."""

    def widget(self, parent):
        return QtGui.QComboBox(parent)

    def setEditorData(self, widget):
        widget.clear()
        widget.addItems([str(False), str(True)])
        index = 1 if self.propertyValue() else 0
        widget.setCurrentIndex(index)

    def setModelData(self, widget):
        self.setProperty(widget.currentText() == str(True))


class _PropertyEditorString(_PropertyEditor):
    """Editor for string values - uses a line edit."""

    def widget(self, parent):
        return QtGui.QLineEdit(parent)

    def setEditorData(self, widget):
        text = "" if self.propertyValue() is None else self.propertyValue()
        widget.setText(text)

    def setModelData(self, widget):
        self.setProperty(widget.text())


class _PropertyEditorQuantity(_PropertyEditor):
    def widget(self, parent):
        return QtGui.QLineEdit(parent)

    def setEditorData(self, widget):
        quantity = self.propertyValue()
        if quantity is None:
            quantity = self.defaultQuantity()
        widget.setText(quantity.getUserPreferred()[0])

    def defaultQuantity(self):
        pass

    def setModelData(self, widget):
        self.setProperty(FreeCAD.Units.Quantity(widget.text()))

    def displayString(self):
        if self.propertyValue() is None:
            return ""
        return self.propertyValue().getUserPreferred()[0]


class _PropertyEditorAngle(_PropertyEditorQuantity):
    """Editor for angle values - uses a line edit"""

    def defaultQuantity(self):
        return FreeCAD.Units.Quantity(0, FreeCAD.Units.Angle)


class _PropertyEditorLength(_PropertyEditorQuantity):
    """Editor for length values - uses a line edit."""

    def defaultQuantity(self):
        return FreeCAD.Units.Quantity(0, FreeCAD.Units.Length)


class _PropertyEditorPercent(_PropertyEditor):
    """Editor for percent values - uses a spin box."""

    def widget(self, parent):
        return QtGui.QSpinBox(parent)

    def setEditorData(self, widget):
        widget.setRange(0, 100)
        value = self.propertyValue()
        if value is None:
            value = 0
        widget.setValue(value)

    def setModelData(self, widget):
        self.setProperty(widget.value())


class _PropertyEditorInteger(_PropertyEditor):
    """Editor for integer values - uses a spin box."""

    def widget(self, parent):
        return QtGui.QSpinBox(parent)

    def setEditorData(self, widget):
        value = self.propertyValue()
        if value is None:
            value = 0
        widget.setValue(value)

    def setModelData(self, widget):
        self.setProperty(widget.value())


class _PropertyEditorFloat(_PropertyEditor):
    """Editor for float values - uses a double spin box."""

    def widget(self, parent):
        return QtGui.QDoubleSpinBox(parent)

    def setEditorData(self, widget):
        value = self.propertyValue()
        if value is None:
            value = 0.0
        widget.setValue(value)

    def setModelData(self, widget):
        self.setProperty(widget.value())


class _PropertyEditorFile(_PropertyEditor):
    def widget(self, parent):
        return QtGui.QLineEdit(parent)

    def setEditorData(self, widget):
        text = "" if self.propertyValue() is None else self.propertyValue()
        widget.setText(text)

    def setModelData(self, widget):
        self.setProperty(widget.text())


class _PropertyEditorEnumeration(_PropertyEditor):
    def widget(self, parent):
        return QtGui.QComboBox(parent)

    def setEditorData(self, widget):
        widget.clear()
        widget.addItems(self.obj.getEnumerationsOfProperty(self.prop))
        widget.setCurrentText(self.propertyValue())

    def setModelData(self, widget):
        self.setProperty(widget.currentText())


_EditorFactory = {
    "App::PropertyAngle": _PropertyEditorAngle,
    "App::PropertyBool": _PropertyEditorBool,
    "App::PropertyDistance": _PropertyEditorLength,
    "App::PropertyEnumeration": _PropertyEditorEnumeration,
    #'App::PropertyFile'        : _PropertyEditorFile,
    "App::PropertyFloat": _PropertyEditorFloat,
    "App::PropertyInteger": _PropertyEditorInteger,
    "App::PropertyLength": _PropertyEditorLength,
    "App::PropertyPercent": _PropertyEditorPercent,
    "App::PropertyString": _PropertyEditorString,
}


def Types():
    """Return the types of properties supported."""
    return [t for t in _EditorFactory]


def Editor(obj, prop):
    """Returns an editor class to be used for the given property."""
    factory = _EditorFactory[obj.getTypeIdOfProperty(prop)]
    if factory:
        return factory(obj, prop)
    return None
