# SPDX-License-Identifier: LGPL-2.1-or-later
"""Tests for SoQtOffscreenRenderer, including writeToBuffer."""

import unittest
import FreeCAD
import FreeCADGui


class TestOffscreenRendererWriteToBuffer(unittest.TestCase):
    """Tests for the writeToBuffer method of SoQtOffscreenRenderer."""

    def setUp(self):
        from pivy import coin

        self.coin = coin

        # Build a minimal scene: camera + light + a simple shape
        self.root = coin.SoSeparator()
        cam = coin.SoOrthographicCamera()
        light = coin.SoDirectionalLight()
        cone = coin.SoCone()
        self.root.addChild(cam)
        self.root.addChild(light)
        self.root.addChild(cone)

        width, height = 64, 64
        viewport = coin.SbViewportRegion(width, height)
        cam.viewAll(self.root, viewport)

        self.root.ref()
        self.renderer = FreeCADGui.SoQtOffscreenRenderer(width, height)
        self.renderer.setBackgroundColor(1.0, 1.0, 1.0)
        self.renderer.render(self.root)

    def tearDown(self):
        self.root.unref()

    def testWriteToBufferReturnsBytesDefaultPNG(self):
        """writeToBuffer() with no arguments returns non-empty bytes."""
        result = self.renderer.writeToBuffer()
        self.assertIsInstance(result, bytes)
        self.assertGreater(len(result), 0)

    def testWriteToBufferPNGSignature(self):
        """writeToBuffer() default output starts with the PNG signature."""
        result = self.renderer.writeToBuffer()
        png_signature = b"\x89PNG\r\n\x1a\n"
        self.assertTrue(
            result.startswith(png_signature), "Output does not start with PNG signature"
        )

    def testWriteToBufferExplicitPNG(self):
        """writeToBuffer('PNG') produces valid PNG data."""
        result = self.renderer.writeToBuffer("PNG")
        png_signature = b"\x89PNG\r\n\x1a\n"
        self.assertTrue(result.startswith(png_signature))

    def testWriteToBufferBMP(self):
        """writeToBuffer('BMP') produces data starting with the BMP signature."""
        result = self.renderer.writeToBuffer("BMP")
        self.assertIsInstance(result, bytes)
        self.assertTrue(result.startswith(b"BM"), "Output does not start with BMP signature")

    def testWriteToBufferJPEG(self):
        """writeToBuffer('JPEG') produces data starting with the JPEG SOI marker."""
        result = self.renderer.writeToBuffer("JPEG")
        self.assertIsInstance(result, bytes)
        jpeg_soi = b"\xff\xd8\xff"
        self.assertTrue(result.startswith(jpeg_soi), "Output does not start with JPEG SOI marker")

    def testWriteToBufferWithQuality(self):
        """writeToBuffer('PNG', quality) accepts a quality parameter."""
        result = self.renderer.writeToBuffer("PNG", 50)
        self.assertIsInstance(result, bytes)
        self.assertGreater(len(result), 0)

    def testWriteToBufferInvalidFormatRaises(self):
        """writeToBuffer with an unsupported format raises RuntimeError."""
        with self.assertRaises(RuntimeError):
            self.renderer.writeToBuffer("NOTAFORMAT")

    def testWriteToBufferConsistentOutput(self):
        """Two calls to writeToBuffer produce identical output."""
        result1 = self.renderer.writeToBuffer("BMP")
        result2 = self.renderer.writeToBuffer("BMP")
        self.assertEqual(result1, result2)

    def testWriteToBufferMatchesWriteToImage(self):
        """writeToBuffer('PNG') produces the same content as writeToImage for PNG."""
        import tempfile
        import os

        buf_data = self.renderer.writeToBuffer("PNG")

        with tempfile.NamedTemporaryFile(suffix=".png", delete=False) as tmp:
            tmp_path = tmp.name

        try:
            self.renderer.writeToImage(tmp_path)
            with open(tmp_path, "rb") as f:
                file_data = f.read()
            self.assertEqual(buf_data, file_data)
        finally:
            os.unlink(tmp_path)
