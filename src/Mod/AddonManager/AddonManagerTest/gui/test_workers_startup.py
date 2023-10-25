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

import json
import unittest
import os
import tempfile

import FreeCAD

from PySide import QtCore

import NetworkManager
from Addon import Addon
from addonmanager_workers_startup import (
    CreateAddonListWorker,
    LoadPackagesFromCacheWorker,
    LoadMacrosFromCacheWorker,
)

run_slow_tests = False


class TestWorkersStartup(unittest.TestCase):

    MODULE = "test_workers_startup"  # file name without extension

    @unittest.skipUnless(run_slow_tests, "This integration test is slow and uses the network")
    def setUp(self):
        """Set up the test"""
        self.test_dir = os.path.join(
            FreeCAD.getHomePath(), "Mod", "AddonManager", "AddonManagerTest", "data"
        )

        self.saved_mod_directory = Addon.mod_directory
        self.saved_cache_directory = Addon.cache_directory
        Addon.mod_directory = os.path.join(tempfile.gettempdir(), "FreeCADTesting", "Mod")
        Addon.cache_directory = os.path.join(tempfile.gettempdir(), "FreeCADTesting", "Cache")

        os.makedirs(Addon.mod_directory, mode=0o777, exist_ok=True)
        os.makedirs(Addon.cache_directory, mode=0o777, exist_ok=True)

        url = "https://api.github.com/zen"
        NetworkManager.InitializeNetworkManager()
        result = NetworkManager.AM_NETWORK_MANAGER.blocking_get(url)
        if result is None:
            self.skipTest("No active internet connection detected")

        self.addon_list = []
        self.macro_counter = 0
        self.workbench_counter = 0
        self.prefpack_counter = 0
        self.addon_from_cache_counter = 0
        self.macro_from_cache_counter = 0

        self.package_cache = {}
        self.macro_cache = []

        self.package_cache_filename = os.path.join(Addon.cache_directory, "packages.json")
        self.macro_cache_filename = os.path.join(Addon.cache_directory, "macros.json")

        # Store the user's preference for whether git is enabled or disabled
        pref = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Addons")
        self.saved_git_disabled_status = pref.GetBool("disableGit", False)

    def tearDown(self):
        """Tear down the test"""
        Addon.mod_directory = self.saved_mod_directory
        Addon.cache_directory = self.saved_cache_directory
        pref = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Addons")
        pref.SetBool("disableGit", self.saved_git_disabled_status)

    def test_create_addon_list_worker(self):
        """Test whether any addons are added: runs the full query, so this potentially is a SLOW
        test."""
        worker = CreateAddonListWorker()
        worker.addon_repo.connect(self._addon_added)
        worker.start()
        while worker.isRunning():
            QtCore.QCoreApplication.processEvents(QtCore.QEventLoop.AllEvents, 50)
        QtCore.QCoreApplication.processEvents(QtCore.QEventLoop.AllEvents)

        self.assertGreater(self.macro_counter, 0, "No macros returned")
        self.assertGreater(self.workbench_counter, 0, "No workbenches returned")

        # Make sure there are no duplicates:
        addon_name_set = set()
        for addon in self.addon_list:
            addon_name_set.add(addon.name)
        self.assertEqual(
            len(addon_name_set), len(self.addon_list), "Duplicate names are not allowed"
        )

        # Write the cache data
        if hasattr(self, "package_cache"):
            with open(self.package_cache_filename, "w", encoding="utf-8") as f:
                f.write(json.dumps(self.package_cache, indent="  "))
        if hasattr(self, "macro_cache"):
            with open(self.macro_cache_filename, "w", encoding="utf-8") as f:
                f.write(json.dumps(self.macro_cache, indent="  "))

        original_macro_counter = self.macro_counter
        original_addon_list = self.addon_list.copy()
        self.macro_counter = 0
        self.workbench_counter = 0
        self.addon_list.clear()

        # Now try loading the same data from the cache we just created
        worker = LoadPackagesFromCacheWorker(self.package_cache_filename)
        worker.override_metadata_cache_path(os.path.join(Addon.cache_directory, "PackageMetadata"))
        worker.addon_repo.connect(self._addon_added)

        worker.start()
        while worker.isRunning():
            QtCore.QCoreApplication.processEvents(QtCore.QEventLoop.AllEvents, 50)
        QtCore.QCoreApplication.processEvents(QtCore.QEventLoop.AllEvents)

        worker = LoadMacrosFromCacheWorker(self.macro_cache_filename)
        worker.add_macro_signal.connect(self._addon_added)

        worker.start()
        while worker.isRunning():
            QtCore.QCoreApplication.processEvents(QtCore.QEventLoop.AllEvents, 50)
        QtCore.QCoreApplication.processEvents(QtCore.QEventLoop.AllEvents)

        # Make sure that every addon in the original list is also in the new list
        fail_counter = 0
        for original_addon in original_addon_list:
            found = False
            for addon in self.addon_list:
                if addon.name == original_addon.name:
                    found = True
                    break
            if not found:
                print(f"Failed to load {addon.name} from cache")
                fail_counter += 1
        self.assertEqual(fail_counter, 0)

        # Make sure there are no duplicates:
        addon_name_set.clear()
        for addon in self.addon_list:
            addon_name_set.add(addon.name)

        self.assertEqual(len(addon_name_set), len(self.addon_list))
        self.assertEqual(len(original_addon_list), len(self.addon_list))

        self.assertEqual(
            original_macro_counter,
            self.macro_counter,
            "Cache loaded a different number of macros",
        )
        # We can't check workbench and preference pack counting at this point, because that relies
        # on the package.xml metadata file, which this test does not download.

    def test_create_addon_list_git_disabled(self):
        """If the user has git enabled, also test the addon manager with git disabled"""
        if self.saved_git_disabled_status:
            self.skipTest("Git is disabled, this test is redundant")

        pref = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Addons")
        pref.SetBool("disableGit", True)

        self.test_create_addon_list_worker()

    def _addon_added(self, addon: Addon):
        """Callback for adding an Addon: tracks the list, and counts the various types"""
        print(f"Addon added: {addon.name}")
        self.addon_list.append(addon)
        if addon.contains_workbench():
            self.workbench_counter += 1
        if addon.contains_macro():
            self.macro_counter += 1
        if addon.contains_preference_pack():
            self.prefpack_counter += 1

        # Also record the information for cache purposes
        if addon.macro is None:
            self.package_cache[addon.name] = addon.to_cache()
        else:
            self.macro_cache.append(addon.macro.to_cache())
