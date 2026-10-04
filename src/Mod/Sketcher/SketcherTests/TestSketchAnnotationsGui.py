# SPDX-License-Identifier: LGPL-2.1-or-later
import FreeCAD as App
import Part
from PySide import QtCore, QtGui
from pivy import coin
from SketcherTests.GuiTestCase import FreeCADGui as Gui, SketcherGuiTestCase


class TestSketchAnnotationsGui(SketcherGuiTestCase):
    def setUp(self):
        super().setUp()
        Gui.activateWorkbench("SketcherWorkbench")
        self.doc = App.newDocument("AnnotationGui")
        self.doc.UndoMode = 1
        self.s = self.doc.addObject("Sketcher::SketchObject", "Sketch")
        self.s.addGeometry(Part.LineSegment(App.Vector(0, 0, 0), App.Vector(30, 0, 0)))
        self.doc.recompute()
        self.text = self.s.addAnnotation(
            {"Type": "Text", "Html": "<b>Note α</b>", "Position": (0.0, 10.0, 0.0)}
        )
        self.leader = self.s.addAnnotation(
            {"Type": "Leader", "Points": [(5.0, 5.0, 0.0), (10.0, 10.0, 0.0)], "Construction": True}
        )
        self.flush_gui(200)

    def scene(self):
        search = coin.SoSearchAction()
        search.setName("SketchAnnotations")
        search.apply(Gui.activeDocument().activeView().getSceneGraph())
        self.assertIsNotNone(search.getPath())
        return search.getPath().getTail()

    def enter(self):
        Gui.activeDocument().setEdit(self.s.Name)
        self.flush_gui(150)
        self.tree = Gui.getMainWindow().findChild(QtGui.QListWidget, "sketchCosmetics")
        self.assertIsNotNone(self.tree)

    def testVisibilityAndTaskList(self):
        self.assertEqual(self.scene().getNumChildren(), 1)
        self.enter()
        self.assertEqual(self.scene().getNumChildren(), 2)
        self.assertEqual(self.tree.count(), 2)
        self.tree.setCurrentItem(self.tree.item(0))
        self.flush_gui(80)
        self.assertIn(
            "Annotation" + str(self.text), Gui.Selection.getSelectionEx()[0].SubElementNames
        )
        Gui.activeDocument().resetEdit()
        self.flush_gui(100)
        self.assertEqual(self.scene().getNumChildren(), 1)

    def text_editor(self):
        scene = Gui.activeDocument().activeView().graphicsView().scene()
        for item in scene.items():
            if (
                isinstance(item, QtGui.QGraphicsTextItem)
                and item.objectName() == "sketchTextEditor"
            ):
                return item
        return None

    def type_text(self, widget, text):
        for char in text:
            self.key_click(widget, ord(char.upper()), char)

    def testEditRichTextUndoAndDelete(self):
        self.enter()
        viewport = Gui.activeDocument().activeView().graphicsView().viewport()
        self.tree.itemDoubleClicked.emit(self.tree.item(0))
        self.flush_gui(150)
        self.assertIsNone(QtGui.QApplication.activeModalWidget(), "Text has no edit dialog")
        editor = self.text_editor()
        self.assertIsNotNone(editor, "Double-click edits the text in the view")
        self.assertIn("Note", editor.toPlainText())
        # The stored text is hidden while its editor is shown in its place.
        self.assertEqual(self.scene().getNumChildren(), 1)
        self.assertIsNotNone(Gui.getMainWindow().findChild(QtGui.QWidget, "sketchTextToolWidget"))
        self.type_text(viewport, "!")
        self.key_click(viewport, QtCore.Qt.Key_Escape)
        self.flush_gui(150)
        self.assertIsNone(self.text_editor())
        self.assertIn("Note α!", self.s.Annotations[0]["Html"])
        self.assertEqual(self.scene().getNumChildren(), 2)
        self.doc.undo()
        self.flush_gui(100)
        self.assertIn("Note", self.s.Annotations[0]["Html"])
        self.tree.setCurrentItem(self.tree.item(0))
        self.tree.setFocus()
        self.key_click(self.tree, QtCore.Qt.Key_Delete)
        self.assertEqual([a["Id"] for a in self.s.Annotations], [self.leader])
        self.doc.undo()
        self.flush_gui(100)
        self.assertEqual(len(self.s.Annotations), 2)

    def testHatchTaskAndRendering(self):
        indices = self.s.addGeometry(
            [
                Part.LineSegment(App.Vector(x, y, 0), App.Vector(u, v, 0))
                for x, y, u, v in ((0, 0, 20, 0), (20, 0, 20, 20), (20, 20, 0, 20), (0, 20, 0, 0))
            ]
        )
        self.s.addAnnotation(
            {
                "Type": "Hatch",
                "Boundary": [self.s.GeometryFacadeList[i].Id for i in indices],
                "Rotation": 45.0,
            }
        )
        self.enter()
        self.assertEqual(self.tree.count(), 3)
        self.assertEqual(self.scene().getNumChildren(), 3)
        view = Gui.activeDocument().activeView()
        view.viewTop()
        view.fitAll()
        self.flush_gui(250)

    def testNativeDrag(self):
        self.enter()
        view = Gui.activeDocument().activeView()
        view.setCamera(
            "#Inventor V2.1 ascii\nOrthographicCamera { position 10 10 100 orientation 0 0 1 0 focalDistance 100 nearDistance 0.001 farDistance 1000 height 40 }"
        )
        viewport = view.graphicsView().viewport()

        def drag():
            self.pump(QtGui.QApplication.doubleClickInterval() + 30)
            start = self.viewport_to_qpoint(
                view, viewport, view.getPointOnScreen(App.Vector(5.5, 5.5, 0))
            )
            end = start + QtCore.QPoint(30, -20)
            self.move(viewport, start)
            self.assertIn(
                "Annotation" + str(self.leader), Gui.Selection.getPreselection().SubElementNames
            )
            self.send_mouse(
                viewport,
                QtCore.QEvent.MouseButtonPress,
                start,
                QtCore.Qt.LeftButton,
                QtCore.Qt.LeftButton,
            )
            self.send_mouse(
                viewport, QtCore.QEvent.MouseMove, end, QtCore.Qt.NoButton, QtCore.Qt.LeftButton
            )
            self.send_mouse(
                viewport,
                QtCore.QEvent.MouseButtonRelease,
                end,
                QtCore.Qt.LeftButton,
                QtCore.Qt.NoButton,
            )
            self.flush_gui(100)

        original = self.s.Annotations[1]["Points"]
        drag()
        self.assertNotEqual(self.s.Annotations[1]["Points"], original)
        self.doc.undo()
        self.flush_gui(100)
        self.assertEqual(self.s.Annotations[1]["Points"], original)

    def testUnchangedTextEditLeavesNoChanges(self):
        self.enter()
        before = self.s.Annotations
        viewport = Gui.activeDocument().activeView().graphicsView().viewport()
        self.tree.itemDoubleClicked.emit(self.tree.item(0))
        self.flush_gui(150)
        self.assertIsNotNone(self.text_editor())
        self.key_click(viewport, QtCore.Qt.Key_Escape)
        self.flush_gui(150)
        self.assertEqual(self.s.Annotations, before)
        self.assertEqual(self.scene().getNumChildren(), 2)

    def testTextAndLeaderCreationCommands(self):
        self.enter()
        view = Gui.activeDocument().activeView()
        view.setCamera(
            "#Inventor V2.1 ascii\nOrthographicCamera { position 10 10 100 orientation 0 0 1 0 focalDistance 100 nearDistance 0.001 farDistance 1000 height 40 }"
        )
        viewport = view.graphicsView().viewport()

        def position(x, y):
            return self.viewport_to_qpoint(
                view, viewport, view.getPointOnScreen(App.Vector(x, y, 0))
            )

        Gui.runCommand("Sketcher_AnnotationText")
        self.flush_gui(100)
        widget = Gui.getMainWindow().findChild(QtGui.QWidget, "sketchTextToolWidget")
        self.assertIsNotNone(widget, "The text tool must show its tool widget")
        widget.findChild(QtGui.QWidget, "textHeight").setProperty("rawValue", 5.0)
        self.click(viewport, position(15, 15))
        self.flush_gui(100)
        self.assertIsNone(QtGui.QApplication.activeModalWidget(), "Text has no edit dialog")
        self.assertIsNotNone(self.text_editor(), "A click opens the in-view text editor")
        self.assertIsNotNone(
            Gui.getMainWindow().findChild(QtGui.QWidget, "sketchTextToolbar"),
            "The formatting toolbar shows above the text",
        )
        self.type_text(viewport, "Hi")
        # Clicking elsewhere in the view finishes the text.
        self.pump(QtGui.QApplication.doubleClickInterval() + 30)
        self.click(viewport, position(5, 25))
        self.flush_gui(150)
        self.assertIsNone(self.text_editor())
        self.assertEqual(len(self.s.Annotations), 3)
        text = self.s.Annotations[-1]
        self.assertEqual(text["Type"], "Text")
        self.assertIn("Hi", text["Html"])
        self.assertAlmostEqual(text["TextSize"], 5.0)
        self.assertEqual(self.scene().getNumChildren(), 3)
        self.key_click(viewport, QtCore.Qt.Key_Escape)
        self.flush_gui(100)
        Gui.runCommand("Sketcher_AnnotationLeader")
        self.flush_gui(100)
        widget = Gui.getMainWindow().findChild(QtGui.QWidget, "sketchLeaderToolWidget")
        self.assertIsNotNone(widget, "The leader tool must show its tool widget")
        style = widget.findChild(QtGui.QComboBox, "leaderArrowStyle")
        style.setCurrentIndex(style.findData("Filled arrow"))
        widget.findChild(QtGui.QWidget, "leaderArrowSize").setProperty("rawValue", 3.0)
        self.click(viewport, position(15, 15))
        self.move(viewport, position(25, 20))
        self.flush_gui(50)
        # The preview draws the leader with its arrowhead before it is created.
        self.assertEqual(self.scene().getNumChildren(), 4)
        self.click(viewport, position(25, 20))
        self.key_click(viewport, QtCore.Qt.Key_Return)
        self.flush_gui(200)
        self.assertEqual(QtGui.QApplication.activeModalWidget(), None, "No dialog may open")
        self.assertEqual(len(self.s.Annotations), 4)
        leader = self.s.Annotations[-1]
        self.assertEqual(leader["Type"], "Leader")
        self.assertEqual(leader["ArrowStyle"], "Filled arrow")
        self.assertAlmostEqual(leader["ArrowSize"], 3.0)
        self.assertEqual(len(leader["Points"]), 2)
        self.key_click(viewport, QtCore.Qt.Key_Escape)
        self.flush_gui(100)
        self.doc.undo()
        self.flush_gui(100)
        self.assertEqual(len(self.s.Annotations), 3)
        self.assertEqual(self.scene().getNumChildren(), 3)

    def testToggleConstructionOnSelectedCosmetics(self):
        self.enter()
        Gui.Selection.clearSelection()
        Gui.Selection.addSelection(self.s, "Annotation" + str(self.text))
        Gui.Selection.addSelection(self.s, "Annotation" + str(self.leader))
        Gui.runCommand("Sketcher_ToggleConstruction")
        self.flush_gui(100)
        values = {a["Id"]: a["Construction"] for a in self.s.Annotations}
        self.assertTrue(values[self.text])
        self.assertFalse(values[self.leader])
        self.assertEqual(self.s.GeometryFacadeList[0].Construction, False)
        self.doc.undo()
        values = {a["Id"]: a["Construction"] for a in self.s.Annotations}
        self.assertFalse(values[self.text])
        self.assertTrue(values[self.leader])

    def listAction(self, label, row=None):
        errors = []

        def choose():
            menu = QtGui.QApplication.activePopupWidget()
            try:
                self.assertIsNotNone(menu)
                action = next(a for a in menu.actions() if a.text() == label)
                self.assertTrue(action.isEnabled())
                action.trigger()
            except Exception as error:
                errors.append(str(error))
            finally:
                if menu:
                    menu.close()

        point = (
            self.tree.visualItemRect(self.tree.item(row)).center()
            if row is not None
            else QtCore.QPoint(20, self.tree.viewport().height() - 3)
        )
        QtCore.QTimer.singleShot(100, choose)
        event = QtGui.QContextMenuEvent(
            QtGui.QContextMenuEvent.Mouse, point, self.tree.viewport().mapToGlobal(point)
        )
        QtGui.QApplication.sendEvent(self.tree.viewport(), event)
        self.flush_gui(120)
        self.assertFalse(errors, errors)

    def testMouseSelectionVisibilityAndDelete(self):
        self.enter()
        row = self.tree.item(0)
        rect = self.tree.visualItemRect(row)
        self.click(self.tree.viewport(), QtCore.QPoint(90, rect.center().y()))
        self.flush_gui(100)
        self.assertTrue(row.isSelected())
        self.assertIn(
            "Annotation" + str(self.text), Gui.Selection.getSelectionEx()[0].SubElementNames
        )
        self.click(self.tree.viewport(), QtCore.QPoint(10, rect.center().y()))
        self.flush_gui(100)
        self.assertEqual(row.checkState(), QtCore.Qt.Unchecked)
        self.assertEqual(self.s.ViewObject.HiddenAnnotations, [self.text])
        self.assertEqual(self.scene().getNumChildren(), 1)
        self.click(self.tree.viewport(), QtCore.QPoint(10, rect.center().y()))
        self.flush_gui(100)
        self.assertEqual(row.checkState(), QtCore.Qt.Checked)
        self.assertEqual(self.scene().getNumChildren(), 2)
        # Select by mouse, then delete via the list's normal focus path.
        self.click(self.tree.viewport(), QtCore.QPoint(90, rect.center().y()))
        self.tree.setFocus()
        self.key_click(self.tree, QtCore.Qt.Key_Delete)
        self.flush_gui(100)
        self.assertEqual([a["Id"] for a in self.s.Annotations], [self.leader])
        self.doc.undo()
        self.flush_gui(100)
        self.assertEqual(self.tree.count(), 2)

    def testTaskboxPaddingMatchesElements(self):
        self.enter()
        elements = Gui.getMainWindow().findChild(QtGui.QListWidget, "listWidgetElements")
        self.assertEqual(self.tree.x(), elements.x())
        self.assertEqual(
            self.tree.parentWidget().width() - self.tree.geometry().right(),
            elements.parentWidget().width() - elements.geometry().right(),
        )

    def testMouseMultiSelectionAndDelete(self):
        self.enter()
        viewport = self.tree.viewport()
        first = self.tree.visualItemRect(self.tree.item(0))
        self.click(viewport, QtCore.QPoint(90, first.center().y()))
        second = self.tree.visualItemRect(self.tree.item(1))
        point = QtCore.QPoint(90, second.center().y())
        for kind, buttons in (
            (QtCore.QEvent.MouseButtonPress, QtCore.Qt.LeftButton),
            (QtCore.QEvent.MouseButtonRelease, QtCore.Qt.NoButton),
        ):
            event = QtGui.QMouseEvent(
                kind,
                point,
                viewport.mapToGlobal(point),
                QtCore.Qt.LeftButton,
                buttons,
                QtCore.Qt.ControlModifier,
            )
            QtGui.QApplication.sendEvent(viewport, event)
        self.flush_gui(120)
        self.assertEqual(len(self.tree.selectedItems()), 2)
        self.assertEqual(
            set(Gui.Selection.getSelectionEx()[0].SubElementNames),
            {"Annotation" + str(self.text), "Annotation" + str(self.leader)},
        )
        self.tree.setFocus()
        self.key_click(self.tree, QtCore.Qt.Key_Delete)
        self.flush_gui(120)
        self.assertEqual(self.s.Annotations, [])
        self.doc.undo()
        self.flush_gui(120)
        self.assertEqual(self.tree.count(), 2)

    def testCosmeticsToolbarAndVisibilityUndo(self):
        self.enter()
        toolbar = Gui.getMainWindow().findChild(QtGui.QToolBar, "Cosmetics")
        self.assertIsNotNone(toolbar)
        commands = [a for a in toolbar.actions() if not a.isSeparator()]
        self.assertEqual(len(commands), 3)
        self.assertTrue(all(not a.icon().isNull() for a in commands))
        for kind in ("Text", "Hatch", "Leader"):
            cursor = QtGui.QImage(":/icons/pointers/Sketcher_Pointer_Cosmetic" + kind + ".svg")
            self.assertFalse(cursor.isNull(), kind)
            self.assertTrue(
                any(
                    cursor.pixelColor(x, y).alpha() > 0
                    for x in range(34, 64)
                    for y in range(34, 64)
                ),
                kind,
            )
        self.assertTrue(all(not self.tree.item(i).icon().isNull() for i in range(2)))
        self.tree.item(0).setCheckState(QtCore.Qt.Unchecked)
        self.flush_gui(120)
        self.assertEqual(self.s.ViewObject.HiddenAnnotations, [self.text])
        self.assertEqual(self.scene().getNumChildren(), 1)
        self.doc.undo()
        self.flush_gui(120)
        self.assertEqual(self.tree.item(0).checkState(), QtCore.Qt.Checked)
        self.assertEqual(self.scene().getNumChildren(), 2)
        self.assertIsNone(Gui.getMainWindow().findChild(QtGui.QCheckBox, "showCosmetics"))
        self.listAction("Hide All")
        self.assertEqual(self.scene().getNumChildren(), 0)
        self.listAction("Show All")
        self.assertEqual(self.scene().getNumChildren(), 2)

    def testCosmeticFilters(self):
        self.enter()
        enabled = Gui.getMainWindow().findChild(QtGui.QCheckBox, "cosmeticFilterEnabled")
        button = Gui.getMainWindow().findChild(QtGui.QToolButton, "cosmeticFilterButton")
        enabled.setChecked(True)
        menu = button.menu()
        self.assertEqual(self.scene().getNumChildren(), 2)
        self.listAction("Hide All")
        self.assertEqual(self.s.ViewObject.HiddenAnnotations, [self.text, self.leader])
        self.listAction("Show All")
        self.assertFalse(self.tree.item(1).isHidden())
        menu.aboutToShow.emit()
        menu.findChild(QtGui.QAction, "cosmeticTypeFilter0").setChecked(False)
        self.assertTrue(self.tree.item(0).isHidden())
        enabled.setChecked(False)
        self.assertFalse(self.tree.item(0).isHidden())

    def testHatchDoesNotInterceptBoundaryPicking(self):
        indices = self.s.addGeometry(
            [
                Part.LineSegment(App.Vector(x, y, 0), App.Vector(u, v, 0))
                for x, y, u, v in (
                    (40, 0, 60, 0),
                    (60, 0, 60, 20),
                    (60, 20, 40, 20),
                    (40, 20, 40, 0),
                )
            ]
        )
        hatch = self.s.addAnnotation(
            {
                "Type": "Hatch",
                "Boundary": [self.s.GeometryFacadeList[i].Id for i in indices],
                "Rotation": 0,
                "Spacing": 2,
            }
        )
        self.doc.recompute()
        self.enter()
        view = Gui.activeDocument().activeView()
        view.setCamera(
            "#Inventor V2.1 ascii\nOrthographicCamera { position 50 10 100 orientation 0 0 1 0 focalDistance 100 nearDistance 0.001 farDistance 1000 height 40 }"
        )
        viewport = view.graphicsView().viewport()
        self.tree.setCurrentItem(self.tree.item(2))
        self.flush_gui(100)
        for point, expected in ((App.Vector(50, 10, 0), None), (App.Vector(60, 10, 0), "Edge3")):
            Gui.Selection.clearPreselection()
            self.move(
                viewport, self.viewport_to_qpoint(view, viewport, view.getPointOnScreen(point))
            )
            preselection = Gui.Selection.getPreselection()
            names = preselection.SubElementNames if preselection else []
            self.assertNotIn("Annotation" + str(hatch), names)
            if expected:
                self.assertIn(expected, names)
        Gui.activeDocument().resetEdit()
        self.flush_gui(150)
        # Ray-pick the cosmetic scene alone to verify the same policy outside edit.
        pick = coin.SoRayPickAction(coin.SbViewportRegion(800, 600))
        pick.setRay(coin.SbVec3f(50, 10, 100), coin.SbVec3f(0, 0, -1))
        pick.apply(self.scene())
        self.assertIsNone(pick.getPickedPoint())

    def square(self, x0, y0, size):
        corners = [(x0, y0), (x0 + size, y0), (x0 + size, y0 + size), (x0, y0 + size)]
        return self.s.addGeometry(
            [
                Part.LineSegment(App.Vector(*corners[i], 0), App.Vector(*corners[(i + 1) % 4], 0))
                for i in range(4)
            ]
        )

    def hatch_view(self):
        view = Gui.activeDocument().activeView()
        view.setCamera(
            "#Inventor V2.1 ascii\nOrthographicCamera { position 55 10 100 orientation 0 0 1 0 focalDistance 100 nearDistance 0.001 farDistance 1000 height 40 }"
        )
        return view, view.graphicsView().viewport()

    def click_sketch(self, view, viewport, x, y):
        # Keep clicks apart so consecutive picks are never taken for a double click.
        self.pump(QtGui.QApplication.doubleClickInterval() + 30)
        point = self.viewport_to_qpoint(view, viewport, view.getPointOnScreen(App.Vector(x, y, 0)))
        self.move(viewport, point)
        self.click(viewport, point)

    def selected_edges(self):
        names = []
        for selection in Gui.Selection.getSelectionEx():
            names += [n for n in selection.SubElementNames if n.startswith("Edge")]
        return sorted(names)

    def testHatchCreationHandler(self):
        outer = self.square(40, 0, 20)
        inner = self.square(45, 5, 10)
        dangling = self.s.addGeometry(
            Part.LineSegment(App.Vector(62, 15, 0), App.Vector(68, 15, 0))
        )
        self.doc.recompute()
        self.enter()
        view, viewport = self.hatch_view()
        Gui.runCommand("Sketcher_AnnotationHatch")
        self.flush_gui(100)
        widget = Gui.getMainWindow().findChild(QtGui.QWidget, "sketchHatchToolWidget")
        self.assertIsNotNone(widget, "The hatch tool must show its tool widget")
        self.assertIsNone(widget.findChild(QtGui.QLineEdit), "The tool widget has no name field")
        before = self.scene().getNumChildren()

        # One edge selects its whole loop and previews the hatch.
        self.click_sketch(view, viewport, 50, 0)
        self.assertEqual(self.selected_edges(), sorted("Edge%d" % (i + 1) for i in outer))
        self.assertEqual(self.scene().getNumChildren(), before + 1)

        # An open edge is refused with a warning and never stays selected.
        self.click_sketch(view, viewport, 65, 15)
        self.assertNotIn("Edge%d" % (dangling + 1), self.selected_edges())
        self.assertEqual(len(self.selected_edges()), 4)

        # The inner loop becomes a hole: only the frame is hatched.
        self.click_sketch(view, viewport, 50, 5)
        self.assertEqual(len(self.selected_edges()), 8)
        pattern = widget.findChild(QtGui.QComboBox, "hatchPattern")
        pattern.setCurrentIndex(pattern.findData("ANSI32"))
        self.flush_gui(50)

        self.key_click(viewport, QtCore.Qt.Key_Return)
        self.flush_gui(200)
        self.assertEqual(QtGui.QApplication.activeModalWidget(), None, "No dialog may open")
        hatch = self.s.Annotations[-1]
        self.assertEqual(hatch["Type"], "Hatch")
        self.assertEqual(hatch["Pattern"], "ANSI32")
        self.assertEqual(len(hatch["Boundary"]), 8)
        self.assertAlmostEqual(self.s.getAnnotationFace(hatch["Id"]).Area, 300.0)
        self.assertEqual(self.selected_edges(), [])
        self.key_click(viewport, QtCore.Qt.Key_Escape)
        self.flush_gui(100)
        self.doc.undo()
        self.assertEqual(len(self.s.Annotations), 2)

    def testHatchLoopToggleAndInvalidEnter(self):
        outer = self.square(40, 0, 20)
        self.doc.recompute()
        self.enter()
        view, viewport = self.hatch_view()
        Gui.runCommand("Sketcher_AnnotationHatch")
        self.flush_gui(100)
        # Enter without a loop warns instead of raising through the event callback.
        self.key_click(viewport, QtCore.Qt.Key_Return)
        self.assertEqual(len(self.s.Annotations), 2)
        self.click_sketch(view, viewport, 60, 10)
        self.assertEqual(len(self.selected_edges()), 4)
        self.click_sketch(view, viewport, 50, 20)
        self.assertEqual(self.selected_edges(), [])
        self.key_click(viewport, QtCore.Qt.Key_Escape)
        self.flush_gui(100)
        self.assertEqual(len(self.s.Annotations), 2)

    def testHatchEditorHasPatternChoice(self):
        refs = [self.s.GeometryFacadeList[i].Id for i in self.square(40, 0, 20)]
        hatch = self.s.addAnnotation({"Type": "Hatch", "Boundary": refs})
        self.doc.recompute()
        self.enter()
        errors = []

        def inspect():
            dialog = QtGui.QApplication.activeModalWidget()
            try:
                combo = dialog.findChild(QtGui.QComboBox, "annotationHatchPattern")
                self.assertIsNotNone(combo)
                labels = [b.text() for b in dialog.findChildren(QtGui.QPushButton)]
                self.assertFalse(any("boundary" in text.lower() for text in labels), labels)
                combo.setCurrentIndex(combo.findData("BRICK"))
                dialog.findChild(QtGui.QDialogButtonBox).button(QtGui.QDialogButtonBox.Ok).click()
            except Exception as error:
                errors.append(str(error))
                if dialog:
                    dialog.reject()

        QtCore.QTimer.singleShot(150, inspect)
        row = next(
            i
            for i in range(self.tree.count())
            if self.tree.item(i).data(QtCore.Qt.UserRole) == hatch
        )
        self.tree.itemDoubleClicked.emit(self.tree.item(row))
        self.assertFalse(errors, errors)
        self.assertEqual(self.s.Annotations[-1]["Pattern"], "BRICK")
