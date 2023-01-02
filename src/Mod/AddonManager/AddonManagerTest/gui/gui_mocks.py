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
    a one-shot QTimer to allow the dialog time to open up. If the specified dialog is found, but
    it does not contain the expected button, button_found will be false, and the dialog will be
    closed with a reject() slot."""

    def __init__(self, dialog_to_watch_for, button=QtWidgets.QDialogButtonBox.NoButton):
        super().__init__()

        # Status variables for tests to check:
        self.dialog_found = False
        self.has_run = False
        self.button_found = False

        self.dialog_to_watch_for = dialog_to_watch_for
        if button != QtWidgets.QDialogButtonBox.NoButton:
            self.button = button
        else:
            self.button = QtWidgets.QDialogButtonBox.Cancel

        self.execution_counter = 0
        self.timer = QtCore.QTimer()
        self.timer.timeout.connect(self.run)
        self.timer.start(10)

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
                self.timer.stop()
        self.has_run = True
        self.execution_counter += 1
        if self.execution_counter > 100:
            print("Stopper timer after 100 iterations")
            self.timer.stop()

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
