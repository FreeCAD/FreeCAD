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
import codecs
import tempfile


class TestMetadata(unittest.TestCase):
    def setUp(self):
        self.test_dir = os.path.join(FreeCAD.getHomePath(), "Mod", "Test", "TestData")

    def test_xml_constructor(self):
        try:
            filename = os.path.join(self.test_dir, "basic_metadata.xml")
            md = FreeCAD.Metadata(filename)
        except Exception:
            self.fail("Metadata construction from XML file failed")

        with self.assertRaises(
            FreeCAD.Base.XMLBaseException,
            msg="Metadata construction from XML file with bad root node did not raise an exception",
        ):
            filename = os.path.join(self.test_dir, "bad_root_node.xml")
            md = FreeCAD.Metadata(filename)

        with self.assertRaises(
            FreeCAD.Base.XMLBaseException,
            msg="Metadata construction from invalid XML file did not raise an exception",
        ):
            filename = os.path.join(self.test_dir, "bad_xml.xml")
            md = FreeCAD.Metadata(filename)

        with self.assertRaises(
            FreeCAD.Base.XMLBaseException,
            msg="Metadata construction from XML file with invalid version did not raise an exception",
        ):
            filename = os.path.join(self.test_dir, "bad_version.xml")
            md = FreeCAD.Metadata(filename)

    def test_toplevel_tags(self):
        filename = os.path.join(self.test_dir, "basic_metadata.xml")
        md = FreeCAD.Metadata(filename)

        # Tags with only one element:
        self.assertEqual(md.Name, "Test Workbench")
        self.assertEqual(md.Description, "A package.xml file for unit testing.")
        self.assertEqual(md.Version, "1.0.1")
        self.assertEqual(md.Date, "2022-01-07")
        self.assertEqual(md.Icon, "Resources/icons/PackageIcon.svg")

        # Tags that are lists of elements:
        maintainers = md.Maintainer
        self.assertEqual(len(maintainers), 2)

        authors = md.Author
        self.assertEqual(len(authors), 3)

        urls = md.Urls
        self.assertEqual(len(urls), 5)

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

        self.assertEqual(len(workbenches), 4)
        self.assertEqual(len(macros), 2)
        self.assertEqual(len(preferencepacks), 1)

    def test_file_path(self):
        # Issue 7112
        try:
            filename = os.path.join(tempfile.gettempdir(), b"H\xc3\xa5vard.xml".decode("utf-8"))
            xmlfile = codecs.open(filename, mode="w", encoding="utf-8")
            xmlfile.write(
                r"""<?xml version="1.0" encoding="UTF-8" standalone="no" ?>
<package format="1" xmlns="https://wiki.freecad.org/Package_Metadata">
  <name>test</name>
  <description>Text</description>
  <version>1.0.0</version>
  <date>2022-01-01</date>
  <content>
    <workbench>
      <classname>Workbench</classname>
    </workbench>
  </content>
</package>"""
            )
            xmlfile.close()
            md = FreeCAD.Metadata(filename)
            self.assertEqual(md.Name, "test")
            self.assertEqual(md.Description, "Text")
            self.assertEqual(md.Version, "1.0.0")
        except UnicodeEncodeError as e:
            print("Ignore UnicodeEncodeError in test_file_path:\n{}".format(str(e)))

    def test_content_item_tags(self):
        filename = os.path.join(self.test_dir, "content_items.xml")
        md = FreeCAD.Metadata(filename)

        content = md.Content
        workbenches = content["workbench"]
        found = [False, False, False, False]
        for workbench in workbenches:
            print(workbench.Classname)
            if workbench.Classname == "TestWorkbenchA":
                found[0] = True
                self.assertEqual(workbench.FreeCADMin, "0.20.0")
                tags = workbench.Tag
                self.assertTrue("Tag A" in tags)
            elif workbench.Classname == "TestWorkbenchB":
                found[1] = True
                self.assertEqual(workbench.FreeCADMin, "0.1.0")
                self.assertEqual(workbench.FreeCADMax, "0.19.0")
                tags = workbench.Tag
                self.assertTrue("Tag B" in tags)
            elif workbench.Classname == "TestWorkbenchC":
                found[2] = True
                tags = workbench.Tag
                self.assertTrue("Tag C" in tags)
                self.assertTrue("Tag D" in tags)
            elif workbench.Classname == "TestWorkbenchD":
                found[3] = True
                dependencies = workbench.Depend
                expected_dependencies = {
                    "DependencyA": {"type": "automatic", "found": False},
                    "InternalWorkbench": {"type": "internal", "found": False},
                    "AddonWorkbench": {"type": "addon", "found": False},
                    "PythonPackage": {"type": "python", "found": False},
                    "DependencyB": {"type": "automatic", "found": False},
                }
                for dep in dependencies:
                    self.assertTrue(dep["package"] in expected_dependencies)
                    self.assertEqual(dep["type"], expected_dependencies[dep["package"]]["type"])
                    expected_dependencies[dep["package"]]["found"] = True
                for name, expected_dep in expected_dependencies.items():
                    self.assertTrue(expected_dep["found"], f"Failed to load dependency '{name}'")
        for f in found:
            self.assertTrue(
                f, f"One of the expected workbenches was not found in the metadata file"
            )

    def test_last_supported_version(self):
        pass

    def test_first_supported_version(self):
        pass

    def test_supports_current(self):
        pass

    def test_generic_metadata(self):
        pass

    def test_min_python_version(self):
        pass
