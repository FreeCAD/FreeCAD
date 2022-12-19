# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2022 FreeCAD Project Association                        *
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


class DialogWatcher(QtCore.QObject):
    """Examine the running GUI and look for a modal dialog with a given title, containing a button
    with a given label. Click that button, which is expected to close the dialog. Generally run on
    a one-shot QTimer to allow the dialog time to open up."""

    def __init__(self, dialog_to_watch_for, button):
        super().__init__()
        self.dialog_found = False
        self.has_run = False
        self.dialog_to_watch_for = dialog_to_watch_for
        self.button = button

    def run(self):
        widget = QtWidgets.QApplication.activeModalWidget()
        if widget:
            # Is this the widget we are looking for?
            if (
                hasattr(widget, "windowTitle")
                and callable(widget.windowTitle)
                and widget.windowTitle() == self.dialog_to_watch_for
            ):
                # Found the dialog we are looking for: now try to "click" the appropriate button
                self.click_button(widget)
                self.dialog_found = True
        self.has_run = True

    def click_button(self, widget):
        buttons = widget.findChildren(QtWidgets.QPushButton)
        for button in buttons:
            text = button.text().replace("&", "")
            if text == self.button:
                button.click()


class DialogInteractor(DialogWatcher):
    def __init__(self, dialog_to_watch_for, interaction):
        """Takes the title of the dialog, a button string, and a callable. The callable is passed
        the widget we found and can do whatever it wants to it. Whatever it does should eventually
        close the dialog, however."""
        super().__init__(dialog_to_watch_for, None)
        self.interaction = interaction

    def run(self):
        widget = QtWidgets.QApplication.activeModalWidget()
        if widget:
            # Is this the widget we are looking for?
            if (
                hasattr(widget, "windowTitle")
                and callable(widget.windowTitle)
                and widget.windowTitle() == self.dialog_to_watch_for
            ):
                self.dialog_found = True
        if self.dialog_found:
            self.has_run = True
            if self.interaction is not None and callable(self.interaction):
                self.interaction(widget)


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
