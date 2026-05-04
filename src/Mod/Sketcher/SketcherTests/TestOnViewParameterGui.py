# SPDX-License-Identifier: LGPL-2.1-or-later

import unittest

import FreeCAD

try:
    import FreeCADGui

    GUI_AVAILABLE = FreeCADGui.getMainWindow() is not None
except (ImportError, AttributeError):
    GUI_AVAILABLE = False

from PySide import QtCore, QtGui


class TestOnViewParameterGui(unittest.TestCase):
    KEYS = {
        "0": QtCore.Qt.Key_0,
        "1": QtCore.Qt.Key_1,
        "2": QtCore.Qt.Key_2,
        "3": QtCore.Qt.Key_3,
        "4": QtCore.Qt.Key_4,
        "5": QtCore.Qt.Key_5,
        "6": QtCore.Qt.Key_6,
        "7": QtCore.Qt.Key_7,
        "8": QtCore.Qt.Key_8,
        "9": QtCore.Qt.Key_9,
        ".": QtCore.Qt.Key_Period,
        "-": QtCore.Qt.Key_Minus,
        " ": QtCore.Qt.Key_Space,
    }

    def setUp(self):
        if not GUI_AVAILABLE:
            self.skipTest("GUI not available")

        FreeCADGui.activateWorkbench("SketcherWorkbench")
        self.doc = FreeCAD.newDocument("TestOnViewParameterGui")
        self.sketch = self.doc.addObject("Sketcher::SketchObject", "Sketch")
        self.doc.recompute()

    def tearDown(self):
        if not GUI_AVAILABLE:
            return

        gui_doc = FreeCADGui.ActiveDocument
        if gui_doc is not None:
            gui_doc.resetEdit()

        if self.doc.Name in FreeCAD.listDocuments():
            FreeCAD.closeDocument(self.doc.Name)

    def pump(self, timeout_ms=50):
        loop = QtCore.QEventLoop()
        QtCore.QTimer.singleShot(timeout_ms, loop.quit)
        loop.exec_()

    def wait_until(self, predicate, timeout_ms=1000, step_ms=50):
        remaining = timeout_ms
        while remaining > 0:
            if predicate():
                return True
            self.pump(step_ms)
            remaining -= step_ms
        return predicate()

    def send_mouse(self, widget, event_type, pos, button, buttons):
        global_pos = widget.mapToGlobal(pos)
        event = QtGui.QMouseEvent(
            event_type,
            pos,
            global_pos,
            button,
            buttons,
            QtCore.Qt.NoModifier,
        )
        QtGui.QApplication.sendEvent(widget, event)

    def click(self, widget, pos):
        self.send_mouse(
            widget,
            QtCore.QEvent.MouseButtonPress,
            pos,
            QtCore.Qt.LeftButton,
            QtCore.Qt.LeftButton,
        )
        self.send_mouse(
            widget,
            QtCore.QEvent.MouseButtonRelease,
            pos,
            QtCore.Qt.LeftButton,
            QtCore.Qt.NoButton,
        )
        self.pump(120)

    def move(self, widget, pos):
        self.send_mouse(
            widget,
            QtCore.QEvent.MouseMove,
            pos,
            QtCore.Qt.NoButton,
            QtCore.Qt.NoButton,
        )
        self.pump(80)

    def key_click(self, widget, key, text=""):
        press = QtGui.QKeyEvent(QtCore.QEvent.KeyPress, key, QtCore.Qt.NoModifier, text)
        release = QtGui.QKeyEvent(QtCore.QEvent.KeyRelease, key, QtCore.Qt.NoModifier, text)
        QtGui.QApplication.sendEvent(widget, press)
        QtGui.QApplication.sendEvent(widget, release)
        self.pump(60)

    def key_text(self, widget, text):
        for ch in text:
            self.key_click(widget, self.KEYS[ch], ch)

    def active_spinbox(self):
        widget = QtGui.QApplication.focusWidget()
        if isinstance(widget, QtGui.QAbstractSpinBox):
            return widget
        if isinstance(widget, QtGui.QLineEdit):
            parent = widget.parent()
            if isinstance(parent, QtGui.QAbstractSpinBox):
                return parent
        return None

    def visible_spinboxes(self):
        main_window = FreeCADGui.getMainWindow()
        return [
            spinbox
            for spinbox in main_window.findChildren(QtGui.QAbstractSpinBox)
            if spinbox.isVisible()
        ]

    @unittest.skipIf(not GUI_AVAILABLE, "GUI not available")
    def test_rectangle_ovp_enter_finishes_without_crash(self):
        """
        Reproduce the rectangle OVP acceptance flow from PR #29201 review:
        click first point, type width, Tab, type height, Enter.

        If the process survives, the rectangle should be created in the sketch.
        """

        FreeCADGui.ActiveDocument.setEdit(self.sketch.Name)
        self.pump(200)

        view = FreeCADGui.ActiveDocument.ActiveView
        view.viewTop()
        self.pump(100)

        viewport = view.graphicsView().viewport()
        first_point = QtCore.QPoint(*view.getPointOnScreen(FreeCAD.Vector(0, 0, 0)))
        self.assertTrue(
            viewport.rect().contains(first_point),
            f"Expected {first_point} to fall inside the sketch viewport {viewport.rect()}",
        )

        move_target = QtCore.QPoint(first_point.x() + 80, first_point.y() - 60)
        move_target.setX(max(10, min(move_target.x(), viewport.rect().right() - 10)))
        move_target.setY(max(10, min(move_target.y(), viewport.rect().bottom() - 10)))

        FreeCADGui.runCommand("Sketcher_CreateRectangle")
        self.pump(250)

        self.move(viewport, first_point)
        self.click(viewport, first_point)
        self.move(viewport, move_target)

        self.assertTrue(
            self.wait_until(lambda: len(self.visible_spinboxes()) == 2, timeout_ms=1000),
            "Expected the rectangle OVPs to become visible after the first click",
        )

        first_spinbox = self.active_spinbox()
        self.assertIsNotNone(first_spinbox, "Expected the first rectangle OVP to have focus")
        self.key_text(first_spinbox, "10")
        self.key_click(first_spinbox, QtCore.Qt.Key_Tab, "\t")

        self.assertTrue(
            self.wait_until(
                lambda: self.active_spinbox() is not None
                and self.active_spinbox() is not first_spinbox,
                timeout_ms=1000,
            ),
            "Expected Tab to move focus to the second rectangle OVP",
        )

        second_spinbox = self.active_spinbox()
        self.assertIsNotNone(second_spinbox, "Expected the second rectangle OVP to have focus")
        self.key_text(second_spinbox, "20")
        self.key_click(second_spinbox, QtCore.Qt.Key_Return, "\r")

        self.pump(500)

        self.assertGreaterEqual(
            self.sketch.GeometryCount,
            4,
            "Expected the rectangle to be created after accepting both OVPs",
        )
