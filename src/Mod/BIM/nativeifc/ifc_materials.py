# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2023 Yorik van Havre <yorik@uncreated.net>              *
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

"""This NativeIFC module deals with materials"""

import ifcopenshell
import ifcopenshell.util.element

import FreeCAD

from . import ifc_tools


def create_material(element, parent, recursive=False):
    """Creates a material object in the given project or parent material"""

    if not element:
        return
    if isinstance(element, (tuple, list)):
        for e in element:
            create_material(e, parent, recursive)
        return
    if hasattr(parent, "Document"):
        doc = parent.Document
    else:
        doc = parent
    exobj = ifc_tools.get_object(element, doc)
    if exobj:
        return exobj
    obj = ifc_tools.add_object(doc, otype="material")
    ifcfile = ifc_tools.get_ifcfile(parent)
    ifc_tools.add_properties(obj, ifcfile, element)
    if parent.isDerivedFrom("App::MaterialObject"):
        parent.Proxy.addObject(parent, obj)
    else:
        ifc_tools.get_group(parent, "IfcMaterialsGroup").addObject(obj)
    if recursive:
        submat = get_material(obj)
        if isinstance(submat, list):
            for s in submat:
                create_material(s, obj, recursive)
        else:
            create_material(submat, obj, recursive)
    return obj


def show_material(obj):
    """Creates and links materials for the given object, if available"""

    material = get_material(obj)
    if not material:
        return
    if not hasattr(obj, "Material"):
        obj.addProperty("App::PropertyLink", "Material", "IFC", locked=True)
    project = ifc_tools.get_project(obj)
    matobj = create_material(material, project, recursive=True)
    obj.Material = matobj


def load_materials(obj):
    """Recursively loads materials of child objects"""

    show_material(obj)
    if isinstance(obj, FreeCAD.DocumentObject):
        group = obj.Group
    else:
        group = obj.Objects
    for child in group:
        load_materials(child)


def get_material(obj):
    """Returns a material attached to this object"""

    element = ifc_tools.get_ifc_element(obj)
    if not element:
        return None
    if element.is_a("IfcMaterialConstituentSet"):
        return element.MaterialConstituents
    elif element.is_a() in [
        "IfcMaterialLayer",
        "IfcMaterialConstituent",
        "IfcMaterialProfile",
    ]:
        return element.Material
    elif element.is_a("IfcMaterialLayerSet"):
        return element.MaterialLayers
    elif element.is_a("IfcMaterialProfileSet"):
        return element.MaterialProfiles
    else:
        material = ifcopenshell.util.element.get_material(
            element, should_skip_usage=True
        )
        return material


def set_material(material, obj):
    """Attributes a material to an object"""

    ifcfile = ifc_tools.get_ifcfile(obj)
    element = ifc_tools.get_ifc_element(obj)
    material_element = ifc_tools.get_ifc_element(material)
    if not ifcfile:
        return
    new = False
    if not material_element or ifc_tools.get_ifcfile(material) != ifcfile:
        material_element = ifc_tools.api_run(
            "material.add_material", ifcfile, name=material.Label
        )
        new = True
        delete = not (ifc_tools.PARAMS.GetBool("KeepAggregated", False))
        if delete and len(material.InList) == 1:
            container = material.InList[0]
            doc = material.Document
            doc.removeObject(material.Name)
            if not container.OutList:
                doc.removeObject(container.Name)
    if material_element:
        try:
            # IfcOpenShell 0.8
            ifc_tools.api_run(
                "material.assign_material",
                ifcfile,
                products=[element],
                type=material_element.is_a(),
                material=material_element,
            )
        except:
            # IfcOpenShell 0.7
            ifc_tools.api_run(
                "material.assign_material",
                ifcfile,
                product=element,
                type=material_element.is_a(),
                material=material_element,
            )
        if new:
            show_material(obj)
