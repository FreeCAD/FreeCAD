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

from time import sleep
import unittest
import FreeCAD

from Addon import Addon

from PySide import QtCore, QtWidgets

from addonmanager_update_all_gui import UpdateAllGUI, AddonStatus


class MockUpdater(QtCore.QObject):
    success = QtCore.Signal(object)
    failure = QtCore.Signal(object)
    finished = QtCore.Signal()

    def __init__(self, addon, addons=[]):
        super().__init__()
        self.addon_to_install = addon
        self.addons = addons
        self.has_run = False
        self.emit_success = True
        self.work_function = None  # Set to some kind of callable to make this function take time

    def run(self):
        self.has_run = True
        if self.work_function is not None and callable(self.work_function):
            self.work_function()
        if self.emit_success:
            self.success.emit(self.addon_to_install)
        else:
            self.failure.emit(self.addon_to_install)
        self.finished.emit()


class MockUpdaterFactory:
    def __init__(self, addons):
        self.addons = addons
        self.work_function = None
        self.updater = None

    def get_updater(self, addon):
        self.updater = MockUpdater(addon, self.addons)
        self.updater.work_function = self.work_function
        return self.updater


class MockAddon:
    def __init__(self, name):
        self.display_name = name
        self.name = name
        self.macro = None

    def status(self):
        return Addon.Status.UPDATE_AVAILABLE


class CallInterceptor:
    def __init__(self):
        self.called = False
        self.args = None

    def intercept(self, *args):
        self.called = True
        self.args = args


class TestUpdateAllGui(unittest.TestCase):
    def setUp(self):
        self.addons = []
        for i in range(3):
            self.addons.append(MockAddon(f"Mock Addon {i}"))
        self.factory = MockUpdaterFactory(self.addons)
        self.test_object = UpdateAllGUI(self.addons)
        self.test_object.updater_factory = self.factory

    def tearDown(self):
        pass

    def test_run(self):
        self.factory.work_function = lambda: sleep(0.1)
        self.test_object.run()
        while self.test_object.is_running():
            QtCore.QCoreApplication.processEvents(QtCore.QEventLoop.AllEvents, 100)
        self.test_object.dialog.accept()

    def test_setup_dialog(self):
        self.test_object._setup_dialog()
        self.assertIsNotNone(
            self.test_object.dialog.buttonBox.button(QtWidgets.QDialogButtonBox.Cancel)
        )
        self.assertEqual(self.test_object.dialog.tableWidget.rowCount(), 3)

    def test_cancelling_installation(self):
        class Worker:
            def __init__(self):
                self.counter = 0
                self.LIMIT = 100
                self.limit_reached = False

            def run(self):
                while self.counter < self.LIMIT:
                    if QtCore.QThread.currentThread().isInterruptionRequested():
                        return
                    self.counter += 1
                    sleep(0.01)
                self.limit_reached = True

        worker = Worker()
        self.factory.work_function = worker.run
        self.test_object.run()
        cancel_timer = QtCore.QTimer()
        cancel_timer.timeout.connect(
            self.test_object.dialog.buttonBox.button(QtWidgets.QDialogButtonBox.Cancel).click
        )
        cancel_timer.start(90)
        while self.test_object.is_running():
            QtCore.QCoreApplication.processEvents(QtCore.QEventLoop.AllEvents, 10)
        self.assertGreater(len(self.test_object.addons_with_update), 0)

    def test_add_addon_to_table(self):
        mock_addon = MockAddon("MockAddon")
        self.test_object.dialog.tableWidget.clear()
        self.test_object._add_addon_to_table(mock_addon)
        self.assertEqual(self.test_object.dialog.tableWidget.rowCount(), 1)

    def test_update_addon_status(self):
        self.test_object._setup_dialog()
        self.test_object._update_addon_status(0, AddonStatus.WAITING)
        self.assertEqual(
            self.test_object.dialog.tableWidget.item(0, 1).text(),
            AddonStatus.WAITING.ui_string(),
        )
        self.test_object._update_addon_status(0, AddonStatus.INSTALLING)
        self.assertEqual(
            self.test_object.dialog.tableWidget.item(0, 1).text(),
            AddonStatus.INSTALLING.ui_string(),
        )
        self.test_object._update_addon_status(0, AddonStatus.SUCCEEDED)
        self.assertEqual(
            self.test_object.dialog.tableWidget.item(0, 1).text(),
            AddonStatus.SUCCEEDED.ui_string(),
        )
        self.test_object._update_addon_status(0, AddonStatus.FAILED)
        self.assertEqual(
            self.test_object.dialog.tableWidget.item(0, 1).text(),
            AddonStatus.FAILED.ui_string(),
        )

    def test_process_next_update(self):
        self.test_object._setup_dialog()
        self.test_object._launch_active_installer = lambda: None
        self.test_object._process_next_update()
        self.assertEqual(
            self.test_object.dialog.tableWidget.item(0, 1).text(),
            AddonStatus.INSTALLING.ui_string(),
        )

        self.test_object._process_next_update()
        self.assertEqual(
            self.test_object.dialog.tableWidget.item(1, 1).text(),
            AddonStatus.INSTALLING.ui_string(),
        )

        self.test_object._process_next_update()
        self.assertEqual(
            self.test_object.dialog.tableWidget.item(2, 1).text(),
            AddonStatus.INSTALLING.ui_string(),
        )

        self.test_object._process_next_update()

    def test_launch_active_installer(self):
        self.test_object.active_installer = self.factory.get_updater(self.addons[0])
        self.test_object._update_succeeded = lambda _: None
        self.test_object._update_failed = lambda _: None
        self.test_object.process_next_update = lambda: None
        self.test_object._launch_active_installer()
        # The above call does not block, so spin until it has completed (basically instantly in testing)
        while self.test_object.worker_thread.isRunning():
            QtCore.QCoreApplication.processEvents(QtCore.QEventLoop.AllEvents, 100)
        self.test_object.dialog.accept()

    def test_update_succeeded(self):
        self.test_object._setup_dialog()
        self.test_object._update_succeeded(self.addons[0])
        self.assertEqual(
            self.test_object.dialog.tableWidget.item(0, 1).text(),
            AddonStatus.SUCCEEDED.ui_string(),
        )

    def test_update_failed(self):
        self.test_object._setup_dialog()
        self.test_object._update_failed(self.addons[0])
        self.assertEqual(
            self.test_object.dialog.tableWidget.item(0, 1).text(),
            AddonStatus.FAILED.ui_string(),
        )

    def test_update_finished(self):
        self.test_object._setup_dialog()
        call_interceptor = CallInterceptor()
        self.test_object.worker_thread = QtCore.QThread()
        self.test_object.worker_thread.start()
        self.test_object._process_next_update = call_interceptor.intercept
        self.test_object.active_installer = self.factory.get_updater(self.addons[0])
        self.test_object._update_finished()
        self.assertFalse(self.test_object.worker_thread.isRunning())
        self.test_object.worker_thread.quit()
        self.assertTrue(call_interceptor.called)
        self.test_object.worker_thread.wait()

    def test_finalize(self):
        self.test_object._setup_dialog()
        self.test_object.worker_thread = QtCore.QThread()
        self.test_object.worker_thread.start()
        self.test_object._finalize()
        self.assertFalse(self.test_object.worker_thread.isRunning())
        self.test_object.worker_thread.quit()
        self.test_object.worker_thread.wait()
        self.assertFalse(self.test_object.running)
        self.assertIsNotNone(
            self.test_object.dialog.buttonBox.button(QtWidgets.QDialogButtonBox.Close)
        )
        self.assertIsNone(
            self.test_object.dialog.buttonBox.button(QtWidgets.QDialogButtonBox.Cancel)
        )

    def test_is_running(self):
        self.assertFalse(self.test_object.is_running())
        self.test_object.run()
        self.assertTrue(self.test_object.is_running())
        while self.test_object.is_running():
            QtCore.QCoreApplication.processEvents(QtCore.QEventLoop.AllEvents, 100)
        self.test_object.dialog.accept()
