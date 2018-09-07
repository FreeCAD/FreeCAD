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
import PathScripts.PathOp as PathOp
import PathScripts.PathOpGui as PathOpGui
import PathScripts.PathPocketShape as PathPocketShape
import PathScripts.PathPocketBaseGui as PathPocketBaseGui
import PathScripts.PathUtil as PathUtil

from PySide import QtCore, QtGui

__title__ = "Path Pocket Shape Operation UI"
__author__ = "sliptonic (Brad Collette)"
__url__ = "http://www.freecadweb.org"
__doc__ = "Pocket Shape operation page controller and command implementation."


if True:
    PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())
    PathLog.trackModule(PathLog.thisModule())
else:
    PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())


class TaskPanelExtensionPage(PathOpGui.TaskPanelPage):

    def initPage(self, obj):
        self.setTitle("Pocket Extensions")
        self.enabled = True
        self.enable(False)
        tc = PathUtil.toolControllerForOp(self.obj)
        if tc:
            self.form.defaultLength.setValue(tc.Tool.Diameter/2)

    def enable(self, ena):
        if ena != self.enabled:
            self.enabled = ena
            if ena:
                self.form.info.hide()
                self.form.extensionEdit.setEnabled(True)
            else:
                self.form.info.show()
                self.form.extensionEdit.setEnabled(False)

    def getForm(self):
        return FreeCADGui.PySideUic.loadUi(":/panels/PageOpPocketExtEdit.ui")

    def updateSelection(self, obj, sel):
        PathLog.track(sel)
        if sel and sel[0].SubElementNames:
            self.form.buttonAdd.setEnabled(True)
        else:
            self.form.buttonAdd.setEnabled(False)

    def currentItemChanged(self, now, prev):
        if 0 == self.form.extensions.rowCount():
            self.form.buttonClear.setEnabled(False)
            self.form.buttonRemove.setEnabled(False)
        else:
            self.form.buttonClear.setEnabled(True)
            if self.form.extensions.selectedItems():
                self.form.buttonRemove.setEnabled(True)
            else:
                self.form.buttonRemove.setEnabled(False)

    def extensionsAdd(self):
        pass

    def extensionsClear(self):
        self.form.extensions.clearContents()

    def extensionsRemove(self):
        pass

    def pageRegisterSignalHandlers(self):
        self.form.extensions.currentItemChanged.connect(self.currentItemChanged)
        self.form.buttonAdd.clicked.connect(self.extensionsAdd)
        self.form.buttonClear.clicked.connect(self.extensionsClear)
        self.form.buttonRemove.clicked.connect(self.extensionsRemove)

        self.updateSelection(self.obj, FreeCADGui.Selection.getSelectionEx())
        self.currentItemChanged(-1, -1)

class TaskPanelOpPage(PathPocketBaseGui.TaskPanelOpPage):
    '''Page controller class for Pocket operation'''

    def pocketFeatures(self):
        '''pocketFeatures() ... return FeaturePocket (see PathPocketBaseGui)'''
        return PathPocketBaseGui.FeaturePocket | PathPocketBaseGui.FeatureOutline

    def taskPanelBaseLocationPage(self, obj, features):
        self.extensionsPanel = TaskPanelExtensionPage(obj, features)
        return self.extensionsPanel

    def enableExtensions(self):
        self.extensionsPanel.enable(self.form.useOutline.isChecked())

    def pageRegisterSignalHandlers(self):
        self.form.useOutline.clicked.connect(self.enableExtensions)

Command = PathOpGui.SetupOperation('Pocket Shape',
        PathPocketShape.Create,
        TaskPanelOpPage,
        'Path-Pocket',
        QtCore.QT_TRANSLATE_NOOP("PathPocket", "Pocket Shape"),
        QtCore.QT_TRANSLATE_NOOP("PathPocket", "Creates a Path Pocket object from a face or faces"),
        PathPocketShape.SetupProperties)

FreeCAD.Console.PrintLog("Loading PathPocketShapeGui... done\n")
