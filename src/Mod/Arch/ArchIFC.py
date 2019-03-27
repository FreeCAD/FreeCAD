import FreeCAD, os, json

if FreeCAD.GuiUp:
    from PySide.QtCore import QT_TRANSLATE_NOOP

def setProperties(obj):
    if not "IfcData" in obj.PropertiesList:
        obj.addProperty("App::PropertyMap","IfcData","Component",QT_TRANSLATE_NOOP("App::Property","IFC data"))
    migrateDeprecatedAttributes(obj)

def onChanged(obj, prop):
    if prop == "IfcRole":
        setupIfcAttributes(obj)
    if obj.getGroupOfProperty(prop) == "IFC Attributes":
        setObjIfcAttributeValue(obj, prop, obj.getPropertyByName(prop))

def getIfcProduct(IfcRole):
    import ArchIFCSchema
    name = "Ifc" + IfcRole.replace(" ", "")
    if name in ArchIFCSchema.IfcProducts:
        return ArchIFCSchema.IfcProducts[name]

def getIfcProductAttribute(ifcProduct, name):
    for attribute in ifcProduct["attributes"]:
        if attribute["name"].replace(' ', '') == name:
            return attribute
    return None

def setupIfcAttributes(obj):
    ifcProduct = getIfcProduct(obj.IfcRole)
    if ifcProduct is None:
        return
    purgeUnusedIfcAttributesFromPropertiesList(ifcProduct, obj)
    addIfcProductAttributesToObj(ifcProduct, obj)

def addIfcProductAttributesToObj(ifcProduct, obj):
    for attribute in ifcProduct["attributes"]:
        if attribute["name"] in obj.PropertiesList \
            or attribute["name"] == "RefLatitude" \
            or attribute["name"] == "RefLongitude":
            continue
        addIfcProductAttribute(obj, attribute)
        addIfcAttributeValueExpressions(obj, attribute)

def addIfcProductAttribute(obj, attribute):
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
        propertyType = "App::" + ArchIFCSchema.IfcTypes[attribute["type"]]["property"]
        obj.addProperty(propertyType, attribute["name"], "IFC Attributes", QT_TRANSLATE_NOOP("App::Property", "Description of IFC attributes are not yet implemented"))

def addIfcAttributeValueExpressions(obj, attribute):
    if obj.getGroupOfProperty(attribute["name"]) != "Ifc Attributes":
        return
    if attribute["name"] == "OverallWidth":
        if hasattr(obj, "Length"):
            obj.setExpression("OverallWidth", "Length.Value")
        elif hasattr(obj, "Width"):
            obj.setExpression("OverallWidth", "Width.Value")
        elif obj.Shape.BoundBox.XLength > obj.Shape.BoundBox.YLength:
            obj.setExpression("OverallWidth", "Shape.BoundBox.XLength")
        else:
            obj.setExpression("OverallWidth", "Shape.BoundBox.YLength")
    elif attribute["name"] == "OverallHeight":
        if hasattr(obj, "Height"):
            obj.setExpression("OverallHeight", "Height.Value")
        else:
            obj.setExpression("OverallHeight", "Shape.BoundBox.ZLength")
    elif attribute["name"] == "ElevationWithFlooring":
        obj.setExpression("ElevationWithFlooring", "Shape.BoundBox.ZMin")
    elif attribute["name"] == "Elevation":
        obj.setExpression("Elevation", "Placement.Base.z")
    elif attribute["name"] == "NominalDiameter":
        obj.setExpression("NominalDiameter", "Diameter.Value")
    elif attribute["name"] == "BarLength":
        obj.setExpression("BarLength", "Length.Value")
    elif attribute["name"] == "RefElevation":
        obj.setExpression("RefElevation", "Elevation.Value")

def setObjIfcAttributeValue(obj, attributeName, value):
    IfcData = obj.IfcData
    IfcAttributes = json.loads(IfcData["attributes"])
    if isinstance(value, FreeCAD.Units.Quantity):
        value = float(value)
    IfcAttributes[attributeName]["value"] = value
    IfcData["attributes"] = json.dumps(IfcAttributes)
    obj.IfcData = IfcData

def purgeUnusedIfcAttributesFromPropertiesList(ifcProduct, obj):
    for property in obj.PropertiesList:
        if obj.getGroupOfProperty(property) != "IFC Attributes":
            continue
        ifcProductAttribute = getIfcProductAttribute(ifcProduct, property)
        if ifcProductAttribute is None or ifcProductAttribute["is_enum"] is True:
            obj.removeProperty(property)

def migrateDeprecatedAttributes(obj):
    # FreeCAD <= 0.17 stored IFC data in IfcAttributes
    if hasattr(obj, "IfcAttributes"):
        obj.IfcData = obj.IfcAttributes
        obj.removeProperty("IfcAttributes")
