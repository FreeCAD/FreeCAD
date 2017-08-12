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
import PathScripts.PathLog as PathLog
import PathScripts.PathOpGui as PathOpGui
import PathScripts.PathProfileFaces as PathProfileFaces
import PathScripts.PathSelection as PathSelection

from PySide import QtCore

def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)

FeatureSide       = 0x01
FeatureProcessing = 0x02

class TaskPanelOpPage(PathOpGui.TaskPanelPage):

    def getForm(self):
        form = FreeCADGui.PySideUic.loadUi(":/panels/PageOpProfileFullEdit.ui")

        if not FeatureSide & self.profileFeatures():
            form.cutSide.hide()
            form.cutSideLabel.hide()

        if not FeatureProcessing & self.profileFeatures():
            form.processCircles.hide()
            form.processHoles.hide()
            form.processPerimeter.hide()

        return form

    def getFields(self, obj):
        self.updateInputField(obj, 'OffsetExtra', self.form.extraOffset)
        if obj.UseComp != self.form.useCompensation.isChecked():
            obj.UseComp = self.form.useCompensation.isChecked()
        if obj.UseStartPoint != self.form.useStartPoint.isChecked():
            obj.UseStartPoint = self.form.useStartPoint.isChecked()
        if obj.Direction != str(self.form.direction.currentText()):
            obj.Direction = str(self.form.direction.currentText())

        self.updateToolController(obj, self.form.toolController)

        if FeatureSide & self.profileFeatures():
            if obj.Side != str(self.form.cutSide.currentText()):
                obj.Side = str(self.form.cutSide.currentText())

        if FeatureProcessing & self.profileFeatures():
            if obj.processHoles != self.form.processHoles.isChecked():
                obj.processHoles = self.form.processHoles.isChecked()
            if obj.processPerimeter != self.form.processPerimeter.isChecked():
                obj.processPerimeter = self.form.processPerimeter.isChecked()
            if obj.processCircles != self.form.processCircles.isChecked():
                obj.processCircles = self.form.processCircles.isChecked()

    def setFields(self, obj):
        self.form.extraOffset.setText(FreeCAD.Units.Quantity(obj.OffsetExtra.Value, FreeCAD.Units.Length).UserString)
        self.form.useCompensation.setChecked(obj.UseComp)
        self.form.useStartPoint.setChecked(obj.UseStartPoint)

        self.selectInComboBox(obj.Direction, self.form.direction)
        self.setupToolController(obj, self.form.toolController)

        if FeatureSide & self.profileFeatures():
            self.selectInComboBox(obj.Side, self.form.cutSide)

        if FeatureProcessing & self.profileFeatures():
            self.form.processHoles.setChecked(obj.processHoles)
            self.form.processPerimeter.setChecked(obj.processPerimeter)
            self.form.processCircles.setChecked(obj.processCircles)

    def getSignalsForUpdate(self, obj):
        signals = []
        signals.append(self.form.direction.currentIndexChanged)
        signals.append(self.form.useCompensation.clicked)
        signals.append(self.form.useStartPoint.clicked)
        signals.append(self.form.extraOffset.editingFinished)

        if FeatureSide & self.profileFeatures():
            signals.append(self.form.cutSide.currentIndexChanged)

        if FeatureProcessing & self.profileFeatures():
            signals.append(self.form.processHoles.clicked)
            signals.append(self.form.processPerimeter.clicked)
            signals.append(self.form.processCircles.clicked)

        return signals
