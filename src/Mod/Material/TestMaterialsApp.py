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
        self.ModelManager = Material.ModelManager.getManager()

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
