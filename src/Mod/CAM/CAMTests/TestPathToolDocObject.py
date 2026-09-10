# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2025 Samuel Abels <knipknap@gmail.com>
# SPDX-FileNotice: Part of the FreeCAD project.

################################################################################
#                                                                              #
#   FreeCAD is free software: you can redistribute it and/or modify            #
#   it under the terms of the GNU Lesser General Public License as             #
#   published by the Free Software Foundation, either version 2.1              #
#   of the License, or (at your option) any later version.                     #
#                                                                              #
#   FreeCAD is distributed in the hope that it will be useful,                 #
#   but WITHOUT ANY WARRANTY; without even the implied warranty                #
#   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                    #
#   See the GNU Lesser General Public License for more details.                #
#                                                                              #
#   You should have received a copy of the GNU Lesser General Public           #
#   License along with FreeCAD. If not, see https://www.gnu.org/licenses       #
#                                                                              #
################################################################################

"""Unit tests for DetachedDocumentObject."""

import unittest
from unittest.mock import patch, MagicMock
import FreeCAD
from Path.Tool.docobject import DetachedDocumentObject


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


if __name__ == "__main__":
    unittest.main()
