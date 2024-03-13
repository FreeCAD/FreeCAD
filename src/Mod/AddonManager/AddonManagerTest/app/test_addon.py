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
import tempfile
import unittest
import os
import sys

sys.path.append("../../")

from Addon import Addon, INTERNAL_WORKBENCHES
from addonmanager_macro import Macro


class TestAddon(unittest.TestCase):
    MODULE = "test_addon"  # file name without extension

    def setUp(self):
        self.test_dir = os.path.join(os.path.dirname(__file__), "..", "data")

    def test_display_name(self):
        # Case 1: No display name set elsewhere: name == display_name
        addon = Addon(
            "FreeCAD",
            "https://github.com/FreeCAD/FreeCAD",
            Addon.Status.NOT_INSTALLED,
            "master",
        )
        self.assertEqual(addon.name, "FreeCAD")
        self.assertEqual(addon.display_name, "FreeCAD")

        # Case 2: Package.xml metadata file sets a display name:
        addon.load_metadata_file(os.path.join(self.test_dir, "good_package.xml"))
        self.assertEqual(addon.name, "FreeCAD")
        self.assertEqual(addon.display_name, "Test Workbench")

    def test_git_url_cleanup(self):
        base_url = "https://github.com/FreeCAD/FreeCAD"
        test_urls = [f"  {base_url}  ", f"{base_url}.git", f"  {base_url}.git  "]
        for url in test_urls:
            addon = Addon("FreeCAD", url, Addon.Status.NOT_INSTALLED, "master")
            self.assertEqual(addon.url, base_url)

    def test_tag_extraction(self):
        addon = Addon(
            "FreeCAD",
            "https://github.com/FreeCAD/FreeCAD",
            Addon.Status.NOT_INSTALLED,
            "master",
        )
        addon.load_metadata_file(os.path.join(self.test_dir, "good_package.xml"))

        tags = addon.tags
        self.assertEqual(len(tags), 5)
        expected_tags = set()
        expected_tags.add("Tag0")
        expected_tags.add("Tag1")
        expected_tags.add("TagA")
        expected_tags.add("TagB")
        expected_tags.add("TagC")
        self.assertEqual(expected_tags, tags)

    def test_contains_functions(self):
        # Test package.xml combinations:

        # Workbenches
        addon_with_workbench = Addon(
            "FreeCAD",
            "https://github.com/FreeCAD/FreeCAD",
            Addon.Status.NOT_INSTALLED,
            "master",
        )
        addon_with_workbench.load_metadata_file(os.path.join(self.test_dir, "workbench_only.xml"))
        self.assertTrue(addon_with_workbench.contains_workbench())
        self.assertFalse(addon_with_workbench.contains_macro())
        self.assertFalse(addon_with_workbench.contains_preference_pack())

        # Macros
        addon_with_macro = Addon(
            "FreeCAD",
            "https://github.com/FreeCAD/FreeCAD",
            Addon.Status.NOT_INSTALLED,
            "master",
        )
        addon_with_macro.load_metadata_file(os.path.join(self.test_dir, "macro_only.xml"))
        self.assertFalse(addon_with_macro.contains_workbench())
        self.assertTrue(addon_with_macro.contains_macro())
        self.assertFalse(addon_with_macro.contains_preference_pack())

        # Preference Packs
        addon_with_prefpack = Addon(
            "FreeCAD",
            "https://github.com/FreeCAD/FreeCAD",
            Addon.Status.NOT_INSTALLED,
            "master",
        )
        addon_with_prefpack.load_metadata_file(os.path.join(self.test_dir, "prefpack_only.xml"))
        self.assertFalse(addon_with_prefpack.contains_workbench())
        self.assertFalse(addon_with_prefpack.contains_macro())
        self.assertTrue(addon_with_prefpack.contains_preference_pack())

        # Combination
        addon_with_all = Addon(
            "FreeCAD",
            "https://github.com/FreeCAD/FreeCAD",
            Addon.Status.NOT_INSTALLED,
            "master",
        )
        addon_with_all.load_metadata_file(os.path.join(self.test_dir, "combination.xml"))
        self.assertTrue(addon_with_all.contains_workbench())
        self.assertTrue(addon_with_all.contains_macro())
        self.assertTrue(addon_with_all.contains_preference_pack())

        # Now do the simple, explicitly-set cases
        addon_wb = Addon(
            "FreeCAD",
            "https://github.com/FreeCAD/FreeCAD",
            Addon.Status.NOT_INSTALLED,
            "master",
        )
        addon_wb.repo_type = Addon.Kind.WORKBENCH
        self.assertTrue(addon_wb.contains_workbench())
        self.assertFalse(addon_wb.contains_macro())
        self.assertFalse(addon_wb.contains_preference_pack())

        addon_m = Addon(
            "FreeCAD",
            "https://github.com/FreeCAD/FreeCAD",
            Addon.Status.NOT_INSTALLED,
            "master",
        )
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
        self.assertEqual(addon.name, "DoNothing")
        self.assertEqual(
            addon.macro.comment,
            "Do absolutely nothing. For Addon Manager integration tests.",
        )
        self.assertEqual(addon.url, "https://github.com/FreeCAD/FreeCAD")
        self.assertEqual(addon.macro.version, "1.0")
        self.assertEqual(len(addon.macro.other_files), 3)
        self.assertEqual(addon.macro.author, "Chris Hennes")
        self.assertEqual(addon.macro.date, "2022-02-28")
        self.assertEqual(addon.macro.icon, "not_real.png")
        self.assertNotEqual(addon.macro.xpm, "")

    def test_cache(self):
        addon = Addon(
            "FreeCAD",
            "https://github.com/FreeCAD/FreeCAD",
            Addon.Status.NOT_INSTALLED,
            "master",
        )
        cache_data = addon.to_cache()
        second_addon = Addon.from_cache(cache_data)

        self.assertTrue(addon.__dict__, second_addon.__dict__)

    def test_dependency_resolution(self):
        addonA = Addon(
            "AddonA",
            "https://github.com/FreeCAD/FakeAddonA",
            Addon.Status.NOT_INSTALLED,
            "master",
        )
        addonB = Addon(
            "AddonB",
            "https://github.com/FreeCAD/FakeAddonB",
            Addon.Status.NOT_INSTALLED,
            "master",
        )
        addonC = Addon(
            "AddonC",
            "https://github.com/FreeCAD/FakeAddonC",
            Addon.Status.NOT_INSTALLED,
            "master",
        )
        addonD = Addon(
            "AddonD",
            "https://github.com/FreeCAD/FakeAddonD",
            Addon.Status.NOT_INSTALLED,
            "master",
        )

        addonA.requires.add("AddonB")
        addonB.requires.add("AddonC")
        addonB.requires.add("AddonD")
        addonD.requires.add("CAM")

        all_addons = {
            addonA.name: addonA,
            addonB.name: addonB,
            addonC.name: addonC,
            addonD.name: addonD,
        }

        deps = Addon.Dependencies()
        addonA.walk_dependency_tree(all_addons, deps)

        self.assertEqual(len(deps.required_external_addons), 3)
        addon_strings = [addon.name for addon in deps.required_external_addons]
        self.assertTrue(
            "AddonB" in addon_strings,
            "AddonB not in required dependencies, and it should be.",
        )
        self.assertTrue(
            "AddonC" in addon_strings,
            "AddonC not in required dependencies, and it should be.",
        )
        self.assertTrue(
            "AddonD" in addon_strings,
            "AddonD not in required dependencies, and it should be.",
        )
        self.assertTrue(
            "CAM" in deps.internal_workbenches,
            "CAM not in workbench dependencies, and it should be.",
        )

    def test_internal_workbench_list(self):
        addon = Addon(
            "FreeCAD",
            "https://github.com/FreeCAD/FreeCAD",
            Addon.Status.NOT_INSTALLED,
            "master",
        )
        addon.load_metadata_file(os.path.join(self.test_dir, "depends_on_all_workbenches.xml"))
        deps = Addon.Dependencies()
        addon.walk_dependency_tree({}, deps)
        self.assertEqual(len(deps.internal_workbenches), len(INTERNAL_WORKBENCHES))

    def test_version_check(self):
        addon = Addon(
            "FreeCAD",
            "https://github.com/FreeCAD/FreeCAD",
            Addon.Status.NOT_INSTALLED,
            "master",
        )
        addon.load_metadata_file(os.path.join(self.test_dir, "test_version_detection.xml"))

        self.assertEqual(
            len(addon.tags),
            1,
            "Wrong number of tags found: version requirements should have restricted to only one",
        )
        self.assertFalse(
            "TagA" in addon.tags,
            "Found 'TagA' in tags, it should have been excluded by version requirement",
        )
        self.assertTrue(
            "TagB" in addon.tags,
            "Failed to find 'TagB' in tags, it should have been included",
        )
        self.assertFalse(
            "TagC" in addon.tags,
            "Found 'TagA' in tags, it should have been excluded by version requirement",
        )

    def test_try_find_wbname_in_files_empty_dir(self):
        with tempfile.TemporaryDirectory() as mod_dir:
            # Arrange
            test_addon = Addon("test")
            test_addon.mod_directory = mod_dir
            os.mkdir(os.path.join(mod_dir, test_addon.name))

            # Act
            wb_name = test_addon.try_find_wbname_in_files()

            # Assert
            self.assertEqual(wb_name, "")

    def test_try_find_wbname_in_files_non_python_ignored(self):
        with tempfile.TemporaryDirectory() as mod_dir:
            # Arrange
            test_addon = Addon("test")
            test_addon.mod_directory = mod_dir
            base_path = os.path.join(mod_dir, test_addon.name)
            os.mkdir(base_path)
            file_path = os.path.join(base_path, "test.txt")
            with open(file_path, "w", encoding="utf-8") as f:
                f.write("Gui.addWorkbench(TestWorkbench())")

            # Act
            wb_name = test_addon.try_find_wbname_in_files()

            # Assert
            self.assertEqual(wb_name, "")

    def test_try_find_wbname_in_files_simple(self):
        with tempfile.TemporaryDirectory() as mod_dir:
            # Arrange
            test_addon = Addon("test")
            test_addon.mod_directory = mod_dir
            base_path = os.path.join(mod_dir, test_addon.name)
            os.mkdir(base_path)
            file_path = os.path.join(base_path, "test.py")
            with open(file_path, "w", encoding="utf-8") as f:
                f.write("Gui.addWorkbench(TestWorkbench())")

            # Act
            wb_name = test_addon.try_find_wbname_in_files()

            # Assert
            self.assertEqual(wb_name, "TestWorkbench")

    def test_try_find_wbname_in_files_subdir(self):
        with tempfile.TemporaryDirectory() as mod_dir:
            # Arrange
            test_addon = Addon("test")
            test_addon.mod_directory = mod_dir
            base_path = os.path.join(mod_dir, test_addon.name)
            os.mkdir(base_path)
            subdir = os.path.join(base_path, "subdirectory")
            os.mkdir(subdir)
            file_path = os.path.join(subdir, "test.py")
            with open(file_path, "w", encoding="utf-8") as f:
                f.write("Gui.addWorkbench(TestWorkbench())")

            # Act
            wb_name = test_addon.try_find_wbname_in_files()

            # Assert
            self.assertEqual(wb_name, "TestWorkbench")

    def test_try_find_wbname_in_files_variable_used(self):
        with tempfile.TemporaryDirectory() as mod_dir:
            # Arrange
            test_addon = Addon("test")
            test_addon.mod_directory = mod_dir
            base_path = os.path.join(mod_dir, test_addon.name)
            os.mkdir(base_path)
            file_path = os.path.join(base_path, "test.py")
            with open(file_path, "w", encoding="utf-8") as f:
                f.write("Gui.addWorkbench(wb)")

            # Act
            wb_name = test_addon.try_find_wbname_in_files()

            # Assert
            self.assertEqual(wb_name, "")

    def test_try_find_wbname_in_files_variants(self):
        variants = [
            "Gui.addWorkbench(TestWorkbench())",
            "Gui.addWorkbench (TestWorkbench())",
            "Gui.addWorkbench( TestWorkbench() )",
            "Gui.addWorkbench(TestWorkbench( ))",
            "Gui.addWorkbench( TestWorkbench( ) )",
            "Gui.addWorkbench( TestWorkbench ( ) )",
            "Gui.addWorkbench ( TestWorkbench ( ) )",
        ]
        for variant in variants:
            with self.subTest(variant=variant):
                with tempfile.TemporaryDirectory() as mod_dir:
                    # Arrange
                    test_addon = Addon("test")
                    test_addon.mod_directory = mod_dir
                    base_path = os.path.join(mod_dir, test_addon.name)
                    os.mkdir(base_path)
                    file_path = os.path.join(base_path, "test.py")
                    with open(file_path, "w", encoding="utf-8") as f:
                        f.write(variant)

                    # Act
                    wb_name = test_addon.try_find_wbname_in_files()

                    # Assert
                    self.assertEqual(wb_name, "TestWorkbench")
