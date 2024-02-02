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

""" Defines a class derived from QWidget for displaying the bar at the top of the addons list. """

from enum import IntEnum
from addonmanager_widget_view_selector import WidgetViewSelector

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

translate = FreeCAD.Qt.translate


class StatusFilter(IntEnum):
    """Predefined filers"""

    ANY = 0
    INSTALLED = 1
    NOT_INSTALLED = 2
    UPDATE_AVAILABLE = 3


# pylint: disable=too-few-public-methods


class WidgetTopBar(QtWidgets.QWidget):
    """A widget to display the buttons at the top of the Addon manager, for changing the view,
    filtering, and sorting."""

    view_changed = QtCore.Signal(int)
    filter_changed = QtCore.Signal(str)
    search_changed = QtCore.Signal(str)

    def __init__(self, parent=None):
        super().__init__(parent)
        self.horizontal_layout = None
        self.package_type_label = None
        self.package_type_combobox = None
        self._setup_ui()
        self._setup_connections()
        self.retranslateUi()

    def _setup_ui(self):
        self.horizontal_layout = QtWidgets.QHBoxLayout()

        self.view_selector = WidgetViewSelector(self)
        self.horizontal_layout.addWidget(self.view_selector)

        self.package_type_label = QtWidgets.QLabel(self)
        self.horizontal_layout.addWidget(self.package_type_label)

        self.package_type_combobox = QtWidgets.QComboBox(self)
        self.horizontal_layout.addWidget(self.package_type_combobox)

        self.status_combo_box = QtWidgets.QComboBox(self)
        self.horizontal_layout.addWidget(self.status_combo_box)

        self.filter_line_edit = QtWidgets.QLineEdit(self)
        self.horizontal_layout.addWidget(self.filter_line_edit)

        # Only shows when the user types in a filter
        self.ui.filter_validity_label = QtWidgets.QLabel(self)
        self.horizontal_layout.addWidget(self.filter_validity_label)
        self.ui.filter_validity_label.hide()

    def _setup_connections(self):
        self.ui.view_selector.view_changed.connect(self.view_changed.emit)

        # Set up the view the same as the last time:
        pref = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Addons")
        package_type = pref.GetInt("PackageTypeSelection", 1)
        self.ui.comboPackageType.setCurrentIndex(package_type)
        status = pref.GetInt("StatusSelection", 0)
        self.ui.comboStatus.setCurrentIndex(status)

    def update_type_filter(self, type_filter: int) -> None:
        """hide/show rows corresponding to the type filter

        type_filter is an integer: 0 for all, 1 for workbenches, 2 for macros,
        and 3 for preference packs

        """

        self.item_filter.setPackageFilter(type_filter)
        pref = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Addons")
        pref.SetInt("PackageTypeSelection", type_filter)

    def update_status_filter(self, status_filter: int) -> None:
        """hide/show rows corresponding to the status filter

        status_filter is an integer: 0 for any, 1 for installed, 2 for not installed,
        and 3 for update available

        """

        self.item_filter.setStatusFilter(status_filter)
        pref = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Addons")
        pref.SetInt("StatusSelection", status_filter)

    def update_text_filter(self, text_filter: str) -> None:
        """filter name and description by the regex specified by text_filter"""

        if text_filter:
            if hasattr(self.item_filter, "setFilterRegularExpression"):  # Added in Qt 5.12
                test_regex = QtCore.QRegularExpression(text_filter)
            else:
                test_regex = QtCore.QRegExp(text_filter)
            if test_regex.isValid():
                self.ui.labelFilterValidity.setToolTip(
                    translate("AddonsInstaller", "Filter is valid")
                )
                icon = QtGui.QIcon.fromTheme("ok", QtGui.QIcon(":/icons/edit_OK.svg"))
                self.ui.labelFilterValidity.setPixmap(icon.pixmap(16, 16))
            else:
                self.ui.labelFilterValidity.setToolTip(
                    translate("AddonsInstaller", "Filter regular expression is invalid")
                )
                icon = QtGui.QIcon.fromTheme("cancel", QtGui.QIcon(":/icons/edit_Cancel.svg"))
                self.ui.labelFilterValidity.setPixmap(icon.pixmap(16, 16))
            self.ui.labelFilterValidity.show()
        else:
            self.ui.labelFilterValidity.hide()
        if hasattr(self.item_filter, "setFilterRegularExpression"):  # Added in Qt 5.12
            self.item_filter.setFilterRegularExpression(text_filter)
        else:
            self.item_filter.setFilterRegExp(text_filter)
