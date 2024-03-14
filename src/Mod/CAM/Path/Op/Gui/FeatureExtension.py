# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2019 sliptonic <shopinthewoods@gmail.com>               *
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

from PySide import QtCore, QtGui
from pivy import coin
import FreeCAD
import FreeCADGui
import Path
import Path.Base.Gui.Util as PathGuiUtil
import Path.Op.FeatureExtension as FeatureExtensions
import Path.Op.Gui.Base as PathOpGui

# lazily loaded modules
from lazy_loader.lazy_loader import LazyLoader

Part = LazyLoader("Part", globals(), "Part")

__title__ = "CAM Feature Extensions UI"
__author__ = "sliptonic (Brad Collette)"
__url__ = "https://www.freecad.org"
__doc__ = "Extensions feature page controller."


translate = FreeCAD.Qt.translate

if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


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
            self.ext = FeatureExtensions.createExtension(obj, base, face, edge)

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


class TaskPanelExtensionPage(PathOpGui.TaskPanelPage):
    DataObject = QtCore.Qt.ItemDataRole.UserRole
    DataSwitch = QtCore.Qt.ItemDataRole.UserRole + 2

    Direction = {
        FeatureExtensions.Extension.DirectionNormal: translate("PathPocket", "Normal"),
        FeatureExtensions.Extension.DirectionX: translate("PathPocket", "X"),
        FeatureExtensions.Extension.DirectionY: translate("PathPocket", "Y"),
    }

    def initPage(self, obj):
        self.setTitle("Extensions")
        self.OpIcon = ":/icons/view-axonometric.svg"
        self.setIcon(self.OpIcon)
        self.initialEdgeCount = -1
        self.edgeCountThreshold = 30
        self.fieldsSet = False
        self.useOutlineCheckbox = None
        self.useOutline = -1
        self.extensionsCache = dict()
        self.extensionsReady = False
        self.enabled = True
        self.lastDefaultLength = ""

        self.extensions = list()

        self.defaultLength = PathGuiUtil.QuantitySpinBox(
            self.form.defaultLength, obj, "ExtensionLengthDefault"
        )

        self.form.extensionTree.setEditTriggers(QtGui.QAbstractItemView.NoEditTriggers)
        self.form.extensionTree.setSelectionBehavior(QtGui.QAbstractItemView.SelectRows)

        self.switch = coin.SoSwitch()
        self.obj.ViewObject.RootNode.addChild(self.switch)
        self.switch.whichChild = coin.SO_SWITCH_ALL

        self.model = QtGui.QStandardItemModel(self.form.extensionTree)
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

        self.blockUpdateData = False

    def cleanupPage(self, obj):
        try:
            self.obj.ViewObject.RootNode.removeChild(self.switch)
        except ReferenceError:
            Path.Log.debug("obj already destroyed - no cleanup required")

    def getForm(self):
        form = FreeCADGui.PySideUic.loadUi(":/panels/PageOpPocketExtEdit.ui")
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
        Path.Log.debug("currentExtensions()")
        extensions = []

        def extractExtension(item, ext):
            if ext and ext.edge and item.checkState() == QtCore.Qt.Checked:
                extensions.append(ext.ext)

        if self.form.enableExtensions.isChecked():
            self.forAllItemsCall(extractExtension)
        Path.Log.track("extensions", extensions)
        return extensions

    def updateProxyExtensions(self, obj):
        Path.Log.debug("updateProxyExtensions()")
        self.extensions = self.currentExtensions()
        FeatureExtensions.setExtensions(obj, self.extensions)

    def getFields(self, obj):
        Path.Log.track(obj.Label, self.model.rowCount(), self.model.columnCount())
        self.blockUpdateData = True

        if obj.ExtensionCorners != self.form.extendCorners.isChecked():
            obj.ExtensionCorners = self.form.extendCorners.isChecked()
        self.defaultLength.updateProperty()

        self.updateProxyExtensions(obj)
        self.blockUpdateData = False

    def setFields(self, obj):
        Path.Log.track(obj.Label)
        # Path.Log.debug("setFields()")

        if obj.ExtensionCorners != self.form.extendCorners.isChecked():
            self.form.extendCorners.toggle()

        self._autoEnableExtensions()  # Check edge count for auto-disable Extensions on initial Task Panel loading
        self._initializeExtensions(obj)  # Efficiently initialize Extensions
        self.defaultLength.updateSpinBox()
        self._getUseOutlineState()  # Find `useOutline` checkbox and get its boolean value
        self.lastDefaultLength = self.form.defaultLength.text()  # set last DL value
        self.fieldsSet = True  # flag to identify initial values set

    def _initializeExtensions(self, obj):
        """_initializeExtensions()...
        Subroutine called inside `setFields()` to initialize Extensions efficiently."""
        if self.enabled:
            self.extensions = FeatureExtensions.getExtensions(obj)
        elif len(obj.ExtensionFeature) > 0:
            self.extensions = FeatureExtensions.getExtensions(obj)
            self.form.enableExtensions.setChecked(True)
            self._includeEdgesAndWires()
        else:
            self.form.extensionEdit.setDisabled(True)
        self.setExtensions(self.extensions)

    def _applyDefaultLengthChange(self, index=None):
        """_applyDefaultLengthChange(index=None)...
        Helper method to update Default Length spinbox,
        and update extensions due to change in Default Length."""
        self.defaultLength.updateSpinBox()
        if self.form.defaultLength.text() != self.lastDefaultLength:
            self.lastDefaultLength = self.form.defaultLength.text()
            self._resetCachedExtensions()  # Reset extension cache because extension dimensions likely changed
            self._enableExtensions()  # Recalculate extensions

    def createItemForBaseModel(self, base, sub, edges, extensions):
        Path.Log.track(
            base.Label, sub, "+", len(edges), len(base.Shape.getElement(sub).Edges)
        )
        # Path.Log.debug("createItemForBaseModel() label: {}, sub: {}, {}, edgeCnt: {}, subEdges: {}".format(base.Label, sub, '+', len(edges), len(base.Shape.getElement(sub).Edges)))

        extendCorners = self.form.extendCorners.isChecked()
        subShape = base.Shape.getElement(sub)

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

        # ext = self._cachedExtension(self.obj, base, sub, None)
        ext = None
        item = QtGui.QStandardItem()
        item.setData(sub, QtCore.Qt.EditRole)
        item.setData(ext, self.DataObject)
        item.setSelectable(False)

        extensionEdges = {}
        if self.useOutline == 1 and sub.startswith("Face"):
            # Only show exterior extensions if `Use Outline` is True
            subEdges = subShape.Wires[0].Edges
        else:
            # Show all exterior and interior extensions if `Use Outline` is False
            subEdges = subShape.Edges

        for edge in subEdges:
            for (e, label) in edges:
                if edge.isSame(e):
                    ext1 = self._cachedExtension(self.obj, base, sub, label)
                    if ext1.isValid():
                        extensionEdges[e] = label[4:]  # isolate edge number
                        if not extendCorners:
                            createSubItem(label, ext1)

        if extendCorners:

            def edgesMatchShape(e0, e1):
                flipped = Path.Geom.flipEdge(e1)
                if flipped:
                    return Path.Geom.edgesMatch(e0, e1) or Path.Geom.edgesMatch(
                        e0, flipped
                    )
                else:
                    return Path.Geom.edgesMatch(e0, e1)

            self.extensionEdges = extensionEdges
            Path.Log.debug(
                "extensionEdges.values(): {}".format(extensionEdges.values())
            )
            for edgeList in Part.sortEdges(
                list(extensionEdges)
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
                    )
                ext2 = self._cachedExtension(self.obj, base, sub, label)
                createSubItem(label, ext2)

        return item

    def setExtensions(self, extensions):
        Path.Log.track(len(extensions))
        Path.Log.debug("setExtensions()")

        if self.extensionsReady:
            Path.Log.debug("setExtensions() returning per `extensionsReady` flag")
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
            ext.hide()
            if ext.root:
                self.switch.removeChild(ext.root)

        self.forAllItemsCall(removeItemSwitch)
        self.model.clear()

        # create extensions for model and given argument
        if self.enabled:
            for base in self.obj.Base:
                show = False
                edges = [
                    (edge, "Edge%d" % (i + 1))
                    for i, edge in enumerate(base[0].Shape.Edges)
                ]
                baseItem = QtGui.QStandardItem()
                baseItem.setData(base[0].Label, QtCore.Qt.EditRole)
                baseItem.setSelectable(False)
                for sub in sorted(base[1]):
                    if sub.startswith("Face"):
                        show = True
                        baseItem.appendRow(
                            self.createItemForBaseModel(base[0], sub, edges, extensions)
                        )
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
        Path.Log.debug("  setExtensions() finished and setting `extensionsReady=True`")

    def updateData(self, obj, prop):
        Path.Log.track(obj.Label, prop, self.blockUpdateData)
        # Path.Log.debug("updateData({})".format(prop))

        if not self.blockUpdateData:
            if self.fieldsSet:
                if self.form.enableExtensions.isChecked():
                    if prop == "ExtensionLengthDefault":
                        self._applyDefaultLengthChange()
                    elif prop == "Base":
                        self.extensionsReady = False
                        self.setExtensions(FeatureExtensions.getExtensions(obj))
                    elif prop == "UseOutline":
                        self._getUseOutlineState()  # Find `useOutline` checkbox and get its boolean value
                        self._includeEdgesAndWires()
                elif prop == "Base":
                    self.extensionsReady = False

    def restoreSelection(self, selection):
        Path.Log.debug("restoreSelection()")
        Path.Log.track()
        if 0 == self.model.rowCount():
            Path.Log.track("-")
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
        Path.Log.debug("selectionChanged()")
        self.restoreSelection([])

    def extensionsClear(self):
        Path.Log.debug("extensionsClear()")

        def disableItem(item, ext):
            item.setCheckState(QtCore.Qt.Unchecked)
            ext.disable()

        self.forAllItemsCall(disableItem)
        self.setDirty()

    def _extensionsSetState(self, state):
        Path.Log.debug("_extensionsSetState()")
        Path.Log.track(state)
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
        Path.Log.track(item)
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
                ext.show()

            self.forAllItemsCall(enableExtensionEdit)
        else:

            def disableExtensionEdit(item, ext):
                if not self.selectionModel.isSelected(item.index()):
                    ext.hide()

            self.forAllItemsCall(disableExtensionEdit)
        # self.setDirty()

    def toggleExtensionCorners(self):
        Path.Log.debug("toggleExtensionCorners()")
        Path.Log.track()
        self.extensionsReady = False
        extensions = FeatureExtensions.getExtensions(self.obj)
        self.setExtensions(extensions)
        self.selectionChanged()
        self.setDirty()

    def getSignalsForUpdate(self, obj):
        Path.Log.track(obj.Label)
        signals = []
        signals.append(self.form.defaultLength.editingFinished)
        signals.append(self.form.enableExtensions.toggled)
        return signals

    def registerSignalHandlers(self, obj):
        self.form.showExtensions.clicked.connect(self.showHideExtension)
        self.form.extendCorners.clicked.connect(self.toggleExtensionCorners)
        self.form.buttonClear.clicked.connect(self.extensionsClear)
        self.form.buttonDisable.clicked.connect(self.extensionsDisable)
        self.form.buttonEnable.clicked.connect(self.extensionsEnable)
        self.form.enableExtensions.toggled.connect(self._enableExtensions)
        self.form.defaultLength.editingFinished.connect(self._applyDefaultLengthChange)

        self.model.itemChanged.connect(self.updateItemEnabled)

        self.selectionModel = self.form.extensionTree.selectionModel()
        self.selectionModel.selectionChanged.connect(self.selectionChanged)
        self.selectionChanged()

    # Support methods
    def _getUseOutlineState(self):
        """_getUseOutlineState() ...
        This method locates the `useOutline` form checkbox in the `Operation` tab,
        and saves that reference to self.useOutlineInput.  If found, then the boolean
        value of the checkbox is saved to self.useOutline.
        """
        if self.useOutlineCheckbox:
            self.useOutline = self.useOutlineCheckbox.isChecked()

        if hasattr(self, "parent"):
            parent = getattr(self, "parent")
            if parent and hasattr(parent, "featurePages"):
                for page in parent.featurePages:
                    if hasattr(page, "panelTitle"):
                        if page.panelTitle == "Operation" and hasattr(
                            page.form, "useOutline"
                        ):
                            Path.Log.debug("Found useOutline checkbox")
                            self.useOutlineCheckbox = page.form.useOutline
                            if page.form.useOutline.isChecked():
                                self.useOutline = 1
                                return
                            else:
                                self.useOutline = 0
                                return

        self.useOutline = -1

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
        enabled = False

        if self.form.enableExtensions.isChecked():
            enabled = True

        Path.Log.debug("_autoEnableExtensions() is {}".format(enabled))
        self.enabled = enabled

    def _enableExtensions(self):
        """_enableExtensions() ...
        This method is called when the enableExtensions push button is toggled.
        This method manages the enabled or disabled state of the extensionsEdit
        Task Panel input group.
        """
        Path.Log.debug("_enableExtensions()")

        if self.form.enableExtensions.isChecked():
            self.enabled = True
            self.extensionsReady = False
            self.form.extensionEdit.setEnabled(True)
            self.extensions = FeatureExtensions.getExtensions(self.obj)
            self.setExtensions(self.extensions)
        else:
            self.form.extensionEdit.setDisabled(True)
            self.enabled = False

    def _includeEdgesAndWires(self):
        """_includeEdgesAndWires() ...
        This method is called when the includeEdges push button is toggled.
        This method manages the state of the button and the message thereof.
        """
        self._getUseOutlineState()  # Find `useOutline` checkbox and get its boolean value
        Path.Log.debug("_includeEdgesAndWires()")
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

        if cacheLabel in self.extensionsCache:
            # Path.Log.debug("return _cachedExtension({})".format(cacheLabel))
            return self.extensionsCache[cacheLabel]
        else:
            # Path.Log.debug("_cachedExtension({}) created".format(cacheLabel))
            _ext = _Extension(obj, base, sub, label)
            self.extensionsCache[cacheLabel] = _ext  # cache the extension
            return _ext

    def _resetCachedExtensions(self):
        Path.Log.debug("_resetCachedExtensions()")
        reset = dict()
        self.extensionsCache = reset
        self.extensionsReady = False


# Eclass

FreeCAD.Console.PrintLog("Loading PathFeatureExtensionsGui... done\n")
