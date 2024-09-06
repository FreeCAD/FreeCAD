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

    def translate(context: str, text: str):
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


class AddonManagerDisplayStyle(IntEnum):
    """The display mode of the Addon Manager"""

    COMPACT = 0
    EXPANDED = 1
    COMPOSITE = 2


class WidgetViewSelector(QtWidgets.QWidget):
    """A widget for selecting the Addon Manager's primary view mode"""

    view_changed = QtCore.Signal(int)

    def __init__(self, parent: QtWidgets.QWidget = None):
        super().__init__(parent)
        self.horizontal_layout = None
        self.composite_button = None
        self.expanded_button = None
        self.compact_button = None
        self._setup_ui()
        self._setup_connections()

    def set_current_view(self, view: AddonManagerDisplayStyle):
        """Set the current selection. Does NOT emit a view_changed signal, only changes the
        interface display."""
        self.compact_button.setChecked(False)
        self.expanded_button.setChecked(False)
        self.composite_button.setChecked(False)
        if view == AddonManagerDisplayStyle.COMPACT:
            self.compact_button.setChecked(True)
        elif view == AddonManagerDisplayStyle.EXPANDED:
            self.expanded_button.setChecked(True)
        elif view == AddonManagerDisplayStyle.COMPOSITE:
            self.composite_button.setChecked(True)
        else:
            if FreeCAD is not None:
                FreeCAD.Console.PrintWarning(f"Unrecognized display style {view}")

    def _setup_ui(self):
        self.horizontal_layout = QtWidgets.QHBoxLayout()
        self.horizontal_layout.setContentsMargins(0, 0, 0, 0)
        self.horizontal_layout.setSpacing(2)
        self.compact_button = QtWidgets.QToolButton(self)
        self.compact_button.setObjectName("compact_button")
        self.compact_button.setIcon(
            QtGui.QIcon.fromTheme("back", QtGui.QIcon(":/icons/compact_view.svg"))
        )
        self.compact_button.setCheckable(True)
        self.compact_button.setAutoExclusive(True)

        self.expanded_button = QtWidgets.QToolButton(self)
        self.expanded_button.setObjectName("expanded_button")
        self.expanded_button.setCheckable(True)
        self.expanded_button.setChecked(True)
        self.expanded_button.setAutoExclusive(True)
        self.expanded_button.setIcon(
            QtGui.QIcon.fromTheme("expanded_view", QtGui.QIcon(":/icons/expanded_view.svg"))
        )

        self.composite_button = QtWidgets.QToolButton(self)
        self.composite_button.setObjectName("composite_button")
        if (
            QtCore.QLibraryInfo.version().majorVersion() == 5
            and QtCore.QLibraryInfo.version().minorVersion() < 15
        ):
            self.composite_button.setEnabled(False)
            self.composite_button.setCheckable(False)
            self.composite_button.setChecked(False)
        else:
            self.composite_button.setCheckable(True)
            self.composite_button.setChecked(True)
        self.composite_button.setAutoExclusive(True)
        self.composite_button.setIcon(
            QtGui.QIcon.fromTheme("composite_button", QtGui.QIcon(":/icons/composite_view.svg"))
        )
        self.horizontal_layout.addWidget(self.compact_button)
        self.horizontal_layout.addWidget(self.expanded_button)
        self.horizontal_layout.addWidget(self.composite_button)

        self.compact_button.clicked.connect(
            lambda: self.view_changed.emit(AddonManagerDisplayStyle.COMPACT)
        )
        self.expanded_button.clicked.connect(
            lambda: self.view_changed.emit(AddonManagerDisplayStyle.EXPANDED)
        )
        self.composite_button.clicked.connect(
            lambda: self.view_changed.emit(AddonManagerDisplayStyle.COMPOSITE)
        )

        self.setLayout(self.horizontal_layout)
        self.retranslateUi(None)

    def _setup_connections(self):
        self.compact_button.clicked.connect(
            lambda: self.view_changed.emit(AddonManagerDisplayStyle.COMPACT)
        )
        self.expanded_button.clicked.connect(
            lambda: self.view_changed.emit(AddonManagerDisplayStyle.EXPANDED)
        )
        self.composite_button.clicked.connect(
            lambda: self.view_changed.emit(AddonManagerDisplayStyle.COMPOSITE)
        )

    def retranslateUi(self, _):
        self.composite_button.setToolTip(translate("AddonsInstaller", "Composite view"))
        self.expanded_button.setToolTip(translate("AddonsInstaller", "Expanded view"))
        self.compact_button.setToolTip(translate("AddonsInstaller", "Compact view"))
