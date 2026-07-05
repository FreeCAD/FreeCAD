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
from unittest.mock import patch

from nativeifc import ifc_classification


class _MockObject:
    def __init__(self, ifc_class="IfcWall", standard_code=""):
        self.IfcClass = ifc_class
        self.StandardCode = standard_code
        self.Label = "Mock Wall"


class _ExistingClassification:
    def __init__(self, name="", source="", location="", edition=""):
        self.Name = name
        self.Source = source
        self.Location = location
        self.Edition = edition


class _ExistingReference:
    def __init__(
        self,
        identification=None,
        item_reference=None,
        location="",
        referenced_source=None,
    ):
        self.Identification = identification
        self.ItemReference = item_reference
        self.Location = location
        self.ReferencedSource = referenced_source


class _MockIfcFile:
    def __init__(self, classifications=None, references=None, projects=None, relations=None):
        self._classifications = classifications or []
        self._references = references or []
        self._projects = projects or []
        self._relations = relations or []

    def by_type(self, type_name):
        mapping = {
            "IfcClassification": self._classifications,
            "IfcClassificationReference": self._references,
            "IfcProject": self._projects,
            "IfcRelAssociatesClassification": self._relations,
        }
        return mapping.get(type_name, [])


class TestIfcClassification(unittest.TestCase):

    def test_classification_match_requires_namespace_not_name_alone(self):
        classification = _ExistingClassification(
            name="IFC",
            source="https://example.org/uri/ifc/older",
            edition="4.0",
        )
        metadata = {
            "name": "IFC",
            "namespace_uri": "https://identifier.buildingsmart.org/uri/buildingsmart/ifc/latest",
            "version": "4.3",
        }

        self.assertFalse(
            ifc_classification._classification_matches_dictionary(classification, metadata)
        )

    def test_classification_match_allows_name_only_reuse_without_namespace(self):
        classification = _ExistingClassification(name="Uniclass", edition="2015")
        metadata = {"name": "Uniclass", "version": "2015"}

        self.assertTrue(
            ifc_classification._classification_matches_dictionary(classification, metadata)
        )

    def test_coerce_contract_value_handles_boolean_numeric_and_empty(self):
        self.assertTrue(ifc_classification._coerce_contract_value("TRUE", "Boolean"))
        self.assertFalse(ifc_classification._coerce_contract_value("NO", "Boolean"))
        self.assertEqual(ifc_classification._coerce_contract_value("42", "Integer"), 42)
        self.assertEqual(ifc_classification._coerce_contract_value("3.5", "Real"), 3.5)
        self.assertIsNone(ifc_classification._coerce_contract_value("", "String"))
        self.assertEqual(ifc_classification._coerce_contract_value("UNDEFINED", "String"), "")

    def test_find_classification_reference_matches_reference_code(self):
        classification = object()
        reference = _ExistingReference(
            identification="IfcWall",
            referenced_source=classification,
        )
        ifcfile = _MockIfcFile(references=[reference])

        found = ifc_classification._find_classification_reference(
            ifcfile,
            classification,
            {"reference_code": "IfcWall"},
        )

        self.assertIs(found, reference)

    def test_find_classification_reference_matches_uri(self):
        classification = object()
        reference = _ExistingReference(
            location="https://example.org/class/IfcWall",
            referenced_source=classification,
        )
        ifcfile = _MockIfcFile(references=[reference])

        found = ifc_classification._find_classification_reference(
            ifcfile,
            classification,
            {"uri": "https://example.org/class/IfcWall"},
        )

        self.assertIs(found, reference)

    def test_apply_canonical_contract_rejects_inapplicable_ifc_type(self):
        obj = _MockObject(ifc_class="IfcSlab")
        contract = {
            "validation_filters": {"applicable_ifc_types": ["IfcWall"]},
            "dictionary_metadata": {},
            "class_metadata": {},
            "extended_properties": [],
        }

        with self.assertRaises(ValueError):
            ifc_classification.apply_canonical_contract(obj, contract)

    def test_apply_canonical_contract_returns_false_without_ifc_context(self):
        obj = _MockObject()
        contract = {
            "validation_filters": {"applicable_ifc_types": ["IfcWall"]},
            "dictionary_metadata": {},
            "class_metadata": {},
            "extended_properties": [],
        }

        with patch.object(ifc_classification.ifc_tools, "get_ifcfile", return_value=None), patch.object(
            ifc_classification.ifc_tools, "get_ifc_element", return_value=None
        ):
            self.assertFalse(ifc_classification.apply_canonical_contract(obj, contract))

    def test_apply_canonical_contract_writes_standard_code_and_calls_helpers(self):
        obj = _MockObject(ifc_class="IfcWall")
        ifcfile = object()
        element = object()
        classification = object()
        reference = object()
        contract = {
            "validation_filters": {"applicable_ifc_types": ["IfcWall"]},
            "dictionary_metadata": {"name": "IFC"},
            "class_metadata": {"reference_code": "IfcWall"},
            "extended_properties": [],
            "legacy_string": "IFC IfcWall",
        }

        with patch.object(ifc_classification.ifc_tools, "get_ifcfile", return_value=ifcfile), patch.object(
            ifc_classification.ifc_tools, "get_ifc_element", return_value=element
        ), patch.object(
            ifc_classification, "_upsert_classification_root", return_value=classification
        ) as mock_root, patch.object(
            ifc_classification, "_upsert_classification_reference", return_value=reference
        ) as mock_ref, patch.object(
            ifc_classification, "_ensure_reference_relation", return_value=True
        ) as mock_rel, patch.object(
            ifc_classification, "_upsert_extended_properties"
        ) as mock_props:
            result = ifc_classification.apply_canonical_contract(obj, contract)

        self.assertTrue(result)
        self.assertEqual(obj.StandardCode, "IFC IfcWall")
        mock_root.assert_called_once_with(ifcfile, contract["dictionary_metadata"])
        mock_ref.assert_called_once_with(ifcfile, classification, contract["class_metadata"])
        mock_rel.assert_called_once_with(ifcfile, reference, element)
        mock_props.assert_called_once_with(obj, contract, ifcfile, element)

    def test_upsert_extended_properties_skips_empty_values(self):
        obj = _MockObject()
        element = object()
        ifcfile = object()
        contract = {
            "extended_properties": [
                {
                    "property_set": "Pset_WallCommon",
                    "name": "Status",
                    "data_type": "String",
                    "value": "",
                }
            ]
        }

        with patch("nativeifc.ifc_psets.get_pset") as mock_get_pset, patch.object(
            ifc_classification.ifc_tools, "api_run"
        ) as mock_api_run, patch("nativeifc.ifc_psets.show_psets") as mock_show_psets:
            ifc_classification._upsert_extended_properties(obj, contract, ifcfile, element)

        mock_get_pset.assert_not_called()
        mock_api_run.assert_not_called()
        mock_show_psets.assert_called_once_with(obj)

    def test_upsert_extended_properties_creates_and_edits_pset(self):
        obj = _MockObject()
        element = object()
        ifcfile = object()
        existing_pset = object()
        contract = {
            "extended_properties": [
                {
                    "property_set": "Pset_WallCommon",
                    "name": "IsExternal",
                    "data_type": "Boolean",
                    "value": "TRUE",
                }
            ]
        }

        with patch("nativeifc.ifc_psets.get_pset", return_value=None), patch.object(
            ifc_classification.ifc_tools, "api_run", side_effect=[existing_pset, None]
        ) as mock_api_run, patch("nativeifc.ifc_psets.show_psets") as mock_show_psets:
            ifc_classification._upsert_extended_properties(obj, contract, ifcfile, element)

        self.assertEqual(mock_api_run.call_count, 2)
        first_call = mock_api_run.call_args_list[0]
        second_call = mock_api_run.call_args_list[1]
        self.assertEqual(first_call.args[0], "pset.add_pset")
        self.assertEqual(second_call.args[0], "pset.edit_pset")
        self.assertEqual(second_call.kwargs["properties"], {"IsExternal": True})
        mock_show_psets.assert_called_once_with(obj)
