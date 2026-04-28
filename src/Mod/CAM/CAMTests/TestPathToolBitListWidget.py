# SPDX-License-Identifier: LGPL-2.1-or-later

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

"""Unit tests for the ToolBitListWidget."""

from typing import cast
import unittest
from Path.Tool.toolbit import ToolBit
from Path.Tool.toolbit.ui.toollist import ToolBitListWidget, ToolBitUriRole
from Path.Tool.toolbit.ui.tablecell import TwoLineTableCell
from .PathTestUtils import PathTestWithAssets  # Import the base test class


class TestToolBitListWidget(PathTestWithAssets):
    """Tests for ToolBitListWidget using real assets."""

    def setUp(self):
        super().setUp()  # Call the base class setUp to initialize assets
        self.widget = ToolBitListWidget()

    def test_add_toolbit(self):
        # Get a real ToolBit asset
        toolbit = cast(ToolBit, self.assets.get("toolbit://5mm_Endmill"))
        tool_no = 1

        self.widget.add_toolbit(toolbit, str(tool_no))

        self.assertEqual(self.widget.count(), 1)
        item = self.widget.item(0)
        self.assertIsNotNone(item)

        cell_widget = self.widget.itemWidget(item)
        self.assertIsInstance(cell_widget, TwoLineTableCell)  # Check against real class

        # Verify cell widget properties are set correctly
        self.assertEqual(cell_widget.tool_no, str(tool_no))
        self.assertEqual(cell_widget.upper_text, toolbit.label)
        # Assuming the 5mm_Endmill asset has a shape named 'Endmill'
        normalized_lower_text = cell_widget.lower_text.replace(",00 ", ".00 ")
        self.assertEqual(normalized_lower_text, "5.00 mm 4-flute endmill, 30.00 mm cutting edge")

        # Verify URI is stored in item data
        stored_uri = item.data(ToolBitUriRole)
        self.assertEqual(stored_uri, str(toolbit.get_uri()))

    def test_clear_list(self):
        # Add some real items first
        toolbit1 = cast(ToolBit, self.assets.get("toolbit://5mm_Endmill"))
        toolbit2 = cast(ToolBit, self.assets.get("toolbit://slittingsaw"))
        self.widget.add_toolbit(toolbit1, 1)
        self.widget.add_toolbit(toolbit2, 2)
        self.assertEqual(self.widget.count(), 2)

        self.widget.clear_list()
        self.assertEqual(self.widget.count(), 0)

    def test_apply_filter(self):
        # Add items with distinct text for filtering
        toolbit1 = cast(ToolBit, self.assets.get("toolbit://5mm_Endmill"))
        toolbit2 = cast(ToolBit, self.assets.get("toolbit://slittingsaw"))
        toolbit3 = cast(ToolBit, self.assets.get("toolbit://probe"))

        self.widget.add_toolbit(toolbit1, 1)
        self.widget.add_toolbit(toolbit2, 2)
        self.widget.add_toolbit(toolbit3, 3)

        items = [self.widget.item(i) for i in range(self.widget.count())]
        cells = [self.widget.itemWidget(item) for item in items]

        # Test filter "Endmill"
        self.widget.apply_filter("Endmill")

        self.assertFalse(items[0].isHidden())  # 5mm Endmill
        self.assertTrue(items[1].isHidden())  # slittingsaw
        self.assertTrue(items[2].isHidden())  # probe

        # Verify highlight was called on all cells
        for cell in cells:
            self.assertEqual(cell.search_highlight, "Endmill")

        # Test filter "Ballnose"
        self.widget.apply_filter("Ballnose")

        self.assertTrue(items[0].isHidden())  # 5mm Endmill
        self.assertTrue(items[1].isHidden())  # slittingsaw
        self.assertTrue(items[2].isHidden())  # probe

        # Verify highlight was called again
        for cell in cells:
            self.assertEqual(cell.search_highlight, "Ballnose")

        # Test filter "3mm"
        self.widget.apply_filter("3mm")

        self.assertTrue(items[0].isHidden())  # 5mm Endmill
        self.assertTrue(items[1].isHidden())  # slittingsaw
        self.assertTrue(items[2].isHidden())  # probe

        # Verify highlight was called again
        for cell in cells:
            self.assertEqual(cell.search_highlight, "3mm")

    def test_get_selected_toolbit_uri(self):
        toolbit1 = cast(ToolBit, self.assets.get("toolbit://5mm_Endmill"))
        toolbit2 = cast(ToolBit, self.assets.get("toolbit://slittingsaw"))

        self.widget.add_toolbit(toolbit1, 1)
        self.widget.add_toolbit(toolbit2, 2)

        # No selection initially
        self.assertIsNone(self.widget.get_selected_toolbit_uri())

        # Select the first item
        self.widget.setCurrentItem(self.widget.item(0))
        self.assertEqual(self.widget.get_selected_toolbit_uri(), str(toolbit1.get_uri()))

        # Select the second item
        self.widget.setCurrentItem(self.widget.item(1))
        self.assertEqual(self.widget.get_selected_toolbit_uri(), str(toolbit2.get_uri()))

        # Clear selection (simulate by setting current item to None)
        self.widget.setCurrentItem(None)
        self.assertIsNone(self.widget.get_selected_toolbit_uri())


if __name__ == "__main__":
    unittest.main()
