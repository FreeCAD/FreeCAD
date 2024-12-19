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
Test module for FreeCAD material models
"""

import unittest
import FreeCAD
import Materials

parseQuantity = FreeCAD.Units.parseQuantity

class ModelTestCases(unittest.TestCase):
    """
    Test class for FreeCAD material models
    """
    def setUp(self):
        """ Setup function to initialize test data """
        self.ModelManager = Materials.ModelManager()
        self.uuids = Materials.UUIDs()

    def testModelManager(self):
        """ Ensure we can access ModelManager member functions """
        self.assertIn("ModelLibraries", dir(self.ModelManager))
        self.assertIn("Models", dir(self.ModelManager))

    def testUUIDs(self):
        """ Verify the common UUIDs are defined and correct """
        self.assertTrue(self.uuids.Father, "9cdda8b6-b606-4778-8f13-3934d8668e67")
        self.assertTrue(self.uuids.MaterialStandard, "1e2c0088-904a-4537-925f-64064c07d700")

        self.assertTrue(self.uuids.ArrudaBoyce, "e10d00de-c7de-4e59-bcdd-058c2ea19ec6")
        self.assertTrue(self.uuids.Density, "454661e5-265b-4320-8e6f-fcf6223ac3af")
        self.assertTrue(self.uuids.Hardness, "3d1a6141-d032-4d82-8bb5-a8f339fff8ad")
        self.assertTrue(self.uuids.IsotropicLinearElastic, "f6f9e48c-b116-4e82-ad7f-3659a9219c50")
        self.assertTrue(self.uuids.LinearElastic,"7b561d1d-fb9b-44f6-9da9-56a4f74d7536")
        self.assertTrue(self.uuids.MooneyRivlin, "beeed169-7770-4da0-ab67-c9172cf7d23d")
        self.assertTrue(self.uuids.NeoHooke, "569ebc58-ef29-434a-83be-555a0980d505")
        self.assertTrue(self.uuids.OgdenN1, "a2634a2c-412f-468d-9bec-74ae5d87a9c0")
        self.assertTrue(self.uuids.OgdenN2, "233540bb-7b13-4f49-ac12-126a5c82cedf")
        self.assertTrue(self.uuids.OgdenN3, "a917d6b8-209f-429e-9972-fe4bbb97af3f")
        self.assertTrue(self.uuids.OgdenYld2004p18, "3ef9e427-cc25-43f7-817f-79ff0d49625f")
        self.assertTrue(self.uuids.OrthotropicLinearElastic, "b19ccc6b-a431-418e-91c2-0ac8c649d146")
        self.assertTrue(self.uuids.PolynomialN1, "285a6042-0f0c-4a36-a898-4afadd6408ce")
        self.assertTrue(self.uuids.PolynomialN2, "4c2fb7b2-5121-4d6f-be0d-8c5970c9e682")
        self.assertTrue(self.uuids.PolynomialN3, "e83ada22-947e-4beb-91e7-482a16f5ba77")
        self.assertTrue(self.uuids.ReducedPolynomialN1, "f8052a3c-db17-42ea-b2be-13aa5ef30730")
        self.assertTrue(self.uuids.ReducedPolynomialN2, "c52b5021-4bb8-441c-80d4-855fce9de15e")
        self.assertTrue(self.uuids.ReducedPolynomialN3, "fa4e58b4-74c7-4292-8e79-7d5fd232fb55")
        self.assertTrue(self.uuids.Yeoh, "cd13c492-21a9-4578-8191-deec003e4c01")

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
        """ Test that the Density model has been loaded correctly """
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

    def testTestModelCompleteness(self):
        """ Test that the Test model has been loaded correctly """
        model = self.ModelManager.getModel(self.uuids.TestModel)
        self.assertIsNotNone(model)
        self.assertEqual(model.Name, "Test Model")
        self.assertEqual(model.UUID, "34d0583d-f999-49ba-99e6-aa40bd5c3a6b")
        self.assertIn("TestString", model.Properties)
        self.assertEqual(len(model.Properties), 17)
        prop = model.Properties["TestString"]
        self.assertIn("Description", dir(prop))
        self.assertIn("Name", dir(prop))
        self.assertIn("Type", dir(prop))
        self.assertIn("URL", dir(prop))
        self.assertIn("Units", dir(prop))
        self.assertEqual(prop.Name, "TestString")
        self.assertEqual(prop.Type, "String")
        self.assertEqual(prop.URL, "")
        self.assertEqual(prop.Units, "")
        self.assertEqual(prop.Description, "A String")
        prop = model.Properties["TestURL"]
        self.assertEqual(prop.Name, "TestURL")
        self.assertEqual(prop.Type, "URL")
        self.assertEqual(prop.URL, "")
        self.assertEqual(prop.Units, "")
        self.assertEqual(prop.Description, "A URL")
        prop = model.Properties["TestList"]
        self.assertEqual(prop.Name, "TestList")
        self.assertEqual(prop.Type, "List")
        self.assertEqual(prop.URL, "")
        self.assertEqual(prop.Units, "")
        self.assertEqual(prop.Description, "A List")
        prop = model.Properties["TestFileList"]
        self.assertEqual(prop.Name, "TestFileList")
        self.assertEqual(prop.Type, "FileList")
        self.assertEqual(prop.URL, "")
        self.assertEqual(prop.Units, "")
        self.assertEqual(prop.Description, "A List of file paths")
        prop = model.Properties["TestImageList"]
        self.assertEqual(prop.Name, "TestImageList")
        self.assertEqual(prop.Type, "ImageList")
        self.assertEqual(prop.URL, "")
        self.assertEqual(prop.Units, "")
        self.assertEqual(prop.Description, "A List of embedded images")
        prop = model.Properties["TestInteger"]
        self.assertEqual(prop.Name, "TestInteger")
        self.assertEqual(prop.Type, "Integer")
        self.assertEqual(prop.URL, "")
        self.assertEqual(prop.Units, "")
        self.assertEqual(prop.Description, "A Integer")
        prop = model.Properties["TestFloat"]
        self.assertEqual(prop.Name, "TestFloat")
        self.assertEqual(prop.Type, "Float")
        self.assertEqual(prop.URL, "")
        self.assertEqual(prop.Units, "")
        self.assertEqual(prop.Description, "A Float")
        prop = model.Properties["TestBoolean"]
        self.assertEqual(prop.Name, "TestBoolean")
        self.assertEqual(prop.Type, "Boolean")
        self.assertEqual(prop.URL, "")
        self.assertEqual(prop.Units, "")
        self.assertEqual(prop.Description, "A Boolean")
        prop = model.Properties["TestColor"]
        self.assertEqual(prop.Name, "TestColor")
        self.assertEqual(prop.Type, "Color")
        self.assertEqual(prop.URL, "")
        self.assertEqual(prop.Units, "")
        self.assertEqual(prop.Description, "A Color")
        prop = model.Properties["TestFile"]
        self.assertEqual(prop.Name, "TestFile")
        self.assertEqual(prop.Type, "File")
        self.assertEqual(prop.URL, "")
        self.assertEqual(prop.Units, "")
        self.assertEqual(prop.Description, "A File")
        prop = model.Properties["TestSVG"]
        self.assertEqual(prop.Name, "TestSVG")
        self.assertEqual(prop.Type, "SVG")
        self.assertEqual(prop.URL, "")
        self.assertEqual(prop.Units, "")
        self.assertEqual(prop.Description, "An SVG")
        prop = model.Properties["TestImage"]
        self.assertEqual(prop.Name, "TestImage")
        self.assertEqual(prop.Type, "Image")
        self.assertEqual(prop.URL, "")
        self.assertEqual(prop.Units, "")
        self.assertEqual(prop.Description, "An Image")
        prop = model.Properties["TestQuantity"]
        self.assertEqual(prop.Name, "TestQuantity")
        self.assertEqual(prop.Type, "Quantity")
        self.assertEqual(prop.URL, "")
        self.assertEqual(prop.Units, "kg/m^3")
        self.assertEqual(prop.Description, "A Quantity")
        prop = model.Properties["TestMultiLineString"]
        self.assertEqual(prop.Name, "TestMultiLineString")
        self.assertEqual(prop.Type, "MultiLineString")
        self.assertEqual(prop.URL, "")
        self.assertEqual(prop.Units, "")
        self.assertEqual(prop.Description, "A string that spans multiple lines")

        prop = model.Properties["TestArray2D"]
        self.assertEqual(prop.Name, "TestArray2D")
        self.assertEqual(prop.Type, "2DArray")
        self.assertEqual(prop.URL, "")
        self.assertEqual(prop.Units, "")
        self.assertEqual(prop.Description, "2 Dimensional array showing density with temperature\n")
        self.assertEqual(len(prop.Columns), 2)
        col = prop.Columns[0]
        self.assertIn("Description", dir(col))
        self.assertIn("Name", dir(col))
        self.assertIn("Type", dir(col))
        self.assertIn("URL", dir(col))
        self.assertIn("Units", dir(col))
        self.assertEqual(col.Name, "Temperature")
        self.assertEqual(col.Type, "Quantity")
        self.assertEqual(col.URL, "")
        self.assertEqual(col.Units, "C")
        self.assertEqual(col.Description, "Temperature")
        col = prop.Columns[1]
        self.assertEqual(col.Name, "Density")
        self.assertEqual(col.Type, "Quantity")
        self.assertEqual(col.URL, "https://en.wikipedia.org/wiki/Density")
        self.assertEqual(col.Units, "kg/m^3")
        self.assertEqual(col.Description, "Density in [FreeCAD Density unit]")

        prop = model.Properties["TestArray2D3Column"]
        self.assertEqual(prop.Name, "TestArray2D3Column")
        self.assertEqual(prop.Type, "2DArray")
        self.assertEqual(prop.URL, "")
        self.assertEqual(prop.Units, "")
        self.assertEqual(prop.Description, "2 Dimensional array showing density and initial yield stress with temperature\n")
        self.assertEqual(len(prop.Columns), 3)
        col = prop.Columns[0]
        self.assertEqual(col.Name, "Temperature")
        self.assertEqual(col.Type, "Quantity")
        self.assertEqual(col.URL, "")
        self.assertEqual(col.Units, "C")
        self.assertEqual(col.Description, "Temperature")
        col = prop.Columns[1]
        self.assertEqual(col.Name, "Density")
        self.assertEqual(col.Type, "Quantity")
        self.assertEqual(col.URL, "https://en.wikipedia.org/wiki/Density")
        self.assertEqual(col.Units, "kg/m^3")
        self.assertEqual(col.Description, "Density in [FreeCAD Density unit]")
        col = prop.Columns[2]
        self.assertEqual(col.Name, "InitialYieldStress")
        self.assertEqual(col.Type, "Quantity")
        self.assertEqual(col.URL, "")
        self.assertEqual(col.Units, "kPa")
        self.assertEqual(col.Description, "Saturation stress for Voce isotropic hardening [FreeCAD Pressure unit]\n")

        prop = model.Properties["TestArray3D"]
        self.assertEqual(prop.Name, "TestArray3D")
        self.assertEqual(prop.Type, "3DArray")
        self.assertEqual(prop.URL, "")
        self.assertEqual(prop.Units, "")
        self.assertEqual(prop.Description, "3 Dimensional array showing stress and strain as a function of temperature\n")
        self.assertEqual(len(prop.Columns), 3)
        col = prop.Columns[0]
        self.assertEqual(col.Name, "Temperature")
        self.assertEqual(col.Type, "Quantity")
        self.assertEqual(col.URL, "")
        self.assertEqual(col.Units, "C")
        self.assertEqual(col.Description, "Temperature")
        col = prop.Columns[1]
        self.assertEqual(col.Name, "Stress")
        self.assertEqual(col.Type, "Quantity")
        self.assertEqual(col.URL, "")
        self.assertEqual(col.Units, "MPa")
        self.assertEqual(col.Description, "Stress")
        col = prop.Columns[2]
        self.assertEqual(col.Name, "Strain")
        self.assertEqual(col.Type, "Quantity")
        self.assertEqual(col.URL, "")
        self.assertEqual(col.Units, "MPa")
        self.assertEqual(col.Description, "Strain")

    def testModelInheritance(self):
        """ Test that the inherited models have been loaded correctly """
        model = self.ModelManager.getModel(self.uuids.LinearElastic)
        self.assertIsNotNone(model)
        self.assertEqual(model.Name, "Linear Elastic")
        self.assertEqual(model.UUID, "7b561d1d-fb9b-44f6-9da9-56a4f74d7536")
        self.assertIn("Density", model.Properties)
        prop = model.Properties["Density"]
        self.assertEqual(prop.Name, "Density")
        self.assertEqual(prop.Type, "Quantity")
        self.assertEqual(prop.URL, "https://en.wikipedia.org/wiki/Density")
        self.assertEqual(prop.Units, "kg/m^3")
        self.assertEqual(prop.Description, "Density in [FreeCAD Density unit]")
        prop = model.Properties["BulkModulus"]
        self.assertEqual(prop.Name, "BulkModulus")
        self.assertEqual(prop.DisplayName, "Bulk Modulus")
        self.assertEqual(prop.Type, "Quantity")
        self.assertEqual(prop.URL, "https://en.wikipedia.org/wiki/Bulk_modulus")
        self.assertEqual(prop.Units, "kPa")
        self.assertEqual(prop.Description, "Bulk modulus in [FreeCAD Pressure unit]")
        prop = model.Properties["PoissonRatio"]
        self.assertEqual(prop.Name, "PoissonRatio")
        self.assertEqual(prop.DisplayName, "Poisson Ratio")
        self.assertEqual(prop.Type, "Float")
        self.assertEqual(prop.URL, "https://en.wikipedia.org/wiki/Poisson%27s_ratio")
        self.assertEqual(prop.Units, "")
        self.assertEqual(prop.Description, "Poisson's ratio [unitless]")
        prop = model.Properties["ShearModulus"]
        self.assertEqual(prop.Name, "ShearModulus")
        self.assertEqual(prop.DisplayName, "Shear Modulus")
        self.assertEqual(prop.Type, "Quantity")
        self.assertEqual(prop.URL, "https://en.wikipedia.org/wiki/Shear_modulus")
        self.assertEqual(prop.Units, "kPa")
        self.assertEqual(prop.Description, "Shear modulus in [FreeCAD Pressure unit]")
        prop = model.Properties["YoungsModulus"]
        self.assertEqual(prop.Name, "YoungsModulus")
        self.assertEqual(prop.DisplayName, "Young's Modulus")
        self.assertEqual(prop.Type, "Quantity")
        self.assertEqual(prop.URL, "https://en.wikipedia.org/wiki/Young%27s_modulus")
        self.assertEqual(prop.Units, "kPa")
        self.assertEqual(prop.Description, "Young's modulus (or E-Module) in [FreeCAD Pressure unit]")