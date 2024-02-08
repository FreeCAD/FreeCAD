# SPDX-License-Identifier: LGPL-2.1-or-later
# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2022-2024 FreeCAD Project Association                   *
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


try:
    import FreeCAD

    translate = FreeCAD.Qt.translate
except ImportError:
    FreeCAD = None

    def translate(_: str, text: str):
        return text


# Get whatever version of PySide we can
try:
    import PySide  # Use the FreeCAD wrapper
except ImportError:
    try:
        import PySide6  # Outside FreeCAD, try Qt6 first

        PySide = PySide6
    except ImportError:
        import PySide2  # Fall back to Qt5 (if this fails, Python will kill this module's import)

        PySide = PySide2

from PySide import QtCore, QtWidgets

from .addonmanager_widget_addon_buttons import WidgetAddonButtons


class PackageDetailsView(QtWidgets.QWidget):
    """The view class for the package details"""

    install_clicked = QtCore.Signal()
    uninstall_clicked = QtCore.Signal()
    enable_clicked = QtCore.Signal()
    disable_clicked = QtCore.Signal()
    update_clicked = QtCore.Signal()
    check_for_updates = QtCore.Signal()
    run_clicked = QtCore.Signal()

    def __init__(self, parent: QtWidgets.QWidget = None):
        super().__init__(parent)
        self.button_bar = None
        self.text_browser = None
        self._setup_ui()

    def _setup_ui(self):
        self.vertical_layout = QtWidgets.QVBoxLayout(self)
        self.button_bar = WidgetAddonButtons(self)
        self.text_browser = QtWidgets.QTextBrowser(self)
        self.vertical_layout.addWidget(self.button_bar)
        self.vertical_layout.addWidget(self.text_browser)
