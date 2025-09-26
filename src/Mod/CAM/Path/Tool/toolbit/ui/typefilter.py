# SPDX-License-Identifier: LGPL-2.1-or-later
# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2025 Billy <billy@ivdc.com>                             *
# *                                                                         *
# *   This file is part of FreeCAD.                                         *
# *                                                                         *
# *   FreeCAD is free software: you can redistribute it and/or modify it    *
# *   under the terms of the GNU Lesser General Public License as           *
# *   published by the Free Software Foundation, either version 2.1 of the  *
# *   License, or (at your option) any later version.                       *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful, but        *
# *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
# *   Lesser General Public License for more details.                       *
# *                                                                         *
# *   You should have received a copy of the GNU Lesser General Public      *
# *   License along with FreeCAD. If not, see                               *
# *   <https://www.gnu.org/licenses/>.                                      *
# *                                                                         *
# ***************************************************************************

import FreeCAD
from ...docobject.ui.docobject import _get_label_text


class ToolBitTypeFilterMixin:
    """
    Mixin providing shared methods for filtering toolbits by type and subtype.

    Classes using this mixin should have:
    - _all_assets: list of ToolBit objects
    - _tool_type_combo: QComboBox for type selection
    - _selected_tool_type: Optional[str] for storing current selection

    Classes must implement:
    - _get_assets_for_type_filter() -> list: Returns list of assets to extract types from
    - _on_type_filter_changed(): Called when type filter changes
    """

    def _get_available_tool_types(self, assets):
        """Get all available tool types and subtypes, grouped for display.

        Args:
            assets: List of ToolBit objects to extract types from

        Returns:
            List of tuples: (display_text, actual_value)
        """
        # Build a mapping: {ParentType: {actual_subtype: display_subtype}}
        type_map = {}
        for asset in assets:
            parent = asset.get_shape_name()  # Preserve original case (e.g., "Probe")
            subtype = asset.get_subtype()
            if subtype:
                # Preserve underscores/hyphens but make displayable
                subtype_disp = subtype.replace("_", " ").replace("-", " ").title()
                type_map.setdefault(parent, {})[subtype] = subtype_disp
            else:
                type_map.setdefault(parent, {})

        # Flatten for combo: parent, then indented subtypes
        # Return tuples of (display, value)
        result = []
        for parent in sorted(type_map):
            parent_display = _get_label_text(parent, keep_case=True, preserve_consecutive_caps=True)
            result.append((parent_display, parent))  # Parent with formatted display
            for subtype_val, subtype_disp in sorted(type_map[parent].items()):
                subtype_display = _get_label_text(
                    subtype_val, keep_case=True, preserve_consecutive_caps=True
                )
                result.append(
                    (f"  {subtype_display}", subtype_val)
                )  # Indent display, preserve value
        return result

    def _get_filtered_assets(self, assets):
        """Filter assets by selected type or subtype, showing subtypes under parent.

        Args:
            assets: List of ToolBit objects to filter

        Returns:
            Filtered list of ToolBit objects
        """
        if self._tool_type_combo.currentIndex() == 0:
            return assets

        # Get the actual value (not display text) from combo item data
        sel_value = self._tool_type_combo.currentData()
        if not sel_value:
            return assets

        # Check if it's a subtype by looking at all subtypes
        all_subtypes = set()
        for asset in assets:
            subtype = asset.get_subtype()
            if subtype:
                all_subtypes.add(subtype)

        if sel_value in all_subtypes:
            # It's a subtype - filter by exact subtype match
            return [a for a in assets if a.get_subtype() == sel_value]
        else:
            # It's a parent type - show all with this shape name (including those without subtypes)
            return [a for a in assets if a.get_shape_name() == sel_value]

    def _update_tool_type_combo(self):
        """Update the tool type combo box with available types.

        Calls _get_assets_for_type_filter() to get the list of assets.
        """
        current_data = self._tool_type_combo.currentData()  # Save current selection by data
        self._tool_type_combo.blockSignals(True)
        try:
            self._tool_type_combo.clear()
            self._tool_type_combo.addItem(FreeCAD.Qt.translate("CAM", "All Toolbit Types"), None)

            assets = self._get_assets_for_type_filter()
            available_types = self._get_available_tool_types(assets)
            for display_text, value in available_types:
                self._tool_type_combo.addItem(display_text, value)

            # Restore previous selection if it still exists
            if current_data is not None:
                for i in range(self._tool_type_combo.count()):
                    if self._tool_type_combo.itemData(i) == current_data:
                        self._tool_type_combo.setCurrentIndex(i)
                        break
                else:
                    self._tool_type_combo.setCurrentIndex(0)
            else:
                self._tool_type_combo.setCurrentIndex(0)
        finally:
            self._tool_type_combo.blockSignals(False)

    def _on_tool_type_combo_changed(self, index):
        """Handle tool type filter selection change.

        Stores the current selection and calls _on_type_filter_changed()
        which should be implemented by the subclass.
        """
        # Store both text (for backward compat) and data
        self._selected_tool_type = self._tool_type_combo.currentText()
        self._on_type_filter_changed()

    def _get_assets_for_type_filter(self):
        """Returns the list of assets to use for type filtering.

        Subclasses should override this method to provide their asset list.
        Default implementation returns _all_assets if it exists.
        """
        return getattr(self, "_all_assets", [])

    def _on_type_filter_changed(self):
        """Called when the type filter changes.

        Subclasses should override this to update their UI accordingly.
        Default implementation does nothing.
        """
        pass

    def _apply_type_and_search_filter(self, assets, search_term=""):
        """Apply both type and search filtering to assets.

        Args:
            assets: List of ToolBit objects to filter
            search_term: Optional search string to filter by (searches label and summary)

        Returns:
            Filtered list of ToolBit objects
        """
        # First apply type filter
        filtered_assets = self._get_filtered_assets(assets)

        # Then apply search filter if provided
        if search_term:
            search_term_lower = search_term.lower()
            search_filtered = []
            for asset in filtered_assets:
                if search_term_lower in asset.label.lower():
                    search_filtered.append(asset)
                    continue
                if search_term_lower in asset.summary.lower():
                    search_filtered.append(asset)
                    continue
                # Also search in tool type
                if hasattr(asset, "get_shape_name"):
                    tool_type = asset.get_shape_name()
                    if tool_type and search_term_lower in tool_type.lower():
                        search_filtered.append(asset)
            filtered_assets = search_filtered

        return filtered_assets

    def _refresh_filtered_list(self, tool_list_widget, search_edit, assets):
        """Refresh the list widget with filtered assets.

        Args:
            tool_list_widget: The QListWidget to update
            search_edit: The QLineEdit containing search text
            assets: List of ToolBit objects to filter and display
        """
        tool_list_widget.clear_list()
        search_term = search_edit.text()
        filtered_assets = self._apply_type_and_search_filter(assets, search_term)

        for asset in filtered_assets:
            tool_list_widget.add_toolbit(asset)
