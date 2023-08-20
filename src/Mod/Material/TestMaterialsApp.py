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
import unittest
import Material

class MaterialTestCases(unittest.TestCase):
    def setUp(self):
        self.ModelManager = Material.ModelManager()
        self.MaterialManager = Material.MaterialManager()

    def testManager(self):
        self.assertIn("ModelLibraries", dir(self.ModelManager))
        self.assertIn("Models", dir(self.ModelManager))

    def testModelLoad(self):
        density = self.ModelManager.getModel("454661e5-265b-4320-8e6f-fcf6223ac3af")
        self.assertIsNotNone(density)
        self.assertEqual(density.Name, "Density")
        self.assertEqual(density.UUID, "454661e5-265b-4320-8e6f-fcf6223ac3af")
        self.assertIn("Density", density.Properties)
        prop = density.Properties["Density"]
        self.assertIn("Description", dir(prop))
        self.assertIn("Name", dir(prop))
        self.assertIn("Type", dir(prop))
        self.assertIn("URL", dir(prop))
        self.assertIn("Units", dir(prop))
        self.assertEqual(prop.Name, "Density")

    def testCalculiXSteel(self):
        steel = self.MaterialManager.getMaterial("92589471-a6cb-4bbc-b748-d425a17dea7d")
        self.assertIsNotNone(steel)
        self.assertEqual(steel.Name, "CalculiX-Steel")
        self.assertEqual(steel.UUID, "92589471-a6cb-4bbc-b748-d425a17dea7d")

        self.assertTrue(steel.hasPhysicalModel('454661e5-265b-4320-8e6f-fcf6223ac3af')) # Density
        self.assertTrue(steel.hasPhysicalModel('f6f9e48c-b116-4e82-ad7f-3659a9219c50')) # IsotropicLinearElastic
        self.assertTrue(steel.hasPhysicalModel('9959d007-a970-4ea7-bae4-3eb1b8b883c7')) # Thermal
        self.assertFalse(steel.hasPhysicalModel('7b561d1d-fb9b-44f6-9da9-56a4f74d7536')) # Legacy linear elastic - Not in the model
        self.assertTrue(steel.hasAppearanceModel('f006c7e4-35b7-43d5-bbf9-c5d572309e6e')) # BasicRendering - inherited from Steel.FCMat

        self.assertTrue(steel.isPhysicalModelComplete('454661e5-265b-4320-8e6f-fcf6223ac3af')) # Density
        self.assertFalse(steel.isPhysicalModelComplete('f6f9e48c-b116-4e82-ad7f-3659a9219c50')) # IsotropicLinearElastic - incomplete
        self.assertTrue(steel.isPhysicalModelComplete('9959d007-a970-4ea7-bae4-3eb1b8b883c7')) # Thermal
        self.assertFalse(steel.isPhysicalModelComplete('7b561d1d-fb9b-44f6-9da9-56a4f74d7536')) # Legacy linear elastic - Not in the model
        self.assertTrue(steel.isAppearanceModelComplete('f006c7e4-35b7-43d5-bbf9-c5d572309e6e')) # BasicRendering - inherited from Steel.FCMat

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

        # print("Density " + properties["Density"])
        # print("BulkModulus " + properties["BulkModulus"])
        # print("PoissonRatio " + properties["PoissonRatio"])
        # print("YoungsModulus " + properties["YoungsModulus"])
        # print("ShearModulus " + properties["ShearModulus"])
        # print("SpecificHeat " + properties["SpecificHeat"])
        # print("ThermalConductivity " + properties["ThermalConductivity"])
        # print("ThermalExpansionCoefficient " + properties["ThermalExpansionCoefficient"])
        # print("AmbientColor " + properties["AmbientColor"])
        # print("DiffuseColor " + properties["DiffuseColor"])
        # print("EmissiveColor " + properties["EmissiveColor"])
        # print("Shininess " + properties["Shininess"])
        # print("SpecularColor " + properties["SpecularColor"])
        # print("Transparency " + properties["Transparency"])
        
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
        
        self.assertEqual(properties["Density"], "7900 kg/m^3")
        # self.assertEqual(properties["BulkModulus"], "")
        self.assertEqual(properties["PoissonRatio"], "0.30000001192092896")
        self.assertEqual(properties["YoungsModulus"], "210000 MPa")
        # self.assertEqual(properties["ShearModulus"], "")
        self.assertEqual(properties["SpecificHeat"], "590 J/kg/K")
        self.assertEqual(properties["ThermalConductivity"], "43 W/m/K")
        self.assertEqual(properties["ThermalExpansionCoefficient"], "0.000012 m/m/K")
        self.assertEqual(properties["AmbientColor"], "(0.0020, 0.0020, 0.0020, 1.0)")
        self.assertEqual(properties["DiffuseColor"], "(0.0000, 0.0000, 0.0000, 1.0)")
        self.assertEqual(properties["EmissiveColor"], "(0.0000, 0.0000, 0.0000, 1.0)")
        self.assertEqual(properties["Shininess"], "0.05999999865889549")
        self.assertEqual(properties["SpecularColor"], "(0.9800, 0.9800, 0.9800, 1.0)")
        self.assertEqual(properties["Transparency"], "0")

    def testMaterialsWithModel(self):
        materials = self.MaterialManager.materialsWithModel('f6f9e48c-b116-4e82-ad7f-3659a9219c50') # IsotropicLinearElastic
        materialsComplete = self.MaterialManager.materialsWithModelComplete('f6f9e48c-b116-4e82-ad7f-3659a9219c50') # IsotropicLinearElastic

        self.assertTrue(len(materialsComplete) <= len(materials)) # Not all will be complete

        materialsLinearElastic = self.MaterialManager.materialsWithModel('7b561d1d-fb9b-44f6-9da9-56a4f74d7536') # LinearElastic

        # All LinearElastic models should be in IsotropicLinearElastic since it is inherited
        self.assertTrue(len(materialsLinearElastic) <= len(materials))
        for mat in materialsLinearElastic:
            self.assertIn(mat, materials)

