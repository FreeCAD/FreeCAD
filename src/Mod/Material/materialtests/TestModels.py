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
# import locale
# locale.setpreferredencoding("UTF8")

class ModelTestCases(unittest.TestCase):
    def setUp(self):
        self.ModelManager = Material.ModelManager()
        self.uuids = Material.UUIDs()

    def testModelManager(self):
        self.assertIn("ModelLibraries", dir(self.ModelManager))
        self.assertIn("Models", dir(self.ModelManager))

    def testUUIDs(self):
        self.assertTrue(self.uuids.Father, "9cdda8b6-b606-4778-8f13-3934d8668e67")
        self.assertTrue(self.uuids.MaterialStandard, "1e2c0088-904a-4537-925f-64064c07d700")

        self.assertTrue(self.uuids.Density, "454661e5-265b-4320-8e6f-fcf6223ac3af")
        self.assertTrue(self.uuids.IsotropicLinearElastic, "f6f9e48c-b116-4e82-ad7f-3659a9219c50")
        self.assertTrue(self.uuids.LinearElastic,"7b561d1d-fb9b-44f6-9da9-56a4f74d7536")
        self.assertTrue(self.uuids.OgdenYld2004p18, "3ef9e427-cc25-43f7-817f-79ff0d49625f")
        self.assertTrue(self.uuids.OrthotropicLinearElastic, "b19ccc6b-a431-418e-91c2-0ac8c649d146")

        self.assertTrue(self.uuids.Fluid, "1ae66d8c-1ba1-4211-ad12-b9917573b202")

        self.assertTrue(self.uuids.Thermal, "9959d007-a970-4ea7-bae4-3eb1b8b883c7")

        self.assertTrue(self.uuids.Electromagnetic, "b2eb5f48-74b3-4193-9fbb-948674f427f3")

        self.assertTrue(self.uuids.Architectural, "32439c3b-262f-4b7b-99a8-f7f44e5894c8")

        self.assertTrue(self.uuids.Costs, "881df808-8726-4c2e-be38-688bb6cce466")

        self.assertTrue(self.uuids.BasicRendering, "f006c7e4-35b7-43d5-bbf9-c5d572309e6e")
        self.assertTrue(self.uuids.TextureRendering, "bbdcc65b-67ca-489c-bd5c-a36e33d1c160")
        self.assertTrue(self.uuids.AdvancedRendering, "c880f092-cdae-43d6-a24b-55e884aacbbf")
        self.assertTrue(self.uuids.VectorRendering, "fdf5a80e-de50-4157-b2e5-b6e5f88b680e")

    def testModelLoad(self):
        density = self.ModelManager.getModel(self.uuids.Density)
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
