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

""" Defines a QWidget-derived class for displaying the view selection buttons. """

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


class WidgetSearch(QtWidgets.QWidget):
    """A widget for selecting the Addon Manager's primary view mode"""

    search_changed = QtCore.Signal(str)

    def __init__(self, parent: QtWidgets.QWidget = None):
        super().__init__(parent)
        self._setup_ui()
        self._setup_connections()
        self.retranslateUi(None)

    def _setup_ui(self):
        self.horizontal_layout = QtWidgets.QHBoxLayout()
        self.horizontal_layout.setContentsMargins(0, 0, 0, 0)
        self.filter_line_edit = QtWidgets.QLineEdit(self)
        self.filter_line_edit.setClearButtonEnabled(True)
        self.horizontal_layout.addWidget(self.filter_line_edit)
        self.filter_validity_label = QtWidgets.QLabel(self)
        self.horizontal_layout.addWidget(self.filter_validity_label)
        self.filter_validity_label.hide()  # This widget starts hidden
        self.setLayout(self.horizontal_layout)

    def _setup_connections(self):
        self.filter_line_edit.textChanged.connect(self.set_text_filter)

    def set_text_filter(self, text_filter: str) -> None:
        """Set the current filter. If the filter is valid, this will emit a filter_changed
        signal. text_filter may be regular expression."""

        if text_filter:
            test_regex = QtCore.QRegularExpression(text_filter)
            if test_regex.isValid():
                self.filter_validity_label.setToolTip(
                    translate("AddonsInstaller", "Filter is valid")
                )
                icon = QtGui.QIcon.fromTheme("ok", QtGui.QIcon(":/icons/edit_OK.svg"))
                self.filter_validity_label.setPixmap(icon.pixmap(16, 16))
            else:
                self.filter_validity_label.setToolTip(
                    translate("AddonsInstaller", "Filter regular expression is invalid")
                )
                icon = QtGui.QIcon.fromTheme("cancel", QtGui.QIcon(":/icons/edit_Cancel.svg"))
                self.filter_validity_label.setPixmap(icon.pixmap(16, 16))
            self.filter_validity_label.show()
        else:
            self.filter_validity_label.hide()
        self.search_changed.emit(text_filter)

    def retranslateUi(self, _):
        self.filter_line_edit.setPlaceholderText(
            QtCore.QCoreApplication.translate("AddonsInstaller", "Search...", None)
        )
