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

"""Unit tests for the ToolBitBrowserWidget."""

import unittest
from unittest.mock import MagicMock
from typing import cast
from Path.Tool.toolbit.ui.browser import ToolBitBrowserWidget, ToolBitUriRole
from Path.Tool.toolbit.ui.tablecell import TwoLineTableCell
from Path.Tool.toolbit.models.base import ToolBit
from .PathTestUtils import PathTestWithAssets


class TestToolBitBrowserWidget(PathTestWithAssets):
    """Tests for ToolBitBrowserWidget using real assets and widgets."""

    def setUp(self):
        super().setUp()  # Call the base class setUp to initialize assets
        # The browser widget uses the global cam_assets, which is set up
        # by PathTestWithAssets.
        self.widget = ToolBitBrowserWidget(self.assets)

    def test_initial_fetch(self):
        # Simulate expose to trigger initial fetch
        self.widget.showEvent(None)

        # Verify that the list widget is populated after initialization
        # The default test assets include some toolbits.
        self.assertGreater(self.widget._tool_list_widget.count(), 0)

        # Verify apply_filter was called on the list widget with empty string
        # We can check the search_highlight property on the cell widgets
        # as apply_filter sets this.
        for i in range(self.widget._tool_list_widget.count()):
            item = self.widget._tool_list_widget.item(i)
            cell = self.widget._tool_list_widget.itemWidget(item)
            self.assertIsInstance(cell, TwoLineTableCell)
            self.assertEqual(cell.search_highlight, "")

    def test_search_filtering(self):
        # Simulate typing in the search box
        search_term = "Endmill"
        self.widget._search_edit.setText(search_term)

        # Directly trigger the fetch and filtering logic
        self.widget._trigger_fetch()

        # Verify that the filter was applied to the list widget
        # We can check if items are hidden/shown based on the filter term
        # This requires knowing the content of the test assets.
        # Assuming '5mm_Endmill' and '10mm_Endmill' contain "Endmill"
        # and 'BallEndmill_3mm' does not.

        # Re-fetch assets to know their labels/names for verification
        all_assets = self.assets.fetch(asset_type="toolbit", depth=0)
        expected_visible_uris = set()
        for asset in all_assets:
            tb = cast(ToolBit, asset)
            is_expected = (
                search_term.lower() in tb.label.lower() or search_term.lower() in tb.summary.lower()
            )
            if is_expected:
                expected_visible_uris.add(str(tb.get_uri()))

        # Simulate expose to trigger initial fetch
        self.widget.showEvent(None)

        actual_visible_uris = set()
        for i in range(self.widget._tool_list_widget.count()):
            item = self.widget._tool_list_widget.item(i)
            cell = self.widget._tool_list_widget.itemWidget(item)
            self.assertIsInstance(cell, TwoLineTableCell)
            item_uri = item.data(ToolBitUriRole)

            # Verify highlight was called on all cells
            self.assertEqual(cell.search_highlight, search_term)

            if not item.isHidden():
                actual_visible_uris.add(item_uri)

        self.assertEqual(actual_visible_uris, expected_visible_uris)

    def test_lazy_loading_on_scroll(self):
        # This test requires more than self._batch_size toolbits to be effective.
        # The default test assets might not have enough.
        # We'll assume there are enough for the test structure.

        initial_count = self.widget._tool_list_widget.count()
        if initial_count < self.widget._batch_size:
            self.skipTest("Not enough toolbits for lazy loading test.")

        # Simulate scrolling to the bottom by emitting the signal
        scrollbar = self.widget._tool_list_widget.verticalScrollBar()
        # Set the scrollbar value to its maximum to simulate reaching the end
        scrollbar.valueChanged.emit(scrollbar.maximum())

        # Verify that more items were loaded
        new_count = self.widget._tool_list_widget.count()
        self.assertGreater(new_count, initial_count)
        # Verify that the number of new items is approximately the batch size
        self.assertAlmostEqual(
            new_count - initial_count, self.widget._batch_size, delta=5
        )  # Allow small delta

    def test_tool_selected_signal(self):
        mock_slot = MagicMock()
        self.widget.toolSelected.connect(mock_slot)

        # Select the first item in the list widget
        if self.widget._tool_list_widget.count() == 0:
            self.skipTest("Not enough toolbits for selection test.")
        first_item = self.widget._tool_list_widget.item(0)
        self.widget._tool_list_widget.setCurrentItem(first_item)

        # Verify signal was emitted with the correct URI
        expected_uri = first_item.data(ToolBitUriRole)
        mock_slot.assert_called_once_with(expected_uri)

    def test_tool_edit_requested_signal(self):
        mock_slot = MagicMock()
        self.widget.itemDoubleClicked.connect(mock_slot)

        # Double-click the first item in the list widget
        if self.widget._tool_list_widget.count() == 0:
            self.skipTest("Not enough toolbits for double-click test.")

        first_item = self.widget._tool_list_widget.item(0)
        # Simulate double-click signal emission from the list widget
        self.widget._tool_list_widget.itemDoubleClicked.emit(first_item)

        # Verify signal was emitted with the correct URI
        expected_uri = first_item.data(ToolBitUriRole)
        mock_slot.assert_called_once_with(expected_uri)


if __name__ == "__main__":
    unittest.main()
