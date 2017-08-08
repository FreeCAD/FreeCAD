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
import PathScripts.PathOpGui as PathOpGui
import PathScripts.PathDrilling as PathDrilling
import PathScripts.PathLog as PathLog
import PathScripts.PathPocketBaseGui as PathPocketBaseGui

from PySide import QtCore, QtGui

if True:
    PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())
    PathLog.trackModule(PathLog.thisModule())
else:
    PathLog.setLevel(PathLog.Level.NOTICE, PathLog.thisModule())

class TaskPanelHoleGeometryPage(PathOpGui.TaskPanelBaseGeometryPage):
    DataFeatureName = QtCore.Qt.ItemDataRole.UserRole
    DataObject      = QtCore.Qt.ItemDataRole.UserRole + 1
    DataObjectSub   = QtCore.Qt.ItemDataRole.UserRole + 2

    def getForm(self):
        return FreeCADGui.PySideUic.loadUi(":/panels/PageBaseHoleGeometryEdit.ui")

    def setFields(self, obj):
        PathLog.track()
        self.form.baseList.blockSignals(True)
        self.form.baseList.clearContents()
        self.form.baseList.setRowCount(0)
        for i, (base, subs) in enumerate(self.obj.Base):
            for sub in subs:
                self.form.baseList.insertRow(self.form.baseList.rowCount())

                item = QtGui.QTableWidgetItem("%s.%s" % (base.Label, sub))
                item.setFlags(item.flags() | QtCore.Qt.ItemIsUserCheckable)
                if self.obj.Proxy.isHoleEnabled(self.obj, base, sub):
                    item.setCheckState(QtCore.Qt.Checked)
                else:
                    item.setCheckState(QtCore.Qt.Unchecked)
                name = "%s.%s" % (base.Name, sub)
                item.setData(self.DataFeatureName, name)
                item.setData(self.DataObject, base)
                item.setData(self.DataObjectSub, sub)
                self.form.baseList.setItem(self.form.baseList.rowCount()-1, 0, item)

                item = QtGui.QTableWidgetItem("{:.3f}".format(self.obj.Proxy.holeDiameter(self.obj, base, sub)))
                item.setData(self.DataFeatureName, name)
                item.setData(self.DataObject, base)
                item.setData(self.DataObjectSub, sub)
                item.setTextAlignment(QtCore.Qt.AlignHCenter)
                self.form.baseList.setItem(self.form.baseList.rowCount()-1, 1, item)

        self.form.baseList.resizeColumnToContents(0)
        self.form.baseList.blockSignals(False)

    def itemActivated(self):
        PathLog.track()
        FreeCADGui.Selection.clearSelection()
        activatedRows = []
        for item in self.form.baseList.selectedItems():
            row = item.row()
            if not row in activatedRows:
                activatedRows.append(row)
                obj = item.data(self.DataObject)
                sub = str(item.data(self.DataObjectSub))
                PathLog.debug("itemActivated() -> %s.%s" % (obj.Label, sub))
                if sub:
                    FreeCADGui.Selection.addSelection(obj, sub)
                else:
                    FreeCADGui.Selection.addSelection(obj)
        #FreeCADGui.updateGui()

    def deleteBase(self):
        PathLog.track()
        deletedRows = []
        selected = self.form.baseList.selectedItems()
        for item in selected:
            row = self.form.baseList.row(item)
            if not row in deletedRows:
                deletedRows.append(row)
                self.form.baseList.removeRow(row)
        self.updateBase()
        #self.obj.Proxy.execute(self.obj)
        FreeCAD.ActiveDocument.recompute()

    def updateBase(self):
        PathLog.track()
        newlist = []
        for i in range(self.form.baseList.rowCount()):
            item = self.form.baseList.item(i, 0)
            obj = item.data(self.DataObject)
            sub = str(item.data(self.DataObjectSub))
            base = (obj, sub)
            PathLog.debug("keeping (%s.%s)" % (obj.Label, sub))
            newlist.append(base)
        PathLog.debug("obj.Base=%s newlist=%s" % (self.obj.Base, newlist))
        self.obj.Base = newlist

    def checkedChanged(self):
        PathLog.track()
        disabled = []
        for i in xrange(0, self.form.baseList.rowCount()):
            item = self.form.baseList.item(i, 0)
            if item.checkState() != QtCore.Qt.Checked:
                disabled.append(item.data(self.DataFeatureName))
        self.obj.Disabled = disabled
        FreeCAD.ActiveDocument.recompute()

    def registerSignalHandlers(self, obj):
        self.form.baseList.itemSelectionChanged.connect(self.itemActivated)
        self.form.addBase.clicked.connect(self.addBase)
        self.form.deleteBase.clicked.connect(self.deleteBase)
        self.form.resetBase.clicked.connect(self.resetBase)
        self.form.baseList.itemChanged.connect(self.checkedChanged)

    def resetBase(self):
        self.obj.Base = []
        self.obj.Disabled = []

        self.obj.Proxy.execute(self.obj)
        FreeCAD.ActiveDocument.recompute()

    def updateData(self, obj, prop):
        if prop in ['Base', 'Disabled']:
            self.setFields(self.obj)

class TaskPanelOpPage(PathOpGui.TaskPanelPage):

    def getForm(self):
        return FreeCADGui.PySideUic.loadUi(":/panels/PageOpDrillingEdit.ui")

    def getFields(self, obj):
        PathLog.track()
        self.obj.PeckDepth = FreeCAD.Units.Quantity(self.form.peckDepth.text()).Value
        self.obj.RetractHeight = FreeCAD.Units.Quantity(self.form.retractHeight.text()).Value
        self.obj.DwellTime = FreeCAD.Units.Quantity(self.form.dwellTime.text()).Value

        self.obj.DwellEnabled = self.form.dwellEnabled.isChecked()
        self.obj.PeckEnabled = self.form.peckEnabled.isChecked()
        self.obj.AddTipLength = self.form.useTipLength.isChecked()

        self.updateToolController(obj, self.form.toolController)

    def setFields(self, obj):
        PathLog.track()

        self.form.peckDepth.setText(FreeCAD.Units.Quantity(self.obj.PeckDepth.Value, FreeCAD.Units.Length).UserString)
        self.form.retractHeight.setText(FreeCAD.Units.Quantity(self.obj.RetractHeight.Value, FreeCAD.Units.Length).UserString)
        self.form.dwellTime.setText(str(self.obj.DwellTime))

        if self.obj.DwellEnabled:
            self.form.dwellEnabled.setCheckState(QtCore.Qt.Checked)
        else:
            self.form.dwellEnabled.setCheckState(QtCore.Qt.Unchecked)

        if self.obj.PeckEnabled:
            self.form.peckEnabled.setCheckState(QtCore.Qt.Checked)
        else:
            self.form.peckEnabled.setCheckState(QtCore.Qt.Unchecked)

        if self.obj.AddTipLength:
            self.form.useTipLength.setCheckState(QtCore.Qt.Checked)
        else:
            self.form.useTipLength.setCheckState(QtCore.Qt.Unchecked)

        self.setupToolController(self.obj, self.form.toolController)

    def getSignalsForUpdate(self, obj):
        signals = []
        signals.append(self.form.retractHeight.editingFinished)
        signals.append(self.form.peckDepth.editingFinished)
        signals.append(self.form.dwellTime.editingFinished)
        signals.append(self.form.dwellEnabled.stateChanged)
        signals.append(self.form.peckEnabled.stateChanged)
        signals.append(self.form.useTipLength.stateChanged)
        return signals

    def taskPanelBaseGeometryPage(self, obj, features):
        return TaskPanelHoleGeometryPage(obj, features)

PathOpGui.SetupOperation('Drilling',
        PathDrilling.Create,
        TaskPanelOpPage,
        'Path-Drilling',
        QtCore.QT_TRANSLATE_NOOP("PathDrilling", "Drilling"),
        "P, O",
        QtCore.QT_TRANSLATE_NOOP("PathDrilling", "Creates a Path Drilling object from a features of a base object"))

FreeCAD.Console.PrintLog("Loading PathDrillingGui... done\n")
