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

import PathScripts.PathGui as PathGui
import PathScripts.PathLog as PathLog
import PathScripts.taskpanels.PathTaskPanelPage as PathTaskPanelPage


__title__ = "Path UI Task Panel Pages base classes"
__author__ = "sliptonic (Brad Collette)"
__url__ = "https://www.freecadweb.org"
__doc__ = "Base classes for UI features within Path operations"


FreeCAD = PathTaskPanelPage.FreeCAD
FreeCADGui = PathTaskPanelPage.FreeCADGui
translate = PathTaskPanelPage.translate


PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())
# PathLog.trackModule(PathLog.thisModule())


class TaskPanelDiametersPage(PathTaskPanelPage.TaskPanelPage):
    """Page controller for diameters."""

    def __init__(self, obj, features):
        super(TaskPanelDiametersPage, self).__init__(obj, features)

        # members initialized later
        self.clearanceHeight = None
        self.safeHeight = None

    def getForm(self):
        return FreeCADGui.PySideUic.loadUi(":/panels/PageDiametersEdit.ui")

    def initPage(self, obj):
        self.minDiameter = PathGui.QuantitySpinBox(
            self.form.minDiameter, obj, "MinDiameter"
        )
        self.maxDiameter = PathGui.QuantitySpinBox(
            self.form.maxDiameter, obj, "MaxDiameter"
        )

    def getTitle(self, obj):
        return translate("Path", "Diameters")

    def getFields(self, obj):
        self.minDiameter.updateProperty()
        self.maxDiameter.updateProperty()

    def setFields(self, obj):
        self.minDiameter.updateSpinBox()
        self.maxDiameter.updateSpinBox()

    def getSignalsForUpdate(self, obj):
        signals = []
        signals.append(self.form.minDiameter.editingFinished)
        signals.append(self.form.maxDiameter.editingFinished)
        return signals

    def pageUpdateData(self, obj, prop):
        if prop in ["MinDiameter", "MaxDiameter"]:
            self.setFields(obj)


FreeCAD.Console.PrintLog("Loading PathTaskPanelDiametersPage... done\n")
