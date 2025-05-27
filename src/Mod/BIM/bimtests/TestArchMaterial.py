# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2025 Furgo                                              *
# *                                                                         *
# *   This file is part of FreeCAD.                                         *
# *                                                                         *
# *   FreeCAD is free software: you can redistribute it and/or modify it    *
# *   under the terms of the GNU Lesser General Public License as           *
# *   published by the Free Software Foundation, either version 2.1 of the  *
# *   License, or (at your option) any later version.                       *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful, but        *
# *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
# *   Lesser General Public License for more details.                       *
# *                                                                         *
# *   You should have received a copy of the GNU Lesser General Public      *
# *   License along with FreeCAD. If not, see                               *
# *   <https://www.gnu.org/licenses/>.                                      *
# *                                                                         *
# ***************************************************************************

import Arch
from bimtests import TestArchBase

class TestArchMaterial(TestArchBase.TestArchBase):

    def test_makeMaterial(self):
        """Test the makeMaterial function."""
        operation = "Testing makeMaterial function"
        self.printTestMessage(operation)

        material = Arch.makeMaterial(name="TestMaterial")
        self.assertIsNotNone(material, "makeMaterial failed to create a material object.")
        self.assertEqual(material.Label, "TestMaterial", "Material label is incorrect.")

    def test_makeMultiMaterial(self):
        """Test the makeMultiMaterial function."""
        operation = "Testing makeMultiMaterial function"
        self.printTestMessage(operation)

        multi_material = Arch.makeMultiMaterial(name="TestMultiMaterial")
        self.assertIsNotNone(multi_material, "makeMultiMaterial failed to create a multi-material object.")
        self.assertEqual(multi_material.Label, "TestMultiMaterial", "Multi-material label is incorrect.")

    def test_getMaterialContainer(self):
        """Test the getMaterialContainer function."""
        operation = "Testing getMaterialContainer function"
        self.printTestMessage(operation)

        container = Arch.getMaterialContainer()
        self.assertIsNotNone(container, "getMaterialContainer failed to retrieve or create a material container.")
        self.assertEqual(container.Label, "Materials", "Material container label is incorrect.")

    def test_getDocumentMaterials(self):
        """Test the getDocumentMaterials function."""
        operation = "Testing getDocumentMaterials function"
        self.printTestMessage(operation)

        materials = Arch.getDocumentMaterials()
        self.assertIsInstance(materials, list, "getDocumentMaterials did not return a list.")