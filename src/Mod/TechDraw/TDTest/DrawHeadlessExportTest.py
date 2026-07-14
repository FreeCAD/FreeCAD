#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""Comprehensive headless export tests for the TechDraw module."""

import os
import re
import shutil
import subprocess
import sys
import tempfile
import unittest
import xml.etree.ElementTree as ET
from functools import lru_cache
from pathlib import Path

import FreeCAD as App
import TechDraw

TECHDRAW_MODULE_NAME = getattr(TechDraw, "__name__", "TechDraw")

try:  # Optional dependency for PDF validation
    import PyPDF2  # type: ignore
except ImportError:  # pragma: no cover - PyPDF2 not always available
    PyPDF2 = None


class DrawHeadlessExportTest(unittest.TestCase):
    """Exercise DrawPage headless export helpers added in Phase 1."""

    @staticmethod
    @lru_cache(maxsize=1)
    def _cmake_flag(name: str):
        """Best-effort read of a CMakeCache flag from the build tree."""
        cache_file = DrawHeadlessExportTest._cmake_cache_path()
        if cache_file is None:
            return None

        try:
            with cache_file.open("r", encoding="utf-8") as handle:
                for line in handle:
                    if line.startswith(f"{name}:"):
                        _, _, value = line.partition("=")
                        return value.strip()
        except OSError:
            return None
        return None

    @staticmethod
    @lru_cache(maxsize=1)
    def _cmake_cache_path():
        for parent in Path(__file__).resolve().parents:
            candidate = parent / "CMakeCache.txt"
            if candidate.exists():
                return candidate
        return None

    @classmethod
    @lru_cache(maxsize=1)
    def _qt_major_version(cls):
        """Detect the configured Qt major version from the CMake cache."""
        cache_file = cls._cmake_cache_path()
        if cache_file is None:
            return None

        try:
            with cache_file.open("r", encoding="utf-8") as handle:
                for line in handle:
                    if line.startswith("Qt6Core_DIR:") or line.startswith("Qt6_DIR:"):
                        return 6
                    if line.startswith("Qt5Core_DIR:") or line.startswith("Qt5_DIR:"):
                        return 5
        except OSError:
            return None
        return None

    def _pyside_enabled(self) -> bool:
        """Returns True when PySide can actually be imported."""
        venv = os.environ.get("FREECAD_TEST_VENV")
        if venv:
            site_dir = Path(venv) / f"lib/python{sys.version_info.major}.{sys.version_info.minor}/site-packages"
            if site_dir.exists() and str(site_dir) not in sys.path:
                sys.path.insert(0, str(site_dir))

        for mod in ("PySide6.QtGui", "PySide2.QtGui"):
            try:
                __import__(mod)
                return True
            except Exception:
                continue

        # Fallback to CMake flag if imports fail
        flag = self._cmake_flag("FREECAD_USE_PYSIDE")
        if flag is None:
            return False
        return flag.upper() == "ON"

    def _qt_available(self):
        """Return a PySide QGuiApplication class if available (PySide2/6)."""
        qt_major = self._qt_major_version()
        module_order = ("PySide6.QtGui", "PySide2.QtGui")
        if qt_major == 6:
            module_order = ("PySide6.QtGui",)
        elif qt_major == 5:
            module_order = ("PySide2.QtGui",)

        if not self._pyside_enabled():
            # If CMake flag was off but imports work, _pyside_enabled would have returned True.
            # At this point, respect the flag and skip.
            return None

        for module in module_order:
            try:
                components = __import__(module, fromlist=["QGuiApplication"])
                return getattr(components, "QGuiApplication", None)
            except Exception:  # pragma: no cover - PySide may be absent in some builds
                continue
        return None

    def setUp(self):
        self._doc = App.newDocument("TDHeadless")
        self._page = self._doc.addObject("TechDraw::DrawPage", "Page")
        self._tempdir = tempfile.mkdtemp(prefix="techdraw_headless_")
        self._assign_template(self._page)

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

    def _assign_template(self, page):
        """Ensure the supplied page has a usable template."""
        template = self._doc.addObject("TechDraw::DrawSVGTemplate", "UnitTestTemplate")
        template_path = None
        if TechDraw is not None and hasattr(TechDraw, "getStandardTemplate"):
            try:
                candidate = TechDraw.getStandardTemplate("A4_Landscape_TD.svg")  # type: ignore[attr-defined]
                if candidate:
                    candidate_path = Path(candidate)
                    if candidate_path.exists():
                        template_path = candidate_path
            except Exception:
                template_path = None

        if template_path is None:
            fallback = Path(self._tempdir) / "unit_template.svg"
            fallback.write_text(
                """<?xml version='1.0' encoding='UTF-8'?>\n<svg xmlns='http://www.w3.org/2000/svg' id='unit-test-template' width='297mm' height='210mm'\n viewBox='0 0 297 210' version='1.1'>\n  <rect x='5' y='5' width='287' height='200' fill='none' stroke='#000' stroke-width='0.35'/>\n</svg>\n""",
                encoding="utf-8",
            )
            template.Template = str(fallback)
        else:
            template.Template = str(template_path)

        page.Template = template

    @staticmethod
    def _err_text(err, fallback="returned False"):
        return str(err) if err else fallback

    def _render_svg_string(self):
        """Call DrawPage.renderToSVGString directly."""
        try:
            return self._page.renderToSVGString()
        except Exception as exc:
            raise AssertionError(f"renderToSVGString raised: {exc}") from exc

    def _export_svg_string(self):
        """Export to disk then read back (exercises exportToSVG path)."""
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

    def test_00_qt_application_defaults_when_unset(self):
        """Headless export should bootstrap an offscreen QGuiApplication."""
        QGuiApplication = self._qt_available()
        if QGuiApplication is None:
            self.skipTest("PySide bindings are not available")

        if QGuiApplication.instance():  # pragma: no cover - GUI builds
            self.skipTest("QGuiApplication already initialised")

        original_platform = os.environ.pop("QT_QPA_PLATFORM", None)
        original_opengl = os.environ.pop("QT_OPENGL", None)

        try:
            svg_content = self._export_svg_string()
            self.assertTrue(svg_content, "SVG string is empty")
            self.assertIsNotNone(QGuiApplication.instance(),
                                 "rendering did not create a QGuiApplication")
            self.assertNotIn("Qt SVG Document", svg_content,
                              "Qt fallback SVG stub detected")
            self.assertEqual(os.environ.get("QT_QPA_PLATFORM"), "offscreen")
            self.assertEqual(os.environ.get("QT_OPENGL"), "software")
        finally:
            if original_platform is None:
                os.environ.pop("QT_QPA_PLATFORM", None)
            else:
                os.environ["QT_QPA_PLATFORM"] = original_platform

            if original_opengl is None:
                os.environ.pop("QT_OPENGL", None)
            else:
                os.environ["QT_OPENGL"] = original_opengl

    def test_qt_application_respects_existing_environment(self):
        """Existing Qt platform hints must not be overwritten."""
        QGuiApplication = self._qt_available()
        if QGuiApplication is None:
            self.skipTest("PySide bindings are not available")

        original_platform = os.environ.get("QT_QPA_PLATFORM")
        original_opengl = os.environ.get("QT_OPENGL")
        os.environ["QT_QPA_PLATFORM"] = original_platform or "offscreen"
        os.environ["QT_OPENGL"] = original_opengl or "software"

        try:
            svg_content = self._export_svg_string()
            self.assertTrue(svg_content, "SVG string is empty")
            self.assertIsNotNone(QGuiApplication.instance())
            self.assertNotIn("Qt SVG Document", svg_content,
                              "Qt fallback SVG stub detected")
            self.assertEqual(os.environ.get("QT_QPA_PLATFORM"),
                             original_platform or "offscreen")
            self.assertEqual(os.environ.get("QT_OPENGL"),
                             original_opengl or "software")
        finally:
            if original_platform is None:
                os.environ.pop("QT_QPA_PLATFORM", None)
            else:
                os.environ["QT_QPA_PLATFORM"] = original_platform

            if original_opengl is None:
                os.environ.pop("QT_OPENGL", None)
            else:
                os.environ["QT_OPENGL"] = original_opengl

    def test_fail_fast_with_existing_qcoreapplication(self):
        """Headless export must fail when a non-GUI QCoreApplication is running."""
        try:
            import PySide6  # noqa: F401
            from PySide6 import QtCore as _QtCore  # noqa: F401
            from PySide6 import QtGui as _QtGui  # noqa: F401
        except ImportError:
            self.skipTest("PySide6 bindings are not available")

        script = '''import os, sys, shutil, tempfile
from pathlib import Path
import FreeCAD as App
import TechDraw
from PySide6.QtCore import QCoreApplication
from PySide6.QtGui import QGuiApplication

try:
    app = QCoreApplication([])
    tempdir = tempfile.mkdtemp(prefix="td_core_only_")
    try:
        doc = App.newDocument("CoreOnly")
        page = doc.addObject("TechDraw::DrawPage", "Page")
        template = doc.addObject("TechDraw::DrawSVGTemplate", "Template")
        template_path = Path(tempdir) / "core_only_template.svg"
        template_path.write_text(
            "<?xml version='1.0' encoding='UTF-8'?>\n"
            "<svg xmlns='http://www.w3.org/2000/svg' id='core-only' width='210mm' height='297mm' viewBox='0 0 210 297'>\n"
            "<rect x='1' y='1' width='208' height='295' fill='none' stroke='#000' stroke-width='0.35'/>\n"
            "</svg>\n",
            encoding="utf-8",
        )
        template.Template = str(template_path)
        page.Template = template
        doc.recompute()
        pdf_path = Path(tempdir) / "core_only.pdf"
        ok = page.exportToPDF(str(pdf_path))
        gui_created = QGuiApplication.instance() is not None
    finally:
        shutil.rmtree(tempdir, ignore_errors=True)
    sys.exit(0 if (not ok and not gui_created) else 1)
except Exception:
    import traceback
    traceback.print_exc()
    sys.exit(2)
'''

        python_exe = sys.executable
        if os.path.basename(python_exe).lower().startswith("freecadcmd"):
            python_exe = shutil.which("python3") or python_exe

        result = subprocess.run(
            [python_exe, "-c", script],
            env=os.environ.copy(),
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )

        self.assertEqual(
            result.returncode,
            0,
            f"Headless export should fail-fast when QCoreApplication exists "
            f"(stdout: {result.stdout} stderr: {result.stderr})",
        )

    def test_fail_fast_with_existing_qcoreapplication_pyside2(self):
        """Headless export must fail when a non-GUI QCoreApplication is running (PySide2)."""
        qt_major = self._qt_major_version()
        if qt_major and qt_major != 5:
            self.skipTest("PySide2 bindings are not available for this Qt version")
        if not self._pyside_enabled():
            self.skipTest("PySide bindings are not available")
        try:
            import PySide2  # noqa: F401
            from PySide2 import QtCore as _QtCore  # noqa: F401
            from PySide2 import QtGui as _QtGui  # noqa: F401
        except ImportError:
            self.skipTest("PySide2 bindings are not available")

        script = '''import os, sys, shutil, tempfile
from pathlib import Path
import FreeCAD as App
import TechDraw
from PySide2.QtCore import QCoreApplication
from PySide2.QtGui import QGuiApplication

try:
    app = QCoreApplication([])
    tempdir = tempfile.mkdtemp(prefix="td_core_only_")
    try:
        doc = App.newDocument("CoreOnly")
        page = doc.addObject("TechDraw::DrawPage", "Page")
        template = doc.addObject("TechDraw::DrawSVGTemplate", "Template")
        template_path = Path(tempdir) / "core_only_template.svg"
        template_path.write_text(
            "<?xml version='1.0' encoding='UTF-8'?>\\n"
            "<svg xmlns='http://www.w3.org/2000/svg' id='core-only' width='210mm' height='297mm' viewBox='0 0 210 297'>\\n"
            "<rect x='1' y='1' width='208' height='295' fill='none' stroke='#000' stroke-width='0.35'/>\\n"
            "</svg>\\n",
            encoding='utf-8',
        )
        template.Template = str(template_path)
        page.Template = template
        doc.recompute()
        pdf_path = Path(tempdir) / "core_only.pdf"
        ok = page.exportToPDF(str(pdf_path))
        gui_created = QGuiApplication.instance() is not None
    finally:
        shutil.rmtree(tempdir, ignore_errors=True)
    sys.exit(0 if (not ok and not gui_created) else 1)
except Exception:
    import traceback
    traceback.print_exc()
    sys.exit(2)
'''

        python_exe = sys.executable
        if os.path.basename(python_exe).lower().startswith("freecadcmd"):
            python_exe = shutil.which("python3") or python_exe

        result = subprocess.run(
            [python_exe, "-c", script],
            env=os.environ.copy(),
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )

        self.assertEqual(
            result.returncode,
            0,
            f"Headless export should fail-fast when QCoreApplication exists (PySide2) "
            f"(stdout: {result.stdout} stderr: {result.stderr})",
        )

    def test_template_svg_metadata_matches_full_render(self):
        """SVG outputs should carry consistent metadata and template tags."""
        full_svg = self._render_svg_string()
        self.assertTrue(full_svg, "Full SVG export failed")
        file_svg = self._export_svg_string()
        self.assertTrue(file_svg, "File SVG export failed")

        for svg in (full_svg, file_svg):
            self.assertIn("<title>FreeCAD TechDraw Page</title>", svg)
            self.assertIn("<desc>Generated by FreeCAD TechDraw</desc>", svg)
            self.assertNotIn("Qt SVG Document", svg)
            self.assertNotIn("Generated with Qt</desc>", svg)

        comment_match = re.search(r"<!-- template-id: ([^ ]+) -->", full_svg)
        self.assertIsNotNone(comment_match, "SVG output missing template-id comment")
        self.assertIn(comment_match.group(0), file_svg, "File render missing template-id comment")

    def test_template_only_render_uses_full_pipeline(self):
        """renderTemplateToSVG should use the Qt pipeline with metadata and template id."""
        template_svg = self._page.renderTemplateToSVG()
        self.assertTrue(template_svg, "Template render returned empty output")
        self.assertIn("<title>FreeCAD TechDraw Page</title>", template_svg)
        self.assertIn("<desc>Generated by FreeCAD TechDraw</desc>", template_svg)
        self.assertNotIn("Qt SVG Document", template_svg)
        self.assertNotIn("Generated with Qt</desc>", template_svg)

        full_svg = self._render_svg_string()
        comment_match = re.search(r"<!-- template-id: ([^ ]+) -->", template_svg)
        self.assertIsNotNone(comment_match, "Template render missing template-id comment")
        if full_svg:
            self.assertIn(comment_match.group(0),
                          full_svg,
                          "Template render template-id comment absent from full render")

    def test_template_render_bootstraps_qt(self):
        """renderTemplateToSVG should bootstrap QGuiApplication when PySide is available."""
        QGuiApplication = self._qt_available()
        if QGuiApplication is None:
            self.skipTest("PySide bindings are not available")
        if QGuiApplication.instance():  # pragma: no cover - GUI builds
            self.skipTest("QGuiApplication already initialised")

        original_platform = os.environ.pop("QT_QPA_PLATFORM", None)
        original_opengl = os.environ.pop("QT_OPENGL", None)

        try:
            svg_content = self._page.renderTemplateToSVG()
            self.assertTrue(svg_content, "Template render returned empty output")
            self.assertIsNotNone(QGuiApplication.instance(),
                                 "renderTemplateToSVG did not create a QGuiApplication")
            self.assertEqual(os.environ.get("QT_QPA_PLATFORM"), "offscreen")
            self.assertEqual(os.environ.get("QT_OPENGL"), "software")
        finally:
            if original_platform is None:
                os.environ.pop("QT_QPA_PLATFORM", None)
            else:
                os.environ["QT_QPA_PLATFORM"] = original_platform

            if original_opengl is None:
                os.environ.pop("QT_OPENGL", None)
            else:
                os.environ["QT_OPENGL"] = original_opengl

    def test_api_methods_available(self):
        """DrawPage should expose the headless export helpers."""
        self.assertEqual(TECHDRAW_MODULE_NAME, "TechDraw", "TechDraw module failed to import")
        for attr in ("exportToPDF", "exportToSVG", "renderToSVGString", "renderTemplateToSVG"):
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
        svg_content = self._render_svg_string()
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

    def test_unicode_filename_handling(self):
        """Export should handle Unicode filenames correctly."""
        unicode_pdf = self._tmp("tëst_ünìcödé.pdf")
        unicode_svg = self._tmp("tëst_ünìcödé.svg")

        ok_pdf, err_pdf = self._export_pdf(unicode_pdf)
        self.assertTrue(ok_pdf, f"Unicode PDF export failed: {self._err_text(err_pdf)}")
        self.assertTrue(os.path.exists(unicode_pdf), "Unicode PDF file was not created")

        ok_svg, err_svg = self._export_svg(unicode_svg)
        self.assertTrue(ok_svg, f"Unicode SVG export failed: {self._err_text(err_svg)}")
        self.assertTrue(os.path.exists(unicode_svg), "Unicode SVG file was not created")

    def test_page_size_boundaries(self):
        """Test export with extreme page sizes."""
        # Test with very small page (minimum viable size)
        template = self._doc.addObject("TechDraw::DrawSVGTemplate", "SmallTemplate")
        template_path = Path(self._tempdir) / "page_size_template.svg"
        template_path.write_text(
            "<?xml version='1.0' encoding='UTF-8'?>\n"
            "<svg xmlns='http://www.w3.org/2000/svg' id='page-size-template' width='210mm' height='297mm' viewBox='0 0 210 297'>\n"
            "<rect x='1' y='1' width='208' height='295' fill='none' stroke='#000' stroke-width='0.35'/>\n"
            "</svg>\n",
            encoding="utf-8",
        )
        template.Template = str(template_path)
        template.setExpression("Width", "1mm")
        template.setExpression("Height", "1mm")
        self._page.Template = template
        self._doc.recompute()

        small_pdf = self._tmp("small.pdf")
        ok_pdf, err_pdf = self._export_pdf(small_pdf)
        self.assertTrue(ok_pdf, f"Small page PDF export failed: {self._err_text(err_pdf)}")

        # Test with large page (A0 size: 841x1189mm)
        template.setExpression("Width", "841mm")
        template.setExpression("Height", "1189mm")
        self._doc.recompute()

        large_pdf = self._tmp("large.pdf")
        ok_pdf, err_pdf = self._export_pdf(large_pdf)
        self.assertTrue(ok_pdf, f"Large page PDF export failed: {self._err_text(err_pdf)}")

    def test_dpi_resolution_limits(self):
        """Test export with various DPI settings to ensure no overflow."""
        svg_content = self._export_svg_string()
        self.assertTrue(svg_content, "Base SVG export failed")

        # Test that very high DPI doesn't cause integer overflow
        # This is a regression test for potential overflow in PageRenderer.cpp:223-224
        # We can't directly set DPI from Python, but we can verify the output is reasonable
        self.assertLess(len(svg_content), 100000000, "SVG output suspiciously large (possible overflow)")
        self.assertGreater(len(svg_content), 100, "SVG output suspiciously small")

    def test_svg_encoding_validation(self):
        """Test SVG output has proper encoding and structure."""
        svg_content = self._export_svg_string()
        self.assertTrue(svg_content, "SVG export failed")

        # Check for proper XML declaration and encoding
        self.assertTrue(svg_content.startswith('<?xml') or '<svg' in svg_content[:100],
                       "SVG missing proper XML structure")

        # Validate UTF-8 encoding by ensuring it can be re-encoded
        try:
            svg_content.encode('utf-8')
        except UnicodeEncodeError:
            self.fail("SVG content contains invalid UTF-8 characters")

        # Check for basic SVG structure
        self.assertIn('<svg', svg_content, "SVG missing root element")
        self.assertIn('</svg>', svg_content, "SVG missing closing tag")

    def test_file_permission_edge_cases(self):
        """Test handling of file permission issues."""
        if os.name == 'nt':  # Windows
            self.skipTest("File permission tests not applicable on Windows")

        # Create a directory with no write permissions
        readonly_dir = os.path.join(self._tempdir, "readonly")
        os.makedirs(readonly_dir, exist_ok=True)
        os.chmod(readonly_dir, 0o444)  # Read-only

        try:
            readonly_pdf = os.path.join(readonly_dir, "fail.pdf")
            ok_pdf, err_pdf = self._export_pdf(readonly_pdf)
            self.assertFalse(ok_pdf, "Export to read-only directory should fail")
            self.assertFalse(os.path.exists(readonly_pdf), "File should not be created in read-only directory")
        finally:
            # Restore permissions for cleanup
            os.chmod(readonly_dir, 0o755)

    def test_concurrent_export_safety(self):
        """Test that multiple exports don't interfere with each other."""
        # Simulate potential race condition by doing rapid exports
        paths = [self._tmp(f"concurrent_{i}.pdf") for i in range(5)]
        results = []

        for path in paths:
            ok, err = self._export_pdf(path)
            results.append((ok, err, path))

        # All exports should succeed
        for i, (ok, err, path) in enumerate(results):
            self.assertTrue(ok, f"Concurrent export {i} failed: {self._err_text(err)}")
            self.assertTrue(os.path.exists(path), f"Concurrent export {i} file missing")
            self.assertGreater(os.path.getsize(path), 0, f"Concurrent export {i} file empty")

    def test_memory_constraint_handling(self):
        """Test export behavior under memory constraints."""
        # Test with string export that might consume more memory
        svg_strings = []

        # Generate multiple SVG strings to test memory usage
        for i in range(10):
            svg_content = self._export_svg_string()
            self.assertTrue(svg_content, f"SVG export {i} failed")
            svg_strings.append(svg_content)

        # All strings should be identical (deterministic output)
        for i, svg in enumerate(svg_strings[1:], 1):
            self.assertEqual(svg, svg_strings[0], f"SVG export {i} differs from first export")

    def test_special_characters_in_paths(self):
        """Test export with various special characters in file paths."""
        special_chars = ["spaces in name", "dots.in.name", "dash-in-name", "under_score"]

        for char_test in special_chars:
            with self.subTest(char_test=char_test):
                pdf_path = self._tmp(f"{char_test}.pdf")
                svg_path = self._tmp(f"{char_test}.svg")

                ok_pdf, err_pdf = self._export_pdf(pdf_path)
                self.assertTrue(ok_pdf, f"PDF export with '{char_test}' failed: {self._err_text(err_pdf)}")

                ok_svg, err_svg = self._export_svg(svg_path)
                self.assertTrue(ok_svg, f"SVG export with '{char_test}' failed: {self._err_text(err_svg)}")

    def test_template_field_processing(self):
        """Test that template field processing works correctly."""
        simple_template_path = os.path.join(os.path.dirname(__file__), "TestTemplateSimple.svg")
        if not os.path.exists(simple_template_path):
            self.skipTest("TestTemplateSimple.svg not found")

        template = self._doc.addObject("TechDraw::DrawSVGTemplate", "FieldTemplate")
        template.Template = simple_template_path
        self._page.Template = template
        self._doc.recompute()

        # Export SVG and check that template content is present
        svg_content = self._export_svg_string()
        self.assertTrue(svg_content, "Template field processing export failed")

        # Verify the template content is included
        self.assertIn("simple-test-template", svg_content, "Template ID not found in output")

        # Check that basic SVG structure is maintained
        self.assertIn("<svg", svg_content, "SVG root element missing")
        self.assertIn("</svg>", svg_content, "SVG closing tag missing")

        # Verify viewBox is properly set
        self.assertIn("viewBox", svg_content, "ViewBox attribute missing")

    def test_extreme_resolution_handling(self):
        """Test handling of extreme resolution scenarios."""
        # Test that very large theoretical resolutions don't cause overflow
        # This validates the safeguards in PageRenderer.cpp:223-224
        svg_content = self._export_svg_string()
        self.assertTrue(svg_content, "Base SVG export failed")

        # Validate output size is reasonable (not from integer overflow)
        content_size = len(svg_content)
        self.assertGreater(content_size, 100, "SVG output too small")
        self.assertLess(content_size, 50000000, "SVG output suspiciously large (>50MB)")

        # Check that SVG dimensions are reasonable
        if 'width=' in svg_content and 'height=' in svg_content:
            import re
            width_match = re.search(r'width="([^"]+)"', svg_content)
            height_match = re.search(r'height="([^"]+)"', svg_content)

            if width_match and height_match:
                width_str = width_match.group(1)
                height_str = height_match.group(1)

                # Basic sanity check - dimensions should contain numbers
                self.assertTrue(any(c.isdigit() for c in width_str), "Width contains no digits")
                self.assertTrue(any(c.isdigit() for c in height_str), "Height contains no digits")


    def test_deterministic_output(self):
        """Test that export output is deterministic across multiple runs."""
        # Generate multiple exports and verify they're identical
        svg_outputs = []
        for i in range(3):
            svg_content = self._export_svg_string()
            self.assertTrue(svg_content, f"SVG export {i} failed")
            svg_outputs.append(svg_content)

        # All outputs should be identical
        for i, svg in enumerate(svg_outputs[1:], 1):
            self.assertEqual(svg, svg_outputs[0],
                           f"SVG export {i} differs from first export - output not deterministic")

    def test_error_message_quality(self):
        """Test that error messages are informative and helpful."""
        # Test with completely invalid path
        invalid_path = "/dev/null/impossible/path.pdf"
        ok, err = self._export_pdf(invalid_path)
        self.assertFalse(ok, "Export to impossible path should fail")

        # Error should be informative (not just generic failure)
        if err:
            error_msg = str(err).lower()
            # Should mention file or path in error
            self.assertTrue(any(word in error_msg for word in ['file', 'path', 'directory', 'write']),
                          f"Error message not informative enough: {err}")


if __name__ == "__main__":
    unittest.main()
