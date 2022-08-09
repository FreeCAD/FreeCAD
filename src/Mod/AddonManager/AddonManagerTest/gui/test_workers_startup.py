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
import unittest
import os
import tempfile
import FreeCAD

from PySide2 import QtCore

import NetworkManager
from Addon import Addon
from addonmanager_workers_startup import (
    CreateAddonListWorker,
    LoadPackagesFromCacheWorker,
    LoadMacrosFromCacheWorker,
    )

class TestWorkersStartup(unittest.TestCase):

    MODULE = "test_workers_startup"  # file name without extension

    def setUp(self):
        """ Set up the test """
        self.test_dir = os.path.join(FreeCAD.getHomePath(), "Mod", "AddonManager", "AddonManagerTest", "data")
        
        self.saved_mod_directory = Addon.mod_directory
        self.saved_cache_directory = Addon.cache_directory
        Addon.mod_directory = os.path.join(tempfile.gettempdir(),"FreeCADTesting","Mod")
        Addon.cache_directory = os.path.join(tempfile.gettempdir(),"FreeCADTesting","Cache")

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
        
        # Populated when the addon list is created in the first test
        self.package_cache = {}
        self.macro_cache = []

        self.package_cache_filename = os.path.join(Addon.cache_directory,"packages.json")
        self.macro_cache_filename = os.path.join(Addon.cache_directory,"macros.json")

    def tearDown(self):
        """ Tear down the test """
        Addon.mod_directory = self.saved_mod_directory
        Addon.cache_directory = self.saved_cache_directory

    def test_create_addon_list_worker(self):
        """ Test whether any addons are added: runs the full query, so this potentially is a SLOW 
        test. Note that this test must be run before any of the other tests, so that the cache gets
        created. """
        worker = CreateAddonListWorker()
        worker.addon_repo.connect(self._addon_added)
        worker.start()
        while worker.isRunning():
            QtCore.QCoreApplication.processEvents(QtCore.QEventLoop.AllEvents, 50)
        QtCore.QCoreApplication.processEvents(QtCore.QEventLoop.AllEvents)

        self.assertGreater(self.macro_counter,0, "No macros returned")
        self.assertGreater(self.workbench_counter,0, "No workbenches returned")
        self.assertGreater(self.prefpack_counter,0, "No preference packs returned")

        # Write the cache data
        if hasattr(self, "package_cache"):
            with open(self.package_cache_filename,"w",encoding="utf-8") as f:
                f.write(json.dumps(self.package_cache, indent="  "))
        if hasattr(self, "macro_cache"):
            with open(self.macro_cache_filename,"w",encoding="utf-8") as f:
                f.write(json.dumps(self.macro_cache, indent="  "))

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
        """ Test loading packages from the cache """
        worker = LoadPackagesFromCacheWorker(self.package_cache_filename)
        worker.override_metadata_cache_path(os.path.join(Addon.cache_directory,"PackageMetadata"))
        worker.addon_repo.connect(self._addon_added_from_cache)
        self.addon_from_cache_counter = 0
        
        worker.start()
        while worker.isRunning():
            QtCore.QCoreApplication.processEvents(QtCore.QEventLoop.AllEvents, 50)
        QtCore.QCoreApplication.processEvents(QtCore.QEventLoop.AllEvents)
        
        self.assertGreater(self.addon_from_cache_counter,0, "No addons in the cache")

    def _addon_added_from_cache(self, addon:Addon):
        """ Callback when addon added from cache """
        print (f"Addon Cache Test: {addon.name}")
        self.addon_from_cache_counter += 1

    def test_load_macros_from_cache_worker(self):
        """ Test loading macros from the cache """
        worker = LoadMacrosFromCacheWorker(self.macro_cache_filename)
        worker.add_macro_signal.connect(self._macro_added_from_cache)
        self.macro_from_cache_counter = 0
        
        worker.start()
        while worker.isRunning():
            QtCore.QCoreApplication.processEvents(QtCore.QEventLoop.AllEvents, 50)
        QtCore.QCoreApplication.processEvents(QtCore.QEventLoop.AllEvents)
        
        self.assertGreater(self.macro_from_cache_counter,0, "No macros in the cache")

    def _macro_added_from_cache(self, addon:Addon):
        """ Callback for adding macros from the cache """
        print (f"Macro Cache Test: {addon.name}")
        self.macro_from_cache_counter += 1