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
import FreeCAD

from Addon import Addon, INTERNAL_WORKBENCHES
from addonmanager_macro import Macro

class TestAddon(unittest.TestCase):

    MODULE = "test_addon"  # file name without extension

    def setUp(self):
        self.test_dir = os.path.join(FreeCAD.getHomePath(), "Mod", "AddonManager", "AddonManagerTest", "data")

    def test_display_name(self):

        # Case 1: No display name set elsewhere: name == display_name
        addon = Addon("FreeCAD","https://github.com/FreeCAD/FreeCAD", Addon.Status.NOT_INSTALLED, "master")
        self.assertEqual(addon.name, "FreeCAD")
        self.assertEqual(addon.display_name, "FreeCAD")

        # Case 2: Package.xml metadata file sets a display name:
        addon.load_metadata_file(os.path.join(self.test_dir, "good_package.xml"))
        self.assertEqual(addon.name, "FreeCAD")
        self.assertEqual(addon.display_name, "Test Workbench")

    def test_metadata_loading(self):
        addon = Addon("FreeCAD","https://github.com/FreeCAD/FreeCAD", Addon.Status.NOT_INSTALLED, "master")
        addon.load_metadata_file(os.path.join(self.test_dir, "good_package.xml"))

        # Generic tests:
        self.assertIsNotNone(addon.metadata)
        self.assertEqual(addon.metadata.Version, "1.0.1")
        self.assertEqual(addon.metadata.Description, "A package.xml file for unit testing.")

        maintainer_list = addon.metadata.Maintainer
        self.assertEqual(len(maintainer_list),1,"Wrong number of maintainers found")
        self.assertEqual(maintainer_list[0]["name"],"FreeCAD Developer")
        self.assertEqual(maintainer_list[0]["email"],"developer@freecad.org")

        license_list = addon.metadata.License
        self.assertEqual(len(license_list),1,"Wrong number of licenses found")
        self.assertEqual(license_list[0]["name"],"LGPLv2.1")
        self.assertEqual(license_list[0]["file"],"LICENSE")

        url_list = addon.metadata.Urls
        self.assertEqual(len(url_list),2,"Wrong number of urls found")
        self.assertEqual(url_list[0]["type"],"repository")
        self.assertEqual(url_list[0]["location"],"https://github.com/chennes/FreeCAD-Package")
        self.assertEqual(url_list[0]["branch"],"main")
        self.assertEqual(url_list[1]["type"],"readme")
        self.assertEqual(url_list[1]["location"],"https://github.com/chennes/FreeCAD-Package/blob/main/README.md")

        contents = addon.metadata.Content
        self.assertEqual(len(contents),1,"Wrong number of content catetories found")
        self.assertEqual(len(contents["workbench"]),1,"Wrong number of workbenches found")

    def test_git_url_cleanup(self):
        base_url = "https://github.com/FreeCAD/FreeCAD"
        test_urls = [f"  {base_url}  ",
                     f"{base_url}.git",
                     f"  {base_url}.git  "]
        for url in test_urls:
            addon = Addon("FreeCAD", url, Addon.Status.NOT_INSTALLED, "master")
            self.assertEqual(addon.url, base_url)

    def test_tag_extraction(self):
        addon = Addon("FreeCAD","https://github.com/FreeCAD/FreeCAD", Addon.Status.NOT_INSTALLED, "master")
        addon.load_metadata_file(os.path.join(self.test_dir, "good_package.xml"))

        tags = addon.tags
        self.assertEqual(len(tags),5)
        expected_tags = set()
        expected_tags.add("Tag0")
        expected_tags.add("Tag1")
        expected_tags.add("TagA")
        expected_tags.add("TagB")
        expected_tags.add("TagC")
        self.assertEqual(tags, expected_tags)

    def test_contains_functions(self):

        # Test package.xml combinations:

        # Workbenches
        addon_with_workbench = Addon("FreeCAD","https://github.com/FreeCAD/FreeCAD", Addon.Status.NOT_INSTALLED, "master")
        addon_with_workbench.load_metadata_file(os.path.join(self.test_dir, "workbench_only.xml"))
        self.assertTrue(addon_with_workbench.contains_workbench())
        self.assertFalse(addon_with_workbench.contains_macro())
        self.assertFalse(addon_with_workbench.contains_preference_pack())

        # Macros
        addon_with_macro = Addon("FreeCAD","https://github.com/FreeCAD/FreeCAD", Addon.Status.NOT_INSTALLED, "master")
        addon_with_macro.load_metadata_file(os.path.join(self.test_dir, "macro_only.xml"))
        self.assertFalse(addon_with_macro.contains_workbench())
        self.assertTrue(addon_with_macro.contains_macro())
        self.assertFalse(addon_with_macro.contains_preference_pack())

        # Preference Packs
        addon_with_prefpack = Addon("FreeCAD","https://github.com/FreeCAD/FreeCAD", Addon.Status.NOT_INSTALLED, "master")
        addon_with_prefpack.load_metadata_file(os.path.join(self.test_dir, "prefpack_only.xml"))
        self.assertFalse(addon_with_prefpack.contains_workbench())
        self.assertFalse(addon_with_prefpack.contains_macro())
        self.assertTrue(addon_with_prefpack.contains_preference_pack())

        # Combination
        addon_with_all = Addon("FreeCAD","https://github.com/FreeCAD/FreeCAD", Addon.Status.NOT_INSTALLED, "master")
        addon_with_all.load_metadata_file(os.path.join(self.test_dir, "combination.xml"))
        self.assertTrue(addon_with_all.contains_workbench())
        self.assertTrue(addon_with_all.contains_macro())
        self.assertTrue(addon_with_all.contains_preference_pack())

        # Now do the simple, explicitly-set cases
        addon_wb = Addon("FreeCAD","https://github.com/FreeCAD/FreeCAD", Addon.Status.NOT_INSTALLED, "master")
        addon_wb.repo_type = Addon.Kind.WORKBENCH
        self.assertTrue(addon_wb.contains_workbench())
        self.assertFalse(addon_wb.contains_macro())
        self.assertFalse(addon_wb.contains_preference_pack())

        addon_m = Addon("FreeCAD","https://github.com/FreeCAD/FreeCAD", Addon.Status.NOT_INSTALLED, "master")
        addon_m.repo_type = Addon.Kind.MACRO
        self.assertFalse(addon_m.contains_workbench())
        self.assertTrue(addon_m.contains_macro())
        self.assertFalse(addon_m.contains_preference_pack())

        # There is no equivalent for preference packs, they are always accompanied by a 
        # metadata file

    def test_create_from_macro(self):
        macro_file = os.path.join(self.test_dir, "DoNothing.FCMacro")
        macro = Macro("DoNothing")
        macro.fill_details_from_file(macro_file)
        addon = Addon.from_macro(macro)

        self.assertEqual(addon.repo_type, Addon.Kind.MACRO)
        self.assertEqual(addon.name,"DoNothing")
        self.assertEqual(addon.macro.comment,"Do absolutely nothing. For Addon Manager unit tests.")
        self.assertEqual(addon.url,"https://github.com/FreeCAD/FreeCAD")
        self.assertEqual(addon.macro.version,"1.0")
        self.assertEqual(len(addon.macro.other_files),3)
        self.assertEqual(addon.macro.author,"Chris Hennes")
        self.assertEqual(addon.macro.date,"2022-02-28")
        self.assertEqual(addon.macro.icon,"not_real.png")
        self.assertEqual(addon.macro.xpm,"")

    def test_cache(self):
        addon = Addon("FreeCAD","https://github.com/FreeCAD/FreeCAD", Addon.Status.NOT_INSTALLED, "master")
        cache_data = addon.to_cache()
        second_addon = Addon.from_cache(cache_data)

        self.assertTrue(addon.__dict__, second_addon.__dict__)

    def test_dependency_resolution(self):
        
        addonA = Addon("AddonA","https://github.com/FreeCAD/FakeAddonA", Addon.Status.NOT_INSTALLED, "master")
        addonB = Addon("AddonB","https://github.com/FreeCAD/FakeAddonB", Addon.Status.NOT_INSTALLED, "master")
        addonC = Addon("AddonC","https://github.com/FreeCAD/FakeAddonC", Addon.Status.NOT_INSTALLED, "master")
        addonD = Addon("AddonD","https://github.com/FreeCAD/FakeAddonD", Addon.Status.NOT_INSTALLED, "master")

        addonA.requires.add("AddonB")
        addonB.requires.add("AddonC")
        addonB.requires.add("AddonD")
        addonD.requires.add("Path")

        all_addons = {
            addonA.name: addonA,
            addonB.name: addonB,
            addonC.name: addonC,
            addonD.name: addonD,
            }

        deps = Addon.Dependencies()
        addonA.walk_dependency_tree(all_addons, deps)

        self.assertEqual(len(deps.required_external_addons),3)
        addon_strings = [addon.name for addon in deps.required_external_addons]
        self.assertTrue("AddonB" in addon_strings, "AddonB not in required dependencies, and it should be.")
        self.assertTrue("AddonC" in addon_strings, "AddonC not in required dependencies, and it should be.")
        self.assertTrue("AddonD" in addon_strings, "AddonD not in required dependencies, and it should be.")
        self.assertTrue("Path" in deps.internal_workbenches, "Path not in workbench dependencies, and it should be.")

    def test_internal_workbench_list(self):
        addon = Addon("FreeCAD","https://github.com/FreeCAD/FreeCAD", Addon.Status.NOT_INSTALLED, "master")
        addon.load_metadata_file(os.path.join(self.test_dir, "depends_on_all_workbenches.xml"))
        deps = Addon.Dependencies()
        addon.walk_dependency_tree({}, deps)
        self.assertEqual(len(deps.internal_workbenches), len(INTERNAL_WORKBENCHES))

    def test_version_check(self):
        addon = Addon("FreeCAD","https://github.com/FreeCAD/FreeCAD", Addon.Status.NOT_INSTALLED, "master")
        addon.load_metadata_file(os.path.join(self.test_dir, "test_version_detection.xml"))

        self.assertEqual(len(addon.tags),1, "Wrong number of tags found: version requirements should have restricted to only one")
        self.assertFalse("TagA" in addon.tags, "Found 'TagA' in tags, it should have been excluded by version requirement")
        self.assertTrue("TagB" in addon.tags, "Failed to find 'TagB' in tags, it should have been included")
        self.assertFalse("TagC" in addon.tags, "Found 'TagA' in tags, it should have been excluded by version requirement")
