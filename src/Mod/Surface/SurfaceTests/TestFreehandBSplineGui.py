# SPDX-License-Identifier: LGPL-2.1-or-later
"""Exercise the native editor through Qt events and its actual task controls."""

import unittest
import FreeCAD as App


@unittest.skipUnless(App.GuiUp, "Requires the FreeCAD GUI")
class TestFreehandBSplineGui(unittest.TestCase):
    def setUp(self):
        import FreeCADGui as Gui
        from PySide import QtCore, QtGui, QtWidgets

        self.Gui, self.Core, self.GuiQt = Gui, QtCore, QtGui
        self.Widgets = QtWidgets
        self.previousWorkbench = Gui.activeWorkbench().name()
        Gui.activateWorkbench("SurfaceWorkbench")
        self.doc = App.newDocument("CurveEditorTest")
        self.curve = self.doc.addObject("Surface::FreehandBSpline", "Curve")
        self.curve.Points = [
            App.Vector(0, 0, 0),
            App.Vector(10, 10, 0),
            App.Vector(20, 0, 0),
            App.Vector(30, 5, 0),
        ]
        self.doc.recompute()
        QtWidgets.QApplication.processEvents()
        self.view = Gui.getDocument(self.doc.Name).activeView()
        self.view.viewTop()
        self.view.fitAll()
        Gui.getDocument(self.doc.Name).setEdit(self.curve.Name)
        QtWidgets.QApplication.processEvents()
        self.panel = Gui.getMainWindow().findChild(QtWidgets.QWidget, "FreehandBSplineEditor")
        self.assertIsNotNone(self.panel)
        self.table = self.panel.findChild(QtWidgets.QTableWidget, "interpolationPoints")
        self.gl = max(
            (
                w
                for w in Gui.getMainWindow().findChildren(QtWidgets.QWidget)
                if w.metaObject().className() == "QOpenGLWidget"
            ),
            key=lambda w: w.width() * w.height(),
        )

    def tearDown(self):
        guiDoc = self.Gui.getDocument(self.doc.Name)
        if guiDoc.getInEdit():
            self.doc.abortTransaction()
            guiDoc.resetEdit()
        self.Core.QCoreApplication.sendPostedEvents(None, self.Core.QEvent.DeferredDelete)
        App.closeDocument(self.doc.Name)
        self.Gui.activateWorkbench(self.previousWorkbench)

    def screen(self, point):
        x, y = self.view.getPointOnScreen(point)
        ratio = self.gl.devicePixelRatioF()
        return self.Core.QPointF(x / ratio, self.gl.height() - y / ratio)

    def mouse(self, kind, pos, button=None, buttons=None, modifiers=None):
        qt = self.Core.Qt
        event = self.GuiQt.QMouseEvent(
            kind,
            pos,
            self.gl.mapToGlobal(pos.toPoint()),
            button or qt.NoButton,
            buttons or qt.NoButton,
            modifiers or qt.NoModifier,
        )
        self.Widgets.QApplication.sendEvent(self.gl, event)

    def click(self, pos, modifiers=None):
        qt, event = self.Core.Qt, self.Core.QEvent
        self.mouse(event.MouseButtonPress, pos, qt.LeftButton, qt.LeftButton, modifiers)
        self.mouse(event.MouseButtonRelease, pos, qt.LeftButton, modifiers=modifiers)

    def key(self, key, modifiers=None):
        event = self.GuiQt.QKeyEvent(
            self.Core.QEvent.KeyPress, key, modifiers or self.Core.Qt.NoModifier
        )
        self.Widgets.QApplication.sendEvent(self.gl, event)

    def drag(self, pos, delta, modifiers=None, key=None):
        qt, event = self.Core.Qt, self.Core.QEvent
        self.mouse(event.MouseButtonPress, pos, qt.LeftButton, qt.LeftButton)
        if key is not None:
            self.key(key)
        self.mouse(event.MouseMove, pos + delta, buttons=qt.LeftButton, modifiers=modifiers)
        self.mouse(event.MouseButtonRelease, pos + delta, qt.LeftButton)

    def button(self, name):
        return self.panel.findChild(self.Widgets.QPushButton, name)

    def finish(self, accept=False):
        role = self.Widgets.QDialogButtonBox.Ok if accept else self.Widgets.QDialogButtonBox.Cancel
        boxes = self.Gui.getMainWindow().findChildren(self.Widgets.QDialogButtonBox)
        button = next(
            box.button(role)
            for box in boxes
            if box.button(role) is not None and box.button(role).isVisible()
        )
        button.click()
        self.Widgets.QApplication.processEvents()

    def test_edit_colors_and_preselection(self):
        from pivy import coin

        prefs = App.ParamGet("User parameter:BaseApp/Preferences/View")
        keys = {
            "EditedEdgeColor": 0xEEEEEEFF,
            "ConstructionColor": 0x223344FF,
            "SelectionColor": 0x556677FF,
            "HighlightColor": 0x8899AAFF,
        }
        previous = {key: prefs.GetUnsigned(key, value) for key, value in keys.items()}
        try:
            for key, value in keys.items():
                prefs.SetUnsigned(key, value)
            self.table.selectRow(1)
            self.table.clearSelection()
            search = coin.SoSearchAction()
            search.setName("FreehandBSplineEditOverlay")
            search.apply(self.view.getSceneGraph())
            overlay = search.getPath().getTail()

            def color(child):
                return overlay.getChild(child).getChild(0).rgb[0].getPackedValue()

            self.assertEqual(color(1), keys["ConstructionColor"])
            self.assertEqual(color(4), keys["EditedEdgeColor"])
            self.assertEqual(color(5), keys["ConstructionColor"])
            self.mouse(self.Core.QEvent.MouseMove, self.screen(self.curve.Points[1]))
            self.assertEqual(color(5), keys["HighlightColor"])
            self.Widgets.QApplication.sendEvent(self.gl, self.Core.QEvent(self.Core.QEvent.Leave))
            self.assertEqual(color(5), keys["ConstructionColor"])
            self.table.selectRow(1)
            self.assertEqual(color(5), keys["SelectionColor"])
            self.mouse(
                self.Core.QEvent.MouseMove,
                self.screen((self.curve.Points[0] + self.curve.Points[1]) * 0.5),
            )
            self.assertEqual(color(1), keys["HighlightColor"])
            self.assertEqual(len(self.curve.Points), 4)
        finally:
            for key, value in previous.items():
                prefs.SetUnsigned(key, value)

    def test_single_unpickable_edit_curve_and_display_restore(self):
        from pivy import coin

        def find(name):
            search = coin.SoSearchAction()
            search.setSearchingAll(True)
            search.setName(name)
            search.apply(self.view.getSceneGraph())
            return search.getPath().getTail() if search.getPath() else None

        hidden = find("FreehandBSplineHiddenDisplay")
        self.assertIsNotNone(hidden)
        self.assertEqual(hidden.whichChild.getValue(), coin.SO_SWITCH_NONE)
        preview = find("FreehandBSplineDrawingPreview")
        self.assertEqual(preview.getChild(0).style.getValue(), coin.SoPickStyle.UNPICKABLE)
        vertices = preview.getChild(preview.getNumChildren() - 2)
        before = str(vertices.point.getValues())
        self.drag(self.screen(self.curve.Points[1]), self.Core.QPointF(12, -8))
        self.assertEqual(hidden.whichChild.getValue(), coin.SO_SWITCH_NONE)
        vertices = preview.getChild(preview.getNumChildren() - 2)
        self.assertNotEqual(str(vertices.point.getValues()), before)
        self.assertFalse(self.curve.Shape.isClosed())
        # The preview is one open line strip, with no extra endpoint-to-endpoint edge.
        self.assertEqual(preview.getChild(preview.getNumChildren() - 1).numVertices.getNum(), 1)
        self.assertNotEqual(
            tuple(vertices.point[0].getValue()),
            tuple(vertices.point[vertices.point.getNum() - 1].getValue()),
        )
        self.assertTrue(self.curve.ViewObject.Selectable)
        self.finish(True)
        self.Core.QCoreApplication.sendPostedEvents(None, self.Core.QEvent.DeferredDelete)
        self.assertIsNone(find("FreehandBSplineHiddenDisplay"))
        self.assertIsNone(find("FreehandBSplineDrawingPreview"))
        self.assertTrue(self.curve.ViewObject.Selectable)
        self.assertTrue(self.curve.ViewObject.Visibility)

    def test_tangent_choices_and_unlock(self):
        import Part

        support = self.doc.addObject("Part::Feature", "TangentWire")
        support.Shape = Part.makePolygon(
            [App.Vector(0, 0, 0), App.Vector(10, 0, 0), App.Vector(10, 10, 0)]
        )
        self.doc.recompute()
        self.assertEqual(self.table.horizontalHeaderItem(1).text(), "Tangent to")
        self.assertEqual(self.table.columnWidth(0), 112)
        self.assertIsNone(self.table.cellWidget(0, 1))
        self.table.item(0, 0).setText("TangentWire.Vertex2")
        combo = self.table.cellWidget(0, 1)
        self.assertEqual(
            [combo.itemText(i) for i in range(combo.count())], ["None", "Edge1", "Edge2"]
        )
        combo.setCurrentIndex(2)
        combo.activated.emit(2)
        self.assertEqual(self.curve.TangentPointIndices, [0])
        edge = self.curve.Shape.Edges[0]
        self.assertLess(edge.tangentAt(edge.FirstParameter).cross(App.Vector(0, 1, 0)).Length, 1e-7)
        self.table.item(0, 0).setText("")
        self.assertEqual(self.curve.TangentPointIndices, [])
        self.assertIsNone(self.table.cellWidget(0, 1))
        self.table.item(0, 0).setText("TangentWire.Edge1")
        combo = self.table.cellWidget(0, 1)
        self.assertEqual([combo.itemText(i) for i in range(combo.count())], ["None", "Edge1"])
        combo.setCurrentIndex(1)
        combo.activated.emit(1)
        self.table.selectRow(0)
        self.key(self.Core.Qt.Key_Delete)
        self.assertEqual(self.curve.TangentPointIndices, [])

    def test_drawing_clicks_lock_snapped_references(self):
        import Part

        self.finish()
        self.curve.ViewObject.Visibility = False
        support = self.doc.addObject("Part::Feature", "DrawingReference")
        support.Shape = Part.makePolygon(
            [App.Vector(-10, -8, 0), App.Vector(10, -8, 0), App.Vector(10, -20, 0)]
        )
        self.doc.recompute()
        self.Gui.runCommand("Surface_FreehandBSpline")
        self.Widgets.QApplication.processEvents()
        created = self.doc.getObject("FreehandBSpline")
        self.panel = self.Gui.getMainWindow().findChild(
            self.Widgets.QWidget, "FreehandBSplineEditor"
        )
        self.table = self.panel.findChild(self.Widgets.QTableWidget, "interpolationPoints")
        vertex = self.screen(App.Vector(10, -8, 0))
        self.mouse(self.Core.QEvent.MouseMove, vertex)
        self.assertEqual(created.SupportPointIndices, [])
        self.click(vertex)
        self.click(self.screen(App.Vector(0, -8, 0)))
        self.click(self.screen(App.Vector(25, 15, 0)))
        self.assertEqual(created.SupportPointIndices, [0, 1])
        self.assertEqual(
            [name for entry in created.Support for name in entry[1]], ["Vertex2", "Edge1"]
        )
        snapping = next(
            box
            for box in self.panel.findChildren(self.Widgets.QCheckBox)
            if box.text() == "Snap to vertices and edges"
        )
        snapping.setChecked(False)
        self.click(self.screen(App.Vector(10, -20, 0)))
        self.assertEqual(len(created.Points), 4)
        self.assertEqual(created.SupportPointIndices, [0, 1])
        self.button("endBSpline").click()
        self.assertEqual(self.table.item(0, 0).text(), "DrawingReference.Vertex2")
        self.assertEqual(self.table.item(1, 0).text(), "DrawingReference.Edge1")
        self.assertIsNotNone(self.table.cellWidget(0, 1))
        self.finish(True)
        support.Placement.Base = App.Vector(0, 0, 5)
        self.doc.recompute()
        self.assertAlmostEqual(created.Points[0].z, 5)
        self.assertAlmostEqual(created.Points[1].z, 5)
        self.assertAlmostEqual(created.Points[2].z, 0)

    def test_point_clicks_take_priority_over_coordinate_labels(self):
        for dx, dy in ((0, 0), (5, 3), (8, 5), (9, 0)):
            self.table.clearSelection()
            self.click(self.screen(self.curve.Points[1]) + self.Core.QPointF(dx, dy))
            self.assertEqual([row.row() for row in self.table.selectionModel().selectedRows()], [1])
            self.assertFalse(
                any(
                    w.isVisible()
                    for w in self.gl.findChildren(
                        self.Widgets.QAbstractSpinBox, "pointCoordinateEditor"
                    )
                )
            )
        before = self.curve.Points[1]
        self.drag(self.screen(before) + self.Core.QPointF(8, 4), self.Core.QPointF(20, -15))
        self.assertGreater((self.curve.Points[1] - before).Length, 0.1)

    def test_inline_coordinate_editing(self):
        point = 1

        def find_editor(axis):
            origin = self.screen(self.curve.Points[point])
            # Locate the rendered glyphs, accounting for Coin's font metrics and viewport scaling.
            for y in range(-24, 49, 2):
                for x in range(8, 37, 2):
                    pos = origin + self.Core.QPointF(x, y)
                    self.click(pos)
                    spin = next(
                        (
                            w
                            for w in self.gl.findChildren(self.Widgets.QAbstractSpinBox)
                            if w.objectName() == "pointCoordinateEditor" and w.isVisible()
                        ),
                        None,
                    )
                    if spin:
                        if spin.toolTip().endswith(axis):
                            return spin
                        event = self.GuiQt.QKeyEvent(
                            self.Core.QEvent.KeyPress,
                            self.Core.Qt.Key_Escape,
                            self.Core.Qt.NoModifier,
                        )
                        self.Widgets.QApplication.sendEvent(spin, event)
            self.fail("Could not click coordinate " + axis)

        spin = find_editor("X")
        self.assertTrue(self.gl.rect().contains(spin.geometry()))
        spin.findChild(self.Widgets.QLineEdit).setText("1.2 cm")
        self.Widgets.QApplication.sendEvent(
            spin,
            self.GuiQt.QKeyEvent(
                self.Core.QEvent.KeyPress, self.Core.Qt.Key_Return, self.Core.Qt.NoModifier
            ),
        )
        self.assertAlmostEqual(self.curve.Points[point].x, 12)
        spin = find_editor("Y")
        original = self.curve.Points[point]
        spin.setProperty("rawValue", 99.0)
        self.Widgets.QApplication.sendEvent(
            spin,
            self.GuiQt.QKeyEvent(
                self.Core.QEvent.KeyPress, self.Core.Qt.Key_Escape, self.Core.Qt.NoModifier
            ),
        )
        self.assertEqual(self.curve.Points[point], original)
        spin = find_editor("Z")
        spin.setProperty("rawValue", 4.0)
        self.Widgets.QApplication.sendEvent(
            spin,
            self.GuiQt.QKeyEvent(
                self.Core.QEvent.KeyPress, self.Core.Qt.Key_Return, self.Core.Qt.NoModifier
            ),
        )
        self.assertAlmostEqual(self.curve.Points[point].z, 4)
        self.finish()
        self.Core.QCoreApplication.sendPostedEvents(None, self.Core.QEvent.DeferredDelete)
        self.assertIsNone(self.gl.findChild(self.Widgets.QAbstractSpinBox, "pointCoordinateEditor"))

    def test_edit_after_deleting_body_reference(self):
        import Part

        self.finish()
        body = self.doc.addObject("PartDesign::Body", "SupportBody")
        solid = body.newObject("PartDesign::Feature", "Solid")
        solid.Shape = Part.makeBox(4, 4, 4)
        body.Tip = solid
        self.doc.recompute()
        self.curve.Support = [(body, ("Vertex1",))]
        self.curve.SupportPointIndices = [0]
        self.curve.SupportParameters = [App.Vector()]
        self.doc.recompute()
        before = list(self.curve.Points)
        self.doc.removeObject(body.Name)
        self.doc.recompute()
        self.assertTrue(self.curve.isValid(), self.curve.getStatusString())
        self.assertEqual(self.curve.SupportPointIndices, [])
        self.assertEqual(self.curve.Points, before)
        # Reproduce already-corrupted metadata from a file created before this fix.
        self.curve.SupportPointIndices = [0]
        self.curve.SupportParameters = [App.Vector()]
        self.Gui.getDocument(self.doc.Name).setEdit(self.curve.Name)
        self.Widgets.QApplication.processEvents()
        self.panel = self.Gui.getMainWindow().findChild(
            self.Widgets.QWidget, "FreehandBSplineEditor"
        )
        self.table = self.panel.findChild(self.Widgets.QTableWidget, "interpolationPoints")
        self.assertEqual(self.table.item(0, 0).text(), "")
        self.assertEqual(self.curve.SupportPointIndices, [])
        self.drag(self.screen(self.curve.Points[1]), self.Core.QPointF(15, -12))
        self.finish(True)
        self.assertIsNone(self.Gui.getDocument(self.doc.Name).getInEdit())
        self.assertTrue(self.curve.ViewObject.Visibility)
        self.assertTrue(self.curve.isValid(), self.curve.getStatusString())

    def test_create_accept_undo_redo_and_cancel(self):
        self.finish()
        self.Gui.runCommand("Surface_FreehandBSpline")
        self.Widgets.QApplication.processEvents()
        created = self.doc.getObject("FreehandBSpline")
        self.assertIsNotNone(created)
        for p in (App.Vector(0, 1, 0), App.Vector(8, 3, 0), App.Vector(14, 1, 0)):
            self.click(self.screen(p))
        self.assertEqual(len(created.Points), 3)
        self.finish(True)
        self.assertTrue(created.Shape.isValid())
        self.doc.undo()
        self.assertIsNone(self.doc.getObject("FreehandBSpline"))
        self.doc.redo()
        self.assertTrue(self.doc.FreehandBSpline.Shape.isValid())
        self.Gui.runCommand("Surface_FreehandBSpline")
        self.Widgets.QApplication.processEvents()
        self.assertIsNotNone(self.doc.getObject("FreehandBSpline001"))
        self.finish()
        self.assertIsNone(self.doc.getObject("FreehandBSpline001"))

    def test_drawing_preview_and_transition_to_editing(self):
        from pivy import coin

        self.finish()
        qt, event = self.Core.Qt, self.Core.QEvent
        for ending in ("button", "escape", "right"):
            self.Gui.runCommand("Surface_FreehandBSpline")
            self.Widgets.QApplication.processEvents()
            created = self.doc.getObject("FreehandBSpline")
            self.panel = self.Gui.getMainWindow().findChild(
                self.Widgets.QWidget, "FreehandBSplineEditor"
            )
            self.table = self.panel.findChild(self.Widgets.QTableWidget, "interpolationPoints")
            self.assertFalse(self.table.isEnabled())
            positions = [self.screen(App.Vector(x, y, 0)) for x, y in ((0, 1), (8, 7), (14, 1))]
            for pos in positions:
                self.click(pos)
            original = list(created.Points)
            shape = created.Shape.copy()
            self.mouse(event.MouseMove, self.screen(App.Vector(19, 9, 0)))
            self.assertEqual(created.Points, original)
            self.assertAlmostEqual(created.Shape.Length, shape.Length)
            search = coin.SoSearchAction()
            search.setName("FreehandBSplineDrawingPreview")
            search.apply(self.view.getSceneGraph())
            self.assertIsNotNone(search.getPath())
            preview = search.getPath().getTail()
            self.assertGreater(preview.getNumChildren(), 0)
            # A second pointer move changes the curve preview, not its committed points.
            vertices = preview.getChild(preview.getNumChildren() - 2)
            before = str(vertices.point.getValues())
            self.mouse(event.MouseMove, self.screen(App.Vector(19, -6, 0)))
            vertices = preview.getChild(preview.getNumChildren() - 2)
            self.assertNotEqual(str(vertices.point.getValues()), before)
            if ending == "button":
                self.button("endBSpline").click()
            elif ending == "escape":
                self.key(qt.Key_Escape)
            else:
                self.mouse(event.MouseButtonPress, positions[-1], qt.RightButton, qt.RightButton)
                self.mouse(event.MouseButtonRelease, positions[-1], qt.RightButton)
            self.assertIsNotNone(self.Gui.getDocument(self.doc.Name).getInEdit())
            self.assertTrue(self.table.isEnabled())
            self.assertGreater(preview.getNumChildren(), 0)
            self.click(self.screen(App.Vector(25, -8, 0)))
            self.assertEqual(created.Points, original)
            edge = created.Shape.Edges[0]
            point = edge.valueAt(
                edge.FirstParameter + (edge.LastParameter - edge.FirstParameter) * 0.3
            )
            pos = self.screen(point)
            # Real Qt double-click sequence includes the initial press and release.
            self.click(pos)
            self.mouse(event.MouseButtonDblClick, pos, qt.LeftButton, qt.LeftButton)
            self.mouse(event.MouseButtonRelease, pos, qt.LeftButton)
            self.assertEqual(len(created.Points), 4)
            self.key(qt.Key_Delete)
            self.assertEqual(len(created.Points), 3)
            self.finish()
            self.assertIsNone(self.doc.getObject("FreehandBSpline"))

    def test_point_reference_lock_and_delete_shortcut(self):
        import Part

        qt = self.Core.Qt
        support = self.doc.addObject("Part::Feature", "SupportEdge")
        support.Shape = Part.makeLine(App.Vector(-10, 3, 0), App.Vector(40, 3, 0))
        self.doc.recompute()
        self.click(self.screen(self.curve.Points[1]))
        self.Gui.Selection.clearSelection()
        self.Gui.Selection.addSelection(support, "Edge1")
        self.button("lockPoint").click()
        self.assertEqual(self.curve.SupportPointIndices, [1])
        self.assertAlmostEqual(self.curve.Points[1].y, 3)
        self.assertEqual(self.table.item(1, 0).text(), "SupportEdge.Edge1")
        self.assertEqual(self.button("lockPoint").text(), "Unlock")
        self.button("lockPoint").click()
        self.assertEqual(self.curve.SupportPointIndices, [])
        self.assertEqual(self.table.item(1, 0).text(), "")
        self.assertEqual(self.button("lockPoint").text(), "Lock to")
        self.button("lockPoint").click()
        self.assertEqual(self.curve.SupportPointIndices, [1])
        self.table.selectRow(2)
        self.assertEqual(self.button("lockPoint").text(), "Lock to")
        self.table.selectRow(1)
        self.assertEqual(self.button("lockPoint").text(), "Unlock")
        edge = self.curve.Shape.Edges[0]
        point = edge.valueAt(edge.FirstParameter + (edge.LastParameter - edge.FirstParameter) * 0.1)
        self.mouse(
            self.Core.QEvent.MouseButtonDblClick, self.screen(point), qt.LeftButton, qt.LeftButton
        )
        self.assertEqual(self.curve.SupportPointIndices, [2])
        self.key(qt.Key_Delete)
        self.assertEqual(self.curve.SupportPointIndices, [1])
        self.drag(self.screen(self.curve.Points[1]), self.Core.QPointF(25, 35))
        self.assertAlmostEqual(self.curve.Points[1].y, 3)
        fraction = self.curve.SupportParameters[0].x
        support.Placement.Base = App.Vector(0, 4, 0)
        self.doc.recompute()
        self.assertAlmostEqual(self.curve.Points[1].y, 7)
        self.assertAlmostEqual(self.curve.SupportParameters[0].x, fraction)
        # Clear the reference through the actual editable table cell.
        self.table.item(1, 0).setText("")
        self.assertEqual(self.curve.SupportPointIndices, [])
        self.table.item(1, 0).setText("SupportEdge")
        self.assertEqual(self.table.item(1, 0).text(), "SupportEdge")
        self.assertEqual(self.curve.SupportPointIndices, [1])
        self.table.item(1, 0).setText("")
        # Lock through a typed reference, then remove both segment endpoints with Delete.
        self.table.item(0, 0).setText("SupportEdge.Vertex1")
        self.assertEqual(self.curve.SupportPointIndices, [0])
        self.click((self.screen(self.curve.Points[0]) + self.screen(self.curve.Points[1])) / 2)
        before = self.curve.Points[2:]
        override = self.GuiQt.QKeyEvent(
            self.Core.QEvent.ShortcutOverride, qt.Key_Delete, qt.NoModifier
        )
        self.Widgets.QApplication.sendEvent(self.gl, override)
        self.assertTrue(override.isAccepted())
        self.key(qt.Key_Delete)
        self.assertEqual(self.curve.Points, before)
        self.assertEqual(self.curve.SupportPointIndices, [])
        self.assertIsNotNone(self.doc.getObject(self.curve.Name))

    def test_native_reference_selection_and_balanced_mouse_events(self):
        import Part

        qt, event = self.Core.Qt, self.Core.QEvent
        support = self.doc.addObject("Part::Feature", "PickReference")
        support.Shape = Part.makeLine(App.Vector(-10, -8, 0), App.Vector(40, -8, 0))
        self.doc.recompute()
        self.Widgets.QApplication.processEvents()
        self.view.fitAll()
        loop = self.Core.QEventLoop()
        self.Core.QTimer.singleShot(600, loop.quit)
        loop.exec()
        self.view.redraw()
        self.Widgets.QApplication.processEvents()
        self.table.selectRow(1)

        class MouseObserver(self.Core.QObject):
            def __init__(self):
                super().__init__()
                self.events = []

            def eventFilter(self, watched, received):
                if received.type() in (event.MouseButtonPress, event.MouseButtonRelease):
                    self.events.append(received.type())
                return False

        observer = MouseObserver()
        self.gl.installEventFilter(observer)
        try:
            # Locate the rendered edge, including viewport scaling and camera fitting.
            x, y = self.view.getPointOnScreen(App.Vector(5, -8, 0))
            hits = [
                candidate
                for candidate in range(y - 40, y + 41)
                if (self.view.getObjectInfo((x, candidate)) or {}).get("Object") == support.Name
            ]
            hitY = hits[len(hits) // 2]
            ratio = self.gl.devicePixelRatioF()
            pos = self.Core.QPointF(x / ratio, self.gl.height() - hitY / ratio)
            self.mouse(event.MouseMove, pos)
            self.click(pos)
            self.Widgets.QApplication.processEvents()
            self.assertEqual(observer.events, [event.MouseButtonPress, event.MouseButtonRelease])
            selected = self.Gui.Selection.getSelectionEx()
            self.assertEqual(len(selected), 1)
            self.assertEqual(selected[0].ObjectName, support.Name)
            self.assertEqual(selected[0].SubElementNames, ("Edge1",))
            self.assertEqual(len(self.table.selectionModel().selectedRows()), 1)
            self.button("lockPoint").click()
            self.assertEqual(self.curve.SupportPointIndices, [1])
            # An empty-space click deselects the spline point and reaches native selection.
            empty = self.Core.QPointF(35, self.gl.height() - 35)
            self.mouse(event.MouseMove, empty)
            self.click(empty)
            self.assertEqual(len(self.table.selectionModel().selectedRows()), 0)
            self.assertEqual(len(observer.events), 4)
            self.finish(True)
            self.mouse(event.MouseMove, empty + self.Core.QPointF(40, -25))
            self.assertFalse(
                any(
                    w.isVisible()
                    for w in self.Gui.getMainWindow().findChildren(self.Widgets.QRubberBand)
                )
            )
        finally:
            self.gl.removeEventFilter(observer)

    def test_tools_follow_selection_validity(self):
        import Part

        self.assertFalse(self.button("alignSelection").isEnabled())
        self.assertFalse(self.button("lockPoint").isEnabled())
        self.assertFalse(self.button("toggleInterpolation").isEnabled())
        self.assertTrue(self.button("selectAll").isEnabled())
        self.table.selectRow(1)
        self.assertFalse(self.button("lockPoint").isEnabled())
        reference = self.doc.addObject("Part::Feature", "Reference")
        reference.Shape = Part.makeLine(App.Vector(0, 3, 0), App.Vector(30, 3, 0))
        self.doc.recompute()
        self.Gui.Selection.addSelection(reference, "Edge1")
        self.assertTrue(self.button("lockPoint").isEnabled())
        self.Gui.Selection.clearSelection()
        self.assertFalse(self.button("lockPoint").isEnabled())
        self.table.selectAll()
        self.assertTrue(self.button("alignSelection").isEnabled())
        self.assertFalse(self.button("selectAll").isEnabled())
        self.assertFalse(self.button("lockPoint").isEnabled())
        self.assertFalse(self.button("toggleInterpolation").isEnabled())
        self.table.clearSelection()
        self.assertFalse(self.button("alignSelection").isEnabled())
        self.assertTrue(self.button("selectAll").isEnabled())

    def test_face_lock_slide_and_cycle_rejection(self):
        import Part

        support = self.doc.addObject("Part::Feature", "SupportFace")
        support.Shape = Part.makePlane(40, 30, App.Vector(-5, -5, 4))
        self.doc.recompute()
        self.table.item(1, 0).setText("SupportFace.Face1")
        self.assertAlmostEqual(self.curve.Points[1].z, 4)
        self.table.item(1, 2).setText("100")
        self.assertAlmostEqual(self.curve.Points[1].x, 35)
        self.table.item(1, 4).setText("20")
        self.assertAlmostEqual(self.curve.Points[1].z, 4)
        support.Placement.Base = App.Vector(0, 0, 6)
        self.doc.recompute()
        self.assertAlmostEqual(self.curve.Points[1].z, 10)
        self.table.item(1, 0).setText(self.curve.Name + ".Edge1")
        self.assertEqual(self.table.item(1, 0).text(), "SupportFace.Face1")
        self.assertEqual(self.curve.SupportPointIndices, [1])

    def test_drag_constraints_selection_and_insertion(self):
        qt = self.Core.Qt
        self.panel.findChild(self.Widgets.QCheckBox, "snapping").setChecked(False)
        before = self.curve.Points
        self.drag(self.screen(before[0]), self.Core.QPointF(30, -15), key=qt.Key_X)
        self.assertGreater(abs(self.curve.Points[0].x - before[0].x), 0.1)
        self.assertAlmostEqual(self.curve.Points[0].y, before[0].y, places=5)
        before = self.curve.Points
        self.drag(self.screen(before[0]), self.Core.QPointF(20, 0))
        normal = (self.curve.Points[0] - before[0]).Length
        before = self.curve.Points
        self.drag(self.screen(before[0]), self.Core.QPointF(20, 0), qt.ShiftModifier)
        self.assertAlmostEqual((self.curve.Points[0] - before[0]).Length / normal, 0.1, places=2)
        before = self.curve.Points
        self.click(self.screen(before[0]))
        self.click(self.screen(before[1]), qt.ControlModifier)
        self.drag(self.screen(before[0]), self.Core.QPointF(0, 20))
        self.assertLess(
            ((self.curve.Points[0] - before[0]) - (self.curve.Points[1] - before[1])).Length, 1e-5
        )
        before = self.curve.Points
        self.drag((self.screen(before[2]) + self.screen(before[3])) / 2, self.Core.QPointF(0, 20))
        self.assertGreater((self.curve.Points[2] - before[2]).Length, 0.1)
        self.assertLess(
            ((self.curve.Points[2] - before[2]) - (self.curve.Points[3] - before[3])).Length, 1e-5
        )
        edge = self.curve.Shape.Edges[0]
        p = edge.valueAt(edge.FirstParameter + (edge.LastParameter - edge.FirstParameter) * 0.13)
        self.mouse(
            self.Core.QEvent.MouseButtonDblClick, self.screen(p), qt.LeftButton, qt.LeftButton
        )
        self.assertEqual(len(self.curve.Points), 5)
        self.key(qt.Key_Delete)
        self.assertEqual(len(self.curve.Points), 4)
        self.key(qt.Key_A, qt.ControlModifier)
        self.assertEqual(len(self.table.selectionModel().selectedRows()), 4)

    def test_snapping_alignment_and_linear_controls(self):
        import Part

        support = self.doc.addObject("Part::Feature", "SnapVertex")
        target = self.curve.Points[0] + App.Vector(2, 1, 3)
        support.Shape = Part.Vertex(target)
        start = self.screen(self.curve.Points[0])
        self.drag(start, self.screen(target) - start + self.Core.QPointF(3, 2))
        self.assertLess((self.curve.Points[0] - target).Length, 1e-6)
        line = self.doc.addObject("Part::Feature", "SnapEdge")
        target = target + App.Vector(0, 2, 2)
        line.Shape = Part.makeLine(target - App.Vector(5, 0, 0), target + App.Vector(5, 0, 0))
        start = self.screen(self.curve.Points[0])
        self.drag(start, self.screen(target) - start + self.Core.QPointF(1, 2))
        self.assertLess(line.Shape.distToShape(Part.Vertex(self.curve.Points[0]))[0], 1e-6)
        self.table.selectAll()
        self.button("alignSelection").click()
        axis = Part.makeLine(self.curve.Points[0], self.curve.Points[-1])
        for p in self.curve.Points:
            self.assertLess(axis.distToShape(Part.Vertex(p))[0], 1e-5)
        self.assertFalse(self.button("toggleInterpolation").isEnabled())
        for index in range(len(self.curve.Points) - 1):
            midpoint = (
                self.screen(self.curve.Points[index]) + self.screen(self.curve.Points[index + 1])
            ) / 2
            self.click(midpoint, self.Core.Qt.ControlModifier if index else None)
        self.assertTrue(self.button("toggleInterpolation").isEnabled())
        self.button("toggleInterpolation").click()
        self.assertTrue(all(self.curve.LinearSegments))
        self.assertEqual(self.button("toggleInterpolation").text(), "Set smooth interpolation")
        self.button("toggleInterpolation").click()
        self.assertFalse(any(self.curve.LinearSegments))
        self.assertEqual(self.button("toggleInterpolation").text(), "Set linear interpolation")
        self.assertIsNone(self.button("deletePoints"))
        self.assertEqual(self.table.horizontalHeaderItem(0).text(), "Locked to")
        self.assertGreater(self.table.columnWidth(0), self.table.columnWidth(1))

    def test_escape_and_cancel_restore_edits(self):
        qt = self.Core.Qt
        before = self.curve.Points
        pos = self.screen(before[0])
        self.mouse(self.Core.QEvent.MouseButtonPress, pos, qt.LeftButton, qt.LeftButton)
        self.mouse(
            self.Core.QEvent.MouseMove, pos + self.Core.QPointF(30, 10), buttons=qt.LeftButton
        )
        self.key(qt.Key_Escape)
        for a, b in zip(before, self.curve.Points):
            self.assertLess((a - b).Length, 1e-6)
        self.table.selectRow(1)
        self.key(qt.Key_Delete)
        self.assertEqual(len(self.curve.Points), 3)
        self.finish()
        self.assertEqual(len(self.curve.Points), 4)
        self.assertIsNone(self.Gui.getDocument(self.doc.Name).getInEdit())
