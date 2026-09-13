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

import FreeCAD

from . import backend
from . import ifc_tools

ifcopenshell = backend.get_backend()


def _name(material):
    """Return the stable user-facing name of a FreeCAD material object."""

    return str(getattr(material, "Label", None) or getattr(material, "Name", None) or "Material")


def _ifc_material(material, ifcfile):
    """Return a reusable IfcMaterial for a linked FreeCAD material."""

    name = _name(material)
    for candidate in ifcfile.by_type("IfcMaterial"):
        if candidate.Name == name:
            return candidate
    return ifc_tools.api_run(
        "material.add_material",
        ifcfile,
        name=name,
        description=getattr(material, "Description", None) or None,
    )


def _swept_profile(product):
    representation = getattr(product, "Representation", None)
    for shape_representation in getattr(representation, "Representations", ()) or ():
        for item in getattr(shape_representation, "Items", ()) or ():
            profile = getattr(item, "SweptArea", None)
            if profile:
                return profile
    return None


def _layer_set(material, ifcfile):
    materials = list(getattr(material, "Materials", ()) or ())
    thicknesses = list(getattr(material, "Thicknesses", ()) or ())
    layers = [
        (mat, thicknesses[index]) for index, mat in enumerate(materials) if index < len(thicknesses)
    ]
    if not layers:
        return None

    name = _name(material)
    scale = 0.001 / ifcopenshell.util.unit.calculate_unit_scale(ifcfile)
    expected = [(_name(mat), float(thickness) * scale) for mat, thickness in layers]
    for candidate in ifcfile.by_type("IfcMaterialLayerSet"):
        actual = [
            (layer.Material.Name, layer.LayerThickness)
            for layer in candidate.MaterialLayers
            if layer.Material
        ]
        if candidate.LayerSetName == name and actual == expected:
            return candidate

    result = ifc_tools.api_run(
        "material.add_material_set", ifcfile, name=name, set_type="IfcMaterialLayerSet"
    )
    for index, (freecad_material, thickness) in enumerate(layers):
        names = getattr(material, "Names", ()) or ()
        layer = ifc_tools.api_run(
            "material.add_layer",
            ifcfile,
            layer_set=result,
            material=_ifc_material(freecad_material, ifcfile),
            name=str(names[index]) if index < len(names) and names[index] else None,
        )
        ifc_tools.api_run(
            "material.edit_layer",
            ifcfile,
            layer=layer,
            attributes={"LayerThickness": float(thickness) * scale},
        )
    return result


def assign_export_material(obj, product, ifcfile):
    """Export and assign the material semantics linked to a FreeCAD object."""

    material = getattr(obj, "Material", None)
    if not material:
        return None

    if product.is_a() in ("IfcWall", "IfcWallStandardCase", "IfcSlab") and hasattr(
        material, "Materials"
    ):
        material_set = _layer_set(material, ifcfile)
        if material_set:
            return ifc_tools.api_run(
                "material.assign_material",
                ifcfile,
                products=[product],
                type="IfcMaterialLayerSetUsage",
                material=material_set,
            )

    if product.is_a() in ("IfcBeam", "IfcColumn", "IfcMember"):
        profile = _swept_profile(product)
        if profile:
            ifc_material = _ifc_material(material, ifcfile)
            profile_set = ifc_tools.api_run(
                "material.add_material_set",
                ifcfile,
                name=f"{_name(material)} - {profile.ProfileName or product.Name}",
                set_type="IfcMaterialProfileSet",
            )
            ifc_tools.api_run(
                "material.add_profile",
                ifcfile,
                profile_set=profile_set,
                material=ifc_material,
                profile=profile,
            )
            return ifc_tools.api_run(
                "material.assign_material",
                ifcfile,
                products=[product],
                type="IfcMaterialProfileSetUsage",
                material=profile_set,
            )

    return ifc_tools.api_run(
        "material.assign_material",
        ifcfile,
        products=[product],
        type="IfcMaterial",
        material=_ifc_material(material, ifcfile),
    )


def assign_export_type_material(obj, type_product, occurrence, ifcfile):
    """Assign a reusable material definition to an exported IFC type."""

    material = getattr(obj, "Material", None)
    if not material:
        return None
    if type_product.is_a() in ("IfcWallType", "IfcSlabType") and hasattr(material, "Materials"):
        material_set = _layer_set(material, ifcfile)
        if material_set:
            return ifc_tools.api_run(
                "material.assign_material",
                ifcfile,
                products=[type_product],
                type="IfcMaterialLayerSet",
                material=material_set,
            )
    if type_product.is_a() in ("IfcBeamType", "IfcColumnType", "IfcMemberType"):
        profile = _swept_profile(occurrence)
        if profile:
            ifc_material = _ifc_material(material, ifcfile)
            name = f"{_name(material)} - {profile.ProfileName or type_product.Name}"
            profile_set = next(
                (
                    candidate
                    for candidate in ifcfile.by_type("IfcMaterialProfileSet")
                    if candidate.Name == name
                ),
                None,
            )
            if profile_set is None:
                profile_set = ifc_tools.api_run(
                    "material.add_material_set",
                    ifcfile,
                    name=name,
                    set_type="IfcMaterialProfileSet",
                )
                ifc_tools.api_run(
                    "material.add_profile",
                    ifcfile,
                    profile_set=profile_set,
                    material=ifc_material,
                    profile=profile,
                )
            return ifc_tools.api_run(
                "material.assign_material",
                ifcfile,
                products=[type_product],
                type="IfcMaterialProfileSet",
                material=profile_set,
            )
    return ifc_tools.api_run(
        "material.assign_material",
        ifcfile,
        products=[type_product],
        type="IfcMaterial",
        material=_ifc_material(material, ifcfile),
    )


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
        obj.addProperty("App::PropertyLinkGlobal", "Material", "IFC", locked=True)
    elif obj.getTypeIdOfProperty("Material") == "App::PropertyLink":
        mat = obj.Material
        obj.setPropertyStatus("Material", "-LockDynamic")
        obj.removeProperty("Material")
        obj.addProperty("App::PropertyLinkGlobal", "Material", "IFC", locked=True)
        obj.Material = mat
    project = ifc_tools.get_project(obj)
    matobj = create_material(material, project, recursive=True)
    obj.Material = matobj


def load_materials(obj):
    """Recursively loads materials of child objects"""

    show_material(obj)
    if isinstance(obj, FreeCAD.DocumentObject) and hasattr(obj, "Group"):
        for child in obj.Group:
            load_materials(child)
    elif isinstance(obj, FreeCAD.Document):
        for child in obj.Objects:
            # Recursion not needed here.
            show_material(child)


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
        material = ifcopenshell.util.element.get_material(element, should_skip_usage=True)
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
        material_element = ifc_tools.api_run("material.add_material", ifcfile, name=material.Label)
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
