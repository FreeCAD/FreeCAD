# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2019 -> Developer Info Here <-                          *
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

# Visit a FreeCAD long-time contributor, sliptonic, page at
# https://github.com/sliptonic/FreeCAD/wiki/Developing-for-Path
# for additional information regarding developing for the Path Workbench.

import FreeCAD
import FreeCADGui
import PathScripts.PathNew1 as PathNew1
import PathScripts.PathGui as PathGui
import PathScripts.PathOpGui as PathOpGui

from PySide import QtCore

__title__ = "Path New 1 Operation GUI"
__author__ = "username (Iamthe Author)"
__url__ = "http://www.freecadweb.org"
__doc__ = "Class and implementation of New 1 operation GUI characteristics."
__contributors__ = ""  # additional contributors may put handles, usernames, or names here
__created__ = "2019"
__scriptVersion__ = "1a"  # Update script version with each iteration of the file
__lastModified__ = "2019-08-06 17:37 CST"  # Update with date and time of last modification


class TaskPanelOpPage(PathOpGui.TaskPanelPage):
    '''Page controller class for the Surface operation.'''

    def getForm(self):
        '''getForm() ... returns UI'''
        return FreeCADGui.PySideUic.loadUi(":/panels/PageOpNew1Edit.ui")

    def getFields(self, obj):
        '''getFields(obj) ... transfers values from UI to obj's proprties'''
        PathGui.updateInputField(obj, 'Line1', self.form.line1)
        PathGui.updateInputField(obj, 'Line2', self.form.line2)

        if obj.Combo1 != str(self.form.combo1.currentText()):
            obj.Combo1 = str(self.form.combo1.currentText())

        if obj.Combo2 != str(self.form.combo2.currentText()):
            obj.Combo2 = str(self.form.combo2.currentText())

        obj.Line1 = FreeCAD.Units.Quantity(self.form.line1.text()).Value

        obj.Line2 = FreeCAD.Units.Quantity(self.form.line2.text()).Value

        if obj.Checkbox1 != self.form.checkbox1.isChecked():
            obj.Checkbox1 = self.form.checkbox1.isChecked()

        if obj.Checkbox2 != self.form.checkbox2.isChecked():
            obj.Checkbox2 = self.form.checkbox2.isChecked()

        if obj.Spin1 != self.form.spin1.value():
            obj.Spin1 = self.form.spin1.value()

        if obj.Spin2 != self.form.spin2.value():
            obj.Spin2 = self.form.spin2.value()

        self.updateToolController(obj, self.form.toolcontroller)

    def setFields(self, obj):
        '''setFields(obj) ... transfers obj's property values to UI'''
        self.selectInComboBox(obj.Combo1, self.form.combo1)
        self.selectInComboBox(obj.Combo2, self.form.combo2)

        self.form.line1.setText(str(obj.Line1))
        self.form.line2.setText(str(obj.Line2))

        self.form.spin1.setValue(obj.Spin1)
        self.form.spin2.setValue(obj.Spin2)

        if obj.Checkbox1 is True:
            self.form.checkbox1.setCheckState(QtCore.Qt.Checked)
        else:
            self.form.checkbox1.setCheckState(QtCore.Qt.Unchecked)

        if obj.Checkbox2 is True:
            self.form.checkbox2.setCheckState(QtCore.Qt.Checked)
        else:
            self.form.checkbox2.setCheckState(QtCore.Qt.Unchecked)

        self.setupToolController(obj, self.form.toolcontroller)

    def getSignalsForUpdate(self, obj):
        '''getSignalsForUpdate(obj) ... return list of signals for updating obj'''
        signals = []
        signals.append(self.form.toolcontroller.currentIndexChanged)
        signals.append(self.form.combo1.currentIndexChanged)
        signals.append(self.form.combo2.currentIndexChanged)
        signals.append(self.form.line1.editingFinished)
        signals.append(self.form.line2.editingFinished)
        signals.append(self.form.spin1.editingFinished)
        signals.append(self.form.spin2.editingFinished)
        signals.append(self.form.checkbox1.stateChanged)
        signals.append(self.form.checkbox2.stateChanged)

        return signals

    def updateVisibility(self):
        '''
        This method should reflect settings in setEditorProperties(obj)
        method in regular PathNew1.py module.'''
        if self.form.combo1.currentText() == 'c1D':
            self.form.combo2.setEnabled(False)
            self.form.checkbox1.setEnabled(True)
        else:
            self.form.combo2.setEnabled(True)
            self.form.checkbox1.setEnabled(False)

        if self.form.checkbox1.isChecked() is True:
            self.form.combo2.setEnabled(False)
        else:
            self.form.combo2.setEnabled(True)

    def registerSignalHandlers(self, obj):
        self.form.combo1.currentIndexChanged.connect(self.updateVisibility)
        self.form.checkbox1.stateChanged.connect(self.updateVisibility)


Command = PathOpGui.SetupOperation('New1',
        PathNew1.Create,
        TaskPanelOpPage,
        'Path-New1',
        QtCore.QT_TRANSLATE_NOOP("New1", "New 1 Op"),
        QtCore.QT_TRANSLATE_NOOP("New1", "New 1 developmental operation"),
        PathNew1.SetupProperties)

FreeCAD.Console.PrintLog("Loading PathNew1Gui... done\n")
