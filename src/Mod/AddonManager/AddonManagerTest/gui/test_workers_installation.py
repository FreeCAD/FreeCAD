# -*- coding: utf-8 -*-

# ***************************************************************************
# *   Copyright (c) 2022 FreeCAD Project Association                        *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
# *                                                                         *
# *   This library is free software; you can redistribute it and/or         *
# *   modify it under the terms of the GNU Lesser General Public            *
# *   License as published by the Free Software Foundation; either          *
# *   version 2.1 of the License, or (at your option) any later version.    *
# *                                                                         *
# *   This library is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU     *
# *   Lesser General Public License for more details.                       *
# *                                                                         *
# *   You should have received a copy of the GNU Lesser General Public      *
# *   License along with this library; if not, write to the Free Software   *
# *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA         *
# *   02110-1301  USA                                                       *
# *                                                                         *
# ***************************************************************************

import json
import os
import shutil
import stat
import tempfile
import unittest
import FreeCAD
from addonmanager_git import initialize_git

from PySide2 import QtCore

import NetworkManager
from Addon import Addon
from addonmanager_workers_startup import (
    CreateAddonListWorker,
    UpdateChecker,
)
from addonmanager_workers_installation import InstallWorkbenchWorker


class TestWorkersInstallation(unittest.TestCase):

    MODULE = "test_workers_installation"  # file name without extension

    addon_list = (
        []
    )  # Cache at the class level so only the first test has to download it

    def setUp(self):
        """Set up the test"""
        self.test_dir = os.path.join(
            FreeCAD.getHomePath(), "Mod", "AddonManager", "AddonManagerTest", "data"
        )

        self.saved_mod_directory = Addon.mod_directory
        self.saved_cache_directory = Addon.cache_directory
        Addon.mod_directory = os.path.join(
            tempfile.gettempdir(), "FreeCADTesting", "Mod"
        )
        Addon.cache_directory = os.path.join(
            tempfile.gettempdir(), "FreeCADTesting", "Cache"
        )

        os.makedirs(Addon.mod_directory, mode=0o777, exist_ok=True)
        os.makedirs(Addon.cache_directory, mode=0o777, exist_ok=True)

        url = "https://api.github.com/zen"
        NetworkManager.InitializeNetworkManager()
        result = NetworkManager.AM_NETWORK_MANAGER.blocking_get(url)
        if result is None:
            self.skipTest("No active internet connection detected")

        self.macro_counter = 0
        self.workbench_counter = 0
        self.prefpack_counter = 0
        self.addon_from_cache_counter = 0
        self.macro_from_cache_counter = 0

        self.package_cache = {}
        self.macro_cache = []

        self.package_cache_filename = os.path.join(
            Addon.cache_directory, "packages.json"
        )
        self.macro_cache_filename = os.path.join(Addon.cache_directory, "macros.json")

        if not TestWorkersInstallation.addon_list:
            self._create_addon_list()

        # Workbench: use the FreeCAD-Help workbench for testing purposes
        self.help_addon = None
        for addon in self.addon_list:
            if addon.name == "Help":
                self.help_addon = addon
                break
        if not self.help_addon:
            print("Unable to locate the FreeCAD-Help addon to test with")
            self.skipTest("No active internet connection detected")

        # Store the user's preference for whether git is enabled or disabled
        pref = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Addons")
        self.saved_git_disabled_status = pref.GetBool("disableGit", False)

    def tearDown(self):
        mod_dir = os.path.join(tempfile.gettempdir(), "FreeCADTesting", "Mod")
        if os.path.exists(mod_dir):
            self._rmdir(mod_dir)
        macro_dir = os.path.join(tempfile.gettempdir(), "FreeCADTesting", "Mod")
        if os.path.exists(macro_dir):
            self._rmdir(macro_dir)
        pref = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Addons")
        pref.SetBool("disableGit", self.saved_git_disabled_status)

    def test_workbench_installation(self):
        addon_location = os.path.join(
            tempfile.gettempdir(), "FreeCADTesting", "Mod", self.help_addon.name
        )
        worker = InstallWorkbenchWorker(self.help_addon, addon_location)
        worker.run()  # Synchronous call, blocks until complete
        self.assertTrue(os.path.exists(addon_location))
        self.assertTrue(os.path.exists(os.path.join(addon_location, "package.xml")))

    def test_workbench_installation_git_disabled(self):
        """If the testing user has git enabled, also test the addon manager with git disabled"""
        if self.saved_git_disabled_status:
            self.skipTest("Git is disabled, this test is redundant")

        pref = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Addons")
        pref.SetBool("disableGit", True)

        self.test_workbench_installation()

        pref.SetBool("disableGit", False)

    def test_workbench_update_checker(self):

        git_manager = initialize_git()

        if not git_manager:
            return

        # Workbench: use the FreeCAD-Help workbench for testing purposes
        help_addon = None
        for addon in self.addon_list:
            if addon.name == "Help":
                help_addon = addon
                break
        if not help_addon:
            print("Unable to locate the FreeCAD-Help addon to test with")
            return

        addon_location = os.path.join(
            tempfile.gettempdir(), "FreeCADTesting", "Mod", self.help_addon.name
        )
        worker = InstallWorkbenchWorker(addon, addon_location)
        worker.run()  # Synchronous call, blocks until complete
        self.assertEqual(help_addon.status(), Addon.Status.PENDING_RESTART)

        # Back up one revision
        git_manager.reset(addon_location, ["--hard", "HEAD~1"])

        # At this point the addon should be "out of date", checked out to one revision behind
        # the most recent.

        worker = UpdateChecker()
        worker.override_mod_directory(
            os.path.join(tempfile.gettempdir(), "FreeCADTesting", "Mod")
        )
        worker.check_workbench(help_addon)  # Synchronous call
        self.assertEqual(help_addon.status(), Addon.Status.UPDATE_AVAILABLE)

        # Now try to "update" it (which is really done via the install worker)
        worker = InstallWorkbenchWorker(addon, addon_location)
        worker.run()  # Synchronous call, blocks until complete
        self.assertEqual(help_addon.status(), Addon.Status.PENDING_RESTART)

    def _rmdir(self, path):
        try:
            shutil.rmtree(path, onerror=self._remove_readonly)
        except Exception as e:
            print(e)

    def _remove_readonly(self, func, path, _) -> None:
        """Remove a read-only file."""

        os.chmod(path, stat.S_IWRITE)
        func(path)

    def _create_addon_list(self):
        """Create the list of addons"""
        worker = CreateAddonListWorker()
        worker.addon_repo.connect(self._addon_added)
        worker.start()
        while worker.isRunning():
            QtCore.QCoreApplication.processEvents(QtCore.QEventLoop.AllEvents, 50)
        QtCore.QCoreApplication.processEvents(QtCore.QEventLoop.AllEvents)

    def _addon_added(self, addon: Addon):
        """Callback for adding an Addon: tracks the list, and counts the various types"""
        print(f"Addon added: {addon.name}")
        TestWorkersInstallation.addon_list.append(addon)
        if addon.contains_workbench():
            self.workbench_counter += 1
        if addon.contains_macro():
            self.macro_counter += 1
        if addon.contains_preference_pack():
            self.prefpack_counter += 1
