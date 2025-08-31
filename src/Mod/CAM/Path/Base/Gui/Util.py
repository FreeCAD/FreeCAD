# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2017 sliptonic <shopinthewoods@gmail.com>               *
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
import FreeCADGui
import Path
import Path.Base.Util as PathUtil
from PySide import QtGui, QtCore

from PySide import QtCore, QtGui

__title__ = "CAM UI helper and utility functions"
__author__ = "sliptonic (Brad Collette)"
__url__ = "https://www.freecad.org"
__doc__ = "A collection of helper and utility functions for the CAM GUI."


if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


def populateCombobox(form, enumTups, comboBoxesPropertyMap):
    """populateCombobox(form, enumTups, comboBoxesPropertyMap) ... populate comboboxes with translated enumerations
    ** comboBoxesPropertyMap will be unnecessary if UI files use strict combobox naming protocol.
    Args:
        form = UI form
        enumTups = list of (translated_text, data_string) tuples
        comboBoxesPropertyMap = list of (translated_text, data_string) tuples
    """
    Path.Log.track(enumTups)

    # Load appropriate enumerations in each combobox
    for cb, prop in comboBoxesPropertyMap:
        box = getattr(form, cb)  # Get the combobox
        box.clear()  # clear the combobox
        for text, data in enumTups[prop]:  #  load enumerations
            box.addItem(text, data)


def updateInputField(obj, prop, widget, onBeforeChange=None):
    """updateInputField(obj, prop, widget) ... update obj's property prop with the value of widget.
    The property's value is only assigned if the new value differs from the current value.
    This prevents onChanged notifications where the value didn't actually change.
    Gui::InputField and Gui::QuantitySpinBox widgets are supported - and the property can
    be of type Quantity or Float.
    If onBeforeChange is specified it is called before a new value is assigned to the property.
    Returns True if a new value was assigned, False otherwise (new value is the same as the current).
    """
    value = widget.property("rawValue")
    Path.Log.track("value: {}".format(value))
    attr = PathUtil.getProperty(obj, prop)
    attrValue = attr.Value if hasattr(attr, "Value") else attr

    isDiff = False
    if not Path.Geom.isRoughly(attrValue, value):
        isDiff = True
    else:
        if hasattr(obj, "ExpressionEngine"):
            exprSet = False
            for prp, expr in obj.ExpressionEngine:
                if prp == prop:
                    exprSet = True
                    Path.Log.debug('prop = "expression": {} = "{}"'.format(prp, expr))
                    value = FreeCAD.Units.Quantity(obj.evalExpression(expr)).Value
                    if not Path.Geom.isRoughly(attrValue, value):
                        isDiff = True
                    break
            if exprSet:
                widget.setReadOnly(True)
                widget.setProperty("exprSet", "true")
                widget.style().unpolish(widget)
                widget.ensurePolished()
            else:
                widget.setReadOnly(False)
                widget.setProperty("exprSet", "false")
                widget.style().unpolish(widget)
                widget.ensurePolished()
            widget.update()

    if isDiff:
        Path.Log.debug("updateInputField(%s, %s): %.2f -> %.2f" % (obj.Label, prop, attr, value))
        if onBeforeChange:
            onBeforeChange(obj)
        PathUtil.setProperty(obj, prop, value)
        return True

    return False


class QuantitySpinBox(QtCore.QObject):
    """Controller class to interface a Gui::QuantitySpinBox.
    The spin box gets bound to a given property and supports update in both directions.
    QuatitySpinBox(widget, obj, prop, onBeforeChange=None)
            widget ... expected to be reference to a Gui::QuantitySpinBox
            obj    ... document object
            prop   ... canonical name of the (sub-) property
            onBeforeChange ... an optional callback being executed before the value of the property is changed
    """

    def __init__(self, widget, obj, prop, onBeforeChange=None):
        super().__init__()
        Path.Log.track(widget)
        self.widget = widget
        self.onBeforeChange = onBeforeChange
        self.prop = None
        self.obj = obj
        self.lastWidgetText = self.widget.text()
        self.attachTo(obj, prop)
        self.widget.installEventFilter(self)
        # Connect local class method as slot
        self.widget.textChanged.connect(self.onWidgetValueChanged)

    def eventFilter(self, obj, event):
        if event.type() == QtCore.QEvent.Type.FocusIn:
            self.updateWidget()
        return False

    def onWidgetValueChanged(self):
        """
        Slot method for determining if a change
        in widget value is a result of an expression edit, or a simple spinbox change.
        If the former, emit a manual `editingFinished` signal because the Expression editor
        window returned a value to the base widget, leaving it in read-only mode,
        and finishing the editing of the value. Otherwise, due nothing if the value
        has not changed, or there is no active expression for the property.
        If the user closes the Expression editor to cancel the edit, the value will not
        be changed, and this manual signal will not be emitted."""
        if self._hasExpression() and self.widget.text() != self.lastWidgetText:
            self.widget.editingFinished.emit()

    def attachTo(self, obj, prop=None):
        """use an existing editor for the given object and property"""
        Path.Log.track(self.prop, prop)
        self.obj = obj
        self.prop = prop
        if obj and prop:
            attr = PathUtil.getProperty(obj, prop)
            if attr is not None:
                if hasattr(attr, "Value"):
                    self.widget.setProperty("unit", attr.getUserPreferred()[2])
                self.widget.setProperty("binding", "%s.%s" % (obj.Name, prop))
                self.valid = True
            else:
                Path.Log.warning("Cannot find property {} of {}".format(prop, obj.Label))
                self.valid = False
        else:
            self.valid = False

    def expression(self):
        """returns the expression if one is bound to the property"""
        Path.Log.track(self.prop, self.valid)
        if self.valid:
            return self.widget.property("expression")
        return ""

    def setMinimum(self, quantity):
        """set the minimum"""
        Path.Log.track(self.prop, self.valid)
        if self.valid:
            value = quantity.Value if hasattr(quantity, "Value") else quantity
            self.widget.setProperty("setMinimum", value)

    def updateWidget(self, quantity=None):
        """
        update the display value of the spin box.
        If no value is provided the value of the bound property is used.
        quantity can be of type Quantity or Float."""
        Path.Log.track(self.prop, self.valid, quantity)

        if self.valid:
            expr = self._hasExpression()
            if quantity is None:
                if expr:
                    quantity = FreeCAD.Units.Quantity(self.obj.evalExpression(expr))
                else:
                    quantity = PathUtil.getProperty(self.obj, self.prop)
            value = quantity.Value if hasattr(quantity, "Value") else quantity
            self.widget.setProperty("rawValue", value)
            self.lastWidgetText = self.widget.text()  # update last widget value
            if expr:
                self.widget.setReadOnly(True)
                self.widget.setProperty("exprSet", "true")
                self.widget.style().unpolish(self.widget)
                self.widget.ensurePolished()
            else:
                self.widget.setReadOnly(False)
                self.widget.setProperty("exprSet", "false")
                self.widget.style().unpolish(self.widget)
                self.widget.ensurePolished()

    def updateProperty(self):
        """updateProperty() ... update the bound property with the value from the spin box"""
        Path.Log.track(self.prop, self.valid)
        if self.valid:
            return updateInputField(self.obj, self.prop, self.widget, self.onBeforeChange)
        return None

    def _hasExpression(self):
        for prop, exp in self.obj.ExpressionEngine:
            if prop == self.prop:
                return exp
        return None


class PropertyComboBox(QtCore.QObject):
    """Base controller class for properties represented as QComboBox."""

    def __init__(self, widget, obj, prop, onBeforeChange=None):
        super().__init__()
        Path.Log.track(widget)
        self.widget = widget
        self.onBeforeChange = onBeforeChange
        self.prop = None
        self.obj = obj
        self.valid = False
        self.attachTo(obj, prop)
        self.widget.currentIndexChanged.connect(self.updateProperty)

    def attachTo(self, obj, prop=None):
        """use an existing editor for the given object and property"""
        Path.Log.track(self.prop, prop)
        self.obj = obj
        self.prop = prop
        if obj and prop:
            attr = PathUtil.getProperty(obj, prop)
            if attr is not None:
                self.valid = True
                self._populateComboBox()
                self.updateWidget()
            else:
                Path.Log.warning("Cannot find property {} of {}".format(prop, obj.Label))
                self.valid = False
        else:
            self.valid = False

    def _populateComboBox(self):
        """To be implemented by subclasses"""
        raise NotImplementedError

    def updateWidget(self, value=None):
        """update the display value of the combo box."""
        Path.Log.track(self.prop, self.valid, value)
        if self.valid:
            if value is None:
                value = PathUtil.getProperty(self.obj, self.prop)
            index = (
                self.widget.findData(value)
                if hasattr(self.widget, "findData")
                else self.widget.findText(str(value))
            )
            if index >= 0:
                self.widget.setCurrentIndex(index)

    def updateProperty(self):
        """update the bound property with the value from the combo box"""
        Path.Log.track(self.prop, self.valid)
        if self.valid and self.prop:
            if self.onBeforeChange:
                self.onBeforeChange()

            current_value = PathUtil.getProperty(self.obj, self.prop)
            new_value = (
                self.widget.currentData()
                if hasattr(self.widget, "currentData")
                else self.widget.currentText()
            )

            if str(new_value) != str(current_value):
                setattr(self.obj, self.prop, new_value)
                return True
        return False


class IntegerSpinBox(QtCore.QObject):
    """Controller class for integer properties represented as QSpinBox.
    IntegerSpinBox(widget, obj, prop, onBeforeChange=None)
            widget ... expected to be reference to a QSpinBox
            obj    ... document object
            prop   ... canonical name of the (sub-) property
            onBeforeChange ... optional callback before property change
    """

    def __init__(self, widget, obj, prop, onBeforeChange=None):
        super().__init__()
        self.widget = widget
        self.onBeforeChange = onBeforeChange
        self.prop = None
        self.obj = obj
        self.valid = False

        # Configure spin box defaults
        self.widget.setMinimum(-2147483647)  # Qt's minimum for spin boxes
        self.widget.setMaximum(2147483647)  # Qt's maximum for spin boxes

        self.attachTo(obj, prop)
        self.widget.valueChanged.connect(self.updateProperty)

    def attachTo(self, obj, prop=None):
        """bind to the given object and property"""
        self.obj = obj
        self.prop = prop
        if obj and prop:
            try:
                prop_value = PathUtil.getProperty(obj, prop)
                if prop_value is not None:
                    self.valid = True
                    self.updateWidget()
                else:
                    Path.Log.warning(f"Cannot get value for property {prop} of {obj.Label}")
                    self.valid = False
            except Exception as e:
                Path.Log.error(f"Error attaching to property {prop}: {str(e)}")
                self.valid = False
        else:
            self.valid = False

    def updateWidget(self, value=None):
        """update the spin box value"""
        if self.valid:
            try:
                if value is None:
                    value = PathUtil.getProperty(self.obj, self.prop)

                # Handle both direct values and Quantity objects
                if hasattr(value, "Value"):  # For Quantity properties
                    value = int(value.Value)

                self.widget.setValue(int(value))
            except Exception as e:
                Path.Log.error(f"Error updating spin box: {str(e)}")

    def updateProperty(self):
        """update the bound property with the spin box value"""
        if self.valid and self.prop:
            if self.onBeforeChange:
                self.onBeforeChange()

            new_value = self.widget.value()
            current_value = PathUtil.getProperty(self.obj, self.prop)

            # Handle Quantity properties
            if hasattr(current_value, "Value"):
                if new_value != current_value.Value:
                    current_value.Value = new_value
                    return True
            elif new_value != current_value:
                setattr(self.obj, self.prop, new_value)
                return True
        return False

    def setRange(self, min_val, max_val):
        """set minimum and maximum values"""
        self.widget.setMinimum(min_val)
        self.widget.setMaximum(max_val)

    def setSingleStep(self, step):
        """setSingleStep(step) ... set the step size"""
        self.widget.setSingleStep(step)


class BooleanComboBox(PropertyComboBox):
    """Controller class for boolean properties represented as QComboBox."""

    def _populateComboBox(self):
        self.widget.clear()
        self.widget.addItem("True", True)
        self.widget.addItem("False", False)


class EnumerationComboBox(PropertyComboBox):
    """Controller class for enumeration properties represented as QComboBox."""

    def _populateComboBox(self):
        self.widget.clear()
        enums = self.obj.getEnumerationsOfProperty(self.prop)
        for item in enums:
            self.widget.addItem(item, item)


class PropertyLabel(QtCore.QObject):
    """Controller class for read-only property display as QLabel."""

    def __init__(self, widget, obj, prop, onBeforeChange=None):
        super().__init__()
        self.widget = widget
        self.obj = obj
        self.prop = prop
        self.valid = False
        self.attachTo(obj, prop)

    def attachTo(self, obj, prop=None):
        """bind to the given object and property"""
        self.obj = obj
        self.prop = prop
        if obj and prop:
            attr = PathUtil.getProperty(obj, prop)
            if attr is not None:
                self.valid = True
                self.updateWidget()
            else:
                Path.Log.warning(f"Cannot find property {prop} of {obj.Label}")
                self.valid = False
        else:
            self.valid = False

    def updateWidget(self, value=None):
        """update the label text"""
        if self.valid:
            if value is None:
                value = PathUtil.getProperty(self.obj, self.prop)
            self.widget.setText(str(value))


def getDocNode():
    doc = FreeCADGui.ActiveDocument.Document.Name
    tws = FreeCADGui.getMainWindow().findChildren(QtGui.QTreeWidget)

    for tw in tws:
        if tw.topLevelItemCount() != 1 or tw.topLevelItem(0).text(0) != "Application":
            continue
        toptree = tw.topLevelItem(0)
        for i in range(0, toptree.childCount()):
            docitem = toptree.child(i)
            if docitem.text(0) == doc:
                return docitem
    return None


def disableItem(item):
    Dragflag = QtCore.Qt.ItemFlag.ItemIsDragEnabled
    Dropflag = QtCore.Qt.ItemFlag.ItemIsDropEnabled
    item.setFlags(item.flags() & ~Dragflag)
    item.setFlags(item.flags() & ~Dropflag)
    for idx in range(0, item.childCount()):
        disableItem(item.child(idx))


def findItem(docitem, objname):
    print(docitem.text(0))
    for i in range(0, docitem.childCount()):
        if docitem.child(i).text(0) == objname:
            return docitem.child(i)
        res = findItem(docitem.child(i), objname)
        if res:
            return res
    return None
