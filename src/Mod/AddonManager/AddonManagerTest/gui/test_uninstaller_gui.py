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

import functools
import unittest

from PySide import QtCore, QtWidgets

import FreeCAD

from AddonManagerTest.gui.gui_mocks import (
    DialogWatcher,
    FakeWorker,
    MockThread,
)
from AddonManagerTest.app.mocks import MockAddon

from addonmanager_uninstaller_gui import AddonUninstallerGUI

translate = FreeCAD.Qt.translate


class TestUninstallerGUI(unittest.TestCase):

    MODULE = "test_uninstaller_gui"  # file name without extension

    def setUp(self):
        self.addon_to_remove = MockAddon()
        self.uninstaller_gui = AddonUninstallerGUI(self.addon_to_remove)
        self.finalized_thread = False
        self.signals_caught = []

    def tearDown(self):
        pass

    def catch_signal(self, signal_name, *_):
        self.signals_caught.append(signal_name)

    def test_confirmation_dialog_yes(self):
        dialog_watcher = DialogWatcher(
            translate("AddonsInstaller", "Confirm remove"),
            QtWidgets.QDialogButtonBox.Yes,
        )
        answer = self.uninstaller_gui._confirm_uninstallation()
        self.assertTrue(dialog_watcher.dialog_found, "Failed to find the expected dialog box")
        self.assertTrue(dialog_watcher.button_found, "Failed to find the expected button")
        self.assertTrue(answer, "Expected a 'Yes' click to return True, but got False")

    def test_confirmation_dialog_cancel(self):
        dialog_watcher = DialogWatcher(
            translate("AddonsInstaller", "Confirm remove"),
            QtWidgets.QDialogButtonBox.Cancel,
        )
        answer = self.uninstaller_gui._confirm_uninstallation()
        self.assertTrue(dialog_watcher.dialog_found, "Failed to find the expected dialog box")
        self.assertTrue(dialog_watcher.button_found, "Failed to find the expected button")
        self.assertFalse(answer, "Expected a 'Cancel' click to return False, but got True")

    def test_progress_dialog(self):
        dialog_watcher = DialogWatcher(
            translate("AddonsInstaller", "Removing Addon"),
            QtWidgets.QDialogButtonBox.Cancel,
        )
        self.uninstaller_gui._show_progress_dialog()
        # That call isn't modal, so spin our own event loop:
        while self.uninstaller_gui.progress_dialog.isVisible():
            QtCore.QCoreApplication.processEvents(QtCore.QEventLoop.AllEvents, 100)
        self.assertTrue(dialog_watcher.dialog_found, "Failed to find the expected dialog box")
        self.assertTrue(dialog_watcher.button_found, "Failed to find the expected button")

    def test_timer_launches_progress_dialog(self):
        worker = FakeWorker()
        dialog_watcher = DialogWatcher(
            translate("AddonsInstaller", "Removing Addon"),
            QtWidgets.QDialogButtonBox.Cancel,
        )
        QtCore.QTimer.singleShot(1000, worker.stop)  # If the test fails, this kills the "worker"
        self.uninstaller_gui._confirm_uninstallation = lambda: True
        self.uninstaller_gui._run_uninstaller = worker.work
        self.uninstaller_gui._finalize = lambda: None
        self.uninstaller_gui.dialog_timer.setInterval(1)  # To speed up the test, only wait 1ms
        self.uninstaller_gui.run()  # Blocks once it hits the fake worker
        self.assertTrue(dialog_watcher.dialog_found, "Failed to find the expected dialog box")
        self.assertTrue(dialog_watcher.button_found, "Failed to find the expected button")
        worker.stop()

    def test_success_dialog(self):
        dialog_watcher = DialogWatcher(
            translate("AddonsInstaller", "Uninstall complete"),
            QtWidgets.QDialogButtonBox.Ok,
        )
        self.uninstaller_gui._succeeded(self.addon_to_remove)
        self.assertTrue(dialog_watcher.dialog_found, "Failed to find the expected dialog box")
        self.assertTrue(dialog_watcher.button_found, "Failed to find the expected button")

    def test_failure_dialog(self):
        dialog_watcher = DialogWatcher(
            translate("AddonsInstaller", "Uninstall failed"),
            QtWidgets.QDialogButtonBox.Ok,
        )
        self.uninstaller_gui._failed(
            self.addon_to_remove, "Some failure message\nAnother failure message"
        )
        self.assertTrue(dialog_watcher.dialog_found, "Failed to find the expected dialog box")
        self.assertTrue(dialog_watcher.button_found, "Failed to find the expected button")

    def test_finalize(self):
        self.uninstaller_gui.finished.connect(functools.partial(self.catch_signal, "finished"))
        self.uninstaller_gui.worker_thread = MockThread()
        self.uninstaller_gui._finalize()
        self.assertIn("finished", self.signals_caught)
