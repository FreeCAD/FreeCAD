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
import os
import json
import os
import unittest

import FreeCAD

import BimBsdd

try:
    from PySide import QtCore, QtNetwork
except ImportError:
    try:
        from PySide6 import QtCore, QtNetwork
    except ImportError:
        from PySide2 import QtCore, QtNetwork


class _FakeReply(QtCore.QObject):
    finished = QtCore.Signal()

    def __init__(self, parent=None):
        super().__init__(parent)


class _FakeNetworkManager:
    def __init__(self):
        self.requests = []

    def get(self, request):
        self.requests.append(request)
        return _FakeReply()


class TestBsdd(unittest.TestCase):

    def setUp(self):
        self.params = FreeCAD.ParamGet(BimBsdd.BSDD_PREFERENCES_PATH)
        self.params.SetBool(BimBsdd.BSDD_PREVIEW_KEY, True)
        self.params.SetBool(BimBsdd.BSDD_TEST_KEY, False)
        self.params.SetBool(BimBsdd.BSDD_INACTIVE_KEY, True)
        self.params.SetString(BimBsdd.BSDD_API_URL_KEY, BimBsdd.BSDD_API_BASE_URL)

    def test_syncs_preferences_on_initialization(self):
        client = BimBsdd.BsddNetworkClient(network_manager=_FakeNetworkManager())
        self.assertTrue(client.settings.include_preview)
        self.assertFalse(client.settings.include_test)
        self.assertTrue(client.settings.include_inactive)

    def test_dictionary_cache_short_circuits_network(self):
        manager = _FakeNetworkManager()
        client = BimBsdd.BsddNetworkClient(network_manager=manager)
        cached_payload = [{"name": "IFC"}]
        received_payloads = []
        client.domain_cache[client._dictionary_cache_key()] = cached_payload
        client.dictionariesReady.connect(received_payloads.append)
        client.fetch_dictionaries()
        self.assertEqual(received_payloads, [cached_payload])
        self.assertEqual(len(manager.requests), 0)

    def test_search_request_includes_status_and_context_filters(self):
        manager = _FakeNetworkManager()
        client = BimBsdd.BsddNetworkClient(network_manager=manager)
        client.search_concepts(
            "wall",
            active_dictionaries=["dict-b", "dict-a"],
            related_ifc_entity="IfcWall",
        )
        self.assertEqual(len(manager.requests), 1)
        query = manager.requests[0].url().query()
        self.assertIn("IncludePreview=true", query)
        self.assertIn("IncludeTestDictionaries=false", query)
        self.assertIn("IncludeInactive=true", query)
        self.assertIn("SearchText=wall", query)
        self.assertIn("DictionaryUris=dict-a", query)
        self.assertIn("DictionaryUris=dict-b", query)
        self.assertIn("RelatedIfcEntities=IfcWall", query)

    def test_settings_change_invalidates_dictionary_and_search_cache_keys(self):
        manager = _FakeNetworkManager()
        client = BimBsdd.BsddNetworkClient(network_manager=manager)

        first_dictionary_key = client._dictionary_cache_key()
        first_search_key = client.search_concepts(
            "wall",
            active_dictionaries=["dict-a"],
            related_ifc_entity="IfcWall",
        )

        self.params.SetBool(BimBsdd.BSDD_TEST_KEY, True)
        client.refresh_settings()

        second_dictionary_key = client._dictionary_cache_key()
        second_search_key = client._search_cache_key("wall", ("dict-a",), "IfcWall")

        self.assertNotEqual(first_dictionary_key, second_dictionary_key)
        self.assertNotEqual(first_search_key, second_search_key)

    def test_fetch_concept_requests_class_and_properties_and_merges_payloads(self):
        manager = _FakeNetworkManager()
        client = BimBsdd.BsddNetworkClient(network_manager=manager)
        received = []
        client.conceptReady.connect(
            lambda concept_uri, payload: received.append((concept_uri, payload))
        )

        concept_uri = "https://example.org/class/IfcWall"
        client.fetch_concept(concept_uri)

        self.assertEqual(len(manager.requests), 2)
        request_urls = [request.url().toString() for request in manager.requests]
        self.assertTrue(any("/api/Class/v1" in url and "Uri=" in url for url in request_urls))
        self.assertTrue(
            any("/api/Class/Properties/v1" in url and "ClassUri=" in url for url in request_urls)
        )

        client._collect_concept_payload(
            concept_uri,
            "base",
            {"referenceCode": "IfcWall", "name": "Wall", "classType": "Class"},
        )
        self.assertEqual(received, [])
        client._collect_concept_payload(
            concept_uri,
            "properties",
            {"classProperties": [{"name": "IsExternal", "propertySet": "Pset_WallCommon"}]},
        )

        self.assertEqual(len(received), 1)
        merged_payload = received[0][1]
        self.assertEqual(received[0][0], concept_uri)
        self.assertEqual(merged_payload.get("referenceCode"), "IfcWall")
        self.assertEqual(merged_payload.get("name"), "Wall")
        self.assertEqual(merged_payload.get("classType"), "Class")
        self.assertTrue(merged_payload.get("classProperties"))
        self.assertEqual(merged_payload["classProperties"][0]["name"], "IsExternal")

    def _run_live_json_request(self, url):
        if QtCore.QCoreApplication.instance() is None:
            QtCore.QCoreApplication([])

        network_manager = QtNetwork.QNetworkAccessManager()
        request = QtNetwork.QNetworkRequest(QtCore.QUrl(url))
        request.setRawHeader(b"Accept", b"application/json")
        request.setRawHeader(b"User-Agent", b"FreeCAD-BIM-Test")
        request.setRawHeader(b"X-User-Agent", b"FreeCAD-BIM-Test")

        state = {"payload": None, "error": None}
        event_loop = QtCore.QEventLoop()
        timeout = QtCore.QTimer()
        timeout.setSingleShot(True)
        timeout.timeout.connect(
            lambda: (state.__setitem__("error", "request timeout"), event_loop.quit())
        )

        reply = network_manager.get(request)

        def finish():
            try:
                if reply.error() != QtNetwork.QNetworkReply.NoError:
                    state["error"] = reply.errorString()
                else:
                    state["payload"] = json.loads(bytes(reply.readAll()).decode("utf-8"))
            except Exception as err:
                state["error"] = str(err)
            finally:
                reply.deleteLater()
                event_loop.quit()

        reply.finished.connect(finish)
        timeout.start(30000)
        event_loop.exec_()
        timeout.stop()
        self.assertIsNone(state["error"], state["error"])
        return state["payload"]

    @unittest.skipUnless(
        os.environ.get("FREECAD_RUN_LIVE_BSDD_TESTS") == "1",
        "Set FREECAD_RUN_LIVE_BSDD_TESTS=1 to run live bSDD integration tests.",
    )
    def test_live_dictionary_loading_returns_ifc_dictionary(self):
        payload = self._run_live_json_request(
            "https://api.bsdd.buildingsmart.org/api/Dictionary/v1"
        )
        dictionaries = payload.get("dictionaries") or payload.get("results") or payload
        self.assertTrue(dictionaries, "Live bSDD dictionary load returned no dictionaries.")

        ifc_dictionary = None
        for item in dictionaries:
            if item.get(
                "name"
            ) == "IFC" or "identifier.buildingsmart.org/uri/buildingsmart/ifc" in item.get(
                "uri", ""
            ):
                ifc_dictionary = item
                break

        self.assertIsNotNone(
            ifc_dictionary, "IFC dictionary not found in live bSDD dictionary response."
        )
        self.assertEqual(ifc_dictionary.get("name"), "IFC")
        self.assertIn(
            "identifier.buildingsmart.org/uri/buildingsmart/ifc", ifc_dictionary.get("uri", "")
        )

    @unittest.skipUnless(
        os.environ.get("FREECAD_RUN_LIVE_BSDD_TESTS") == "1",
        "Set FREECAD_RUN_LIVE_BSDD_TESTS=1 to run live bSDD integration tests.",
    )
    def test_live_wall_search_returns_ifc_wall_data(self):
        if QtCore.QCoreApplication.instance() is None:
            QtCore.QCoreApplication([])

        client = BimBsdd.BsddNetworkClient()
        received = {}
        failures = []
        event_loop = QtCore.QEventLoop()

        def on_search_ready(search_key, payload):
            received["search_key"] = search_key
            received["payload"] = payload
            items = payload.get("classes") or payload.get("results") or []
            wall_item = None
            for item in items:
                reference_code = item.get("referenceCode", "")
                name = item.get("name", "")
                uri = item.get("uri", "")
                if (
                    reference_code == "IfcWall"
                    or name == "IfcWall"
                    or uri.endswith("/class/IfcWall")
                ):
                    wall_item = item
                    break
            if wall_item is None:
                failures.append("bSDD search did not return an IfcWall entry")
                event_loop.quit()
                return
            received["wall_item"] = wall_item
            client.fetch_concept(wall_item["uri"])

        def on_concept_ready(concept_uri, payload):
            received["concept_uri"] = concept_uri
            received["concept_payload"] = payload
            event_loop.quit()

        def on_failure(request_kind, message, cache_key):
            failures.append(f"{request_kind}: {message} ({cache_key})")
            event_loop.quit()

        timeout = QtCore.QTimer()
        timeout.setSingleShot(True)
        timeout.timeout.connect(
            lambda: (failures.append("Timed out waiting for live bSDD response"), event_loop.quit())
        )

        client.searchReady.connect(on_search_ready)
        client.conceptReady.connect(on_concept_ready)
        client.requestFailed.connect(on_failure)

        timeout.start(30000)
        client.search_concepts(
            "wall",
            active_dictionaries=[BimBsdd.BSDD_IFC_DICTIONARY_URI],
            related_ifc_entity="IfcWall",
        )
        event_loop.exec_()
        timeout.stop()

        if failures:
            self.fail("; ".join(failures))

        search_payload = received["payload"]
        search_items = search_payload.get("classes") or search_payload.get("results") or []
        self.assertTrue(search_items, "Live bSDD search returned no classes.")
        self.assertEqual(received["wall_item"]["referenceCode"], "IfcWall")

        concept_payload = received["concept_payload"]
        self.assertEqual(concept_payload.get("referenceCode"), "IfcWall")
        self.assertEqual(concept_payload.get("name"), "Wall")
        self.assertEqual(concept_payload.get("classType"), "Class")

    @unittest.skipUnless(
        os.environ.get("FREECAD_RUN_LIVE_BSDD_TESTS") == "1",
        "Set FREECAD_RUN_LIVE_BSDD_TESTS=1 to run live bSDD integration tests.",
    )
    def test_live_ifc_wall_properties_returns_property_data(self):
        payload = self._run_live_json_request(
            "https://api.bsdd.buildingsmart.org/api/Class/Properties/v1"
            "?ClassUri=https%3A%2F%2Fidentifier.buildingsmart.org%2Furi%2Fbuildingsmart%2Fifc%2Flatest%2Fclass%2FIfcWall"
        )
        class_properties = payload.get("classProperties") or payload.get("properties") or []
        self.assertTrue(
            class_properties, "Live bSDD properties response returned no properties for IfcWall."
        )

        property_names = {item.get("name") for item in class_properties}
        self.assertIn("IsExternal", property_names)

        wall_common_property = None
        for item in class_properties:
            if item.get("name") == "IsExternal":
                wall_common_property = item
                break

        self.assertIsNotNone(wall_common_property, "IfcWall properties did not include IsExternal.")
        self.assertEqual(wall_common_property.get("propertySet"), "Pset_WallCommon")
