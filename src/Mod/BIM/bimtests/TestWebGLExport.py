# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2025 baidakovil <baidakovil@icloud.com>                 *
# *                                                                         *
# *   This file is part of FreeCAD.                                         *
# *                                                                         *
# *   FreeCAD is free software: you can redistribute it and/or modify it    *
# *   under the terms of the GNU Lesser General Public License as           *
# *   published by the Free Software Foundation, either version 2.1 of the  *
# *   License, or (at your option) any later version.                       *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful, but        *
# *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
# *   Lesser General Public License for more details.                       *
# *                                                                         *
# *   You should have received a copy of the GNU Lesser General Public      *
# *   License along with FreeCAD. If not, see                               *
# *   <https://www.gnu.org/licenses/>.                                      *
# *                                                                         *
# ***************************************************************************


"""
Unit tests for WebGL export functionality.
Tests both the template handling and the main export function.
"""

import os
import tempfile
import unittest
from unittest.mock import patch, MagicMock, mock_open

import FreeCAD
from BIM.importers import importWebGL


class TestWebGLExport(unittest.TestCase):

    def setUp(self):
        self.test_dir = tempfile.mkdtemp()
        self.test_template_content = """
        <!DOCTYPE html>
        <html>
        <head><title>$pagetitle</title></head>
        <body>WebGL Content: $data</body>
        </html>
        """

    def tearDown(self):
        import shutil

        shutil.rmtree(self.test_dir, ignore_errors=True)

    # === INTEGRATION TESTS (Real File System) ===

    def test_actual_default_template_readable_and_valid(self):
        """Test that the actual default template can be read and contains
        required placeholders"""
        # Test with real configuration - no mocks
        with patch(
            "BIM.importers.importWebGL.params.get_param", return_value=False
        ):  # Disable custom template
            result = importWebGL.getHTMLTemplate()

            if result is not None:  # Only test if template exists
                self.assertIsInstance(result, str)

                # Check for basic HTML structure
                self.assertIn("<html", result.lower())
                self.assertIn("$pagetitle", result.lower())
                self.assertIn("$data", result.lower())

    def test_custom_template_with_real_file(self):
        """Test custom template functionality with real temporary file"""
        # Create real temporary template file
        custom_path = os.path.join(self.test_dir, "custom_template.html")
        with open(custom_path, "w", encoding="utf-8") as f:
            f.write(self.test_template_content)

        with patch(
            "BIM.importers.importWebGL.params.get_param"
        ) as mock_params:
            mock_params.side_effect = lambda param, path=None: {
                "useCustomWebGLExportTemplate": True,
                "WebGLTemplateCustomPath": custom_path,
            }.get(param, False)

            result = importWebGL.getHTMLTemplate()
            self.assertIsNotNone(result)
            self.assertEqual(
                result.strip(), self.test_template_content.strip()
            )

    # === UNIT TESTS (Logic Paths with Mocks) ===

    def test_default_template_logic_when_custom_disabled(self):
        """Test code path when custom template is disabled"""
        with patch(
            "BIM.importers.importWebGL.params.get_param", return_value=False
        ):
            with patch("os.path.isfile", return_value=True):
                with patch(
                    "builtins.open",
                    mock_open(read_data=self.test_template_content),
                ):
                    result = importWebGL.getHTMLTemplate()
                    self.assertIsNotNone(result)
                    if result is not None:
                        self.assertIn("WebGL Content", result)

    def test_custom_template_not_found_headless_mode(self):
        """Test behavior when custom template not found in headless mode"""
        with patch(
            "BIM.importers.importWebGL.params.get_param"
        ) as mock_params:
            mock_params.side_effect = lambda param, path=None: {
                "useCustomWebGLExportTemplate": True,
                "WebGLTemplateCustomPath": "/nonexistent/template.html",
            }.get(param, False)

            with patch(
                "BIM.importers.importWebGL.FreeCADGui", None
            ):  # Simulate headless mode
                result = importWebGL.getHTMLTemplate()
                self.assertIsNone(result)

    @patch("BIM.importers.importWebGL.FreeCADGui")
    def test_custom_template_not_found_gui_user_accepts_fallback(
        self, mock_gui
    ):
        """Test GUI dialog when custom template not found -
        user accepts fallback"""
        from PySide import QtWidgets

        mock_gui.getMainWindow.return_value = MagicMock()

        with patch(
            "BIM.importers.importWebGL.params.get_param"
        ) as mock_params:
            mock_params.side_effect = lambda param, path=None: {
                "useCustomWebGLExportTemplate": True,
                "WebGLTemplateCustomPath": "/nonexistent/template.html",
            }.get(param, False)

            with patch(
                "PySide.QtWidgets.QMessageBox.question",
                return_value=QtWidgets.QMessageBox.Yes,
            ):
                with patch("os.path.isfile", return_value=True):
                    with patch(
                        "builtins.open",
                        mock_open(read_data=self.test_template_content),
                    ):
                        result = importWebGL.getHTMLTemplate()
                        self.assertIsNotNone(result)

    @patch("BIM.importers.importWebGL.FreeCADGui")
    def test_custom_template_not_found_gui_user_rejects_fallback(
        self, mock_gui
    ):
        """Test GUI dialog when custom template not found -
        user rejects fallback"""
        from PySide import QtWidgets

        mock_gui.getMainWindow.return_value = MagicMock()

        with patch(
            "BIM.importers.importWebGL.params.get_param"
        ) as mock_params:
            mock_params.side_effect = lambda param, path=None: {
                "useCustomWebGLExportTemplate": True,
                "WebGLTemplateCustomPath": "/nonexistent/template.html",
            }.get(param, False)

            with patch(
                "PySide.QtWidgets.QMessageBox.question",
                return_value=QtWidgets.QMessageBox.No,
            ):
                result = importWebGL.getHTMLTemplate()
                self.assertIsNone(result)

    # === MAIN EXPORT FUNCTION TEST ===

    def test_export_returns_false_when_no_template(self):
        """Test that export function returns False when
        no template is available"""
        with patch(
            "BIM.importers.importWebGL.getHTMLTemplate", return_value=None
        ):
            with patch("BIM.importers.importWebGL.FreeCADGui") as mock_gui:
                # Mock the GUI components that might be accessed
                mock_active_doc = MagicMock()
                mock_active_doc.ActiveView = MagicMock()
                mock_gui.ActiveDocument = mock_active_doc

                result = importWebGL.export(
                    [], os.path.join(self.test_dir, "test.html")
                )
                self.assertFalse(result)

    def test_export_returns_true_when_template_available(self):
        """Test that export function returns True when template is available"""
        mock_template = """<html><body>
            $pagetitle $version $data $threejs_version
            </body></html>"""

        with patch(
            "BIM.importers.importWebGL.getHTMLTemplate",
            return_value=mock_template,
        ):
            with patch(
                "BIM.importers.importWebGL.FreeCAD.ActiveDocument"
            ) as mock_doc:
                mock_doc.Label = "Test Document"
                with patch("BIM.importers.importWebGL.FreeCADGui") as mock_gui:
                    # Mock the GUI components that might be accessed
                    mock_active_doc = MagicMock()
                    mock_active_doc.ActiveView = MagicMock()
                    mock_gui.ActiveDocument = mock_active_doc

                    # Mock the functions that populate data to return JSON-serializable values
                    with patch(
                        "BIM.importers.importWebGL.populate_camera"
                    ) as mock_populate_camera:
                        with patch(
                            "BIM.importers.importWebGL.Draft.get_group_contents",
                            return_value=[],
                        ):
                            mock_populate_camera.return_value = (
                                None  # Modifies data dict in place
                            )

                            result = importWebGL.export(
                                [], os.path.join(self.test_dir, "test.html")
                            )
                            self.assertTrue(result)


if __name__ == "__main__":
    # Allow running tests directly
    unittest.main(verbosity=2)
