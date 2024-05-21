# **************************************************************************
#   Copyright (c) 2023 David Carter <dcarter@davidcarter.ca>              *
#                                                                         *
#   This file is part of the FreeCAD CAx development system.              *
#                                                                         *
#   This program is free software; you can redistribute it and/or modify  *
#   it under the terms of the GNU Lesser General Public License (LGPL)    *
#   as published by the Free Software Foundation; either version 2 of     *
#   the License, or (at your option) any later version.                   *
#   for detail see the LICENCE text file.                                 *
#                                                                         *
#   FreeCAD is distributed in the hope that it will be useful,            *
#   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
#   GNU Library General Public License for more details.                  *
#                                                                         *
#   You should have received a copy of the GNU Library General Public     *
#   License along with FreeCAD; if not, write to the Free Software        *
#   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#   USA                                                                   *
# **************************************************************************

"""
Test module for FreeCAD material cards and APIs
"""
import os

import unittest
import FreeCAD
import Materials

parseQuantity = FreeCAD.Units.parseQuantity

UUIDAcrylicLegacy = ""  # This can't be known until it is loaded
UUIDAluminumAppearance = "3c6d0407-66b3-48ea-a2e8-ee843edf0311"
UUIDAluminumMixed = "5f546608-fcbb-40db-98d7-d8e104eb33ce"
UUIDAluminumPhysical = "a8e60089-550d-4370-8e7e-1734db12a3a9"
UUIDBrassAppearance = "fff3d5c8-98c3-4ee2-8fe5-7e17403c48fcc"


class MaterialFilterTestCases(unittest.TestCase):
    """
    Test class for FreeCAD material cards and APIs
    """

    def setUp(self):
        """Setup function to initialize test data"""
        self.ModelManager = Materials.ModelManager()
        self.MaterialManager = Materials.MaterialManager()
        self.uuids = Materials.UUIDs()

        # Use our test files as a custom directory
        param = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Material/Resources")
        self.customDir = param.GetString("CustomMaterialsDir", "")
        self.useBuiltInDir = param.GetBool("UseBuiltInMaterials", False)
        self.useWorkbenchDir = param.GetBool("UseMaterialsFromWorkbenches", False)
        self.useUserDir = param.GetBool("UseMaterialsFromConfigDir", False)
        self.useCustomDir = param.GetBool("UseMaterialsFromCustomDir", False)

        filePath = os.path.dirname(__file__) + os.sep
        testPath = filePath + "Materials"
        param.SetString("CustomMaterialsDir", testPath)
        param.SetBool("UseBuiltInMaterials", False)
        param.SetBool("UseMaterialsFromWorkbenches", False)
        param.SetBool("UseMaterialsFromConfigDir", False)
        param.SetBool("UseMaterialsFromCustomDir", True)

        self.MaterialManager.refresh()

    def tearDown(self):
        param = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Material/Resources")

        # Restore preferences
        param.SetString("CustomMaterialsDir", self.customDir)
        param.SetBool("UseBuiltInMaterials", self.useBuiltInDir)
        param.SetBool("UseMaterialsFromWorkbenches", self.useWorkbenchDir)
        param.SetBool("UseMaterialsFromConfigDir", self.useUserDir)
        param.SetBool("UseMaterialsFromCustomDir", self.useCustomDir)

        self.MaterialManager.refresh()

    def testFilter(self):
        """Test that our filter returns the correct materials"""

        # First check that our materials are loading
        material = self.MaterialManager.getMaterial(UUIDAluminumAppearance)
        self.assertIsNotNone(material)
        self.assertEqual(material.Name, "TestAluminumAppearance")
        self.assertEqual(material.UUID, UUIDAluminumAppearance)

        material = self.MaterialManager.getMaterial(UUIDAluminumMixed)
        self.assertIsNotNone(material)
        self.assertEqual(material.Name, "TestAluminumMixed")
        self.assertEqual(material.UUID, UUIDAluminumMixed)

        material = self.MaterialManager.getMaterial(UUIDAluminumPhysical)
        self.assertIsNotNone(material)
        self.assertEqual(material.Name, "TestAluminumPhysical")
        self.assertEqual(material.UUID, UUIDAluminumPhysical)

        material = self.MaterialManager.getMaterial(UUIDBrassAppearance)
        self.assertIsNotNone(material)
        self.assertEqual(material.Name, "TestBrassAppearance")
        self.assertEqual(material.UUID, UUIDBrassAppearance)
