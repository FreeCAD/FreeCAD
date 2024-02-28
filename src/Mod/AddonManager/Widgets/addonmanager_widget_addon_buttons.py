# SPDX-License-Identifier: LGPL-2.1-or-later
# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2024 FreeCAD Project Association                        *
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

""" Defines a QWidget-derived class for displaying the single-addon buttons. """

from enum import Enum, auto

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

from PySide import QtCore, QtGui, QtWidgets


class ButtonBarDisplayMode(Enum):
    TextOnly = auto()
    IconsOnly = auto()
    TextAndIcons = auto()


class WidgetAddonButtons(QtWidgets.QWidget):
    def __init__(self, parent: QtWidgets.QWidget = None):
        super().__init__(parent)
        self.display_mode = ButtonBarDisplayMode.TextAndIcons
        self._setup_ui()
        self._set_icons()
        self.retranslateUi(None)

    def set_display_mode(self, mode: ButtonBarDisplayMode):
        """NOTE: Not really implemented yet -- TODO: Implement this functionality"""
        if mode == self.display_mode:
            return
        self._setup_ui()
        self._set_icons()
        self.retranslateUi(None)

    def _setup_ui(self):
        if self.layout():
            self.setLayout(None)  # TODO: Check this
        self.horizontal_layout = QtWidgets.QHBoxLayout()
        self.horizontal_layout.setContentsMargins(0, 0, 0, 0)
        self.back = QtWidgets.QToolButton(self)
        self.install = QtWidgets.QPushButton(self)
        self.uninstall = QtWidgets.QPushButton(self)
        self.enable = QtWidgets.QPushButton(self)
        self.disable = QtWidgets.QPushButton(self)
        self.update = QtWidgets.QPushButton(self)
        self.run_macro = QtWidgets.QPushButton(self)
        self.change_branch = QtWidgets.QPushButton(self)
        self.check_for_update = QtWidgets.QPushButton(self)
        self.horizontal_layout.addWidget(self.back)
        self.horizontal_layout.addStretch()
        self.horizontal_layout.addWidget(self.check_for_update)
        self.horizontal_layout.addWidget(self.install)
        self.horizontal_layout.addWidget(self.uninstall)
        self.horizontal_layout.addWidget(self.enable)
        self.horizontal_layout.addWidget(self.disable)
        self.horizontal_layout.addWidget(self.update)
        self.horizontal_layout.addWidget(self.run_macro)
        self.horizontal_layout.addWidget(self.change_branch)
        self.setLayout(self.horizontal_layout)

    def set_show_back_button(self, show: bool) -> None:
        self.back.setVisible(show)

    def _set_icons(self):
        self.back.setIcon(QtGui.QIcon.fromTheme("back", QtGui.QIcon(":/icons/button_left.svg")))

    def retranslateUi(self, _):
        self.check_for_update.setText(translate("AddonsInstaller", "Check for update"))
        self.install.setText(translate("AddonsInstaller", "Install"))
        self.uninstall.setText(translate("AddonsInstaller", "Uninstall"))
        self.disable.setText(translate("AddonsInstaller", "Disable"))
        self.enable.setText(translate("AddonsInstaller", "Enable"))
        self.update.setText(translate("AddonsInstaller", "Update"))
        self.run_macro.setText(translate("AddonsInstaller", "Run"))
        self.change_branch.setText(translate("AddonsInstaller", "Change branch..."))
        self.back.setToolTip(translate("AddonsInstaller", "Return to package list"))
