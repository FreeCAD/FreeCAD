#***************************************************************************
#*   Copyright (c) 2019 Dion Moult <dion@thinkmoult.com>                   *
#*   Copyright (c) 2019 Yorik van Havre <yorik@uncreated.net>              *
#*   Copyright (c) 2020 FreeCAD Developers                                 *
#*                                                                         *
#*   This program is free software; you can redistribute it and/or modify  *
#*   it under the terms of the GNU Lesser General Public License (LGPL)    *
#*   as published by the Free Software Foundation; either version 2 of     *
#*   the License, or (at your option) any later version.                   *
#*   for detail see the LICENCE text file.                                 *
#*                                                                         *
#*   This program is distributed in the hope that it will be useful,       *
#*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
#*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
#*   GNU Library General Public License for more details.                  *
#*                                                                         *
#*   You should have received a copy of the GNU Library General Public     *
#*   License along with this program; if not, write to the Free Software   *
#*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#*   USA                                                                   *
#*                                                                         *
#***************************************************************************

"""This modules sets up and manages the IFC-related properties, types
and attributes of Arch/BIM objects.
"""

import json

import FreeCAD
import ArchIFCSchema

if FreeCAD.GuiUp:
    from PySide.QtCore import QT_TRANSLATE_NOOP
else:
    def QT_TRANSLATE_NOOP(ctx,txt):
        return txt

def uncamel(t):
    return ''.join(map(lambda x: x if x.islower() else " "+x, t[3:]))[1:]

IfcTypes = [uncamel(t) for t in ArchIFCSchema.IfcProducts.keys()]

class IfcRoot:
    """This class defines the common methods and properties for managing IFC data.

    IFC, or Industry Foundation Classes are a standardised way to digitally
    describe the built environment.  The ultimate goal of IFC is to provide
    better interoperability between software that deals with the built
    environment. You can learn more here:
    https://technical.buildingsmart.org/standards/ifc/

    You can learn more about the technical details of the IFC schema here:
    https://standards.buildingsmart.org/IFC/RELEASE/IFC4/FINAL/HTML/

    This class is further segmented down into IfcProduct and IfcContext.
    """

    def setProperties(self, obj):
        """Give the object properties for storing IFC data.

        Also migrate old versions of IFC properties to the new property names
        using the .migrateDeprecatedAttributes() method.
        """

        if not "IfcData" in obj.PropertiesList:
            obj.addProperty("App::PropertyMap","IfcData","IFC",QT_TRANSLATE_NOOP("App::Property","IFC data"))

        if not "IfcType" in obj.PropertiesList:
            obj.addProperty("App::PropertyEnumeration","IfcType","IFC",QT_TRANSLATE_NOOP("App::Property","The type of this object"))
            obj.IfcType = self.getCanonicalisedIfcTypes()

        if not "IfcProperties" in obj.PropertiesList:
            obj.addProperty("App::PropertyMap","IfcProperties","IFC",QT_TRANSLATE_NOOP("App::Property","IFC properties of this object"))

        self.migrateDeprecatedAttributes(obj)

    def onChanged(self, obj, prop):
        """Method called when the object has a property changed.

        If the object's IfcType has changed, change the object's properties
        that relate to IFC attributes in order to match the IFC schema
        definition of the new IFC type.

        If a property changes that is in the "IFC Attributes" group, also
        change the value stored in the IfcData property's JSON.

        Parameters
        ----------
        prop: string
            The name of the property that has changed.
        """

        if prop == "IfcType":
            self.setupIfcAttributes(obj)
            self.setupIfcComplexAttributes(obj)
        if prop in obj.PropertiesList:
            if obj.getGroupOfProperty(prop) == "IFC Attributes":
                self.setObjIfcAttributeValue(obj, prop, obj.getPropertyByName(prop))

    def setupIfcAttributes(self, obj):
        """Set up the IFC attributes in the object's properties.

        Add the attributes specified in the object's IFC type schema, to the
        object's properties. Do not re-add them if they're already present.
        Also remove old IFC attribute properties that no longer appear in the
        schema for backwards compatibility.

        Do so using the .addIfcAttributes() and
        .purgeUnusedIfcAttributesFromPropertiesList() methods.

        Learn more about IFC attributes here:
        https://standards.buildingsmart.org/IFC/RELEASE/IFC4/FINAL/HTML/schema/chapter-3.htm#attribute
        """

        ifcTypeSchema = self.getIfcTypeSchema(obj.IfcType)
        if ifcTypeSchema is None:
            return
        self.purgeUnusedIfcAttributesFromPropertiesList(ifcTypeSchema, obj)
        self.addIfcAttributes(ifcTypeSchema, obj)

    def setupIfcComplexAttributes(self, obj):
        """Add the IFC type's complex attributes to the object.

        Get the object's IFC type schema, and add the schema for the type's
        complex attributes within the IfcData property.
        """

        ifcTypeSchema = self.getIfcTypeSchema(obj.IfcType)
        if ifcTypeSchema is None:
            return
        IfcData = obj.IfcData
        if "complex_attributes" not in IfcData:
            IfcData["complex_attributes"] = "{}"
        ifcComplexAttributes = json.loads(IfcData["complex_attributes"])
        for attribute in ifcTypeSchema["complex_attributes"]:
            if attribute["name"] not in ifcComplexAttributes:
                ifcComplexAttributes[attribute["name"]] = {}
        IfcData["complex_attributes"] = json.dumps(ifcComplexAttributes)
        obj.IfcData = IfcData

    def getIfcTypeSchema(self, IfcType):
        """Get the schema of the IFC type provided.

        If the IFC type is undefined, return the schema of the
        IfcBuildingElementProxy.

        Parameter
        ---------
        IfcType: str
            The IFC type whose schema you want.

        Returns
        -------
        dict
            Returns the schema of the type as a dict.
        None
            Returns None if the IFC type does not exist.
        """
        name = "Ifc" + IfcType.replace(" ", "")
        if IfcType == "Undefined":
            name = "IfcBuildingElementProxy"
        if name in self.getIfcSchema():
            return self.getIfcSchema()[name]
        return None

    def getIfcSchema(self):
        """Get the IFC schema of all types relevant to this class.

        Intended to be overwritten by the classes that inherit this class.

        Returns
        -------
        dict
            The schema of all the types relevant to this class.
        """

        return {}

    def getCanonicalisedIfcTypes(self):
        """Get the names of IFC types, converted to the form used in Arch.

        Change the names of all IFC types to a more human readable form which
        is used instead throughout Arch instead of the raw type names. The
        names have the "Ifc" stripped from the start of their name, and spaces
        inserted between the words.

        Returns
        -------
        list of str
            The list of every IFC type name in their form used in Arch. List
            will have names in the same order as they appear in the schema's
            JSON, as per the .keys() method of dicts.

        """
        schema = self.getIfcSchema()
        return [''.join(map(lambda x: x if x.islower() else " "+x, t[3:]))[1:] for t in schema.keys()]

    def getIfcAttributeSchema(self, ifcTypeSchema, name):
        """Get the schema of an IFC attribute with the given name.

        Convert the IFC attribute's name from the human readable version Arch
        uses, and convert it to the less readable name it has in the IFC
        schema.

        Parameters
        ----------
        ifcTypeSchema: dict
            The schema of the IFC type to access the attribute of.
        name: str
            The name the attribute has in Arch.

        Returns
        -------
        dict
            Returns the schema of the attribute.
        None
            Returns None if the IFC type does not have the attribute requested.

        """

        for attribute in ifcTypeSchema["attributes"]:
            if attribute["name"].replace(' ', '') == name:
                return attribute
        return None

    def addIfcAttributes(self, ifcTypeSchema, obj):
        """Add the attributes of the IFC type's schema to the object's properties.

        Add the attributes as properties of the object. Also add the
        attribute's schema within the object's IfcData property. Do so using
        the .addIfcAttribute() method.

        Also add expressions to copy data from the object's editable
        properties.  This means the IFC properties will remain accurate with
        the actual values of the object. Do not do so for all IFC properties.
        Do so using the .addIfcAttributeValueExpressions() method.

        Learn more about expressions here:
        https://wiki.freecad.org/Expressions

        Do not add the attribute if the object has a property with the
        attribute's name. Also do not add the attribute if its name is
        RefLatitude, RefLongitude, or Name.

        Parameters
        ----------
        ifcTypeSchema: dict
            The schema of the IFC type.
        """

        for attribute in ifcTypeSchema["attributes"]:
            if attribute["name"] in obj.PropertiesList \
                or attribute["name"] == "RefLatitude" \
                or attribute["name"] == "RefLongitude" \
                or attribute["name"] == "Name":
                continue
            self.addIfcAttribute(obj, attribute)
            self.addIfcAttributeValueExpressions(obj, attribute)

    def addIfcAttribute(self, obj, attribute):
        """Add an IFC type's attribute to the object, within its properties.

        Add the attribute's schema to the object's IfcData property, as an
        item under its "attributes" array.

        Also add the attribute as a property of the object.

        Parameters
        ----------
        attribute: dict
            The attribute to add. Should have the structure of an attribute
            found within the IFC schemas.
        """
        if not hasattr(obj, "IfcData"):
            return
        IfcData = obj.IfcData

        if "attributes" not in IfcData:
            IfcData["attributes"] = "{}"
        IfcAttributes = json.loads(IfcData["attributes"])
        IfcAttributes[attribute["name"]] = attribute
        IfcData["attributes"] = json.dumps(IfcAttributes)

        obj.IfcData = IfcData
        if attribute["is_enum"]:
            obj.addProperty("App::PropertyEnumeration",
                            attribute["name"],
                            "IFC Attributes",
                            QT_TRANSLATE_NOOP("App::Property", "Description of IFC attributes are not yet implemented"))
            setattr(obj, attribute["name"], attribute["enum_values"])
        else:
            propertyType = "App::" + ArchIFCSchema.IfcTypes[attribute["type"]]["property"]
            obj.addProperty(propertyType,
                            attribute["name"],
                            "IFC Attributes",
                            QT_TRANSLATE_NOOP("App::Property", "Description of IFC attributes are not yet implemented"))

    def addIfcAttributeValueExpressions(self, obj, attribute):
        """Add expressions for IFC attributes, so they stay accurate with the object.

        Add expressions to the object that copy data from the editable
        properties of the object. This ensures that the IFC attributes will
        remain accurate with the actual values of the object.

        Currently, add expressions for the following IFC attributes:

        - OverallWidth
        - OverallHeight
        - ElevationWithFlooring
        - Elevation
        - NominalDiameter
        - BarLength
        - RefElevation
        - LongName

        Learn more about expressions here:
        https://wiki.freecad.org/Expressions

        Parameters
        ----------
        attribute: dict
            The schema of the attribute to add the expression for.
        """

        if obj.getGroupOfProperty(attribute["name"]) != "IFC Attributes" \
            or attribute["name"] not in obj.PropertiesList:
            return
        if attribute["name"] == "OverallWidth":
            if "Length" in obj.PropertiesList:
                obj.setExpression("OverallWidth", "Length.Value")
            elif "Width" in obj.PropertiesList:
                obj.setExpression("OverallWidth", "Width.Value")
            elif obj.Shape and (obj.Shape.BoundBox.XLength > obj.Shape.BoundBox.YLength):
                obj.setExpression("OverallWidth", "Shape.BoundBox.XLength")
            elif obj.Shape:
                obj.setExpression("OverallWidth", "Shape.BoundBox.YLength")
        elif attribute["name"] == "OverallHeight":
            if "Height" in obj.PropertiesList:
                obj.setExpression("OverallHeight", "Height.Value")
            else:
                obj.setExpression("OverallHeight", "Shape.BoundBox.ZLength")
        elif attribute["name"] == "ElevationWithFlooring" and "Shape" in obj.PropertiesList:
            obj.setExpression("ElevationWithFlooring", "Shape.BoundBox.ZMin")
        elif attribute["name"] == "Elevation" and "Placement" in obj.PropertiesList:
            obj.setExpression("Elevation", "Placement.Base.z")
        elif attribute["name"] == "NominalDiameter" and "Diameter" in obj.PropertiesList:
            obj.setExpression("NominalDiameter", "Diameter.Value")
        elif attribute["name"] == "BarLength" and "Length" in obj.PropertiesList:
            obj.setExpression("BarLength", "Length.Value")
        elif attribute["name"] == "RefElevation" and "Elevation" in obj.PropertiesList:
            obj.setExpression("RefElevation", "Elevation.Value")
        elif attribute["name"] == "LongName":
            obj.LongName = obj.Label

    def setObjIfcAttributeValue(self, obj, attributeName, value):
        """Change the value of an IFC attribute within the IfcData property's json.

        Parameters
        ----------
        attributeName: str
            The name of the attribute to change.
        value:
            The new value to set.
        """
        IfcData = obj.IfcData
        if "attributes" not in IfcData:
            IfcData["attributes"] = "{}"
        IfcAttributes = json.loads(IfcData["attributes"])
        if isinstance(value, FreeCAD.Units.Quantity):
            value = float(value)
        if not attributeName in IfcAttributes:
            IfcAttributes[attributeName] = {}
        IfcAttributes[attributeName]["value"] = value
        IfcData["attributes"] = json.dumps(IfcAttributes)
        obj.IfcData = IfcData

    def setObjIfcComplexAttributeValue(self, obj, attributeName, value):
        """Changes the value of the complex attribute in the IfcData property JSON.

        Parameters
        ----------
        attributeName: str
            The name of the attribute to change.
        value:
            The new value to set.
        """

        IfcData = obj.IfcData
        IfcAttributes = json.loads(IfcData["complex_attributes"])
        IfcAttributes[attributeName] = value
        IfcData["complex_attributes"] = json.dumps(IfcAttributes)
        obj.IfcData = IfcData

    def getObjIfcComplexAttribute(self, obj, attributeName):
        """Get the value of the complex attribute, as stored in the IfcData JSON.

        Parameters
        ----------
        attributeName: str
            The name of the complex attribute to access.

        Returns
        -------
        The value of the complex attribute.
        """

        return json.loads(obj.IfcData["complex_attributes"])[attributeName]

    def purgeUnusedIfcAttributesFromPropertiesList(self, ifcTypeSchema, obj):
        """Remove properties representing IFC attributes if they no longer appear.

        Remove the property representing an IFC attribute, if it does not
        appear in the schema of the IFC type provided. Also, remove the
        property if its attribute is an enum type, presumably for backwards
        compatibility.

        Learn more about IFC enums here:
        https://standards.buildingsmart.org/IFC/RELEASE/IFC4/FINAL/HTML/schema/chapter-3.htm#enumeration
        """

        for property in obj.PropertiesList:
            if obj.getGroupOfProperty(property) != "IFC Attributes":
                continue
            ifcAttribute = self.getIfcAttributeSchema(ifcTypeSchema, property)
            if ifcAttribute is None or ifcAttribute["is_enum"] is True:
                obj.removeProperty(property)

    def migrateDeprecatedAttributes(self, obj):
        """Update the object to use the newer property names for IFC related properties.
        """

        if "Role" in obj.PropertiesList:
            r = obj.Role
            obj.removeProperty("Role")
            if r in IfcTypes:
                obj.IfcType = r
                FreeCAD.Console.PrintMessage("Upgrading "+obj.Label+" Role property to IfcType\n")

        if "IfcRole" in obj.PropertiesList:
            r = obj.IfcRole
            obj.removeProperty("IfcRole")
            if r in IfcTypes:
                obj.IfcType = r
                FreeCAD.Console.PrintMessage("Upgrading "+obj.Label+" IfcRole property to IfcType\n")

        if "IfcAttributes"in obj.PropertiesList:
            obj.IfcData = obj.IfcAttributes
            obj.removeProperty("IfcAttributes")

class IfcProduct(IfcRoot):
    """This class is subclassed by classes that have a specific location in space.

    The obvious example are actual structures, such as the _Wall class, but it
    also includes the _Floor class, which is just a grouping of all the
    structures that make up one floor of a building.

    You can learn more about how products fit into the IFC schema here:
    https://standards.buildingsmart.org/IFC/RELEASE/IFC4/FINAL/HTML/schema/ifckernel/lexical/ifcproduct.htm
    """

    def getIfcSchema(self):
        """Get the IFC schema of all IFC types that inherit from IfcProducts.

        Returns
        -------
        dict
            The schema of all the types relevant to this class.
        """
        return ArchIFCSchema.IfcProducts

class IfcContext(IfcRoot):
    """This class is subclassed by classes that define a particular context.

    Currently, only the _Project inherits this class.

    You can learn more about how contexts fit into the IFC schema here:
    https://standards.buildingsmart.org/IFC/RELEASE/IFC4/FINAL/HTML/schema/ifckernel/lexical/ifccontext.htm
    """

    def getIfcSchema(self):
        """Get the IFC schema of all IFC types that inherit from IfcContexts.

        Returns
        -------
        dict
            The schema of all the types relevant to this class.
        """
        return ArchIFCSchema.IfcContexts
