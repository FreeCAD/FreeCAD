# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2019 Russell Johnson [russ4262] <russ4262@gmail.com>    *
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
import PathScripts.PathPartAlign as PathPartAlign
import PathScripts.PathGui as PathGui
import PathScripts.PathOpGui as PathOpGui

from PySide import QtCore

__title__ = "Path Part Align Operation UI"
__author__ = "russ4262 (Russell Johnson)"
__url__ = "http://www.freecadweb.org"
__doc__ = "Part Alignment operation GUI controller and command implementation."
__contributors__ = ""
__createdDate__ = "2019"
__scriptVersion__ = "1b usable"
__lastModified__ = "2019-08-06 17:35 CST"


class TaskPanelOpPage(PathOpGui.TaskPanelPage):
    '''Page controller class for the Surface operation.'''

    def getForm(self):
        '''getForm() ... returns UI'''
        # return FreeCADGui.PySideUic.loadUi(":/panels/PageOpPartAlignEdit.ui")
        return FreeCADGui.PySideUic.loadUi(":/panels/PageOpPartAlignEdit.ui")

    def getFields(self, obj):
        '''getFields(obj) ... transfers values from GUI to obj's proprties'''

        PathGui.updateInputField(obj, 'ApproachDistance', self.form.approachDistance)
        PathGui.updateInputField(obj, 'StopBelowVertex', self.form.stopBelowVertex)

        if obj.AlignmentMode != str(self.form.alignmentMode.currentText()):
            obj.AlignmentMode = str(self.form.alignmentMode.currentText())

        if obj.AlignmentType != str(self.form.alignmentType.currentText()):
            obj.AlignmentType = str(self.form.alignmentType.currentText())

        if obj.FeedRatePercentForApproach != self.form.feedRatePercentForApproach.value():
            obj.FeedRatePercentForApproach = self.form.feedRatePercentForApproach.value()

        self.updateToolController(obj, self.form.toolController)

    def setFields(self, obj):
        '''setFields(obj) ... transfers obj's property values to GUI'''

        self.selectInComboBox(obj.AlignmentMode, self.form.alignmentMode)
        self.selectInComboBox(obj.AlignmentType, self.form.alignmentType)

        self.form.approachDistance.setText(FreeCAD.Units.Quantity(obj.ApproachDistance.Value, FreeCAD.Units.Length).UserString)
        self.form.stopBelowVertex.setText(FreeCAD.Units.Quantity(obj.StopBelowVertex.Value, FreeCAD.Units.Length).UserString)

        self.form.feedRatePercentForApproach.setValue(obj.FeedRatePercentForApproach)

        self.setupToolController(obj, self.form.toolController)
        self.updateVisibility()

    def getSignalsForUpdate(self, obj):
        '''getSignalsForUpdate(obj) ... return list of signals for updating obj'''
        signals = []
        signals.append(self.form.toolController.currentIndexChanged)
        signals.append(self.form.alignmentMode.currentIndexChanged)
        signals.append(self.form.alignmentType.currentIndexChanged)
        signals.append(self.form.approachDistance.editingFinished)
        signals.append(self.form.stopBelowVertex.editingFinished)
        signals.append(self.form.feedRatePercentForApproach.editingFinished)
        return signals

    def updateVisibility(self):
        '''updateVisibility() ... 
        updates visibility of property(settings) inputs in the GUI editor window'''
        if self.form.alignmentMode.currentText() == "Auto":
            self.form.alignmentType.setEnabled(False)
        else:
            self.form.alignmentType.setEnabled(True)

    def registerSignalHandlers(self, obj):
        self.form.alignmentMode.currentIndexChanged.connect(self.updateVisibility)

Command = PathOpGui.SetupOperation('PartAlign',
                                   PathPartAlign.Create,
                                   TaskPanelOpPage,
                                   'Path-PartAlign',
                                   QtCore.QT_TRANSLATE_NOOP("PartAlign", "Part Alignment"),
                                   QtCore.QT_TRANSLATE_NOOP("PartAlign", "Create a Part Alignment operation from edges of a model"),
                                   PathPartAlign.SetupProperties)

FreeCAD.Console.PrintLog("Loading PathPartAlignGui... done\n")
