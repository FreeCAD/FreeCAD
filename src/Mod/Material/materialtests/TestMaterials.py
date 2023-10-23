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

# import FreeCAD
from os import walk
import unittest
import FreeCAD
import Material

parseQuantity = FreeCAD.Units.parseQuantity

class MaterialTestCases(unittest.TestCase):
    def setUp(self):
        self.ModelManager = Material.ModelManager()
        self.MaterialManager = Material.MaterialManager()
        self.uuids = Material.UUIDs()

    def testMaterialManager(self):
        self.assertIn("MaterialLibraries", dir(self.MaterialManager))
        self.assertIn("Materials", dir(self.MaterialManager))

    def testCalculiXSteel(self):
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
        self.assertFalse(steel.isPhysicalModelComplete(self.uuids.LinearElastic)) # Not in the model
        self.assertTrue(steel.isAppearanceModelComplete(self.uuids.BasicRendering)) # inherited from Steel.FCMat

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

        self.assertEqual(properties["Density"], parseQuantity("7900.00 kg/m^3").UserString)
        # self.assertEqual(properties["BulkModulus"], "")
        self.assertAlmostEqual(parseQuantity(properties["PoissonRatio"]).Value, parseQuantity("0.3").Value)
        self.assertEqual(properties["YoungsModulus"], parseQuantity("210.00 GPa").UserString)
        # self.assertEqual(properties["ShearModulus"], "")
        self.assertEqual(properties["SpecificHeat"], parseQuantity("590.00 J/kg/K").UserString)
        self.assertEqual(properties["ThermalConductivity"], parseQuantity("43.00 W/m/K").UserString)
        self.assertEqual(properties["ThermalExpansionCoefficient"], parseQuantity("12.00 µm/m/K").UserString)
        self.assertEqual(properties["AmbientColor"], "(0.0020, 0.0020, 0.0020, 1.0)")
        self.assertEqual(properties["DiffuseColor"], "(0.0000, 0.0000, 0.0000, 1.0)")
        self.assertEqual(properties["EmissiveColor"], "(0.0000, 0.0000, 0.0000, 1.0)")
        self.assertAlmostEqual(parseQuantity(properties["Shininess"]).Value, parseQuantity("0.06").Value)
        self.assertEqual(properties["SpecularColor"], "(0.9800, 0.9800, 0.9800, 1.0)")
        self.assertAlmostEqual(parseQuantity(properties["Transparency"]).Value, parseQuantity("0").Value)

        print("Density " + steel.getPhysicalValue("Density").UserString)
        # print("BulkModulus " + properties["BulkModulus"])
        print("PoissonRatio %f" % steel.getPhysicalValue("PoissonRatio"))
        print("YoungsModulus " + steel.getPhysicalValue("YoungsModulus").UserString)
        # print("ShearModulus " + properties["ShearModulus"])
        print("SpecificHeat " + steel.getPhysicalValue("SpecificHeat").UserString)
        print("ThermalConductivity " + steel.getPhysicalValue("ThermalConductivity").UserString)
        print("ThermalExpansionCoefficient " + steel.getPhysicalValue("ThermalExpansionCoefficient").UserString)
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
        materials = self.MaterialManager.materialsWithModel('f6f9e48c-b116-4e82-ad7f-3659a9219c50') # IsotropicLinearElastic
        materialsComplete = self.MaterialManager.materialsWithModelComplete('f6f9e48c-b116-4e82-ad7f-3659a9219c50') # IsotropicLinearElastic

        self.assertTrue(len(materialsComplete) <= len(materials)) # Not all will be complete

        materialsLinearElastic = self.MaterialManager.materialsWithModel('7b561d1d-fb9b-44f6-9da9-56a4f74d7536') # LinearElastic

        # All LinearElastic models should be in IsotropicLinearElastic since it is inherited
        self.assertTrue(len(materialsLinearElastic) <= len(materials))
        for mat in materialsLinearElastic:
            self.assertIn(mat, materials)

    def testMaterialByPath(self):
        steel = self.MaterialManager.getMaterialByPath('StandardMaterial/Metal/Steel/CalculiX-Steel.FCMat', 'System')
        self.assertIsNotNone(steel)
        self.assertEqual(steel.Name, "CalculiX-Steel")
        self.assertEqual(steel.UUID, "92589471-a6cb-4bbc-b748-d425a17dea7d")

        steel2 = self.MaterialManager.getMaterialByPath('/StandardMaterial/Metal/Steel/CalculiX-Steel.FCMat', 'System')
        self.assertIsNotNone(steel2)
        self.assertEqual(steel2.Name, "CalculiX-Steel")
        self.assertEqual(steel2.UUID, "92589471-a6cb-4bbc-b748-d425a17dea7d")

        steel3 = self.MaterialManager.getMaterialByPath('/System/StandardMaterial/Metal/Steel/CalculiX-Steel.FCMat', 'System')
        self.assertIsNotNone(steel3)
        self.assertEqual(steel3.Name, "CalculiX-Steel")
        self.assertEqual(steel3.UUID, "92589471-a6cb-4bbc-b748-d425a17dea7d")
