# SPDX-License-Identifier: LGPL-2.1-or-later
import FreeCAD as App
import Part
import Sketcher
from PySide import QtCore, QtGui
from SketcherTests.TestSketchLayers import isolatedLayerDefaults
from SketcherTests.GuiTestCase import FreeCADGui as Gui, SketcherGuiTestCase


class TestSketchLayersGui(SketcherGuiTestCase):
    def setUp(self):
        super().setUp()
        self.defaults = isolatedLayerDefaults(self)
        self.layerPrefs = App.ParamGet("User parameter:BaseApp/Preferences/Mod/Sketcher")
        self.previousShowLayers = self.layerPrefs.GetBool("ShowLayers", False)
        self.layerPrefs.SetBool("ShowLayers", True)
        Gui.activateWorkbench("SketcherWorkbench")
        self.doc = App.newDocument("SketchLayersGui")
        self.doc.UndoMode = 1
        self.sketch = self.doc.addObject("Sketcher::SketchObject", "Sketch")
        self.sketch.addGeometry(Part.LineSegment(App.Vector(0, 0, 0), App.Vector(20, 0, 0)))
        self.doc.recompute()
        Gui.activeDocument().setEdit(self.sketch.Name)
        self.flush_gui(100)
        self.tree = Gui.getMainWindow().findChild(QtGui.QTreeWidget, "sketchLayers")
        self.assertIsNotNone(self.tree)

    def tearDown(self):
        try:
            super().tearDown()
        finally:
            self.layerPrefs.SetBool("ShowLayers", self.previousShowLayers)

    @staticmethod
    def item_id(item):
        # The generic layer widget keeps string ids; -1 stands for its "Add Layer" row.
        if item.data(1, QtCore.Qt.UserRole + 2):
            return -1
        return int(item.data(1, QtCore.Qt.UserRole))

    def row(self, layer):
        return next(
            self.tree.topLevelItem(i)
            for i in range(self.tree.topLevelItemCount())
            if self.item_id(self.tree.topLevelItem(i)) == layer
        )

    def remove_layer(
        self, layer, destination=0, delete=False, cancel=False, context=False, populated=True
    ):
        errors = []
        decisions = []

        def choose_geometry_action():
            dialog = QtGui.QApplication.activeModalWidget()
            try:
                self.assertIsNotNone(dialog)
                self.assertEqual(dialog.objectName(), "removeSketchLayerDialog")
                decisions.append(True)
                target = dialog.findChild(QtGui.QComboBox, "sketchLayerDestination")
                self.assertEqual(target.findData(layer), -1)
                if cancel:
                    dialog.reject()
                else:
                    if delete:
                        dialog.findChild(QtGui.QRadioButton, "deleteSketchLayerGeometry").click()
                        self.assertFalse(target.isEnabled())
                    else:
                        index = target.findData(destination)
                        self.assertGreaterEqual(index, 0)
                        target.setCurrentIndex(index)
                    dialog.findChild(QtGui.QDialogButtonBox).button(
                        QtGui.QDialogButtonBox.Ok
                    ).click()
            except Exception:
                import traceback

                errors.append(traceback.format_exc())
                if dialog:
                    dialog.reject()

        def choose_remove():
            menu = QtGui.QApplication.activePopupWidget()
            try:
                self.assertIsNotNone(menu)
                action = next(a for a in menu.actions() if a.text() == "Remove Layer")
                self.assertTrue(action.isEnabled())
                if populated:
                    QtCore.QTimer.singleShot(100, choose_geometry_action)
                action.trigger()
            except Exception:
                import traceback

                errors.append(traceback.format_exc())
            finally:
                if menu:
                    menu.close()

        if context:
            # Right-clicking must operate on this row, even when another row is current.
            self.tree.scrollToItem(self.row(layer))
            point = self.tree.visualItemRect(self.row(layer)).center()
            QtCore.QTimer.singleShot(100, choose_remove)
            event = QtGui.QContextMenuEvent(
                QtGui.QContextMenuEvent.Mouse, point, self.tree.viewport().mapToGlobal(point)
            )
            QtGui.QApplication.sendEvent(self.tree.viewport(), event)
        else:
            self.tree.setCurrentItem(self.row(layer), 1)
            self.tree.setFocus()
            self.flush_gui(50)
            self.assertTrue(self.tree.hasFocus())
            if populated:
                QtCore.QTimer.singleShot(100, choose_geometry_action)
            self.key_click(self.tree, QtCore.Qt.Key_Delete)
        self.flush_gui(150)
        self.assertFalse(errors, errors)
        self.assertEqual(bool(decisions), populated)

    def layer_action(self, layer, text):
        errors = []

        def choose():
            menu = QtGui.QApplication.activePopupWidget()
            try:
                action = next(a for a in menu.actions() if a.text() == text)
                action.trigger()
            except Exception:
                import traceback

                errors.append(traceback.format_exc())
            finally:
                if menu:
                    menu.close()

        self.tree.scrollToItem(self.row(layer))
        point = self.tree.visualItemRect(self.row(layer)).center()
        QtCore.QTimer.singleShot(100, choose)
        event = QtGui.QContextMenuEvent(
            QtGui.QContextMenuEvent.Mouse, point, self.tree.viewport().mapToGlobal(point)
        )
        QtGui.QApplication.sendEvent(self.tree.viewport(), event)
        self.flush_gui(150)
        self.assertFalse(errors, errors)

    def testLayerSelectionActions(self):
        s = self.sketch
        s.addGeometry(Part.LineSegment(App.Vector(30, 0, 0), App.Vector(40, 0, 0)))
        point = s.addGeometry(Part.Point(App.Vector(40, 10, 0)))
        s.addConstraint(Sketcher.Constraint("Horizontal", 0))
        s.addConstraint(Sketcher.Constraint("Distance", 1, 10.0))
        s.addConstraint(Sketcher.Constraint("Coincident", 0, 2, 1, 1))
        layer = s.addLayer("Selected layer")
        s.setGeometryLayer([0, point], layer)
        source = self.doc.addObject("Part::Feature", "Source")
        source.Shape = Part.makeLine(App.Vector(0, 20, 0), App.Vector(20, 20, 0))
        self.doc.recompute()
        s.addExternal(source.Name, "Edge1")
        s.setGeometryLayer([-3], layer)
        self.doc.recompute()
        # Explicit layer selection includes its hidden geometry and constraints.
        s.ViewObject.HiddenLayers = [layer]
        self.flush_gui(100)

        def selected():
            return {name for obj in Gui.Selection.getSelectionEx() for name in obj.SubElementNames}

        self.layer_action(layer, "Select Layer Geometry")
        geometry = selected()
        self.assertIn("Edge1", geometry)
        self.assertIn("ExternalEdge1", geometry)
        self.assertTrue(any(name.startswith("Vertex") for name in geometry))
        self.assertEqual(len(geometry), 3)
        self.layer_action(layer, "Select Layer Constraints")
        self.assertEqual(selected(), {"Constraint1", "Constraint3"})
        self.layer_action(layer, "Select Layer Contents")
        self.assertEqual(selected(), geometry | {"Constraint1", "Constraint3"})
        self.assertEqual(s.ActiveLayer, 0)

    def testHideOtherLayersShowsTargetAndUndoes(self):
        s = self.sketch
        layer = s.addLayer("Target")
        other = s.addLayer("Other")
        s.ViewObject.HiddenLayers = [layer]
        self.flush_gui(100)
        self.layer_action(layer, "Hide Other Layers")
        self.assertEqual(set(s.ViewObject.HiddenLayers), {0, other})
        self.assertEqual(s.ActiveLayer, 0)
        self.doc.undo()
        self.flush_gui(100)
        self.assertEqual(s.ViewObject.HiddenLayers, [layer])
        self.doc.redo()
        self.flush_gui(100)
        self.assertEqual(set(s.ViewObject.HiddenLayers), {0, other})

    def testInlineCreationCancelValidationAndUndo(self):
        s = self.sketch
        s.addLayer("Layer1")
        self.flush_gui(100)

        def begin():
            self.tree.scrollToItem(self.row(-1))
            rect = self.tree.visualItemRect(self.row(-1))
            # Clicking the plus in the eye column must also start editing.
            self.click(
                self.tree.viewport(),
                QtCore.QPoint(self.tree.columnWidth(0) // 2, rect.center().y()),
            )
            editor = self.tree.findChild(QtGui.QLineEdit, "newLayerName")
            self.assertIsNotNone(editor)
            self.assertEqual(editor.selectedText(), "Layer2")
            return editor

        editor = begin()
        self.key_click(editor, QtCore.Qt.Key_Escape)
        self.flush_gui(100)
        self.assertEqual(len(s.Layers), 2)
        self.assertIsNotNone(Gui.activeDocument().getInEdit())
        self.assertEqual(self.row(-1).text(1), "Add Layer")
        editor = begin()
        for name in ("", "Layer1"):
            editor.setText(name)
            self.key_click(editor, QtCore.Qt.Key_Return)
            self.assertEqual(len(s.Layers), 2)
            self.assertTrue(editor.isVisible())
        editor.selectAll()
        self.key_click(editor, QtCore.Qt.Key_P, "P")
        self.assertEqual(editor.text(), "P")
        editor.setText("Profilé 'α'")
        self.key_click(editor, QtCore.Qt.Key_Return)
        self.flush_gui(100)
        layer = s.ActiveLayer
        self.assertEqual(s.Layers[str(layer)], "Profilé 'α'")
        self.doc.undo()
        self.flush_gui(100)
        self.assertNotIn(str(layer), s.Layers)
        self.assertEqual(s.ActiveLayer, 0)
        self.doc.redo()
        self.flush_gui(100)
        self.assertEqual(s.Layers[str(layer)], "Profilé 'α'")
        self.assertEqual(s.ActiveLayer, layer)

    def layer_order(self):
        return [
            self.item_id(self.tree.topLevelItem(i)) for i in range(self.tree.topLevelItemCount())
        ]

    def drop_layer(self, source, target, below=False):
        item = self.row(source)
        index = self.tree.indexFromItem(item, 1)
        mime = self.tree.model().mimeData([index])
        target_item = self.row(target)
        self.tree.scrollToItem(target_item)
        rect = self.tree.visualItemRect(target_item)
        point = QtCore.QPoint(rect.center().x(), rect.bottom() if below else rect.top())
        enter = QtGui.QDragEnterEvent(
            point, QtCore.Qt.MoveAction, mime, QtCore.Qt.LeftButton, QtCore.Qt.NoModifier
        )
        QtGui.QApplication.sendEvent(self.tree.viewport(), enter)
        if not enter.isAccepted():
            return False
        move = QtGui.QDragMoveEvent(
            point, QtCore.Qt.MoveAction, mime, QtCore.Qt.LeftButton, QtCore.Qt.NoModifier
        )
        QtGui.QApplication.sendEvent(self.tree.viewport(), move)
        drop = QtGui.QDropEvent(
            QtCore.QPointF(point),
            QtCore.Qt.MoveAction,
            mime,
            QtCore.Qt.LeftButton,
            QtCore.Qt.NoModifier,
        )
        QtGui.QApplication.sendEvent(self.tree.viewport(), drop)
        self.flush_gui(100)
        return drop.isAccepted()

    def testLayerReorderUndoSaveAndPinnedAddRow(self):
        import os
        import tempfile

        s = self.sketch
        first = s.addLayer("First")
        second = s.addLayer("Second")
        self.flush_gui(100)
        self.assertFalse(self.row(-1).flags() & QtCore.Qt.ItemIsDragEnabled)
        self.assertTrue(self.drop_layer(second, 0))
        self.assertEqual(self.layer_order(), [second, 0, first, -1])
        self.assertEqual(s.ViewObject.LayerOrder, [second, 0, first])
        self.assertEqual(s.getGeometryLayer(0), 0)
        self.assertEqual(s.ActiveLayer, 0)
        self.doc.undo()
        self.flush_gui(100)
        self.assertEqual(self.layer_order(), [0, first, second, -1])
        self.doc.redo()
        self.flush_gui(100)
        self.assertEqual(self.layer_order(), [second, 0, first, -1])
        self.assertTrue(self.drop_layer(second, -1, below=True))
        self.assertEqual(self.layer_order(), [0, first, second, -1])
        self.assertFalse(self.drop_layer(-1, 0))
        self.assertEqual(self.layer_order(), [0, first, second, -1])
        self.assertTrue(self.drop_layer(second, 0))
        with tempfile.TemporaryDirectory() as folder:
            file = os.path.join(folder, "ordered.FCStd")
            Gui.activeDocument().resetEdit()
            self.doc.recompute()
            self.doc.saveAs(file)
            App.closeDocument(self.doc.Name)
            self.doc = App.openDocument(file)
            self.sketch = self.doc.getObject("Sketch")
            Gui.activeDocument().setEdit(self.sketch.Name)
            self.flush_gui(100)
            self.tree = Gui.getMainWindow().findChild(QtGui.QTreeWidget, "sketchLayers")
            self.assertEqual(self.layer_order(), [second, 0, first, -1])
            third = self.sketch.addLayer("Third")
            self.sketch.removeLayer(first)
            self.flush_gui(100)
            self.assertEqual(self.layer_order(), [second, 0, third, -1])
            Gui.activeDocument().resetEdit()
            App.closeDocument(self.doc.Name)
            self.doc = None

    def testShowLayersDefaultAndEditSettings(self):
        from pivy import coin

        s = self.sketch
        layer = s.addLayer("Hidden")
        s.setGeometryLayer([0], layer)
        s.ViewObject.HiddenLayers = [layer]
        self.flush_gui(100)
        box = Gui.getMainWindow().findChild(QtGui.QWidget, "sketchLayersTaskBox")
        constraints = Gui.getMainWindow().findChild(QtGui.QListWidget, "listWidgetConstraints")
        elements = Gui.getMainWindow().findChild(QtGui.QListWidget, "listWidgetElements")
        self.assertLess(
            box.mapToGlobal(QtCore.QPoint()).y(), constraints.mapToGlobal(QtCore.QPoint()).y()
        )
        self.assertLess(
            constraints.mapToGlobal(QtCore.QPoint()).y(), elements.mapToGlobal(QtCore.QPoint()).y()
        )

        def curves_shown():
            search = coin.SoSearchAction()
            search.setName("CurvesLineSet0")
            search.setSearchingAll(True)
            search.apply(Gui.activeDocument().activeView().getSceneGraph())
            path = search.getPath()
            group = path.getNode(path.getLength() - 3)
            return all(list(group.getField("enable").getValues())[15:20])

        self.assertFalse(curves_shown())
        self.layerPrefs.RemBool("ShowLayers")
        self.flush_gui(100)
        self.assertFalse(box.isVisible())
        self.assertTrue(curves_shown())
        self.assertEqual(s.ViewObject.HiddenLayers, [layer])
        Gui.Selection.addSelection(s, "Edge1")
        action = Gui.getMainWindow().findChild(QtGui.QAction, "showSketchLayers")
        self.assertIsNotNone(action)
        action.parent().aboutToShow.emit()
        self.assertFalse(action.isChecked())
        action.trigger()
        self.flush_gui(100)
        self.assertTrue(self.layerPrefs.GetBool("ShowLayers"))
        self.assertTrue(box.isVisible())
        self.assertFalse(curves_shown())
        self.assertFalse(Gui.Selection.getSelectionEx())
        action.trigger()
        self.flush_gui(100)
        self.assertFalse(box.isVisible())
        self.assertTrue(curves_shown())
        Gui.activeDocument().resetEdit()
        Gui.activeDocument().setEdit(s.Name)
        self.flush_gui(100)
        box = Gui.getMainWindow().findChild(QtGui.QWidget, "sketchLayersTaskBox")
        self.assertFalse(box.isVisible())
        self.assertTrue(curves_shown())
        self.assertEqual(s.ViewObject.HiddenLayers, [layer])

    def testShowLayersPreferencePage(self):
        self.layerPrefs.SetBool("ShowLayers", False)
        self.flush_gui(100)
        errors = []

        def change_preference():
            dialog = QtGui.QApplication.activeModalWidget()
            try:
                checkbox = dialog.findChild(QtGui.QCheckBox, "checkBoxShowLayers")
                self.assertIsNotNone(checkbox)
                self.assertFalse(checkbox.isChecked())
                checkbox.setChecked(True)
                buttons = next(
                    b
                    for b in dialog.findChildren(QtGui.QDialogButtonBox)
                    if b.button(QtGui.QDialogButtonBox.Apply)
                )
                buttons.button(QtGui.QDialogButtonBox.Apply).click()
            except Exception:
                import traceback

                errors.append(traceback.format_exc())
            finally:
                if dialog:
                    dialog.reject()

        QtCore.QTimer.singleShot(300, change_preference)
        Gui.showPreferences("Sketcher", 0)
        self.flush_gui(100)
        self.assertFalse(errors, errors)
        self.assertTrue(self.layerPrefs.GetBool("ShowLayers"))
        box = Gui.getMainWindow().findChild(QtGui.QWidget, "sketchLayersTaskBox")
        self.assertTrue(box.isVisible())

    def testAddRowKeyboardAndEmptyLayerContextRemoval(self):
        s = self.sketch
        self.tree.setCurrentItem(self.row(-1), 1)
        self.tree.setFocus()
        self.key_click(self.tree, QtCore.Qt.Key_Return)
        self.flush_gui(100)
        self.assertEqual(s.Layers, {"0": "Default"})
        editor = self.tree.findChild(QtGui.QLineEdit, "newLayerName")
        self.assertIsNotNone(editor)
        self.assertEqual(editor.selectedText(), "Layer1")
        self.key_click(editor, QtCore.Qt.Key_Return)
        self.flush_gui(100)
        layer = s.ActiveLayer
        self.assertNotEqual(layer, 0)
        s.setActiveLayer(0)
        self.flush_gui(100)
        self.remove_layer(layer, context=True, populated=False)
        self.assertNotIn(str(layer), s.Layers)
        self.assertEqual(s.GeometryCount, 1)
        self.assertEqual(s.ActiveLayer, 0)
        self.doc.undo()
        self.assertIn(str(layer), s.Layers)

    def testDeleteOnDefaultAndAddRowDoesNotDeleteGeometry(self):
        s = self.sketch
        Gui.Selection.addSelection(s, "Edge1")
        for layer in (0, -1):
            self.tree.setCurrentItem(self.row(layer), 1)
            self.tree.setFocus()
            self.key_click(self.tree, QtCore.Qt.Key_Delete)
            self.assertEqual(s.GeometryCount, 1)
            self.assertEqual(s.Layers, {"0": "Default"})

    def testRemoveLayerMoveCancelAndUndo(self):
        s = self.sketch
        layer = s.addLayer("Profile")
        target = s.addLayer("Destination")
        s.setGeometryLayer([0], layer)
        s.setActiveLayer(layer)
        self.flush_gui(100)
        self.remove_layer(layer, cancel=True)
        self.assertIn(str(layer), s.Layers)
        self.assertEqual(s.getGeometryLayer(0), layer)
        self.assertEqual(s.ActiveLayer, layer)
        self.remove_layer(layer, destination=target, context=True)
        self.assertNotIn(str(layer), s.Layers)
        self.assertEqual(s.getGeometryLayer(0), target)
        self.assertEqual(s.ActiveLayer, target)
        self.doc.undo()
        self.flush_gui(100)
        self.assertEqual(s.getGeometryLayer(0), layer)
        self.assertEqual(s.ActiveLayer, layer)
        self.doc.redo()
        self.flush_gui(100)
        self.assertEqual(s.getGeometryLayer(0), target)
        self.assertNotIn(str(layer), s.Layers)

    def testRemoveLayerDeletesGeometryAndReferencesWithUndo(self):
        s = self.sketch
        source = self.doc.addObject("Part::Feature", "Source")
        source.Shape = Part.makeLine(App.Vector(0, 10, 0), App.Vector(20, 10, 0))
        s.addGeometry(Part.LineSegment(App.Vector(30, 0, 0), App.Vector(40, 0, 0)))
        s.addConstraint(Sketcher.Constraint("Distance", 0, 20.0))
        layer = s.addLayer("Delete me")
        s.setGeometryLayer([0], layer)
        s.setActiveLayer(layer)
        self.doc.recompute()
        s.addExternal(source.Name, "Edge1")
        self.doc.recompute()
        self.flush_gui(100)
        self.assertEqual(s.getGeometryLayer(-3), layer)
        self.remove_layer(layer, delete=True)
        self.assertNotIn(str(layer), s.Layers)
        self.assertEqual(s.GeometryCount, 1)
        self.assertEqual(s.getGeometryLayer(0), 0)
        self.assertEqual(s.ConstraintCount, 0)
        self.assertEqual(len(s.ExternalGeometry), 0)
        self.assertIsNotNone(self.doc.getObject(source.Name))
        self.doc.undo()
        self.flush_gui(100)
        self.assertEqual(s.GeometryCount, 2)
        self.assertEqual(s.ConstraintCount, 1)
        self.assertEqual(s.getGeometryLayer(0), layer)
        self.assertEqual(s.getGeometryLayer(-3), layer)
        self.doc.redo()
        self.flush_gui(100)
        self.assertEqual(s.GeometryCount, 1)
        self.assertNotIn(str(layer), s.Layers)

    def testTaskBoxCreateActivateRenameVisibility(self):
        s = self.sketch
        self.assertIsNone(Gui.getMainWindow().findChild(QtGui.QPushButton, "addSketchLayer"))
        self.assertIsNone(Gui.getMainWindow().findChild(QtGui.QPushButton, "removeSketchLayer"))
        add = self.row(-1)
        self.assertEqual(add.text(1), "Add Layer")
        self.assertFalse(add.icon(0).isNull())
        self.assertFalse(add.flags() & QtCore.Qt.ItemIsEditable)
        self.tree.scrollToItem(add)
        point = self.tree.visualItemRect(add).center()
        self.click(self.tree.viewport(), point)
        self.flush_gui(100)
        self.assertEqual(s.Layers, {"0": "Default"})
        editor = self.tree.findChild(QtGui.QLineEdit, "newLayerName")
        self.assertIsNotNone(editor)
        self.assertEqual(editor.selectedText(), "Layer1")
        self.key_click(editor, QtCore.Qt.Key_Return)
        self.flush_gui(100)
        layer = s.ActiveLayer
        self.assertNotEqual(layer, 0)
        self.assertEqual(self.tree.topLevelItemCount(), 3)
        self.assertEqual(self.item_id(self.tree.currentItem()), layer)
        self.assertTrue(self.tree.currentItem().font(1).bold())
        # F2 edits the name column, including names requiring Python escaping.
        self.tree.setFocus()
        self.key_click(self.tree, QtCore.Qt.Key_F2)
        editor = self.tree.findChild(QtGui.QLineEdit)
        self.assertIsNotNone(editor)
        editor.setText("Profilé 'α'")
        self.key_click(editor, QtCore.Qt.Key_Return)
        self.flush_gui(100)
        self.assertEqual(s.Layers[str(layer)], "Profilé 'α'")
        self.tree.itemClicked.emit(self.row(0), 1)
        self.flush_gui(100)
        self.assertEqual(s.ActiveLayer, 0)
        self.tree.itemClicked.emit(self.row(layer), 0)
        self.flush_gui(100)
        self.assertEqual(s.ActiveLayer, 0)
        self.assertIn(layer, s.ViewObject.HiddenLayers)
        self.tree.itemClicked.emit(self.row(layer), 1)
        self.flush_gui(100)
        self.assertEqual(s.ActiveLayer, layer)
        self.assertNotIn(layer, s.ViewObject.HiddenLayers)
        geo = s.addGeometry(Part.Circle(App.Vector(30, 0, 0), App.Vector(0, 0, 1), 5))
        self.assertEqual(s.getGeometryLayer(geo), layer)

    def testHideDeselectUndoRemoveAndReenter(self):
        s = self.sketch
        layer = s.addLayer("Hidden profile")
        s.setGeometryLayer([0], layer)
        self.flush_gui(100)
        Gui.Selection.addSelection(s, "Edge1")
        self.assertTrue(Gui.Selection.getSelectionEx())
        self.tree.itemClicked.emit(self.row(layer), 0)
        self.flush_gui(100)
        self.assertIn(layer, s.ViewObject.HiddenLayers)
        self.assertFalse(Gui.Selection.getSelectionEx())
        self.doc.undo()
        self.flush_gui(100)
        self.assertNotIn(layer, s.ViewObject.HiddenLayers)
        s.setActiveLayer(layer)
        self.flush_gui(100)
        self.remove_layer(layer, destination=0)
        self.flush_gui(100)
        self.assertNotIn(str(layer), s.Layers)
        self.assertEqual(s.getGeometryLayer(0), 0)
        self.assertEqual(self.tree.topLevelItemCount(), 2)
        self.doc.undo()
        self.flush_gui(100)
        self.assertEqual(s.getGeometryLayer(0), layer)
        self.assertEqual(self.tree.topLevelItemCount(), 3)
        Gui.activeDocument().resetEdit()
        Gui.activeDocument().setEdit(s.Name)
        self.flush_gui(100)
        tree = Gui.getMainWindow().findChild(QtGui.QTreeWidget, "sketchLayers")
        self.assertEqual(tree.topLevelItemCount(), 3)

    def testHideDeselectsConstraintsAndKeepsVisibleSelection(self):
        s = self.sketch
        s.addGeometry(Part.LineSegment(App.Vector(30, 0, 0), App.Vector(40, 0, 0)))
        s.addConstraint(Sketcher.Constraint("Distance", 0, 20.0))
        s.addConstraint(Sketcher.Constraint("Distance", 1, 10.0))
        layer = s.addLayer("Hidden profile")
        s.setGeometryLayer([0], layer)
        self.doc.recompute()
        self.flush_gui(100)
        Gui.Selection.clearSelection()
        for name in ("Edge1", "Edge2", "Constraint1", "Constraint2"):
            Gui.Selection.addSelection(s, name)
        self.tree.itemClicked.emit(self.row(layer), 0)
        self.flush_gui(100)
        selected = {name for obj in Gui.Selection.getSelectionEx() for name in obj.SubElementNames}
        self.assertEqual(selected, {"Edge2", "Constraint2"})
        self.doc.undo()
        self.flush_gui(100)
        self.assertNotIn(layer, s.ViewObject.HiddenLayers)
        Gui.Selection.addSelection(s, "Constraint1")
        self.doc.redo()
        self.flush_gui(100)
        selected = {name for obj in Gui.Selection.getSelectionEx() for name in obj.SubElementNames}
        self.assertEqual(selected, {"Edge2", "Constraint2"})

    def testGeometryContextMenu(self):
        s = self.sketch
        layer = s.addLayer("Destination")
        self.flush_gui(100)
        Gui.Selection.clearSelection()
        Gui.Selection.addSelection(s, "Edge1")
        self.flush_gui(100)
        elements = Gui.getMainWindow().findChild(QtGui.QListWidget, "listWidgetElements")
        errors = []

        def choose_layer():
            menu = QtGui.QApplication.activePopupWidget()
            try:
                actions = menu.actions()
                layer_action = next(action for action in actions if action.text() == "Layer")
                submenu = layer_action.menu()
                layer_actions = submenu.actions()
                destination = next(
                    action for action in layer_actions if action.text() == "Destination"
                )
                destination.trigger()
            except Exception as error:
                import traceback

                errors.append(traceback.format_exc())
            finally:
                try:
                    if menu:
                        menu.close()
                except RuntimeError:
                    pass

        QtCore.QTimer.singleShot(100, choose_layer)
        point = elements.rect().center()
        event = QtGui.QContextMenuEvent(
            QtGui.QContextMenuEvent.Mouse, point, elements.mapToGlobal(point)
        )
        QtGui.QApplication.sendEvent(elements.viewport(), event)
        self.flush_gui(150)
        self.assertFalse(errors, errors)
        self.assertEqual(s.getGeometryLayer(0), layer)
        self.assertEqual(s.ActiveLayer, 0)
        self.doc.undo()
        self.assertEqual(s.getGeometryLayer(0), 0)

    def testLayerRenderingSwitches(self):
        from pivy import coin

        s = self.sketch
        layer = s.addLayer("Profile")
        s.setGeometryLayer([0], layer)
        self.flush_gui(100)

        search = coin.SoSearchAction()
        search.setName("CurvesLineSet30")  # Second named layer, ordinary element visibility.
        search.setSearchingAll(True)
        search.apply(Gui.activeDocument().activeView().getSceneGraph())
        path = search.getPath()
        self.assertIsNotNone(path)
        self.assertGreater(path.getTail().getField("numVertices").getNum(), 0)
        group = path.getNode(path.getLength() - 3)

        def curve_switches():
            return list(group.getField("enable").getValues())

        enabled = curve_switches()
        self.assertEqual(len(enabled), 30)  # Two named layers, three visual states, five sublayers.
        self.assertTrue(all(enabled[15:20]))
        self.tree.itemClicked.emit(self.row(layer), 0)
        self.flush_gui(100)
        enabled = curve_switches()
        self.assertFalse(any(enabled[15:30]))
        self.assertTrue(all(enabled[:5]))
        self.tree.itemClicked.emit(self.row(layer), 0)
        self.flush_gui(100)
        self.assertTrue(all(curve_switches()[15:20]))

    def setting_row(self, layer, text):
        # Use model indexes for children to avoid PySide item-wrapper ownership changes.
        model = self.tree.model()
        parent = self.tree.indexFromItem(self.row(layer), 0)
        for row in range(model.rowCount(parent)):
            if model.index(row, 1, parent).data() == text:
                return model.index(row, 2, parent)
        self.fail("Missing layer setting: " + text)

    def set_setting(self, layer, text, checked):
        self.tree.model().setData(self.setting_row(layer, text), checked, QtCore.Qt.CheckStateRole)

    def scene_node(self, name):
        from pivy import coin

        search = coin.SoSearchAction()
        search.setName(name)
        search.apply(Gui.activeDocument().activeView().getSceneGraph())
        self.assertIsNotNone(search.getPath(), name)
        return search.getPath().getTail()

    def testLayerTreeSettingsAndColorDialog(self):
        s = self.sketch
        self.assertEqual(self.tree.columnCount(), 4)
        self.assertEqual(self.row(0).childCount(), 5)
        self.assertEqual(self.row(-1).childCount(), 5)
        self.assertEqual([self.tree.header().logicalIndex(i) for i in range(4)], [0, 3, 1, 2])
        self.assertFalse(self.row(0).icon(3).isNull())
        self.assertTrue(self.row(-1).icon(3).isNull())
        self.row(0).setExpanded(True)
        for setting in ("Use constraints", "Use solved state colors"):
            index = self.setting_row(0, setting)
            self.assertTrue(index.data(QtCore.Qt.ToolTipRole))
            self.assertEqual(
                index.data(QtCore.Qt.ToolTipRole),
                index.siblingAtColumn(1).data(QtCore.Qt.ToolTipRole),
            )
        self.assertTrue(self.setting_row(0, "Line thickness").flags() & QtCore.Qt.ItemIsEnabled)
        self.set_setting(0, "Use constraints", QtCore.Qt.Unchecked)
        self.flush_gui(100)
        self.assertEqual(s.UnconstrainedLayers, [0])
        self.assertTrue(self.row(0).isExpanded())
        solved = self.setting_row(0, "Use solved state colors")
        self.assertFalse(solved.flags() & QtCore.Qt.ItemIsEnabled)
        self.assertEqual(s.DoF, 0)
        self.set_setting(0, "Use constraints", QtCore.Qt.Checked)
        self.flush_gui(100)
        self.assertEqual(s.UnconstrainedLayers, [])
        self.assertTrue(
            self.setting_row(0, "Use solved state colors").flags() & QtCore.Qt.ItemIsEnabled
        )
        rect = self.tree.visualRect(self.tree.indexFromItem(self.row(0), 3))
        self.click(self.tree.viewport(), rect.center())
        self.flush_gui(100)
        self.assertEqual(s.LockedLayers, [0])
        self.assertEqual(self.row(0).data(3, QtCore.Qt.AccessibleTextRole), "Locked")
        self.assertEqual(s.DoF, 0)
        self.doc.undo()
        self.flush_gui(100)
        self.assertEqual(s.LockedLayers, [])
        self.assertEqual(self.row(0).data(3, QtCore.Qt.AccessibleTextRole), "Unlocked")
        self.assertEqual(s.DoF, 4)
        self.row(0).setExpanded(False)
        errors = []
        dialogs = []

        def choose():
            dialog = QtGui.QApplication.activeModalWidget()
            try:
                self.assertIsInstance(dialog, QtGui.QColorDialog)
                dialogs.append(dialog.objectName())
                dialog.setCurrentColor(QtGui.QColor("#e04080"))
                dialog.accept()
            except Exception:
                import traceback

                errors.append(traceback.format_exc())
                if dialog:
                    dialog.reject()

        rect = self.tree.visualRect(self.tree.indexFromItem(self.row(0), 1))
        QtCore.QTimer.singleShot(100, choose)
        self.click(self.tree.viewport(), QtCore.QPoint(rect.left() + 9, rect.center().y()))
        self.flush_gui(150)
        self.assertFalse(errors, errors)
        self.assertEqual(dialogs, ["layerColorDialog"])
        self.assertEqual(s.ViewObject.LayerColors, {"0": "#e04080"})
        self.doc.undo()
        self.flush_gui(100)
        self.assertEqual(s.ViewObject.LayerColors, {})

    def testLayerScreenWidthsInsideAndOutsideEdit(self):
        from pivy import coin

        s = self.sketch
        layer = s.addLayer("Thick circle")
        other = s.addGeometry(Part.Circle(App.Vector(10, 20, 0), App.Vector(0, 0, 1), 5))
        s.setGeometryLayer([other], layer)
        s.addGeometry(Part.LineSegment(App.Vector(0, 10, 0), App.Vector(20, 10, 0)), True)
        s.solve()
        self.flush_gui(100)
        construction = self.scene_node("CurvesConstructionDrawStyle").lineWidth.getValue()
        widths = App.ParamGet("User parameter:BaseApp/Preferences/Mod/Sketcher/View")
        scale = self.scene_node("LayerDrawStyle0").lineWidth.getValue() / widths.GetInt(
            "EdgeWidth", 2
        )
        self.row(layer).setExpanded(True)
        picker = self.tree.findChild(QtGui.QDoubleSpinBox, f"layerLineWidth{layer}")
        picker.setValue(6.5)
        self.flush_gui(100)
        self.assertEqual(s.ViewObject.LayerLineWidths, {str(layer): "6.5"})
        self.assertAlmostEqual(self.scene_node("LayerDrawStyle3").lineWidth.getValue(), 6.5 * scale)
        self.assertEqual(
            self.scene_node("CurvesConstructionDrawStyle").lineWidth.getValue(), construction
        )
        Gui.activeDocument().activeView().fitAll()
        self.flush_gui(100)
        self.assertAlmostEqual(self.scene_node("LayerDrawStyle3").lineWidth.getValue(), 6.5 * scale)
        self.doc.undo()
        self.flush_gui(100)
        self.assertEqual(s.ViewObject.LayerLineWidths, {})
        self.doc.redo()
        self.flush_gui(100)
        self.assertAlmostEqual(self.scene_node("LayerDrawStyle3").lineWidth.getValue(), 6.5 * scale)
        Gui.activeDocument().resetEdit()
        self.doc.recompute()
        self.flush_gui(100)
        search = coin.SoSearchAction()
        search.setType(coin.SoType.fromName("SoBrepEdgeSet"))
        search.apply(Gui.activeDocument().activeView().getSceneGraph())
        node = search.getPath().getTail()
        self.assertEqual(sorted(node.lineWidths.getValues()), sorted([s.ViewObject.LineWidth, 6.5]))
        s.ViewObject.LineWidth = 3
        self.flush_gui(80)
        self.assertEqual(sorted(node.lineWidths.getValues()), [3.0, 6.5])
        # Malformed stored values safely inherit the sketch width.
        for value in ("nan", "inf", "-1", "1000", "bad"):
            s.ViewObject.LayerLineWidths = {str(layer): value}
            self.flush_gui(40)
            self.assertEqual(list(node.lineWidths.getValues()), [3.0, 3.0])

    def testDefaultControlsAndNewLayers(self):
        s = self.sketch
        add = self.row(-1)
        rect = self.tree.visualRect(self.tree.indexFromItem(add, 1))
        # Clicking the disclosure arrow must expand defaults without starting creation.
        self.click(
            self.tree.viewport(),
            QtCore.QPoint(rect.left() - self.tree.indentation() // 2, rect.center().y()),
        )
        self.flush_gui(60)
        self.assertTrue(self.row(-1).isExpanded())
        self.assertIsNone(self.tree.findChild(QtGui.QLineEdit, "newLayerName"))
        pattern = self.tree.findChild(QtGui.QComboBox, "layerPatternDefaults")
        pattern.setCurrentIndex(2)
        pattern.activated.emit(2)
        width = self.tree.findChild(QtGui.QDoubleSpinBox, "layerLineWidthDefaults")
        width.setValue(4.5)
        self.assertEqual(self.defaults.GetInt("Pattern"), 0xAAAA)
        self.assertEqual(self.defaults.GetFloat("LineWidth"), 4.5)
        errors = []

        def choose():
            dialog = QtGui.QApplication.activeModalWidget()
            try:
                self.assertIsInstance(dialog, QtGui.QColorDialog)
                dialog.setCurrentColor(QtGui.QColor("#20c080"))
                dialog.accept()
            except Exception as exc:
                errors.append(str(exc))
                if dialog:
                    dialog.reject()

        QtCore.QTimer.singleShot(100, choose)
        self.tree.findChild(QtGui.QPushButton, "layerColorDefaults").click()
        self.flush_gui(100)
        self.assertFalse(errors, errors)
        self.assertEqual(self.defaults.GetString("Color"), "#20c080")
        self.set_setting(-1, "Default solver colors", QtCore.Qt.Unchecked)
        self.flush_gui(60)
        self.set_setting(-1, "Default use constraints", QtCore.Qt.Unchecked)
        self.flush_gui(60)
        self.assertFalse(
            self.setting_row(-1, "Default solver colors").flags() & QtCore.Qt.ItemIsEnabled
        )
        self.assertFalse(self.defaults.GetBool("UseConstraints", True))
        self.assertFalse(self.defaults.GetBool("UseSolvedStateColors", True))
        self.assertEqual(s.ViewObject.LayerColors, {})
        self.assertEqual(s.ViewObject.LayerPatterns, {})
        self.assertEqual(s.ViewObject.LayerLineWidths, {})
        self.assertEqual(s.LockedLayers, [])
        self.assertEqual(s.UnconstrainedLayers, [])
        mime = self.tree.model().mimeData(
            [self.setting_row(-1, "Default use constraints").siblingAtColumn(1)]
        )
        self.assertFalse(mime.hasFormat("application/x-sketch-layer"))
        self.doc.openTransaction("New layer with defaults")
        layer = s.addLayer("Defaults")
        self.doc.commitTransaction()
        self.flush_gui(100)

        def check(sketch, layer):
            key = str(layer)
            self.assertEqual(sketch.ViewObject.LayerColors[key], "#20c080")
            self.assertEqual(int(sketch.ViewObject.LayerPatterns[key]), 0xAAAA)
            self.assertEqual(float(sketch.ViewObject.LayerLineWidths[key]), 4.5)
            self.assertNotIn(layer, sketch.LockedLayers)
            self.assertIn(layer, sketch.UnconstrainedLayers)
            self.assertIn(layer, sketch.ViewObject.LayerSolverColorsDisabled)

        check(s, layer)
        self.doc.undo()
        self.flush_gui(80)
        self.defaults.SetFloat("LineWidth", 9)
        self.defaults.SetString("Color", "#ff0000")
        self.doc.redo()
        self.flush_gui(80)
        check(s, layer)
        self.defaults.SetFloat("LineWidth", 4.5)
        self.defaults.SetString("Color", "#20c080")
        other = self.doc.addObject("Sketcher::SketchObject", "DefaultSketch")
        check(other, 0)
        self.doc.removeObject(other.Name)

    def testDefaultsDoNotAlterRestoredSketches(self):
        import os
        import tempfile

        Gui.activeDocument().resetEdit()
        self.doc.recompute()
        with tempfile.TemporaryDirectory() as folder:
            path = os.path.join(folder, "defaults.FCStd")
            self.doc.saveAs(path)
            App.closeDocument(self.doc.Name)
            self.defaults.SetString("Color", "#ff0000")
            self.defaults.SetInt("Pattern", 0xAAAA)
            self.defaults.SetFloat("LineWidth", 12)
            self.defaults.SetBool("Locked", True)
            self.defaults.SetBool("UseConstraints", False)
            self.defaults.SetBool("UseSolvedStateColors", False)
            self.doc = App.openDocument(path)
            self.sketch = self.doc.getObject("Sketch")
            self.assertEqual(self.sketch.ViewObject.LayerColors, {})
            self.assertEqual(self.sketch.ViewObject.LayerPatterns, {})
            self.assertEqual(self.sketch.ViewObject.LayerLineWidths, {})
            self.assertEqual(self.sketch.ViewObject.LayerSolverColorsDisabled, [])
            self.assertEqual(self.sketch.LockedLayers, [])
            self.assertEqual(self.sketch.UnconstrainedLayers, [])
            layer = self.sketch.addLayer("New after restoring")
            self.assertEqual(float(self.sketch.ViewObject.LayerLineWidths[str(layer)]), 12)
            self.assertNotIn(layer, self.sketch.LockedLayers)
            App.closeDocument(self.doc.Name)
            self.doc = None

    def testLayerColorPatternAndSolverOverrideRendering(self):
        s = self.sketch
        normal = self.scene_node("CurvesMaterials0")
        s.addGeometry(Part.LineSegment(App.Vector(0, 10, 0), App.Vector(20, 10, 0)), True)
        s.solve()
        self.flush_gui(100)
        construction = tuple(self.scene_node("CurvesMaterials1").diffuseColor[0].getValue())
        s.ViewObject.LayerColors = {"0": "#e04080"}
        picker = self.tree.itemWidget(self.row(0), 2)
        picker.setCurrentIndex(1)
        picker.activated.emit(1)
        self.flush_gui(100)
        self.assertEqual(s.ViewObject.LayerPatterns, {"0": "61680"})
        self.assertEqual(int(self.scene_node("LayerDrawStyle0").linePattern.getValue()), 0xF0F0)
        expected = (224 / 255.0, 64 / 255.0, 128 / 255.0)

        def assert_color(node, color):
            for actual, wanted in zip(node.diffuseColor[0].getValue(), color):
                self.assertAlmostEqual(actual, wanted, places=5)

        assert_color(self.scene_node("CurvesMaterials0"), expected)
        assert_color(self.scene_node("CurvesMaterials1"), construction)
        s.addConstraint(Sketcher.Constraint("Block", 0))
        s.solve()
        self.flush_gui(100)
        self.assertNotEqual(
            tuple(self.scene_node("CurvesMaterials0").diffuseColor[0].getValue()), expected
        )
        self.set_setting(0, "Use solved state colors", QtCore.Qt.Unchecked)
        self.flush_gui(100)
        assert_color(self.scene_node("CurvesMaterials0"), expected)
        Gui.activeDocument().resetEdit()
        self.doc.recompute()
        self.flush_gui(100)
        assert_color(self.scene_node("LineMaterial"), expected)
        # The outside-edit renderer retains a separate stipple value for each edge.
        from pivy import coin

        search = coin.SoSearchAction()
        search.setType(coin.SoType.fromName("SoBrepEdgeSet"))
        search.apply(Gui.activeDocument().activeView().getSceneGraph())
        self.assertIsNotNone(search.getPath())
        node = search.getPath().getTail()
        self.assertEqual(list(node.linePatterns.getValues()), [0xF0F0])

    def testExpandedLayerDragAndSettingsPersistence(self):
        import os
        import tempfile

        s = self.sketch
        layer = s.addLayer("Styled")
        s.setGeometryLayer([0], layer)
        self.flush_gui(100)
        self.row(layer).setExpanded(True)
        self.assertTrue(self.drop_layer(layer, 0))
        self.assertEqual(self.layer_order(), [layer, 0, -1])
        self.assertTrue(self.row(layer).isExpanded())
        child = self.setting_row(layer, "Use constraints")
        mime = self.tree.model().mimeData([child.siblingAtColumn(1)])
        self.assertFalse(mime.hasFormat("application/x-sketch-layer"))
        s.ViewObject.LayerColors = {str(layer): "#2060e0"}
        s.ViewObject.LayerPatterns = {str(layer): str(0xAAAA)}
        s.ViewObject.LayerSolverColorsDisabled = [layer]
        s.ViewObject.LayerLineWidths = {str(layer): "5.5"}
        with tempfile.TemporaryDirectory() as folder:
            path = os.path.join(folder, "styled.FCStd")
            Gui.activeDocument().resetEdit()
            self.doc.recompute()
            self.doc.saveAs(path)
            App.closeDocument(self.doc.Name)
            self.doc = App.openDocument(path)
            self.sketch = self.doc.getObject("Sketch")
            self.assertEqual(self.sketch.ViewObject.LayerColors, {str(layer): "#2060e0"})
            self.assertEqual(self.sketch.ViewObject.LayerPatterns, {str(layer): str(0xAAAA)})
            self.assertEqual(self.sketch.ViewObject.LayerSolverColorsDisabled, [layer])
            self.assertEqual(self.sketch.ViewObject.LayerOrder, [layer, 0])
            self.assertEqual(self.sketch.ViewObject.LayerLineWidths, {str(layer): "5.5"})
            App.closeDocument(self.doc.Name)
            self.doc = None

    def testIndependentLayerStylesSurviveVisibilityRefresh(self):
        s = self.sketch
        layer = s.addLayer("Colored")
        # A tessellated circle has many segments but only one edge material.
        other = s.addGeometry(Part.Circle(App.Vector(10, 20, 0), App.Vector(0, 0, 1), 5))
        s.setGeometryLayer([other], layer)
        s.ViewObject.LayerColors = {"0": "#ff0000", str(layer): "#00ff00"}
        s.ViewObject.LayerPatterns = {"0": str(0xF0F0), str(layer): str(0xAAAA)}
        s.ViewObject.LayerSolverColorsDisabled = [0, layer]
        s.solve()
        self.flush_gui(100)
        self.assertEqual(
            tuple(self.scene_node("CurvesMaterials0").diffuseColor[0].getValue()), (1.0, 0.0, 0.0)
        )
        self.assertEqual(
            tuple(self.scene_node("CurvesMaterials30").diffuseColor[0].getValue()), (0.0, 1.0, 0.0)
        )
        self.assertEqual(int(self.scene_node("LayerDrawStyle0").linePattern.getValue()), 0xF0F0)
        self.assertEqual(int(self.scene_node("LayerDrawStyle3").linePattern.getValue()), 0xAAAA)
        Gui.activeDocument().resetEdit()
        self.doc.recompute()
        Gui.Selection.clearSelection()
        for visible in (False, True):
            s.ViewObject.Visibility = visible
            self.flush_gui(60)
        colors = self.scene_node("LineMaterial").diffuseColor
        from pivy import coin

        self.assertEqual(
            self.scene_node("LineBind").value.getValue(), coin.SoMaterialBinding.PER_FACE
        )
        self.assertEqual(
            {tuple(color.getValue()) for color in colors.getValues()},
            {(1.0, 0.0, 0.0), (0.0, 1.0, 0.0)},
        )

    def testLockedLayerDragIsSilent(self):
        s = self.sketch
        s.moveGeometry(0, 0, App.Vector(0, 10, 0), True)
        self.doc.recompute()
        view = Gui.activeDocument().activeView()
        view.setCamera(
            "#Inventor V2.1 ascii\nOrthographicCamera { position 10 10 100 "
            "orientation 0 0 1 0 focalDistance 100 height 60 }"
        )
        viewport = view.graphicsView().viewport()
        reports = [
            widget
            for widget in Gui.getMainWindow().findChildren(QtGui.QTextEdit)
            if widget.metaObject().className().endswith("ReportOutput")
        ]
        self.assertTrue(reports)

        def drag_at(position):
            # Keep separate gestures outside the navigation double-click interval.
            self.pump(QtGui.QApplication.doubleClickInterval() + 20)
            start = self.viewport_to_qpoint(view, viewport, view.getPointOnScreen(position))
            end = start + QtCore.QPoint(35, -25)
            Gui.Selection.clearSelection()
            self.move(viewport, start)
            self.assertTrue(
                Gui.Selection.getPreselection().SubElementNames,
                f"No geometry at {start}; viewport={viewport.size()}, view={view.getSize()}",
            )
            self.send_mouse(
                viewport,
                QtCore.QEvent.MouseButtonPress,
                start,
                QtCore.Qt.LeftButton,
                QtCore.Qt.LeftButton,
            )
            for point in (start + QtCore.QPoint(8, -6), end):
                self.send_mouse(
                    viewport,
                    QtCore.QEvent.MouseMove,
                    point,
                    QtCore.Qt.NoButton,
                    QtCore.Qt.LeftButton,
                )
                self.flush_gui(60)
            self.send_mouse(
                viewport,
                QtCore.QEvent.MouseButtonRelease,
                end,
                QtCore.Qt.LeftButton,
                QtCore.Qt.NoButton,
            )
            self.flush_gui(100)

        s.LockedLayers = [0]
        self.flush_gui(100)
        before = s.Geometry[0].Content
        messages = [report.toPlainText() for report in reports]
        for point in (App.Vector(10, 10, 0), App.Vector(0, 10, 0)):
            drag_at(point)
            self.assertEqual(s.Geometry[0].Content, before)
            self.assertEqual([report.toPlainText() for report in reports], messages)
        # The same gesture must actually drag when the layer is unlocked.
        s.LockedLayers = []
        self.flush_gui(100)
        drag_at(App.Vector(10, 10, 0))
        self.assertNotEqual(s.Geometry[0].Content, before)

    def testShowLayersCheckboxAlignment(self):
        action = Gui.getMainWindow().findChild(QtGui.QAction, "showSketchLayers")
        menu = action.parent()
        menu.popup(Gui.getMainWindow().mapToGlobal(QtCore.QPoint(100, 100)))
        try:
            self.flush_gui(100)
            boxes = {box.text(): box for box in menu.findChildren(QtGui.QCheckBox)}
            positions = [
                boxes[text].mapToGlobal(QtCore.QPoint()).x()
                for text in ("Auto-update", "Show layers", "Display grid")
            ]
            self.assertEqual(len(set(positions)), 1)
            boxes["Show layers"].click()
            self.flush_gui(100)
            self.assertFalse(self.layerPrefs.GetBool("ShowLayers"))
            self.assertFalse(action.isChecked())
        finally:
            menu.close()

    def testNoConstraintsLayerHidesAndDeactivatesExistingConstraints(self):
        s = self.sketch
        other = s.addGeometry(Part.LineSegment(App.Vector(0, 10, 0), App.Vector(20, 10, 0)))
        layer = s.addLayer("Free drawing")
        s.setGeometryLayer([0], layer)
        own, cross, unaffected = s.addConstraint(
            [
                Sketcher.Constraint("Distance", 0, 20.0),
                Sketcher.Constraint("Equal", 0, other),
                Sketcher.Constraint("Horizontal", other),
            ]
        )
        s.solve()
        self.flush_gui(100)
        self.assertEqual(list(self.scene_node("ConstraintGroup").enable.getValues()), [True] * 3)
        Gui.Selection.addSelection(s, "Constraint1")
        self.set_setting(layer, "Use constraints", QtCore.Qt.Unchecked)
        self.flush_gui(100)
        self.assertEqual(
            [s.getActive(index) for index in (own, cross, unaffected)], [False, False, True]
        )
        self.assertEqual(
            list(self.scene_node("ConstraintGroup").enable.getValues()), [False, False, True]
        )
        self.assertFalse(Gui.Selection.getSelectionEx())
        self.assertEqual(s.DoF, 3)
        self.doc.undo()
        self.flush_gui(100)
        self.assertTrue(s.getActive(own))
        self.assertTrue(s.getActive(cross))
        self.assertEqual(list(self.scene_node("ConstraintGroup").enable.getValues()), [True] * 3)

    def testGeometryLayerSelector(self):
        layer = self.sketch.addLayer("Geometry")
        self.flush_gui(120)
        elements = Gui.getMainWindow().findChild(QtGui.QListWidget, "listWidgetElements")
        combo = elements.indexWidget(elements.model().index(0, 0))
        self.assertIsInstance(combo, QtGui.QComboBox)
        rect = elements.visualItemRect(elements.item(0))
        self.click(elements.viewport(), QtCore.QPoint(165, rect.center().y()))
        self.flush_gui(100)
        self.assertTrue(elements.item(0).isSelected())
        self.assertIn("Edge1", Gui.Selection.getSelectionEx()[0].SubElementNames)
        combo.setCurrentIndex(combo.findData(layer))
        combo.activated.emit(combo.currentIndex())
        self.flush_gui(150)
        self.assertEqual(self.sketch.getGeometryLayer(0), layer)
        self.doc.undo()
        self.flush_gui(150)
        combo = elements.indexWidget(elements.model().index(0, 0))
        self.assertEqual(combo.currentData(), 0)
        self.sketch.LockedLayers = [0]
        self.flush_gui(150)
        self.assertFalse(elements.indexWidget(elements.model().index(0, 0)).isEnabled())
        self.layerPrefs.SetBool("ShowLayers", False)
        self.flush_gui(150)
        self.assertIsNone(elements.indexWidget(elements.model().index(0, 0)))
