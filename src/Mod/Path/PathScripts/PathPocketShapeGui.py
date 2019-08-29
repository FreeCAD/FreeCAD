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
import Part
import PathScripts.PathGeom as PathGeom
import PathScripts.PathGui as PathGui
import PathScripts.PathLog as PathLog
import PathScripts.PathOpGui as PathOpGui
import PathScripts.PathPocketShape as PathPocketShape
import PathScripts.PathPocketBaseGui as PathPocketBaseGui

from PySide import QtCore, QtGui
from pivy import coin

__title__ = "Path Pocket Shape Operation UI"
__author__ = "sliptonic (Brad Collette)"
__url__ = "http://www.freecadweb.org"
__doc__ = "Pocket Shape operation page controller and command implementation."

def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)

LOGLEVEL = False

if LOGLEVEL:
    PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())
    PathLog.trackModule(PathLog.thisModule())
else:
    PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())

class _Extension(object):
    ColourEnabled = (1.0,  .5, 1.0)
    ColourDisabled = (1.0, 1.0, .5)
    TransparencySelected = 0.0
    TransparencyDeselected = 0.7

    def __init__(self, obj, base, face, edge):
        self.obj = obj
        self.base = base
        self.face = face
        self.edge = edge
        if edge is None:
            self.ext = None
        else:
            self.ext  = obj.Proxy.createExtension(obj, base, face, edge)
        self.switch = self.createExtensionSoSwitch(self.ext)
        self.root = self.switch

    def createExtensionSoSwitch(self, ext):
        sep = coin.SoSeparator()
        pos = coin.SoTranslation()
        mat = coin.SoMaterial()
        crd = coin.SoCoordinate3()
        fce = coin.SoFaceSet()
        hnt = coin.SoShapeHints()

        if not ext is None:
            wire =  ext.getWire()
            if wire:
                if isinstance(wire, (list, tuple)):
                    p0 = [p for p in wire[0].discretize(Deflection=0.02)]
                    p1 = [p for p in wire[1].discretize(Deflection=0.02)]
                    p2 = list(reversed(p1))
                    polygon = [(p.x, p.y, p.z) for p in (p0 + p2)]
                else:
                    poly = [p for p in wire.discretize(Deflection=0.02)][:-1]
                    polygon = [(p.x, p.y, p.z) for p in poly]
                crd.point.setValues(polygon)
            else:
                return None

            mat.diffuseColor = self.ColourDisabled
            mat.transparency = self.TransparencyDeselected

            hnt.faceType = coin.SoShapeHints.UNKNOWN_FACE_TYPE
            hnt.vertexOrdering = coin.SoShapeHints.CLOCKWISE

            sep.addChild(pos)
            sep.addChild(mat)
            sep.addChild(hnt)
            sep.addChild(crd)
            sep.addChild(fce)

        switch = coin.SoSwitch()
        switch.addChild(sep)
        switch.whichChild = coin.SO_SWITCH_NONE

        self.material = mat

        return switch

    def _setColour(self, r, g, b):
        self.material.diffuseColor = (r, g, b)

    def isValid(self):
        return not self.root is None

    def show(self):
        self.switch.whichChild = coin.SO_SWITCH_ALL

    def hide(self):
        self.switch.whichChild = coin.SO_SWITCH_NONE

    def enable(self, ena = True):
        if ena:
            self.material.diffuseColor = self.ColourEnabled
        else:
            self.disable()

    def disable(self):
        self.material.diffuseColor = self.ColourDisabled

    def select(self):
        self.material.transparency = self.TransparencySelected

    def deselect(self):
        self.material.transparency = self.TransparencyDeselected

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
        self.extensions = obj.Proxy.getExtensions(obj) # pylint: disable=attribute-defined-outside-init

        self.defaultLength = PathGui.QuantitySpinBox(self.form.defaultLength, obj, 'ExtensionLengthDefault') # pylint: disable=attribute-defined-outside-init

        self.form.extensionTree.setEditTriggers(QtGui.QAbstractItemView.NoEditTriggers)
        self.form.extensionTree.setSelectionBehavior(QtGui.QAbstractItemView.SelectRows)

        self.switch = coin.SoSwitch() # pylint: disable=attribute-defined-outside-init
        self.obj.ViewObject.RootNode.addChild(self.switch)
        self.switch.whichChild = coin.SO_SWITCH_ALL

        self.model = QtGui.QStandardItemModel(self.form.extensionTree) # pylint: disable=attribute-defined-outside-init
        self.model.setHorizontalHeaderLabels(['Base', 'Extension'])

        if 0 < len(obj.ExtensionFeature):
            self.form.showExtensions.setCheckState(QtCore.Qt.Checked)
        else:
            self.form.showExtensions.setCheckState(QtCore.Qt.Unchecked)

        self.blockUpdateData = False # pylint: disable=attribute-defined-outside-init

    def cleanupPage(self, obj):
        # If the object was already destroyed we can't access obj.Name.
        # This is the case if this was a new op and the user hit Cancel.
        # Unfortunately there's no direct way to determine the object's
        # livelihood without causing an error so we look for the object
        # in the document and clean up if it still exists.
        for o in self.obj.Document.getObjectsByLabel(self.obj.Label):
            if o == obj:
                self.obj.ViewObject.RootNode.removeChild(self.switch)
                return
        PathLog.debug("%s already destroyed - no cleanup required" % (obj.Label))

    def getForm(self):
        return FreeCADGui.PySideUic.loadUi(":/panels/PageOpPocketExtEdit.ui")

    def forAllItemsCall(self, cb):
        for modelRow in range(self.model.rowCount()):
            model = self.model.item(modelRow, 0)
            for featureRow in range(model.rowCount()):
                feature = model.child(featureRow, 0)
                for edgeRow in range(feature.rowCount()):
                    item = feature.child(edgeRow, 0)
                    ext = item.data(self.DataObject)
                    cb(item, ext)

    def currentExtensions(self):
        extensions = []
        def extractExtension(item, ext):
            if ext and ext.edge and item.checkState() == QtCore.Qt.Checked:
                extensions.append(ext.ext)
        self.forAllItemsCall(extractExtension)
        PathLog.track('extensions', extensions)
        return extensions

    def updateProxyExtensions(self, obj):
        self.extensions = self.currentExtensions() # pylint: disable=attribute-defined-outside-init
        obj.Proxy.setExtensions(obj, self.extensions)

    def getFields(self, obj):
        PathLog.track(obj.Label, self.model.rowCount(), self.model.columnCount())
        self.blockUpdateData = True # pylint: disable=attribute-defined-outside-init

        if obj.ExtensionCorners != self.form.extendCorners.isChecked():
            obj.ExtensionCorners = self.form.extendCorners.isChecked()
        self.defaultLength.updateProperty()

        self.updateProxyExtensions(obj)
        self.blockUpdateData = False # pylint: disable=attribute-defined-outside-init

    def setFields(self, obj):
        PathLog.track(obj.Label)

        if obj.ExtensionCorners != self.form.extendCorners.isChecked():
            self.form.extendCorners.toggle()
        self.defaultLength.updateSpinBox()
        self.extensions = obj.Proxy.getExtensions(obj) # pylint: disable=attribute-defined-outside-init
        self.setExtensions(self.extensions)

    def createItemForBaseModel(self, base, sub, edges, extensions):
        PathLog.track()
        ext = _Extension(self.obj, base, sub, None)
        item = QtGui.QStandardItem()
        item.setData(sub, QtCore.Qt.EditRole)
        item.setData(ext, self.DataObject)
        item.setSelectable(False)

        extendCorners = self.form.extendCorners.isChecked()

        def createSubItem(label, ext0):
            self.switch.addChild(ext0.root)
            item0 = QtGui.QStandardItem()
            item0.setData(label, QtCore.Qt.EditRole)
            item0.setData(ext0, self.DataObject)
            item0.setCheckable(True)
            for e in extensions:
                if e.obj == base and e.sub == label:
                    item0.setCheckState(QtCore.Qt.Checked)
                    ext0.enable()
                    break
            item.appendRow([item0])

        extensionEdges = {}
        for edge in base.Shape.getElement(sub).Edges:
            for (e, label) in edges:
                if edge.isSame(e):
                    ext0 = _Extension(self.obj, base, sub, label)
                    if ext0.isValid():
                        extensionEdges[e] = label[4:]
                        if not extendCorners:
                            createSubItem(label, ext0)
                    break

        if extendCorners:
            def edgesMatchShape(e0, e1):
                return PathGeom.edgesMatch(e0, e1) or PathGeom.edgesMatch(e0, PathGeom.flipEdge(e1))

            self.extensionEdges = extensionEdges # pylint: disable=attribute-defined-outside-init
            for edgeList in Part.sortEdges(list(extensionEdges.keys())):
                self.edgeList = edgeList # pylint: disable=attribute-defined-outside-init
                if len(edgeList) == 1:
                    label = "Edge%s" % [extensionEdges[keyEdge] for keyEdge in extensionEdges.keys() if edgesMatchShape(keyEdge, edgeList[0])][0]
                else:
                    label = "Wire(%s)" % ','.join(sorted([extensionEdges[keyEdge] for e in edgeList for keyEdge in extensionEdges.keys() if edgesMatchShape(e, keyEdge)], key=lambda s: int(s))) # pylint: disable=unnecessary-lambda
                ext0 = _Extension(self.obj, base, sub, label)
                createSubItem(label, ext0)

        return item

    def setExtensions(self, extensions):
        PathLog.track(len(extensions))
        self.form.extensionTree.blockSignals(True)

        # remember current visual state
        if hasattr(self, 'selectionModel'):
            selectedExtensions = [self.model.itemFromIndex(index).data(self.DataObject).ext for index in self.selectionModel.selectedIndexes()]
        else:
            selectedExtensions = []
        collapsedModels = []
        collapsedFeatures = []
        for modelRow in range(self.model.rowCount()):
            model = self.model.item(modelRow, 0)
            modelName = model.data(QtCore.Qt.EditRole)
            if not self.form.extensionTree.isExpanded(model.index()):
                collapsedModels.append(modelName)
            for featureRow in range(model.rowCount()):
                feature = model.child(featureRow, 0)
                if not self.form.extensionTree.isExpanded(feature.index()):
                    collapsedFeatures.append("%s.%s" % (modelName, feature.data(QtCore.Qt.EditRole)))

        # remove current extensions and all their visuals
        def removeItemSwitch(item, ext):
            # pylint: disable=unused-argument
            ext.hide()
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

        self.form.extensionTree.setModel(self.model)
        self.form.extensionTree.expandAll()
        self.form.extensionTree.resizeColumnToContents(0)

        # restore previous state - at least the parts that are still valid
        for modelRow in range(self.model.rowCount()):
            model = self.model.item(modelRow, 0)
            modelName = model.data(QtCore.Qt.EditRole)
            if modelName in collapsedModels:
                self.form.extensionTree.setExpanded(model.index(), False)
            for featureRow in range(model.rowCount()):
                feature = model.child(featureRow, 0)
                featureName =  "%s.%s" % (modelName, feature.data(QtCore.Qt.EditRole))
                if featureName in collapsedFeatures:
                    self.form.extensionTree.setExpanded(feature.index(), False)
        if hasattr(self, 'selectionModel') and selectedExtensions:
            self.restoreSelection(selectedExtensions)

        self.form.extensionTree.blockSignals(False)

    def updateData(self, obj, prop):
        PathLog.track(obj.Label, prop, self.blockUpdateData)
        if not self.blockUpdateData:
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
                # pylint: disable=unused-argument
                for sel in selection:
                    if ext.base == sel.obj and ext.edge == sel.sub:
                        return True
                return False

            def setSelectionVisuals(item, ext):
                if selectItem(item, ext):
                    self.selectionModel.select(item.index(), QtCore.QItemSelectionModel.Select)

                selected = self.selectionModel.isSelected(item.index())
                if selected:
                    FreeCADGui.Selection.addSelection(ext.base, ext.face)
                    ext.select()
                else:
                    ext.deselect()

                if self.form.showExtensions.isChecked() or selected:
                    ext.show()
                else:
                    ext.hide()
            self.forAllItemsCall(setSelectionVisuals)

    def selectionChanged(self):
        self.restoreSelection([])

    def extensionsClear(self):
        def disableItem(item, ext):
            item.setCheckState(QtCore.Qt.Unchecked)
            ext.disable()

        self.forAllItemsCall(disableItem)
        self.setDirty()

    def _extensionsSetState(self, state):
        PathLog.track(state)
        for index in self.selectionModel.selectedIndexes():
            item = self.model.itemFromIndex(index)
            ext = item.data(self.DataObject)
            if ext.edge:
                item.setCheckState(state)
                ext.enable(state == QtCore.Qt.Checked)
        self.setDirty()

    def extensionsDisable(self):
        self._extensionsSetState(QtCore.Qt.Unchecked)

    def extensionsEnable(self):
        self._extensionsSetState(QtCore.Qt.Checked)

    def updateItemEnabled(self, item):
        PathLog.track(item)
        ext = item.data(self.DataObject)
        if item.checkState() == QtCore.Qt.Checked:
            ext.enable()
        else:
            ext.disable()
        self.updateProxyExtensions(self.obj)
        self.setDirty()

    def showHideExtension(self):
        if self.form.showExtensions.isChecked():
            def enableExtensionEdit(item, ext):
                # pylint: disable=unused-argument
                ext.show()
            self.forAllItemsCall(enableExtensionEdit)
        else:
            def disableExtensionEdit(item, ext):
                if not self.selectionModel.isSelected(item.index()):
                    ext.hide()
            self.forAllItemsCall(disableExtensionEdit)
        #self.setDirty()

    def toggleExtensionCorners(self):
        PathLog.track()
        self.setExtensions(self.obj.Proxy.getExtensions(self.obj))
        self.selectionChanged()
        self.setDirty()

    def getSignalsForUpdate(self, obj):
        PathLog.track(obj.Label)
        signals = []
        signals.append(self.form.defaultLength.editingFinished)
        return signals

    def registerSignalHandlers(self, obj):
        self.form.showExtensions.clicked.connect(self.showHideExtension)
        self.form.extendCorners.clicked.connect(self.toggleExtensionCorners)
        self.form.buttonClear.clicked.connect(self.extensionsClear)
        self.form.buttonDisable.clicked.connect(self.extensionsDisable)
        self.form.buttonEnable.clicked.connect(self.extensionsEnable)

        self.model.itemChanged.connect(self.updateItemEnabled)

        self.selectionModel = self.form.extensionTree.selectionModel() # pylint: disable=attribute-defined-outside-init
        self.selectionModel.selectionChanged.connect(self.selectionChanged)
        self.selectionChanged()

class TaskPanelOpPage(PathPocketBaseGui.TaskPanelOpPage):
    '''Page controller class for Pocket operation'''

    def pocketFeatures(self):
        '''pocketFeatures() ... return FeaturePocket (see PathPocketBaseGui)'''
        return PathPocketBaseGui.FeaturePocket | PathPocketBaseGui.FeatureOutline

    def taskPanelBaseLocationPage(self, obj, features):
        if not hasattr(self, 'extensionsPanel'):
            self.extensionsPanel = TaskPanelExtensionPage(obj, features) # pylint: disable=attribute-defined-outside-init
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
