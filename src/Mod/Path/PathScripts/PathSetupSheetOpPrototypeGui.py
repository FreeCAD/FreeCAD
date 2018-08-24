# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2018 sliptonic <shopinthewoods@gmail.com>               *
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
import PathScripts.PathGui as PathGui
import PathScripts.PathIconViewProvider as PathIconViewProvider
import PathScripts.PathLog as PathLog
import PathScripts.PathSetupSheet as PathSetupSheet
import PathScripts.PathSetupSheetOpPrototype as PathSetupSheetOpPrototype
import PathScripts.PathUtil as PathUtil

from PySide import QtCore, QtGui

__title__ = "Setup Sheet Editor"
__author__ = "sliptonic (Brad Collette)"
__url__ = "http://www.freecadweb.org"
__doc__ = "Task panel editor for a SetupSheet"

# Qt tanslation handling
def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)

if True:
    PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())
    PathLog.trackModule(PathLog.thisModule())
else:
    PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())

class _PropertyEditor(object):
    def __init__(self, prop):
        self.prop = prop

class _PropertyEnumEditor(_PropertyEditor):

    def widget(self, parent):
        PathLog.track(self.prop.name, self.prop.getEnumValues())
        return QtGui.QComboBox(parent);

    def setEditorData(self, widget):
        widget.clear()
        widget.addItems(self.prop.getEnumValues())
        if self.prop.getValue():
            index = widget.findText(self.prop.getValue(), QtCore.Qt.MatchFixedString)
            if index >= 0:
                widget.setCurrentIndex(index)

    def setModelData(self, widget):
        self.prop.setValue(widget.currentText())


_EditorFactory = {
        PathSetupSheetOpPrototype.Property: None,
        PathSetupSheetOpPrototype.PropertyBool: None,
        PathSetupSheetOpPrototype.PropertyDistance: None,
        PathSetupSheetOpPrototype.PropertyEnumeration: _PropertyEnumEditor,
        PathSetupSheetOpPrototype.PropertyFloat: None,
        PathSetupSheetOpPrototype.PropertyPercent: None,
        PathSetupSheetOpPrototype.PropertyString: None,
        }

X = []

def Editor(prop):
    '''Returns an editor class to be used for that property.'''
    global X
    X.append(prop)
    factory = _EditorFactory[prop.__class__]
    if factory:
        return factory(prop)
    return None
