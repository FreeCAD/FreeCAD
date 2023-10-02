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

"""GUI functions for uninstalling an Addon or Macro."""

import FreeCAD
import FreeCADGui

from PySide import QtCore, QtWidgets

from addonmanager_uninstaller import AddonUninstaller, MacroUninstaller
import addonmanager_utilities as utils

translate = FreeCAD.Qt.translate


class AddonUninstallerGUI(QtCore.QObject):
    """User interface for uninstalling an Addon: asks for confirmation, displays a progress dialog,
    displays completion and/or error dialogs, and emits the finished() signal when all work is
    complete."""

    finished = QtCore.Signal()

    def __init__(self, addon_to_remove):
        super().__init__()
        self.addon_to_remove = addon_to_remove
        if hasattr(self.addon_to_remove, "macro") and self.addon_to_remove.macro is not None:
            self.uninstaller = MacroUninstaller(self.addon_to_remove)
        else:
            self.uninstaller = AddonUninstaller(self.addon_to_remove)
        self.uninstaller.success.connect(self._succeeded)
        self.uninstaller.failure.connect(self._failed)
        self.worker_thread = QtCore.QThread()
        self.uninstaller.moveToThread(self.worker_thread)
        self.uninstaller.finished.connect(self.worker_thread.quit)
        self.worker_thread.started.connect(self.uninstaller.run)
        self.progress_dialog = None
        self.dialog_timer = QtCore.QTimer()
        self.dialog_timer.timeout.connect(self._show_progress_dialog)
        self.dialog_timer.setSingleShot(True)
        self.dialog_timer.setInterval(1000)  # Can override from external (e.g. testing) code

    def run(self):
        """Begin the user interaction: asynchronous, only blocks while showing the initial modal
        confirmation dialog."""
        ok_to_proceed = self._confirm_uninstallation()
        if not ok_to_proceed:
            self._finalize()
            return

        self.dialog_timer.start()
        self._run_uninstaller()

    def _confirm_uninstallation(self) -> bool:
        """Present a modal dialog asking the user if they really want to uninstall. Returns True to
        continue with the uninstallation, or False to stop the process."""
        confirm = QtWidgets.QMessageBox.question(
            utils.get_main_am_window(),
            translate("AddonsInstaller", "Confirm remove"),
            translate("AddonsInstaller", "Are you sure you want to uninstall {}?").format(
                self.addon_to_remove.display_name
            ),
            QtWidgets.QMessageBox.Yes | QtWidgets.QMessageBox.Cancel,
        )
        return confirm == QtWidgets.QMessageBox.Yes

    def _show_progress_dialog(self):
        self.progress_dialog = QtWidgets.QMessageBox(
            QtWidgets.QMessageBox.NoIcon,
            translate("AddonsInstaller", "Removing Addon"),
            translate("AddonsInstaller", "Removing {}").format(self.addon_to_remove.display_name)
            + "...",
            QtWidgets.QMessageBox.Cancel,
            parent=utils.get_main_am_window(),
        )
        self.progress_dialog.rejected.connect(self._cancel_removal)
        self.progress_dialog.show()

    def _run_uninstaller(self):
        self.worker_thread.start()

    def _cancel_removal(self):
        """Ask the QThread to interrupt. Probably has no effect, most of the work is in a single OS
        call."""
        self.worker_thread.requestInterruption()

    def _succeeded(self, addon):
        """Callback for successful removal"""
        self.dialog_timer.stop()
        if self.progress_dialog:
            self.progress_dialog.hide()
        QtWidgets.QMessageBox.information(
            utils.get_main_am_window(),
            translate("AddonsInstaller", "Uninstall complete"),
            translate("AddonInstaller", "Finished removing {}").format(addon.display_name),
        )
        self._finalize()

    def _failed(self, addon, message):
        """Callback for failed or partially failed removal"""
        self.dialog_timer.stop()
        if self.progress_dialog:
            self.progress_dialog.hide()
        QtWidgets.QMessageBox.critical(
            utils.get_main_am_window(),
            translate("AddonsInstaller", "Uninstall failed"),
            translate("AddonInstaller", "Failed to remove some files") + ":\n" + message,
        )
        self._finalize()

    def _finalize(self):
        """Clean up and emit finished signal"""
        if self.worker_thread.isRunning():
            self.worker_thread.requestInterruption()
            self.worker_thread.quit()
            self.worker_thread.wait(500)
        self.finished.emit()
