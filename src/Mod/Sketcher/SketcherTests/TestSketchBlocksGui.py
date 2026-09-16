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
        # These tests exercise mouse placement, not the editable on-view controls.
        # Those controls can cover the click target depending on the platform's fonts.
        tools = App.ParamGet("User parameter:BaseApp/Preferences/Mod/Sketcher/Tools")
        visibility = tools.GetInt("OnViewParameterVisibility", 1)
        self.addCleanup(tools.SetInt, "OnViewParameterVisibility", visibility)
        tools.SetInt("OnViewParameterVisibility", 0)
        params = App.ParamGet("User parameter:BaseApp/Preferences/Mod/Sketcher")
        continuous = params.GetBool("ContinuousCreationMode", True)
        self.addCleanup(params.SetBool, "ContinuousCreationMode", continuous)
        params.SetBool("ContinuousCreationMode", True)
        self.directory = tempfile.TemporaryDirectory()
        self.library_preferences = App.ParamGet(
            "User parameter:BaseApp/Preferences/Mod/Sketcher/BlockLibrary"
        )
        self.saved_folders = self.library_preferences.GetString("Folders")
        self.saved_file = self.library_preferences.GetString("LastFile")
        Gui.activateWorkbench("SketcherWorkbench")
        self.doc = App.newDocument("SketchBlocksGuiTest")
        self.doc.UndoMode = 1
        self.sketch = self.doc.addObject("Sketcher::SketchObject", "Sketch")
        self.doc.recompute()
        Gui.activeDocument().setEdit(self.sketch.Name)
        view = Gui.activeDocument().activeView()
        view.setCamera(
            "#Inventor V2.1 ascii\nOrthographicCamera { position 0 0 100 "
            "orientation 0 0 1 0 focalDistance 100 height 160 }"
        )
        self.flush_gui(100)

    def tearDown(self):
        super().tearDown()
        self.directory.cleanup()
        self.library_preferences.SetString("Folders", self.saved_folders)
        self.library_preferences.SetString("LastFile", self.saved_file)

    def visible_widget(self, kind, name):
        widgets = []

        def find_visible():
            widgets[:] = [
                widget
                for widget in Gui.getMainWindow().findChildren(kind, name)
                if widget.isVisible()
            ]
            return len(widgets) == 1

        self.assertTrue(self.wait_until(find_visible, timeout_ms=3000), name)
        return widgets[0]

    def when_popup_visible(self, kind, callback):
        # A fixed delay can expire while a slow runner is still rendering the view,
        # before the mouse event has opened the menu or file dialog.
        timer = QtCore.QTimer(Gui.getMainWindow())
        timer.setInterval(25)

        def check():
            widgets = (
                QtWidgets.QApplication.activePopupWidget(),
                QtWidgets.QApplication.activeModalWidget(),
            )
            if any(isinstance(widget, kind) and widget.isVisible() for widget in widgets):
                timer.stop()
                callback()

        timer.timeout.connect(check)
        self.addCleanup(timer.deleteLater)
        self.addCleanup(timer.stop)
        timer.start()

    def click(self, widget, pos):
        # Settle task-panel layout changes and restore focus after file dialogs.
        widget.setFocus(QtCore.Qt.OtherFocusReason)
        self.flush_gui(50)
        self.assertTrue(widget.rect().contains(pos), "Click must lie inside the viewport")
        super().click(widget, pos)

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

    def selectLibraryBlock(self, name):
        tree = self.visible_widget(QtWidgets.QTreeWidget, "blockLibraryTree")
        self.assertIsNotNone(tree)
        tree.topLevelItem(0).setExpanded(True)
        items = tree.findItems(name, QtCore.Qt.MatchExactly | QtCore.Qt.MatchRecursive)
        self.assertTrue(items, name)
        tree.setCurrentItem(items[0])
        return tree

    def checkBlockPlacement(self, fixed_size, fixed_orientation, height=False):
        import math
        import Part

        view = Gui.activeDocument().activeView()
        view.setCamera(
            "#Inventor V2.1 ascii\nOrthographicCamera { position 0 0 100 "
            "orientation 0 0 1 0 focalDistance 100 height 160 }"
        )
        self.flush_gui(100)
        Gui.runCommand("Sketcher_InsertBlock", 0)
        self.flush_gui(100)
        combos = [w for w in Gui.getMainWindow().findChildren(QtWidgets.QComboBox) if w.isVisible()]
        self.selectLibraryBlock("CE")
        method = next(
            combo
            for combo in combos
            if combo.findText("Width") >= 0 and combo.findText("Height") >= 0
        )
        method.setCurrentIndex(method.findText("Height" if height else "Width"))
        boxes = [w for w in Gui.getMainWindow().findChildren(QtWidgets.QCheckBox) if w.isVisible()]
        size_box = next(box for box in boxes if box.text() == "Fixed Size")
        orientation_box = next(box for box in boxes if box.text() == "Fixed Orientation")
        self.assertFalse(size_box.isChecked())
        self.assertFalse(orientation_box.isChecked())
        size_box.setChecked(fixed_size)
        orientation_box.setChecked(fixed_orientation)
        # Changing options must only update the preview, never insert geometry.
        self.assertEqual(self.sketch.GeometryCount, 0)
        viewport = view.graphicsView().viewport()

        def place(x, y):
            point = self.viewport_to_qpoint(
                view, viewport, view.getPointOnScreen(App.Vector(x, y, 0))
            )
            self.move(viewport, point)
            self.click(viewport, point)

        place(-20, -10)
        if not (fixed_size and fixed_orientation):
            self.assertEqual(self.sketch.GeometryCount, 0)
            place(10, 20)
        self.assertEqual(self.sketch.solve(), 0)
        groups = [c for c in self.sketch.Constraints if c.Type == "Group"]
        self.assertEqual(len(groups), 1)
        group = groups[0]
        self.assertEqual(group.FileHeight, height)
        handle = self.sketch.Geometry[group.First]
        origin = App.Vector(handle.X, handle.Y, handle.Z) if fixed_size else handle.StartPoint
        direction = None if fixed_size else handle.EndPoint - handle.StartPoint
        source = SketcherBlock.read(group.File)
        bounds = Part.makeCompound([geo.toShape() for geo in source]).BoundBox
        native_length = bounds.YLength if height else bounds.XLength
        expected_length = native_length if fixed_size else math.hypot(30, 30)
        if fixed_size:
            self.assertIsInstance(handle, Part.Point)
            members = [g for i, g in enumerate(self.sketch.Geometry) if i != group.First]
            self.assertAlmostEqual(
                sum(g.toShape().Length for g in members),
                sum(g.toShape().Length for g in source),
                delta=1e-5,
            )
        else:
            self.assertAlmostEqual(direction.Length, expected_length, delta=0.4)
        expected_angle = (math.pi / 2 if height else 0) if fixed_orientation else math.pi / 4
        angle = (
            group.FileAngle + (math.pi / 2 if height else 0)
            if fixed_size
            else math.atan2(direction.y, direction.x)
        )
        self.assertAlmostEqual(angle, expected_angle, delta=0.03)
        self.assertAlmostEqual(origin.x, -20, delta=0.4)
        self.assertAlmostEqual(origin.y, -10, delta=0.4)
        if fixed_size and fixed_orientation:
            # Continuous insertion keeps both choices and still takes one click.
            place(15, 17)
            self.assertEqual(len([c for c in self.sketch.Constraints if c.Type == "Group"]), 2)
            self.doc.undo()
            self.assertEqual(len([c for c in self.sketch.Constraints if c.Type == "Group"]), 1)

    def testBlockFixedSizeWidth(self):
        self.checkBlockPlacement(True, False, False)

    def testBlockFixedSizeHeight(self):
        self.checkBlockPlacement(True, False, True)

    def testBlockFixedOrientationWidth(self):
        self.checkBlockPlacement(False, True, False)

    def testBlockFixedOrientationHeight(self):
        self.checkBlockPlacement(False, True, True)

    def testBlockOneClickWidth(self):
        self.checkBlockPlacement(True, True, False)

    def testBlockOneClickHeight(self):
        self.checkBlockPlacement(True, True, True)

    def testBlockFreePlacementWidth(self):
        self.checkBlockPlacement(False, False, False)

    def testBlockFreePlacementHeight(self):
        self.checkBlockPlacement(False, False, True)

    def testFixedBlockOriginMoveReloadAndCopy(self):
        import math
        import Part

        path = Path(self.directory.name) / "offset.txt"
        path.write_text(
            "# Copied from sketcher.\ngeoList = [Part.LineSegment(App.Vector(5,9,0), App.Vector(15,9,0))]\n"
            "objectStr.addGeometry(geoList,False)\n",
            encoding="utf-8",
        )
        index = SketcherBlock.insert_geometry(
            self.sketch, SketcherBlock.read(path), path, True, App.Vector(20, -7, 0), math.pi / 2
        )
        self.assertEqual(self.sketch.solve(), 0)
        group = self.sketch.Constraints[index]
        handle = group.First
        member = group.Second
        self.assertIsInstance(self.sketch.Geometry[handle], Part.Point)
        edge = self.sketch.Geometry[member]
        self.assertLess((edge.StartPoint - App.Vector(11, -2, 0)).Length, 1e-6)
        self.assertAlmostEqual(edge.length(), 10)
        self.sketch.moveGeometry(handle, 1, App.Vector(30, -3, 0), 0)
        self.assertEqual(self.sketch.solve(), 0)
        moved = self.sketch.Geometry[member]
        self.assertLess((moved.StartPoint - App.Vector(21, 2, 0)).Length, 1e-6)
        self.assertAlmostEqual(moved.length(), 10)
        path.write_text(path.read_text().replace("15,9,0", "25,9,0"), encoding="utf-8")
        SketcherBlock.reload(self.sketch, index)
        self.assertEqual(self.sketch.solve(), 0)
        group = self.sketch.Constraints[index]
        self.assertAlmostEqual(self.sketch.Geometry[group.Second].length(), 20)
        self.assertAlmostEqual(group.FileAngle, math.pi / 2)
        snapshot = Path(self.directory.name) / "snapshot.txt"
        snapshot.write_text(
            "# Copied from sketcher.\n"
            + "\n".join(
                line
                for line in self.sketch.toPythonCommands()
                if "exposeInternalGeometry(" not in line
            ).replace("ActiveSketch", "objectStr"),
            encoding="utf-8",
        )
        self.assertEqual(len(SketcherBlock.read(snapshot)), 2)
        saved = Path(self.directory.name) / "fixed.FCStd"
        self.doc.saveAs(str(saved))
        reopened = saved.with_name("reopened.FCStd")
        reopened.write_bytes(saved.read_bytes())
        restored = App.openDocument(str(reopened), hidden=True)
        try:
            restored_group = restored.getObject(self.sketch.Name).Constraints[index]
            self.assertAlmostEqual(restored_group.FileAngle, math.pi / 2)
        finally:
            App.closeDocument(restored.Name)
            App.setActiveDocument(self.doc.Name)

    def testBlockConstraintLabels(self):
        import Part

        for filename in ("", "source.svg", "source.txt"):
            start = self.sketch.addGeometry(
                Part.LineSegment(App.Vector(0, 0, 0), App.Vector(10, 0, 0)), True
            )
            member = self.sketch.addGeometry(
                Part.LineSegment(App.Vector(0, 1, 0), App.Vector(10, 1, 0))
            )
            self.sketch.addConstraint(
                SketcherBlock.Sketcher.Constraint("Group", [start, 0, member, 0], filename)
            )
        self.doc.recompute()
        self.flush_gui(100)
        listing = self.visible_widget(QtWidgets.QListWidget, "listWidgetConstraints")
        labels = [listing.item(i).text() for i in range(listing.count())]
        self.assertTrue(any("Group" in label for label in labels), labels)
        self.assertTrue(any("SVG" in label for label in labels), labels)
        self.assertTrue(any("Block" in label for label in labels), labels)

    def testEditBlockUpdatesAllOpenInstances(self):
        import Part

        path = Path(self.directory.name) / "editor.txt"
        path.write_text(
            "# Copied from sketcher.\n# Sketcher block fixed size: true\n"
            "geoList = [Part.Circle(App.Vector(5,9,0),App.Vector(0,0,1),2)]\n"
            "objectStr.addGeometry(geoList,False)\n",
            encoding="utf-8",
        )
        SketcherBlock.insert_geometry(self.sketch, SketcherBlock.read(path), path, True)
        other = App.newDocument("OtherBlockInstances")
        other.UndoMode = 1
        other_sketch = other.addObject("Sketcher::SketchObject", "Sketch")
        for x in (10, 30):
            SketcherBlock.insert_geometry(
                other_sketch, SketcherBlock.read(path), path, True, App.Vector(x, 0, 0)
            )
        other.recompute()
        App.setActiveDocument(self.doc.Name)
        clipboard = QtWidgets.QApplication.clipboard()
        clipboard.setText("block editor clipboard sentinel")
        timer = QtCore.QTimer()
        timer.timeout.connect(
            lambda: [
                w.accept()
                for w in QtWidgets.QApplication.topLevelWidgets()
                if isinstance(w, QtWidgets.QMessageBox) and w.isVisible()
            ]
        )
        timer.start(50)
        try:
            Gui.Selection.clearSelection()
            Gui.Selection.addSelection(self.sketch, "Constraint1")
            self.assertTrue(Gui.Command.get("Sketcher_EditBlock").isActive())
            Gui.runCommand("Sketcher_EditBlock", 0)
            self.assertTrue(self.wait_until(lambda: bool(SketcherBlock._editors)))
            session = next(iter(SketcherBlock._editors.values()))
            name = session.document.Name
            self.assertEqual(session.document.Label, "Block Edit")
            self.assertEqual(Gui.activeDocument().getInEdit().Object, session.sketch)
            self.assertEqual(session.sketch.GeometryCount, 1)
            self.assertAlmostEqual(session.sketch.Geometry[0].Center.x, 5)
            session.sketch.setDatum(
                session.sketch.addConstraint(SketcherBlock.Sketcher.Constraint("Radius", 0, 4.0)),
                App.Units.Quantity("4 mm"),
            )
            Gui.activeDocument().resetEdit()
            self.assertTrue(self.wait_until(lambda: name not in App.listDocuments(), 3000))
            self.assertIsNone(session.error)
            self.assertAlmostEqual(SketcherBlock.read(path)[0].Radius, 4)
            self.assertTrue(SketcherBlock.metadata(path)["fixed_size"])
            _, source_constraints = SketcherBlock.read(path, with_constraints=True)
            self.assertEqual(len(source_constraints), 1)
            self.assertEqual(source_constraints[0].Type, "Radius")
            for sketch in (self.sketch, other_sketch):
                for group in sketch.Constraints:
                    self.assertAlmostEqual(sketch.Geometry[group.Second].Radius, 4)
            self.assertEqual(clipboard.text(), "block editor clipboard sentinel")
            other.undo()
            for group in other_sketch.Constraints:
                self.assertAlmostEqual(other_sketch.Geometry[group.Second].Radius, 2)
        finally:
            timer.stop()
            for session in list(SketcherBlock._editors.values()):
                session.closing = True
                session._detach()
                App.closeDocument(session.document.Name)
            App.closeDocument(other.Name)
            App.setActiveDocument(self.doc.Name)

    def testBlockEditorFailedSavePreservesSourceAndInstances(self):
        from unittest import mock

        path = Path(self.directory.name) / "failed-save.txt"
        path.write_text(
            "# Copied from sketcher.\n# Sketcher block fixed size: true\n"
            "geoList = [Part.Circle(App.Vector(5,9,0),App.Vector(0,0,1),2)]\n"
            "objectStr.addGeometry(geoList,False)\n",
            encoding="utf-8",
        )
        SketcherBlock.insert_geometry(self.sketch, SketcherBlock.read(path), path, True)
        before = path.read_bytes()
        session = SketcherBlock._open_editor(self.sketch, str(path))
        session.sketch.addConstraint(SketcherBlock.Sketcher.Constraint("Radius", 0, 4.0))
        real_save_file = QtCore.QSaveFile

        class FailedCommit:
            def __init__(self, filename):
                self.file = real_save_file(filename)

            def open(self, flags):
                return self.file.open(flags)

            def write(self, data):
                return self.file.write(data)

            def commit(self):
                return False

            def cancelWriting(self):
                self.file.cancelWriting()

            def errorString(self):
                return "Simulated save failure"

        timer = QtCore.QTimer()
        timer.timeout.connect(
            lambda: [
                w.accept()
                for w in QtWidgets.QApplication.topLevelWidgets()
                if isinstance(w, QtWidgets.QMessageBox) and w.isVisible()
            ]
        )
        timer.start(50)
        try:
            with mock.patch.object(QtCore, "QSaveFile", FailedCommit):
                Gui.activeDocument().resetEdit()
                self.assertTrue(self.wait_until(lambda: session.error is not None, 3000))
            self.assertIn(session.document.Name, App.listDocuments())
            self.assertEqual(path.read_bytes(), before)
            self.assertAlmostEqual(
                self.sketch.Geometry[self.sketch.Constraints[0].Second].Radius, 2
            )
            self.assertAlmostEqual(session.sketch.Geometry[0].Radius, 4)
        finally:
            timer.stop()
            session.closing = True
            session._detach()
            App.closeDocument(session.document.Name)
            App.setActiveDocument(self.doc.Name)

    def testBlocksCommandGroupAndStandaloneText(self):
        main = Gui.getMainWindow()
        text_action = Gui.Command.get("Sketcher_CreateText").getAction()[0]
        self.assertTrue(
            any(text_action in bar.actions() for bar in main.findChildren(QtWidgets.QToolBar))
        )
        group_actions = Gui.Command.get("Sketcher_CompBlocks").getAction()
        self.assertEqual(len(group_actions), 4)
        self.assertTrue(
            any(
                button.menu() and all(action in button.menu().actions() for action in group_actions)
                for bar in main.findChildren(QtWidgets.QToolBar)
                for button in bar.findChildren(QtWidgets.QToolButton)
            )
        )
        self.assertTrue(
            any(
                menu.title() == "Blocks"
                and all(action in menu.actions() for action in group_actions)
                for menu in main.findChildren(QtWidgets.QMenu)
            )
        )
        self.assertNotIn("Sketcher_CompCreateOutlines", Gui.listCommands())
        for command in (
            "Sketcher_InsertBlock",
            "Sketcher_CreateBlock",
            "Sketcher_EditBlock",
            "Sketcher_ReloadBlock",
        ):
            self.assertIn(command, Gui.listCommands())

    def testBlockLibraryFoldersAndPersistence(self):
        import json

        root = Path(self.directory.name) / "custom 'é"
        nested = root / "Electrical" / "Switches"
        nested.mkdir(parents=True)
        (root / "Empty").mkdir()
        source = nested / "CE.txt"
        source.write_text(
            "# Copied from sketcher.\ngeoList = [Part.Circle(App.Vector(0,0,0),App.Vector(0,0,1),3)]\n"
            "objectStr.addGeometry(geoList,False)\n",
            encoding="utf-8",
        )
        (nested / "ignored.svg").write_text("ignored", encoding="utf-8")
        self.library_preferences.SetString("Folders", "[]")
        self.library_preferences.SetString("LastFile", "")
        Gui.runCommand("Sketcher_InsertBlock", 0)
        self.flush_gui(100)
        main = Gui.getMainWindow()
        tree = self.visible_widget(QtWidgets.QTreeWidget, "blockLibraryTree")
        self.assertEqual(tree.topLevelItem(0).text(0), "Built-in Blocks")
        self.assertTrue(tree.findItems("CE", QtCore.Qt.MatchExactly | QtCore.Qt.MatchRecursive))
        button = self.visible_widget(QtWidgets.QPushButton, "addBlockFolder")
        preferences = App.ParamGet("User parameter:BaseApp/Preferences/Dialog")
        old_native = preferences.GetBool("DontUseNativeDialog")
        preferences.SetBool("DontUseNativeDialog", True)
        errors = []

        def choose():
            dialog = next(
                w
                for w in QtWidgets.QApplication.topLevelWidgets()
                if isinstance(w, QtWidgets.QFileDialog) and w.isVisible()
            )
            try:
                self.assertEqual(
                    Path(dialog.directory().absolutePath()),
                    Path(App.getResourceDir()) / "Mod/Sketcher/Blocks",
                )
                dialog.setDirectory(str(root))
                QtCore.QTimer.singleShot(150, dialog.accept)
            except Exception as error:
                errors.append(error)
                dialog.reject()

        try:
            self.when_popup_visible(QtWidgets.QFileDialog, choose)
            button.click()
        finally:
            preferences.SetBool("DontUseNativeDialog", old_native)
        self.assertFalse(errors, errors)
        self.assertEqual(
            json.loads(self.library_preferences.GetString("Folders")),
            [str(root).replace("\\", "/")],
        )
        custom = tree.topLevelItem(tree.topLevelItemCount() - 1)
        self.assertEqual(custom.text(0), root.name)
        electrical = next(
            custom.child(i)
            for i in range(custom.childCount())
            if custom.child(i).text(0) == "Electrical"
        )
        self.assertTrue(any(custom.child(i).text(0) == "Empty" for i in range(custom.childCount())))
        electrical.setExpanded(True)
        switches = electrical.child(0)
        switches.setExpanded(True)
        self.assertEqual(switches.childCount(), 1)
        block = switches.child(0)
        self.assertEqual(block.text(0), "CE")
        self.assertEqual(Path(block.data(0, QtCore.Qt.UserRole)), source)
        tree.setCurrentItem(block)
        self.assertEqual(Path(self.library_preferences.GetString("LastFile")), source)
        # Reopen the tool: custom roots and the selected nested file are restored.
        Gui.activeDocument().resetEdit()
        Gui.activeDocument().setEdit(self.sketch.Name)
        Gui.runCommand("Sketcher_InsertBlock", 0)
        tree = self.visible_widget(QtWidgets.QTreeWidget, "blockLibraryTree")
        self.assertEqual(Path(tree.currentItem().data(0, QtCore.Qt.UserRole)), source)
        added = nested / "New Block.txt"
        added.write_bytes(source.read_bytes())
        self.visible_widget(QtWidgets.QPushButton, "refreshBlockLibrary").click()
        self.assertTrue(
            tree.findItems("New Block", QtCore.Qt.MatchExactly | QtCore.Qt.MatchRecursive)
        )
        # Select and insert the nested block, despite its duplicate built-in name.
        self.visible_widget(QtWidgets.QCheckBox, "blockFixedSize").setChecked(True)
        self.visible_widget(QtWidgets.QCheckBox, "blockFixedOrientation").setChecked(True)
        view = Gui.activeDocument().activeView()
        viewport = view.graphicsView().viewport()
        point = self.viewport_to_qpoint(view, viewport, view.getPointOnScreen(App.Vector(3, 4, 0)))
        self.move(viewport, point)
        self.click(viewport, point)
        group = next(c for c in self.sketch.Constraints if c.Type == "Group")
        self.assertEqual(Path(group.File), source)
        # Removing a root affects preferences, never the files themselves.
        tree = self.visible_widget(QtWidgets.QTreeWidget, "blockLibraryTree")
        custom = tree.topLevelItem(tree.topLevelItemCount() - 1)
        tree.setCurrentItem(custom)
        self.visible_widget(QtWidgets.QPushButton, "removeBlockFolder").click()
        self.assertEqual(json.loads(self.library_preferences.GetString("Folders")), [])
        self.assertTrue(source.is_file())
        self.assertTrue(added.is_file())

    def testInsertBlockTool(self):
        self.assertIn("Sketcher_InsertBlock", Gui.listCommands())
        view = Gui.activeDocument().activeView()
        view.setCamera(
            "#Inventor V2.1 ascii\nOrthographicCamera { position 0 0 100 "
            "orientation 0 0 1 0 focalDistance 100 height 160 }"
        )
        self.flush_gui(100)
        Gui.runCommand("Sketcher_InsertBlock", 0)
        self.flush_gui(100)
        combos = [w for w in Gui.getMainWindow().findChildren(QtWidgets.QComboBox) if w.isVisible()]
        self.selectLibraryBlock("CE")
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
            "orientation 0 0 1 0 focalDistance 100 height 160 }"
        )
        self.flush_gui(100)
        Gui.runCommand("Sketcher_InsertBlock", 0)
        combos = [w for w in Gui.getMainWindow().findChildren(QtWidgets.QComboBox) if w.isVisible()]
        choose_file = self.visible_widget(QtWidgets.QPushButton, "chooseBlockFile")
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
            self.when_popup_visible(QtWidgets.QFileDialog, choose)
            choose_file.click()
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
        constraints = self.visible_widget(QtWidgets.QListWidget, "listWidgetConstraints")
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

        self.when_popup_visible(QtWidgets.QMenu, reload_action)
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

                self.when_popup_visible(QtWidgets.QMenu, inspect_menu)
                self.right_click(viewport, QtCore.QPoint(40, viewport.height() - 40))
                self.assertEqual(found, [expected])

    def checkCreateBlock(self, fixed_size):
        import Part

        self.prepareBlockSelection()
        # Anchors to the source sketch must not pull the exported geometry back
        # after its coordinates are translated to the chosen block origin.
        self.sketch.addConstraint(Sketcher.Constraint("DistanceX", 0, 3, 0.0))
        self.sketch.addConstraint(Sketcher.Constraint("Radius", 0, 5.0))
        self.doc.recompute()
        clipboard = QtWidgets.QApplication.clipboard()
        previous = clipboard.text()
        preferences = App.ParamGet("User parameter:BaseApp/Preferences/Dialog")
        old_native = preferences.GetBool("DontUseNativeDialog")
        preferences.SetBool("DontUseNativeDialog", True)
        path = Path(self.directory.name) / "created 'é.txt"
        errors = []
        before = [geo.Content for geo in self.sketch.Geometry]
        constraints_before = [str(c) for c in self.sketch.Constraints]
        view = Gui.activeDocument().activeView()
        view.setCamera(
            "#Inventor V2.1 ascii\nOrthographicCamera { position 0 0 100 "
            "orientation 0 0 1 0 focalDistance 100 height 160 }"
        )
        viewport = view.graphicsView().viewport()

        def choose_origin():
            point = self.viewport_to_qpoint(
                view, viewport, view.getPointOnScreen(App.Vector(3, 4, 0))
            )
            self.move(viewport, point)
            self.click(viewport, point)

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
                self.assertTrue(self.wait_until(action.isEnabled), "Create Block action is enabled")
                action.trigger()
            except Exception as error:
                errors.append(error)
            finally:
                if menu:
                    menu.close()

        try:
            clipboard.setText("Preserve the clipboard")
            self.when_popup_visible(QtWidgets.QMenu, create_from_menu)
            self.right_click(viewport, QtCore.QPoint(40, viewport.height() - 40))
            self.flush_gui(100)
            self.assertFalse(errors, errors)
            self.assertIsNone(QtWidgets.QApplication.activeModalWidget())
            widget = self.visible_widget(QtWidgets.QWidget, "CreateBlockWidget")
            self.assertIsNotNone(widget)
            box = widget.findChild(QtWidgets.QCheckBox, "createBlockFixedSize")
            self.assertFalse(box.isChecked())
            box.setChecked(fixed_size)
            self.assertFalse(path.exists())
            self.when_popup_visible(QtWidgets.QFileDialog, choose_save)
            choose_origin()
            self.flush_gui(100)
            self.assertFalse(errors, errors)
            self.assertFalse(widget.isVisible())
            geometry, constraints = SketcherBlock.read(path, with_constraints=True)
            self.assertEqual(len(geometry), 2)
            self.assertEqual(
                [Sketcher.GeometryFacade(geo).Construction for geo in geometry], [False, True]
            )
            self.assertAlmostEqual(geometry[0].Center.x, -3, delta=0.4)
            self.assertAlmostEqual(geometry[0].Center.y, -4, delta=0.4)
            self.assertAlmostEqual(geometry[1].StartPoint.x - geometry[0].Center.x, 10)
            self.assertEqual([c.Type for c in constraints], ["Radius"])
            self.assertEqual(SketcherBlock.metadata(path)["fixed_size"], fixed_size)
            self.assertEqual(clipboard.text(), "Preserve the clipboard")
            self.assertEqual([geo.Content for geo in self.sketch.Geometry], before)
            self.assertEqual([str(c) for c in self.sketch.Constraints], constraints_before)
            saved = path.read_bytes()

            # Cancelling the save dialog must leave the source and existing file intact.
            Gui.Selection.clearSelection()
            for edge in ("Edge1", "Edge2"):
                Gui.Selection.addSelection(self.sketch, edge)
            Gui.runCommand("Sketcher_CreateBlock", 0)
            self.when_popup_visible(
                QtWidgets.QFileDialog, lambda: QtWidgets.QApplication.activeModalWidget().reject()
            )
            choose_origin()
            self.assertEqual(path.read_bytes(), saved)
            self.assertEqual([geo.Content for geo in self.sketch.Geometry], before)

            # The insertion tool restores the file's default, including when a tool is reopened.
            import json

            self.library_preferences.SetString("Folders", json.dumps([str(path.parent)]))
            self.library_preferences.SetString("LastFile", str(path))
            Gui.runCommand("Sketcher_InsertBlock", 0)
            self.flush_gui(100)
            box = self.visible_widget(QtWidgets.QCheckBox, "blockFixedSize")
            self.assertEqual(box.isChecked(), fixed_size)
            self.visible_widget(QtWidgets.QCheckBox, "blockFixedOrientation").setChecked(True)

            def place(x, y):
                point = self.viewport_to_qpoint(
                    view, viewport, view.getPointOnScreen(App.Vector(x, y, 0))
                )
                self.move(viewport, point)
                self.click(viewport, point)

            place(30, 20)
            if not fixed_size:
                place(60, 20)
            group = next(c for c in self.sketch.Constraints if c.Type == "Group")
            handle = self.sketch.Geometry[group.First]
            placed_origin = App.Vector(handle.X, handle.Y, 0) if fixed_size else handle.StartPoint
            native_width = Part.makeCompound([g.toShape() for g in geometry]).BoundBox.XLength
            scale = 1 if fixed_size else handle.length() / native_width
            center = self.sketch.Geometry[group.Second].Center
            self.assertLess((center - (placed_origin + geometry[0].Center * scale)).Length, 1e-5)
            SketcherBlock.reload(
                self.sketch,
                next(i for i, c in enumerate(self.sketch.Constraints) if c.Type == "Group"),
            )
            group = next(c for c in self.sketch.Constraints if c.Type == "Group")
            self.assertLess((self.sketch.Geometry[group.Second].Center - center).Length, 1e-5)
            box.setChecked(True)
            self.selectLibraryBlock("CE")
            self.assertFalse(box.isChecked())
        finally:
            clipboard.setText(previous)
            preferences.SetBool("DontUseNativeDialog", old_native)

    def testCreateBlockSaveAndCancel(self):
        self.checkCreateBlock(False)

    def testCreateBlockFixedSizeDefault(self):
        self.checkCreateBlock(True)
