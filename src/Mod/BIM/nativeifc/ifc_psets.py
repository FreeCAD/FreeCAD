# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2023 Yorik van Havre <yorik@uncreated.net>              *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
#*   You should have received a copy of the GNU Library General Public     *
#*   License along with this program; if not, write to the Free Software   *
#*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#*   USA                                                                   *
# *                                                                         *
# ***************************************************************************

"""This NativeIFC module deals with properties and property sets"""


import re
import FreeCAD
from nativeifc import ifc_tools


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
            while pname in obj.PropertiesList:
                # print("DEBUG: property", pname, "(", value, ") already exists in", obj.Label)
                pname += "_"
            if ptype in [
                "IfcPositiveLengthMeasure",
                "IfcLengthMeasure",
                "IfcNonNegativeLengthMeasure",
            ]:
                obj.addProperty("App::PropertyDistance", pname, gname, ttip)
            elif ptype in ["IfcVolumeMeasure"]:
                obj.addProperty("App::PropertyVolume", pname, gname, ttip)
            elif ptype in ["IfcPositivePlaneAngleMeasure", "IfcPlaneAngleMeasure"]:
                obj.addProperty("App::PropertyAngle", pname, gname, ttip)
                value = float(value)
                while value > 360:
                    value = value - 360
            elif ptype in ["IfcMassMeasure"]:
                obj.addProperty("App::PropertyMass", pname, gname, ttip)
            elif ptype in ["IfcAreaMeasure"]:
                obj.addProperty("App::PropertyArea", pname, gname, ttip)
            elif ptype in ["IfcCountMeasure", "IfcInteger"]:
                obj.addProperty("App::PropertyInteger", pname, gname, ttip)
                value = int(value.strip("."))
            elif ptype in ["IfcReal"]:
                obj.addProperty("App::PropertyFloat", pname, gname, ttip)
                value = float(value)
            elif ptype in ["IfcBoolean", "IfcLogical"]:
                obj.addProperty("App::PropertyBool", pname, gname, ttip)
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
                obj.addProperty("App::PropertyTime", pname, gname, ttip)
            else:
                obj.addProperty("App::PropertyString", pname, gname, ttip)
            # print("DEBUG: setting",pname, ptype, value)
            setattr(obj, pname, value)


def edit_pset(obj, prop, value=None):
    """Edits the corresponding property"""

    pset = obj.getGroupOfProperty(prop)
    ttip = obj.getDocumentationOfProperty(prop)
    if value is None:
        value = getattr(obj, prop)
    ifcfile = ifc_tools.get_ifcfile(obj)
    element = ifc_tools.get_ifc_element(obj)
    pset_exist = get_psets(element)
    if ttip.startswith("Ifc") and ":" in ttip:
        target_prop = ttip.split(":", 1)[-1]
    else:
        # no tooltip set - try to build a name
        prop = prop.rstrip("_")
        prop_uncamel = re.sub(r"(\w)([A-Z])", r"\1 \2", prop)
        prop_unslash = re.sub(r"(\w)([A-Z])", r"\1\/\2", prop)
        target_prop = None
    if pset in pset_exist:
        if not target_prop:
            if prop in pset_exist[pset]:
                target_prop = prop
            elif prop_uncamel in pset_exist[pset]:
                target_prop = prop_uncamel
            elif prop_unslash in pset_exist[pset]:
                target_prop = prop_unslash
        if target_prop:
            value_exist = pset_exist[pset][target_prop].split("(", 1)[1][:-1].strip("'")
            if value_exist in [".F.", ".U."]:
                value_exist = False
            elif value_exist in [".T."]:
                value_exist = True
            elif isinstance(value, int):
                value_exist = int(value_exist.strip("."))
            elif isinstance(value, float):
                value_exist = float(value_exist)
            elif isinstance(value, FreeCAD.Units.Quantity):
                if value.Unit.Type == "Angle":
                    value_exist = float(value_exist)
                    while value_exist > 360:
                        value_exist = value_exist - 360
                value_exist = FreeCAD.Units.Quantity(float(value_exist), value.Unit)
            if value == value_exist:
                return False
            else:
                FreeCAD.Console.PrintLog(
                    "IFC: property changed for "
                    + obj.Label
                    + " ("
                    + str(obj.StepId)
                    + ") : "
                    + str(target_prop)
                    + " : "
                    + str(value)
                    + " ("
                    + str(type(value))
                    + ") -> "
                    + str(value_exist)
                    + " ("
                    + str(type(value_exist))
                    + ")\n"
                )
        pset = get_pset(pset, element)
    else:
        pset = ifc_tools.api_run("pset.add_pset", ifcfile, product=element, name=pset)
    if not target_prop:
        target_prop = prop
    ifc_tools.api_run(
        "pset.edit_pset", ifcfile, pset=pset, properties={target_prop: value}
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
