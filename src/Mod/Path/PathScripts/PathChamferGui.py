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
import PathScripts.PathChamfer as PathChamfer
import PathScripts.PathGui as PathGui
import PathScripts.PathLog as PathLog
import PathScripts.PathOpGui as PathOpGui
import PathScripts.PathSelection as PathSelection

from PySide import QtCore, QtGui

__title__ = "Path Chamfer Operation UI"
__author__ = "sliptonic (Brad Collette)"
__url__ = "http://www.freecadweb.org"
__doc__ = "Chamfer operation page controller and command implementation."

if False:
    PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())
    PathLog.trackModule(PathLog.thisModule())
else:
    PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())

def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)


class TaskPanelOpPage(PathOpGui.TaskPanelPage):
    '''Page controller class for the Chamfer operation.'''

    def getForm(self):
        return FreeCADGui.PySideUic.loadUi(":/panels/PageOpChamferEdit.ui")

    def initPage(self, obj):
        self.opImagePath = "{}Mod/Path/Images/Ops/{}".format(FreeCAD.getHomePath(), 'chamfer.svg')
        self.opImage = QtGui.QPixmap(self.opImagePath)
        self.form.opImage.setPixmap(self.opImage)

    def getFields(self, obj):
        PathGui.updateInputField(obj, 'Width', self.form.value_W)
        PathGui.updateInputField(obj, 'ExtraDepth', self.form.value_h)

        self.updateToolController(obj, self.form.toolController)

    def setFields(self, obj):
        self.form.value_W.setText(FreeCAD.Units.Quantity(obj.Width.Value, FreeCAD.Units.Length).UserString)
        self.form.value_h.setText(FreeCAD.Units.Quantity(obj.ExtraDepth.Value, FreeCAD.Units.Length).UserString)
        self.setupToolController(obj, self.form.toolController)

    def updateWidth(self):
        PathGui.updateInputField(self.obj, 'Width', self.form.value_W)

    def updateExtraDepth(self):
        PathGui.updateInputField(self.obj, 'ExtraDepth', self.form.value_h)

    def registerSignalHandlers(self, obj):
        self.form.value_W.editingFinished.connect(self.updateWidth)
        self.form.value_h.editingFinished.connect(self.updateExtraDepth)


Command = PathOpGui.SetupOperation('Chamfer',
        PathChamfer.Create,
        TaskPanelOpPage,
        'Path-Chamfer',
        QtCore.QT_TRANSLATE_NOOP("PathChamfer", "Chamfer"),
        QtCore.QT_TRANSLATE_NOOP("PathChamfer", "Creates an Engraving Path around a Draft ShapeString"))

FreeCAD.Console.PrintLog("Loading PathChamferGui... done\n")

