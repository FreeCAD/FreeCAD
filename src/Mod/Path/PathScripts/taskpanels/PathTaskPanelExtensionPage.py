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

import Part
import PathScripts.PathGeom as PathGeom
import PathScripts.PathGui as PathGui
import PathScripts.PathLog as PathLog
import PathScripts.taskpanels.PathTaskPanelPage as PathTaskPanelPage
import PathScripts.PathFeatureExtensions as PathFeatureExtensions
from pivy import coin

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


class TaskPanelExtensionPage(PathTaskPanelPage.TaskPanelPage):
    DataObject = QtCore.Qt.ItemDataRole.UserRole
    DataSwitch = QtCore.Qt.ItemDataRole.UserRole + 2

    Direction = {
        PathFeatureExtensions.Extension.DirectionNormal: translate(
            "PathPocket", "Normal"
        ),
        PathFeatureExtensions.Extension.DirectionX: translate("PathPocket", "X"),
        PathFeatureExtensions.Extension.DirectionY: translate("PathPocket", "Y"),
    }

    def initPage(self, obj):
        self.obj = obj
        self.title = "Extensions"
        # self.setTitle("Extensions")
        self.OpIcon = ":/icons/view-axonometric.svg"
        self.setIcon(self.OpIcon)
        self.initialEdgeCount = -1
        self.edgeCountThreshold = 30
        self.fieldsSet = False
        self.operationPageForm = None
        self.usePerimeter = False
        self.extensionsCache = dict()
        self.extensionsReady = False
        self.enabled = True
        self.extensions = list()

        self.defaultLength = PathGui.QuantitySpinBox(
            self.form.defaultLength, obj, "ExtensionLengthDefault"
        )  # pylint: disable=attribute-defined-outside-init

        self.form.extensionTree.setEditTriggers(QtGui.QAbstractItemView.NoEditTriggers)
        self.form.extensionTree.setSelectionBehavior(QtGui.QAbstractItemView.SelectRows)

        self.switch = coin.SoSwitch()  # pylint: disable=attribute-defined-outside-init
        self.obj.ViewObject.RootNode.addChild(self.switch)
        self.switch.whichChild = coin.SO_SWITCH_ALL

        self.model = QtGui.QStandardItemModel(
            self.form.extensionTree
        )  # pylint: disable=attribute-defined-outside-init
        self.model.setHorizontalHeaderLabels(["Base", "Extension"])

        """
        # russ4262: This `if` block shows all available extensions upon edit of operation with any extension enabled.
        # This can cause the model(s) to overly obscured due to previews of extensions.
        # Would be great if only enabled extensions were shown.
        if 0 < len(obj.ExtensionFeature):
            self.form.showExtensions.setCheckState(QtCore.Qt.Checked)
        else:
            self.form.showExtensions.setCheckState(QtCore.Qt.Unchecked)
        """
        self.form.showExtensions.setCheckState(QtCore.Qt.Unchecked)

        self.blockUpdateData = False  # pylint: disable=attribute-defined-outside-init

    def cleanupPage(self, obj):
        try:
            self.obj.ViewObject.RootNode.removeChild(self.switch)
        except ReferenceError:
            PathLog.debug("obj already destroyed - no cleanup required")

    def getForm(self):
        form = FreeCADGui.PySideUic.loadUi(":/panels/PageExtensionsEdit.ui")
        # Hide warning label by default
        form.enableExtensionsWarning.hide()
        form.includeEdges.setChecked(False)
        form.includeSpecial.setChecked(False)
        return form

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
        PathLog.debug("currentExtensions()")
        extensions = []

        def extractExtension(item, ext):
            if ext and ext.edge and item.checkState() == QtCore.Qt.Checked:
                extensions.append(ext.ext)

        if self.form.enableExtensions.isChecked():
            self.forAllItemsCall(extractExtension)
        PathLog.track("extensions", extensions)
        return extensions

    def updateProxyExtensions(self, obj):
        PathLog.debug("updateProxyExtensions()")
        self.extensions = (
            self.currentExtensions()
        )  # pylint: disable=attribute-defined-outside-init
        PathFeatureExtensions.setExtensions(obj, self.extensions)

    def getFields(self, obj):
        PathLog.track(obj.Label, self.model.rowCount(), self.model.columnCount())
        self.blockUpdateData = True  # pylint: disable=attribute-defined-outside-init

        if obj.ExtensionCorners != self.form.extendCorners.isChecked():
            obj.ExtensionCorners = self.form.extendCorners.isChecked()
        self.defaultLength.updateProperty()

        self.updateProxyExtensions(obj)
        self.blockUpdateData = False  # pylint: disable=attribute-defined-outside-init

    def setFields(self, obj):
        PathLog.track(obj.Label)
        PathLog.debug("setFields()")

        if obj.ExtensionCorners != self.form.extendCorners.isChecked():
            self.form.extendCorners.toggle()

        self._findOperationPageForm()
        self._getUsePerimeterState()  # Determine if only perimeter is being processed
        self._autoEnableExtensions()  # Check edge count for auto-disable Extensions on initial Task Panel loading
        self._initializeExtensions(obj)  # Efficiently initialize Extensions
        self.defaultLength.updateSpinBox()
        self.fieldsSet = True  # flag to identify initial values set

    def _initializeExtensions(self, obj):
        """_initializeExtensions()...
        Subroutine called inside `setFields()` to initialize Extensions efficiently."""
        PathLog.debug("_initializeExtensions(self.enabled={})".format(self.enabled))

        # Check for existing extensions first
        if (
            len(obj.ExtensionFeature) > 0
        ):  # latter test loads pre-existing extensions (editing of existing operation)
            hasEdges = False
            hasSpecial = False
            for (__, __, subFeat) in PathFeatureExtensions.readObjExtensionFeature(obj):
                if subFeat.startswith("Edge") or subFeat.startswith("Wire"):
                    hasEdges = True
                if subFeat.startswith("Extend") or subFeat.startswith("Waterline"):
                    hasSpecial = True

            self.extensions = PathFeatureExtensions.getExtensions(obj)
            self.form.enableExtensions.setChecked(True)

            if not hasEdges and not hasSpecial:
                self._enableExtensions()

            if hasEdges:
                self.form.includeEdges.setChecked(True)
                self._includeEdgesAndWires()
            if hasSpecial:
                self.form.includeSpecial.setChecked(True)
                self._includeSpecialExtensions()
        elif self.enabled:
            self.extensions = PathFeatureExtensions.getExtensions(obj)
            self.form.includeEdges.setChecked(True)
            # self.form.includeSpecial.setChecked(True)  # Leave special disabled until user enables them

        self.setExtensions(self.extensions)

    def updateQuantitySpinBoxes(self, index=None):
        prevValue = self.form.defaultLength.text()
        self.defaultLength.updateSpinBox()
        postValue = self.form.defaultLength.text()

        if postValue != prevValue:
            PathLog.debug("updateQuantitySpinBoxes() post != prev value")
            self._resetCachedExtensions()  # Reset extension cache because extension dimensions likely changed
            self._enableExtensions()  # Recalculate extensions

    def createItemForBaseModel(self, base, sub, edges, extensions):
        PathLog.track(
            base.Label, sub, "+", len(edges), len(base.Shape.getElement(sub).Edges)
        )
        # PathLog.debug("createItemForBaseModel() label: {}, sub: {}, {}, edgeCnt: {}, subEdges: {}".format(base.Label, sub, '+', len(edges), len(base.Shape.getElement(sub).Edges)))

        extendCorners = self.form.extendCorners.isChecked()
        includeEdges = self.form.includeEdges.isChecked()
        includeSpecial = self.form.includeSpecial.isChecked()
        subShape = base.Shape.getElement(sub)
        subHasItem = False

        def createSubItem(label, ext0):
            # PathLog.info("createSubItem({})".format(label))
            if ext0.root:
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

        # ext = self._cachedExtension(self.obj, base, sub, None)
        ext = None
        item = QtGui.QStandardItem()
        item.setData(sub, QtCore.Qt.EditRole)
        item.setData(ext, self.DataObject)
        item.setSelectable(False)

        extensionEdges = {}
        if includeEdges:
            if self.usePerimeter and sub.startswith("Face"):
                # Only show exterior extensions if `Use Outline` is True
                subEdges = subShape.Wires[0].Edges
            else:
                # Show all exterior and interior extensions if `usePerimeter` is False, or sub is an edge
                subEdges = subShape.Edges

            for edge in subEdges:
                for (e, label) in edges:
                    if edge.isSame(e):
                        ext1 = self._cachedExtension(self.obj, base, sub, label)
                        if ext1.isValid():
                            extensionEdges[e] = label[4:]  # isolate edge number
                            if not extendCorners:
                                createSubItem(label, ext1)
                                subHasItem = True

        if extendCorners and includeEdges:

            def edgesMatchShape(e0, e1):
                flipped = PathGeom.flipEdge(e1)
                if flipped:
                    return PathGeom.edgesMatch(e0, e1) or PathGeom.edgesMatch(
                        e0, flipped
                    )
                else:
                    return PathGeom.edgesMatch(e0, e1)

            self.extensionEdges = extensionEdges
            PathLog.debug("extensionEdges.values(): {}".format(extensionEdges.values()))
            for edgeList in Part.sortEdges(
                list(extensionEdges.keys())
            ):  # Identify connected edges that form wires
                self.edgeList = edgeList
                if len(edgeList) == 1:
                    label = (
                        "Edge%s"
                        % [
                            extensionEdges[keyEdge]
                            for keyEdge in extensionEdges.keys()
                            if edgesMatchShape(keyEdge, edgeList[0])
                        ][0]
                    )
                else:
                    label = "Wire(%s)" % ",".join(
                        sorted(
                            [
                                extensionEdges[keyEdge]
                                for e in edgeList
                                for keyEdge in extensionEdges.keys()
                                if edgesMatchShape(e, keyEdge)
                            ],
                            key=lambda s: int(s),
                        )
                    )  # pylint: disable=unnecessary-lambda
                ext2 = self._cachedExtension(self.obj, base, sub, label)
                createSubItem(label, ext2)
                subHasItem = True

        # Only add these subItems for horizontally oriented faces, not edges or vertical faces (from vertical face loops)
        if sub.startswith("Face") and PathGeom.isHorizontal(subShape):
            if includeSpecial:
                # Add entry to extend outline of face
                label = "Extend_" + sub
                ext3 = self._cachedExtension(self.obj, base, sub, label)
                createSubItem(label, ext3)

                # Add entry for waterline at face
                label = "Waterline_" + sub
                ext4 = self._cachedExtension(self.obj, base, sub, label)
                createSubItem(label, ext4)

            # Add entry for avoid face
            label = "Avoid_" + sub
            ext5 = self._cachedExtension(self.obj, base, sub, label)
            createSubItem(label, ext5)
            subHasItem = True

        if subHasItem:
            # PathLog.info("subHasItem")
            return item

        return None

    def createItemForBaseModel_Edges(self, base, sub, edges, extensions):
        PathLog.track(
            base.Label, sub, "+", len(edges), len(base.Shape.getElement(sub).Edges)
        )
        # PathLog.debug("createItemForBaseModel_Edges() label: {}, sub: {}, {}, edgeCnt: {}, subEdges: {}".format(base.Label, sub, '+', len(edges), len(base.Shape.getElement(sub).Edges)))

        extendCorners = self.form.extendCorners.isChecked()
        includeEdges = self.form.includeEdges.isChecked()
        includeSpecial = self.form.includeSpecial.isChecked()
        subShape = base.Shape.getElement(sub)
        subHasItem = False

        def createSubItem(label, ext0):
            if ext0.root:
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
                subHasItem = True

        # ext = self._cachedExtension(self.obj, base, sub, None)
        ext = None
        item = QtGui.QStandardItem()
        item.setData(sub, QtCore.Qt.EditRole)
        item.setData(ext, self.DataObject)
        item.setSelectable(False)

        extensionEdges = {}
        if includeEdges:
            if self.usePerimeter and sub.startswith("Face"):
                # Only show exterior extensions if `Use Outline` is True
                subEdges = subShape.Wires[0].Edges
            else:
                # Show all exterior and interior extensions if `usePerimeter` is False
                subEdges = subShape.Edges

            for edge in subEdges:
                for (e, label) in edges:
                    if edge.isSame(e):
                        ext1 = self._cachedExtension(self.obj, base, sub, label)
                        if ext1.isValid():
                            extensionEdges[e] = label[4:]  # isolate edge number
                            if not extendCorners:
                                createSubItem(label, ext1)

        if extendCorners and includeEdges:

            def edgesMatchShape(e0, e1):
                flipped = PathGeom.flipEdge(e1)
                if flipped:
                    return PathGeom.edgesMatch(e0, e1) or PathGeom.edgesMatch(
                        e0, flipped
                    )
                else:
                    return PathGeom.edgesMatch(e0, e1)

            self.extensionEdges = extensionEdges
            PathLog.debug("extensionEdges.values(): {}".format(extensionEdges.values()))
            for edgeList in Part.sortEdges(
                list(extensionEdges.keys())
            ):  # Identify connected edges that form wires
                self.edgeList = edgeList
                if len(edgeList) == 1:
                    label = (
                        "Edge%s"
                        % [
                            extensionEdges[keyEdge]
                            for keyEdge in extensionEdges.keys()
                            if edgesMatchShape(keyEdge, edgeList[0])
                        ][0]
                    )
                else:
                    label = "Wire(%s)" % ",".join(
                        sorted(
                            [
                                extensionEdges[keyEdge]
                                for e in edgeList
                                for keyEdge in extensionEdges.keys()
                                if edgesMatchShape(e, keyEdge)
                            ],
                            key=lambda s: int(s),
                        )
                    )  # pylint: disable=unnecessary-lambda
                ext2 = self._cachedExtension(self.obj, base, sub, label)
                createSubItem(label, ext2)

        if includeSpecial:
            pass

        if subHasItem:
            return item

        return None

    def setExtensions(self, extensions):
        PathLog.track(len(extensions))
        PathLog.debug("setExtensions()")

        if self.extensionsReady:
            PathLog.debug("setExtensions() returning per `extensionsReady` flag")
            return

        self.form.extensionTree.blockSignals(True)

        # remember current visual state
        if hasattr(self, "selectionModel"):
            selectedExtensions = [
                self.model.itemFromIndex(index).data(self.DataObject).ext
                for index in self.selectionModel.selectedIndexes()
            ]
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
                    collapsedFeatures.append(
                        "%s.%s" % (modelName, feature.data(QtCore.Qt.EditRole))
                    )

        # remove current extensions and all their visuals
        def removeItemSwitch(item, ext):
            # pylint: disable=unused-argument
            ext.hide()
            if ext.root:
                self.switch.removeChild(ext.root)

        self.forAllItemsCall(removeItemSwitch)
        self.model.clear()

        # create extensions for model and given argument
        if self.enabled:
            for base in self.obj.Base:
                show = True
                edges = [
                    (edge, "Edge%d" % (i + 1))
                    for i, edge in enumerate(base[0].Shape.Edges)
                ]
                baseItem = QtGui.QStandardItem()
                baseItem.setData(base[0].Label, QtCore.Qt.EditRole)
                baseItem.setSelectable(False)

                if self._allEdges(base[1]):
                    for sub in sorted(base[1]):
                        rowItem = self.createItemForBaseModel(
                            base[0], sub, edges, extensions
                        )
                        if rowItem:
                            baseItem.appendRow(rowItem)
                else:
                    for sub in sorted(base[1]):
                        rowItem = self.createItemForBaseModel(
                            base[0], sub, edges, extensions
                        )
                        if rowItem:
                            baseItem.appendRow(rowItem)
                if show:
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
                featureName = "%s.%s" % (modelName, feature.data(QtCore.Qt.EditRole))
                if featureName in collapsedFeatures:
                    self.form.extensionTree.setExpanded(feature.index(), False)
        if hasattr(self, "selectionModel") and selectedExtensions:
            self.restoreSelection(selectedExtensions)

        self.form.extensionTree.blockSignals(False)
        self.extensionsReady = True
        PathLog.debug("  setExtensions() finished and setting `extensionsReady=True`")

    def _allEdges(self, featureLabels):
        for label in featureLabels:
            if not label.startswith("Edge"):
                return False
        return True

    def updateData(self, obj, prop):
        PathLog.track(obj.Label, prop, self.blockUpdateData)
        # PathLog.debug("updateData({})".format(prop))

        if not self.blockUpdateData:
            if self.fieldsSet:
                if self.form.enableExtensions.isChecked():
                    if prop == "ExtensionLengthDefault":
                        self.updateQuantitySpinBoxes()
                    elif prop == "Base":
                        self.extensionsReady = False
                        self.setExtensions(PathFeatureExtensions.getExtensions(obj))
                    elif prop in ["ProcessPerimiter", "ProcessHoles", "ProcessCircles"]:
                        self._getUsePerimeterState()  # Find `useOutline` checkbox and get its boolean value
                        self._includeEdgesAndWires()
                elif prop == "Base":
                    self.extensionsReady = False

    def restoreSelection(self, selection):
        PathLog.debug("restoreSelection()")
        PathLog.track()
        if 0 == self.model.rowCount():
            PathLog.track("-")
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
                    self.selectionModel.select(
                        item.index(), QtCore.QItemSelectionModel.Select
                    )

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
        PathLog.debug("selectionChanged()")
        self.restoreSelection([])

    def extensionsClear(self):
        PathLog.debug("extensionsClear()")

        def disableItem(item, ext):
            item.setCheckState(QtCore.Qt.Unchecked)
            ext.disable()

        self.forAllItemsCall(disableItem)
        self.setDirty()

    def _extensionsSetState(self, state):
        PathLog.debug("_extensionsSetState()")
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
        # self.setDirty()

    def toggleExtensionCorners(self):
        PathLog.debug("toggleExtensionCorners()")
        PathLog.track()
        self.extensionsReady = False
        extensions = PathFeatureExtensions.getExtensions(self.obj)
        self.setExtensions(extensions)
        self.selectionChanged()
        self.setDirty()

    def getSignalsForUpdate(self, obj):
        PathLog.track(obj.Label)
        signals = []
        signals.append(self.form.defaultLength.editingFinished)
        signals.append(self.form.enableExtensions.toggled)
        signals.append(self.form.includeEdges.toggled)
        return signals

    def registerSignalHandlers(self, obj):
        self.form.showExtensions.clicked.connect(self.showHideExtension)
        self.form.extendCorners.clicked.connect(self.toggleExtensionCorners)
        self.form.buttonClear.clicked.connect(self.extensionsClear)
        self.form.buttonDisable.clicked.connect(self.extensionsDisable)
        self.form.buttonEnable.clicked.connect(self.extensionsEnable)
        self.form.defaultLength.editingFinished.connect(self.updateQuantitySpinBoxes)
        self.form.enableExtensions.toggled.connect(self._enableExtensions)
        self.form.includeEdges.toggled.connect(self._includeEdgesAndWires)
        self.form.includeSpecial.toggled.connect(self._includeSpecialExtensions)

        self.model.itemChanged.connect(self.updateItemEnabled)

        self.selectionModel = (
            self.form.extensionTree.selectionModel()
        )  # pylint: disable=attribute-defined-outside-init
        self.selectionModel.selectionChanged.connect(self.selectionChanged)
        self.selectionChanged()

    # Support methods
    def _findOperationPageForm(self):
        """_findOperationPageForm() ...
        This method locates the `Operation` tab form page, and saves that page reference
        to self.operationPageForm.
        """
        PathLog.debug("_findOperationPageForm()")

        if self.operationPageForm:
            return

        for page in self.parent.featurePages:
            if page.title == "Operation":
                PathLog.debug("Found Operation page form ")
                self.operationPageForm = page.form
                return

    def _getUsePerimeterState(self):
        """_getUsePerimeterState() ...
        This method attempts to determine if only the perimeter of the selections are being processed,
        and sets a representative boolean value to self.usePerimeter.
        """
        PathLog.debug("_getUsePerimeterState()")

        opf = self.operationPageForm
        if not opf:
            return

        if (
            hasattr(opf, "processPerimeter")
            and hasattr(opf, "processHoles")
            and hasattr(opf, "processCircles")
        ):
            PathLog.debug("Found processPerimeter ")
            self.usePerimeter = (
                opf.processPerimeter.isChecked()
                and not opf.processHoles.isChecked()
                and not opf.processCircles.isChecked()
            )
            return

    # Methods for enable and disablement of Extensions feature
    def _autoEnableExtensions(self):
        """_autoEnableExtensions() ...
        This method is called to determine if the Extensions feature should be enabled,
        or auto disabled due to total edge count of selected faces.
        The auto enable/disable feature is designed to allow quicker access
        to operations that implement the Extensions feature when selected faces contain
        large numbers of edges, which require long computation times for preparation.

        The return value is a simple boolean to communicate whether or not Extensions
        are be enabled.
        """
        enabled = True

        if self.initialEdgeCount < 1 and enabled:
            self.initialEdgeCount = 0
            for base in self.obj.Base:
                for sub in sorted(base[1]):
                    self.initialEdgeCount += len(base[0].Shape.getElement(sub).Edges)
            if self.initialEdgeCount > self.edgeCountThreshold:
                # Block signals
                self.form.enableExtensions.blockSignals(True)
                self.form.enableExtensionsWarning.blockSignals(True)
                self.form.includeEdges.blockSignals(True)

                # Make changes to form
                msg = translate(
                    "PathPocketShape",
                    "Edge count greater than threshold of"
                    + " "
                    + str(self.edgeCountThreshold)
                    + ":  "
                    + str(self.initialEdgeCount),
                )
                self.form.enableExtensionsWarning.setText(msg)
                self.form.enableExtensions.setChecked(False)
                self.form.enableExtensionsWarning.show()
                msg = translate("PathFeatureExtensions", "Click to enable Extensions")
                self.form.enableExtensions.setText(msg)
                self.form.extensionEdit.setDisabled(True)
                self.form.includeEdges.setChecked(False)
                msg = translate("PathFeatureExtensions", "Click to include Edges/Wires")
                self.form.includeEdges.setText(msg)

                # Unblock signals
                self.form.enableExtensions.blockSignals(False)
                self.form.enableExtensionsWarning.blockSignals(False)
                self.form.includeEdges.blockSignals(False)

                enabled = False
        elif not self.form.enableExtensions.isChecked():
            enabled = False

        PathLog.debug("_autoEnableExtensions() is {}".format(enabled))
        self.enabled = enabled

    def _enableExtensions(self):
        """_enableExtensions() ...
        This method is called when the enableExtensions push button is toggled.
        This method manages the enabled or disabled state of the extensionsEdit
        Task Panel input group.
        """
        PathLog.debug("_enableExtensions()")

        if self.form.enableExtensions.isChecked():
            self.enabled = True
            self.extensionsReady = False
            msg = translate("PathFeatureExtensions", "Extensions enabled")
            self.form.enableExtensions.setText(msg)
            self.form.enableExtensionsWarning.hide()
            self.form.extensionEdit.setEnabled(True)
            self.extensions = PathFeatureExtensions.getExtensions(self.obj)
            self.setExtensions(self.extensions)
        else:
            msg = translate("PathFeatureExtensions", "Click to enable Extensions")
            self.form.enableExtensions.setText(msg)
            self.form.extensionEdit.setDisabled(True)
            self.enabled = False

    def _includeEdgesAndWires(self):
        """_includeEdgesAndWires() ...
        This method is called when the includeEdges checkbox is toggled.
        This method manages the message thereof.
        """
        PathLog.debug("_includeEdgesAndWires()")

        self._getUsePerimeterState()  # Find `useOutline` checkbox and get its boolean value
        if self.form.includeEdges.isChecked():
            msg = translate("PathFeatureExtensions", "Including Edges/Wires")
            self.form.includeEdges.setText(msg)
        else:
            msg = translate("PathFeatureExtensions", "Include Edges/Wires")
            self.form.includeEdges.setText(msg)
        self.extensionsReady = False
        self._enableExtensions()

    def _includeSpecialExtensions(self):
        """_includeSpecialExtensions() ...
        This method is called when the includeSpecial checkbox is toggled.
        This method manages the message thereof.
        """
        PathLog.debug("_includeSpecialExtensions()")

        if self.form.includeSpecial.isChecked():
            msg = translate("PathFeatureExtensions", "Including special options")
            self.form.includeSpecial.setText(msg)
        else:
            msg = translate("PathFeatureExtensions", "Include special options")
            self.form.includeSpecial.setText(msg)
        self.extensionsReady = False
        self._enableExtensions()

    # Methods for creating and managing cached extensions
    def _cachedExtension(self, obj, base, sub, label):
        """_cachedExtension(obj, base, sub, label)...
        This method creates a new _Extension object if none is found within
        the extensionCache dictionary."""

        if label:
            cacheLabel = base.Name + "_" + sub + "_" + label
        else:
            cacheLabel = base.Name + "_" + sub + "_None"

        if cacheLabel in self.extensionsCache.keys():
            # PathLog.debug("return _cachedExtension({})".format(cacheLabel))
            return self.extensionsCache[cacheLabel]
        else:
            # PathLog.debug("_cachedExtension({}) created".format(cacheLabel))
            _ext = _Extension(obj, base, sub, label)
            self.extensionsCache[cacheLabel] = _ext  # cache the extension
            return _ext

    def _resetCachedExtensions(self):
        PathLog.debug("_resetCachedExtensions()")
        reset = dict()
        # Keep waterline extensions as they will not change
        for k in self.extensionsCache.keys():
            if k.startswith("Waterline"):
                reset[k] = self.extensionsCache[k]
        self.extensionsCache = reset
        self.extensionsReady = False


# Helper class for TaskPanelExtensionPage() class to provide extension visualizations in viewport
class _Extension(object):
    ColourEnabled = (1.0, 0.5, 1.0)
    ColourDisabled = (1.0, 1.0, 0.5)
    TransparencySelected = 0.0
    TransparencyDeselected = 0.7

    def __init__(self, obj, base, face, edge):
        self.obj = obj
        self.base = base
        self.face = face
        self.edge = edge
        self.ext = None

        if edge:
            self.ext = PathFeatureExtensions.createExtension(obj, base, face, edge)

        self.switch = self.createExtensionSoSwitch(self.ext)
        self.root = self.switch

    def createExtensionSoSwitch(self, ext):
        if not ext:
            return None

        sep = coin.SoSeparator()
        pos = coin.SoTranslation()
        mat = coin.SoMaterial()
        crd = coin.SoCoordinate3()
        fce = coin.SoFaceSet()
        hnt = coin.SoShapeHints()
        numVert = list()  # track number of vertices in each polygon face

        try:
            wire = ext.getWire()
        except FreeCAD.Base.FreeCADError:
            wire = None

        if not wire:
            return None

        if isinstance(wire, (list, tuple)):
            p0 = [p for p in wire[0].discretize(Deflection=0.02)]
            p1 = [p for p in wire[1].discretize(Deflection=0.02)]
            p2 = list(reversed(p1))
            polygon = [(p.x, p.y, p.z) for p in (p0 + p2)]
        else:
            if ext.extFaces:
                # Create polygon for each extension face in compound extensions
                allPolys = list()
                extFaces = ext.getExtensionFaces(wire)
                for f in extFaces:
                    pCnt = 0
                    wCnt = 0
                    for w in f.Wires:
                        if wCnt == 0:
                            poly = [p for p in w.discretize(Deflection=0.01)]
                        else:
                            poly = [p for p in w.discretize(Deflection=0.01)][:-1]
                        pCnt += len(poly)
                        allPolys.extend(poly)
                    numVert.append(pCnt)
                polygon = [(p.x, p.y, p.z) for p in allPolys]
            else:
                # poly = [p for p in wire.discretize(Deflection=0.02)][:-1]
                poly = [p for p in wire.discretize(Deflection=0.02)]
                polygon = [(p.x, p.y, p.z) for p in poly]
        crd.point.setValues(polygon)

        mat.diffuseColor = self.ColourDisabled
        mat.transparency = self.TransparencyDeselected

        hnt.faceType = coin.SoShapeHints.UNKNOWN_FACE_TYPE
        hnt.vertexOrdering = coin.SoShapeHints.CLOCKWISE

        if numVert:
            # Transfer vertex counts for polygon faces
            fce.numVertices.setValues(tuple(numVert))

        sep.addChild(pos)
        sep.addChild(mat)
        sep.addChild(hnt)
        sep.addChild(crd)
        sep.addChild(fce)

        # Finalize SoSwitch
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
        if self.switch:
            self.switch.whichChild = coin.SO_SWITCH_ALL

    def hide(self):
        if self.switch:
            self.switch.whichChild = coin.SO_SWITCH_NONE

    def enable(self, ena=True):
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


FreeCAD.Console.PrintLog("Loading PathTaskPanelExtensionPage... done\n")
