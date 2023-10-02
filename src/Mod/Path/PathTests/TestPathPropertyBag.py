# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2021 sliptonic <shopinthewoods@gmail.com>               *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************

import FreeCAD
import Path.Base.PropertyBag as PathPropertyBag
import PathTests.PathTestUtils as PathTestUtils


class TestPathPropertyBag(PathTestUtils.PathTestBase):
    def setUp(self):
        self.doc = FreeCAD.newDocument("test-property-bag")

    def tearDown(self):
        FreeCAD.closeDocument(self.doc.Name)

    def test00(self):
        """basic PropertyBag creation and access test"""
        bag = PathPropertyBag.Create()
        self.assertTrue(hasattr(bag, "Proxy"))
        self.assertEqual(bag.Proxy.getCustomProperties(), [])
        self.assertEqual(bag.CustomPropertyGroups, [])

    def test01(self):
        """adding properties to a PropertyBag is tracked properly"""
        bag = PathPropertyBag.Create()
        proxy = bag.Proxy
        proxy.addCustomProperty(
            "App::PropertyString", "Title", "Address", "Some description"
        )
        self.assertTrue(hasattr(bag, "Title"))
        bag.Title = "Madame"
        self.assertEqual(bag.Title, "Madame")
        self.assertEqual(bag.Proxy.getCustomProperties(), ["Title"])
        self.assertEqual(bag.CustomPropertyGroups, ["Address"])

    def test02(self):
        """refreshCustomPropertyGroups deletes empty groups"""
        bag = PathPropertyBag.Create()
        proxy = bag.Proxy
        proxy.addCustomProperty(
            "App::PropertyString", "Title", "Address", "Some description"
        )
        bag.Title = "Madame"
        bag.removeProperty("Title")
        proxy.refreshCustomPropertyGroups()
        self.assertEqual(bag.Proxy.getCustomProperties(), [])
        self.assertEqual(bag.CustomPropertyGroups, [])

    def test03(self):
        """refreshCustomPropertyGroups does not delete non-empty groups"""
        bag = PathPropertyBag.Create()
        proxy = bag.Proxy
        proxy.addCustomProperty(
            "App::PropertyString", "Title", "Address", "Some description"
        )
        proxy.addCustomProperty("App::PropertyString", "Gender", "Attributes")
        bag.Title = "Madame"
        bag.Gender = "Female"
        bag.removeProperty("Gender")
        proxy.refreshCustomPropertyGroups()
        self.assertEqual(bag.Proxy.getCustomProperties(), ["Title"])
        self.assertEqual(bag.CustomPropertyGroups, ["Address"])
