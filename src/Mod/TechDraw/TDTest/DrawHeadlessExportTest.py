#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""Headless export smoke tests for the TechDraw module."""

import os
import shutil
import tempfile
import unittest
import xml.etree.ElementTree as ET

import FreeCAD as App
import TechDraw

TECHDRAW_MODULE_NAME = getattr(TechDraw, "__name__", "TechDraw")

try:  # Optional dependency for PDF validation
    import PyPDF2  # type: ignore
except ImportError:  # pragma: no cover - PyPDF2 not always available
    PyPDF2 = None


class DrawHeadlessExportTest(unittest.TestCase):
    """Exercise DrawPage headless export helpers added in Phase 1."""

    def setUp(self):
        self._doc = App.newDocument("TDHeadless")
        self._page = self._doc.addObject("TechDraw::DrawPage", "Page")
        self._tempdir = tempfile.mkdtemp(prefix="techdraw_headless_")

    def tearDown(self):
        doc_name = self._doc.Name
        App.closeDocument(doc_name)
        shutil.rmtree(self._tempdir, ignore_errors=True)

    # Helpers -----------------------------------------------------------------

    def _tmp(self, filename):
        return os.path.join(self._tempdir, filename)

    def _export_pdf(self, target):
        """Invoke TechDraw export helper and normalise the return value."""
        try:
            result = self._page.exportToPDF(target)
        except Exception as exc:  # pragma: no cover - exercised in failure tests
            return False, exc
        return bool(result), None

    def _export_svg(self, target):
        """Invoke TechDraw export helper and normalise the return value."""
        try:
            result = self._page.exportToSVG(target)
        except Exception as exc:  # pragma: no cover - exercised in failure tests
            return False, exc
        return bool(result), None

    @staticmethod
    def _err_text(err, fallback="returned False"):
        return str(err) if err else fallback

    def _export_svg_string(self):
        temp_svg = self._tmp("temp_render.svg")
        ok, err = self._export_svg(temp_svg)
        if not ok:
            raise AssertionError(f"SVG export failed: {self._err_text(err, 'unknown error')}")
        try:
            with open(temp_svg, "r", encoding="utf-8") as handle:
                return handle.read()
        finally:
            if os.path.exists(temp_svg):
                os.remove(temp_svg)

    # Tests -------------------------------------------------------------------

    def test_api_methods_available(self):
        """DrawPage should expose the headless export helpers."""
        self.assertEqual(TECHDRAW_MODULE_NAME, "TechDraw", "TechDraw module failed to import")
        for attr in ("exportToPDF", "exportToSVG", "renderToSVGString"):
            self.assertTrue(hasattr(self._page, attr), f"DrawPage missing {attr}")

    def test_pdf_export_creates_file(self):
        """exportToPDF should create a non-empty PDF file."""
        pdf_path = self._tmp("basic.pdf")
        ok, err = self._export_pdf(pdf_path)
        self.assertTrue(ok, f"exportToPDF failed: {self._err_text(err)}")
        self.assertTrue(os.path.exists(pdf_path), "PDF file was not created")
        self.assertGreater(os.path.getsize(pdf_path), 0, "PDF file is empty")

        # Validate PDF header or parse with PyPDF2 if available
        with open(pdf_path, "rb") as handle:
            head = handle.read(4)
        self.assertEqual(head, b"%PDF", "PDF header missing")

        if PyPDF2 is not None:
            reader = PyPDF2.PdfReader(pdf_path)
            self.assertGreaterEqual(len(reader.pages), 1, "PDF should contain at least one page")

    def test_svg_export_creates_file(self):
        """exportToSVG should emit an SVG file with content."""
        svg_path = self._tmp("basic.svg")
        ok, err = self._export_svg(svg_path)
        self.assertTrue(ok, f"exportToSVG failed: {self._err_text(err)}")
        self.assertTrue(os.path.exists(svg_path), "SVG file was not created")
        with open(svg_path, "r", encoding="utf-8") as handle:
            snippet = handle.read(200)
        self.assertTrue(snippet.strip(), "SVG file is empty")

        tree = ET.parse(svg_path)
        self.assertEqual(tree.getroot().tag.split('}')[-1], "svg")

    def test_render_to_svg_string_returns_markup(self):
        """renderToSVGString should return SVG markup."""
        svg_content = self._export_svg_string()
        self.assertTrue(svg_content, "SVG string is empty")

        root = ET.fromstring(svg_content)
        self.assertEqual(root.tag.split('}')[-1], "svg")

    def test_export_rejects_empty_path(self):
        """Empty paths must be rejected with a False return value."""
        ok_pdf, err_pdf = self._export_pdf("")
        self.assertFalse(ok_pdf, f"Empty path PDF export unexpectedly succeeded: {err_pdf}")

        ok_svg, err_svg = self._export_svg("")
        self.assertFalse(ok_svg, f"Empty path SVG export unexpectedly succeeded: {err_svg}")

    def test_export_invalid_path_sets_error(self):
        bad_dir = os.path.join(self._tempdir, "missing")
        pdf_bad = os.path.join(bad_dir, "fail.pdf")
        svg_bad = os.path.join(bad_dir, "fail.svg")

        ok_pdf, err_pdf = self._export_pdf(pdf_bad)
        self.assertFalse(ok_pdf, f"Invalid path PDF export unexpectedly succeeded: {err_pdf}")
        self.assertFalse(os.path.exists(pdf_bad))

        ok_svg, err_svg = self._export_svg(svg_bad)
        self.assertFalse(ok_svg, f"Invalid path SVG export unexpectedly succeeded: {err_svg}")
        self.assertFalse(os.path.exists(svg_bad))

    def test_export_with_template_object(self):
        """A linked SVG template should still export successfully."""
        template = self._doc.addObject("TechDraw::DrawSVGTemplate", "Template")
        template_path = os.path.join(os.path.dirname(__file__), "TestTemplate.svg")
        template.Template = template_path
        self._page.Template = template
        self._doc.recompute()

        pdf_path = self._tmp("template.pdf")
        svg_path = self._tmp("template.svg")

        ok_pdf, err_pdf = self._export_pdf(pdf_path)
        self.assertTrue(ok_pdf, f"Template PDF export failed: {self._err_text(err_pdf)}")
        self.assertTrue(os.path.exists(pdf_path))
        ok_svg, err_svg = self._export_svg(svg_path)
        self.assertTrue(ok_svg, f"Template SVG export failed: {self._err_text(err_svg)}")
        self.assertTrue(os.path.exists(svg_path))


if __name__ == "__main__":
    unittest.main()
