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

""" Defines a QWidget-derived class for displaying the view selection buttons. """

from enum import IntEnum

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


class FilterType(IntEnum):
    """There are currently two sections in this drop down, for two different types of filters."""

    PACKAGE_CONTENTS = 0
    INSTALLATION_STATUS = 1


class StatusFilter(IntEnum):
    """Predefined filters for status"""

    ANY = 0
    INSTALLED = 1
    NOT_INSTALLED = 2
    UPDATE_AVAILABLE = 3


class ContentFilter(IntEnum):
    """Predefined filters for addon content type"""

    ANY = 0
    WORKBENCH = 1
    MACRO = 2
    PREFERENCE_PACK = 3


class Filter:
    def __init__(self):
        self.status_filter = StatusFilter.ANY
        self.content_filter = ContentFilter.ANY


class WidgetFilterSelector(QtWidgets.QComboBox):
    """A label and menu for selecting what sort of addons are displayed"""

    filter_changed = QtCore.Signal(object)  # technically, actually class Filter

    def __init__(self, parent: QtWidgets.QWidget = None):
        super().__init__(parent)
        self.addon_type_index = 0
        self.installation_status_index = 0
        self.extra_padding = 64
        self._setup_ui()
        self._setup_connections()
        self.retranslateUi(None)
        self.setSizeAdjustPolicy(QtWidgets.QComboBox.AdjustToContents)

    def _setup_ui(self):
        self._build_menu()

    def _build_menu(self):
        self.clear()
        self.addItem(translate("AddonsInstaller", "Filter by..."))
        self.insertSeparator(self.count())
        self.addItem(translate("AddonsInstaller", "Addon Type"))
        self.addon_type_index = self.count() - 1
        self.addItem(
            translate("AddonsInstaller", "Any"), (FilterType.PACKAGE_CONTENTS, ContentFilter.ANY)
        )
        self.addItem(
            translate("AddonsInstaller", "Workbench"),
            (FilterType.PACKAGE_CONTENTS, ContentFilter.WORKBENCH),
        )
        self.addItem(
            translate("AddonsInstaller", "Macro"),
            (FilterType.PACKAGE_CONTENTS, ContentFilter.MACRO),
        )
        self.addItem(
            translate("AddonsInstaller", "Preference Pack"),
            (FilterType.PACKAGE_CONTENTS, ContentFilter.PREFERENCE_PACK),
        )
        self.insertSeparator(self.count())
        self.addItem(translate("AddonsInstaller", "Installation Status"))
        self.installation_status_index = self.count() - 1
        self.addItem(
            translate("AddonsInstaller", "Any"), (FilterType.INSTALLATION_STATUS, StatusFilter.ANY)
        )
        self.addItem(
            translate("AddonsInstaller", "Not installed"),
            (FilterType.INSTALLATION_STATUS, StatusFilter.NOT_INSTALLED),
        )
        self.addItem(
            translate("AddonsInstaller", "Installed"),
            (FilterType.INSTALLATION_STATUS, StatusFilter.INSTALLED),
        )
        self.addItem(
            translate("AddonsInstaller", "Update available"),
            (FilterType.INSTALLATION_STATUS, StatusFilter.UPDATE_AVAILABLE),
        )
        model: QtCore.QAbstractItemModel = self.model()
        for row in range(model.rowCount()):
            if row <= self.addon_type_index:
                model.item(row).setEnabled(False)
            elif row < self.installation_status_index:
                item = model.item(row)
                item.setCheckState(QtCore.Qt.Unchecked)
            elif row == self.installation_status_index:
                model.item(row).setEnabled(False)
            else:
                item = model.item(row)
                item.setCheckState(QtCore.Qt.Unchecked)

        for row in range(model.rowCount()):
            data = self.itemData(row)
            if data:
                item = model.item(row)
                if data[0] == FilterType.PACKAGE_CONTENTS and data[1] == ContentFilter.ANY:
                    item.setCheckState(QtCore.Qt.Checked)
                elif data[0] == FilterType.INSTALLATION_STATUS and data[1] == StatusFilter.ANY:
                    item.setCheckState(QtCore.Qt.Checked)
                else:
                    item.setCheckState(QtCore.Qt.Unchecked)

    def set_contents_filter(self, contents_filter: ContentFilter):
        model = self.model()
        for row in range(model.rowCount()):
            item = model.item(row)
            user_data = self.itemData(row)
            if user_data and user_data[0] == FilterType.PACKAGE_CONTENTS:
                if user_data[1] == contents_filter:
                    item.setCheckState(QtCore.Qt.Checked)
                else:
                    item.setCheckState(QtCore.Qt.Unchecked)
        self._update_first_row_text()

    def set_status_filter(self, status_filter: StatusFilter):
        model = self.model()
        for row in range(model.rowCount()):
            item = model.item(row)
            user_data = self.itemData(row)
            if user_data and user_data[0] == FilterType.INSTALLATION_STATUS:
                if user_data[1] == status_filter:
                    item.setCheckState(QtCore.Qt.Checked)
                else:
                    item.setCheckState(QtCore.Qt.Unchecked)
        self._update_first_row_text()

    def _setup_connections(self):
        self.activated.connect(self._selected)

    def _adjust_dropdown_width(self):
        max_width = 0
        font_metrics = self.fontMetrics()
        for index in range(self.count()):
            width = font_metrics.horizontalAdvance(self.itemText(index))
            max_width = max(max_width, width)
        self.view().setMinimumWidth(max_width + self.extra_padding)

    def retranslateUi(self, _):
        self._build_menu()
        self._adjust_dropdown_width()

    def _selected(self, row: int):
        if row == 0:
            return
        if row == self.installation_status_index or row == self.addon_type_index:
            self.setCurrentIndex(0)
            return
        model = self.model()
        selected_data = self.itemData(row)
        if not selected_data:
            return
        selected_row_type = selected_data[0]

        for row in range(model.rowCount()):
            item = model.item(row)
            user_data = self.itemData(row)
            if user_data and user_data[0] == selected_row_type:
                if user_data[1] == selected_data[1]:
                    item.setCheckState(QtCore.Qt.Checked)
                else:
                    item.setCheckState(QtCore.Qt.Unchecked)
        self._emit_current_filter()
        self.setCurrentIndex(0)
        self._update_first_row_text()

    def _emit_current_filter(self):
        model = self.model()
        new_filter = Filter()
        for row in range(model.rowCount()):
            item = model.item(row)
            data = self.itemData(row)
            if data and item.checkState() == QtCore.Qt.Checked:
                if data[0] == FilterType.INSTALLATION_STATUS:
                    new_filter.status_filter = data[1]
                elif data[0] == FilterType.PACKAGE_CONTENTS:
                    new_filter.content_filter = data[1]
        self.filter_changed.emit(new_filter)

    def _update_first_row_text(self):
        model = self.model()
        state1 = ""
        state2 = ""
        for row in range(model.rowCount()):
            item = model.item(row)
            if item.checkState() == QtCore.Qt.Checked:
                if not state1:
                    state1 = item.text()
                else:
                    state2 = item.text()
                    break
        model.item(0).setText(translate("AddonsInstaller", "Filter") + f": {state1}, {state2}")
