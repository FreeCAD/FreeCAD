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
import PathScripts.PathGui as PathGui
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

def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)

if True:
    PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())
    PathLog.trackModule(PathLog.thisModule())
else:
    PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())


Wires = []

class TaskPanelExtensionPage(PathOpGui.TaskPanelPage):
    DataObject = QtCore.Qt.ItemDataRole.UserRole
    Direction = {
            PathPocketShape.Extension.DirectionNormal: translate('PathPocket', 'Normal'),
            PathPocketShape.Extension.DirectionX: translate('PathPocket', 'X'),
            PathPocketShape.Extension.DirectionY: translate('PathPocket', 'Y')
            }

    def initPage(self, obj):
        self.setTitle("Pocket Extensions")
        self.enabled = not obj.UseOutline
        self.enable(not self.enabled)
        self.extensions = obj.Proxy.getExtensions(obj)

        self.defaultLength = PathGui.QuantitySpinBox(self.form.defaultLength, obj, 'ExtensionLengthDefault')

        self.form.extensions.setEditTriggers(QtGui.QAbstractItemView.NoEditTriggers)
        self.form.extensions.setSelectionBehavior(QtGui.QAbstractItemView.SelectRows)

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

    def getFields(self, obj):
        self.defaultLength.updateProperty()
        exts = []
        for row in range(self.form.extensions.rowCount()):
            item = self.form.extensions.item(row, 0)
            exts.append(item.data(self.DataObject))
        obj.Proxy.setExtensions(obj, exts)

    def setFields(self, obj):
        self.defaultLength.updateSpinBox()

        self.form.extensions.blockSignals(True)
        self.form.extensions.clearContents()
        self.form.extensions.setRowCount(0)
        for row, ext in enumerate(self.extensions):
            PathLog.info("{}.{}".format(ext.obj.Label, ext.sub))

            self.form.extensions.insertRow(row)

            item = QtGui.QTableWidgetItem("{}.{}".format(ext.obj.Label, ext.sub))
            item.setData(self.DataObject, ext)
            self.form.extensions.setItem(row, 0, item)

            item = QtGui.QTableWidgetItem("{}".format(ext.length))
            item.setData(self.DataObject, ext)
            self.form.extensions.setItem(row, 1, item)

            item = QtGui.QTableWidgetItem("{}".format(self.Direction[ext.direction]))
            item.setData(self.DataObject, ext)
            self.form.extensions.setItem(row, 2, item)

        self.form.extensions.resizeColumnsToContents()
        self.form.extensions.blockSignals(False)

    def updateSelection(self, obj, sel):
        if sel and sel[0].SubElementNames:
            self.form.buttonAdd.setEnabled(True)
        else:
            self.form.buttonAdd.setEnabled(False)

    def itemSelectionChanged(self):
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
        for sel in FreeCADGui.Selection.getSelectionEx():
            for subname in sel.SubElementNames:
                row = self.form.extensions.rowCount()
                self.extensions.append(self.obj.Proxy.createExtension(self.obj, sel.Object, subname))
        self.setFields(self.obj)
        self.setDirty()

    def extensionsClear(self):
        self.form.extensions.clearContents()
        self.form.extensions.setRowCount(0)

    def extensionsRemove(self):
        global Wires
        Wires = []
        processed = []
        for item in self.form.extensions.selectedItems():
            ext = item.data(self.DataObject)
            if not ext in processed:
                Wires.append(ext)
                wire = ext.getWire()
                if wire:
                    import Part
                    Part.show(wire)
                processed.append(ext)

    def pageRegisterSignalHandlers(self):
        self.form.extensions.itemSelectionChanged.connect(self.itemSelectionChanged)
        self.form.buttonAdd.clicked.connect(self.extensionsAdd)
        self.form.buttonClear.clicked.connect(self.extensionsClear)
        self.form.buttonRemove.clicked.connect(self.extensionsRemove)

        self.updateSelection(self.obj, FreeCADGui.Selection.getSelectionEx())
        self.itemSelectionChanged()

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
