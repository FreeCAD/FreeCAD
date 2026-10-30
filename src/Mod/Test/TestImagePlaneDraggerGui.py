# SPDX-License-Identifier: LGPL-2.1-or-later
"""Editing an image plane shows the transform dragger (#34003).

https://github.com/FreeCAD/FreeCAD/issues/34003

The image task panel and the dragger must stay in sync in both directions,
and Cancel must restore the original placement.

To run tests:
    FreeCAD -t TestImagePlaneDraggerGui
"""

import math
import time
import unittest

import FreeCAD as App

if App.GuiUp:
    import FreeCADGui as Gui
    from pivy import coin
    from PySide import QtCore, QtGui, QtWidgets

DEFAULT_EDIT_MODE = 0
TRANSFORM_EDIT_MODE = 1
START = App.Vector(10, 20, 0)


@unittest.skipIf(not App.GuiUp, "GUI tests require FreeCAD GUI mode")
class TestImagePlaneDraggerGui(unittest.TestCase):
    def setUp(self):
        self.doc = App.newDocument("ImagePlaneDragger34003")
        self.addCleanup(self._close_document)
        self.image = self.doc.addObject("Image::ImagePlane", "Image")
        self.image.XSize = 100
        self.image.YSize = 100
        self.image.Placement = App.Placement(START, App.Rotation())
        self.doc.recompute()
        self.gui_doc = Gui.getDocument(self.doc.Name)
        Gui.ActiveDocument = self.gui_doc
        self.view = self.gui_doc.activeView()
        self.original_navigation = self.view.getNavigationType()
        self.addCleanup(self.view.setNavigationType, self.original_navigation)
        self.view.setNavigationType("Gui::CADNavigationStyle")
        self.view.viewTop()
        self.view.fitAll()
        self.pump()

    def _close_document(self):
        if Gui.Control.activeDialog():
            Gui.Control.activeTaskDialog().reject()
            self.pump()
        if Gui.Control.activeDialog():
            Gui.Control.closeDialog()
        if self.gui_doc.getInEdit() is not None:
            self.gui_doc.resetEdit()
        App.closeDocument(self.doc.Name)
        self.pump(20)

    # ------------------------------------------------------------------ helpers

    def pump(self, milliseconds=60):
        app = QtWidgets.QApplication.instance()
        Gui.updateGui()
        app.processEvents()
        time.sleep(milliseconds / 1000.0)
        app.processEvents()

    def open_image_panel(self):
        self.assertTrue(self.gui_doc.setEdit(self.image.Name, DEFAULT_EDIT_MODE))
        self.pump()
        self.assertTrue(Gui.Control.activeDialog())
        panel = self.image_panel()
        self.assertIsNotNone(panel, "the image task panel is not shown")
        return panel

    @staticmethod
    def image_panel():
        # uic turns the "Gui::TaskImage" form name into "Gui__TaskImage"
        main_window = Gui.getMainWindow()
        for name in ("Gui__TaskImage", "Gui::TaskImage"):
            panel = main_window.findChild(QtWidgets.QWidget, name)
            if panel is not None:
                return panel
        return None

    @staticmethod
    def spin(panel, name):
        widget = panel.findChild(QtWidgets.QAbstractSpinBox, name)
        assert widget is not None, name
        return widget

    def manager(self):
        return self.view.getViewer().getSoRenderManager()

    def dragger(self):
        search = coin.SoSearchAction()
        search.setType(coin.SoType.fromName("SoTransformDragger"))
        search.setInterest(coin.SoSearchAction.FIRST)
        search.apply(self.manager().getSceneGraph())
        path = search.getPath()
        return path.getTail() if path else None

    @staticmethod
    def field_vector(node, name):
        field = node.getField(name)
        value = field.getValue() if hasattr(field, "getValue") else None
        if value is not None and hasattr(value, "getValue"):
            return App.Vector(*value.getValue())
        text = field.get()
        return App.Vector(*(float(v) for v in str(text).split()))

    def pick_types(self, x, y):
        pick = coin.SoRayPickAction(self.manager().getViewportRegion())
        pick.setPoint(coin.SbVec2s(int(x), int(y)))
        pick.setRadius(1.0)
        pick.apply(self.manager().getSceneGraph())
        picked = pick.getPickedPoint()
        if picked is None:
            return []
        path = coin.SoFullPath.fromSoPath(picked.getPath())
        return [str(path.getNode(i).getTypeId().getName()) for i in range(path.getLength())]

    def x_translator_point(self):
        """Ray-pick along the image X axis for the dragger's X translation arrow."""
        base = self.image.Placement.Base
        origin = self.view.getPointOnScreen(base)
        ahead = self.view.getPointOnScreen(base + App.Vector(10, 0, 0))
        dx, dy = ahead[0] - origin[0], ahead[1] - origin[1]
        length = math.hypot(dx, dy)
        dx, dy = dx / length, dy / length

        hits = []
        for r in range(4, 400):
            x, y = origin[0] + dx * r, origin[1] + dy * r
            types = self.pick_types(x, y)
            if (
                "SoLinearDragger" in types
                and "SoRotationDragger" not in types
                and "SoPlanarDragger" not in types
            ):
                hits.append(r)
        self.assertTrue(hits, "Could not ray-pick the X translation arrow of the dragger")

        # The farthest run of hits along +X is the X arrow (the Z arrow sits on the origin)
        run = [hits[-1]]
        for r in reversed(hits[:-1]):
            if r != run[-1] - 1:
                break
            run.append(r)
        r = run[len(run) // 2]

        size = self.manager().getViewportRegion().getViewportSizePixels()
        width, height = int(size[0]), int(size[1])
        viewport = self.view.graphicsView().viewport()
        sx = viewport.width() / width
        sy = viewport.height() / height
        point = QtCore.QPoint(
            round((origin[0] + dx * r) * sx), round((height - 1 - (origin[1] + dy * r)) * sy)
        )
        direction = QtCore.QPointF(dx * sx, -dy * sy)
        return point, direction

    def send_mouse(self, kind, point, button, buttons):
        viewport = self.view.graphicsView().viewport()
        event = QtGui.QMouseEvent(
            kind,
            QtCore.QPointF(point),
            QtCore.QPointF(viewport.mapToGlobal(point)),
            button,
            buttons,
            QtCore.Qt.NoModifier,
        )
        QtWidgets.QApplication.sendEvent(viewport, event)
        self.pump(15)

    def drag_x_arrow(self, pixels):
        start, direction = self.x_translator_point()
        left = QtCore.Qt.LeftButton
        self.send_mouse(QtCore.QEvent.MouseMove, start, QtCore.Qt.NoButton, QtCore.Qt.NoButton)
        self.send_mouse(QtCore.QEvent.MouseButtonPress, start, left, left)
        steps = 6
        for i in range(1, steps + 1):
            offset = QtCore.QPoint(
                round(direction.x() * pixels * i / steps), round(direction.y() * pixels * i / steps)
            )
            self.send_mouse(QtCore.QEvent.MouseMove, start + offset, QtCore.Qt.NoButton, left)
        end = start + QtCore.QPoint(round(direction.x() * pixels), round(direction.y() * pixels))
        self.send_mouse(QtCore.QEvent.MouseButtonRelease, end, left, QtCore.Qt.NoButton)
        self.pump()

    # -------------------------------------------------------------------- tests

    def test_dragger_moves_image_and_cancel_restores(self):
        panel = self.open_image_panel()
        dragger = self.dragger()
        self.assertIsNotNone(dragger, "editing an image plane should show the transform dragger")
        self.assertLess((self.field_vector(dragger, "translation") - START).Length, 1e-4)

        self.drag_x_arrow(80)

        moved = self.image.Placement
        self.assertGreater(moved.Base.x - START.x, 1.0, "dragging the X arrow should move the image")
        self.assertAlmostEqual(moved.Base.y, START.y, places=6)
        self.assertAlmostEqual(moved.Base.z, START.z, places=6)
        self.assertTrue(moved.Rotation.isSame(App.Rotation(), 1e-9))

        # dragger -> panel
        self.assertAlmostEqual(self.spin(panel, "spinBoxX").property("rawValue"), moved.Base.x, 6)
        self.assertAlmostEqual(self.spin(panel, "spinBoxY").property("rawValue"), START.y, 6)

        # panel -> dragger
        self.spin(panel, "spinBoxY").setProperty("rawValue", 35.0)
        self.pump()
        self.assertAlmostEqual(self.image.Placement.Base.y, 35.0, places=6)
        self.assertAlmostEqual(self.image.Placement.Base.x, moved.Base.x, places=6)
        position = self.field_vector(self.dragger(), "translation")
        self.assertLess((position - self.image.Placement.Base).Length, 1e-4)

        # Cancel restores the original placement and leaves edit mode
        Gui.Control.activeTaskDialog().reject()
        self.pump()
        self.assertFalse(Gui.Control.activeDialog())
        self.assertIsNone(self.gui_doc.getInEdit())
        self.assertIsNone(self.dragger())
        self.assertTrue(
            self.image.Placement.isSame(App.Placement(START, App.Rotation()), 1e-9),
            f"Cancel should restore the placement, got {self.image.Placement}",
        )

    def test_ok_keeps_dragged_placement(self):
        self.open_image_panel()
        self.drag_x_arrow(80)
        moved = self.image.Placement
        self.assertGreater(moved.Base.x - START.x, 1.0)

        Gui.Control.activeTaskDialog().accept()
        self.pump()
        self.assertFalse(Gui.Control.activeDialog())
        self.assertIsNone(self.gui_doc.getInEdit())
        self.assertTrue(self.image.Placement.isSame(moved, 1e-9))

    def test_calibration_and_size_still_work(self):
        panel = self.open_image_panel()
        calibration = panel.findChild(QtWidgets.QGroupBox, "groupBoxCalibration")
        start_scale = panel.findChild(QtWidgets.QPushButton, "pushButtonScale")
        cancel_scale = panel.findChild(QtWidgets.QPushButton, "pushButtonCancel")

        start_scale.click()
        self.pump()
        self.assertFalse(calibration.isHidden())
        self.assertIsNotNone(self.dragger())
        cancel_scale.click()
        self.pump()
        self.assertTrue(calibration.isHidden())

        self.spin(panel, "spinBoxWidth").setProperty("rawValue", 50.0)
        self.pump()
        self.assertAlmostEqual(self.image.XSize.Value, 50.0, places=6)

        Gui.Control.activeTaskDialog().reject()
        self.pump()
        self.assertAlmostEqual(self.image.XSize.Value, 100.0, places=6)
        self.assertTrue(self.image.Placement.isSame(App.Placement(START, App.Rotation()), 1e-9))

    def test_transform_mode_keeps_transform_panel(self):
        self.assertTrue(self.gui_doc.setEdit(self.image.Name, TRANSFORM_EDIT_MODE))
        self.pump()
        self.assertTrue(Gui.Control.activeDialog())
        self.assertIsNone(self.image_panel())
        self.assertIsNotNone(self.dragger())
        Gui.Control.activeTaskDialog().reject()
        self.pump()
        self.assertIsNone(self.gui_doc.getInEdit())
