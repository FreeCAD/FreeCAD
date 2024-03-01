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

from enum import IntEnum, auto

try:
    import FreeCAD
except ImportError:
    FreeCAD = None

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
from .addonmanager_widget_view_selector import WidgetViewSelector
from .addonmanager_widget_filter_selector import WidgetFilterSelector
from .addonmanager_widget_search import WidgetSearch

translate = QtCore.QCoreApplication.translate


class SortOptions(IntEnum):
    _SortRoleOffset = 100
    Alphabetical = QtCore.Qt.UserRole + _SortRoleOffset + 0
    LastUpdated = QtCore.Qt.UserRole + _SortRoleOffset + 1
    DateAdded = QtCore.Qt.UserRole + _SortRoleOffset + 2
    Stars = QtCore.Qt.UserRole + _SortRoleOffset + 3
    Score = QtCore.Qt.UserRole + _SortRoleOffset + 4


default_sort_order = {
    SortOptions.Alphabetical: QtCore.Qt.AscendingOrder,
    SortOptions.LastUpdated: QtCore.Qt.DescendingOrder,
    SortOptions.DateAdded: QtCore.Qt.DescendingOrder,
    SortOptions.Stars: QtCore.Qt.DescendingOrder,
    SortOptions.Score: QtCore.Qt.DescendingOrder,
}


class WidgetViewControlBar(QtWidgets.QWidget):
    """A bar containing a view selection widget, a filter widget, and a search widget"""

    view_changed = QtCore.Signal(int)
    filter_changed = QtCore.Signal(object)
    search_changed = QtCore.Signal(str)
    sort_changed = QtCore.Signal(int)
    sort_order_changed = QtCore.Signal(QtCore.Qt.SortOrder)

    def __init__(self, parent: QtWidgets.QWidget = None):
        super().__init__(parent)
        self.has_rankings = False
        self._setup_ui()
        self._setup_connections()
        self.retranslateUi(None)
        self.sort_order = QtCore.Qt.AscendingOrder
        self._set_sort_order_icon()

    def _setup_ui(self):
        self.horizontal_layout = QtWidgets.QHBoxLayout()
        self.horizontal_layout.setContentsMargins(0, 0, 0, 0)
        self.view_selector = WidgetViewSelector(self)
        self.filter_selector = WidgetFilterSelector(self)
        self.sort_selector = QtWidgets.QComboBox(self)
        self.sort_order_button = QtWidgets.QToolButton(self)
        self.sort_order_button.setIcon(
            QtGui.QIcon.fromTheme("ascending", QtGui.QIcon(":/icons/sort_ascending.svg"))
        )
        self.search = WidgetSearch(self)
        self.horizontal_layout.addWidget(self.view_selector)
        self.horizontal_layout.addWidget(self.filter_selector)
        self.horizontal_layout.addWidget(self.sort_selector)
        self.horizontal_layout.addWidget(self.sort_order_button)
        self.horizontal_layout.addWidget(self.search)
        self.setLayout(self.horizontal_layout)

    def _sort_order_clicked(self):
        if self.sort_order == QtCore.Qt.AscendingOrder:
            self.set_sort_order(QtCore.Qt.DescendingOrder)
        else:
            self.set_sort_order(QtCore.Qt.AscendingOrder)
        self.sort_order_changed.emit(self.sort_order)

    def set_sort_order(self, order: QtCore.Qt.SortOrder) -> None:
        self.sort_order = order
        self._set_sort_order_icon()

    def _set_sort_order_icon(self):
        if self.sort_order == QtCore.Qt.AscendingOrder:
            self.sort_order_button.setIcon(
                QtGui.QIcon.fromTheme(
                    "view-sort-ascending", QtGui.QIcon(":/icons/sort_ascending.svg")
                )
            )
        else:
            self.sort_order_button.setIcon(
                QtGui.QIcon.fromTheme(
                    "view-sort-descending", QtGui.QIcon(":/icons/sort_descending.svg")
                )
            )

    def set_rankings_available(self, rankings_available: bool) -> None:
        self.has_rankings = rankings_available
        self.retranslateUi(None)

    def _setup_connections(self):
        self.view_selector.view_changed.connect(self.view_changed.emit)
        self.filter_selector.filter_changed.connect(self.filter_changed.emit)
        self.search.search_changed.connect(self.search_changed.emit)
        self.sort_selector.currentIndexChanged.connect(self._sort_changed)
        self.sort_order_button.clicked.connect(self._sort_order_clicked)

    def _sort_changed(self, index: int):
        sort_role = self.sort_selector.itemData(index)
        if sort_role is None:
            sort_role = SortOptions.Alphabetical
        self.set_sort_order(default_sort_order[sort_role])
        self.sort_changed.emit(sort_role)
        self.sort_order_changed.emit(self.sort_order)

    def retranslateUi(self, _=None):
        self.sort_selector.clear()
        self.sort_selector.addItem(
            translate("AddonsInstaller", "Alphabetical", "Sort order"), SortOptions.Alphabetical
        )
        self.sort_selector.addItem(
            translate("AddonsInstaller", "Last Updated", "Sort order"), SortOptions.LastUpdated
        )
        self.sort_selector.addItem(
            translate("AddonsInstaller", "Date Created", "Sort order"), SortOptions.DateAdded
        )
        self.sort_selector.addItem(
            translate("AddonsInstaller", "GitHub Stars", "Sort order"), SortOptions.Stars
        )
        if self.has_rankings:
            self.sort_selector.addItem(
                translate("AddonsInstaller", "Score", "Sort order"), SortOptions.Score
            )
