# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2013 Yorik van Havre <yorik@uncreated.net>              *
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

import FreeCAD as App
import Arch
from bimtests import TestArchBase

class TestArchBuildingPart(TestArchBase.TestArchBase):

    def testMakeFloorEmpty(self):
        floor = Arch.makeFloor()
        self.assertIsNotNone(floor, "Failed to create an empty floor")

    def testMakeFloorWithObjects(self):
        obj = App.ActiveDocument.addObject("Part::Box", "Box")
        floor = Arch.makeFloor([obj])
        self.assertIn(obj, floor.Group, "Object not added to the floor")

    def testFloorProperties(self):
        floor = Arch.makeFloor()
        self.assertEqual(floor.Label, "Level", "Default label is incorrect")

    def testFloor(self):
        App.Console.PrintLog ('Checking Arch Floor...\n')
        structure = Arch.makeStructure(length=2, width=3, height=5)
        floor = Arch.makeFloor([structure])
        self.assertTrue(floor,"Arch Floor failed")

    def testBuilding(self):
        App.Console.PrintLog ('Checking Arch Building...\n')
        structure = Arch.makeStructure(length=2, width=3, height=5)
        floor = Arch.makeFloor([structure])
        building = Arch.makeBuilding([floor])
        self.assertTrue(building, "Arch Building failed")

    def testSite(self):
        App.Console.PrintLog('Checking Arch Site...\n')
        structure = Arch.makeStructure(length=2, width=3, height=5)
        floor = Arch.makeFloor([structure])
        building = Arch.makeBuilding([floor])
        site = Arch.makeSite([building])
        self.assertTrue(site, "Arch Site failed")

    def test_makeBuildingPart(self):
        """Test the makeBuildingPart function."""
        operation = "Testing makeBuildingPart function"
        self.printTestMessage(operation)

        part = Arch.makeBuildingPart(name="TestBuildingPart")
        self.assertIsNotNone(part, "makeBuildingPart failed to create a building part.")
        self.assertEqual(part.Label, "TestBuildingPart", "Building part label is incorrect.")

    def test_makeFloor(self):
        """Test the makeFloor function."""
        operation = "Testing makeFloor function"
        self.printTestMessage(operation)

        floor = Arch.makeFloor(name="TestFloor")
        self.assertIsNotNone(floor, "makeFloor failed to create a floor object.")
        self.assertEqual(floor.Label, "TestFloor", "Floor label is incorrect.")

    def test_makeBuilding(self):
        """Test the makeBuilding function."""
        operation = "Testing makeBuilding function"
        self.printTestMessage(operation)

        building = Arch.makeBuilding(name="TestBuilding")
        self.assertIsNotNone(building, "makeBuilding failed to create a building object.")
        self.assertEqual(building.Label, "TestBuilding", "Building label is incorrect.")
    
    def test_convertFloors(self):
        """Test the convertFloors function."""
        operation = "Testing convertFloors..."
        self.printTestMessage(operation)

        # Create a mock floor object
        floor = Arch.makeFloor()
        Arch.convertFloors(floor)
        self.assertEqual(floor.IfcType, "Building Storey", "convertFloors failed to set IfcType correctly")

    def test_make2DDrawing(self):
        """Test the make2DDrawing function."""
        operation = "Testing make2DDrawing..."
        self.printTestMessage(operation)

        obj = Arch.make2DDrawing()
        self.assertIsNotNone(obj, "make2DDrawing failed to create an object")
        self.assertEqual(obj.Label, "Drawing", "Incorrect default label for 2D Drawing")
    