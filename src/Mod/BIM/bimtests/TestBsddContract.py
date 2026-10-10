# SPDX-License-Identifier: LGPL-2.1-or-later
#
# ***************************************************************************
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

import unittest

import BimBsddContract


class _MockObject:
    IfcClass = "IfcWall"


class TestBsddContract(unittest.TestCase):

    def test_builds_canonical_contract(self):
        payload = {
            "dictionaryName": "IFC",
            "dictionaryUri": "https://identifier.buildingsmart.org/uri/buildingsmart/ifc/latest",
            "dictionaryVersion": "4.3",
            "referenceCode": "IfcWall",
            "name": "Wall",
            "description": "A wall element.",
            "uri": "https://identifier.buildingsmart.org/uri/buildingsmart/ifc/latest/class/IfcWall",
            "relatedIfcEntities": ["IfcWall"],
            "classProperties": [
                {
                    "propertySet": "Pset_WallCommon",
                    "name": "IsExternal",
                    "dataType": "Boolean",
                    "predefinedValue": "TRUE",
                }
            ],
        }

        contract = BimBsddContract.build_canonical_contract(payload, active_object=_MockObject())

        self.assertEqual(contract["dictionary_metadata"]["name"], "IFC")
        self.assertEqual(
            contract["dictionary_metadata"]["namespace_uri"],
            "https://identifier.buildingsmart.org/uri/buildingsmart/ifc/latest",
        )
        self.assertEqual(contract["class_metadata"]["reference_code"], "IfcWall")
        self.assertEqual(contract["class_metadata"]["name"], "Wall")
        self.assertTrue(contract["validation_filters"]["is_applicable"])
        self.assertEqual(contract["validation_filters"]["active_ifc_type"], "IfcWall")
        self.assertEqual(contract["extended_properties"][0]["property_set"], "Pset_WallCommon")

    def test_merge_payloads_prefers_detail_properties(self):
        base_payload = {
            "referenceCode": "IfcWall",
            "name": "Wall",
            "classProperties": [{"name": "BaseProp"}],
        }
        detail_payload = {
            "description": "Detailed wall",
            "classProperties": [{"name": "DetailProp"}],
        }

        merged = BimBsddContract.merge_payloads(base_payload, detail_payload)

        self.assertEqual(merged["referenceCode"], "IfcWall")
        self.assertEqual(merged["description"], "Detailed wall")
        self.assertEqual(merged["classProperties"][0]["name"], "DetailProp")

    def test_builds_legacy_classification_string(self):
        contract = {
            "dictionary_metadata": {"name": "IFC"},
            "class_metadata": {"reference_code": "IfcWall", "name": "Wall"},
        }
        self.assertEqual(
            BimBsddContract.build_legacy_classification_string(contract), "IFC IfcWall"
        )

    def test_refreshes_validation_for_target_object(self):
        class SlabObject:
            IfcClass = "IfcSlab"

        payload = {
            "referenceCode": "IfcWall",
            "name": "Wall",
            "relatedIfcEntities": ["IfcWall"],
        }
        contract = BimBsddContract.build_canonical_contract(payload, active_object=_MockObject())
        self.assertTrue(contract["validation_filters"]["is_applicable"])

        refreshed = BimBsddContract.refresh_contract_validation(contract, SlabObject())

        self.assertEqual(refreshed["validation_filters"]["active_ifc_type"], "IfcSlab")
        self.assertFalse(refreshed["validation_filters"]["is_applicable"])
        self.assertTrue(contract["validation_filters"]["is_applicable"])

    def test_extract_applicable_ifc_types_collects_scalar_and_list_keys(self):
        payload = {
            "relatedIfcEntity": "IfcWall",
            "relatedIfcEntities": ["IfcSlab", ""],
            "applicableIfcClasses": ("IfcBeam", None),
        }

        values = BimBsddContract._extract_applicable_ifc_types(payload)

        self.assertEqual(values, ["IfcSlab", "IfcWall", "IfcBeam"])

    def test_extract_property_value_prefers_predefined_then_default(self):
        prop = {"predefinedValue": "TRUE", "defaultValue": "FALSE"}
        self.assertEqual(BimBsddContract._extract_property_value(prop), "TRUE")

        prop = {"defaultValue": "FALSE", "exampleValue": "TRUE"}
        self.assertEqual(BimBsddContract._extract_property_value(prop), "FALSE")

    def test_missing_property_value_does_not_fall_back_to_allowed_enum(self):
        payload = {
            "referenceCode": "IfcWall",
            "name": "Wall",
            "classProperties": [
                {
                    "propertySet": "Pset_WallCommon",
                    "name": "Status",
                    "dataType": "String",
                    "allowedValues": [{"value": "NEW"}, {"value": "EXISTING"}],
                }
            ],
        }

        contract = BimBsddContract.build_canonical_contract(payload, active_object=_MockObject())

        self.assertEqual(contract["extended_properties"][0]["name"], "Status")
        self.assertIsNone(contract["extended_properties"][0]["value"])
