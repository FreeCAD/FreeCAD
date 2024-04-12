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

""" Defines a QWidget-derived class for displaying the cache load status. """

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

_TOTAL_INCREMENTS = 1000


class WidgetProgressBar(QtWidgets.QWidget):
    """A multipart progress bar widget, including a stop button and a status label. Defaults to a
    single range with 100 increments, but can be configured with any number of major and minor
    ranges. Clicking the stop button will emit a signal, but does not otherwise affect the
    widget."""

    stop_clicked = QtCore.Signal()

    def __init__(self, parent: QtWidgets.QWidget = None):
        super().__init__(parent)
        self.vertical_layout = None
        self.horizontal_layout = None
        self.progress_bar = None
        self.status_label = None
        self.stop_button = None
        self._setup_ui()

    def _setup_ui(self):
        self.vertical_layout = QtWidgets.QVBoxLayout(self)
        self.horizontal_layout = QtWidgets.QHBoxLayout()
        self.progress_bar = QtWidgets.QProgressBar(self)
        self.status_label = QtWidgets.QLabel(self)
        self.stop_button = QtWidgets.QToolButton(self)
        self.progress_bar.setMaximum(_TOTAL_INCREMENTS)
        self.stop_button.clicked.connect(self.stop_clicked)
        self.stop_button.setIcon(
            QtGui.QIcon.fromTheme("stop", QtGui.QIcon(":/icons/media-playback-stop.svg"))
        )
        self.vertical_layout.addLayout(self.horizontal_layout)
        self.vertical_layout.addWidget(self.status_label)
        self.horizontal_layout.addWidget(self.progress_bar)
        self.horizontal_layout.addWidget(self.stop_button)
        self.vertical_layout.setContentsMargins(0, 0, 0, 0)
        self.horizontal_layout.setContentsMargins(0, 0, 0, 0)

    def set_status(self, status: str):
        self.status_label.setText(status)

    def set_value(self, value: int):
        self.progress_bar.setValue(value)
