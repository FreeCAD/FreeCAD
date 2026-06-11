# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2024 Yorik van Havre <yorik@uncreated.net>              *
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


from . import ifc_psets  # lazy import
from . import ifc_tools  # lazy import


def edit_classification(obj):
    """Edits the classification of this object"""

    element = ifc_tools.get_ifc_element(obj)
    ifcfile = ifc_tools.get_ifcfile(obj)
    if not element or not ifcfile:
        return
    # TODO: remove previous reference?
    # ifc_tools.api_run("classification.remove_reference",
    #                   ifcfile, reference=ref, products=[obj])
    classifications = ifcfile.by_type("IfcClassification")
    classification = getattr(obj, "Classification", "")
    if classification:
        cname, code = classification.split(" ", 1)
        cnames = [c.Name for c in classifications]
        if cname in cnames:
            system = classifications[cnames.index(cname)]
        else:
            system = ifc_tools.api_run(
                "classification.add_classification", ifcfile, classification=cname
            )
        for ref in getattr(system, "HasReferences", []):
            rname = ref.Name or ref.Identification
            if code == rname:
                return
            elif code.startswith(rname):
                if getattr(ref, "ClassificationRefForObjects", None):
                    rel = ref.ClassificationRefForObjects[0]
                    if not element in rel.RelatedObjects:
                        ifc_tools.edit_attribute(
                            rel, "RelatedObjects", rel.RelatedObjects + [element]
                        )
                else:
                    # we have a reference, but no classForObjects
                    # this is weird and shouldn't exist...
                    rel = ifcfile.createIfcRelAssociatesClassification(
                        ifc_tools.ifcopenshell.guid.new(),
                        history,
                        "FreeCADClassificationRel",
                        None,
                        [element],
                        ref,
                    )
        else:
            ifc_tools.api_run(
                "classification.add_reference",
                ifcfile,
                products=[element],
                classification=system,
                identification=code,
            )
    else:
        # classification property is empty
        for rel in getattr(element, "HasAssociations", []):
            if rel.is_a("IfcRelAssociatesClassification"):
                # removing existing classification if only user
                if len(rel.RelatedObjects) == 1 and rel.RelatedObjects[0] == element:
                    ifc_tools.api_run(
                        "classification.remove_reference",
                        ifcfile,
                        reference=rel.RelatingClassification,
                        products=[element],
                    )
            # TODO: Remove IfcClassification too?


def show_classification(obj):
    """Loads the classification of this object"""

    element = ifc_tools.get_ifc_element(obj)
    ifcfile = ifc_tools.get_ifcfile(obj)
    if not element or not ifcfile:
        return
    for system in ifcfile.by_type("IfcClassification"):
        for ref in getattr(system, "HasReferences", []):
            for rel in ref.ClassificationRefForObjects:
                if element in rel.RelatedObjects:
                    if not "Classification" in obj.PropertiesList:
                        obj.addProperty("App::PropertyString", "Classification", "IFC", locked=True)
                    sname = system.Name
                    cname = ref.Name or ref.Identification
                    obj.Classification = sname + " " + cname
                    break


def _first_non_empty(*values):
    for value in values:
        if value not in [None, ""]:
            return value
    return ""


def _get_object_ifc_type(obj):
    for attr_name in ["IfcClass", "IfcType", "IfcRole"]:
        value = getattr(obj, attr_name, "")
        if value:
            return str(value)
    return ""


def _coerce_contract_value(value, data_type="", allowed_values=None):
    if isinstance(value, dict):
        value = _first_non_empty(value.get("value"), value.get("code"), value.get("name"))
    if isinstance(value, str):
        upper_value = value.strip().upper()
        if upper_value in ["TRUE", ".T.", "YES"]:
            return True
        if upper_value in ["FALSE", ".F.", "NO"]:
            return False
        if upper_value in ["UNKNOWN", "UNDEFINED", "NOTDEFINED", "N/A"]:
            return ""
        lowered_type = (data_type or "").lower()
        if "int" in lowered_type:
            try:
                return int(value)
            except Exception:
                pass
        if any(token in lowered_type for token in ["real", "float", "number", "numeric"]):
            try:
                return float(value)
            except Exception:
                pass
        try:
            if "." in value:
                return float(value)
            return int(value)
        except Exception:
            return value
    if value in [None, ""]:
        return None
    return value


def _get_project_element(ifcfile):
    projects = ifcfile.by_type("IfcProject")
    return projects[0] if projects else None


def _get_classification_namespace(classification):
    for attr_name in ["Source", "Location"]:
        value = getattr(classification, attr_name, "")
        if value:
            return value
    return ""


def _classification_matches_dictionary(classification, dictionary_metadata):
    target_name = dictionary_metadata.get("name", "")
    target_namespace = dictionary_metadata.get("namespace_uri", "")
    target_version = dictionary_metadata.get("version", "")
    classification_name = getattr(classification, "Name", "") or ""
    classification_namespace = _get_classification_namespace(classification)
    classification_version = getattr(classification, "Edition", "") or ""

    if target_namespace:
        if classification_namespace != target_namespace:
            return False
        if target_version and classification_version and classification_version != target_version:
            return False
        return True

    if classification_namespace:
        return False
    if target_name and classification_name != target_name:
        return False
    if target_version and classification_version and classification_version != target_version:
        return False
    return bool(target_name)


def _ensure_project_classification_relation(ifcfile, classification):
    project = _get_project_element(ifcfile)
    if not project:
        return
    for rel in ifcfile.by_type("IfcRelAssociatesClassification"):
        if rel.RelatingClassification == classification and project in rel.RelatedObjects:
            return
    owner_history = None
    try:
        owner_history = ifc_tools.ifcopenshell.api.owner.create_owner_history(ifcfile)
    except Exception:
        pass
    ifcfile.create_entity(
        "IfcRelAssociatesClassification",
        GlobalId=ifc_tools.ifcopenshell.guid.new(),
        OwnerHistory=owner_history,
        RelatedObjects=[project],
        RelatingClassification=classification,
    )


def _find_classification_root(ifcfile, dictionary_metadata):
    for classification in ifcfile.by_type("IfcClassification"):
        if _classification_matches_dictionary(classification, dictionary_metadata):
            return classification
    return None


def _upsert_classification_root(ifcfile, dictionary_metadata):
    classification = _find_classification_root(ifcfile, dictionary_metadata)
    if not classification:
        classification = ifc_tools.api_run(
            "classification.add_classification",
            ifcfile,
            classification=dictionary_metadata.get("name", "bSDD"),
        )
    attrs = {}
    if dictionary_metadata.get("name"):
        attrs["Name"] = dictionary_metadata["name"]
    if dictionary_metadata.get("namespace_uri") and hasattr(classification, "Source"):
        attrs["Source"] = dictionary_metadata["namespace_uri"]
    if dictionary_metadata.get("namespace_uri") and hasattr(classification, "Location"):
        attrs["Location"] = dictionary_metadata["namespace_uri"]
    if dictionary_metadata.get("version") and hasattr(classification, "Edition"):
        attrs["Edition"] = dictionary_metadata["version"]
    if attrs:
        ifc_tools.api_run(
            "classification.edit_classification",
            ifcfile,
            classification=classification,
            attributes=attrs,
        )
    _ensure_project_classification_relation(ifcfile, classification)
    return classification


def _get_reference_identification(reference):
    identification = getattr(reference, "Identification", None)
    if not identification:
        identification = getattr(reference, "ItemReference", None)
    return identification


def _find_classification_reference(ifcfile, classification, class_metadata):
    reference_code = class_metadata.get("reference_code", "")
    concept_uri = class_metadata.get("uri", "")
    for reference in ifcfile.by_type("IfcClassificationReference"):
        if getattr(reference, "ReferencedSource", None) != classification:
            continue
        if reference_code and _get_reference_identification(reference) == reference_code:
            return reference
        if concept_uri and getattr(reference, "Location", "") == concept_uri:
            return reference
    return None


def _upsert_classification_reference(ifcfile, classification, class_metadata):
    reference = _find_classification_reference(ifcfile, classification, class_metadata)
    if not reference:
        reference = ifcfile.createIfcClassificationReference(
            Name=class_metadata.get("name", ""),
            ReferencedSource=classification,
        )
    attrs = {"Name": class_metadata.get("name", "")}
    if class_metadata.get("description") and hasattr(reference, "Description"):
        attrs["Description"] = class_metadata["description"]
    if class_metadata.get("uri") and hasattr(reference, "Location"):
        attrs["Location"] = class_metadata["uri"]
    identification_attr = (
        "ItemReference" if hasattr(reference, "ItemReference") else "Identification"
    )
    if class_metadata.get("reference_code"):
        attrs[identification_attr] = class_metadata["reference_code"]
    ifc_tools.api_run(
        "classification.edit_reference",
        ifcfile,
        reference=reference,
        attributes=attrs,
    )
    return reference


def _find_reference_relation(ifcfile, reference):
    for rel in ifcfile.by_type("IfcRelAssociatesClassification"):
        if rel.RelatingClassification == reference:
            return rel
    return None


def _ensure_reference_relation(ifcfile, reference, element):
    rel = _find_reference_relation(ifcfile, reference)
    if rel:
        related_objects = set(rel.RelatedObjects)
        if element in related_objects:
            return False
        related_objects.add(element)
        rel.RelatedObjects = list(related_objects)
        try:
            ifc_tools.ifcopenshell.api.owner.update_owner_history(ifcfile, element=rel)
        except Exception:
            pass
        return True
    owner_history = None
    try:
        owner_history = ifc_tools.ifcopenshell.api.owner.create_owner_history(ifcfile)
    except Exception:
        pass
    ifcfile.create_entity(
        "IfcRelAssociatesClassification",
        GlobalId=ifc_tools.ifcopenshell.guid.new(),
        OwnerHistory=owner_history,
        RelatedObjects=[element],
        RelatingClassification=reference,
    )
    return True


def _upsert_extended_properties(obj, contract, ifcfile, element):
    for prop in contract.get("extended_properties", []):
        pset_name = prop.get("property_set", "")
        prop_name = prop.get("name", "")
        if not pset_name or not prop_name:
            continue
        coerced_value = _coerce_contract_value(
            prop.get("value"), prop.get("data_type", ""), prop.get("allowed_values", [])
        )
        if coerced_value in [None, ""]:
            continue
        pset = ifc_psets.get_pset(pset_name, element)
        if not pset:
            pset = ifc_tools.api_run("pset.add_pset", ifcfile, product=element, name=pset_name)
        ifc_tools.api_run(
            "pset.edit_pset",
            ifcfile,
            pset=pset,
            properties={prop_name: coerced_value},
        )
    try:
        ifc_psets.show_psets(obj)
    except Exception:
        pass


def apply_canonical_contract(obj, contract):
    """Applies a canonical bSDD contract to a native IFC object."""

    validation = contract.get("validation_filters", {})
    applicable_ifc_types = validation.get("applicable_ifc_types") or []
    active_ifc_type = _get_object_ifc_type(obj)
    is_applicable = (not applicable_ifc_types) or (active_ifc_type in applicable_ifc_types)
    if applicable_ifc_types and not is_applicable:
        raise ValueError(
            "Classification is not applicable to IFC type {}".format(
                active_ifc_type or validation.get("active_ifc_type", "")
            )
        )
    ifcfile = ifc_tools.get_ifcfile(obj)
    element = ifc_tools.get_ifc_element(obj, ifcfile)
    if not ifcfile or not element:
        return False

    classification = _upsert_classification_root(ifcfile, contract.get("dictionary_metadata", {}))
    reference = _upsert_classification_reference(
        ifcfile, classification, contract.get("class_metadata", {})
    )
    _ensure_reference_relation(ifcfile, reference, element)
    _upsert_extended_properties(obj, contract, ifcfile, element)

    legacy_string = _first_non_empty(
        contract.get("legacy_string"),
        "{} {}".format(
            contract.get("dictionary_metadata", {}).get("name", ""),
            contract.get("class_metadata", {}).get("reference_code", ""),
        ).strip(),
    )
    if hasattr(obj, "StandardCode") and legacy_string:
        obj.StandardCode = legacy_string
    return True
