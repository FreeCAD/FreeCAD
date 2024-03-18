# -*- coding: utf-8 -*-
# ***************************************************************************
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
import Path.Op.Custom as PathCustom
import Path.Op.Gui.Base as PathOpGui

from PySide.QtCore import QT_TRANSLATE_NOOP


__title__ = "CAM Custom Operation UI"
__author__ = "sliptonic (Brad Collette)"
__url__ = "https://www.freecad.org"
__doc__ = "Custom operation page controller and command implementation."


class TaskPanelOpPage(PathOpGui.TaskPanelPage):
    """Page controller class for the Custom operation."""

    def getForm(self):
        """getForm() ... returns UI"""
        return FreeCADGui.PySideUic.loadUi(":/panels/PageOpCustomEdit.ui")

    def getFields(self, obj):
        """getFields(obj) ... transfers values from UI to obj's properties"""
        self.updateToolController(obj, self.form.toolController)
        self.updateCoolant(obj, self.form.coolantController)

    def setFields(self, obj):
        """setFields(obj) ... transfers obj's property values to UI"""
        self.setupToolController(obj, self.form.toolController)
        self.form.txtGCode.setText("\n".join(obj.Gcode))
        self.setupCoolant(obj, self.form.coolantController)

    def getSignalsForUpdate(self, obj):
        """getSignalsForUpdate(obj) ... return list of signals for updating obj"""
        signals = []
        signals.append(self.form.toolController.currentIndexChanged)
        signals.append(self.form.coolantController.currentIndexChanged)
        self.form.txtGCode.textChanged.connect(self.setGCode)
        return signals

    def setGCode(self):
        self.obj.Gcode = self.form.txtGCode.toPlainText().splitlines()


Command = PathOpGui.SetupOperation(
    "Custom",
    PathCustom.Create,
    TaskPanelOpPage,
    "CAM_Custom",
    QT_TRANSLATE_NOOP("CAM_Custom", "Custom"),
    QT_TRANSLATE_NOOP("CAM_Custom", "Create custom G-code snippet"),
    PathCustom.SetupProperties,
)

FreeCAD.Console.PrintLog("Loading PathCustomGui... done\n")
