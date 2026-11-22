# SPDX-License-Identifier: LGPL-2.1-or-later

"""GUI visual regression test for corner coordinate system axis letters.

This covers a regression where the default 10% corner coordinate system drew
the colored axes but not the X/Y/Z letter textures. Instead of comparing a full
golden screenshot, the test captures the same view with the corner overlay
hidden and visible, then looks for newly drawn low-saturation, non-white
pixels in the corner crop. With a white background, the default black/gray
letter pixels are easy to separate from the saturated red/green/blue axis lines
without depending on exact antialiasing or full-frame rendering details.

Run with:
    FreeCAD -t TestCornerAxisCrossVisual
"""

import time
import unittest

import FreeCAD
import FreeCADGui
from PySide6 import QtWidgets


class TestCornerAxisCrossVisual(unittest.TestCase):
    """Verify that the default-size corner coordinate system draws axis letters."""

    _CORNER_SIZE_PERCENT = 10
    _MIN_NEW_LETTER_PIXELS = 12

    def setUp(self):
        self.doc = FreeCAD.newDocument("TestCornerAxisCrossVisual")
        FreeCADGui.ActiveDocument = FreeCADGui.getDocument(self.doc.Name)
        self.view = FreeCADGui.ActiveDocument.ActiveView
        self.viewer = self.view.getViewer()

        self._had_axis_cross = self.view.hasAxisCross()
        self._had_corner_cross = self.view.isCornerCrossVisible()
        self._corner_cross_size = self.view.getCornerCrossSize()
        self._had_navi_cube = self.viewer.isEnabledNaviCube()

    def tearDown(self):
        self.view.setAxisCross(self._had_axis_cross)
        self.view.setCornerCrossSize(self._corner_cross_size)
        self.view.setCornerCrossVisible(self._had_corner_cross)
        self.viewer.setEnabledNaviCube(self._had_navi_cube)

        if FreeCAD.getDocument(self.doc.Name):
            FreeCAD.closeDocument(self.doc.Name)

    def test_default_corner_axis_cross_shows_axis_letters(self):
        self.view.setAxisCross(False)
        self.viewer.setEnabledNaviCube(False)
        self.view.setCornerCrossSize(self._CORNER_SIZE_PERCENT)

        self.view.setCornerCrossVisible(False)
        self._prepare_view()
        before = self._grab_framebuffer()

        self.view.setCornerCrossVisible(True)
        self._flush_gui()
        after = self._grab_framebuffer()

        # Compare against the hidden-overlay frame so the assertion is local to
        # newly drawn corner overlay pixels, not the user's theme or viewport.
        crop = self._corner_crop(after)
        new_letter_pixels = self._new_low_saturation_non_white_pixel_count(before, after, crop)

        self.assertGreaterEqual(
            new_letter_pixels,
            self._MIN_NEW_LETTER_PIXELS,
            (
                "Default-size corner coordinate system did not render visible axis "
                f"letter pixels; found {new_letter_pixels} new low-saturation "
                f"non-white pixels in crop {crop}"
            ),
        )

    def _prepare_view(self):
        self.viewer.setGradientBackground("")
        self.viewer.setBackgroundColor(1.0, 1.0, 1.0)
        self.view.setCameraType("Orthographic")
        self.view.viewAxometric()
        self._flush_gui()

    def _flush_gui(self):
        for _ in range(6):
            FreeCADGui.updateGui()
            QtWidgets.QApplication.processEvents()
            self.view.redraw()
            time.sleep(0.05)

    def _grab_framebuffer(self):
        try:
            image = self.viewer.grabFramebuffer()
        except RuntimeError:
            image = self._grab_viewport().toImage()
        self.assertFalse(image.isNull(), "3D view framebuffer capture returned a null image")
        return image

    def _grab_viewport(self):
        graphics_view = self.view.graphicsView()
        viewport = graphics_view.viewport()
        viewport.repaint()
        self._flush_gui()

        screen = viewport.screen()
        if screen is not None:
            pixmap = screen.grabWindow(int(viewport.winId()))
            if not pixmap.isNull():
                return pixmap

        return viewport.grab()

    def _corner_crop(self, image):
        width = image.width()
        height = image.height()
        axis_size = int(min(width, height) * self._CORNER_SIZE_PERCENT / 100.0)
        crop_size = max(72, int(axis_size * 1.8))
        crop_size = min(crop_size, width, height)
        return (width - crop_size, height - crop_size, width, height)

    def _new_low_saturation_non_white_pixel_count(self, before, after, crop):
        left, top, right, bottom = crop
        count = 0
        for y in range(top, bottom):
            for x in range(left, right):
                after_rgb = self._rgb(after, x, y)
                is_letter_pixel = self._is_low_saturation_non_white_pixel(after_rgb)
                changed = self._changed_enough(self._rgb(before, x, y), after_rgb)
                if is_letter_pixel and changed:
                    count += 1
        return count

    @staticmethod
    def _rgb(image, x, y):
        color = image.pixelColor(x, y)
        return (color.red(), color.green(), color.blue())

    @staticmethod
    def _is_low_saturation_non_white_pixel(rgb):
        # Default and shipped preference-pack letter colors are black or gray.
        # The colored axes are saturated, and the test background is white.
        return max(rgb) < 245 and max(rgb) - min(rgb) < 55

    @staticmethod
    def _changed_enough(before, after):
        return sum(abs(a - b) for a, b in zip(before, after)) > 45
