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
        self.useBuiltInDir = param.GetBool("UseBuiltInMaterials", True)
        self.useWorkbenchDir = param.GetBool("UseMaterialsFromWorkbenches", True)
        self.useUserDir = param.GetBool("UseMaterialsFromConfigDir", True)
        self.useCustomDir = param.GetBool("UseMaterialsFromCustomDir", False)

        paramExternal = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Material/ExternalInterface")
        self.useExternal = paramExternal.GetBool("UseExternal", False)

        filePath = os.path.dirname(__file__) + os.sep
        testPath = filePath + "Materials"
        param.SetString("CustomMaterialsDir", testPath)
        param.SetBool("UseBuiltInMaterials", False)
        param.SetBool("UseMaterialsFromWorkbenches", False)
        param.SetBool("UseMaterialsFromConfigDir", False)
        param.SetBool("UseMaterialsFromCustomDir", True)

        paramExternal.SetBool("UseExternal", False)

        self.MaterialManager.refresh()

    def tearDown(self):
        param = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Material/Resources")

        # Restore preferences
        param.SetString("CustomMaterialsDir", self.customDir)
        param.SetBool("UseBuiltInMaterials", self.useBuiltInDir)
        param.SetBool("UseMaterialsFromWorkbenches", self.useWorkbenchDir)
        param.SetBool("UseMaterialsFromConfigDir", self.useUserDir)
        param.SetBool("UseMaterialsFromCustomDir", self.useCustomDir)

        paramExternal = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Material/ExternalInterface")
        paramExternal.SetBool("UseExternal", self.useExternal)

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

        # Create an empty filter
        filter = Materials.MaterialFilter()
        self.assertEqual(len(self.MaterialManager.MaterialLibraries), 1)

        filtered = self.MaterialManager.filterMaterials(filter)
        self.assertEqual(len(filtered), 4)

        filtered = self.MaterialManager.filterMaterials(filter, includeLegacy=True)
        self.assertEqual(len(filtered), 5)

        # Create a basic rendering filter
        filter.Name = "Basic Appearance"
        filter.RequiredCompleteModels = [self.uuids.BasicRendering]

        filtered = self.MaterialManager.filterMaterials(filter)
        self.assertEqual(len(filtered), 3)

        filtered = self.MaterialManager.filterMaterials(filter, includeLegacy=True)
        self.assertEqual(len(filtered), 3)

        # Create an advanced rendering filter
        filter= Materials.MaterialFilter()
        filter.Name = "Advanced Appearance"
        filter.RequiredCompleteModels = [self.uuids.AdvancedRendering]

        filtered = self.MaterialManager.filterMaterials(filter)
        self.assertEqual(len(filtered), 0)

        filtered = self.MaterialManager.filterMaterials(filter, includeLegacy=True)
        self.assertEqual(len(filtered), 0)

        # Create a Density filter
        filter= Materials.MaterialFilter()
        filter.Name = "Density"
        filter.RequiredCompleteModels = [self.uuids.Density]

        filtered = self.MaterialManager.filterMaterials(filter)
        self.assertEqual(len(filtered), 2)

        filtered = self.MaterialManager.filterMaterials(filter, includeLegacy=True)
        self.assertEqual(len(filtered), 3)

        # Create a Hardness filter
        filter= Materials.MaterialFilter()
        filter.Name = "Hardness"
        filter.RequiredCompleteModels = [self.uuids.Hardness]

        filtered = self.MaterialManager.filterMaterials(filter)
        self.assertEqual(len(filtered), 0)

        filtered = self.MaterialManager.filterMaterials(filter, includeLegacy=True)
        self.assertEqual(len(filtered), 0)

        # Create a Density and Basic Rendering filter
        filter= Materials.MaterialFilter()
        filter.Name = "Density and Basic Rendering"
        filter.RequiredCompleteModels = [self.uuids.Density, self.uuids.BasicRendering]

        filtered = self.MaterialManager.filterMaterials(filter)
        self.assertEqual(len(filtered), 1)

        filtered = self.MaterialManager.filterMaterials(filter, includeLegacy=True)
        self.assertEqual(len(filtered), 1)

        # Create a Linear Elastic filter
        filter= Materials.MaterialFilter()
        filter.Name = "Linear Elastic"
        filter.RequiredCompleteModels = [self.uuids.LinearElastic]

        filtered = self.MaterialManager.filterMaterials(filter)
        self.assertEqual(len(filtered), 0)

        filtered = self.MaterialManager.filterMaterials(filter, includeLegacy=True)
        self.assertEqual(len(filtered), 0)

        filter= Materials.MaterialFilter()
        filter.Name = "Linear Elastic - incomplete"
        filter.RequiredModels = [self.uuids.LinearElastic]

        filtered = self.MaterialManager.filterMaterials(filter)
        self.assertEqual(len(filtered), 2)

        filtered = self.MaterialManager.filterMaterials(filter, includeLegacy=True)

    def testErrorInput(self):

        self.assertRaises(TypeError, self.MaterialManager.filterMaterials, [])
