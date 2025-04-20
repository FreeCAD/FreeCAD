# -*- coding: utf-8 -*-
# Unit tests for the Path.Tool.Shape module and its utilities.

import unittest
from pathlib import Path
from unittest.mock import patch, MagicMock, call
from Path.Tool.Shape import util, ToolBitShapeBallEnd, ToolBitShapeVBit

mock_freecad = MagicMock(Name="FreeCAD_Mock")
mock_freecad.Console = MagicMock()
mock_freecad.Console.PrintWarning = MagicMock()
mock_freecad.Console.PrintError = MagicMock()

mock_obj = MagicMock(Name="Object_Mock")
mock_obj.Label = "MockObjectLabel"
mock_obj.Name = "MockObjectName"

mock_doc = MagicMock(Name="Document_Mock")
mock_doc.Objects = [mock_obj]


class TestPathToolShapeUtil(unittest.TestCase):
    def setUp(self):
        """Reset mocks before each test."""
        mock_freecad.Console.PrintWarning.reset_mock()
        mock_freecad.Console.PrintError.reset_mock()
        mock_freecad.openDocument.reset_mock()
        mock_freecad.closeDocument.reset_mock()
        mock_doc.reset_mock()
        mock_obj.reset_mock()
        mock_doc.Objects = [mock_obj]  # Reset objects list
        mock_obj.Label = "MockObjectLabel"
        mock_obj.Name = "MockObjectName"
        # Clear any dynamically added attributes from previous tests
        for attr in list(mock_obj.__dict__.keys()):
            if not attr.startswith("_") and attr not in [
                "Label",
                "Name",
                "reset_mock",
            ]:
                try:
                    delattr(mock_obj, attr)
                except AttributeError:
                    pass  # Ignore errors if attribute is already gone

    def test_get_shape_class(self):
        """Test the get_shape_class function."""
        self.assertEqual(
            util.get_shape_class_from_name("ballend"),
            ToolBitShapeBallEnd)
        self.assertEqual(
            util.get_shape_class_from_name("v-bit"),
            ToolBitShapeVBit)
        self.assertEqual(
            util.get_shape_class_from_name("vbit"),  # test using alias
            ToolBitShapeVBit)
        self.assertIsNone(util.get_shape_class_from_name("nonexistent"))

# Test execution
if __name__ == "__main__":
    unittest.main()
