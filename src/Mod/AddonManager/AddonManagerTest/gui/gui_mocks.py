# SPDX-License-Identifier: LGPL-2.1-or-later
# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2022-2023 FreeCAD Project Association                   *
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

from PySide import QtCore, QtWidgets

from AddonManagerTest.app.mocks import SignalCatcher


class DialogInteractor(QtCore.QObject):
    """Takes the title of the dialog and a callable. The callable is passed the widget
    we found and can do whatever it wants to it. Whatever it does should eventually
    close the dialog, however."""

    def __init__(self, dialog_to_watch_for, interaction):
        super().__init__()

        # Status variables for tests to check:
        self.dialog_found = False
        self.has_run = False
        self.button_found = False
        self.interaction = interaction

        self.dialog_to_watch_for = dialog_to_watch_for

        self.execution_counter = 0
        self.timer = QtCore.QTimer()
        self.timer.timeout.connect(self.run)
        self.timer.start(10)

    def run(self):
        widget = QtWidgets.QApplication.activeModalWidget()
        if widget and self._dialog_matches(widget):
            # Found the dialog we are looking for: now try to run the interaction
            if self.interaction is not None and callable(self.interaction):
                self.interaction(widget)
            self.dialog_found = True
            self.timer.stop()

        self.has_run = True
        self.execution_counter += 1
        if self.execution_counter > 100:
            print("Stopped timer after 100 iterations")
            self.timer.stop()

    def _dialog_matches(self, widget) -> bool:
        # Is this the widget we are looking for? Only applies on Linux and Windows: macOS
        # doesn't set the title of a modal dialog:
        os = QtCore.QSysInfo.productType()  # Qt5 gives "osx", Qt6 gives "macos"
        if os in ["osx", "macos"] or (
            hasattr(widget, "windowTitle")
            and callable(widget.windowTitle)
            and widget.windowTitle() == self.dialog_to_watch_for
        ):
            return True
        return False


class DialogWatcher(DialogInteractor):
    """Examine the running GUI and look for a modal dialog with a given title, containing a button
    with a role. Click that button, which is expected to close the dialog. Generally run on
    a one-shot QTimer to allow the dialog time to open up. If the specified dialog is found, but
    it does not contain the expected button, button_found will be false, and the dialog will be
    closed with a reject() slot."""

    def __init__(self, dialog_to_watch_for, button=QtWidgets.QDialogButtonBox.NoButton):
        super().__init__(dialog_to_watch_for, self.click_button)
        if button != QtWidgets.QDialogButtonBox.NoButton:
            self.button = button
        else:
            self.button = QtWidgets.QDialogButtonBox.Cancel

    def click_button(self, widget):
        button_boxes = widget.findChildren(QtWidgets.QDialogButtonBox)
        if len(button_boxes) == 1:  # There should be one, and only one
            button_to_click = button_boxes[0].button(self.button)
            if button_to_click:
                self.button_found = True
                button_to_click.click()
            else:
                widget.reject()
        else:
            widget.reject()


class FakeWorker:
    def __init__(self):
        self.called = False
        self.should_continue = True

    def work(self):
        while self.should_continue:
            QtCore.QCoreApplication.processEvents(QtCore.QEventLoop.AllEvents, 100)

    def stop(self):
        self.should_continue = False


class MockThread:
    def wait(self):
        pass

    def isRunning(self):
        return False


class AsynchronousMonitor:
    """Watch for a signal to be emitted for at most some given number of milliseconds"""

    def __init__(self, signal):
        self.signal = signal
        self.signal_catcher = SignalCatcher()
        self.signal.connect(self.signal_catcher.catch_signal)
        self.kill_timer = QtCore.QTimer()
        self.kill_timer.setSingleShot(True)
        self.kill_timer.timeout.connect(self.signal_catcher.die)

    def wait_for_at_most(self, max_wait_millis) -> None:
        self.kill_timer.setInterval(max_wait_millis)
        self.kill_timer.start()
        while not self.signal_catcher.caught and not self.signal_catcher.killed:
            QtCore.QCoreApplication.processEvents(QtCore.QEventLoop.AllEvents, 10)
        self.kill_timer.stop()

    def good(self) -> bool:
        return self.signal_catcher.caught and not self.signal_catcher.killed
