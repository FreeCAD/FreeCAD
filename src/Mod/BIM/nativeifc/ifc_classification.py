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


from . import ifc_tools  # lazy import


def _value(obj, name, default=None):
    value = getattr(obj, name, default)
    return value or default


def _classification_data(obj):
    reference = getattr(obj, "ClassificationReference", None)
    if reference:
        system = getattr(reference, "ReferencedSource", None)
        if system:
            return {
                "system_name": _value(system, "ClassificationName", system.Label),
                "source": _value(system, "Source"),
                "edition": _value(system, "Edition"),
                "edition_date": _value(system, "EditionDate"),
                "system_location": _value(system, "Location"),
                "identification": _value(reference, "Identification"),
                "name": _value(reference, "ReferenceName", reference.Label),
                "location": _value(reference, "Location"),
                "description": _value(reference, "Description"),
            }
    classification = getattr(obj, "Classification", None) or getattr(obj, "StandardCode", None)
    if classification and " " in classification:
        system_name, identification = classification.split(" ", 1)
        return {"system_name": system_name, "identification": identification}
    return None


def _classification_system(ifcfile, data):
    for system in ifcfile.by_type("IfcClassification"):
        if system.Name == data["system_name"]:
            return system
    if ifcfile.wrapped_data.schema_name().startswith("IFC4"):
        return ifcfile.create_entity(
            "IfcClassification",
            Source=data.get("source"),
            Edition=data.get("edition"),
            EditionDate=data.get("edition_date"),
            Name=data["system_name"],
            Location=data.get("system_location"),
        )
    edition_date = None
    try:
        year, month, day = (int(value) for value in data.get("edition_date", "").split("-"))
        edition_date = ifcfile.createIfcCalendarDate(day, month, year)
    except (AttributeError, TypeError, ValueError):
        pass
    return ifcfile.create_entity(
        "IfcClassification",
        Source=data.get("source"),
        Edition=data.get("edition"),
        EditionDate=edition_date,
        Name=data["system_name"],
    )


def _classification_reference(ifcfile, system, data):
    identification = data["identification"]
    for reference in ifcfile.by_type("IfcClassificationReference"):
        reference_id = getattr(reference, "Identification", None) or getattr(
            reference, "ItemReference", None
        )
        if reference.ReferencedSource == system and reference_id == identification:
            return reference
    attributes = {
        "Location": data.get("location"),
        "Name": data.get("name"),
        "ReferencedSource": system,
    }
    if ifcfile.wrapped_data.schema_name().startswith("IFC4"):
        attributes["Identification"] = identification
        attributes["Description"] = data.get("description")
    else:
        attributes["ItemReference"] = identification
    return ifcfile.create_entity("IfcClassificationReference", **attributes)


def assign_export_classification(obj, product, ifcfile):
    """Assign explicit FreeCAD classification metadata to an IFC object."""

    data = _classification_data(obj)
    if not data or not data.get("identification"):
        return None
    system = _classification_system(ifcfile, data)
    reference = _classification_reference(ifcfile, system, data)
    relationships = list(getattr(reference, "ClassificationRefForObjects", ()) or ())
    if relationships:
        relationship = relationships[0]
        if product not in relationship.RelatedObjects:
            relationship.RelatedObjects = [*relationship.RelatedObjects, product]
        return relationship
    history = next(iter(ifcfile.by_type("IfcOwnerHistory")), None)
    return ifcfile.createIfcRelAssociatesClassification(
        ifc_tools.ifcopenshell.guid.new(),
        history,
        "FreeCADClassificationRel",
        None,
        [product],
        reference,
    )


def _group(source, ifcfile):
    tag = source.Name
    for group in ifcfile.by_type("IfcGroup"):
        if group.ObjectType == tag:
            return group
    group = ifc_tools.api_run(
        "group.add_group",
        ifcfile,
        name=source.Label,
        description=getattr(source, "Description", None),
    )
    group.ObjectType = tag
    return group


def assign_export_groups(obj, product, ifcfile):
    """Export non-spatial FreeCAD document-group membership as IfcGroup."""

    result = []
    for parent in obj.InList:
        if not parent.isDerivedFrom("App::DocumentObjectGroup"):
            continue
        if getattr(parent, "IfcType", None) not in (None, "", "Undefined", "Group"):
            continue
        group = _group(parent, ifcfile)
        relationship = ifc_tools.api_run(
            "group.assign_group", ifcfile, products=[product], group=group
        )
        result.append(relationship)
    return result


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
