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

import unittest
import os
import FreeCAD
from addonmanager_workers_utility import ConnectionChecker
from PySide import QtCore

import NetworkManager


class TestWorkersUtility(unittest.TestCase):

    MODULE = "test_workers_utility"  # file name without extension

    @unittest.skip("Test is slow and uses the network: refactor!")
    def setUp(self):
        self.test_dir = os.path.join(
            FreeCAD.getHomePath(), "Mod", "AddonManager", "AddonManagerTest", "data"
        )
        self.last_result = None

        url = "https://api.github.com/zen"
        NetworkManager.InitializeNetworkManager()
        result = NetworkManager.AM_NETWORK_MANAGER.blocking_get(url)
        if result is None:
            self.skipTest("No active internet connection detected")

    def test_connection_checker_basic(self):
        """Tests the connection checking worker's basic operation: does not exit until worker thread completes"""
        worker = ConnectionChecker()
        worker.success.connect(self.connection_succeeded)
        worker.failure.connect(self.connection_failed)
        self.last_result = None
        worker.start()
        while worker.isRunning():
            QtCore.QCoreApplication.processEvents(QtCore.QEventLoop.AllEvents, 50)
        QtCore.QCoreApplication.processEvents(QtCore.QEventLoop.AllEvents)
        self.assertEqual(self.last_result, "SUCCESS")

    def test_connection_checker_thread_interrupt(self):
        worker = ConnectionChecker()
        worker.success.connect(self.connection_succeeded)
        worker.failure.connect(self.connection_failed)
        self.last_result = None
        worker.start()
        worker.requestInterruption()
        while worker.isRunning():
            QtCore.QCoreApplication.processEvents(QtCore.QEventLoop.AllEvents, 50)
        QtCore.QCoreApplication.processEvents(QtCore.QEventLoop.AllEvents)
        self.assertIsNone(self.last_result, "Requesting interruption of thread failed to interrupt")

    def connection_succeeded(self):
        self.last_result = "SUCCESS"

    def connection_failed(self):
        self.last_result = "FAILURE"
