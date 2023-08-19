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
        self.assertFalse(steel.hasPhysicalModel('7b561d1d-fb9b-44f6-9da9-56a4f74d7536')) # Legacy linear elastic

        self.assertTrue(steel.hasPhysicalProperty("Density"))
        self.assertTrue(steel.hasPhysicalProperty("BulkModulus"))
        self.assertTrue(steel.hasPhysicalProperty("PoissonRatio"))
        self.assertTrue(steel.hasPhysicalProperty("YoungsModulus"))
        self.assertTrue(steel.hasPhysicalProperty("ShearModulus"))
        self.assertTrue(steel.hasPhysicalProperty("SpecificHeat"))
        self.assertTrue(steel.hasPhysicalProperty("ThermalConductivity"))
        self.assertTrue(steel.hasPhysicalProperty("ThermalExpansionCoefficient"))

        properties = steel.PhysicalProperties
        self.assertIn("Density", properties)
        self.assertIn("BulkModulus", properties)
        self.assertIn("PoissonRatio", properties)
        self.assertIn("YoungsModulus", properties)
        self.assertIn("ShearModulus", properties)
        self.assertIn("SpecificHeat", properties)
        self.assertIn("ThermalConductivity", properties)
        self.assertIn("ThermalExpansionCoefficient", properties)

        properties = steel.AppearanceProperties
        self.assertNotIn("Density", properties)
        self.assertNotIn("BulkModulus", properties)
        self.assertNotIn("PoissonRatio", properties)
        self.assertNotIn("YoungsModulus", properties)
        self.assertNotIn("ShearModulus", properties)
        self.assertNotIn("SpecificHeat", properties)
        self.assertNotIn("ThermalConductivity", properties)
        self.assertNotIn("ThermalExpansionCoefficient", properties)

        properties = steel.Properties
        self.assertIn("Density", properties)
        self.assertIn("BulkModulus", properties)
        self.assertIn("PoissonRatio", properties)
        self.assertIn("YoungsModulus", properties)
        self.assertIn("ShearModulus", properties)
        self.assertIn("SpecificHeat", properties)
        self.assertIn("ThermalConductivity", properties)
        self.assertIn("ThermalExpansionCoefficient", properties)
