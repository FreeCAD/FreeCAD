# SPDX-License-Identifier: LGPL-2.1-or-later

from pathlib import Path
import tempfile

import FreeCAD as App
import Sketcher
import SketcherBlock
from PySide import QtCore, QtGui, QtWidgets
from SketcherTests.GuiTestCase import FreeCADGui as Gui, SketcherGuiTestCase


class TestSketchBlocksGui(SketcherGuiTestCase):
    def setUp(self):
        super().setUp()
        self.directory = tempfile.TemporaryDirectory()
        Gui.activateWorkbench("SketcherWorkbench")
        self.doc = App.newDocument("SketchBlocksGuiTest")
        self.doc.UndoMode = 1
        self.sketch = self.doc.addObject("Sketcher::SketchObject", "Sketch")
        self.doc.recompute()
        Gui.activeDocument().setEdit(self.sketch.Name)

    def tearDown(self):
        super().tearDown()
        self.directory.cleanup()

    def block(self, circle=False):
        path = Path(self.directory.name) / "block 'é.txt"
        if circle:
            geometry = ["Part.Circle(App.Vector(5, 5, 0), App.Vector(0, 0, 1), 5)"]
        else:
            geometry = [
                "Part.LineSegment(App.Vector(0, 0, 0), App.Vector(10, 0, 0))",
                "Part.LineSegment(App.Vector(10, 0, 0), App.Vector(10, 5, 0))",
                "Part.LineSegment(App.Vector(10, 5, 0), App.Vector(0, 5, 0))",
                "Part.LineSegment(App.Vector(0, 5, 0), App.Vector(0, 0, 0))",
            ]
        text = "# Copied from sketcher.\ngeoList = [" + ",\n".join(geometry)
        text += "]\nobjectStr.addGeometry(geoList, False)\n"
        path.write_text(text, encoding="utf-8")
        return path

    def testCopyPastePreservesSource(self):
        path = self.block()
        SketcherBlock.insert_geometry(self.sketch, SketcherBlock.read(path), path)
        s = self.sketch
        group = s.Constraints[0]
        clipboard = QtWidgets.QApplication.clipboard()
        saved = clipboard.text()
        try:
            Gui.Selection.clearSelection()
            Gui.Selection.addSelection(s, f"Edge{group.First + 1}")
            Gui.runCommand("Sketcher_CopyClipboard", 0)
            copied = clipboard.text()
            self.assertIn("len(objectStr.Geometry)", copied)
            copied_file = Path(self.directory.name) / "copied.txt"
            copied_file.write_text(copied, encoding="utf-8")
            self.assertEqual(len(SketcherBlock.read(copied_file)), s.GeometryCount)
            Gui.runCommand("Sketcher_Paste", 0)
            self.assertEqual(s.solve(), 0)
            self.assertEqual(len(s.Constraints), 2)
            self.assertEqual(s.Constraints[1].File, group.File)
            self.assertEqual(s.Constraints[1].FileHeight, group.FileHeight)
        finally:
            clipboard.setText(saved)

    def testInsertBlockTool(self):
        self.assertIn("Sketcher_InsertBlock", Gui.listCommands())
        view = Gui.activeDocument().activeView()
        view.setCamera(
            "#Inventor V2.1 ascii\nOrthographicCamera { position 0 0 100 "
            "orientation 0 0 1 0 focalDistance 100 height 100 }"
        )
        self.flush_gui(100)
        Gui.runCommand("Sketcher_InsertBlock", 0)
        self.flush_gui(100)
        combos = Gui.getMainWindow().findChildren(QtWidgets.QComboBox)
        library = next(combo for combo in combos if combo.findText("CE") >= 0)
        library.setCurrentIndex(library.findText("CE"))
        viewport = view.graphicsView().viewport()
        for x, y in ((-20, -10), (20, -10)):
            point = self.viewport_to_qpoint(
                view, viewport, view.getPointOnScreen(App.Vector(x, y, 0))
            )
            self.move(viewport, point)
            self.click(viewport, point)
        self.assertEqual(self.sketch.solve(), 0)
        groups = [c for c in self.sketch.Constraints if c.Type == "Group"]
        self.assertEqual(len(groups), 1)
        self.assertEqual(Path(groups[0].File).name, "CE.txt")
        self.assertGreater(self.sketch.GeometryCount, 1)

    def testInsertCustomSingleCurveBlock(self):
        path = Path(self.directory.name) / "custom 'é.txt"
        path.write_text(
            "# Copied from sketcher.\ngeoList = []\n"
            "geoList.append(Part.Circle(App.Vector(0,0,0), App.Vector(0,0,1), 5))\n"
            "objectStr.addGeometry(geoList, False)\n",
            encoding="utf-8",
        )
        preferences = App.ParamGet("User parameter:BaseApp/Preferences/Dialog")
        old_native = preferences.GetBool("DontUseNativeDialog")
        preferences.SetBool("DontUseNativeDialog", True)
        view = Gui.activeDocument().activeView()
        view.setCamera(
            "#Inventor V2.1 ascii\nOrthographicCamera { position 0 0 100 "
            "orientation 0 0 1 0 focalDistance 100 height 100 }"
        )
        self.flush_gui(100)
        Gui.runCommand("Sketcher_InsertBlock", 0)
        combos = Gui.getMainWindow().findChildren(QtWidgets.QComboBox)
        library = next(combo for combo in combos if combo.findText("CE") >= 0)
        errors = []

        def choose():
            dialog = next(
                w
                for w in QtWidgets.QApplication.topLevelWidgets()
                if isinstance(w, QtWidgets.QFileDialog) and w.isVisible()
            )
            try:
                dialog.findChild(QtWidgets.QLineEdit, "fileNameEdit").setText('"' + str(path) + '"')
                dialog.accept()
            except Exception as error:
                errors.append(error)
                dialog.reject()

        try:
            QtCore.QTimer.singleShot(100, choose)
            library.setCurrentIndex(library.count() - 1)
        finally:
            preferences.SetBool("DontUseNativeDialog", old_native)
        self.assertFalse(errors, errors)
        viewport = view.graphicsView().viewport()
        for x, y in ((-20, -10), (20, -10)):
            point = self.viewport_to_qpoint(
                view, viewport, view.getPointOnScreen(App.Vector(x, y, 0))
            )
            self.move(viewport, point)
            self.click(viewport, point)
        self.assertEqual(self.sketch.GeometryCount, 2)
        self.assertEqual(self.sketch.solve(), 0)
        group = next(c for c in self.sketch.Constraints if c.Type == "Group")
        self.assertEqual(Path(group.File), path)

    def testReloadContextMenuAndUndo(self):
        path = self.block()
        SketcherBlock.insert_geometry(self.sketch, SketcherBlock.read(path), path)
        self.block(True)
        self.flush_gui(100)
        constraints = Gui.getMainWindow().findChild(QtWidgets.QListWidget, "listWidgetConstraints")
        constraints.setCurrentRow(0)
        errors = []

        def reload_action():
            menu = QtWidgets.QApplication.activePopupWidget()
            try:
                action = next(
                    action for action in menu.actions() if action.text() == "Reload From File"
                )
                action.trigger()
            except Exception as error:
                errors.append(error)
            finally:
                if menu:
                    menu.close()

        QtCore.QTimer.singleShot(100, reload_action)
        point = QtCore.QPoint(5, 5)
        event = QtGui.QContextMenuEvent(
            QtGui.QContextMenuEvent.Mouse, point, constraints.viewport().mapToGlobal(point)
        )
        QtWidgets.QApplication.sendEvent(constraints.viewport(), event)
        self.assertFalse(errors, errors)
        self.assertEqual(self.sketch.GeometryCount, 2)
        self.assertEqual(self.sketch.solve(), 0)
        self.doc.undo()
        self.assertEqual(self.sketch.GeometryCount, 5)

    def prepareBlockSelection(self):
        import Part

        self.sketch.addGeometry(Part.Circle(App.Vector(0, 0, 0), App.Vector(0, 0, 1), 5))
        self.sketch.addGeometry(Part.LineSegment(App.Vector(10, 0, 0), App.Vector(10, 8, 0)), True)
        self.sketch.addGeometry(Part.Circle(App.Vector(20, 0, 0), App.Vector(0, 0, 1), 2))
        self.doc.recompute()
        Gui.Selection.clearSelection()
        for edge in ("Edge1", "Edge2"):
            Gui.Selection.addSelection(self.sketch, edge)
        self.flush_gui(100)

    def testCreateBlockMenuAvailability(self):
        self.prepareBlockSelection()
        command = Gui.Command.get("Sketcher_CreateBlock")
        action = command.getAction()[0]
        self.assertTrue(
            any(
                action in menu.actions()
                for menu in Gui.getMainWindow().findChildren(QtWidgets.QMenu)
            )
        )
        self.assertFalse(
            any(
                action in bar.actions()
                for bar in Gui.getMainWindow().findChildren(QtWidgets.QToolBar)
            )
        )
        viewport = Gui.activeDocument().activeView().graphicsView().viewport()
        for names, expected in (
            ([], False),
            (["Edge1"], False),
            (["Edge1", "Vertex1"], False),
            (["Edge1", "Edge2"], True),
        ):
            with self.subTest(selection=names):
                Gui.Selection.clearSelection()
                for name in names:
                    Gui.Selection.addSelection(self.sketch, name)
                self.flush_gui(100)
                self.assertEqual(command.isActive(), expected)
                found = []

                def inspect_menu():
                    menu = QtWidgets.QApplication.activePopupWidget()
                    found.append(isinstance(menu, QtWidgets.QMenu) and action in menu.actions())
                    if menu:
                        menu.close()

                QtCore.QTimer.singleShot(100, inspect_menu)
                self.right_click(viewport, QtCore.QPoint(20, 20))
                self.assertEqual(found, [expected])

    def testCreateBlockSaveAndCancel(self):
        self.prepareBlockSelection()
        clipboard = QtWidgets.QApplication.clipboard()
        previous = clipboard.text()
        preferences = App.ParamGet("User parameter:BaseApp/Preferences/Dialog")
        old_native = preferences.GetBool("DontUseNativeDialog")
        preferences.SetBool("DontUseNativeDialog", True)
        path = Path(self.directory.name) / "created 'é.txt"
        errors = []
        before = [geo.Content for geo in self.sketch.Geometry]

        def choose_save():
            dialog = QtWidgets.QApplication.activeModalWidget()
            try:
                self.assertIsInstance(dialog, QtWidgets.QFileDialog)
                self.assertEqual(dialog.acceptMode(), QtWidgets.QFileDialog.AcceptSave)
                self.assertEqual(
                    Path(dialog.directory().absolutePath()),
                    Path(App.getResourceDir()) / "Mod/Sketcher/Blocks",
                )
                dialog.findChild(QtWidgets.QLineEdit, "fileNameEdit").setText(
                    '"' + str(path.with_suffix("")) + '"'
                )
                dialog.accept()
            except Exception as error:
                errors.append(error)
                if dialog:
                    dialog.reject()

        def create_from_menu():
            menu = QtWidgets.QApplication.activePopupWidget()
            try:
                self.assertIsInstance(menu, QtWidgets.QMenu)
                action = Gui.Command.get("Sketcher_CreateBlock").getAction()[0]
                self.assertIn(action, menu.actions())
                QtCore.QTimer.singleShot(100, choose_save)
                action.trigger()
            except Exception as error:
                errors.append(error)
            finally:
                if menu:
                    menu.close()

        try:
            Gui.runCommand("Sketcher_CopyClipboard", 0)
            expected = clipboard.text()
            clipboard.setText("Preserve the clipboard")
            QtCore.QTimer.singleShot(100, create_from_menu)
            viewport = Gui.activeDocument().activeView().graphicsView().viewport()
            self.right_click(viewport, QtCore.QPoint(20, 20))
            self.assertFalse(errors, errors)
            self.assertEqual(path.read_text(encoding="utf-8"), expected)
            geometry = SketcherBlock.read(path)
            self.assertEqual(len(geometry), 2)
            self.assertEqual(
                [Sketcher.GeometryFacade(geo).Construction for geo in geometry], [False, True]
            )
            self.assertEqual(clipboard.text(), "Preserve the clipboard")
            self.assertEqual([geo.Content for geo in self.sketch.Geometry], before)

            def cancel_save():
                dialog = QtWidgets.QApplication.activeModalWidget()
                if dialog:
                    dialog.reject()

            QtCore.QTimer.singleShot(100, cancel_save)
            Gui.runCommand("Sketcher_CreateBlock", 0)
            self.assertEqual(list(Path(self.directory.name).iterdir()), [path])
            self.assertEqual(path.read_text(encoding="utf-8"), expected)
            self.assertEqual(clipboard.text(), "Preserve the clipboard")
            self.assertEqual([geo.Content for geo in self.sketch.Geometry], before)
        finally:
            clipboard.setText(previous)
            preferences.SetBool("DontUseNativeDialog", old_native)
