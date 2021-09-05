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

import PathScripts.PathGetPoint as PathGetPoint
import PathScripts.PathLog as PathLog
import PathScripts.taskpanels.PathTaskPanelPage as PathTaskPanelPage

from PySide import QtCore, QtGui


__title__ = "Path UI Task Panel Pages base classes"
__author__ = "sliptonic (Brad Collette)"
__url__ = "https://www.freecadweb.org"
__doc__ = "Base classes for UI features within Path operations"


FreeCAD = PathTaskPanelPage.FreeCAD
FreeCADGui = PathTaskPanelPage.FreeCADGui
translate = PathTaskPanelPage.translate

PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())
# PathLog.trackModule(PathLog.thisModule())


class TaskPanelBaseLocationPage(PathTaskPanelPage.TaskPanelPage):
    """Page controller for base locations. Uses PathGetPoint."""

    DataLocation = QtCore.Qt.ItemDataRole.UserRole

    def __init__(self, obj, features):
        super(TaskPanelBaseLocationPage, self).__init__(obj, features)

        # members initialized later
        self.editRow = None
        self.title = "Base Location"

    def getForm(self):
        self.formLoc = FreeCADGui.PySideUic.loadUi(":/panels/PageBaseLocationEdit.ui")
        if QtCore.qVersion()[0] == "4":
            self.formLoc.baseList.horizontalHeader().setResizeMode(
                QtGui.QHeaderView.Stretch
            )
        else:
            self.formLoc.baseList.horizontalHeader().setSectionResizeMode(
                QtGui.QHeaderView.Stretch
            )
        self.getPoint = PathGetPoint.TaskPanel(self.formLoc.addRemoveEdit)
        return self.formLoc

    def modifyStandardButtons(self, buttonBox):
        self.getPoint.buttonBox = buttonBox

    def getTitle(self, obj):
        return translate("PathOp2", "Base Location")

    def getFields(self, obj):
        pass

    def setFields(self, obj):
        self.formLoc.baseList.blockSignals(True)
        self.formLoc.baseList.clearContents()
        self.formLoc.baseList.setRowCount(0)
        for location in self.obj.Locations:
            self.formLoc.baseList.insertRow(self.formLoc.baseList.rowCount())

            item = QtGui.QTableWidgetItem("%.2f" % location.x)
            item.setData(self.DataLocation, location.x)
            self.formLoc.baseList.setItem(self.formLoc.baseList.rowCount() - 1, 0, item)

            item = QtGui.QTableWidgetItem("%.2f" % location.y)
            item.setData(self.DataLocation, location.y)
            self.formLoc.baseList.setItem(self.formLoc.baseList.rowCount() - 1, 1, item)
        self.formLoc.baseList.resizeColumnToContents(0)
        self.formLoc.baseList.blockSignals(False)
        self.itemActivated()

    def removeLocation(self):
        deletedRows = []
        selected = self.formLoc.baseList.selectedItems()
        for item in selected:
            row = self.formLoc.baseList.row(item)
            if row not in deletedRows:
                deletedRows.append(row)
                self.formLoc.baseList.removeRow(row)
        self.updateLocations()
        FreeCAD.ActiveDocument.recompute()

    def updateLocations(self):
        PathLog.track()
        locations = []
        for i in range(self.formLoc.baseList.rowCount()):
            x = self.formLoc.baseList.item(i, 0).data(self.DataLocation)
            y = self.formLoc.baseList.item(i, 1).data(self.DataLocation)
            location = FreeCAD.Vector(x, y, 0)
            locations.append(location)
        self.obj.Locations = locations

    def addLocation(self):
        self.getPoint.getPoint(self.addLocationAt)

    def addLocationAt(self, point, obj):
        # pylint: disable=unused-argument
        if point:
            locations = self.obj.Locations
            locations.append(point)
            self.obj.Locations = locations
            FreeCAD.ActiveDocument.recompute()

    def editLocation(self):
        selected = self.formLoc.baseList.selectedItems()
        if selected:
            row = self.formLoc.baseList.row(selected[0])
            self.editRow = row
            x = self.formLoc.baseList.item(row, 0).data(self.DataLocation)
            y = self.formLoc.baseList.item(row, 1).data(self.DataLocation)
            start = FreeCAD.Vector(x, y, 0)
            self.getPoint.getPoint(self.editLocationAt, start)

    def editLocationAt(self, point, obj):
        # pylint: disable=unused-argument
        if point:
            self.formLoc.baseList.item(self.editRow, 0).setData(
                self.DataLocation, point.x
            )
            self.formLoc.baseList.item(self.editRow, 1).setData(
                self.DataLocation, point.y
            )
            self.updateLocations()
            FreeCAD.ActiveDocument.recompute()

    def itemActivated(self):
        if self.formLoc.baseList.selectedItems():
            self.form.removeLocation.setEnabled(True)
            self.form.editLocation.setEnabled(True)
        else:
            self.form.removeLocation.setEnabled(False)
            self.form.editLocation.setEnabled(False)

    def registerSignalHandlers(self, obj):
        self.form.baseList.itemSelectionChanged.connect(self.itemActivated)
        self.formLoc.addLocation.clicked.connect(self.addLocation)
        self.formLoc.removeLocation.clicked.connect(self.removeLocation)
        self.formLoc.editLocation.clicked.connect(self.editLocation)

    def pageUpdateData(self, obj, prop):
        if prop in ["Locations"]:
            self.setFields(obj)


FreeCAD.Console.PrintLog("Loading PathTaskPanelBaseLocationPage... done\n")
