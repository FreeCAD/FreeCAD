# -*- coding: utf-8 -*-
# Unit tests for the Path.Tool.Shape module and its document utilities.
import os
import unittest
from unittest.mock import patch, MagicMock
import FreeCAD
from Path.Tool.docobject import util, DetachedDocumentObject


mock_freecad = MagicMock(Name="FreeCAD_Mock")
mock_freecad.Console = MagicMock()
mock_freecad.Console.PrintWarning = MagicMock()
mock_freecad.Console.PrintError = MagicMock()

mock_obj = MagicMock(Name="Object_Mock")
mock_obj.Label = "MockObjectLabel"
mock_obj.Name = "MockObjectName"

mock_doc = MagicMock(Name="Document_Mock")
mock_doc.Objects = [mock_obj]


class TestPathToolDocObject(unittest.TestCase):
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

    def test_doc_get_object_properties_found(self):
        """Test get_object_properties extracts existing properties."""
        setattr(mock_obj, "Diameter", "10 mm")
        setattr(mock_obj, "Length", "50 mm")
        params = util.get_object_properties(mock_obj, ["Diameter", "Length"])
        # Expecting just the values, not tuples
        self.assertEqual(params, {"Diameter": "10 mm", "Length": "50 mm"})
        mock_freecad.Console.PrintWarning.assert_not_called()

    def test_doc_get_object_properties_missing(self):
        """Test get_object_properties handles missing properties with warning."""
        setattr(mock_obj, "Diameter", "10 mm")
        # Explicitly delete Height to ensure hasattr returns False for MagicMock
        if hasattr(mock_obj, "Height"):
            delattr(mock_obj, "Height")
        params = util.get_object_properties(mock_obj, ["Diameter", "Height"])
        # Expecting just the values, not tuples
        self.assertEqual(params, {"Diameter": "10 mm", "Height": None})  # Height is missing

    @patch("FreeCAD.openDocument")
    @patch("FreeCAD.getDocument")
    @patch("FreeCAD.closeDocument")
    def test_DocFromBytes(self, mock_close_doc, mock_get_doc, mock_open_doc):
        """Test DocFromBytes loads doc from a byte string."""
        content = b"fake_content"
        mock_opened_doc = MagicMock(Name="OpenedDoc_Mock")
        mock_get_doc.return_value = mock_opened_doc

        temp_file_path = None
        try:
            with util.DocFromBytes(content=content) as temp_doc:
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
    def test_DocFromBytes_open_exception(self, mock_close_doc, mock_get_doc, mock_open_doc):
        """Test DocFromBytes propagates exceptions and cleans up."""
        content = b"fake_content_exception"
        load_error = Exception("Fake load error")
        mock_open_doc.side_effect = load_error

        temp_file_path = None
        try:
            with self.assertRaises(Exception) as cm:
                with util.DocFromBytes(content=content):
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
    def test_DocFromBytes_exit_cleans_up(self, mock_close_doc, mock_get_doc, mock_open_doc):
        """Test DocFromBytes __exit__ cleans up temp file."""
        content = b"fake_content_cleanup"
        mock_opened_doc = MagicMock(Name="OpenedDoc_Cleanup_Mock")
        mock_get_doc.return_value = mock_opened_doc

        temp_file_path = None
        try:
            with util.DocFromBytes(content=content):
                mock_open_doc.assert_called_once()
                temp_file_path = mock_open_doc.call_args[0][0]
                self.assertTrue(os.path.exists(temp_file_path))
                with open(temp_file_path, "rb") as f:
                    self.assertEqual(f.read(), content)

            # Verify cleanup after exiting the context
            mock_close_doc.assert_called_once_with(mock_open_doc.return_value.Name)
            self.assertFalse(os.path.exists(temp_file_path))

        finally:
            # Ensure cleanup even if test fails before assertion
            if temp_file_path and os.path.exists(temp_file_path):
                os.remove(temp_file_path)


class TestDetachedDocumentObject(unittest.TestCase):
    def setUp(self):
        self.mock_path_log = MagicMock(name="Path_Log_Mock")
        self.patcher_path_log = patch("Path.Log", new=self.mock_path_log)
        self.patcher_path_log.start()

    def tearDown(self):
        self.patcher_path_log.stop()

    def test_init(self):
        obj = DetachedDocumentObject("TestLabel")
        self.assertEqual(obj.Label, "TestLabel")
        self.assertEqual(obj.Name, "TestLabel")
        self.assertEqual(obj.PropertiesList, [])
        self.assertEqual(obj._properties, {})
        self.assertEqual(obj._property_groups, {})
        self.assertEqual(obj._property_types, {})
        self.assertEqual(obj._property_docs, {})
        self.assertEqual(obj._editor_modes, {})
        self.assertEqual(obj._property_enums, {})

    def test_addProperty(self):
        obj = DetachedDocumentObject()
        obj.addProperty("App::PropertyString", "MyString", "Group1", "Doc for MyString")
        self.assertIn("MyString", obj.PropertiesList)
        self.assertIsNone(obj.MyString)
        self.assertEqual(obj._property_groups["MyString"], "Group1")
        self.assertEqual(obj._property_types["MyString"], "App::PropertyString")
        self.assertEqual(obj._property_docs["MyString"], "Doc for MyString")

    @patch("FreeCAD.Units.Quantity")
    def test_addProperty_quantity(self, mock_quantity_class):
        obj = DetachedDocumentObject()
        obj.addProperty("App::PropertyLength", "MyLength", "Group2", "Doc for MyLength")
        self.assertIn("MyLength", obj.PropertiesList)
        mock_quantity_class.assert_called_once_with(0.0)
        self.assertEqual(obj.MyLength, mock_quantity_class.return_value)

    def test_getPropertyByName(self):
        obj = DetachedDocumentObject()
        obj.addProperty("App::PropertyString", "MyString", "Group1", "Doc for MyString")
        obj.MyString = "Hello"
        self.assertEqual(obj.getPropertyByName("MyString"), "Hello")

    def test_setPropertyByName(self):
        obj = DetachedDocumentObject()
        obj.addProperty("App::PropertyString", "MyString", "Group1", "Doc for MyString")
        obj.setPropertyByName("MyString", "World")
        self.assertEqual(obj.MyString, "World")

    def test_setattr_new_property(self):
        obj = DetachedDocumentObject()
        obj.NewProp = 123
        self.assertEqual(obj.NewProp, 123)
        self.assertIn("NewProp", obj._properties)
        self.assertNotIn("NewProp", obj.PropertiesList)  # Not added via addProperty

    @patch("FreeCAD.Units.Quantity")
    def test_setattr_quantity_conversion(self, mock_quantity_class):
        obj = DetachedDocumentObject()
        obj.addProperty("App::PropertyLength", "Length", "Group", "Doc")
        obj.Length = "10 mm"
        mock_quantity_class.assert_called_with("10 mm")
        self.assertEqual(obj.Length, mock_quantity_class.return_value)

    def test_setattr_enumeration(self):
        obj = DetachedDocumentObject()
        obj.addProperty("App::PropertyEnumeration", "ToolShape", "Group", "Doc")
        enum_choices = ["Ball", "Flat", "V-Bit"]
        obj.ToolShape = enum_choices
        self.assertEqual(obj.getEnumerationsOfProperty("ToolShape"), enum_choices)
        self.assertEqual(obj.ToolShape, "Ball")  # Default to first choice

    def test_setattr_enumeration_empty_list_raises_error(self):
        obj = DetachedDocumentObject()
        obj.addProperty("App::PropertyEnumeration", "ToolShape", "Group", "Doc")
        with self.assertRaisesRegex(
            AssertionError, "Enum property 'ToolShape' must have at least one entry"
        ):
            obj.ToolShape = []

    def test_getattr_existing_property(self):
        obj = DetachedDocumentObject()
        obj.addProperty("App::PropertyString", "MyString", "Group1", "Doc for MyString")
        obj.MyString = "TestValue"
        self.assertEqual(obj.MyString, "TestValue")

    def test_getattr_missing_property(self):
        obj = DetachedDocumentObject()
        with self.assertRaisesRegex(
            AttributeError, "'DetachedDocumentObject' object has no attribute 'NonExistent'"
        ):
            _ = obj.NonExistent

    def test_set_get_editor_mode(self):
        obj = DetachedDocumentObject()
        obj.setEditorMode("MyProp", 1)
        self.assertEqual(obj.getEditorMode("MyProp"), 1)
        self.assertEqual(obj.getEditorMode("NonExistent"), 0)

    def test_get_group_of_property(self):
        obj = DetachedDocumentObject()
        obj.addProperty("App::PropertyString", "MyString", "Group1", "Doc for MyString")
        self.assertEqual(obj.getGroupOfProperty("MyString"), "Group1")
        self.assertIsNone(obj.getGroupOfProperty("NonExistent"))

    def test_get_type_id_of_property(self):
        obj = DetachedDocumentObject()
        obj.addProperty("App::PropertyString", "MyString", "Group1", "Doc for MyString")
        self.assertEqual(obj.getTypeIdOfProperty("MyString"), "App::PropertyString")
        self.assertIsNone(obj.getTypeIdOfProperty("NonExistent"))

    def test_get_enumerations_of_property(self):
        obj = DetachedDocumentObject()
        obj.addProperty("App::PropertyEnumeration", "ToolShape", "Group", "Doc")
        enum_choices = ["Ball", "Flat"]
        obj.ToolShape = enum_choices
        self.assertEqual(obj.getEnumerationsOfProperty("ToolShape"), enum_choices)
        self.assertEqual(obj.getEnumerationsOfProperty("NonExistent"), [])

    def test_expression_engine(self):
        obj = DetachedDocumentObject()
        self.assertEqual(obj.ExpressionEngine, [])

    def test_copy_to(self):
        detached_obj = DetachedDocumentObject("DetachedTool")
        detached_obj.addProperty("App::PropertyString", "ToolName", "Base", "Name of the tool")
        detached_obj.ToolName = "EndMill"
        detached_obj.addProperty("App::PropertyLength", "Diameter", "Dimensions", "Tool diameter")
        detached_obj.Diameter = "10 mm"  # Store as string
        detached_obj.addProperty(
            "App::PropertyEnumeration", "ToolShape", "Shape", "Shape of the tool"
        )
        detached_obj.ToolShape = ["Ball", "Flat"]
        detached_obj.setEditorMode("ToolName", 1)

        doc = None
        target_obj = None
        try:
            doc = FreeCAD.newDocument("TestDocForCopyTo")
            target_obj = doc.addObject("App::FeaturePython", "RealTool")
            target_obj.Label = "Real Tool"

            detached_obj.copy_to(target_obj)

            # Verify properties exist and values are set
            self.assertTrue(hasattr(target_obj, "ToolName"))
            self.assertEqual(target_obj.ToolName, "EndMill")

            self.assertTrue(hasattr(target_obj, "Diameter"))
            # Check if it's a FreeCAD.Units.Quantity and its value
            self.assertIsInstance(target_obj.Diameter, FreeCAD.Units.Quantity)
            self.assertEqual(str(target_obj.Diameter), "10.0 mm")

            self.assertTrue(hasattr(target_obj, "ToolShape"))
            # For enumeration, verify the enum choices and the current value
            self.assertEqual(target_obj.getEnumerationsOfProperty("ToolShape"), ["Ball", "Flat"])
            self.assertEqual(target_obj.ToolShape, "Ball")

            # Editor mode is set, but difficult to verify directly on a real object
            # without more FreeCAD API knowledge. Rely on the copy_to logic.

        finally:
            if doc:
                FreeCAD.closeDocument(doc.Name)

    def test_copy_to_property_exists(self):
        detached_obj = DetachedDocumentObject("DetachedTool")
        detached_obj.addProperty("App::PropertyString", "ToolName", "Base", "Name of the tool")
        detached_obj.ToolName = "EndMill"

        doc = None
        target_obj = None
        try:
            doc = FreeCAD.newDocument("TestDocForCopyToExists")
            target_obj = doc.addObject("App::FeaturePython", "RealToolExists")
            target_obj.Label = "Real Tool Exists"
            # Manually add the property to simulate it already existing
            target_obj.addProperty("App::PropertyString", "ToolName", "Base", "Name of the tool")
            target_obj.ToolName = "ExistingTool"

            detached_obj.copy_to(target_obj)

            # Verify the value is updated
            self.assertEqual(target_obj.ToolName, "EndMill")

        finally:
            if doc:
                FreeCAD.closeDocument(doc.Name)


# Test execution
if __name__ == "__main__":
    unittest.main()
