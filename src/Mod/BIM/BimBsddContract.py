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

"""Canonical bSDD contract and validation helpers."""


def _as_list(value):
    if value is None:
        return []
    if isinstance(value, (list, tuple, set)):
        return [v for v in value if v not in [None, ""]]
    return [value]


def _first_non_empty(*values):
    for value in values:
        if value not in [None, ""]:
            return value
    return ""


def _extract_active_ifc_type(obj):
    if not obj:
        return ""
    for attr_name in ["IfcClass", "IfcType", "IfcRole"]:
        value = getattr(obj, attr_name, "")
        if value:
            return str(value)
    return ""


def _extract_applicable_ifc_types(payload):
    values = []
    for key in ["relatedIfcEntities", "relatedIfcEntity", "applicableIfcClasses"]:
        values.extend(_as_list(payload.get(key)))
    return [str(value) for value in values if value]


def _extract_dictionary_metadata(payload):
    return {
        "name": _first_non_empty(
            payload.get("dictionaryName"),
            payload.get("classificationName"),
            payload.get("source"),
        ),
        "namespace_uri": _first_non_empty(
            payload.get("dictionaryUri"),
            payload.get("namespaceUri"),
            payload.get("dictionaryNamespaceUri"),
        ),
        "version": _first_non_empty(
            payload.get("dictionaryVersion"),
            payload.get("version"),
            payload.get("versionDate"),
        ),
    }


def _extract_class_metadata(payload):
    return {
        "reference_code": _first_non_empty(
            payload.get("referenceCode"),
            payload.get("identification"),
            payload.get("code"),
        ),
        "name": _first_non_empty(
            payload.get("name"),
            payload.get("description"),
            payload.get("referenceCode"),
        ),
        "description": _first_non_empty(
            payload.get("description"),
            payload.get("definition"),
            payload.get("name"),
        ),
        "uri": _first_non_empty(
            payload.get("uri"),
            payload.get("conceptUri"),
            payload.get("location"),
        ),
    }


def _extract_property_value(prop):
    for key in [
        "predefinedValue",
        "defaultValue",
        "value",
        "valueExpression",
        "exampleValue",
    ]:
        if prop.get(key) not in [None, ""]:
            return prop.get(key)
    return None


def _extract_extended_properties(payload):
    results = []
    for prop in payload.get("classProperties", []) or []:
        results.append(
            {
                "property_set": _first_non_empty(
                    prop.get("propertySet"),
                    prop.get("propertySetName"),
                    prop.get("pset"),
                ),
                "name": _first_non_empty(prop.get("name"), prop.get("code")),
                "data_type": _first_non_empty(
                    prop.get("dataType"),
                    prop.get("propertyDomainName"),
                    prop.get("type"),
                ),
                "value": _extract_property_value(prop),
                "allowed_values": prop.get("allowedValues") or prop.get("values") or [],
                "uri": _first_non_empty(prop.get("uri"), prop.get("propertyUri")),
            }
        )
    return results


def merge_payloads(base_payload, detail_payload):
    merged = {}
    if isinstance(base_payload, dict):
        merged.update(base_payload)
    if isinstance(detail_payload, dict):
        merged.update(detail_payload)
        base_props = (base_payload or {}).get("classProperties") or []
        detail_props = detail_payload.get("classProperties") or []
        merged["classProperties"] = detail_props or base_props
    return merged


def build_canonical_contract(base_payload, detail_payload=None, active_object=None):
    payload = merge_payloads(base_payload, detail_payload)
    active_ifc_type = _extract_active_ifc_type(active_object)
    applicable_ifc_types = _extract_applicable_ifc_types(payload)
    is_applicable = (not applicable_ifc_types) or (active_ifc_type in applicable_ifc_types)
    return {
        "dictionary_metadata": _extract_dictionary_metadata(payload),
        "class_metadata": _extract_class_metadata(payload),
        "validation_filters": {
            "applicable_ifc_types": applicable_ifc_types,
            "active_ifc_type": active_ifc_type,
            "is_applicable": is_applicable,
        },
        "extended_properties": _extract_extended_properties(payload),
    }


def refresh_contract_validation(contract, active_object=None):
    refreshed = dict(contract or {})
    validation = dict(refreshed.get("validation_filters", {}) or {})
    applicable_ifc_types = list(validation.get("applicable_ifc_types") or [])
    active_ifc_type = _extract_active_ifc_type(active_object)
    validation["active_ifc_type"] = active_ifc_type
    validation["is_applicable"] = (not applicable_ifc_types) or (
        active_ifc_type in applicable_ifc_types
    )
    refreshed["validation_filters"] = validation
    return refreshed


def build_legacy_classification_string(contract):
    dictionary_name = contract.get("dictionary_metadata", {}).get("name", "")
    reference_code = contract.get("class_metadata", {}).get("reference_code", "")
    name = contract.get("class_metadata", {}).get("name", "")
    code = reference_code or name
    if dictionary_name and code:
        return "{} {}".format(dictionary_name, code)
    return code or dictionary_name
