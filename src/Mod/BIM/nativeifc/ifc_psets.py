# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2023 Yorik van Havre <yorik@uncreated.net>              *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU General Public License (GPL)            *
# *   as published by the Free Software Foundation; either version 3 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU General Public License for more details.                          *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************

"""This NativeIFC module deals with properties and property sets"""


import os
import re
import FreeCAD
from nativeifc import ifc_tools

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


def edit_pset(obj, prop, value=None, force=False):
    """Edits the corresponding property. If force is True,
    the property is created even if it has no value"""

    pset = obj.getGroupOfProperty(prop)
    ptype = obj.getDocumentationOfProperty(prop)
    if value is None:
        value = getattr(obj, prop)
    ifcfile = ifc_tools.get_ifcfile(obj)
    element = ifc_tools.get_ifc_element(obj)
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
        if ifcprop == "App::PropertyDistance":
            ptype = "IfcLengthMeasure"
        elif ifcprop == "App::PropertyLength":
            ptype = "IfcPositiveLengthMeasure"
        elif ifcprop == "App::PropertyBool":
            ptype = "IfcBoolean"
        elif ifcprop == "App::PropertyInteger":
            ptype = "IfcInteger"
        elif ifcprop == "App::PropertyFloat":
            ptype = "IfcReal"
        elif ifcprop == "App::PropertyArea":
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
        + str(obj.StepId)
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

    from nativeifc import ifc_tools
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
