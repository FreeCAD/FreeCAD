# SPDX-License-Identifier: LGPL-2.1-or-later
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

"""Class to manage the display of an Update All dialog."""

from enum import IntEnum, auto
import os
from typing import List

import FreeCAD
import FreeCADGui

from PySide import QtCore, QtWidgets

from Addon import Addon

from addonmanager_installer import AddonInstaller, MacroInstaller

translate = FreeCAD.Qt.translate

# pylint: disable=too-few-public-methods,too-many-instance-attributes


class UpdaterFactory:
    """A factory class for generating updaters. Mainly exists to allow easily mocking
    those updaters during testing. A replacement class need only provide a
    "get_updater" function that returns mock updater objects. Those objects must be
    QObjects with a run() function and a finished signal."""

    def __init__(self, addons):
        self.addons = addons

    def get_updater(self, addon):
        """Get an updater for this addon (either a MacroInstaller or an
        AddonInstaller)"""
        if addon.macro is not None:
            return MacroInstaller(addon)
        return AddonInstaller(addon, self.addons)


class AddonStatus(IntEnum):
    """The current status of the installation process for a given addon"""

    WAITING = auto()
    INSTALLING = auto()
    SUCCEEDED = auto()
    FAILED = auto()

    def ui_string(self):
        """Get the string that the UI should show for this status"""
        if self.value == AddonStatus.WAITING:
            return ""
        if self.value == AddonStatus.INSTALLING:
            return translate("AddonsInstaller", "Installing") + "..."
        if self.value == AddonStatus.SUCCEEDED:
            return translate("AddonsInstaller", "Succeeded")
        if self.value == AddonStatus.FAILED:
            return translate("AddonsInstaller", "Failed")
        return "[INTERNAL ERROR]"


class UpdateAllGUI(QtCore.QObject):
    """A GUI to display and manage an "update all" process."""

    finished = QtCore.Signal()
    addon_updated = QtCore.Signal(object)

    def __init__(self, addons: List[Addon]):
        super().__init__()
        self.addons = addons
        self.dialog = FreeCADGui.PySideUic.loadUi(
            os.path.join(os.path.dirname(__file__), "update_all.ui")
        )
        self.dialog.setWindowFlag(QtCore.Qt.WindowStaysOnTopHint, True)
        self.row_map = {}
        self.in_process_row = None
        self.active_installer = None
        self.addons_with_update: List[Addon] = []
        self.updater_factory = UpdaterFactory(addons)
        self.worker_thread = None
        self.running = False
        self.cancelled = False

    def run(self):
        """Run the Update All process. Blocks until updates are complete or
        cancelled."""
        self.running = True
        self._setup_dialog()
        self.dialog.show()
        self._process_next_update()

    def _setup_dialog(self):
        """Prepare the dialog for display"""
        self.dialog.rejected.connect(self._cancel_installation)
        self.dialog.tableWidget.clear()
        self.in_process_row = None
        self.row_map = {}
        for addon in self.addons:
            if addon.status() == Addon.Status.UPDATE_AVAILABLE:
                self._add_addon_to_table(addon)
                self.addons_with_update.append(addon)

    def _cancel_installation(self):
        self.cancelled = True
        if self.worker_thread and self.worker_thread.isRunning():
            self.worker_thread.requestInterruption()

    def _add_addon_to_table(self, addon: Addon):
        """Add the given addon to the list, with no icon in the first column"""
        new_row = self.dialog.tableWidget.rowCount()
        self.dialog.tableWidget.setColumnCount(2)
        self.dialog.tableWidget.setRowCount(new_row + 1)
        self.dialog.tableWidget.setItem(new_row, 0, QtWidgets.QTableWidgetItem(addon.display_name))
        self.dialog.tableWidget.setItem(new_row, 1, QtWidgets.QTableWidgetItem(""))
        self.row_map[addon.name] = new_row

    def _update_addon_status(self, row: int, status: AddonStatus):
        """Update the GUI to reflect this addon's new status."""
        self.dialog.tableWidget.item(row, 1).setText(status.ui_string())

    def _process_next_update(self):
        """Grab the next addon in the list and start its updater."""
        if self.addons_with_update:
            addon = self.addons_with_update.pop(0)
            self.in_process_row = self.row_map[addon.name] if addon.name in self.row_map else None
            self._update_addon_status(self.in_process_row, AddonStatus.INSTALLING)
            self.dialog.tableWidget.scrollToItem(
                self.dialog.tableWidget.item(self.in_process_row, 0)
            )
            self.active_installer = self.updater_factory.get_updater(addon)
            self._launch_active_installer()
        else:
            self._finalize()

    def _launch_active_installer(self):
        """Set up and run the active installer in a new thread."""

        self.active_installer.success.connect(self._update_succeeded)
        self.active_installer.failure.connect(self._update_failed)
        self.active_installer.finished.connect(self._update_finished)

        self.worker_thread = QtCore.QThread()
        self.active_installer.moveToThread(self.worker_thread)
        self.worker_thread.started.connect(self.active_installer.run)
        self.worker_thread.start()

    def _update_succeeded(self, addon):
        """Callback for a successful update"""
        self._update_addon_status(self.row_map[addon.name], AddonStatus.SUCCEEDED)
        self.addon_updated.emit(addon)

    def _update_failed(self, addon):
        """Callback for a failed update"""
        self._update_addon_status(self.row_map[addon.name], AddonStatus.FAILED)

    def _update_finished(self):
        """Callback for updater that has finished all its work"""
        if self.worker_thread is not None and self.worker_thread.isRunning():
            self.worker_thread.quit()
            self.worker_thread.wait()
        self.addon_updated.emit(self.active_installer.addon_to_install)
        if not self.cancelled:
            self._process_next_update()
        else:
            self._setup_cancelled_state()

    def _finalize(self):
        """No more updates, clean up and shut down"""
        if self.worker_thread is not None and self.worker_thread.isRunning():
            self.worker_thread.quit()
            self.worker_thread.wait()
        text = translate("Addons installer", "Finished updating the following addons")
        self._set_dialog_to_final_state(text)
        self.running = False
        self.finished.emit()

    def _setup_cancelled_state(self):
        text1 = translate("AddonsInstaller", "Update was cancelled")
        text2 = translate("AddonsInstaller", "some addons may have been updated")
        self._set_dialog_to_final_state(text1 + ": " + text2)
        self.running = False
        self.finished.emit()

    def _set_dialog_to_final_state(self, new_content):
        self.dialog.buttonBox.clear()
        self.dialog.buttonBox.addButton(QtWidgets.QDialogButtonBox.Close)
        self.dialog.label.setText(new_content)

    def is_running(self):
        """True if the thread is running, and False if not"""
        return self.running
