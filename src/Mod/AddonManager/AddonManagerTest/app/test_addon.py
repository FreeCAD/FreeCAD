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

from Addon import Addon

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