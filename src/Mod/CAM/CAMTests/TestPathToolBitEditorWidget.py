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
# *   License along with this program; if not, write to the FreeCAD         *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************

"""Unit tests for the ToolBitEditorWidget."""

import unittest
from unittest.mock import MagicMock
from Path.Tool.toolbit.ui.editor import ToolBitPropertiesWidget
from Path.Tool.toolbit.models.base import ToolBit
from Path.Tool.shape.ui.shapewidget import ShapeWidget
from Path.Tool.docobject.ui.property import BasePropertyEditorWidget
from .PathTestUtils import PathTestWithAssets


class TestToolBitPropertiesWidget(PathTestWithAssets):
    """Tests for ToolBitEditorWidget using real assets and widgets."""

    def setUp(self):
        super().setUp()  # Call the base class setUp to initialize assets
        self.widget = ToolBitPropertiesWidget()

    def test_load_toolbit(self):
        # Get a real ToolBit asset
        toolbit = self.assets.get("toolbit://5mm_Endmill")
        self.assertIsInstance(toolbit, ToolBit)

        self.widget.load_toolbit(toolbit)

        # Verify label and ID are populated
        self.assertEqual(self.widget._label_edit.text(), toolbit.obj.Label)
        self.assertEqual(self.widget._id_label.text(), toolbit.get_id())

        # Verify DocumentObjectEditorWidget is populated
        self.assertEqual(self.widget._property_editor._obj, toolbit.obj)
        # Check if properties were passed to the property editor
        self.assertGreater(len(self.widget._property_editor._properties_to_show), 0)

        # Verify ShapeWidget is created and populated
        self.assertIsNotNone(self.widget._shape_widget)
        self.assertIsInstance(self.widget._shape_widget, ShapeWidget)
        # We can't easily check the internal shape of ShapeWidget without mocks,
        # but we can verify it was created.

    def test_label_changed_updates_object(self):
        toolbit = self.assets.get("toolbit://5mm_Endmill")
        self.widget.load_toolbit(toolbit)

        new_label = "Updated Endmill"
        self.widget._label_edit.setText(new_label)
        # Simulate editing finished signal
        self.widget._label_edit.editingFinished.emit()

        # Verify the toolbit object's label is updated
        self.assertEqual(toolbit.obj.Label, new_label)

    def test_property_changed_signal_emitted(self):
        toolbit = self.assets.get("toolbit://5mm_Endmill")
        self.widget.load_toolbit(toolbit)

        mock_slot = MagicMock()
        self.widget.toolBitChanged.connect(mock_slot)

        # Simulate a property change in the DocumentObjectEditorWidget
        # We need to trigger the signal from the real property editor.
        # This requires accessing a child editor and emitting its signal.
        # Find a child editor (e.g., the first one)
        if self.widget._property_editor._property_editors:
            first_prop_name = list(self.widget._property_editor._property_editors.keys())[0]
            child_editor = self.widget._property_editor._property_editors[first_prop_name]
            self.assertIsInstance(child_editor, BasePropertyEditorWidget)

            # Emit the propertyChanged signal from the child editor
            child_editor.propertyChanged.emit()

            # Verify the ToolBitEditorWidget's signal was emitted
            mock_slot.assert_called_once()
        else:
            self.skipTest("DocumentObjectEditorWidget has no property editors.")

    def test_save_toolbit(self):
        # The save_toolbit method primarily ensures the label is updated
        # and potentially calls updateObject on the property editor.
        # Since property changes are signal-driven, this method is more
        # for explicit save actions.

        toolbit = self.assets.get("toolbit://5mm_Endmill")
        self.widget.load_toolbit(toolbit)

        initial_label = toolbit.obj.Label
        new_label = "Another Label"
        self.widget._label_edit.setText(new_label)

        # Call save_toolbit
        self.widget.save_toolbit()

        # Verify the label was updated
        self.assertEqual(toolbit.obj.Label, new_label)

        # We can't easily verify if updateObject was called on the property
        # editor without mocks, but we trust the implementation based on
        # the DocumentObjectEditorWidget's own tests.


if __name__ == "__main__":
    unittest.main()
