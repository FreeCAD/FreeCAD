# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2024 FreeCAD Team                                       *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENSE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the          *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************

"""Unit tests for the Property Editor Widgets."""

import unittest
import FreeCAD
from Path.Tool.docobject import DetachedDocumentObject
from Path.Tool.docobject.ui.property import (
    BasePropertyEditorWidget,
    QuantityPropertyEditorWidget,
    BoolPropertyEditorWidget,
    IntPropertyEditorWidget,
    EnumPropertyEditorWidget,
    LabelPropertyEditorWidget,
)


class TestPropertyEditorFactory(unittest.TestCase):
    """Tests the BasePropertyEditorWidget.for_property factory method."""

    def setUp(self):
        # Use the real DetachedDocumentObject
        self.obj = DetachedDocumentObject()
        # Add properties using the DetachedDocumentObject API with correct signature
        self.obj.addProperty("App::PropertyLength", "Length", "Base", "Length property")
        self.obj.Length = FreeCAD.Units.Quantity(10.0)  # Set value separately

        self.obj.addProperty("App::PropertyBool", "IsEnabled", "Base", "Boolean property")
        self.obj.IsEnabled = True  # Set value separately

        self.obj.addProperty("App::PropertyInt", "Count", "Base", "Integer property")
        self.obj.Count = 5  # Set value separately

        self.obj.addProperty("App::PropertyEnumeration", "Mode", "Base", "Enumeration property")
        # Set enums and initial value separately
        self.obj.Mode = ["Auto", "Manual"]

        self.obj.addProperty("App::PropertyString", "Comment", "Base", "String property")
        self.obj.Comment = "Test"  # Set value separately

    def test_quantity_creation(self):
        widget = BasePropertyEditorWidget.for_property(self.obj, "Length")
        self.assertIsInstance(widget, QuantityPropertyEditorWidget)

    def test_bool_creation(self):
        widget = BasePropertyEditorWidget.for_property(self.obj, "IsEnabled")
        self.assertIsInstance(widget, BoolPropertyEditorWidget)

    def test_int_creation(self):
        widget = BasePropertyEditorWidget.for_property(self.obj, "Count")
        self.assertIsInstance(widget, IntPropertyEditorWidget)

    def test_enum_creation(self):
        widget = BasePropertyEditorWidget.for_property(self.obj, "Mode")
        self.assertIsInstance(widget, EnumPropertyEditorWidget)

    def test_label_creation_for_string(self):
        widget = BasePropertyEditorWidget.for_property(self.obj, "Comment")
        self.assertIsInstance(widget, LabelPropertyEditorWidget)

    def test_label_creation_for_invalid_prop(self):
        widget = BasePropertyEditorWidget.for_property(self.obj, "NonExistent")
        self.assertIsInstance(widget, LabelPropertyEditorWidget)


class TestQuantityPropertyEditorWidget(unittest.TestCase):
    """Tests for QuantityPropertyEditorWidget."""

    def setUp(self):
        self.obj = DetachedDocumentObject()
        self.obj.addProperty("App::PropertyLength", "Length", "Base", "Length property")
        self.obj.Length = FreeCAD.Units.Quantity("10.0 mm")
        self.widget = QuantityPropertyEditorWidget(self.obj, "Length")
        # Access the real editor widget
        self.editor = self.widget._editor_widget
        self.widget.updateWidget()

    def test_update_property(self):
        # Check if the real widget's value is updated
        self.assertEqual(self.editor.property("rawValue"), 10.0)
        # Check if the real widget's value and unit are updated
        self.assertEqual(self.editor.property("value").UserString, "10.00 mm")

        # Simulate changing the raw value and check if the object's value updates
        self.editor.lineEdit().setText("12.0")
        self.widget.updateProperty()
        self.assertEqual(self.obj.Length.Value, 12.0)
        self.assertEqual(self.obj.Length.UserString, "12.00 mm")

        # Try assignment with unit.
        self.editor.lineEdit().setText("15.5 in")
        self.widget.updateProperty()
        updated_value = self.obj.getPropertyByName("Length")
        self.assertIsInstance(updated_value, FreeCAD.Units.Quantity)
        self.assertEqual(updated_value, FreeCAD.Units.Quantity("15.5 in"))


class TestBoolPropertyEditorWidget(unittest.TestCase):
    """Tests for BoolPropertyEditorWidget."""

    def setUp(self):
        self.obj = DetachedDocumentObject()
        self.obj.addProperty("App::PropertyBool", "IsEnabled", "Base", "Boolean property")
        self.obj.IsEnabled = True
        self.widget = BoolPropertyEditorWidget(self.obj, "IsEnabled")
        # Access the real editor widget
        self.editor = self.widget._editor_widget

    def test_update_widget(self):
        self.widget.updateWidget()
        # Check if the real widget's value is updated
        self.assertEqual(self.editor.currentIndex(), 1)  # True is index 1

        self.obj.setPropertyByName("IsEnabled", False)
        self.widget.updateWidget()
        self.assertEqual(self.editor.currentIndex(), 0)  # False is index 0

    def test_update_property(self):
        # Simulate user changing value in the combobox
        self.editor.setCurrentIndex(0)  # Select False
        self.widget._on_index_changed(0)
        self.assertEqual(self.obj.getPropertyByName("IsEnabled"), False)

        self.editor.setCurrentIndex(1)  # Select True
        self.widget._on_index_changed(1)
        self.assertEqual(self.obj.getPropertyByName("IsEnabled"), True)


class TestIntPropertyEditorWidget(unittest.TestCase):
    """Tests for IntPropertyEditorWidget."""

    def setUp(self):
        self.obj = DetachedDocumentObject()
        self.obj.addProperty("App::PropertyInt", "Count", "Base", "Integer property")
        self.obj.Count = 5
        self.widget = IntPropertyEditorWidget(self.obj, "Count")
        # Access the real editor widget
        self.editor = self.widget._editor_widget

    def test_update_widget(self):
        self.widget.updateWidget()
        # Check if the real widget's value is updated
        self.assertEqual(self.editor.value(), 5)

        self.obj.setPropertyByName("Count", 100)
        self.widget.updateWidget()
        self.assertEqual(self.editor.value(), 100)

    def test_update_property(self):
        # Simulate user changing value in the spinbox
        self.editor.setValue(42)
        self.widget.updateProperty()
        self.assertEqual(self.obj.getPropertyByName("Count"), 42)


class TestEnumPropertyEditorWidget(unittest.TestCase):
    """Tests for EnumPropertyEditorWidget."""

    def setUp(self):
        self.obj = DetachedDocumentObject()
        self.obj.addProperty("App::PropertyEnumeration", "Mode", "Base", "Enumeration property")
        self.obj.Mode = ["Auto", "Manual", "Semi"]  # Set enums and initial value
        self.widget = EnumPropertyEditorWidget(self.obj, "Mode")
        # Access the real editor widget
        self.editor = self.widget._editor_widget

    def test_populate_enum(self):
        # Check if the real widget is populated
        self.assertEqual(self.editor.count(), 3)
        self.assertEqual(self.editor.itemText(0), "Auto")
        self.assertEqual(self.editor.itemText(1), "Manual")
        self.assertEqual(self.editor.itemText(2), "Semi")

    def test_update_widget(self):
        self.widget.updateWidget()
        # Check if the real widget's value is updated
        self.assertEqual(self.editor.currentText(), "Auto")

        self.obj.setPropertyByName("Mode", "Manual")
        self.widget.updateWidget()
        self.assertEqual(self.editor.currentText(), "Manual")

    def test_update_property(self):
        # Simulate user changing value in the combobox
        self.editor.setCurrentIndex(1)  # Select Manual
        self.widget._on_index_changed(1)
        self.assertEqual(self.obj.getPropertyByName("Mode"), "Manual")

        self.editor.setCurrentIndex(2)  # Select Semi
        self.widget._on_index_changed(2)
        self.assertEqual(self.obj.getPropertyByName("Mode"), "Semi")


class TestLabelPropertyEditorWidget(unittest.TestCase):
    """Tests for LabelPropertyEditorWidget."""

    def setUp(self):
        self.obj = DetachedDocumentObject()
        self.obj.addProperty("App::PropertyString", "Comment", "Base", "String property")
        self.obj.Comment = "Test Comment"
        self.widget = LabelPropertyEditorWidget(self.obj, "Comment")
        # Access the real editor widget
        self.editor = self.widget._editor_widget

    def test_update_widget(self):
        self.widget.updateWidget()
        # Check if the real widget's value is updated
        self.assertEqual(self.editor.text(), "Test Comment")

        self.obj.setPropertyByName("Comment", "New Comment")
        self.widget.updateWidget()
        self.assertEqual(self.editor.text(), "New Comment")

    def test_update_property_is_noop(self):
        # updateProperty should do nothing for a read-only label
        self.widget.updateProperty()
        # No assertions needed, just ensure it doesn't raise errors


if __name__ == "__main__":
    unittest.main()
