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

"""This NativeIFC module deals with properties and property sets"""

import os
import re

import FreeCAD

from . import ifc_tools

translate = FreeCAD.Qt.translate


def has_psets(obj):
    """Returns True if an object has attached psets"""

    element = ifc_tools.get_ifc_element(obj)
    psets = getattr(element, "IsDefinedBy", [])
    if psets and [p for p in psets if p.is_a("IfcRelDefinesByProperties")]:
        # TODO verify too if these psets are not already there
        return True
    psets = getattr(element, "HasProperties", [])
    if psets:
        return True
    return False


def get_psets(element):
    """Returns a dictionary of dictionaries representing the
    properties of an element in the form:
    { pset_name : { property_name : IfcType(value), ... }, ... }"""

    result = {}
    psets = getattr(element, "IsDefinedBy", [])
    psets = [p for p in psets if p.is_a("IfcRelDefinesByProperties")]
    psets = [p.RelatingPropertyDefinition for p in psets]
    if not psets:
        psets = getattr(element, "HasProperties", [])
    if not psets:
        return result
    for pset in psets:
        pset_dict = {}
        if pset.is_a("IfcPropertySet") and pset.HasProperties:
            for prop in pset.HasProperties:
                pset_dict[prop.Name] = str(prop.NominalValue)
        if pset.is_a("IfcMaterialProperties") and pset.Properties:
            for prop in pset.Properties:
                pset_dict[prop.Name] = str(prop.NominalValue)
        elif pset.is_a("IfcElementQuantity"):
            # TODO implement quantities
            pass
        if pset_dict:
            result[pset.Name] = pset_dict
    return result


def get_pset(psetname, element):
    """Returns an IfcPropertySet with the given name"""

    psets = getattr(element, "IsDefinedBy", [])
    psets = [p for p in psets if p.is_a("IfcRelDefinesByProperties")]
    for p in psets:
        pset = p.RelatingPropertyDefinition
        if pset.Name == psetname:
            return pset
    return None


def show_psets(obj):
    """Shows the psets attached to the given object as properties"""

    element = ifc_tools.get_ifc_element(obj)
    if not element:
        return
    psets = get_psets(element)
    for gname, pset in psets.items():
        for pname, pvalue in pset.items():
            oname = pname
            ptype, value = pvalue.split("(", 1)
            value = value.strip(")")
            value = value.strip("'")
            pname = re.sub(r"[^0-9a-zA-Z]+", "", pname)
            if pname[0].isdigit():
                pname = "_" + pname
            ttip = (
                ptype + ":" + oname
            )  # setting IfcType:PropName as a tooltip to desambiguate
            #while pname in obj.PropertiesList:
                # print("DEBUG: property", pname, "(", value, ") already exists in", obj.Label)
            #    pname += "_"
            ftype = None
            if ptype in [
                "IfcPositiveLengthMeasure",
                "IfcLengthMeasure",
                "IfcNonNegativeLengthMeasure",
            ]:
                ftype = "App::PropertyDistance"
            elif ptype in ["IfcVolumeMeasure"]:
                ftype = "App::PropertyVolume"
            elif ptype in ["IfcPositivePlaneAngleMeasure", "IfcPlaneAngleMeasure"]:
                ftype = "App::PropertyAngle"
                value = float(value)
                while value > 360:
                    value = value - 360
            elif ptype in ["IfcMassMeasure"]:
                ftype = "App::PropertyMass"
            elif ptype in ["IfcAreaMeasure"]:
                ftype = "App::PropertyArea"
            elif ptype in ["IfcCountMeasure", "IfcInteger"]:
                ftype = "App::PropertyInteger"
                value = int(value.strip("."))
            elif ptype in ["IfcReal"]:
                ftype = "App::PropertyFloat"
                value = float(value)
            elif ptype in ["IfcBoolean", "IfcLogical"]:
                ftype = "App::PropertyBool"
                if value in [".T."]:
                    value = True
                else:
                    value = False
            elif ptype in [
                "IfcDateTime",
                "IfcDate",
                "IfcTime",
                "IfcDuration",
                "IfcTimeStamp",
            ]:
                ftype = "App::PropertyTime"
            elif isinstance(value, str) and "::" in value:
                # FreeCAD-specific: split strings by :: delimiter
                ftype = "App::PropertyStringList"
                value = value.split("::")
            else:
                ftype = "App::PropertyString"
            # print("DEBUG: setting",pname, ptype, value)
            if ftype:
                if pname in obj.PropertiesList \
                and obj.getGroupOfProperty(pname) == gname:
                    if obj.getTypeOfProperty(pname) == ftype:
                        pass
                    if ftype == "App::PropertyString" \
                    and obj.getTypeOfProperty(pname) == "App::PropertyStringList":
                        value = [value]
                else:
                    print(pname, gname, obj.PropertiesList)
                    obj.addProperty(ftype, pname, gname, ttip, locked=True)
            if pname in obj.PropertiesList:
                setattr(obj, pname, value)


def edit_pset(obj, prop, value=None, force=False, ifcfile=None, element=None):
    """Edits the corresponding property. If force is True,
    the property is created even if it has no value"""

    pset = obj.getGroupOfProperty(prop)
    ptype = obj.getDocumentationOfProperty(prop)
    if value is None:
        value = getattr(obj, prop)
    if not ifcfile:
        ifcfile = ifc_tools.get_ifcfile(obj)
        if not ifcfile:
            return
    if not element:
        element = ifc_tools.get_ifc_element(obj)
        if not element:
            return
    pset_exist = get_psets(element)
    target_prop = None
    value_exist = None

    # build prop name and type
    if ptype.startswith("Ifc"):
        if ":" in ptype:
            target_prop = ptype.split(":", 1)[-1]
            ptype = ptype.split(":", 1)[0]
    else:
        ptype = obj.getTypeIdOfProperty(prop)
        if ptype == "App::PropertyDistance":
            ptype = "IfcLengthMeasure"
        elif ptype == "App::PropertyLength":
            ptype = "IfcPositiveLengthMeasure"
        elif ptype == "App::PropertyBool":
            ptype = "IfcBoolean"
        elif ptype == "App::PropertyInteger":
            ptype = "IfcInteger"
        elif ptype == "App::PropertyFloat":
            ptype = "IfcReal"
        elif ptype == "App::PropertyArea":
            ptype = "IfcAreaMeasure"
        else:
            # default
            ptype = "IfcLabel"
    if not target_prop:
        # test if the prop exists under different forms (uncameled, unslashed...)
        prop = prop.rstrip("_")
        prop_uncamel = re.sub(r"(\w)([A-Z])", r"\1 \2", prop)
        prop_unslash = re.sub(r"(\w)([A-Z])", r"\1\/\2", prop)
        if pset in pset_exist:
            if prop in pset_exist[pset]:
                target_prop = prop
            elif prop_uncamel in pset_exist[pset]:
                target_prop = prop_uncamel
            elif prop_unslash in pset_exist[pset]:
                target_prop = prop_unslash
    if not target_prop:
        target_prop = prop

    # create pset if needed
    if pset in pset_exist:
        ifcpset = get_pset(pset, element)
        if target_prop in pset_exist[pset]:
            value_exist = pset_exist[pset][target_prop].split("(", 1)[1][:-1].strip("'")
    else:
        ifcpset = ifc_tools.api_run("pset.add_pset", ifcfile, product=element, name=pset)

    # value conversions
    if value_exist in [".F.", ".U."]:
        value_exist = False
    elif value_exist in [".T."]:
        value_exist = True
    elif isinstance(value, int):
        if value_exist:
            value_exist = int(value_exist.strip("."))
    elif isinstance(value, float):
        if value_exist:
            value_exist = float(value_exist)
    elif isinstance(value, FreeCAD.Units.Quantity):
        if value_exist:
            value_exist = float(value_exist)
        if value.Unit.Type == "Angle":
            if value_exist:
                while value_exist > 360:
                    value_exist = value_exist - 360
            value = value.getValueAs("deg")
        elif value.Unit.Type == "Length":
            value = value.getValueAs("mm").Value * ifc_tools.get_scale(ifcfile)
        else:
            print("DEBUG: unhandled quantity type:",value, value.Unit.Type)
            return False
    if value == value_exist:
        return False
    if not force and not value and not value_exist:
        return False
    FreeCAD.Console.PrintLog(
        "IFC: property changed for "
        + obj.Label
        + " ("
        + str(element.id())
        + "): "
        + str(target_prop)
        + ": "
        + str(value_exist)
        + " ("
        + type(value_exist).__name__
        + ") -> "
        + str(value)
        + " ("
        + type(value).__name__
        + ")\n"
    )

    # run the change
    # TODO the property type is automatically determined by ifcopenhell
    # https://docs.ifcopenshell.org/autoapi/ifcopenshell/api/pset/edit_pset/index.html
    # and is therefore wrong for Quantity types. Research a way to overcome that
    ifc_tools.api_run(
        "pset.edit_pset", ifcfile, pset=ifcpset, properties={target_prop: value}
    )
    # TODO manage quantities
    return True


def load_psets(obj):
    """Recursively loads psets of child objects"""

    show_psets(obj)
    if isinstance(obj, FreeCAD.DocumentObject):
        group = obj.Group
    else:
        group = obj.Objects
    for child in group:
        load_psets(child)


def add_pset(obj, psetname):
    """Adds a pset with the given name to the given object"""

    ifcfile = ifc_tools.get_ifcfile(obj)
    element = ifc_tools.get_ifc_element(obj)
    if ifcfile and element:
        pset = ifc_tools.api_run(
            "pset.add_pset", ifcfile, product=element, name=psetname
        )
        return pset


def add_property(ifcfile, pset, name, value=""):
    """Adds a property with the given name to the given pset. The type is deduced from
    the value: string is IfcLabel, True/False is IfcBoolean, number is IfcLengthMeasure.
    To force a certain type, value can also be an IFC element such as IfcLabel"""

    ifc_tools.api_run("pset.edit_pset", ifcfile, pset=pset, properties={name: value})


def get_freecad_type(ptype):
    """Returns a FreeCAD property type corresponding to an IFC property type"""

    conv = read_properties_conversion()
    for key, values in conv.items():
        if ptype.lower() in [v.lower() for v in values.split(":")]:
            return key
    return "App::PropertyString"


def get_ifc_type(fctype):
    """Returns an IFC property type corresponding to a FreeCAD property type"""

    conv = read_properties_conversion()
    for key, values in conv.items():
        if fctype.lower() == key.lower():
            return values.split(":")[0]
    return "IfcLabel"


def read_properties_conversion():
    """Reads the properties conversion table"""

    import csv
    csvfile = os.path.join(
        FreeCAD.getResourceDir(), "Mod", "BIM", "Presets", "properties_conversion.csv"
    )
    result = {}
    if os.path.exists(csvfile):
        with open(csvfile, "r") as f:
            reader = csv.reader(f, delimiter=",")
            for row in reader:
                result[row[0]] = row[1]
    return result


def remove_property(obj, prop):
    """Removes a custom property"""

    from . import ifc_tools
    ifcfile = ifc_tools.get_ifcfile(obj)
    if not ifcfile:
        return
    element = ifc_tools.get_ifc_element(obj, ifcfile)
    if not element:
        return
    psets = get_psets(element)
    for psetname, props in psets.items():
        if prop in props:
            pset = get_pset(psetname, element)
            if pset:
                FreeCAD.Console.PrintMessage(translate("BIM","Removing property")+": "+prop)
                ifc_tools.api_run("pset.edit_pset", ifcfile, pset=pset, properties={prop: None})
                if len(props) == 1:
                    # delete the pset too
                    FreeCAD.Console.PrintMessage(translate("BIM","Removing property set")+": "+psetname)
                    ifc_tools.api_run("pset.remove_pset", ifcfile, product=element, pset=pset)


# Quantity types
# https://ifc43-docs.standards.buildingsmart.org/IFC/RELEASE/IFC4x3/HTML/ifcsharedbldgelements/content.html#6.1.5-Quantity-Sets
