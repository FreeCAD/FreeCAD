import FreeCAD, os, json

"This modules sets up and manages the IFC-related properties, types and attributes of Arch/BIM objects"

if FreeCAD.GuiUp:
    from PySide.QtCore import QT_TRANSLATE_NOOP
else:
    def QT_TRANSLATE_NOOP(ctx,txt):
        return txt

import ArchIFCSchema
IfcTypes = [''.join(map(lambda x: x if x.islower() else " "+x, t[3:]))[1:] for t in ArchIFCSchema.IfcProducts.keys()]

class IfcRoot:
    def setProperties(self, obj):
        if not "IfcData" in obj.PropertiesList:
            obj.addProperty("App::PropertyMap","IfcData","IFC",QT_TRANSLATE_NOOP("App::Property","IFC data"))

        if not "IfcType" in obj.PropertiesList:
            obj.addProperty("App::PropertyEnumeration","IfcType","IFC",QT_TRANSLATE_NOOP("App::Property","The type of this object"))
            obj.IfcType = self.canonicaliseIfcTypes(self.getIfcTypes())


        if not "IfcProperties" in obj.PropertiesList:
            obj.addProperty("App::PropertyMap","IfcProperties","IFC",QT_TRANSLATE_NOOP("App::Property","IFC properties of this object"))

        self.migrateDeprecatedAttributes(obj)

    def onChanged(self, obj, prop):
        if prop == "IfcType":
            print('setting up ifc attrs')
            self.setupIfcAttributes(obj)
        if obj.getGroupOfProperty(prop) == "IFC Attributes":
            self.setObjIfcAttributeValue(obj, prop, obj.getPropertyByName(prop))

    def getIfcTypes(self):
        return {}

    def canonicaliseIfcTypes(self, IfcTypes):
        return [''.join(map(lambda x: x if x.islower() else " "+x, t[3:]))[1:] for t in IfcTypes.keys()]

    def getIfcProduct(self, IfcType):
        name = "Ifc" + IfcType.replace(" ", "")
        if name in self.getIfcTypes():
            return self.getIfcTypes()[name]

    def getIfcProductAttribute(self, ifcProduct, name):
        for attribute in ifcProduct["attributes"]:
            if attribute["name"].replace(' ', '') == name:
                return attribute
        return None

    def setupIfcAttributes(self, obj):
        ifcProduct = self.getIfcProduct(obj.IfcType)
        if ifcProduct is None:
            return
        self.purgeUnusedIfcAttributesFromPropertiesList(ifcProduct, obj)
        self.addIfcProductAttributesToObj(ifcProduct, obj)

    def addIfcProductAttributesToObj(self, ifcProduct, obj):
        for attribute in ifcProduct["attributes"]:
            if attribute["name"] in obj.PropertiesList \
                or attribute["name"] == "RefLatitude" \
                or attribute["name"] == "RefLongitude":
                continue
            self.addIfcProductAttribute(obj, attribute)
            self.addIfcAttributeValueExpressions(obj, attribute)

    def addIfcProductAttribute(self, obj, attribute):
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

    def addIfcAttributeValueExpressions(self, obj, attribute):
        if obj.getGroupOfProperty(attribute["name"]) != "IFC Attributes":
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
        elif attribute["name"] == "LongName":
            obj.LongName = obj.Label

    def setObjIfcAttributeValue(self, obj, attributeName, value):
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

    def purgeUnusedIfcAttributesFromPropertiesList(self, ifcProduct, obj):
        for property in obj.PropertiesList:
            if obj.getGroupOfProperty(property) != "IFC Attributes":
                continue
            ifcProductAttribute = self.getIfcProductAttribute(ifcProduct, property)
            if ifcProductAttribute is None or ifcProductAttribute["is_enum"] is True:
                obj.removeProperty(property)

    def migrateDeprecatedAttributes(self, obj):
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
    def getIfcTypes(self):
        return ArchIFCSchema.IfcProducts

class IfcContext(IfcRoot):
    def getIfcTypes(self):
        return ArchIFCSchema.IfcContexts
