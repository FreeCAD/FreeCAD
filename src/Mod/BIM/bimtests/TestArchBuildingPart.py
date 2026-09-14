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
import ifcopenshell
from bimtests import TestArchBase
from nativeifc import ifc_export
from nativeifc import ifc_tools


class TestArchBuildingPart(TestArchBase.TestArchBase):

    @staticmethod
    def _get_pset(element, pset_name):
        for rel in getattr(element, "IsDefinedBy", []) or []:
            if not rel.is_a("IfcRelDefinesByProperties"):
                continue
            pset = rel.RelatingPropertyDefinition
            if pset and getattr(pset, "Name", None) == pset_name:
                return pset
        return None

    @staticmethod
    def _get_quantity_value(element, quantity_name):
        for rel in getattr(element, "IsDefinedBy", []) or []:
            if not rel.is_a("IfcRelDefinesByProperties"):
                continue
            pset = rel.RelatingPropertyDefinition
            if not pset or not pset.is_a("IfcElementQuantity"):
                continue
            for quantity in getattr(pset, "Quantities", []) or []:
                if quantity.Name != quantity_name:
                    continue
                if hasattr(quantity, "LengthValue"):
                    return quantity.LengthValue
        return None

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
        App.Console.PrintLog("Checking Arch Floor...\n")
        structure = Arch.makeStructure(length=2, width=3, height=5)
        floor = Arch.makeFloor([structure])
        self.assertTrue(floor, "Arch Floor failed")

    def testBuilding(self):
        App.Console.PrintLog("Checking Arch Building...\n")
        structure = Arch.makeStructure(length=2, width=3, height=5)
        floor = Arch.makeFloor([structure])
        building = Arch.makeBuilding([floor])
        self.assertTrue(building, "Arch Building failed")

    def testSite(self):
        App.Console.PrintLog("Checking Arch Site...\n")
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
        self.assertEqual(
            floor.IfcType, "Building Storey", "convertFloors failed to set IfcType correctly"
        )

    def test_nativeifc_aggregate_storey_pset_respects_file_scale(self):
        self.printTestMessage("Testing NativeIFC storey pset restore respects file scale")

        class DummyStorey:
            PropertiesList = ["Height", "LevelOffset"]
            Height = 0
            LevelOffset = 0

            @staticmethod
            def getTypeIdOfProperty(_property_name):
                return "App::PropertyLength"

        pset = type("Pset", (), {})()
        pset.HasProperties = [
            type(
                "Prop",
                (),
                {
                    "Name": "FreeCAD_Height",
                    "NominalValue": type("NominalValue", (), {"wrappedValue": 9.842519685})(),
                },
            )(),
            type(
                "Prop",
                (),
                {
                    "Name": "FreeCAD_LevelOffset",
                    "NominalValue": type("NominalValue", (), {"wrappedValue": 0.4101049869})(),
                },
            )(),
        ]
        floor = DummyStorey()

        self.assertTrue(
            ifc_tools.restore_freecad_property(
                floor,
                object(),
                "Height",
                object(),
                pset=pset,
                scale=304.8,
            )
        )
        self.assertTrue(
            ifc_tools.restore_freecad_property(
                floor,
                object(),
                "LevelOffset",
                object(),
                pset=pset,
                scale=304.8,
            )
        )

        self.assertAlmostEqual(floor.Height, 3000, delta=0.001)
        self.assertAlmostEqual(floor.LevelOffset, 125, delta=0.001)

    def test_nativeifc_aggregate_storey_preserves_level_data(self):
        self.printTestMessage("Testing NativeIFC aggregated storey level data")

        project = ifc_tools.create_document(self.document, silent=True)
        site = ifc_tools.aggregate(Arch.makeSite(), project)
        building = ifc_tools.aggregate(Arch.makeBuilding(), site)

        source_storey = Arch.makeFloor(name="AggregatedLevel")
        source_storey.Height = 3000
        source_storey.LevelOffset = 125
        source_storey.Placement.move(App.Vector(0, 0, 6000))

        storey = ifc_tools.aggregate(source_storey, building)
        self.document.recompute()

        self.assertAlmostEqual(storey.Height.Value, 3000, delta=0.001)
        self.assertAlmostEqual(storey.LevelOffset.Value, 125, delta=0.001)
        self.assertAlmostEqual(storey.Placement.Base.z, 6000, delta=0.001)
        self.assertAlmostEqual(storey.Elevation.Value, 6000, delta=0.001)

        element = project.Proxy.ifcfile[storey.StepId]
        self.assertAlmostEqual(element.Elevation, 6.0, delta=1e-6)
        if getattr(element, "ObjectPlacement", None):
            matrix = ifcopenshell.util.placement.get_local_placement(element.ObjectPlacement)
            self.assertAlmostEqual(matrix[2][3], 6.0, delta=1e-6)

        pset = self._get_pset(element, "FreeCADPropertySet")
        self.assertIsNotNone(pset)
        prop_values = {
            prop.Name: prop.NominalValue.wrappedValue
            for prop in getattr(pset, "HasProperties", []) or []
            if getattr(prop, "NominalValue", None)
        }
        self.assertAlmostEqual(prop_values["FreeCAD_Height"], 3.0, delta=1e-6)
        self.assertAlmostEqual(prop_values["FreeCAD_LevelOffset"], 0.125, delta=1e-6)
        storey.Placement.move(App.Vector(0, 0, 500))
        self.document.recompute()
        self.assertAlmostEqual(storey.Elevation.Value, 6500, delta=0.001)

    def test_strict_ifc_direct_conversion_preserves_level_data(self):
        self.printTestMessage("Testing Strict IFC direct-conversion storey level data")

        source_storey = Arch.makeFloor(name="ConvertedLevel")
        source_storey.Height = 3000
        source_storey.Placement.move(App.Vector(0, 0, 6000))

        load_orphans = ifc_tools.PARAMS.GetBool("LoadOrphans", True)
        try:
            ifc_tools.PARAMS.SetBool("LoadOrphans", True)
            ifc_export.direct_conversion([source_storey], self.document)
            self.document.recompute()
        finally:
            ifc_tools.PARAMS.SetBool("LoadOrphans", load_orphans)

        converted = [
            obj
            for obj in self.document.Objects
            if getattr(obj, "IfcClass", "") == "IfcBuildingStorey"
        ]
        self.assertEqual(len(converted), 1)

        storey = converted[0]
        self.assertAlmostEqual(storey.Height.Value, 3000, delta=0.001)
        self.assertAlmostEqual(storey.Placement.Base.z, 6000, delta=0.001)
        self.assertAlmostEqual(storey.Elevation.Value, 6000, delta=0.001)

        element = self.document.Proxy.ifcfile[storey.StepId]
        self.assertAlmostEqual(element.Elevation, 6.0, delta=1e-6)
        if getattr(element, "ObjectPlacement", None):
            matrix = ifcopenshell.util.placement.get_local_placement(element.ObjectPlacement)
            self.assertAlmostEqual(matrix[2][3], 6.0, delta=1e-6)
        self.assertAlmostEqual(self._get_quantity_value(element, "Height"), 3.0, delta=1e-6)
        storey.Placement.move(App.Vector(0, 0, 500))
        self.document.recompute()
        self.assertAlmostEqual(storey.Elevation.Value, 6500, delta=0.001)

    def test_make2DDrawing(self):
        """Test the make2DDrawing function."""
        operation = "Testing make2DDrawing..."
        self.printTestMessage(operation)

        obj = Arch.make2DDrawing()
        self.assertIsNotNone(obj, "make2DDrawing failed to create an object")
        self.assertEqual(obj.Label, "Drawing", "Incorrect default label for 2D Drawing")

    def test_make2DDrawing_uses_base_object_label(self):
        """Test make2DDrawing default label from a base object."""
        operation = "Testing make2DDrawing label with base object..."
        self.printTestMessage(operation)

        section = Arch.makeSectionPlane(name="Floor Plan Level 01")
        obj = Arch.make2DDrawing(baseobj=section)

        self.assertIsNotNone(obj, "make2DDrawing failed to create an object")
        self.assertEqual(
            obj.Label,
            "Floor Plan Level 01 - Drawing",
            "2D Drawing label should include the base object label",
        )

    def test_make2DDrawing_uses_base_object_name_without_label(self):
        """Test make2DDrawing label fallback when base label is unavailable."""
        operation = "Testing make2DDrawing label fallback..."
        self.printTestMessage(operation)

        base = App.ActiveDocument.addObject("App::DocumentObject", "BaseSection")
        base.Label = ""
        obj = Arch.make2DDrawing(baseobj=base)

        self.assertIsNotNone(obj, "make2DDrawing failed to create an object")
        self.assertEqual(
            obj.Label,
            "BaseSection - Drawing",
            "2D Drawing label should fall back to the base object name",
        )

    def test_make2DDrawing_name_overrides_base_object_label(self):
        """Test explicit make2DDrawing name takes precedence."""
        operation = "Testing make2DDrawing explicit name..."
        self.printTestMessage(operation)

        base = App.ActiveDocument.addObject("App::DocumentObject", "BaseSection")
        base.Label = "Floor Plan Level 01"
        obj = Arch.make2DDrawing(baseobj=base, name="Custom Drawing")

        self.assertIsNotNone(obj, "make2DDrawing failed to create an object")
        self.assertEqual(
            obj.Label,
            "Custom Drawing",
            "Explicit 2D Drawing label should override the base object label",
        )
