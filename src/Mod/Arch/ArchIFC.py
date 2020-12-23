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
            obj.IfcType = self.getCanonicalisedIfcTypes()

        if not "IfcProperties" in obj.PropertiesList:
            obj.addProperty("App::PropertyMap","IfcProperties","IFC",QT_TRANSLATE_NOOP("App::Property","IFC properties of this object"))

        self.migrateDeprecatedAttributes(obj)

    def onChanged(self, obj, prop):
        if prop == "IfcType":
            self.setupIfcAttributes(obj)
            self.setupIfcComplexAttributes(obj)
        if prop in obj.PropertiesList:
            if obj.getGroupOfProperty(prop) == "IFC Attributes":
                self.setObjIfcAttributeValue(obj, prop, obj.getPropertyByName(prop))

    def setupIfcAttributes(self, obj):
        ifcTypeSchema = self.getIfcTypeSchema(obj.IfcType)
        if ifcTypeSchema is None:
            return
        self.purgeUnusedIfcAttributesFromPropertiesList(ifcTypeSchema, obj)
        self.addIfcAttributes(ifcTypeSchema, obj)

    def setupIfcComplexAttributes(self, obj):
        ifcTypeSchema = self.getIfcTypeSchema(obj.IfcType)
        if ifcTypeSchema is None:
            return
        IfcData = obj.IfcData
        if "complex_attributes" not in IfcData:
            IfcData["complex_attributes"] = "{}"
        ifcComplexAttributes = json.loads(IfcData["complex_attributes"])
        for attribute in ifcTypeSchema["complex_attributes"]:
            if attribute["name"] not in ifcComplexAttributes.keys():
                ifcComplexAttributes[attribute["name"]] = {}
        IfcData["complex_attributes"] = json.dumps(ifcComplexAttributes)
        obj.IfcData = IfcData

    def getIfcTypeSchema(self, IfcType):
        name = "Ifc" + IfcType.replace(" ", "")
        if IfcType == "Undefined":
            name = "IfcBuildingElementProxy"
        if name in self.getIfcSchema():
            return self.getIfcSchema()[name]
        return None

    def getIfcSchema(self):
        return {}

    def getCanonicalisedIfcTypes(self):
        schema = self.getIfcSchema()
        return [''.join(map(lambda x: x if x.islower() else " "+x, t[3:]))[1:] for t in schema.keys()]

    def getIfcAttributeSchema(self, ifcTypeSchema, name):
        for attribute in ifcTypeSchema["attributes"]:
            if attribute["name"].replace(' ', '') == name:
                return attribute
        return None

    def addIfcAttributes(self, ifcTypeSchema, obj):
        for attribute in ifcTypeSchema["attributes"]:
            if attribute["name"] in obj.PropertiesList \
                or attribute["name"] == "RefLatitude" \
                or attribute["name"] == "RefLongitude" \
                or attribute["name"] == "Name":
                continue
            self.addIfcAttribute(obj, attribute)
            self.addIfcAttributeValueExpressions(obj, attribute)

    def addIfcAttribute(self, obj, attribute):
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
            obj.addProperty("App::PropertyEnumeration", attribute["name"], "IFC Attributes", QT_TRANSLATE_NOOP("App::Property", "Description of IFC attributes are not yet implemented"))
            setattr(obj, attribute["name"], attribute["enum_values"])
        else:
            import ArchIFCSchema
            propertyType = "App::" + ArchIFCSchema.IfcTypes[attribute["type"]]["property"]
            obj.addProperty(propertyType, attribute["name"], "IFC Attributes", QT_TRANSLATE_NOOP("App::Property", "Description of IFC attributes are not yet implemented"))

    def addIfcAttributeValueExpressions(self, obj, attribute):
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
        IfcData = obj.IfcData
        IfcAttributes = json.loads(IfcData["complex_attributes"])
        IfcAttributes[attributeName] = value
        IfcData["complex_attributes"] = json.dumps(IfcAttributes)
        obj.IfcData = IfcData

    def getObjIfcComplexAttribute(self, obj, attributeName):
        return json.loads(obj.IfcData["complex_attributes"])[attributeName]

    def purgeUnusedIfcAttributesFromPropertiesList(self, ifcTypeSchema, obj):
        for property in obj.PropertiesList:
            if obj.getGroupOfProperty(property) != "IFC Attributes":
                continue
            ifcAttribute = self.getIfcAttributeSchema(ifcTypeSchema, property)
            if ifcAttribute is None or ifcAttribute["is_enum"] is True:
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
    def getIfcSchema(self):
        return ArchIFCSchema.IfcProducts

class IfcContext(IfcRoot):
    def getIfcSchema(self):
        return ArchIFCSchema.IfcContexts
