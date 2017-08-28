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
import PathScripts.PathEngrave as PathEngrave
import PathScripts.PathLog as PathLog
import PathScripts.PathOpGui as PathOpGui
import PathScripts.PathSelection as PathSelection

from PySide import QtCore, QtGui

__title__ = "Path Engrave Operation UI"
__author__ = "sliptonic (Brad Collette)"
__url__ = "http://www.freecadweb.org"
__doc__ = "Engrave operation page controller and command implementation."

def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)

class TaskPanelOpPage(PathOpGui.TaskPanelPage):
    '''Page controller class for the Engrave operation.'''

    def getForm(self):
        '''getForm() ... returns UI'''
        return FreeCADGui.PySideUic.loadUi(":/panels/PageOpEngraveEdit.ui")

    def getFields(self, obj):
        '''getFields(obj) ... transfers values from UI to obj's proprties'''
        if obj.StartVertex != self.form.startVertex.value():
            obj.StartVertex = self.form.startVertex.value()
        self.updateToolController(obj, self.form.toolController)

    def setFields(self, obj):
        '''setFields(obj) ... transfers obj's property values to UI'''
        self.form.startVertex.setValue(obj.StartVertex)
        self.setupToolController(obj, self.form.toolController)

    def getSignalsForUpdate(self, obj):
        '''getSignalsForUpdate(obj) ... return list of signals for updating obj'''
        signals = []
        signals.append(self.form.startVertex.editingFinished)
        signals.append(self.form.toolController.currentIndexChanged)
        return signals

Command = PathOpGui.SetupOperation('Engrave',
        PathEngrave.Create,
        TaskPanelOpPage,
        'Path-Engrave',
        QtCore.QT_TRANSLATE_NOOP("PathEngrave", "Engrave"),
        QtCore.QT_TRANSLATE_NOOP("PathEngrave", "Creates an Engraving Path around a Draft ShapeString"))

FreeCAD.Console.PrintLog("Loading PathEngraveGui... done\n")
