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
import PathScripts.PathGeom as PathGeom
import PathScripts.PathLog as PathLog
import PathScripts.PathUtil as PathUtil


__title__ = "Path UI helper and utility functions"
__author__ = "sliptonic (Brad Collette)"
__url__ = "https://www.freecadweb.org"
__doc__ = "A collection of helper and utility functions for the Path GUI."


if False:
    PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())
    PathLog.trackModule(PathLog.thisModule())
else:
    PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())


def updateInputField(obj, prop, widget, onBeforeChange=None):
    """updateInputField(obj, prop, widget) ... update obj's property prop with the value of widget.
    The property's value is only assigned if the new value differs from the current value.
    This prevents onChanged notifications where the value didn't actually change.
    Gui::InputField and Gui::QuantitySpinBox widgets are supported - and the property can
    be of type Quantity or Float.
    If onBeforeChange is specified it is called before a new value is assigned to the property.
    Returns True if a new value was assigned, False otherwise (new value is the same as the current).
    """
    PathLog.track()
    value = widget.property("rawValue")
    attr = PathUtil.getProperty(obj, prop)
    attrValue = attr.Value if hasattr(attr, "Value") else attr

    isDiff = False
    if not PathGeom.isRoughly(attrValue, value):
        isDiff = True
    else:
        if hasattr(obj, "ExpressionEngine"):
            noExpr = True
            for (prp, expr) in obj.ExpressionEngine:
                if prp == prop:
                    noExpr = False
                    PathLog.debug('prop = "expression": {} = "{}"'.format(prp, expr))
                    value = FreeCAD.Units.Quantity(obj.evalExpression(expr)).Value
                    if not PathGeom.isRoughly(attrValue, value):
                        isDiff = True
                    break
            if noExpr:
                widget.setReadOnly(False)
                widget.setStyleSheet("color: black")
            else:
                widget.setReadOnly(True)
                widget.setStyleSheet("color: gray")
            widget.update()

    if isDiff:
        PathLog.debug(
            "updateInputField(%s, %s): %.2f -> %.2f" % (obj.Label, prop, attr, value)
        )
        if onBeforeChange:
            onBeforeChange(obj)
        PathUtil.setProperty(obj, prop, value)
        return True

    return False


class QuantitySpinBox:
    """Controller class to interface a Gui::QuantitySpinBox.
    The spin box gets bound to a given property and supports update in both directions.
    QuatitySpinBox(widget, obj, prop, onBeforeChange=None)
            widget ... expected to be reference to a Gui::QuantitySpinBox
            obj    ... document object
            prop   ... canonical name of the (sub-) property
            onBeforeChange ... an optional callback being executed before the value of the property is changed
    """

    def __init__(self, widget, obj, prop, onBeforeChange=None):
        PathLog.track(widget)
        self.widget = widget
        self.onBeforeChange = onBeforeChange
        self.prop = None
        self.obj = obj
        self.attachTo(obj, prop)

    def attachTo(self, obj, prop=None):
        """attachTo(obj, prop=None) ... use an existing editor for the given object and property"""
        PathLog.track(self.prop, prop)
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
                PathLog.warning("Cannot find property {} of {}".format(prop, obj.Label))
                self.valid = False
        else:
            self.valid = False

    def expression(self):
        """expression() ... returns the expression if one is bound to the property"""
        PathLog.track(self.prop, self.valid)
        if self.valid:
            return self.widget.property("expression")
        return ""

    def setMinimum(self, quantity):
        """setMinimum(quantity) ... set the minimum"""
        PathLog.track(self.prop, self.valid)
        if self.valid:
            value = quantity.Value if hasattr(quantity, "Value") else quantity
            self.widget.setProperty("setMinimum", value)

    def updateSpinBox(self, quantity=None):
        """updateSpinBox(quantity=None) ... update the display value of the spin box.
        If no value is provided the value of the bound property is used.
        quantity can be of type Quantity or Float."""
        PathLog.track(self.prop, self.valid)

        if self.valid:
            expr = self._hasExpression()
            if quantity is None:
                if expr:
                    quantity = FreeCAD.Units.Quantity(self.obj.evalExpression(expr))
                else:
                    quantity = PathUtil.getProperty(self.obj, self.prop)
            value = quantity.Value if hasattr(quantity, "Value") else quantity
            self.widget.setProperty("rawValue", value)

    def updateProperty(self):
        """updateProperty() ... update the bound property with the value from the spin box"""
        PathLog.track(self.prop, self.valid)
        if self.valid:
            return updateInputField(
                self.obj, self.prop, self.widget, self.onBeforeChange
            )
        return None

    def _hasExpression(self):
        for (prop, exp) in self.obj.ExpressionEngine:
            if prop == self.prop:
                return exp
        return None
