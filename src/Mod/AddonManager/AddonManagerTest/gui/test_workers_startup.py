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

import unittest
import os
import tempfile
import FreeCAD

from PySide2 import QtCore

import NetworkManager
from Addon import Addon
from addonmanager_workers_startup import (
    CreateAddonListWorker
    )

class TestWorkersStartup(unittest.TestCase):

    MODULE = "test_workers_startup"  # file name without extension

    def setUp(self):
        self.test_dir = os.path.join(FreeCAD.getHomePath(), "Mod", "AddonManager", "AddonManagerTest", "data")

        url = "https://api.github.com/zen"
        NetworkManager.InitializeNetworkManager()
        result = NetworkManager.AM_NETWORK_MANAGER.blocking_get(url)
        if result is None:
            self.skipTest("No active internet connection detected")

        self.addon_list = []
        self.macro_counter = 0
        self.workbench_counter = 0
        self.prefpack_counter = 0
        
        # Populated when the addon list is created in the first test
        self.package_cache = {}
        self.macro_cache = {}

        self.package_cache_file = tempfile.NamedTemporaryFile(mode='w', encoding="utf-8", delete=False)
        self.macro_cache_file = tempfile.NamedTemporaryFile(mode='w', encoding="utf-8", delete=False)

    def test_create_addon_list_worker(self):
        """ Test whether any addons are added: runs the full query, so this potentially is a SLOW 
        test. Note that this test must be run before any of the other tests, so that the cache gets
        created. """
        worker = CreateAddonListWorker()
        worker.addon_repo.connect(self._addon_added)
        worker.start()
        while worker.isRunning():
            QtCore.QCoreApplication.processEvents(QtCore.QEventLoop.AllEvents, 50)

        self.assertGreater(self.macro_counter,0, "No macros returned")
        self.assertGreater(self.workbench_counter,0, "No workbenches returned")
        self.assertGreater(self.prefpack_counter,0, "No preference packs returned")

        # Write the cache data
        if hasattr(self, "package_cache"):
            self.package_cache_file.write(json.dumps(self.package_cache, indent="  "))
            self.package_cache_file.close()
        if hasattr(self, "macro_cache"):
            self.macro_cache_file.write(json.dumps(self.macro_cache, indent="  "))
            self.macro_cache_file.close()

    def _addon_added(self, addon:Addon):
        """ Callback for adding an Addon: tracks the list, and counts the various types """
        print (f"Addon Test: {addon.name}")
        self.addon_list.append(addon)
        if addon.contains_workbench():
            self.workbench_counter += 1
        if addon.contains_macro():
            self.macro_counter += 1
        if addon.contains_preference_pack():
            self.prefpack_counter += 1

        # Also record the information for cache purposes
        self.package_cache[addon.name] = addon.to_cache()
        
        if addon.macro is not None:
            self.macro_cache.append(addon.macro.to_cache())

    def test_load_packages_from_cache_worker(self):
        pass

    def test_load_macros_from_cache_worker(self):
        pass
