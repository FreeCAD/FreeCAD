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
from pivy import coin

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

def createExtensionSoSwitch(ext):
    sep = coin.SoSeparator()
    pos = coin.SoTranslation()
    mat = coin.SoMaterial()
    crd = coin.SoCoordinate3()
    fce = coin.SoFaceSet()

    wire =  ext.getWire()
    if wire:
        polygon = []
        for p in wire.discretize(Deflection=0.01):
            polygon.append((p.x, p.y, p.z))
        crd.point.setValues(polygon)

    mat.diffuseColor = (1.0, 0.0, 0.0)
    mat.transparency = 0.5

    sep.addChild(pos)
    sep.addChild(mat)
    sep.addChild(crd)
    sep.addChild(fce)

    switch = coin.SoSwitch()
    switch.addChild(sep)
    switch.whichChild = coin.SO_SWITCH_NONE

    return switch

class _Extension(object):
    def __init__(self, ext):
        self.ext = ext
        self.switch = createExtensionSoSwitch(ext)
        self.root = self.switch


class TaskPanelExtensionPage(PathOpGui.TaskPanelPage):
    DataObject = QtCore.Qt.ItemDataRole.UserRole
    DataSwitch = QtCore.Qt.ItemDataRole.UserRole + 2

    Direction = {
            PathPocketShape.Extension.DirectionNormal: translate('PathPocket', 'Normal'),
            PathPocketShape.Extension.DirectionX: translate('PathPocket', 'X'),
            PathPocketShape.Extension.DirectionY: translate('PathPocket', 'Y')
            }

    def initPage(self, obj):
        self.setTitle("Pocket Extensions")
        self.extensions = obj.Proxy.getExtensions(obj)

        self.defaultLength = PathGui.QuantitySpinBox(self.form.defaultLength, obj, 'ExtensionLengthDefault')

        self.form.extensions.setEditTriggers(QtGui.QAbstractItemView.NoEditTriggers)
        self.form.extensions.setSelectionBehavior(QtGui.QAbstractItemView.SelectRows)

        self.switch = coin.SoSwitch()
        self.obj.ViewObject.RootNode.addChild(self.switch)
        self.switch.whichChild = coin.SO_SWITCH_ALL

    def cleanupPage(self, obj):
        self.obj.ViewObject.RootNode.removeChild(self.switch)

    def getForm(self):
        return FreeCADGui.PySideUic.loadUi(":/panels/PageOpPocketExtEdit.ui")

    def getFields(self, obj):
        PathLog.track(obj.Label)
        self.defaultLength.updateProperty()
        exts = []
        for row in range(self.form.extensions.rowCount()):
            item = self.form.extensions.item(row, 0)
            exts.append(item.data(self.DataObject).ext)
        obj.Proxy.setExtensions(obj, exts)

    def setFields(self, obj):
        PathLog.track(obj.Label)
        self.defaultLength.updateSpinBox()
        self.setExtensions(self.extensions)

    def setExtensions(self, extensions):
        self.form.extensions.blockSignals(True)
        for row in range(self.form.extensions.rowCount()):
            self.switch.removeChild(self.form.extensions.item(row, 0).data(self.DataObject).root)

        self.form.extensions.clearContents()
        self.form.extensions.setRowCount(0)
        for row, ext in enumerate(extensions):
            PathLog.info("{}.{}".format(ext.obj.Label, ext.sub))
            self.form.extensions.insertRow(row)

            _ext = _Extension(ext)

            item0 = QtGui.QTableWidgetItem("{}.{}".format(ext.obj.Label, ext.sub))
            item0.setData(self.DataObject, _ext)
            self.form.extensions.setItem(row, 0, item0)

            item1 = QtGui.QTableWidgetItem("{}".format(ext.length))
            item1.setData(self.DataObject, _ext)
            item1.setFlags(item1.flags() & ~QtCore.Qt.ItemIsEnabled)
            self.form.extensions.setItem(row, 1, item1)

            item2 = QtGui.QTableWidgetItem("{}".format(self.Direction[ext.direction]))
            item2.setData(self.DataObject, _ext)
            item2.setFlags(item2.flags() & ~QtCore.Qt.ItemIsEnabled)
            self.form.extensions.setItem(row, 2, item2)

            self.switch.addChild(_ext.root)

        self.form.extensions.resizeColumnsToContents()
        self.form.extensions.blockSignals(False)
        self.extensions = extensions

    def updateData(self, obj, prop):
        if prop in ['ExtensionLengthDefault']:
            pass
        if prop in ['ExtensionFeature']:
            self.setExtensions(obj.Proxy.getExtensions(obj))

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

            FreeCADGui.Selection.clearSelection()
            print("rowCount = %s" % self.form.extensions.rowCount())

            for row in range(self.form.extensions.rowCount()):
                item = self.form.extensions.item(row, 0)
                ext = item.data(self.DataObject)
                ext.switch.whichChild = coin.SO_SWITCH_NONE

            processed = []
            for item in self.form.extensions.selectedItems():
                ext = item.data(self.DataObject)
                if not ext in processed:
                    FreeCADGui.Selection.addSelection(ext.ext.obj, ext.ext.sub)
                    ext.switch.whichChild = coin.SO_SWITCH_ALL
                    processed.append(ext)

    def extensionsAdd(self):
        extensions = self.extensions
        for sel in FreeCADGui.Selection.getSelectionEx():
            for subname in sel.SubElementNames:
                row = self.form.extensions.rowCount()
                extensions.append(self.obj.Proxy.createExtension(self.obj, sel.Object, subname))
        self.obj.Proxy.setExtensions(self.obj, extensions)
        self.setDirty()

    def extensionsClear(self):
        self.obj.Proxy.setExtensions(self.obj, [])
        self.setDirty()

    def extensionsRemove(self):
        extensions = self.extensions
        for item in self.form.extensions.selectedItems():
            ext = item.data(self.DataObject).ext
            if ext in extensions:
                extensions.remove(ext)
        self.obj.Proxy.setExtensions(self.obj, extensions)
        self.setDirty()

    def getSignalsForUpdate(self, obj):
        PathLog.track(obj.Label)
        return [self.form.defaultLength.editingFinished]

    def registerSignalHandlers(self, obj):
        self.form.buttonAdd.clicked.connect(self.extensionsAdd)
        self.form.buttonClear.clicked.connect(self.extensionsClear)
        self.form.buttonRemove.clicked.connect(self.extensionsRemove)

        self.updateSelection(self.obj, FreeCADGui.Selection.getSelectionEx())
        self.form.extensions.itemSelectionChanged.connect(self.itemSelectionChanged)
        self.itemSelectionChanged()

class TaskPanelOpPage(PathPocketBaseGui.TaskPanelOpPage):
    '''Page controller class for Pocket operation'''

    def pocketFeatures(self):
        '''pocketFeatures() ... return FeaturePocket (see PathPocketBaseGui)'''
        return PathPocketBaseGui.FeaturePocket | PathPocketBaseGui.FeatureOutline

    def taskPanelBaseLocationPage(self, obj, features):
        self.extensionsPanel = TaskPanelExtensionPage(obj, features)
        return self.extensionsPanel

    def pageRegisterSignalHandlers(self):
        pass

Command = PathOpGui.SetupOperation('Pocket Shape',
        PathPocketShape.Create,
        TaskPanelOpPage,
        'Path-Pocket',
        QtCore.QT_TRANSLATE_NOOP("PathPocket", "Pocket Shape"),
        QtCore.QT_TRANSLATE_NOOP("PathPocket", "Creates a Path Pocket object from a face or faces"),
        PathPocketShape.SetupProperties)

FreeCAD.Console.PrintLog("Loading PathPocketShapeGui... done\n")
