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
from .addonmanager_widget_view_selector import WidgetViewSelector
from .addonmanager_widget_filter_selector import WidgetFilterSelector
from .addonmanager_widget_search import WidgetSearch


class WidgetViewControlBar(QtWidgets.QWidget):
    """A bar containing a view selection widget, a filter widget, and a search widget"""

    view_changed = QtCore.Signal(int)
    filter_changed = QtCore.Signal(object)
    search_changed = QtCore.Signal(str)

    def __init__(self, parent: QtWidgets.QWidget = None):
        super().__init__(parent)
        self._setup_ui()
        self._setup_connections()
        self.retranslateUi(None)

    def _setup_ui(self):
        self.horizontal_layout = QtWidgets.QHBoxLayout()
        self.horizontal_layout.setContentsMargins(0, 0, 0, 0)
        self.view_selector = WidgetViewSelector(self)
        self.filter_selector = WidgetFilterSelector(self)
        self.search = WidgetSearch(self)
        self.horizontal_layout.addWidget(self.view_selector)
        self.horizontal_layout.addWidget(self.filter_selector)
        self.horizontal_layout.addWidget(self.search)
        self.setLayout(self.horizontal_layout)

    def _setup_connections(self):
        self.view_selector.view_changed.connect(self.view_changed.emit)
        self.filter_selector.filter_changed.connect(self.filter_changed.emit)
        self.search.search_changed.connect(self.search_changed.emit)

    def retranslateUi(self, _=None):
        pass
