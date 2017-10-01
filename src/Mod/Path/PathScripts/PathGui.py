# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
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
import PathScripts.PathGeom as PathGeom
import PathScripts.PathGetPoint as PathGetPoint
import PathScripts.PathLog as PathLog
import PathScripts.PathSelection as PathSelection
import PathScripts.PathOp as PathOp
import PathScripts.PathUtils as PathUtils
import importlib

from PathScripts.PathGeom import PathGeom
from PySide import QtCore, QtGui

__title__ = "Path UI helper and utility functions"
__author__ = "sliptonic (Brad Collette)"
__url__ = "http://www.freecadweb.org"
__doc__ = "A collection of helper and utility functions for the Path GUI."


def updateInputField(obj, prop, widget, onBeforeChange = None):
    '''updateInputField(obj, prop, widget) ... helper function to update obj's property named prop with the value from widget, if it has changed.'''
    value = FreeCAD.Units.Quantity(widget.text()).Value
    attr = getattr(obj, prop)
    attrValue = attr.Value if hasattr(attr, 'Value') else attr
    if not PathGeom.isRoughly(attrValue, value):
        PathLog.debug("updateInputField(%s, %s): %.2f -> %.2f" % (obj.Label, prop, getattr(obj, prop), value))
        if onBeforeChange:
            onBeforeChange(obj)
        setattr(obj, prop, value)
        return True
    return False

