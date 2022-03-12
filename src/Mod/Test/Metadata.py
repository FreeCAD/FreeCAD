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

    def test_content_types(self):
        pass

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