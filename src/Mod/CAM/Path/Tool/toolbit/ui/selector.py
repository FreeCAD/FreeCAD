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
from .browser import ToolBitBrowserWidget
from .typefilter import ToolBitTypeFilterMixin


class ToolBitSelector(QtWidgets.QDialog, ToolBitTypeFilterMixin):
    """
    A dialog for selecting ToolBits using the ToolBitBrowserWidget.
    """

    def __init__(
        self, parent=None, compact=False, button_label=FreeCAD.Qt.translate("CAM", "Add Tool")
    ):
        super().__init__(parent)

        self.setMinimumSize(600, 400)

        self.setWindowTitle(FreeCAD.Qt.translate("CAM", "Select Toolbit"))

        self._browser_widget = ToolBitBrowserWidget(cam_assets, compact=compact)

        # Add tool type filter combo box to the browser widget
        self._tool_type_combo = QtGui.QComboBox()
        self._tool_type_combo.setSizePolicy(
            QtGui.QSizePolicy.Expanding, QtGui.QSizePolicy.Preferred
        )
        self._browser_widget._top_layout.insertWidget(0, self._tool_type_combo, 1)
        self._tool_type_combo.currentTextChanged.connect(self._on_tool_type_combo_changed)

        # Create OK and Cancel buttons
        self._ok_button = QtWidgets.QPushButton(button_label)
        self._cancel_button = QtWidgets.QPushButton("Cancel")

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

        self._selected_tool_uri = None
        self._selected_tool_type = None

        # Initialize the tool type combo after browser widget is set up
        self._browser_widget.refresh()
        self._update_tool_type_combo()

    def _get_assets_for_type_filter(self):
        """Returns the list of assets to use for type filtering."""
        return getattr(self._browser_widget, "_all_assets", [])

    def _on_type_filter_changed(self):
        """Called when the type filter changes - updates the filtered list."""
        all_assets = getattr(self._browser_widget, "_all_assets", [])
        self._refresh_filtered_list(
            self._browser_widget._tool_list_widget,
            self._browser_widget._search_edit,
            all_assets,
        )
        self._browser_widget._tool_list_widget.apply_filter("")

    def _on_tool_selected(self, uri):
        """Enables/disables OK button based on selection."""
        self._selected_tool_uri = uri
        self._ok_button.setEnabled(uri is not None)

    def get_selected_tool_uri(self):
        """Returns the URI of the selected tool bit."""
        return self._selected_tool_uri

    def get_selected_tool(self) -> ToolBit:
        """Returns the selected ToolBit object, or None if none selected."""
        uri = self.get_selected_tool_uri()
        if uri:
            # Assuming ToolBit.from_uri exists and loads the ToolBit object
            return cam_assets.get(uri)
        return None
