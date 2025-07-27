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
Unit tests for WebGL export functionality. Gui tests are in `TestWebGLExportGui.py`.
"""

import os
import tempfile
import unittest
from unittest.mock import patch, mock_open

from BIM.importers import importWebGL
from .TestArchBase import TestArchBase


class TestWebGLExport(TestArchBase):

    def setUp(self):
        """Using TestArchBase setUp to initialize the document for convenience,
        but also create a temporary directory for tests."""
        super().setUp()  
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
        super().tearDown()

    def test_actual_default_template_readable_and_valid(self):
        """Test that the actual default template can be read and contains
        required placeholders"""
        operation = "Testing actual default template reading and validation"
        self.printTestMessage(operation)
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
        operation = "Testing custom template with real file"
        self.printTestMessage(operation)
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

    def test_default_template_logic_when_custom_disabled(self):
        """Test code path when custom template is disabled"""
        operation = "Testing default template logic when custom is disabled"
        self.printTestMessage(operation)
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
        operation = "Testing custom template not found in headless mode"
        self.printTestMessage(operation)
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


if __name__ == "__main__":
    # Allow running tests directly
    unittest.main(verbosity=2)
