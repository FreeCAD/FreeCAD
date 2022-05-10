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

import FreeCAD
import unittest
import os

class TestMetadata(unittest.TestCase):

    def setUp(self):
        self.test_dir = os.path.join(FreeCAD.getHomePath(), "Mod", "Test", "TestData")

    def test_xml_constructor(self):
        try:
            filename = os.path.join(self.test_dir, "basic_metadata.xml")
            md = FreeCAD.Metadata(filename)
        except Exception:
            self.fail("Metadata construction from XML file failed")

        with self.assertRaises(FreeCAD.Base.XMLBaseException, msg="Metadata construction from XML file with bad root node did not raise an exception"):
            filename = os.path.join(self.test_dir, "bad_root_node.xml")
            md = FreeCAD.Metadata(filename)

        with self.assertRaises(FreeCAD.Base.XMLBaseException, msg="Metadata construction from invalid XML file did not raise an exception"):
            filename = os.path.join(self.test_dir, "bad_xml.xml")
            md = FreeCAD.Metadata(filename)

        with self.assertRaises(FreeCAD.Base.XMLBaseException, msg="Metadata construction from XML file with invalid version did not raise an exception"):
            filename = os.path.join(self.test_dir, "bad_version.xml")
            md = FreeCAD.Metadata(filename)

    def test_toplevel_tags(self):
        filename = os.path.join(self.test_dir, "basic_metadata.xml")
        md = FreeCAD.Metadata(filename)

        # Tags with only one element:
        self.assertEqual(md.Name, "Test Workbench")
        self.assertEqual(md.Description, "A package.xml file for unit testing.")
        self.assertEqual(md.Version, "1.0.1")
        #self.assertEqual(md.Date, "2022-01-07")
        self.assertEqual(md.Icon, "Resources/icons/PackageIcon.svg")

        # Tags that are lists of elements:
        maintainers = md.Maintainer
        self.assertEqual(len(maintainers), 2)
        
        authors = md.Author
        self.assertEqual(len(authors), 3)

        urls = md.Urls
        self.assertEqual(len(urls), 4)

        tags = md.Tag
        self.assertEqual(len(tags), 2)

    def test_copy_constructor(self):
        filename = os.path.join(self.test_dir, "basic_metadata.xml")
        md = FreeCAD.Metadata(filename)
        copy_of_md = FreeCAD.Metadata(md)
        self.assertEqual(md.Name, copy_of_md.Name)
        self.assertEqual(md.Description, copy_of_md.Description)
        self.assertEqual(md.Version, copy_of_md.Version)

    def test_default_constructor(self):
        try:
            _ = FreeCAD.Metadata()
        except Exception:
            self.fail("Metadata default constructor failed")

    def test_content_types(self):
        filename = os.path.join(self.test_dir, "content_items.xml")
        md = FreeCAD.Metadata(filename)

        content = md.Content
        self.assertTrue("workbench" in content)
        self.assertTrue("preferencepack" in content)
        self.assertTrue("macro" in content)
        self.assertTrue("other_content_item" in content)

        workbenches = content["workbench"]
        preferencepacks = content["preferencepack"]
        macros = content["macro"]
        other = content["other_content_item"]

        self.assertEqual(len(workbenches), 3)
        self.assertEqual(len(macros), 2)
        self.assertEqual(len(preferencepacks), 1)

    def test_content_item_tags(self):
        pass

    def test_last_supported_version(self):
        pass

    def test_first_supported_version(self):
        pass

    def test_supports_current(self):
        pass

    def test_generic_metadata(self):
        pass