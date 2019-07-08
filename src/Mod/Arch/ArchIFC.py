import FreeCAD, os, json

"This modules sets up and manages the IFC-related properties, types and attributes of Arch/BIM objects"

if FreeCAD.GuiUp:
    from PySide.QtCore import QT_TRANSLATE_NOOP
else:
    def QT_TRANSLATE_NOOP(ctx,txt):
        return txt

import ArchIFCSchema
IfcTypes = ['Undefined']+[''.join(map(lambda x: x if x.islower() else " "+x, t[3:]))[1:] for t in ArchIFCSchema.IfcProducts.keys()]

def setProperties(obj):

    "Checks and sets all the needed IFC-related properties"

    if not "IfcType" in obj.PropertiesList:
        obj.addProperty("App::PropertyEnumeration","IfcType","IFC",QT_TRANSLATE_NOOP("App::Property","The type of this object"))
        obj.IfcType = IfcTypes

    if not "IfcData" in obj.PropertiesList:
        obj.addProperty("App::PropertyMap","IfcData","IFC",QT_TRANSLATE_NOOP("App::Property","IFC data"))

    if not "IfcProperties" in obj.PropertiesList:
        obj.addProperty("App::PropertyMap","IfcProperties","IFC",QT_TRANSLATE_NOOP("App::Property","IFC properties of this object"))

    migrateDeprecatedAttributes(obj)

def onChanged(obj, prop):

    "Called by Arch object's OnChanged method"

    if prop == "IfcType":
        setupIfcAttributes(obj)
    if obj.getGroupOfProperty(prop) == "IFC Attributes":
        setObjIfcAttributeValue(obj, prop, obj.getPropertyByName(prop))

def getIfcProduct(IfcType):
    
    "Returns an IFC product name from an obj.IfcType"
    
    name = "Ifc" + IfcType.replace(" ", "")
    if IfcType == "Undefined":
        name = "IfcBuildingElementProxy"
    if name in ArchIFCSchema.IfcProducts:
        return ArchIFCSchema.IfcProducts[name]
    return None

def getIfcProductAttribute(ifcProduct, name):
    
    "Returns the attributes of a given product"

    for attribute in ifcProduct["attributes"]:
        if attribute["name"].replace(' ', '') == name:
            return attribute
    return None

def setupIfcAttributes(obj):

    "Sets the necessary IFC attribute properties for an object"

    ifcProduct = getIfcProduct(obj.IfcType)
    if ifcProduct is None:
        return
    purgeUnusedIfcAttributesFromPropertiesList(ifcProduct, obj)
    addIfcProductAttributesToObj(ifcProduct, obj)

def addIfcProductAttributesToObj(ifcProduct, obj):

    "Adds the necessary attribute properties to an object"

    for attribute in ifcProduct["attributes"]:
        if attribute["name"] in obj.PropertiesList \
            or attribute["name"] == "RefLatitude" \
            or attribute["name"] == "RefLongitude":
            continue
        addIfcProductAttribute(obj, attribute)
        addIfcAttributeValueExpressions(obj, attribute)

def addIfcProductAttribute(obj, attribute):
    
    "Adds a given attribute property"
    
    if not hasattr(obj,"IfcData"):
        return
    IfcData = obj.IfcData
    if "attributes" not in IfcData:
        IfcData["attributes"] = "{}"
    IfcAttributes = json.loads(IfcData["attributes"])
    IfcAttributes[attribute["name"]] = attribute
    IfcData["attributes"] = json.dumps(IfcAttributes)
    obj.IfcData = IfcData
    if attribute["is_enum"]:
        obj.addProperty("App::PropertyEnumeration", attribute["name"], "IFC Attributes", QT_TRANSLATE_NOOP("App::Property", "Description of IFC attributes are not yet implemented"))
        setattr(obj, attribute["name"], attribute["enum_values"])
    else:
        import ArchIFCSchema
        propertyType = "App::" + ArchIFCSchema.IfcTypes[attribute["type"]]["property"]
        obj.addProperty(propertyType, attribute["name"], "IFC Attributes", QT_TRANSLATE_NOOP("App::Property", "Description of IFC attributes are not yet implemented"))

def addIfcAttributeValueExpressions(obj, attribute):
    
    "Binds the given attribute properties with expressions"

    if not attribute["name"] in obj.PropertiesList:
        return
    if obj.getGroupOfProperty(attribute["name"]) != "IFC Attributes":
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
    elif attribute["name"] == "ElevationWithFlooring":
        if "Shape" in obj.PropertiesList:
            obj.setExpression("ElevationWithFlooring", "Shape.BoundBox.ZMin")
    elif attribute["name"] == "Elevation":
        if "Placement" in obj.PropertiesList:
            obj.setExpression("Elevation", "Placement.Base.z")
    elif attribute["name"] == "NominalDiameter":
        if "Diameter" in obj.PropertiesList:
            obj.setExpression("NominalDiameter", "Diameter.Value")
    elif attribute["name"] == "BarLength":
        if "Length" in obj.PropertiesList:
            obj.setExpression("BarLength", "Length.Value")
    elif attribute["name"] == "RefElevation":
        if "Elevation" in obj.PropertiesList:
            obj.setExpression("RefElevation", "Elevation.Value")
    elif attribute["name"] == "LongName":
        obj.LongName = obj.Label

def setObjIfcAttributeValue(obj, attributeName, value):
    
    "Sets the value of a given attribute property"
    
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

def purgeUnusedIfcAttributesFromPropertiesList(ifcProduct, obj):
    
    "Removes unused attribute properties"
    
    for property in obj.PropertiesList:
        if obj.getGroupOfProperty(property) != "IFC Attributes":
            continue
        ifcProductAttribute = getIfcProductAttribute(ifcProduct, property)
        if ifcProductAttribute is None or ifcProductAttribute["is_enum"] is True:
            obj.removeProperty(property)

def migrateDeprecatedAttributes(obj):
    
    "Fixes obsolete properties"

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
