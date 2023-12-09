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
        self.assertTrue(self.uuids.ArchitecturalRendering, "27e48ac9-54e1-4a1f-aa49-d5d690242705")

        self.assertTrue(self.uuids.Costs, "881df808-8726-4c2e-be38-688bb6cce466")

        self.assertTrue(self.uuids.BasicRendering, "f006c7e4-35b7-43d5-bbf9-c5d572309e6e")
        self.assertTrue(self.uuids.TextureRendering, "bbdcc65b-67ca-489c-bd5c-a36e33d1c160")
        self.assertTrue(self.uuids.AdvancedRendering, "c880f092-cdae-43d6-a24b-55e884aacbbf")
        self.assertTrue(self.uuids.VectorRendering, "fdf5a80e-de50-4157-b2e5-b6e5f88b680e")

        self.assertTrue(self.uuids.RenderAppleseed, "b0a10f70-13bf-4598-ab63-bcfbbcd813e3")
        self.assertTrue(self.uuids.RenderCarpaint, "4d2cc163-0707-40e2-a9f7-14288c4b97bd")
        self.assertTrue(self.uuids.RenderCycles, "a6da1b66-929c-48bf-ae80-3b0495c7b50b")
        self.assertTrue(self.uuids.RenderDiffuse, "c19b2d30-c55b-48aa-a938-df9e2f7779cf")
        self.assertTrue(self.uuids.RenderDisney, "f8723572-4470-4c39-a749-6d3b71358a5b")
        self.assertTrue(self.uuids.RenderEmission, "9f6cb588-c89d-4a74-9d0f-2786a8568cec")
        self.assertTrue(self.uuids.RenderGlass, "d76a56f5-7250-4efb-bb89-8ea0a9ccaa6b")
        self.assertTrue(self.uuids.RenderLuxcore, "6b992304-33e0-490b-a391-e9d0af79bb69")
        self.assertTrue(self.uuids.RenderLuxrender, "67ac6a63-e173-4e05-898b-af743f1f9563")
        self.assertTrue(self.uuids.RenderMixed, "84bab333-984f-47fe-a512-d17c7cb2daa9")
        self.assertTrue(self.uuids.RenderOspray, "a4792c23-0be9-47c2-b16d-47b2d2d5efd6")
        self.assertTrue(self.uuids.RenderPbrt, "35b34b82-4325-4d27-97bd-d10bb2c56586")
        self.assertTrue(self.uuids.RenderPovray, "6ec8b415-4c7b-4206-a80b-2ea64101f34b")
        self.assertTrue(self.uuids.RenderSubstancePBR, "f212b643-db96-452e-8428-376a4534e5ab")
        self.assertTrue(self.uuids.RenderTexture, "fc9b6135-95cd-4ba8-ad9a-0972caeebad2")
        self.assertTrue(self.uuids.RenderWB, "344008be-a837-43af-90bc-f795f277b309")

        self.assertTrue(self.uuids.TestModel, "34d0583d-f999-49ba-99e6-aa40bd5c3a6b")

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
