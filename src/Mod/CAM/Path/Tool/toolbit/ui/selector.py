# -*- coding: utf-8 -*-
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

from PySide import QtWidgets
import FreeCAD
from ...camassets import cam_assets
from ...toolbit import ToolBit
from .browser import ToolBitBrowserWidget


class ToolBitSelector(QtWidgets.QDialog):
    """
    A dialog for selecting ToolBits using the ToolBitBrowserWidget.
    """

    def __init__(
        self, parent=None, compact=False, button_label=FreeCAD.Qt.translate("CAM", "Add Tool")
    ):
        super().__init__(parent)

        self.setMinimumSize(600, 400)

        self.setWindowTitle(FreeCAD.Qt.translate("CAM", "Select Tool Bit"))

        self._browser_widget = ToolBitBrowserWidget(cam_assets, compact=compact)

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
