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
"""Asynchronous bSDD network core with preference-backed filtering."""

import json

import FreeCAD
from PySide import QtCore, QtNetwork

BSDD_API_BASE_URL = "https://api.bsdd.buildingsmart.org"
BSDD_PREFERENCES_PATH = "User parameter:BaseApp/Preferences/Mod/BIM"
BSDD_PREVIEW_KEY = "BsddLoadPreviewDomains"
BSDD_TEST_KEY = "BsddLoadTestDomains"
BSDD_INACTIVE_KEY = "BsddLoadInactiveDomains"
BSDD_API_URL_KEY = "BsddApiBaseUrl"
BSDD_IFC_DICTIONARY_URI = "https://identifier.buildingsmart.org/uri/buildingsmart/ifc/latest"
_DOMAIN_CACHE_KEY = "__domains__"
_BSDD_CLIENT = None


class BsddSettings:
    """Stores the bSDD settings synchronized from FreeCAD parameters."""

    def __init__(self):
        self.include_preview = False
        self.include_test = False
        self.include_inactive = False
        self.api_base_url = BSDD_API_BASE_URL


class BsddNetworkClient(QtCore.QObject):
    """Background-only asynchronous bSDD client with in-memory caches."""

    dictionariesReady = QtCore.Signal(object)
    searchReady = QtCore.Signal(object, object)
    conceptReady = QtCore.Signal(str, object)
    requestFailed = QtCore.Signal(str, str, object)

    def __init__(self, network_manager=None, parent=None):
        super().__init__(parent)
        self._parameter_group = FreeCAD.ParamGet(BSDD_PREFERENCES_PATH)
        self.settings = BsddSettings()
        self.domain_cache = {}
        self.search_cache = {}
        self.concept_cache = {}
        self._pending_concept_payloads = {}
        self._network_manager = network_manager or QtNetwork.QNetworkAccessManager(self)
        self.refresh_settings()

    def refresh_settings(self):
        """Synchronizes the cached settings with the central parameter store."""
        self.settings.include_preview = self._parameter_group.GetBool(BSDD_PREVIEW_KEY, False)
        self.settings.include_test = self._parameter_group.GetBool(BSDD_TEST_KEY, False)
        self.settings.include_inactive = self._parameter_group.GetBool(BSDD_INACTIVE_KEY, False)
        self.settings.api_base_url = self._parameter_group.GetString(
            BSDD_API_URL_KEY, BSDD_API_BASE_URL
        ).rstrip("/")
        return self.settings

    def clear_caches(self):
        """Clears all in-memory bSDD caches."""
        self.domain_cache.clear()
        self.search_cache.clear()
        self.concept_cache.clear()

    def fetch_dictionaries(self, force_refresh=False):
        """Retrieves the dictionary registry payload."""
        self.refresh_settings()
        cache_key = self._dictionary_cache_key()
        if (not force_refresh) and (cache_key in self.domain_cache):
            self.dictionariesReady.emit(self.domain_cache[cache_key])
            return
        self._submit_json_request(
            request_kind="dictionary",
            cache_key=cache_key,
            endpoint="/api/Dictionary/v1",
            query_items=self._status_query_items(),
        )

    def search_concepts(self, query_text, active_dictionaries=None, related_ifc_entity=""):
        """Retrieves filtered search results."""
        self.refresh_settings()
        active_dictionaries = tuple(sorted(active_dictionaries or ()))
        search_key = self._search_cache_key(query_text, active_dictionaries, related_ifc_entity)
        if search_key in self.search_cache:
            self.searchReady.emit(search_key, self.search_cache[search_key])
            return search_key
        query_items = self._status_query_items()
        query_items.append(("SearchText", query_text))
        for dictionary_uri in active_dictionaries:
            query_items.append(("DictionaryUris", dictionary_uri))
        if related_ifc_entity:
            query_items.append(("RelatedIfcEntities", related_ifc_entity))
        self._submit_json_request(
            request_kind="search",
            cache_key=search_key,
            endpoint="/api/Class/Search/v1",
            query_items=query_items,
        )
        return search_key

    def fetch_concept(self, concept_uri):
        """Retrieves a full concept payload."""
        self.refresh_settings()
        cache_key = self._concept_cache_key(concept_uri)
        if cache_key in self.concept_cache:
            self.conceptReady.emit(concept_uri, self.concept_cache[cache_key])
            return
        self._pending_concept_payloads[cache_key] = {"base": None, "properties": None}
        query_items = self._status_query_items()
        query_items.append(("Uri", concept_uri))
        self._submit_json_request(
            request_kind="concept_base",
            cache_key=cache_key,
            endpoint="/api/Class/v1",
            query_items=query_items,
        )
        property_query_items = self._status_query_items()
        property_query_items.append(("ClassUri", concept_uri))
        self._submit_json_request(
            request_kind="concept_properties",
            cache_key=cache_key,
            endpoint="/api/Class/Properties/v1",
            query_items=property_query_items,
        )

    def _status_query_items(self):
        return [
            ("IncludePreview", self._bool_string(self.settings.include_preview)),
            ("IncludeTestDictionaries", self._bool_string(self.settings.include_test)),
            ("IncludeInactive", self._bool_string(self.settings.include_inactive)),
        ]

    def _submit_json_request(self, request_kind, cache_key, endpoint, query_items):
        request = self._build_request(endpoint, query_items)
        reply = self._network_manager.get(request)
        reply.finished.connect(
            lambda rk=request_kind, ck=cache_key, rp=reply: self._process_reply(rk, ck, rp)
        )

    def _build_request(self, endpoint, query_items):
        url = QtCore.QUrl(self.settings.api_base_url + endpoint)
        url_query = QtCore.QUrlQuery()
        for key, value in query_items:
            url_query.addQueryItem(key, value)
        url.setQuery(url_query)
        request = QtNetwork.QNetworkRequest(url)
        request.setRawHeader(b"Accept", b"application/json")
        user_agent = self._user_agent().encode("utf-8")
        request.setRawHeader(b"User-Agent", user_agent)
        request.setRawHeader(b"X-User-Agent", user_agent)
        return request

    def _process_reply(self, request_kind, cache_key, reply):
        try:
            if reply.error() != QtNetwork.QNetworkReply.NoError:
                if request_kind in ["concept_base", "concept_properties"]:
                    self._pending_concept_payloads.pop(cache_key, None)
                self.requestFailed.emit(request_kind, reply.errorString(), cache_key)
                return
            payload = bytes(reply.readAll())
            parsed_payload = json.loads(payload.decode("utf-8")) if payload else None
            if request_kind == "dictionary":
                self.domain_cache[cache_key] = parsed_payload
                self.dictionariesReady.emit(parsed_payload)
            elif request_kind == "search":
                self.search_cache[cache_key] = parsed_payload
                self.searchReady.emit(cache_key, parsed_payload)
            elif request_kind == "concept_base":
                self._collect_concept_payload(cache_key, "base", parsed_payload)
            elif request_kind == "concept_properties":
                self._collect_concept_payload(cache_key, "properties", parsed_payload)
        except Exception as err:
            if request_kind in ["concept_base", "concept_properties"]:
                self._pending_concept_payloads.pop(cache_key, None)
            self.requestFailed.emit(request_kind, str(err), cache_key)
        finally:
            reply.deleteLater()

    def _collect_concept_payload(self, concept_uri, payload_kind, payload):
        if (
            isinstance(concept_uri, tuple)
            and len(concept_uri) == 2
            and isinstance(concept_uri[1], tuple)
        ):
            cache_key = concept_uri
            concept_uri = concept_uri[0]
        else:
            cache_key = self._concept_cache_key(concept_uri)
        pending = self._pending_concept_payloads.setdefault(
            cache_key, {"base": None, "properties": None}
        )
        if payload_kind == "base":
            pending["base"] = payload or {}
        elif payload_kind == "properties":
            pending["properties"] = self._normalize_concept_properties_payload(payload)
        if pending["base"] is None or pending["properties"] is None:
            return
        merged_payload = self._merge_concept_payloads(pending["base"], pending["properties"])
        self.concept_cache[cache_key] = merged_payload
        self._pending_concept_payloads.pop(cache_key, None)
        self.conceptReady.emit(concept_uri, merged_payload)

    def _settings_cache_signature(self):
        return (
            self.settings.api_base_url,
            bool(self.settings.include_preview),
            bool(self.settings.include_test),
            bool(self.settings.include_inactive),
        )

    def _dictionary_cache_key(self):
        return (_DOMAIN_CACHE_KEY, self._settings_cache_signature())

    def _search_cache_key(self, query_text, active_dictionaries, related_ifc_entity):
        return (
            query_text,
            tuple(active_dictionaries or ()),
            related_ifc_entity or "",
            self._settings_cache_signature(),
        )

    def _concept_cache_key(self, concept_uri):
        return (concept_uri, self._settings_cache_signature())

    @staticmethod
    def _normalize_concept_properties_payload(payload):
        if not isinstance(payload, dict):
            return {"classProperties": payload or []}
        if payload.get("classProperties") is not None:
            return payload
        if payload.get("properties") is not None:
            normalized = dict(payload)
            normalized["classProperties"] = payload.get("properties") or []
            return normalized
        return payload

    @staticmethod
    def _merge_concept_payloads(base_payload, properties_payload):
        merged = {}
        if isinstance(base_payload, dict):
            merged.update(base_payload)
        if isinstance(properties_payload, dict):
            merged.update(properties_payload)
            if properties_payload.get("classProperties") is not None:
                merged["classProperties"] = properties_payload.get("classProperties") or []
        return merged

    @staticmethod
    def _bool_string(value):
        return "true" if value else "false"

    @staticmethod
    def _user_agent():
        try:
            version = ".".join(FreeCAD.Version()[:3])
        except Exception:
            version = "unknown"
        return f"FreeCAD-BIM/{version}"


def get_bsdd_network_client():
    """Returns a lazily-instantiated shared bSDD client."""
    global _BSDD_CLIENT
    if _BSDD_CLIENT is None:
        _BSDD_CLIENT = BsddNetworkClient()
    return _BSDD_CLIENT
