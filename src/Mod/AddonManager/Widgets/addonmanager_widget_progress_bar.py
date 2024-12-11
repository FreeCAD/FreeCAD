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
    from PySide import QtCore, QtGui, QtWidgets  # Use the FreeCAD wrapper
except ImportError:
    try:
        from PySide6 import QtCore, QtGui, QtWidgets  # Outside FreeCAD, try Qt6 first
    except ImportError:
        from PySide2 import QtCore, QtGui, QtWidgets  # Fall back to Qt5

from dataclasses import dataclass

_TOTAL_INCREMENTS = 1000


class Progress:
    """Represents progress through a process composed of multiple sub-tasks."""

    def __init__(
        self,
        *,
        status_text: str = "",
        number_of_tasks: int = 1,
        current_task: int = 0,
        current_task_progress: float = 0.0,
    ):
        if number_of_tasks < 1:
            raise ValueError(f"Number of tasks must be at least one, not {number_of_tasks}")
        if current_task < 0 or current_task >= number_of_tasks:
            raise ValueError(
                "Current task must be between 0 and the number of tasks "
                f"({number_of_tasks}), not {current_task}"
            )
        if current_task_progress < 0.0:
            current_task_progress = 0.0
        elif current_task_progress > 100.0:
            current_task_progress = 100.0
        self.status_text: str = status_text
        self._number_of_tasks: int = number_of_tasks
        self._current_task: int = current_task
        self._current_task_progress: float = current_task_progress

    @property
    def number_of_tasks(self):
        return self._number_of_tasks

    @number_of_tasks.setter
    def number_of_tasks(self, value: int):
        if not isinstance(value, int):
            raise TypeError("Number of tasks must be an integer")
        if value < 1:
            raise ValueError("Number of tasks must be at least one")
        self._number_of_tasks = value

    @property
    def current_task(self):
        """The current task (zero-indexed, always less than the number of tasks)"""
        return self._current_task

    @current_task.setter
    def current_task(self, value: int):
        if not isinstance(value, int):
            raise TypeError("Current task must be an integer")
        if value < 0:
            raise ValueError("Current task must be at least zero")
        if value >= self._number_of_tasks:
            raise ValueError("Current task must be less than the total number of tasks")
        self._current_task = value

    @property
    def current_task_progress(self):
        """Current task progress, guaranteed to be in the range [0.0, 100.0]. Attempts to set a
        value outside that range are clamped to the range."""
        return self._current_task_progress

    @current_task_progress.setter
    def current_task_progress(self, value: float):
        """Set the current task's progress. Rather than raising an exception when the value is
        outside the expected range of [0,100], clamp the task progress to allow for some
        floating point imprecision in its calculation."""
        if value < 0.0:
            value = 0.0
        elif value > 100.0:
            value = 100.0
        self._current_task_progress = value

    def next_task(self) -> None:
        """Increment the task counter and reset the progress"""
        self.current_task += 1
        self.current_task_progress = 0.0

    def overall_progress(self) -> float:
        """Gets the overall progress as a fractional value in the range [0, 1]"""
        base = self._current_task / self._number_of_tasks
        fraction = self._current_task_progress / (100.0 * self._number_of_tasks)
        return base + fraction


class WidgetProgressBar(QtWidgets.QWidget):
    """A multipart progress bar widget, including a stop button and a status label."""

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
            QtGui.QIcon.fromTheme("stop", QtGui.QIcon(":/icons/debug-stop.svg"))
        )
        self.vertical_layout.addLayout(self.horizontal_layout)
        self.vertical_layout.addWidget(self.status_label)
        self.horizontal_layout.addWidget(self.progress_bar)
        self.horizontal_layout.addWidget(self.stop_button)
        self.vertical_layout.setContentsMargins(0, 0, 0, 0)
        self.horizontal_layout.setContentsMargins(0, 0, 0, 0)

    def set_progress(self, progress: Progress) -> None:
        self.status_label.setText(progress.status_text)
        self.progress_bar.setValue(progress.overall_progress() * _TOTAL_INCREMENTS)
