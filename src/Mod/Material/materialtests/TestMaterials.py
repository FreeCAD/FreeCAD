#**************************************************************************
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
#**************************************************************************

"""
Test module for FreeCAD material cards and APIs
"""

import unittest
import FreeCAD
import Material

parseQuantity = FreeCAD.Units.parseQuantity

class MaterialTestCases(unittest.TestCase):
    """
    Test class for FreeCAD material cards and APIs
    """

    def setUp(self):
        """ Setup function to initialize test data """
        self.ModelManager = Material.ModelManager()
        self.MaterialManager = Material.MaterialManager()
        self.uuids = Material.UUIDs()

    def testMaterialManager(self):
        """ Ensure the MaterialManager has been initialized correctly """
        self.assertIn("MaterialLibraries", dir(self.MaterialManager))
        self.assertIn("Materials", dir(self.MaterialManager))

    def testCalculiXSteel(self):
        """
        Test a representative material card for CalculX Steel

        As a well populated material card, the CalculiX Steel material is a good subject
        for testing as many of the properties and API access methods as possible.
        """

        steel = self.MaterialManager.getMaterial("92589471-a6cb-4bbc-b748-d425a17dea7d")
        self.assertIsNotNone(steel)
        self.assertEqual(steel.Name, "CalculiX-Steel")
        self.assertEqual(steel.UUID, "92589471-a6cb-4bbc-b748-d425a17dea7d")

        self.assertTrue(steel.hasPhysicalModel(self.uuids.Density))
        self.assertTrue(steel.hasPhysicalModel(self.uuids.IsotropicLinearElastic))
        self.assertTrue(steel.hasPhysicalModel(self.uuids.Thermal))
        self.assertFalse(steel.hasPhysicalModel(self.uuids.LinearElastic)) # Not in the model
        self.assertTrue(steel.hasAppearanceModel(self.uuids.BasicRendering)) # inherited from Steel.FCMat

        self.assertTrue(steel.isPhysicalModelComplete(self.uuids.Density))
        self.assertFalse(steel.isPhysicalModelComplete(self.uuids.IsotropicLinearElastic))
        self.assertTrue(steel.isPhysicalModelComplete(self.uuids.Thermal))
        self.assertFalse(steel.isPhysicalModelComplete(self.uuids.LinearElastic))
        self.assertTrue(steel.isAppearanceModelComplete(self.uuids.BasicRendering))

        self.assertTrue(steel.hasPhysicalProperty("Density"))
        self.assertTrue(steel.hasPhysicalProperty("BulkModulus"))
        self.assertTrue(steel.hasPhysicalProperty("PoissonRatio"))
        self.assertTrue(steel.hasPhysicalProperty("YoungsModulus"))
        self.assertTrue(steel.hasPhysicalProperty("ShearModulus"))
        self.assertTrue(steel.hasPhysicalProperty("SpecificHeat"))
        self.assertTrue(steel.hasPhysicalProperty("ThermalConductivity"))
        self.assertTrue(steel.hasPhysicalProperty("ThermalExpansionCoefficient"))
        self.assertTrue(steel.hasAppearanceProperty("AmbientColor"))
        self.assertTrue(steel.hasAppearanceProperty("DiffuseColor"))
        self.assertTrue(steel.hasAppearanceProperty("EmissiveColor"))
        self.assertTrue(steel.hasAppearanceProperty("Shininess"))
        self.assertTrue(steel.hasAppearanceProperty("SpecularColor"))
        self.assertTrue(steel.hasAppearanceProperty("Transparency"))

        properties = steel.PhysicalProperties
        self.assertIn("Density", properties)
        self.assertNotIn("BulkModulus", properties)
        self.assertIn("PoissonRatio", properties)
        self.assertIn("YoungsModulus", properties)
        self.assertNotIn("ShearModulus", properties)
        self.assertIn("SpecificHeat", properties)
        self.assertIn("ThermalConductivity", properties)
        self.assertIn("ThermalExpansionCoefficient", properties)
        self.assertNotIn("AmbientColor", properties)
        self.assertNotIn("DiffuseColor", properties)
        self.assertNotIn("EmissiveColor", properties)
        self.assertNotIn("Shininess", properties)
        self.assertNotIn("SpecularColor", properties)
        self.assertNotIn("Transparency", properties)

        properties = steel.AppearanceProperties
        self.assertNotIn("Density", properties)
        self.assertNotIn("BulkModulus", properties)
        self.assertNotIn("PoissonRatio", properties)
        self.assertNotIn("YoungsModulus", properties)
        self.assertNotIn("ShearModulus", properties)
        self.assertNotIn("SpecificHeat", properties)
        self.assertNotIn("ThermalConductivity", properties)
        self.assertNotIn("ThermalExpansionCoefficient", properties)
        self.assertIn("AmbientColor", properties)
        self.assertIn("DiffuseColor", properties)
        self.assertIn("EmissiveColor", properties)
        self.assertIn("Shininess", properties)
        self.assertIn("SpecularColor", properties)
        self.assertIn("Transparency", properties)

        properties = steel.Properties
        self.assertIn("Density", properties)
        self.assertNotIn("BulkModulus", properties)
        self.assertIn("PoissonRatio", properties)
        self.assertIn("YoungsModulus", properties)
        self.assertNotIn("ShearModulus", properties)
        self.assertIn("SpecificHeat", properties)
        self.assertIn("ThermalConductivity", properties)
        self.assertIn("ThermalExpansionCoefficient", properties)
        self.assertIn("AmbientColor", properties)
        self.assertIn("DiffuseColor", properties)
        self.assertIn("EmissiveColor", properties)
        self.assertIn("Shininess", properties)
        self.assertIn("SpecularColor", properties)
        self.assertIn("Transparency", properties)

        #
        # The test for ThermalExpansionCoefficient causes problems with some localizations
        # due to the Unicode mu character in the units. This will happen with
        # locales that don't support UTF8 such as zh_CN (It does support UTF-8)
        #
        # When this is a problem simply comment the lines printing ThermalExpansionCoefficient
        print("Density " + properties["Density"])
        # print("BulkModulus " + properties["BulkModulus"])
        print("PoissonRatio " + properties["PoissonRatio"])
        print("YoungsModulus " + properties["YoungsModulus"])
        # print("ShearModulus " + properties["ShearModulus"])
        print("SpecificHeat " + properties["SpecificHeat"])
        print("ThermalConductivity " + properties["ThermalConductivity"])
        print("ThermalExpansionCoefficient " + properties["ThermalExpansionCoefficient"])
        print("AmbientColor " + properties["AmbientColor"])
        print("DiffuseColor " + properties["DiffuseColor"])
        print("EmissiveColor " + properties["EmissiveColor"])
        print("Shininess " + properties["Shininess"])
        print("SpecularColor " + properties["SpecularColor"])
        print("Transparency " + properties["Transparency"])

        self.assertTrue(len(properties["Density"]) > 0)
        # self.assertTrue(len(properties["BulkModulus"]) == 0)
        self.assertTrue(len(properties["PoissonRatio"]) > 0)
        self.assertTrue(len(properties["YoungsModulus"]) > 0)
        # self.assertTrue(len(properties["ShearModulus"]) == 0)
        self.assertTrue(len(properties["SpecificHeat"]) > 0)
        self.assertTrue(len(properties["ThermalConductivity"]) > 0)
        self.assertTrue(len(properties["ThermalExpansionCoefficient"]) > 0)
        self.assertTrue(len(properties["AmbientColor"]) > 0)
        self.assertTrue(len(properties["DiffuseColor"]) > 0)
        self.assertTrue(len(properties["EmissiveColor"]) > 0)
        self.assertTrue(len(properties["Shininess"]) > 0)
        self.assertTrue(len(properties["SpecularColor"]) > 0)
        self.assertTrue(len(properties["Transparency"]) > 0)

        self.assertEqual(parseQuantity(properties["Density"]).UserString,
                         parseQuantity("7900.00 kg/m^3").UserString)
        # self.assertEqual(properties["BulkModulus"], "")
        self.assertAlmostEqual(parseQuantity(properties["PoissonRatio"]).Value,
                               parseQuantity("0.3").Value)
        self.assertEqual(parseQuantity(properties["YoungsModulus"]).UserString,
                         parseQuantity("210.00 GPa").UserString)
        # self.assertEqual(properties["ShearModulus"], "")
        self.assertEqual(parseQuantity(properties["SpecificHeat"]).UserString,
                         parseQuantity("590.00 J/kg/K").UserString)
        self.assertEqual(parseQuantity(properties["ThermalConductivity"]).UserString,
                         parseQuantity("43.00 W/m/K").UserString)
        self.assertEqual(parseQuantity(properties["ThermalExpansionCoefficient"]).UserString,
                         parseQuantity("12.00 µm/m/K").UserString)
        self.assertEqual(properties["AmbientColor"], "(0.0020, 0.0020, 0.0020, 1.0)")
        self.assertEqual(properties["DiffuseColor"], "(0.0000, 0.0000, 0.0000, 1.0)")
        self.assertEqual(properties["EmissiveColor"], "(0.0000, 0.0000, 0.0000, 1.0)")
        self.assertAlmostEqual(parseQuantity(properties["Shininess"]).Value, parseQuantity("0.06").Value)
        self.assertEqual(properties["SpecularColor"], "(0.9800, 0.9800, 0.9800, 1.0)")
        self.assertAlmostEqual(parseQuantity(properties["Transparency"]).Value,
                               parseQuantity("0").Value)

        print("Density " + steel.getPhysicalValue("Density").UserString)
        # print("BulkModulus " + properties["BulkModulus"])
        print("PoissonRatio %f" % steel.getPhysicalValue("PoissonRatio"))
        print("YoungsModulus " + steel.getPhysicalValue("YoungsModulus").UserString)
        # print("ShearModulus " + properties["ShearModulus"])
        print("SpecificHeat " + steel.getPhysicalValue("SpecificHeat").UserString)
        print("ThermalConductivity " + steel.getPhysicalValue("ThermalConductivity").UserString)
        print("ThermalExpansionCoefficient " + \
              steel.getPhysicalValue("ThermalExpansionCoefficient").UserString)
        print("AmbientColor " + steel.getAppearanceValue("AmbientColor"))
        print("DiffuseColor " + steel.getAppearanceValue("DiffuseColor"))
        print("EmissiveColor " + steel.getAppearanceValue("EmissiveColor"))
        print("Shininess %f" % steel.getAppearanceValue("Shininess"))
        print("SpecularColor " + steel.getAppearanceValue("SpecularColor"))
        print("Transparency %f" % steel.getAppearanceValue("Transparency"))

        self.assertAlmostEqual(steel.getPhysicalValue("Density").Value, 7.9e-06)
        self.assertAlmostEqual(steel.getPhysicalValue("PoissonRatio"), 0.3)
        self.assertAlmostEqual(steel.getPhysicalValue("YoungsModulus").Value, 210000000.0)
        self.assertAlmostEqual(steel.getPhysicalValue("SpecificHeat").Value, 590000000.0)
        self.assertAlmostEqual(steel.getPhysicalValue("ThermalConductivity").Value, 43000.0)
        self.assertAlmostEqual(steel.getPhysicalValue("ThermalExpansionCoefficient").Value, 1.2e-05)
        self.assertEqual(steel.getAppearanceValue("AmbientColor"), "(0.0020, 0.0020, 0.0020, 1.0)")
        self.assertEqual(steel.getAppearanceValue("DiffuseColor"), "(0.0000, 0.0000, 0.0000, 1.0)")
        self.assertEqual(steel.getAppearanceValue("EmissiveColor"), "(0.0000, 0.0000, 0.0000, 1.0)")
        self.assertAlmostEqual(steel.getAppearanceValue("Shininess"), 0.06)
        self.assertEqual(steel.getAppearanceValue("SpecularColor"), "(0.9800, 0.9800, 0.9800, 1.0)")
        self.assertAlmostEqual(steel.getAppearanceValue("Transparency"), 0.0)

    def testMaterialsWithModel(self):
        """
        Test functions that return a list of models supporting specific material models
        """
        # IsotropicLinearElastic
        materials = self.MaterialManager.materialsWithModel('f6f9e48c-b116-4e82-ad7f-3659a9219c50')
        materialsComplete = self.MaterialManager \
            .materialsWithModelComplete('f6f9e48c-b116-4e82-ad7f-3659a9219c50')

        self.assertTrue(len(materialsComplete) <= len(materials)) # Not all will be complete

        materialsLinearElastic = self.MaterialManager \
            .materialsWithModel('7b561d1d-fb9b-44f6-9da9-56a4f74d7536') # LinearElastic

        # All LinearElastic models should be in IsotropicLinearElastic since it is inherited
        self.assertTrue(len(materialsLinearElastic) <= len(materials))
        for mat in materialsLinearElastic:
            self.assertIn(mat, materials)

    def testMaterialByPath(self):
        """
        Test loading models by path

        Valid models may have different prefixes
        """
        steel = self.MaterialManager \
            .getMaterialByPath('Standard/Metal/Steel/CalculiX-Steel.FCMat', 'System')
        self.assertIsNotNone(steel)
        self.assertEqual(steel.Name, "CalculiX-Steel")
        self.assertEqual(steel.UUID, "92589471-a6cb-4bbc-b748-d425a17dea7d")

        steel2 = self.MaterialManager \
            .getMaterialByPath('/Standard/Metal/Steel/CalculiX-Steel.FCMat', 'System')
        self.assertIsNotNone(steel2)
        self.assertEqual(steel2.Name, "CalculiX-Steel")
        self.assertEqual(steel2.UUID, "92589471-a6cb-4bbc-b748-d425a17dea7d")

        steel3 = self.MaterialManager \
            .getMaterialByPath('/System/Standard/Metal/Steel/CalculiX-Steel.FCMat', 'System')
        self.assertIsNotNone(steel3)
        self.assertEqual(steel3.Name, "CalculiX-Steel")
        self.assertEqual(steel3.UUID, "92589471-a6cb-4bbc-b748-d425a17dea7d")

    def testLists(self):
        """
        Test API access to lists
        """

        mat = self.MaterialManager.getMaterial("c6c64159-19c1-40b5-859c-10561f20f979")
        self.assertIsNotNone(mat)
        self.assertEqual(mat.Name, "Test Material")
        self.assertEqual(mat.UUID, "c6c64159-19c1-40b5-859c-10561f20f979")

        self.assertTrue(mat.hasPhysicalModel(self.uuids.TestModel))
        self.assertFalse(mat.isPhysicalModelComplete(self.uuids.TestModel))

        self.assertTrue(mat.hasPhysicalProperty("TestList"))

        testList = mat.getPhysicalValue("TestList")
        self.assertEqual(len(testList), 6)
        self.assertEqual(testList[0],
                         "Now is the time for all good men to come to the aid of the party")
        self.assertEqual(testList[1], "The quick brown fox jumps over the lazy dogs back")
        self.assertEqual(testList[2], "Lore Ipsum")
        self.assertEqual(testList[3], "Single quote '")
        self.assertEqual(testList[4], "Double quote \"")
        self.assertEqual(testList[5], "Backslash \\")

        properties = mat.Properties
        self.assertIn("TestList", properties)
        self.assertTrue(len(properties["TestList"]) == 0)

    def test2DArray(self):
        """
        Test API access to 2D arrays
        """

        mat = self.MaterialManager.getMaterial("c6c64159-19c1-40b5-859c-10561f20f979")
        self.assertIsNotNone(mat)
        self.assertEqual(mat.Name, "Test Material")
        self.assertEqual(mat.UUID, "c6c64159-19c1-40b5-859c-10561f20f979")

        self.assertTrue(mat.hasPhysicalModel(self.uuids.TestModel))
        self.assertFalse(mat.isPhysicalModelComplete(self.uuids.TestModel))

        self.assertTrue(mat.hasPhysicalProperty("TestArray2D"))

        array = mat.getPhysicalValue("TestArray2D")
        self.assertIsNotNone(array)
        self.assertEqual(array.Rows, 3)
        self.assertEqual(array.Columns, 2)

        arrayData = array.Array
        self.assertIsNotNone(arrayData)
        self.assertEqual(len(arrayData), 3)
        self.assertEqual(len(arrayData[0]), 2)
        self.assertEqual(len(arrayData[1]), 2)
        self.assertEqual(len(arrayData[2]), 2)

        self.assertEqual(arrayData[0][0].UserString, parseQuantity("10.00 C").UserString)
        self.assertEqual(arrayData[0][1].UserString, parseQuantity("10.00 kg/m^3").UserString)
        self.assertEqual(arrayData[1][0].UserString, parseQuantity("20.00 C").UserString)
        self.assertEqual(arrayData[1][1].UserString, parseQuantity("20.00 kg/m^3").UserString)
        self.assertEqual(arrayData[2][0].UserString, parseQuantity("30.00 C").UserString)
        self.assertEqual(arrayData[2][1].UserString, parseQuantity("30.00 kg/m^3").UserString)

        self.assertAlmostEqual(arrayData[0][0].Value, 10.0)
        self.assertAlmostEqual(arrayData[0][1].Value, 1e-8)
        self.assertAlmostEqual(arrayData[1][0].Value, 20.0)
        self.assertAlmostEqual(arrayData[1][1].Value, 2e-8)
        self.assertAlmostEqual(arrayData[2][0].Value, 30.0)
        self.assertAlmostEqual(arrayData[2][1].Value, 3e-8)

        self.assertAlmostEqual(arrayData[-1][0].Value, 30.0) # Last in list
        with self.assertRaises(IndexError):
            self.assertAlmostEqual(arrayData[3][0].Value, 10.0)
        self.assertAlmostEqual(arrayData[0][-1].Value, 1e-8)
        with self.assertRaises(IndexError):
            self.assertAlmostEqual(arrayData[0][2].Value, 10.0)

        self.assertEqual(array.getValue(0,0).UserString, parseQuantity("10.00 C").UserString)
        self.assertEqual(array.getValue(0,1).UserString, parseQuantity("10.00 kg/m^3").UserString)
        self.assertEqual(array.getValue(1,0).UserString, parseQuantity("20.00 C").UserString)
        self.assertEqual(array.getValue(1,1).UserString, parseQuantity("20.00 kg/m^3").UserString)
        self.assertEqual(array.getValue(2,0).UserString, parseQuantity("30.00 C").UserString)
        self.assertEqual(array.getValue(2,1).UserString, parseQuantity("30.00 kg/m^3").UserString)

        self.assertAlmostEqual(array.getValue(0,0).Value, 10.0)
        self.assertAlmostEqual(array.getValue(0,1).Value, 1e-8)
        self.assertAlmostEqual(array.getValue(1,0).Value, 20.0)
        self.assertAlmostEqual(array.getValue(1,1).Value, 2e-8)
        self.assertAlmostEqual(array.getValue(2,0).Value, 30.0)
        self.assertAlmostEqual(array.getValue(2,1).Value, 3e-8)

        with self.assertRaises(IndexError):
            self.assertAlmostEqual(array.getValue(-1,0).Value, 10.0)
        with self.assertRaises(IndexError):
            self.assertAlmostEqual(array.getValue(3,0).Value, 10.0)
        with self.assertRaises(IndexError):
            self.assertAlmostEqual(array.getValue(0,-1).Value, 10.0)
        with self.assertRaises(IndexError):
            self.assertAlmostEqual(array.getValue(0,2).Value, 10.0)

        for rowIndex in range(0, array.Rows):
            row = array.getRow(rowIndex)
            self.assertIsNotNone(row)
            self.assertEqual(len(row), 2)

        with self.assertRaises(IndexError):
            row = array.getRow(-1)
        with self.assertRaises(IndexError):
            row = array.getRow(3)

    def test3DArray(self):
        """
        Test API access to 3D arrays
        """

        mat = self.MaterialManager.getMaterial("c6c64159-19c1-40b5-859c-10561f20f979")
        self.assertIsNotNone(mat)
        self.assertEqual(mat.Name, "Test Material")
        self.assertEqual(mat.UUID, "c6c64159-19c1-40b5-859c-10561f20f979")

        self.assertTrue(mat.hasPhysicalModel(self.uuids.TestModel))
        self.assertFalse(mat.isPhysicalModelComplete(self.uuids.TestModel))

        self.assertTrue(mat.hasPhysicalProperty("TestArray3D"))

        array = mat.getPhysicalValue("TestArray3D")
        self.assertIsNotNone(array)
        self.assertEqual(array.Depth, 3)
        self.assertEqual(array.Columns, 2)
        self.assertEqual(array.getRows(), 2)
        self.assertEqual(array.getRows(0), 2)
        self.assertEqual(array.getRows(1), 0)
        self.assertEqual(array.getRows(2), 3)

        arrayData = array.Array
        self.assertIsNotNone(arrayData)
        self.assertEqual(len(arrayData), 3)
        self.assertEqual(len(arrayData[0]), 2)
        self.assertEqual(len(arrayData[1]), 0)
        self.assertEqual(len(arrayData[2]), 3)

        self.assertEqual(arrayData[0][0][0].UserString, parseQuantity("11.00 Pa").UserString)
        self.assertEqual(arrayData[0][0][1].UserString, parseQuantity("12.00 Pa").UserString)
        self.assertEqual(arrayData[0][1][0].UserString, parseQuantity("21.00 Pa").UserString)
        self.assertEqual(arrayData[0][1][1].UserString, parseQuantity("22.00 Pa").UserString)
        self.assertEqual(arrayData[2][0][0].UserString, parseQuantity("10.00 Pa").UserString)
        self.assertEqual(arrayData[2][0][1].UserString, parseQuantity("11.00 Pa").UserString)
        self.assertEqual(arrayData[2][1][0].UserString, parseQuantity("20.00 Pa").UserString)
        self.assertEqual(arrayData[2][1][1].UserString, parseQuantity("21.00 Pa").UserString)
        self.assertEqual(arrayData[2][2][0].UserString, parseQuantity("30.00 Pa").UserString)
        self.assertEqual(arrayData[2][2][1].UserString, parseQuantity("31.00 Pa").UserString)

        self.assertEqual(array.getDepthValue(0).UserString, parseQuantity("10.00 C").UserString)
        self.assertEqual(array.getDepthValue(1).UserString, parseQuantity("20.00 C").UserString)
        self.assertEqual(array.getDepthValue(2).UserString, parseQuantity("30.00 C").UserString)

        self.assertEqual(arrayData[0][0][-1].UserString, parseQuantity("12.00 Pa").UserString)
        with self.assertRaises(IndexError):
            self.assertEqual(arrayData[0][0][2].UserString, parseQuantity("11.00 Pa").UserString)
        self.assertEqual(arrayData[0][-1][0].UserString, parseQuantity("21.00 Pa").UserString)
        with self.assertRaises(IndexError):
            self.assertEqual(arrayData[0][2][0].UserString, parseQuantity("11.00 Pa").UserString)
        with self.assertRaises(IndexError):
            self.assertEqual(arrayData[1][0][0].UserString, parseQuantity("11.00 Pa").UserString)
        self.assertEqual(arrayData[-1][0][0].UserString, parseQuantity("10.00 Pa").UserString)
        with self.assertRaises(IndexError):
            self.assertEqual(arrayData[3][0][0].UserString, parseQuantity("11.00 Pa").UserString)

        with self.assertRaises(IndexError):
            self.assertEqual(array.getDepthValue(-1).UserString,
                             parseQuantity("10.00 C").UserString)
        with self.assertRaises(IndexError):
            self.assertEqual(array.getDepthValue(3).UserString,
                             parseQuantity("10.00 C").UserString)

        self.assertEqual(array.getValue(0,0,0).UserString, parseQuantity("11.00 Pa").UserString)
        self.assertEqual(array.getValue(0,0,1).UserString, parseQuantity("12.00 Pa").UserString)
        self.assertEqual(array.getValue(0,1,0).UserString, parseQuantity("21.00 Pa").UserString)
        self.assertEqual(array.getValue(0,1,1).UserString, parseQuantity("22.00 Pa").UserString)
        self.assertEqual(array.getValue(2,0,0).UserString, parseQuantity("10.00 Pa").UserString)
        self.assertEqual(array.getValue(2,0,1).UserString, parseQuantity("11.00 Pa").UserString)
        self.assertEqual(array.getValue(2,1,0).UserString, parseQuantity("20.00 Pa").UserString)
        self.assertEqual(array.getValue(2,1,1).UserString, parseQuantity("21.00 Pa").UserString)
        self.assertEqual(array.getValue(2,2,0).UserString, parseQuantity("30.00 Pa").UserString)
        self.assertEqual(array.getValue(2,2,1).UserString, parseQuantity("31.00 Pa").UserString)

        with self.assertRaises(IndexError):
            self.assertEqual(array.getValue(0,0,-1).UserString,
                             parseQuantity("11.00 Pa").UserString)
        with self.assertRaises(IndexError):
            self.assertEqual(array.getValue(0,0,2).UserString,
                             parseQuantity("11.00 Pa").UserString)
        with self.assertRaises(IndexError):
            self.assertEqual(array.getValue(0,-1,0).UserString,
                             parseQuantity("11.00 Pa").UserString)
        with self.assertRaises(IndexError):
            self.assertEqual(array.getValue(0,2,0).UserString,
                             parseQuantity("11.00 Pa").UserString)
        with self.assertRaises(IndexError):
            self.assertEqual(array.getValue(1,0,0).UserString,
                             parseQuantity("11.00 Pa").UserString)
        with self.assertRaises(IndexError):
            self.assertEqual(array.getValue(-1,0,0).UserString,
                             parseQuantity("11.00 Pa").UserString)
        with self.assertRaises(IndexError):
            self.assertEqual(array.getValue(3,0,0).UserString,
                             parseQuantity("11.00 Pa").UserString)
