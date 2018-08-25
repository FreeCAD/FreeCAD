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
import PathScripts.PathOp as PathOp
import PathScripts.PathOpGui as PathOpGui
import PathScripts.PathPocketShape as PathPocketShape
import PathScripts.PathPocketBaseGui as PathPocketBaseGui

from PySide import QtCore, QtGui

__title__ = "Path Pocket Shape Operation UI"
__author__ = "sliptonic (Brad Collette)"
__url__ = "http://www.freecadweb.org"
__doc__ = "Pocket Shape operation page controller and command implementation."

class TaskPanelExtensionPage(PathOpGui.TaskPanelPage):

    def initPage(self, obj):
        self.extensions = [self.form.negXInput, self.form.posXInput, self.form.negYInput, self.form.posYInput]
        self.setTitle("Pocket Extensions")
        self.enabled = True
        self.enable(False)

    def enable(self, ena):
        if ena != self.enabled:
            self.enabled = ena
            if ena:
                self.form.info.hide()
                for ext in self.extensions:
                    ext.setEnabled(True)
            else:
                self.form.info.show()
                for ext in self.extensions:
                    ext.setEnabled(False)

    def getForm(self):
        return FreeCADGui.PySideUic.loadUi(":/panels/PageOpExtensionEdit.ui")


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
