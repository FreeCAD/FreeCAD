# -*- coding: utf-8 -*-
import unittest
import unittest.mock
import pathlib
from tempfile import TemporaryDirectory
from PySide import QtCore, QtGui
from CAMTests.PathTestUtils import PathTestWithAssets
from Path.Tool.assets import DummyAssetSerializer
from Path.Tool.shape.models.icon import (
    ToolBitShapeIcon,
    ToolBitShapeSvgIcon,
    ToolBitShapePngIcon,
)


class TestToolBitShapeIconBase(PathTestWithAssets):
    """Base class for ToolBitShapeIcon tests."""

    ICON_CLASS = ToolBitShapeIcon

    def setUp(self):
        super().setUp()
        # Ensure a QApplication exists for QPixmap tests
        self.app = QtGui.QApplication.instance()

        # Create a test shape and a test SVG icon.
        self.test_shape = self.assets.get("toolbitshape://ballend")
        self.test_svg = self.test_shape.icon
        assert self.test_svg is not None
        self.icon = self.ICON_CLASS("test_icon_base", b"")

    def tearDown(self):
        self.app = None
        return super().tearDown()

    def test_create_instance(self):
        # Test basic instance creation
        icon_id = "test_icon_123.dat"
        icon = self.ICON_CLASS(icon_id, b"")
        self.assertEqual(icon.get_id(), icon_id)
        self.assertEqual(icon.data, b"")
        self.assertIsInstance(icon.abbreviations, dict)

    def test_to_bytes(self):
        # Test serializing to bytes
        icon_id = "test_to_bytes.bin"
        icon_data = b"some_binary_data"
        icon = ToolBitShapeIcon(icon_id, icon_data)
        self.assertEqual(icon.to_bytes(DummyAssetSerializer), icon_data)

    def test_get_size_in_bytes(self):
        # Test getting icon data length
        icon_with_data = ToolBitShapeIcon("with_data.bin", b"abc")
        self.assertEqual(icon_with_data.get_size_in_bytes(), 3)

        icon_no_data = ToolBitShapeIcon("no_data.dat", b"")
        self.assertEqual(icon_no_data.get_size_in_bytes(), 0)

    @unittest.mock.patch("Path.Tool.shape.util.create_thumbnail_from_data")
    def test_from_shape_data_success(self, mock_create_thumbnail):
        # Test creating instance from shape data - success case
        shape_id = "test_shape"
        thumbnail_data = b"png thumbnail data"
        mock_create_thumbnail.return_value = thumbnail_data
        icon = ToolBitShapeIcon.from_shape_data(self.test_svg.data, shape_id)

        mock_create_thumbnail.assert_called_once_with(self.test_svg.data)
        self.assertIsNotNone(icon)
        self.assertIsInstance(icon, ToolBitShapePngIcon)
        self.assertEqual(icon.get_id(), shape_id)
        self.assertEqual(icon.data, thumbnail_data)
        self.assertEqual(icon.abbreviations, {})

    @unittest.mock.patch("Path.Tool.shape.util.create_thumbnail_from_data")
    def test_from_shape_data_failure(self, mock_create_thumbnail):
        # Test creating instance from shape data - failure case
        shape_id = "test_shape"
        mock_create_thumbnail.return_value = None
        icon_failed = ToolBitShapeIcon.from_shape_data(self.test_svg.data, shape_id)

        mock_create_thumbnail.assert_called_once_with(self.test_svg.data)
        self.assertIsNone(icon_failed)

    def test_get_png(self):
        if not self.app:
            self.skipTest("QApplication not available, skipping test_get_png")
        if type(self) is TestToolBitShapeIconBase:
            self.skipTest("Skipping test on abstract base class")
        # Test getting PNG data from the icon
        icon_size = QtCore.QSize(16, 16)
        png_data = self.icon.get_png(icon_size)
        self.assertIsInstance(png_data, bytes)
        self.assertTrue(len(png_data) > 0)

    def test_get_qpixmap(self):
        if not self.app:
            self.skipTest("QApplication not available, skipping test_get_qpixmap")
        if type(self) is TestToolBitShapeIconBase:
            self.skipTest("Skipping test on abstract base class")
        # Test getting QPixmap from the icon
        icon_size = QtCore.QSize(31, 32)
        pixmap = self.icon.get_qpixmap(icon_size)
        self.assertIsInstance(pixmap, QtGui.QPixmap)
        self.assertFalse(pixmap.isNull())
        self.assertEqual(pixmap.size().width(), 31)
        self.assertEqual(pixmap.size().height(), 32)


class TestToolBitShapeSvgIcon(TestToolBitShapeIconBase):
    """Tests specifically for ToolBitShapeSvgIcon."""

    ICON_CLASS = ToolBitShapeSvgIcon

    def setUp(self):
        super().setUp()
        self.icon = ToolBitShapeSvgIcon("test_icon_svg", self.test_svg.data)

    def test_from_bytes_svg(self):
        # Test creating instance from bytes with SVG
        icon_svg = ToolBitShapeSvgIcon.from_bytes(
            self.test_svg.data, "test_from_bytes.svg", {}, DummyAssetSerializer
        )
        self.assertEqual(icon_svg.get_id(), "test_from_bytes.svg")
        self.assertEqual(icon_svg.data, self.test_svg.data)
        self.assertIsInstance(icon_svg.abbreviations, dict)

    def test_round_trip_serialization_svg(self):
        # Test serialization and deserialization round trip for SVG
        svg_id = "round_trip_svg.svg"
        icon_svg = ToolBitShapeSvgIcon(svg_id, self.test_svg.data)
        serialized_svg = icon_svg.to_bytes(DummyAssetSerializer)
        deserialized_svg = ToolBitShapeSvgIcon.from_bytes(
            serialized_svg, svg_id, {}, DummyAssetSerializer
        )
        self.assertEqual(deserialized_svg.get_id(), svg_id)
        self.assertEqual(deserialized_svg.data, self.test_svg.data)
        # Abbreviations are extracted on access, so we don't check the dict directly
        self.assertIsInstance(deserialized_svg.abbreviations, dict)

    def test_from_file_svg(self):
        # We cannot use NamedTemporaryFile on Windows, because there
        # we may not have permission to read the tempfile while it is
        # still open.
        # So we use TemporaryDirectory instead, to ensure cleanup while
        # still having a the temporary file inside it.
        with TemporaryDirectory() as thedir:
            tempfile = pathlib.Path(thedir, "test.svg")
            tempfile.write_bytes(self.test_svg.data)

            icon_id = "dummy_icon"
            icon = ToolBitShapeIcon.from_file(tempfile, icon_id)
            self.assertIsInstance(icon, ToolBitShapeSvgIcon)
            self.assertEqual(icon.get_id(), icon_id)
            self.assertEqual(icon.data, self.test_svg.data)
            self.assertIsInstance(icon.abbreviations, dict)

    def test_abbreviations_cached_property_svg(self):
        # Test abbreviations property and caching for SVG
        icon_svg = ToolBitShapeSvgIcon("cached_abbr.svg", self.test_svg.data)

        # Accessing the property should call the static method
        with unittest.mock.patch.object(
            ToolBitShapeSvgIcon, "get_abbreviations_from_svg"
        ) as mock_get_abbr:
            mock_get_abbr.return_value = {"param1": "A1"}
            abbr1 = icon_svg.abbreviations
            abbr2 = icon_svg.abbreviations
            mock_get_abbr.assert_called_once_with(self.test_svg.data)
            self.assertEqual(abbr1, {"param1": "A1"})
            self.assertEqual(abbr2, {"param1": "A1"})

    def test_get_abbr_svg(self):
        # Test getting abbreviations for SVG
        icon_data = self.test_svg.data
        icon = ToolBitShapeSvgIcon("abbr_test.svg", icon_data)
        # Assuming the test_svg data has 'diameter' and 'length' ids
        self.assertIsNotNone(icon.get_abbr("Diameter"))
        self.assertIsNotNone(icon.get_abbr("Length"))
        self.assertIsNone(icon.get_abbr("NonExistent"))

    def test_get_abbreviations_from_svg_static(self):
        # Test static method get_abbreviations_from_svg
        svg_content = self.test_svg.data
        abbr = ToolBitShapeSvgIcon.get_abbreviations_from_svg(svg_content)
        # Assuming the test_svg data has 'diameter' and 'length' ids
        self.assertIn("Diameter", abbr)
        self.assertIn("Length", abbr)

        # Test with invalid SVG
        invalid_svg = b"<svg><text id='param1'>A1</text>"  # Missing closing tag
        abbr_invalid = ToolBitShapeSvgIcon.get_abbreviations_from_svg(invalid_svg)
        self.assertEqual(abbr_invalid, {})

        # Test with no text elements
        no_text_svg = b'<svg xmlns="http://www.w3.org/2000/svg"><rect/></svg>'
        abbr_no_text = ToolBitShapeSvgIcon.get_abbreviations_from_svg(no_text_svg)
        self.assertEqual(abbr_no_text, {})


class TestToolBitShapePngIcon(TestToolBitShapeIconBase):
    """Tests specifically for ToolBitShapePngIcon."""

    ICON_CLASS = ToolBitShapePngIcon

    def setUp(self):
        super().setUp()
        self.png_data = b"\x89PNG\r\n\x1a\n"  # Basic PNG signature
        self.icon = ToolBitShapePngIcon("test_icon_png", self.png_data)

    def test_from_bytes_png(self):
        # Test creating instance from bytes with PNG
        icon_png = ToolBitShapePngIcon.from_bytes(
            self.png_data, "test_from_bytes.png", {}, DummyAssetSerializer
        )
        self.assertEqual(icon_png.get_id(), "test_from_bytes.png")
        self.assertEqual(icon_png.data, self.png_data)
        self.assertEqual(icon_png.abbreviations, {})  # No abbreviations for PNG

    def test_round_trip_serialization_png(self):
        # Test serialization and deserialization round trip for PNG
        png_id = "round_trip_png.png"
        serialized_png = self.icon.to_bytes(DummyAssetSerializer)
        deserialized_png = ToolBitShapePngIcon.from_bytes(
            serialized_png, png_id, {}, DummyAssetSerializer
        )
        self.assertEqual(deserialized_png.get_id(), png_id)
        self.assertEqual(deserialized_png.data, self.png_data)
        self.assertEqual(deserialized_png.abbreviations, {})

    def test_from_file_png(self):
        png_data = b"\\x89PNG\\r\\n\\x1a\\n"
        # We cannot use NamedTemporaryFile on Windows, because there
        # we may not have permission to read the tempfile while it is
        # still open.
        # So we use TemporaryDirectory instead, to ensure cleanup while
        # still having a the temporary file inside it.
        with TemporaryDirectory() as thedir:
            tempfile = pathlib.Path(thedir, "test.png")
            tempfile.write_bytes(png_data)

            icon_id = "dummy_icon"
            icon = ToolBitShapeIcon.from_file(tempfile, icon_id)
            self.assertIsInstance(icon, ToolBitShapePngIcon)
            self.assertEqual(icon.get_id(), icon_id)
            self.assertEqual(icon.data, png_data)
            self.assertEqual(icon.abbreviations, {})

    def test_abbreviations_cached_property_png(self):
        # Test abbreviations property and caching for PNG
        self.assertEqual(self.icon.abbreviations, {})

    def test_get_abbr_png(self):
        # Test getting abbreviations for PNG
        self.assertIsNone(self.icon.get_abbr("Diameter"))

    def test_get_qpixmap(self):
        self.skipTest("Skipping test, have no test data")


if __name__ == "__main__":
    unittest.main()
