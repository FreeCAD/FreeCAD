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

    if not ext is None:
        wire =  ext.getWire()
        if wire:
            polygon = []
            for p in wire.discretize(Deflection=0.01):
                polygon.append((p.x, p.y, p.z))
            crd.point.setValues(polygon)
        else:
            return None

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
    def __init__(self, obj, base, face, edge):
        self.base = base
        self.face = face
        self.edge = edge
        if edge is None:
            self.ext = None
        else:
            self.ext  = obj.Proxy.createExtension(obj, base, edge)
        self.switch = createExtensionSoSwitch(self.ext)
        self.root = self.switch

    def isValid(self):
        return not self.root is None

Page = None

class TaskPanelExtensionPage(PathOpGui.TaskPanelPage):
    DataObject = QtCore.Qt.ItemDataRole.UserRole
    DataSwitch = QtCore.Qt.ItemDataRole.UserRole + 2

    Direction = {
            PathPocketShape.Extension.DirectionNormal: translate('PathPocket', 'Normal'),
            PathPocketShape.Extension.DirectionX: translate('PathPocket', 'X'),
            PathPocketShape.Extension.DirectionY: translate('PathPocket', 'Y')
            }

    def initPage(self, obj):
        self.setTitle("Extensions")
        self.extensions = obj.Proxy.getExtensions(obj)

        self.defaultLength = PathGui.QuantitySpinBox(self.form.defaultLength, obj, 'ExtensionLengthDefault')

        self.form.extensions.setEditTriggers(QtGui.QAbstractItemView.NoEditTriggers)
        self.form.extensions.setSelectionBehavior(QtGui.QAbstractItemView.SelectRows)

        self.switch = coin.SoSwitch()
        self.obj.ViewObject.RootNode.addChild(self.switch)
        self.switch.whichChild = coin.SO_SWITCH_ALL

        self.model = QtGui.QStandardItemModel(self.form.extensions)
        self.model.setHorizontalHeaderLabels(['Base', 'Extension'])

        global Page
        Page = self

    def cleanupPage(self, obj):
        self.obj.ViewObject.RootNode.removeChild(self.switch)

    def getForm(self):
        return FreeCADGui.PySideUic.loadUi(":/panels/PageOpPocketExtEdit.ui")

    def forAllItemsCall(self, cb):
        PathLog.track()
        for modelRow in range(self.model.rowCount()):
            model = self.model.item(modelRow, 0)
            for featureRow in range(model.rowCount()):
                feature = model.child(featureRow, 0);
                for edgeRow in range(feature.rowCount()):
                    cb(feature.child(edgeRow, 0))

    def getFields(self, obj):
        PathLog.track(obj.Label, self.model.rowCount(), self.model.columnCount())
        self.defaultLength.updateProperty()
        extensions = []

        def extractExtension(item):
            ext = item.data(self.DataObject)
            if ext and ext.edge and item.checkState() == QtCore.Qt.Checked:
                extensions.append(ext.ext)

        self.forAllItemsCall(extractExtension)

        self.extensions = extensions
        obj.Proxy.setExtensions(obj, extensions)

    def setFields(self, obj):
        PathLog.track(obj.Label)
        self.defaultLength.updateSpinBox()
        self.setExtensions(self.extensions)

    def createItemForBaseModel(self, base, sub, edges, extensions):
        PathLog.track()
        ext = _Extension(self.obj, base, sub, None)
        item = QtGui.QStandardItem()
        item.setData(sub, QtCore.Qt.EditRole)
        item.setData(ext, self.DataObject)
        item.setSelectable(False)

        for edge in base.Shape.getElement(sub).Edges:
            for (e, label) in edges:
                if edge.isSame(e):
                    ext0 = _Extension(self.obj, base, sub, label)
                    if ext0.isValid():
                        self.switch.addChild(ext0.root)
                        item0 = QtGui.QStandardItem()
                        item0.setData(label, QtCore.Qt.EditRole)
                        item0.setData(ext0, self.DataObject)
                        item0.setCheckable(True)
                        for e in extensions:
                            if e.obj == base and e.sub == label:
                                item0.setCheckState(QtCore.Qt.Checked)
                                break
                        item.appendRow([item0])
                    break

        return item

    def setExtensions(self, extensions):
        PathLog.track(len(extensions))
        self.form.extensions.blockSignals(True)

        # remember current visual state
        if hasattr(self, 'selectionModel'):
            selectedExtensions = [self.model.itemFromIndex(index).data(self.DataObject).ext for index in self.selectionModel.selectedIndexes()]
        else:
            selectedExtensions = []

        # remove current extensions and all their visuals
        def removeItemSwitch(item):
            ext = item.data(self.DataObject)
            ext.switch.whichChild = coin.SO_SWITCH_NONE
            self.switch.removeChild(ext.root)
        self.forAllItemsCall(removeItemSwitch)
        self.model.clear()

        # create extensions for model and given argument
        for base in self.obj.Base:
            edges = [(edge, "Edge%d" % (i + 1)) for i, edge in enumerate(base[0].Shape.Edges)]
            baseItem = QtGui.QStandardItem()
            baseItem.setData(base[0].Label, QtCore.Qt.EditRole)
            baseItem.setSelectable(False)
            for sub in sorted(base[1]):
                baseItem.appendRow(self.createItemForBaseModel(base[0], sub, edges, extensions))
            self.model.appendRow(baseItem)

        self.form.extensions.setModel(self.model)
        self.form.extensions.expandAll()
        self.form.extensions.resizeColumnToContents(0)

        # restore previous state - at least the parts that are still valid
        if hasattr(self, 'selectionModel') and selectedExtensions:
            self.restoreSelection(selectedExtensions)

    def updateData(self, obj, prop):
        if prop in ['Base', 'ExtensionLengthDefault']:
            self.setExtensions(obj.Proxy.getExtensions(obj))

    def restoreSelection(self, selection):
        PathLog.track()
        if 0 == self.model.rowCount():
            PathLog.track('-')
            self.form.buttonClear.setEnabled(False)
            self.form.buttonDisable.setEnabled(False)
            self.form.buttonEnable.setEnabled(False)
        else:
            self.form.buttonClear.setEnabled(True)

            if selection or self.selectionModel.selectedIndexes():
                self.form.buttonDisable.setEnabled(True)
                self.form.buttonEnable.setEnabled(True)
            else:
                self.form.buttonDisable.setEnabled(False)
                self.form.buttonEnable.setEnabled(False)

            FreeCADGui.Selection.clearSelection()

            def selectItem(item, ext):
                for sel in selection:
                    if ext.base == sel.obj and ext.edge == sel.sub:
                        return True
                return False

            def setSelectionVisuals(item):
                ext = item.data(self.DataObject)
                if selectItem(item, ext):
                    self.selectionModel.select(item.index(), QtGui.QItemSelectionModel.Select)
                if self.selectionModel.isSelected(item.index()):
                    FreeCADGui.Selection.addSelection(ext.base, ext.face)
                    ext.switch.whichChild = coin.SO_SWITCH_ALL
                else:
                    ext.switch.whichChild = coin.SO_SWITCH_NONE
            self.forAllItemsCall(setSelectionVisuals)

    def selectionChanged(self):
        self.restoreSelection([])

    def extensionsClear(self):
        self.forAllItemsCall(lambda item: item.setCheckState(QtCore.Qt.Unchecked))
        self.setDirty()

    def _extensionsSetState(self, state):
        for index in self.selectionModel.selectedIndexes():
            item = self.model.itemFromIndex(index)
            ext = item.data(self.DataObject)
            if ext.edge:
                item.setCheckState(state)
        self.setDirty()

    def extensionsDisable(self):
        self._extensionsSetState(QtCore.Qt.Unchecked)

    def extensionsEnable(self):
        self._extensionsSetState(QtCore.Qt.Checked)


    def getSignalsForUpdate(self, obj):
        PathLog.track(obj.Label)
        return [self.form.defaultLength.editingFinished]

    def registerSignalHandlers(self, obj):
        self.form.buttonClear.clicked.connect(self.extensionsClear)
        self.form.buttonDisable.clicked.connect(self.extensionsDisable)
        self.form.buttonEnable.clicked.connect(self.extensionsEnable)

        self.model.itemChanged.connect(lambda x: self.setDirty())

        self.selectionModel = self.form.extensions.selectionModel()
        self.selectionModel.selectionChanged.connect(self.selectionChanged)

class TaskPanelOpPage(PathPocketBaseGui.TaskPanelOpPage):
    '''Page controller class for Pocket operation'''

    def pocketFeatures(self):
        '''pocketFeatures() ... return FeaturePocket (see PathPocketBaseGui)'''
        return PathPocketBaseGui.FeaturePocket | PathPocketBaseGui.FeatureOutline

    def taskPanelBaseLocationPage(self, obj, features):
        if not hasattr(self, 'extensionsPanel'):
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
