#!/usr/bin/env python3
"""
TechDraw Headless Export Test Suite

This script provides comprehensive testing for the TechDraw headless export functionality.
Run this script within FreeCAD Python console or as a macro.

Usage:
1. In FreeCAD GUI: Run as macro
2. In FreeCADCmd: python src/Mod/TechDraw/TDTest/DrawHeadlessExportSmoke.py
3. External: freecadcmd src/Mod/TechDraw/TDTest/DrawHeadlessExportSmoke.py
"""

import os
import sys
import tempfile
import traceback

# Try to import FreeCAD - adjust for different environments
try:
    import FreeCAD as App
    import TechDraw
    FREECAD_AVAILABLE = True
    TECHDRAW_MODULE_NAME = getattr(TechDraw, "__name__", "TechDraw")
except ImportError:
    print("Error: FreeCAD not available. Run this script within FreeCAD environment.")
    FREECAD_AVAILABLE = False
    sys.exit(1)

TEMPLATE_FILE = os.path.join(
    os.path.dirname(__file__),
    "src",
    "Mod",
    "TechDraw",
    "TDTest",
    "TestTemplate.svg",
)


class TechDrawHeadlessTestSuite:
    def __init__(self):
        self.test_results = {}
        self.temp_dir = tempfile.mkdtemp(prefix='techdraw_test_')
        self.test_doc = None
        print(
            "Test suite initialized. "
            f"Temporary directory: {self.temp_dir}. "
            f"TechDraw module: {TECHDRAW_MODULE_NAME}"
        )

    def setup_test_environment(self):
        """Create test document and basic objects"""
        try:
            # Create test document
            self.test_doc = App.newDocument("TechDrawHeadlessTest")

            # Create basic 3D object for testing
            self.test_box = self.test_doc.addObject("Part::Box", "TestBox")
            self.test_box.Length = 100
            self.test_box.Width = 50
            self.test_box.Height = 30

            # Create TechDraw page
            self.test_page = self.test_doc.addObject('TechDraw::DrawPage', 'TestPage')

            # Recompute document
            self.test_doc.recompute()

            print("✓ Test environment setup completed")
            return True

        except Exception as e:
            print(f"✗ Test environment setup failed: {e}")
            return False

    def _export_page_as_pdf(self, file_path):
        """Helper to export the current page to PDF using TechDraw API"""
        try:
            result = self.test_page.exportToPDF(file_path)
        except Exception as exc:
            return False, exc
        return bool(result), None

    def _export_page_as_svg(self, file_path):
        """Helper to export the current page to SVG using TechDraw API"""
        try:
            result = self.test_page.exportToSVG(file_path)
        except Exception as exc:
            return False, exc
        return bool(result), None

    def _export_svg_to_string(self):
        """Export the page as SVG and return its contents"""
        fd, temp_name = tempfile.mkstemp(prefix='techdraw_svg_', suffix='.svg', dir=self.temp_dir)
        os.close(fd)
        ok, err = self._export_page_as_svg(temp_name)
        if not ok:
            raise Exception(f"exportPageAsSvg failed: {err}")
        try:
            with open(temp_name, 'r', encoding='utf-8') as svg_file:
                return svg_file.read()
        finally:
            if os.path.exists(temp_name):
                os.remove(temp_name)

    def test_api_availability(self):
        """Test 1: Verify API methods are available"""
        test_name = "API Availability"
        try:
            required = ("exportToPDF", "exportToSVG", "renderToSVGString")
            missing = [name for name in required if not hasattr(self.test_page, name)]
            if missing:
                raise Exception(f"Missing exports: {', '.join(missing)}")

            print(f"✓ {test_name}: All required exports available")
            self.test_results[test_name] = "PASS"
            return True

        except Exception as e:
            print(f"✗ {test_name}: {e}")
            self.test_results[test_name] = f"FAIL: {e}"
            return False

    def test_pdf_export_basic(self):
        """Test 2: Basic PDF export functionality"""
        test_name = "PDF Export Basic"
        try:
            pdf_path = os.path.join(self.temp_dir, 'test_basic.pdf')

            # Test PDF export
            ok, err = self._export_page_as_pdf(pdf_path)

            # Verify result
            if not ok:
                raise Exception(f"exportPageAsPdf failed: {err}")

            if not os.path.exists(pdf_path):
                raise Exception("PDF file was not created")

            file_size = os.path.getsize(pdf_path)
            if file_size == 0:
                raise Exception("PDF file is empty")

            print(f"✓ {test_name}: PDF created successfully ({file_size} bytes)")
            self.test_results[test_name] = f"PASS: {file_size} bytes"
            return True

        except Exception as e:
            print(f"✗ {test_name}: {e}")
            self.test_results[test_name] = f"FAIL: {e}"
            return False

    def test_svg_export_basic(self):
        """Test 3: Basic SVG export functionality"""
        test_name = "SVG Export Basic"
        try:
            svg_path = os.path.join(self.temp_dir, 'test_basic.svg')

            # Test SVG export
            ok, err = self._export_page_as_svg(svg_path)

            # Verify result
            if not ok:
                raise Exception(f"exportPageAsSvg failed: {err}")

            if not os.path.exists(svg_path):
                raise Exception("SVG file was not created")

            file_size = os.path.getsize(svg_path)
            if file_size == 0:
                raise Exception("SVG file is empty")

            # Check if it's valid XML/SVG
            with open(svg_path, 'r') as f:
                content = f.read(200)
                if 'svg' not in content.lower():
                    print(f"Warning: SVG file may not contain valid SVG content")

            print(f"✓ {test_name}: SVG created successfully ({file_size} bytes)")
            self.test_results[test_name] = f"PASS: {file_size} bytes"
            return True

        except Exception as e:
            print(f"✗ {test_name}: {e}")
            self.test_results[test_name] = f"FAIL: {e}"
            return False

    def test_svg_string_rendering(self):
        """Test 4: SVG string rendering"""
        test_name = "SVG String Rendering"
        try:
            # Test SVG string rendering
            svg_content = self._export_svg_to_string()

            if not svg_content:
                raise Exception("SVG export returned empty string")

            if len(svg_content) < 10:
                raise Exception("SVG string too short")

            # Basic SVG validation
            if 'svg' not in svg_content.lower():
                print(f"Warning: SVG string may not contain valid SVG content")

            print(f"✓ {test_name}: SVG string generated ({len(svg_content)} characters)")
            self.test_results[test_name] = f"PASS: {len(svg_content)} chars"
            return True

        except Exception as e:
            print(f"✗ {test_name}: {e}")
            self.test_results[test_name] = f"FAIL: {e}"
            return False

    def test_error_handling(self):
        """Test 5: Error handling for invalid inputs"""
        test_name = "Error Handling"
        try:
            invalid_path = os.path.join(self.temp_dir, "missing", "fail.pdf")
            ok_pdf, err_pdf = self._export_page_as_pdf(invalid_path)
            if ok_pdf:
                raise Exception("Export to invalid path unexpectedly succeeded")

            ok_svg, err_svg = self._export_page_as_svg(os.path.join(self.temp_dir, "missing", "fail.svg"))
            if ok_svg:
                raise Exception("SVG export to invalid path unexpectedly succeeded")

            ok_empty, err_empty = self._export_page_as_pdf("")
            if ok_empty:
                raise Exception("Export with empty path unexpectedly succeeded")

            print(f"✓ {test_name}: Invalid path handling behaves as expected")
            self.test_results[test_name] = "PASS"
            return True

        except Exception as e:
            print(f"✗ {test_name}: {e}")
            self.test_results[test_name] = f"FAIL: {e}"
            return False

    def test_with_template(self):
        """Test 6: Export with repository SVG template"""
        test_name = "Template Export"
        try:
            if not os.path.exists(TEMPLATE_FILE):
                print(f"~ {test_name}: Template file not found, skipping")
                self.test_results[test_name] = "SKIP: template missing"
                return True

            template = self.test_doc.addObject('TechDraw::DrawSVGTemplate', 'RepoTemplate')
            template.Template = TEMPLATE_FILE
            self.test_page.Template = template
            self.test_doc.recompute()

            svg_path = os.path.join(self.temp_dir, 'with_template.svg')
            ok, err = self._export_page_as_svg(svg_path)
            if not ok:
                raise Exception(f"Template export failed: {err}")

            self.assert_file_exists(svg_path)
            print(f"✓ {test_name}: Template export succeeded")
            self.test_results[test_name] = "PASS"
            return True

        except Exception as e:
            print(f"✗ {test_name}: {e}")
            self.test_results[test_name] = f"FAIL: {e}"
            return False

    # Utility assertions -------------------------------------------------

    def assert_file_exists(self, path):
        if not os.path.exists(path):
            raise AssertionError(f"Expected file missing: {path}")
        if os.path.getsize(path) <= 0:
            raise AssertionError(f"Generated file empty: {path}")

    def cleanup(self):
        """Clean up test environment"""
        try:
            if self.test_doc:
                App.closeDocument(self.test_doc.Name)

            # Clean up temporary files
            import shutil
            shutil.rmtree(self.temp_dir, ignore_errors=True)

            print(f"✓ Cleanup completed")

        except Exception as e:
            print(f"Warning: Cleanup failed: {e}")

    def run_all_tests(self):
        """Run all tests and generate report"""
        print("=" * 60)
        print("TechDraw Headless Export Test Suite")
        print("=" * 60)

        if not FREECAD_AVAILABLE:
            print("✗ FreeCAD not available - cannot run tests")
            return False

        # Setup
        if not self.setup_test_environment():
            print("✗ Test environment setup failed - aborting tests")
            return False

        # Run tests
        tests = [
            self.test_api_availability,
            self.test_pdf_export_basic,
            self.test_svg_export_basic,
            self.test_svg_string_rendering,
            self.test_error_handling,
            self.test_with_template,
        ]

        passed = 0
        failed = 0

        for test in tests:
            print("-" * 40)
            try:
                if test():
                    passed += 1
                else:
                    failed += 1
            except Exception as e:
                print(f"✗ Test {test.__name__} crashed: {e}")
                traceback.print_exc()
                failed += 1

        # Generate report
        print("=" * 60)
        print("TEST RESULTS SUMMARY")
        print("=" * 60)

        total = passed + failed
        print(f"Total tests: {total}")
        print(f"Passed: {passed}")
        print(f"Failed: {failed}")
        print(f"Success rate: {(passed/total*100):.1f}%" if total > 0 else "N/A")

        print("\nDetailed Results:")
        for test_name, result in self.test_results.items():
            status = "✓" if result.startswith("PASS") else "✗" if result.startswith("FAIL") else "~"
            print(f"  {status} {test_name}: {result}")

        print(f"\nTemporary files location: {self.temp_dir}")
        print("Note: Inspect generated PDF/SVG files manually for quality validation")

        # Cleanup
        self.cleanup()

        return failed == 0

def main():
    """Main test execution"""
    if len(sys.argv) > 1 and sys.argv[1] == '--help':
        print(__doc__)
        return

    test_suite = TechDrawHeadlessTestSuite()
    success = test_suite.run_all_tests()

    if success:
        print("\n🎉 All tests passed!")
        sys.exit(0)
    else:
        print("\n❌ Some tests failed. Check the results above.")
        sys.exit(1)

if __name__ == "__main__":
    main()
