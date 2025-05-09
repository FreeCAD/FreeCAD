# -*- coding: utf-8 -*-
# Unit tests for the Path.Tool.Shape module and its utilities.

import CAMTests.PathTestUtils as PathTestUtils
from Path.Tool.shape.registry import SHAPE_REGISTRY
from Path.Tool.shape import (
    ToolBitShape,
    ToolBitShapeBallend,
    ToolBitShapeVBit,
    ToolBitShapeBullnose,
    ToolBitShapeSlittingSaw,
)


class TestPathToolShapeRegistry(PathTestUtils.PathTestBase):
    """Tests for the ShapeRegistry class."""

    def test_get_shape_name_from_filename_from_xml(self):
        """Test extracting shape name from XML."""
        # Use a known file with a PartDesign::Body and a matching shape class
        filename = "ballend.fcstd"
        shape_name = SHAPE_REGISTRY.get_shape_name_from_filename(filename)
        self.assertEqual(shape_name, "Ballend")

    def test_get_shape_class(self):
        """Test the get_shape_class function."""
        self.assertEqual(SHAPE_REGISTRY.get_shape_class_from_name("ballend"), ToolBitShapeBallend)
        self.assertEqual(SHAPE_REGISTRY.get_shape_class_from_name("v-bit"), ToolBitShapeVBit)
        self.assertEqual(SHAPE_REGISTRY.get_shape_class_from_name("VBit"), ToolBitShapeVBit)
        self.assertEqual(SHAPE_REGISTRY.get_shape_class_from_name("torus"), ToolBitShapeBullnose)
        self.assertEqual(
            SHAPE_REGISTRY.get_shape_class_from_name("slitting-saw"), ToolBitShapeSlittingSaw
        )
        self.assertIsNone(SHAPE_REGISTRY.get_shape_class_from_name("nonexistent"))

    def test_get_shape_name_from_filename_from_filename(self):
        """Test inferring shape name from filename when XML extraction fails."""
        # Use a filename that matches a shape class but might not have a body
        # or the XML extraction fails. We'll simulate this by using a non-existent
        # file with a valid shape name in the filename.
        filename = "drill.fcstd"  # Assuming 'drill' is a valid shape alias
        # This test relies on the file not existing or XML extraction failing,
        # which is hard to guarantee without mocks.
        # A more robust test would involve creating a dummy file.
        # For now, we test the fallback logic with a filename.
        shape_name = SHAPE_REGISTRY.get_shape_name_from_filename(filename)
        # The expected behavior is to fall back to inferring from filename
        # if XML extraction fails or file doesn't exist.
        # We expect the inferred name to be 'Drill'.
        self.assertEqual(shape_name, "Drill")

    def test_get_shape_name_from_filename_default(self):
        """Test defaulting to Endmill if name cannot be determined."""
        # Use a filename that does not match any known shape alias
        filename = "unknown_shape.fcstd"
        shape_name = SHAPE_REGISTRY.get_shape_name_from_filename(filename)
        self.assertEqual(shape_name, "Endmill")

    def test_get_shape_from_filename_success(self):
        """Test retrieving a shape instance from a file."""
        filename = "ballend.fcstd"
        shape = SHAPE_REGISTRY.get_shape_from_filename(filename)
        self.assertIsInstance(shape, ToolBitShape)
        self.assertEqual(shape.name, "Ballend")

    def test_get_shape_from_filename_not_found(self):
        """Test raising exception if file and alias not found."""
        filename = "nonexistent_shape.fcstd"
        with self.assertRaises(FileNotFoundError):
            SHAPE_REGISTRY.get_shape_from_filename(filename)
