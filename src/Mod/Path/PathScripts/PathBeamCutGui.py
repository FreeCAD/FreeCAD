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
import PathScripts.PathBeamCut as PathBeamCut
import PathScripts.PathGui as PathGui
import PathScripts.PathLog as PathLog
import PathScripts.PathOpGui as PathOpGui
import PathScripts.PathSelection as PathSelection

from PySide import QtCore, QtGui

__title__ = "Path BeamCut Operation UI"
__author__ = "sliptonic (Brad Collette)"
__url__ = "http://www.freecadweb.org"
__doc__ = "BeamCut operation page controller and command implementation."

if False:
    PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())
    PathLog.trackModule(PathLog.thisModule())
else:
    PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())

def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)

class TaskPanelOpPage(PathOpGui.TaskPanelPage):
    '''Page controller class for the BeamCut operation.'''

    def getForm(self):
        return FreeCADGui.PySideUic.loadUi(":/panels/PageOpBeamCutEdit.ui")

    def initPage(self, obj):
        self.opImagePath = "{}Mod/Path/Images/Ops/{}".format(FreeCAD.getHomePath(), 'offset.svg')
        self.opImage = QtGui.QPixmap(self.opImagePath)
        self.form.opImage.setPixmap(self.opImage)
        iconMiter = QtGui.QIcon(':/icons/edge-join-miter-not.svg')
        iconMiter.addFile(':/icons/edge-join-miter.svg', state=QtGui.QIcon.On)
        iconRound = QtGui.QIcon(':/icons/edge-join-round-not.svg')
        iconRound.addFile(':/icons/edge-join-round.svg', state=QtGui.QIcon.On)
        self.form.Bool_C.setEnabled(True)
        self.form.joinMiter.setIcon(iconMiter)
        self.form.joinRound.setIcon(iconRound)


    def getFields(self, obj):
        PathGui.updateInputField(obj, 'Offset', self.form.value_W)
        self.form.Bool_C
        if obj.ToolComp != self.form.Bool_C.isChecked():
            obj.ToolComp = self.form.Bool_C.isChecked()
        if self.form.joinRound.isChecked():
            obj.Join = 'Round'
        elif self.form.joinMiter.isChecked():
            obj.Join = 'Miter'

        self.updateToolController(obj, self.form.toolController)

    def setFields(self, obj):
        self.form.value_W.setText(FreeCAD.Units.Quantity(obj.Offset.Value, FreeCAD.Units.Length).UserString)
        self.setupToolController(obj, self.form.toolController)
        self.form.Bool_C.setChecked(obj.ToolComp)
        self.form.joinRound.setChecked('Round' == obj.Join)
        self.form.joinMiter.setChecked('Miter' == obj.Join)
        self.form.joinFrame.hide()

    def updateOffset(self):
        PathGui.updateInputField(self.obj, 'Offset', self.form.value_W)

    def getSignalsForUpdate(self, obj):
        '''getSignalsForUpdate(obj) ... return list of signals for updating obj'''
        signals = []
        signals.append(self.form.value_W.editingFinished)
        signals.append(self.form.Bool_C.clicked)
        signals.append(self.form.joinMiter.clicked)
        signals.append(self.form.joinRound.clicked)
        return signals

Command = PathOpGui.SetupOperation('BeamCut',
        PathBeamCut.Create,
        TaskPanelOpPage,
        'Path-BeamCut',
        QtCore.QT_TRANSLATE_NOOP("PathBeamCut", "BeamCut"),
        QtCore.QT_TRANSLATE_NOOP("PathBeamCut", "Creates a BeamCut Path along Edges or around Faces"),
        PathBeamCut.SetupProperties)

FreeCAD.Console.PrintLog("Loading PathBeamCutGui... done\n")

