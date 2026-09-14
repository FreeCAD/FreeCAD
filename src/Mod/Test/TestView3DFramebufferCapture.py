# SPDX-License-Identifier: LGPL-2.1-or-later

"""GUI contract tests for 3D viewer image capture."""

from contextlib import suppress
import time
import unittest

import FreeCAD
import FreeCADGui

try:
    from PySide6 import QtWidgets
except ImportError:
    from PySide import QtGui as QtWidgets  # type: ignore


class TestView3DFramebufferCapture(unittest.TestCase):
    def setUp(self):
        self.doc = FreeCAD.newDocument("TestView3DFramebufferCapture")
        FreeCADGui.ActiveDocument = FreeCADGui.getDocument(self.doc.Name)
        self.view = FreeCADGui.ActiveDocument.ActiveView
        self.viewer = self.view.getViewer()

        self._had_axis_cross = self.view.hasAxisCross()
        self.view.setAxisCross(False)

        self._had_navi_cube = self.viewer.isEnabledNaviCube()
        self.viewer.setEnabledNaviCube(False)

    def tearDown(self):
        with suppress(Exception):
            self.view.setAxisCross(self._had_axis_cross)
        with suppress(Exception):
            self.viewer.setEnabledNaviCube(self._had_navi_cube)

        if FreeCAD.getDocument(self.doc.Name):
            FreeCAD.closeDocument(self.doc.Name)

    def test_render_to_image_uses_requested_dimensions(self):
        self._flush_gui()

        image = self.viewer.renderToImage(width=240, height=135, samples=0)

        self.assertFalse(image.isNull())
        self.assertEqual(image.width(), 240)
        self.assertEqual(image.height(), 135)

    def test_render_to_image_preserves_argument_errors(self):
        with self.assertRaises(TypeError):
            self.viewer.renderToImage(unknown=True)

    def test_grab_framebuffer_uses_raster_orientation(self):
        self.viewer.setGradientBackground("LINEAR")
        self.viewer.setGradientBackgroundColor((1.0, 0.0, 0.0), (0.0, 0.0, 1.0))
        self._flush_gui()

        image = self.viewer.grabFramebuffer()
        x = image.width() // 2
        top = image.pixelColor(x, image.height() // 8)
        bottom = image.pixelColor(x, image.height() * 7 // 8)

        self.assertGreater(top.red(), top.blue())
        self.assertGreater(bottom.blue(), bottom.red())

    def _flush_gui(self):
        for _ in range(4):
            FreeCADGui.updateGui()
            QtWidgets.QApplication.processEvents()
            self.view.redraw()
            time.sleep(0.05)
