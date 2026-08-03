# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2025 Samuel Abels <knipknap@gmail.com>                  *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************

"""ToolBit selector dialog."""

from PySide import QtWidgets, QtGui
import FreeCAD
from ...camassets import cam_assets
from ...toolbit import ToolBit
from ...library.ui.browser import LibraryBrowserWithCombo


class ToolBitSelector(QtWidgets.QDialog):
    """
    A dialog for selecting ToolBits using the LibraryBrowserWithCombo.
    Can show tools from libraries or all available toolbits.
    """

    def __init__(
        self,
        parent=None,
        compact=False,
        button_label=FreeCAD.Qt.translate("CAM", "Add Tool"),
        show_all_tools=False,
    ):
        super().__init__(parent)

        self.setMinimumSize(700, 500)
        self.setWindowTitle(FreeCAD.Qt.translate("CAM", "Select Toolbit"))

        # Use LibraryBrowserWithCombo which handles library selection and "All Tools" option
        self._browser_widget = LibraryBrowserWithCombo(
            asset_manager=cam_assets,
            store="local",
            compact=compact,
            show_all_tools=show_all_tools,
        )

        # Create OK and Cancel buttons
        self._ok_button = QtWidgets.QPushButton(button_label)
        self._cancel_button = QtWidgets.QPushButton(FreeCAD.Qt.translate("CAM", "Cancel"))

        # Connect buttons to their actions
        self._ok_button.clicked.connect(self.accept)
        self._cancel_button.clicked.connect(self.reject)

        # Layout setup
        layout = QtWidgets.QVBoxLayout(self)
        layout.addWidget(self._browser_widget)

        button_layout = QtWidgets.QHBoxLayout()
        button_layout.addStretch()
        button_layout.addWidget(self._cancel_button)
        button_layout.addWidget(self._ok_button)

        layout.addLayout(button_layout)

        # Disable OK button initially until a tool is selected
        self._ok_button.setEnabled(False)
        self._browser_widget.toolSelected.connect(self._on_tool_selected)
        self._browser_widget.itemDoubleClicked.connect(self.accept)

        # Initialize the browser
        self._browser_widget.refresh()

    def _on_tool_selected(self, uri):
        """Enables/disables OK button based on selection."""
        self._ok_button.setEnabled(uri is not None)

    def get_selected_tool(self) -> ToolBit:
        """Returns the first selected ToolBit object, or None if none selected."""
        tools = self.get_selected_tools()
        return tools[0] if tools else None

    def get_selected_tools(self) -> list:
        """Returns a list of all selected ToolBit objects."""
        return self._browser_widget.get_selected_bits()

    def get_tool_numbers(self) -> dict:
        """
        Returns a dict mapping ToolBit URIs to tool numbers.
        If a library is selected, uses library tool numbers.
        If \"All Tools\" is selected, returns empty dict (auto-increment).
        """
        tool_numbers = {}

        # Only get library numbers if a specific library is selected
        if self._browser_widget.current_library:
            for toolbit in self.get_selected_tools():
                tool_no = self._browser_widget.get_tool_no_from_current_library(toolbit)
                if tool_no is not None:
                    tool_numbers[str(toolbit.get_uri())] = tool_no

        return tool_numbers  # Empty dict means auto-increment for all

    def get_tool_number(self):
        """
        Returns the tool number for the first selected tool.
        Kept for backward compatibility.
        """
        tool_numbers = self.get_tool_numbers()
        if tool_numbers:
            first_tool = self.get_selected_tool()
            if first_tool:
                return tool_numbers.get(str(first_tool.get_uri()))
        return None
