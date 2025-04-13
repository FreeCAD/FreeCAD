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


def edit_classification(obj):
    """Edits the classification of this object"""

    element = ifc_tools.get_ifc_element(obj)
    ifcfile = ifc_tools.get_ifcfile(obj)
    if not element or not ifcfile:
        return
    # TODO: remove previous reference?
    #ifc_tools.api_run("classification.remove_reference",
    #                   ifcfile, reference=ref, products=[obj])
    classifications = ifcfile.by_type("IfcClassification")
    classification = getattr(obj, "Classification", "")
    if classification:
        cname, code = classification.split(" ", 1)
        cnames = [c.Name for c in classifications]
        if cname in cnames:
            system = classifications[cnames.index(cname)]
        else:
            system = ifc_tools.api_run("classification.add_classification", ifcfile,
                                       classification=cname)
        for ref in getattr(system, "HasReferences", []):
            rname = ref.Name or ref.Identification
            if code == rname:
                return
            elif code.startswith(rname):
                if getattr(ref, "ClassificationRefForObjects", None):
                    rel = ref.ClassificationRefForObjects[0]
                    if not element in rel.RelatedObjects:
                        ifc_tools.edit_attribute(rel, "RelatedObjects",
                                                 rel.RelatedObjects + [element]
                        )
                else:
                    # we have a reference, but no classForObjects
                    # this is weird and shouldn't exist...
                    rel = ifcfile.createIfcRelAssociatesClassification(
                        ifc_tools.ifcopenshell.guid.new(),
                        history,'FreeCADClassificationRel',
                        None,
                        [element],
                        ref
                    )
        else:
            ifc_tools.api_run("classification.add_reference", ifcfile,
                              products = [element],
                              classification = system,
                              identification = code
            )
    else:
        # classification property is empty
        for rel in getattr(element, "HasAssociations", []):
            if rel.is_a("IfcRelAssociatesClassification"):
                # removing existing classification if only user
                if len(rel.RelatedObjects) == 1 and rel.RelatedObjects[0] == element:
                    ifc_tools.api_run("classification.remove_reference",
                                      ifcfile,
                                      reference=rel.RelatingClassification,
                                      products=[element]
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
