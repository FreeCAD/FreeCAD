# SPDX-License-Identifier: LGPL-2.1-or-later

import unittest

import FreeCAD

try:
    import FreeCADGui

    GUI_MODULE_AVAILABLE = True
except ImportError:
    FreeCADGui = None
    GUI_MODULE_AVAILABLE = False

try:
    from PySide import QtCore, QtGui

    QT_MODULE_AVAILABLE = True
except ImportError:
    QtCore = None
    QtGui = None
    QT_MODULE_AVAILABLE = False


def gui_available():
    if not GUI_MODULE_AVAILABLE:
        return False

    try:
        return FreeCADGui.getMainWindow() is not None
    except (AttributeError, RuntimeError):
        return False


class SketcherGuiTestCase(unittest.TestCase):
    def setUp(self):
        super().setUp()
        if not gui_available():
            self.skipTest("GUI not available")

    def tearDown(self):
        try:
            self.cleanup_gui_document(getattr(self, "doc", None))
        finally:
            super().tearDown()

    def pump(self, timeout_ms=50):
        if not QT_MODULE_AVAILABLE:
            return

        loop = QtCore.QEventLoop()
        QtCore.QTimer.singleShot(timeout_ms, loop.quit)
        loop.exec_()

    def flush_gui(self, timeout_ms=0):
        if not gui_available():
            return

        if QT_MODULE_AVAILABLE:
            QtGui.QApplication.processEvents()

        FreeCADGui.updateGui()

        if timeout_ms:
            self.pump(timeout_ms)

    def cleanup_gui_document(self, doc, timeout_ms=80):
        if not gui_available():
            return

        gui_doc = FreeCADGui.ActiveDocument
        if gui_doc is not None:
            gui_doc.resetEdit()
            self.flush_gui(timeout_ms)

        if FreeCADGui.Control.activeDialog() is not None:
            FreeCADGui.Control.closeDialog()
            self.flush_gui(timeout_ms)

        if doc is not None and doc.Name in FreeCAD.listDocuments():
            FreeCAD.closeDocument(doc.Name)
            self.flush_gui(timeout_ms)

    def wait_until(self, predicate, timeout_ms=1000, step_ms=50):
        remaining = timeout_ms
        while remaining > 0:
            if predicate():
                return True
            self.flush_gui(step_ms)
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

    def right_click(self, widget, pos):
        self.send_mouse(
            widget,
            QtCore.QEvent.MouseButtonPress,
            pos,
            QtCore.Qt.RightButton,
            QtCore.Qt.RightButton,
        )
        self.send_mouse(
            widget,
            QtCore.QEvent.MouseButtonRelease,
            pos,
            QtCore.Qt.RightButton,
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

    def clamp_to_widget(self, widget, pos, margin=10):
        rect = widget.rect()
        return QtCore.QPoint(
            max(margin, min(pos.x(), rect.right() - margin)),
            max(margin, min(pos.y(), rect.bottom() - margin)),
        )

    def device_pixel_ratio(self, widget):
        return widget.devicePixelRatioF()

    def viewport_to_qpoint(self, view, viewport, point):
        _, height = view.getSize()
        scale = self.device_pixel_ratio(viewport)
        x = int(round(point[0] / scale))
        y = int(round((height - point[1] - 1) / scale))
        return QtCore.QPoint(x, y)
