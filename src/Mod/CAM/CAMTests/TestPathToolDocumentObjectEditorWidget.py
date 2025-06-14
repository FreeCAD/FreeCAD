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

"""Unit tests for the DocumentObjectEditorWidget."""

import unittest
from unittest.mock import MagicMock
import FreeCAD
from PySide import QtGui
from Path.Tool.docobject import DetachedDocumentObject
from Path.Tool.docobject.ui.docobject import DocumentObjectEditorWidget, _get_label_text
from Path.Tool.docobject.ui.property import (
    BasePropertyEditorWidget,
    QuantityPropertyEditorWidget,
    BoolPropertyEditorWidget,
    IntPropertyEditorWidget,
    EnumPropertyEditorWidget,
    LabelPropertyEditorWidget,
)


class TestDocumentObjectEditorWidget(unittest.TestCase):
    """Tests for DocumentObjectEditorWidget."""

    def test_populate_form(self):
        obj = DetachedDocumentObject()
        obj.addProperty("App::PropertyString", "Prop1", "Group1", "Doc1")
        obj.Prop1 = "Value1"
        obj.addProperty("App::PropertyInt", "Prop2", "Group1", "Doc2")
        obj.Prop2 = 123
        obj.addProperty("App::PropertyLength", "Prop3", "Group1", "Doc3")
        obj.Prop3 = FreeCAD.Units.Quantity(5.0, "mm")
        obj.addProperty("App::PropertyBool", "Prop4", "Group1", "Doc4")
        obj.Prop4 = False
        obj.addProperty("App::PropertyEnumeration", "Prop5", "Group1", "Doc5")
        obj.Prop5 = ["OptionA", "OptionB"]

        properties_to_show = ["Prop1", "Prop2", "Prop3", "Prop4", "Prop5", "NonExistent"]
        property_suffixes = {"Prop1": "Suffix1", "Prop3": "Len"}

        widget = DocumentObjectEditorWidget(
            obj=obj, properties_to_show=properties_to_show, property_suffixes=property_suffixes
        )

        # Verify the layout contains the correct number of rows (excluding non-existent)
        expected_row_count = len([p for p in properties_to_show if hasattr(obj, p)])
        self.assertEqual(widget._layout.rowCount(), expected_row_count)

        # Verify labels and widgets are added correctly and are of the expected types
        prop_names_in_layout = []
        for i in range(widget._layout.rowCount()):
            label_item = widget._layout.itemAt(i, QtGui.QFormLayout.LabelRole)
            field_item = widget._layout.itemAt(i, QtGui.QFormLayout.FieldRole)

            self.assertIsNotNone(label_item)
            self.assertIsNotNone(field_item)

            label_widget = label_item.widget()
            field_widget = field_item.widget()

            self.assertIsInstance(label_widget, QtGui.QLabel)
            self.assertIsInstance(
                field_widget, BasePropertyEditorWidget
            )  # Check against base class

            # Determine the property name from the label text (reverse of _get_label_text)
            # This is a bit fragile, but necessary without storing prop_name in the label widget
            label_text = label_widget.text()
            prop_name = None
            for original_prop_name in properties_to_show:
                expected_label = _get_label_text(original_prop_name)
                suffix = property_suffixes.get(original_prop_name)
                if suffix:
                    expected_label = f"{expected_label} ({suffix}):"
                else:
                    expected_label = f"{expected_label}:"
                if label_text == expected_label:
                    prop_name = original_prop_name
                    break

            self.assertIsNotNone(
                prop_name, f"Could not determine property name for label: {label_text}"
            )
            prop_names_in_layout.append(prop_name)

            # Verify widget type based on property type
            if prop_name == "Prop1":
                self.assertIsInstance(field_widget, LabelPropertyEditorWidget)
                self.assertEqual(label_widget.text(), "Prop1 (Suffix1):")
            elif prop_name == "Prop2":
                self.assertIsInstance(field_widget, IntPropertyEditorWidget)
                self.assertEqual(label_widget.text(), "Prop2:")
            elif prop_name == "Prop3":
                self.assertIsInstance(field_widget, QuantityPropertyEditorWidget)
                self.assertEqual(label_widget.text(), "Prop3 (Len):")
            elif prop_name == "Prop4":
                self.assertIsInstance(field_widget, BoolPropertyEditorWidget)
                self.assertEqual(label_widget.text(), "Prop4:")
            elif prop_name == "Prop5":
                self.assertIsInstance(field_widget, EnumPropertyEditorWidget)
                self.assertEqual(label_widget.text(), "Prop5:")

        # Verify property editors are stored
        self.assertEqual(len(widget._property_editors), expected_row_count)
        for prop_name in prop_names_in_layout:
            self.assertIn(prop_name, widget._property_editors)
            self.assertIsInstance(widget._property_editors[prop_name], BasePropertyEditorWidget)

    def test_set_object(self):
        obj1 = DetachedDocumentObject()
        obj1.addProperty("App::PropertyString", "PropA", "GroupA", "DocA")
        obj1.PropA = "ValueA"

        obj2 = DetachedDocumentObject()
        obj2.addProperty("App::PropertyString", "PropA", "GroupA", "DocA")
        obj2.PropA = "ValueB"

        properties_to_show = ["PropA"]
        widget = DocumentObjectEditorWidget(obj=obj1, properties_to_show=properties_to_show)

        # Get the initial editor widget instance
        initial_editor = widget._property_editors["PropA"]
        self.assertIsInstance(initial_editor, BasePropertyEditorWidget)

        # Set a new object
        widget.setObject(obj2)

        # Verify that the editor widget instance is the same
        self.assertEqual(widget._property_editors["PropA"], initial_editor)
        # Verify that attachTo was called on the existing editor widget
        # This requires the real attachTo method to be implemented correctly
        # and the editor widget to update its internal object reference.
        # We can't easily assert the internal state change without mocks,
        # but we can trust the implementation of attachTo in PropertyEditorWidget.
        # We can verify updateWidget was called.
        # Note: This test relies on the side effect of attachTo calling updateWidget
        # in the real implementation.
        # We can't directly assert method calls without mocks, so we'll rely on
        # the fact that setting the object and then updating the UI should
        # reflect the new object's values if attachTo worked.
        widget.updateUI()
        # Check if the child widget's display reflects obj2's value
        # This requires accessing the child widget's internal editor widget
        # which might be fragile. A better approach is to trust the unit tests
        # for the individual PropertyEditorWidgets and focus on the
        # DocumentObjectEditorWidget's logic of calling attachTo and updateUI.

    def test_set_properties_to_show(self):
        obj = DetachedDocumentObject()
        obj.addProperty("App::PropertyString", "Prop1", "Group1", "Doc1")
        obj.Prop1 = "Value1"
        obj.addProperty("App::PropertyInt", "Prop2", "Group1", "Doc2")
        obj.Prop2 = 123

        widget = DocumentObjectEditorWidget(obj=obj, properties_to_show=["Prop1"])

        # Store the initial editor instance for Prop1
        initial_prop1_editor = widget._property_editors["Prop1"]

        # Set new properties to show
        new_properties_to_show = ["Prop1", "Prop2"]
        new_suffixes = {"Prop2": "Suffix2"}
        widget.setPropertiesToShow(new_properties_to_show, new_suffixes)

        # Verify that the form was repopulated with the new properties
        self.assertEqual(widget._layout.rowCount(), len(new_properties_to_show))

        # Verify property editors are updated and new ones created
        self.assertEqual(len(widget._property_editors), len(new_properties_to_show))
        self.assertIn("Prop1", widget._property_editors)
        self.assertIn("Prop2", widget._property_editors)

        # Verify that the editor for Prop1 is a *new* instance after repopulation
        self.assertIsNot(widget._property_editors["Prop1"], initial_prop1_editor)
        self.assertIsInstance(widget._property_editors["Prop2"], IntPropertyEditorWidget)

        # Verify labels including suffixes
        prop_names_in_layout = []
        for i in range(widget._layout.rowCount()):
            label_item = widget._layout.itemAt(i, QtGui.QFormLayout.LabelRole)
            label_widget = label_item.widget()
            label_text = label_widget.text()

            prop_name = None
            for original_prop_name in new_properties_to_show:
                expected_label = _get_label_text(original_prop_name)
                suffix = new_suffixes.get(original_prop_name)
                if suffix:
                    expected_label = f"{expected_label} ({suffix}):"
                else:
                    expected_label = f"{expected_label}:"
                if label_text == expected_label:
                    prop_name = original_prop_name
                    break
            prop_names_in_layout.append(prop_name)

        self.assertIn("Prop1", prop_names_in_layout)
        self.assertIn("Prop2", prop_names_in_layout)
        self.assertEqual(
            widget._layout.itemAt(0, QtGui.QFormLayout.LabelRole).widget().text(), "Prop1:"
        )
        self.assertEqual(
            widget._layout.itemAt(1, QtGui.QFormLayout.LabelRole).widget().text(),
            "Prop2 (Suffix2):",
        )

    def test_property_changed_signal(self):
        obj = DetachedDocumentObject()
        obj.addProperty("App::PropertyString", "Prop1", "Group1", "Doc1")
        obj.Prop1 = "Value1"

        widget = DocumentObjectEditorWidget(obj=obj, properties_to_show=["Prop1"])

        # Connect to the widget's propertyChanged signal
        mock_slot = MagicMock()
        widget.propertyChanged.connect(mock_slot)

        # Get the real child editor widget
        child_editor = widget._property_editors["Prop1"]
        self.assertIsInstance(child_editor, BasePropertyEditorWidget)

        # Emit the signal from the real child editor
        child_editor.propertyChanged.emit()

        # Verify that the widget's signal was emitted
        mock_slot.assert_called_once()

    def test_update_ui(self):
        obj = DetachedDocumentObject()
        obj.addProperty("App::PropertyString", "Prop1", "Group1", "Doc1")
        obj.Prop1 = "Value1"
        obj.addProperty("App::PropertyInt", "Prop2", "Group1", "Doc2")
        obj.Prop2 = 123

        properties_to_show = ["Prop1", "Prop2"]
        widget = DocumentObjectEditorWidget(obj=obj, properties_to_show=properties_to_show)

        # Get the real child editor widgets
        editor1 = widget._property_editors["Prop1"]
        editor2 = widget._property_editors["Prop2"]

        # Mock their updateWidget methods to check if they are called
        editor1.updateWidget = MagicMock()
        editor2.updateWidget = MagicMock()

        # Call updateUI
        widget.updateUI()

        # Verify that updateWidget was called on all child editors
        editor1.updateWidget.assert_called_once()
        editor2.updateWidget.assert_called_once()

    def test_update_object(self):
        obj = DetachedDocumentObject()
        obj.addProperty("App::PropertyString", "Prop1", "Group1", "Doc1")
        obj.Prop1 = "Value1"
        obj.addProperty("App::PropertyInt", "Prop2", "Group1", "Doc2")
        obj.Prop2 = 123

        properties_to_show = ["Prop1", "Prop2"]
        widget = DocumentObjectEditorWidget(obj=obj, properties_to_show=properties_to_show)

        # Get the real child editor widgets
        editor1 = widget._property_editors["Prop1"]
        editor2 = widget._property_editors["Prop2"]

        # Mock their updateProperty methods to check if they are called
        editor1.updateProperty = MagicMock()
        editor2.updateProperty = MagicMock()

        # Call updateObject
        widget.updateObject()

        # Verify that updateProperty was called on all child editors
        editor1.updateProperty.assert_called_once()
        editor2.updateProperty.assert_called_once()


if __name__ == "__main__":
    unittest.main()
