# SPDX-License-Identifier: LGPL-2.1-or-later

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
from nativeifc import ifc_classification


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
        self.assertEqual(contract["class_metadata"]["reference_code"], "IfcWall")
        self.assertTrue(contract["validation_filters"]["is_applicable"])
        self.assertEqual(contract["extended_properties"][0]["property_set"], "Pset_WallCommon")

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

    def test_classification_match_requires_namespace_not_name_alone(self):
        class ExistingClassification:
            Name = "IFC"
            Source = "https://example.org/uri/ifc/older"
            Location = ""
            Edition = "4.0"

        metadata = {
            "name": "IFC",
            "namespace_uri": "https://identifier.buildingsmart.org/uri/buildingsmart/ifc/latest",
            "version": "4.3",
        }

        self.assertFalse(
            ifc_classification._classification_matches_dictionary(
                ExistingClassification(), metadata
            )
        )

    def test_classification_match_allows_name_only_reuse_without_namespace(self):
        class ExistingClassification:
            Name = "Uniclass"
            Source = ""
            Location = ""
            Edition = "2015"

        metadata = {
            "name": "Uniclass",
            "version": "2015",
        }

        self.assertTrue(
            ifc_classification._classification_matches_dictionary(
                ExistingClassification(), metadata
            )
        )

    def test_missing_property_value_does_not_fall_back_to_allowed_enum(self):
        self.assertIsNone(
            ifc_classification._coerce_contract_value(
                None, "String", [{"value": "ENUM_A"}, {"value": "ENUM_B"}]
            )
        )
        self.assertIsNone(
            ifc_classification._coerce_contract_value(
                "", "String", [{"value": "ENUM_A"}, {"value": "ENUM_B"}]
            )
        )

    def test_contract_does_not_invent_value_from_allowed_enum(self):
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
