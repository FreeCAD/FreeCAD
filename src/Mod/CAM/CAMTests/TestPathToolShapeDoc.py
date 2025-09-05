# -*- coding: utf-8 -*-
# Unit tests for the Path.Tool.Shape module and its document utilities.
import unittest
from unittest.mock import patch, MagicMock, call
from Path.Tool.shape import doc
import os


mock_freecad = MagicMock(Name="FreeCAD_Mock")
mock_freecad.Console = MagicMock()
mock_freecad.Console.PrintWarning = MagicMock()
mock_freecad.Console.PrintError = MagicMock()

mock_obj = MagicMock(Name="Object_Mock")
mock_obj.Label = "MockObjectLabel"
mock_obj.Name = "MockObjectName"

mock_doc = MagicMock(Name="Document_Mock")
mock_doc.Objects = [mock_obj]


class TestPathToolShapeDoc(unittest.TestCase):
    def setUp(self):
        """Reset mocks before each test."""
        # Resetting the top-level mock recursively resets its children
        # (newDocument, getDocument, openDocument, closeDocument, Console, etc.)
        # and their call counts, return_values, side_effects.
        mock_freecad.reset_mock()
        mock_doc.reset_mock()
        mock_obj.reset_mock()

        # Re-establish default state/attributes potentially cleared by reset_mock
        # or needed for tests.
        mock_doc.Objects = [mock_obj]
        mock_obj.Label = "MockObjectLabel"
        mock_obj.Name = "MockObjectName"
        mock_obj.getTypeIdOfProperty = MagicMock(return_value="App::PropertyString")
        # Ensure mock_doc also has a Name attribute used in tests/code
        mock_doc.Name = "Document_Mock"  # Used in closeDocument calls

        # Clear attributes potentially added by setattr in previous tests.
        # reset_mock() doesn't remove attributes added this way.
        # Focus on attributes known to be added by tests in this file.
        for attr_name in ["Diameter", "Length", "Height"]:
            if hasattr(mock_obj, attr_name):
                try:
                    delattr(mock_obj, attr_name)
                except AttributeError:
                    pass  # Ignore if already gone

    """Tests for the document utility functions in Path/Tool/Shape/doc.py"""

    def test_doc_find_shape_object_body_priority(self):
        """Test find_shape_object prioritizes PartDesign::Body."""
        body_obj = MagicMock(Name="Body_Mock")
        body_obj.isDerivedFrom = lambda typeName: typeName == "PartDesign::Body"
        part_obj = MagicMock(Name="Part_Mock")
        part_obj.isDerivedFrom = lambda typeName: typeName == "Part::Feature"
        mock_doc.Objects = [part_obj, body_obj]
        found = doc.find_shape_object(mock_doc)
        self.assertEqual(found, body_obj)

    def test_doc_find_shape_object_part_fallback(self):
        """Test find_shape_object falls back to Part::Feature."""
        part_obj = MagicMock(Name="Part_Mock")
        part_obj.isDerivedFrom = lambda typeName: typeName == "Part::Feature"
        other_obj = MagicMock(Name="Other_Mock")
        other_obj.isDerivedFrom = lambda typeName: False
        mock_doc.Objects = [other_obj, part_obj]
        found = doc.find_shape_object(mock_doc)
        self.assertEqual(found, part_obj)

    def test_doc_find_shape_object_first_obj_fallback(self):
        """Test find_shape_object falls back to the first object."""
        other_obj1 = MagicMock(Name="Other1_Mock")
        other_obj1.isDerivedFrom = lambda typeName: False
        other_obj2 = MagicMock(Name="Other2_Mock")
        other_obj2.isDerivedFrom = lambda typeName: False
        mock_doc.Objects = [other_obj1, other_obj2]
        found = doc.find_shape_object(mock_doc)
        self.assertEqual(found, other_obj1)

    def test_doc_find_shape_object_no_objects(self):
        """Test find_shape_object returns None if document has no objects."""
        mock_doc.Objects = []
        found = doc.find_shape_object(mock_doc)
        self.assertIsNone(found)

    def test_doc_get_object_properties_found(self):
        """Test get_object_properties extracts existing properties."""
        setattr(mock_obj, "Diameter", "10 mm")
        setattr(mock_obj, "Length", "50 mm")
        params = doc.get_object_properties(mock_obj, ["Diameter", "Length"])
        # Expecting just the values, not tuples
        self.assertEqual(
            params,
            {
                "Diameter": ("10 mm", "App::PropertyString"),
                "Length": ("50 mm", "App::PropertyString"),
            },
        )
        mock_freecad.Console.PrintWarning.assert_not_called()

    def test_doc_get_object_properties_missing(self):
        """Test get_object_properties handles missing properties with warning."""
        # Re-import doc within the patch context to use the mocked FreeCAD
        import Path.Tool.shape.doc as doc_patched

        setattr(mock_obj, "Diameter", "10 mm")
        # Explicitly delete Height to ensure hasattr returns False for MagicMock
        if hasattr(mock_obj, "Height"):
            delattr(mock_obj, "Height")
        params = doc_patched.get_object_properties(mock_obj, ["Diameter", "Height"])
        # Expecting just the values, not tuples
        self.assertEqual(
            params,
            {
                "Diameter": ("10 mm", "App::PropertyString"),
                "Height": (None, "App::PropertyString"),
            },
        )  # Height is missing

    @patch("FreeCAD.openDocument")
    @patch("FreeCAD.getDocument")
    @patch("FreeCAD.closeDocument")
    def test_ShapeDocFromBytes(self, mock_close_doc, mock_get_doc, mock_open_doc):
        """Test ShapeDocFromBytes loads doc from a byte string."""
        content = b"fake_content"
        mock_opened_doc = MagicMock(Name="OpenedDoc_Mock")
        mock_get_doc.return_value = mock_opened_doc

        temp_file_path = None
        try:
            with doc.ShapeDocFromBytes(content=content) as temp_doc:
                # Verify temp file creation and content
                mock_open_doc.assert_called_once()
                temp_file_path = mock_open_doc.call_args[0][0]
                self.assertTrue(os.path.exists(temp_file_path))
                with open(temp_file_path, "rb") as f:
                    self.assertEqual(f.read(), content)

                self.assertEqual(temp_doc, mock_open_doc.return_value)

            # Verify cleanup after exiting the context
            mock_close_doc.assert_called_once_with(mock_open_doc.return_value.Name)
            self.assertFalse(os.path.exists(temp_file_path))

        finally:
            # Ensure cleanup even if test fails before assertion
            if temp_file_path and os.path.exists(temp_file_path):
                os.remove(temp_file_path)

    @patch("FreeCAD.openDocument")
    @patch("FreeCAD.getDocument")
    @patch("FreeCAD.closeDocument")
    def test_ShapeDocFromBytes_open_exception(self, mock_close_doc, mock_get_doc, mock_open_doc):
        """Test ShapeDocFromBytes propagates exceptions and cleans up."""
        content = b"fake_content_exception"
        load_error = Exception("Fake load error")
        mock_open_doc.side_effect = load_error

        temp_file_path = None
        try:
            with self.assertRaises(Exception) as cm:
                with doc.ShapeDocFromBytes(content=content):
                    pass

            self.assertEqual(cm.exception, load_error)

            # Verify temp file was created before the exception
            mock_open_doc.assert_called_once()
            temp_file_path = mock_open_doc.call_args[0][0]
            self.assertTrue(os.path.exists(temp_file_path))
            with open(temp_file_path, "rb") as f:
                self.assertEqual(f.read(), content)

            mock_get_doc.assert_not_called()
            # closeDocument is called in __exit__ only if _doc is not None,
            # which it will be if openDocument failed.
            mock_close_doc.assert_not_called()

        finally:
            # Verify cleanup after exiting the context (even with exception)
            if temp_file_path and os.path.exists(temp_file_path):
                os.remove(temp_file_path)
            # Assert removal only if temp_file_path was set
            if temp_file_path:
                self.assertFalse(os.path.exists(temp_file_path))

    @patch("FreeCAD.openDocument")
    @patch("FreeCAD.getDocument")
    @patch("FreeCAD.closeDocument")
    def test_ShapeDocFromBytes_exit_cleans_up(self, mock_close_doc, mock_get_doc, mock_open_doc):
        """Test ShapeDocFromBytes __exit__ cleans up temp file."""
        content = b"fake_content_cleanup"
        mock_opened_doc = MagicMock(Name="OpenedDoc_Cleanup_Mock")
        mock_get_doc.return_value = mock_opened_doc

        temp_file_path = None
        try:
            with doc.ShapeDocFromBytes(content=content):
                mock_open_doc.assert_called_once()
                temp_file_path = mock_open_doc.call_args[0][0]
                self.assertTrue(os.path.exists(temp_file_path))
                with open(temp_file_path, "rb") as f:
                    self.assertEqual(f.read(), content)
                # No assertions on the returned doc here, focus is on cleanup
                pass  # Exit the context

            # Verify cleanup after exiting the context
            mock_close_doc.assert_called_once_with(mock_open_doc.return_value.Name)
            self.assertFalse(os.path.exists(temp_file_path))

        finally:
            # Ensure cleanup even if test fails before assertion
            if temp_file_path and os.path.exists(temp_file_path):
                os.remove(temp_file_path)


# Test execution
if __name__ == "__main__":
    unittest.main()
