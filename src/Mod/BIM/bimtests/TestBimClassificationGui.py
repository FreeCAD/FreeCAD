# SPDX-License-Identifier: LGPL-2.1-or-later
#
# Copyright (c) 2026 Furgo
#
# This file is part of FreeCAD.
# You can find the full license text in the LICENSE file in the root directory.

import importlib
import sys
import types
from unittest.mock import patch

import FreeCAD
import FreeCADGui
from PySide import QtCore

from bimtests import TestArchBaseGui


def _ensure_stub_module(name):
    module = sys.modules.get(name)
    if module is None:
        module = types.ModuleType(name)
        sys.modules[name] = module
    return module


_bsdd_stub = _ensure_stub_module("BimBsdd")
_contract_stub = _ensure_stub_module("BimBsddContract")


def _build_canonical_contract(concept, detail_payload=None, active_object=None):
    merged_payload = dict(concept or {})
    if isinstance(detail_payload, dict):
        merged_payload.update(detail_payload)
    related_entities = merged_payload.get("relatedIfcEntities") or []
    if not related_entities and merged_payload.get("relatedIfcEntity"):
        related_entities = [merged_payload.get("relatedIfcEntity")]
    return {
        "class_metadata": {
            "name": merged_payload.get("name"),
            "reference_code": merged_payload.get("referenceCode"),
            "uri": merged_payload.get("uri"),
            "related_ifc_entities": related_entities,
        },
        "dictionary_metadata": {
            "name": merged_payload.get("dictionaryName") or "Test Dictionary",
            "uri": merged_payload.get("dictionaryUri") or "",
        },
        "extended_properties": list((detail_payload or {}).get("classProperties", [])),
        "active_object_name": getattr(active_object, "Name", "") if active_object else "",
    }


def _build_legacy_classification_string(contract):
    class_metadata = contract.get("class_metadata", {})
    dictionary_metadata = contract.get("dictionary_metadata", {})
    prefix = dictionary_metadata.get("name") or "bSDD"
    code = class_metadata.get("reference_code") or class_metadata.get("name") or ""
    return "{} {}".format(prefix, code).strip()


def _refresh_contract_validation(contract, obj):
    refreshed = dict(contract or {})
    refreshed["validation"] = {"is_valid": True, "object_name": getattr(obj, "Name", "")}
    return refreshed


if not hasattr(_contract_stub, "build_canonical_contract"):
    _contract_stub.build_canonical_contract = _build_canonical_contract
if not hasattr(_contract_stub, "build_legacy_classification_string"):
    _contract_stub.build_legacy_classification_string = _build_legacy_classification_string
if not hasattr(_contract_stub, "refresh_contract_validation"):
    _contract_stub.refresh_contract_validation = _refresh_contract_validation
if not hasattr(_bsdd_stub, "get_bsdd_network_client"):
    _bsdd_stub.get_bsdd_network_client = lambda: None


BimClassification = importlib.import_module("bimcommands.BimClassification")


class FakeBsddClient(QtCore.QObject):
    dictionariesReady = QtCore.Signal(object)
    searchReady = QtCore.Signal(object, object)
    conceptReady = QtCore.Signal(str, object)
    requestFailed = QtCore.Signal(str, str, object)

    def __init__(self):
        super().__init__()
        self.fetch_dictionaries_calls = 0
        self.search_calls = []
        self.fetch_concept_calls = []

    def fetch_dictionaries(self):
        self.fetch_dictionaries_calls += 1

    def _search_cache_key(self, query_text, active_dictionaries, related_ifc_entity):
        return (
            query_text or "",
            tuple(active_dictionaries or ()),
            related_ifc_entity or "",
        )

    def search_concepts(self, query_text, active_dictionaries=None, related_ifc_entity=""):
        call = {
            "query_text": query_text,
            "active_dictionaries": tuple(active_dictionaries or ()),
            "related_ifc_entity": related_ifc_entity or "",
        }
        self.search_calls.append(call)
        return self._search_cache_key(
            query_text,
            tuple(active_dictionaries or ()),
            related_ifc_entity,
        )

    def fetch_concept(self, concept_uri):
        self.fetch_concept_calls.append(concept_uri)
        return concept_uri


class TestBimClassificationGui(TestArchBaseGui.TestArchBaseGui):
    def setUp(self):
        super().setUp()
        self.params = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/BIM")
        self._saved_provider = self.params.GetString(BimClassification.BSDD_PROVIDER_MODE_KEY, "")
        self._saved_visible = self.params.GetInt("BimClassificationVisibleState", 0)
        self._saved_prefix = self.params.GetInt("BimClassificationSystemNamePrefix", 1)
        self.params.SetString(BimClassification.BSDD_PROVIDER_MODE_KEY, "Legacy")
        self.params.SetInt("BimClassificationVisibleState", 0)
        self.params.SetInt("BimClassificationSystemNamePrefix", 1)

        self.obj = self.document.addObject("Part::Feature", "BsddTarget")
        self.obj.Label = "bSDD Target"
        self.obj.addProperty("App::PropertyString", "StandardCode", "BIM")
        self.obj.StandardCode = "Legacy OLD-01"
        self.obj.addProperty("App::PropertyString", "IfcClass", "IFC")
        self.obj.IfcClass = "IfcWall"
        self.document.recompute()

        FreeCADGui.Selection.clearSelection()
        self.fake_client = FakeBsddClient()
        self.client_patch = patch.object(
            BimClassification.BimBsdd,
            "get_bsdd_network_client",
            return_value=self.fake_client,
        )
        self.client_patch.start()

        self.cmd = BimClassification.BIM_Classification()
        self.cmd.Activated()
        self.cmd._search_timer.stop()

    def tearDown(self):
        try:
            if getattr(self, "cmd", None) and getattr(self.cmd, "form", None):
                self.cmd.reject()
        except Exception:
            pass
        try:
            self.client_patch.stop()
        except Exception:
            pass
        FreeCADGui.Selection.clearSelection()
        self.params.SetString(BimClassification.BSDD_PROVIDER_MODE_KEY, self._saved_provider)
        self.params.SetInt("BimClassificationVisibleState", self._saved_visible)
        self.params.SetInt("BimClassificationSystemNamePrefix", self._saved_prefix)
        super().tearDown()

    def _activate_bsdd_provider(self):
        if self.cmd.form.comboProvider.currentText() != "bSDD":
            self.cmd.form.comboProvider.setCurrentText("bSDD")
        self.cmd._search_timer.stop()

    def _load_dictionaries(self, dictionaries, active_uris=None):
        self.cmd._has_saved_bsdd_dictionary_state = active_uris is not None
        self.cmd._restored_bsdd_dictionary_uris = set(active_uris or [])
        self.cmd._on_bsdd_dictionaries_ready({"dictionaries": dictionaries})
        self.cmd._search_timer.stop()

    def _set_dictionary_checked(self, uri, checked=True):
        for row in range(self.cmd.form.bsddDictionaryList.count()):
            item = self.cmd.form.bsddDictionaryList.item(row)
            if item.data(QtCore.Qt.UserRole) == uri:
                item.setCheckState(QtCore.Qt.Checked if checked else QtCore.Qt.Unchecked)
                break
        self.cmd._search_timer.stop()

    def _get_object_tree_item(self):
        items = self.cmd.form.treeObjects.findItems(
            self.obj.Label,
            QtCore.Qt.MatchExactly | QtCore.Qt.MatchRecursive,
            0,
        )
        self.assertTrue(items, "Could not find the BIM test object in the classification tree.")
        item = items[0]
        self.cmd.form.treeObjects.setCurrentItem(item)
        item.setSelected(True)
        return item

    def _append_concept_result(self, dictionary_uri, concept):
        self.cmd._initialize_bsdd_result_tree([dictionary_uri])
        self.cmd._append_bsdd_dictionary_results(dictionary_uri, [concept])
        return self.cmd._bsdd_dictionary_nodes[dictionary_uri].child(0)

    def _row_contains_property(self, label):
        for row in range(self.cmd.form.bsddPropertyTable.rowCount()):
            item = self.cmd.form.bsddPropertyTable.item(row, 0)
            if item and item.text() == label:
                return True
        return False

    def test_activated_initializes_bsdd_widgets_and_requests_dictionary_load(self):
        self.assertIs(self.cmd._bsdd_client, self.fake_client)
        self.assertEqual(self.fake_client.fetch_dictionaries_calls, 1)
        self.assertTrue(self.cmd._bsdd_signals_connected)
        self.assertEqual(self.cmd.form.comboProvider.currentText(), "Legacy")
        self.assertFalse(self.cmd.form.bsddPanel.isVisible())
        self.assertTrue(hasattr(self.cmd.form, "bsddResultsTree"))
        self.assertTrue(hasattr(self.cmd.form, "bsddPropertyTable"))

    def test_provider_switch_shows_bsdd_panel_and_refreshes_requests(self):
        self._activate_bsdd_provider()

        self.assertTrue(self.cmd._is_bsdd_provider_active())
        self.assertTrue(self.cmd.form.bsddPanel.isVisible())
        self.assertFalse(self.cmd.form.comboSystem.isVisible())
        self.assertEqual(
            self.cmd.form.groupClasses.title(),
            "buildingSMART Data Dictionary",
        )
        self.assertEqual(self.fake_client.fetch_dictionaries_calls, 2)
        self.assertEqual(
            self.params.GetString(BimClassification.BSDD_PROVIDER_MODE_KEY, ""),
            "bSDD",
        )

    def test_dictionaries_ready_restores_active_selection_and_document_meta(self):
        self._activate_bsdd_provider()
        dictionaries = [
            {"name": "OmniClass", "uri": "dict://omni", "status": "Active"},
            {"name": "Uniclass", "uri": "dict://uniclass", "status": "Preview"},
        ]

        self._load_dictionaries(dictionaries, active_uris={"dict://omni"})

        self.assertEqual(self.cmd.form.bsddDictionaryList.count(), 2)
        first_item = self.cmd.form.bsddDictionaryList.item(0)
        second_item = self.cmd.form.bsddDictionaryList.item(1)
        self.assertEqual(first_item.text(), "OmniClass (Active)")
        self.assertEqual(first_item.checkState(), QtCore.Qt.Checked)
        self.assertEqual(second_item.checkState(), QtCore.Qt.Unchecked)
        self.assertEqual(self.document.Meta.get(BimClassification.BSDD_DICTIONARY_META_KEY), "dict://omni")
        self.assertEqual(
            self.document.Meta.get(BimClassification.BSDD_DICTIONARY_META_PRESENT_KEY),
            "1",
        )

    def test_perform_search_without_active_dictionaries_shows_placeholder(self):
        self._activate_bsdd_provider()
        self._load_dictionaries([{"name": "OmniClass", "uri": "dict://omni"}], active_uris=None)
        self.cmd.form.bsddDictionaryToggle.setChecked(False)

        self.cmd._perform_bsdd_search()

        self.assertEqual(self.cmd.form.bsddResultsModel.rowCount(), 1)
        self.assertIn(
            "Select at least one bSDD dictionary",
            self.cmd.form.bsddResultsModel.item(0, 0).text(),
        )
        self.assertTrue(self.cmd.form.bsddDictionaryToggle.isChecked())
        self.assertEqual(len(self.fake_client.search_calls), 0)

    def test_search_dispatches_requests_and_renders_search_results(self):
        self._activate_bsdd_provider()
        self._load_dictionaries([{"name": "OmniClass", "uri": "dict://omni"}], active_uris=None)
        self._set_dictionary_checked("dict://omni", True)
        self.cmd.form.bsddSearch.setText("wall")

        self.cmd._perform_bsdd_search()

        self.assertEqual(len(self.fake_client.search_calls), 1)
        self.assertEqual(self.fake_client.search_calls[0]["query_text"], "wall")
        self.assertEqual(self.fake_client.search_calls[0]["active_dictionaries"], ("dict://omni",))

        concept = {
            "name": "Exterior Wall",
            "referenceCode": "RC-10",
            "dictionaryName": "OmniClass",
            "uri": "concept://wall",
            "relatedIfcEntities": ["IfcWall"],
        }
        search_key = self.fake_client._search_cache_key("wall", ("dict://omni",), "")
        self.cmd._on_bsdd_search_ready(search_key, {"classes": [concept]})

        dictionary_node = self.cmd._bsdd_dictionary_nodes["dict://omni"]
        self.assertEqual(dictionary_node.text(), "OmniClass")
        self.assertEqual(dictionary_node.rowCount(), 1)
        self.assertEqual(dictionary_node.child(0).text(), "Exterior Wall")

    def test_result_selection_fetches_concept_and_updates_properties_with_detail(self):
        self._activate_bsdd_provider()
        concept = {
            "name": "Exterior Wall",
            "referenceCode": "RC-10",
            "dictionaryName": "Test Dictionary",
            "uri": "concept://wall",
            "relatedIfcEntities": ["IfcWall"],
        }
        concept_item = self._append_concept_result("dict://omni", concept)
        self.cmd.form.bsddResultsTree.setCurrentIndex(concept_item.index())
        FreeCADGui.Selection.addSelection(self.document.Name, self.obj.Name)

        self.cmd._on_bsdd_result_changed(concept_item.index(), QtCore.QModelIndex())

        self.assertEqual(self.cmd._bsdd_selected_concept, concept)
        self.assertEqual(self.fake_client.fetch_concept_calls, ["concept://wall"])
        self.assertEqual(self.cmd._bsdd_contract["class_metadata"]["reference_code"], "RC-10")
        self.assertGreaterEqual(self.cmd.form.bsddPropertyTable.rowCount(), 4)

        detail_payload = {
            "name": "Exterior Wall",
            "referenceCode": "RC-10",
            "dictionaryName": "Test Dictionary",
            "uri": "concept://wall",
            "classProperties": [
                {"name": "FireRating", "propertySet": "Pset_WallCommon"},
                {"name": "LoadBearing", "dataType": "Boolean"},
            ],
        }
        self.cmd._on_bsdd_concept_ready("concept://wall", detail_payload)

        self.assertEqual(self.cmd._bsdd_contract_detail_payload, detail_payload)
        self.assertTrue(self._row_contains_property("FireRating"))
        self.assertTrue(self._row_contains_property("LoadBearing"))

    def test_request_failure_marks_dictionary_branch_as_failed(self):
        self._activate_bsdd_provider()
        self.cmd._bsdd_search_batch_id = 4
        self.cmd._initialize_bsdd_result_tree(["dict://omni"])
        cache_key = self.fake_client._search_cache_key("wall", ("dict://omni",), "")
        self.cmd._bsdd_pending_requests[cache_key] = (4, "dict://omni")
        self.cmd._bsdd_dictionary_candidates["dict://omni"] = []

        self.cmd._on_bsdd_request_failed("search", "boom", cache_key)

        node = self.cmd._bsdd_dictionary_nodes["dict://omni"]
        self.assertEqual(node.rowCount(), 1)
        self.assertEqual(node.child(0).text(), "Request failed for this dictionary.")

    def test_apply_and_accept_persist_bsdd_code_on_selected_object(self):
        self._activate_bsdd_provider()
        contract = {
            "class_metadata": {
                "name": "Exterior Wall",
                "reference_code": "RC-10",
            },
            "dictionary_metadata": {
                "name": "Test Dictionary",
            },
            "legacy_string": "Test Dictionary RC-10",
        }
        self.cmd._bsdd_contract = contract
        item = self._get_object_tree_item()

        self.cmd.apply()

        self.assertEqual(item.text(1), "Test Dictionary RC-10")
        self.assertEqual(self.cmd._get_stored_bsdd_contract_for_item(item), contract)

        with patch.object(self.cmd, "_apply_bsdd_contract_to_object", return_value=True) as mock_apply:
            self.cmd.accept()

        self.assertEqual(self.obj.StandardCode, "Test Dictionary RC-10")
        mock_apply.assert_called_once_with(self.obj, contract)
