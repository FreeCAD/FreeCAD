# SPDX-License-Identifier: LGPL-2.1-or-later

"""Tests for FreeCADGui.LayerWidget, the generic layer list driven by a Python adapter.

To run tests:
    FreeCAD -t TestLayerWidget
"""

import unittest

import FreeCAD
import FreeCADGui
from PySide import QtCore, QtGui

DEFAULTS = "User parameter:BaseApp/Preferences/Test/LayerWidgetDefaults"
ID_ROLE = QtCore.Qt.UserRole
ADD_ROW_ROLE = QtCore.Qt.UserRole + 2


class MemoryLayers:
    """A minimal adapter: layers live in a list, ids are ints."""

    def __init__(self):
        self.items = [
            {"id": 0, "name": "Default", "visible": True, "active": True, "removable": False},
            {"id": 7, "name": "Walls", "visible": True, "color": (1.0, 0.0, 0.0)},
        ]
        self.calls = []

    def find(self, layer_id):
        return next(item for item in self.items if item["id"] == layer_id)

    def layers(self):
        return [dict(item) for item in self.items]

    def setLayerVisible(self, layer_id, visible):
        self.calls.append(("setLayerVisible", layer_id, visible))
        self.find(layer_id)["visible"] = visible


class FullLayers(MemoryLayers):
    """Implements every optional method."""

    def __init__(self):
        super().__init__()
        self.settings = {}

    def addLayer(self, name):
        new_id = max(item["id"] for item in self.items) + 1
        self.items.append({"id": new_id, "name": name, "visible": True})
        self.setActiveLayer(new_id)

    def removeLayer(self, layer_id):
        self.items.remove(self.find(layer_id))

    def renameLayer(self, layer_id, name):
        self.find(layer_id)["name"] = name

    def reorderLayers(self, ids):
        self.items.sort(key=lambda item: ids.index(item["id"]))

    def setActiveLayer(self, layer_id):
        for item in self.items:
            item["active"] = item["id"] == layer_id

    def setLayerLocked(self, layer_id, locked):
        self.find(layer_id)["locked"] = locked

    def setLayerColor(self, layer_id, color):
        self.find(layer_id)["color"] = color

    def setLayerPattern(self, layer_id, pattern):
        self.find(layer_id)["pattern"] = pattern

    def setLayerLineWidth(self, layer_id, width):
        self.find(layer_id)["lineWidth"] = width

    def extraSettings(self):
        return [
            {"key": "Print", "label": "Print", "default": True},
            {"key": "Plot", "label": "Plot style", "type": "choice",
             "choices": ["Normal", "Screened"], "dependsOn": "Print"},
        ]

    def setting(self, layer_id, key):
        return self.settings.get((layer_id, key), key == "Print")

    def setSetting(self, layer_id, key, value):
        self.calls.append(("setSetting", layer_id, key, value))
        self.settings[(layer_id, key)] = value

    def contextMenu(self, layer_id):
        return [("Select Contents", lambda: self.calls.append(("select", layer_id))), None]

    def defaultsGroup(self):
        return DEFAULTS


class DocumentLayers(FullLayers):
    """Stores layer names in a document, so changes are undoable."""

    def __init__(self, doc):
        super().__init__()
        self.doc = doc
        self.holder = doc.addObject("App::FeaturePython", "LayerNames")
        self.holder.addProperty("App::PropertyMap", "Names")
        self.holder.Names = {"0": "Default", "7": "Walls"}

    def document(self):
        return self.doc

    def layers(self):
        items = super().layers()
        for item in items:
            item["name"] = self.holder.Names[str(item["id"])]
        return items

    def renameLayer(self, layer_id, name):
        names = dict(self.holder.Names)
        names[str(layer_id)] = name
        self.holder.Names = names


class TestLayerWidget(unittest.TestCase):
    def setUp(self):
        FreeCAD.ParamGet(DEFAULTS).Clear()
        self.panels = []

    def tearDown(self):
        for panel in self.panels:
            form = panel.form
            if form is not None:
                form.deleteLater()
        self.flush()
        FreeCAD.ParamGet(DEFAULTS).Clear()

    def flush(self):
        for _ in range(3):
            QtGui.QApplication.processEvents()
            # Rows rebuilt by a refresh leave their old editors to deleteLater.
            QtGui.QApplication.sendPostedEvents(None, QtCore.QEvent.DeferredDelete)

    def create(self, adapter):
        panel = FreeCADGui.LayerWidget(adapter)
        self.panels.append(panel)
        tree = panel.form.findChild(QtGui.QTreeWidget, "layers")
        self.assertIsNotNone(tree)
        return panel, tree

    def rows(self, tree):
        return [tree.topLevelItem(i) for i in range(tree.topLevelItemCount())]

    def row(self, tree, layer_id):
        return next(item for item in self.rows(tree) if item.data(1, ID_ROLE) == str(layer_id))

    def child(self, tree, layer_id, text):
        model = tree.model()
        parent = tree.indexFromItem(self.row(tree, layer_id), 0)
        for row in range(model.rowCount(parent)):
            if model.index(row, 1, parent).data() == text:
                return model.index(row, 2, parent)
        self.fail("Missing setting " + text)

    def testMinimalAdapterHidesOptionalFeatures(self):
        adapter = MemoryLayers()
        panel, tree = self.create(adapter)
        self.assertIsInstance(panel.form, QtGui.QWidget)
        self.assertEqual([item.text(1) for item in self.rows(tree)], ["Default", "Walls"])
        self.assertTrue(self.row(tree, 0).font(1).bold())
        # No addLayer: no "Add Layer" row. No setLayerLocked: no lock column.
        self.assertFalse(any(item.data(1, ADD_ROW_ROLE) for item in self.rows(tree)))
        self.assertTrue(tree.isColumnHidden(3))
        # No style setters: no color square and no settings to expand.
        self.assertTrue(self.row(tree, 7).icon(1).isNull())
        self.assertEqual(self.row(tree, 7).childCount(), 0)
        self.assertFalse(self.row(tree, 7).flags() & QtCore.Qt.ItemIsEditable)

    def testVisibilityClickPassesOriginalIds(self):
        adapter = MemoryLayers()
        panel, tree = self.create(adapter)
        tree.itemClicked.emit(self.row(tree, 7), 0)
        self.flush()
        self.assertEqual(adapter.calls, [("setLayerVisible", 7, False)])
        self.assertEqual(self.row(tree, 7).data(0, QtCore.Qt.AccessibleTextRole), "Hidden")

    def testRefreshReloadsFromAdapter(self):
        adapter = MemoryLayers()
        panel, tree = self.create(adapter)
        adapter.items.append({"id": 9, "name": "Doors", "visible": False})
        panel.refresh()
        self.assertEqual(self.row(tree, 9).text(1), "Doors")

    def testFullAdapterFeatures(self):
        adapter = FullLayers()
        panel, tree = self.create(adapter)
        self.assertFalse(tree.isColumnHidden(3))
        self.assertTrue(self.rows(tree)[-1].data(1, ADD_ROW_ROLE))
        # Built-in style rows, then the adapter's settings.
        self.assertEqual(self.row(tree, 7).childCount(), 5)
        self.assertFalse(self.row(tree, 7).icon(1).isNull())

        tree.itemClicked.emit(self.row(tree, 7), 1)
        self.flush()
        self.assertTrue(adapter.find(7)["active"])
        self.assertTrue(self.row(tree, 7).font(1).bold())

        tree.itemClicked.emit(self.row(tree, 7), 3)
        self.flush()
        self.assertTrue(adapter.find(7)["locked"])

        pattern = panel.form.findChild(QtGui.QComboBox, "layerPattern7")
        pattern.setCurrentIndex(1)
        pattern.activated.emit(1)
        self.flush()
        self.assertEqual(adapter.find(7)["pattern"], 0xF0F0)

        width = panel.form.findChild(QtGui.QDoubleSpinBox, "layerLineWidth7")
        width.setValue(3.5)
        self.flush()
        self.assertEqual(adapter.find(7)["lineWidth"], 3.5)

    def testExtraSettings(self):
        adapter = FullLayers()
        panel, tree = self.create(adapter)
        printing = self.child(tree, 7, "Print")
        self.assertEqual(QtCore.Qt.CheckState(printing.data(QtCore.Qt.CheckStateRole)),
                         QtCore.Qt.Checked)
        plot = panel.form.findChild(QtGui.QComboBox, "layerSettingPlot7")
        self.assertTrue(plot.isEnabled())
        tree.model().setData(printing, QtCore.Qt.Unchecked, QtCore.Qt.CheckStateRole)
        self.flush()
        self.assertIn(("setSetting", 7, "Print", False), adapter.calls)
        # Plot depends on Print.
        plot = panel.form.findChild(QtGui.QComboBox, "layerSettingPlot7")
        self.assertFalse(plot.isEnabled())

    def testDefaultsGoToTheAdapterGroup(self):
        adapter = FullLayers()
        panel, tree = self.create(adapter)
        pattern = panel.form.findChild(QtGui.QComboBox, "layerPatternDefaults")
        pattern.setCurrentIndex(2)
        pattern.activated.emit(2)
        width = panel.form.findChild(QtGui.QDoubleSpinBox, "layerLineWidthDefaults")
        width.setValue(4.5)
        add = self.rows(tree)[-1]
        model = tree.model()
        parent = tree.indexFromItem(add, 0)
        printing = next(model.index(r, 2, parent) for r in range(model.rowCount(parent))
                        if model.index(r, 1, parent).data() == "Print")
        model.setData(printing, QtCore.Qt.Unchecked, QtCore.Qt.CheckStateRole)
        self.flush()
        group = FreeCAD.ParamGet(DEFAULTS)
        self.assertEqual(group.GetInt("Pattern"), 0xAAAA)
        self.assertEqual(group.GetFloat("LineWidth"), 4.5)
        self.assertFalse(group.GetBool("Print", True))

    def testContextMenuEntries(self):
        adapter = FullLayers()
        panel, tree = self.create(adapter)
        errors = []

        def choose():
            menu = QtGui.QApplication.activePopupWidget()
            try:
                texts = [action.text() for action in menu.actions() if not action.isSeparator()]
                self.assertEqual(texts[0], "Select Contents")
                self.assertIn("Hide Other Layers", texts)
                self.assertIn("Remove Layer", texts)
                next(a for a in menu.actions() if a.text() == "Select Contents").trigger()
            except Exception as exc:
                errors.append(str(exc))
            finally:
                if menu:
                    menu.close()

        panel.form.show()
        point = tree.visualItemRect(self.row(tree, 7)).center()
        QtCore.QTimer.singleShot(100, choose)
        tree.customContextMenuRequested.emit(point)
        self.flush()
        self.assertFalse(errors, errors)
        self.assertIn(("select", 7), adapter.calls)

    def testDocumentAdapterUsesTransactions(self):
        doc = FreeCAD.newDocument("LayerWidgetTest")
        doc.UndoMode = 1
        try:
            adapter = DocumentLayers(doc)
            doc.recompute()
            panel, tree = self.create(adapter)
            item = self.row(tree, 7)
            item.setText(1, "Façades")
            self.flush()
            self.assertEqual(adapter.holder.Names["7"], "Façades")
            self.assertIn("Rename layer", doc.UndoNames)
            doc.undo()
            self.flush()
            # The widget follows document changes without an explicit refresh.
            self.assertEqual(self.row(tree, 7).text(1), "Walls")
        finally:
            for panel in self.panels:
                if panel.form is not None:
                    panel.form.deleteLater()
            self.panels = []
            self.flush()
            FreeCAD.closeDocument(doc.Name)

    def testAdapterErrorsAreReportedNotRaised(self):
        class Broken(MemoryLayers):
            def setLayerVisible(self, layer_id, visible):
                raise ValueError("refused")

        panel, tree = self.create(Broken())
        errors = []

        def dismiss():
            box = QtGui.QApplication.activeModalWidget()
            try:
                self.assertIsInstance(box, QtGui.QMessageBox)
                self.assertIn("refused", box.text())
            except Exception as exc:
                errors.append(str(exc))
            finally:
                if box:
                    box.close()

        QtCore.QTimer.singleShot(100, dismiss)
        tree.itemClicked.emit(self.row(tree, 7), 0)
        self.flush()
        self.assertFalse(errors, errors)
        self.assertEqual(self.row(tree, 7).data(0, QtCore.Qt.AccessibleTextRole), "Visible")
