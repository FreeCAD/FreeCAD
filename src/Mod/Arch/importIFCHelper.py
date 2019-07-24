import Arch, ArchIFC, math

class ProjectImporter:
    def __init__(self, file, objects):
        self.file = file
        self.objects = objects

    def execute(self):
        self.project = self.file.by_type("IfcProject")[0]
        self.object = Arch.makeProject()
        self.objects[self.project.id()] = self.object
        self.setAttributes()
        self.setComplexAttributes()

    def setAttributes(self):
        for property in self.object.PropertiesList:
            if hasattr(self.project, property) and getattr(self.project, property):
                setattr(self.object, property, getattr(self.project, property))

    def setComplexAttributes(self):
        try:
            mapConversion = self.project.RepresentationContexts[0].HasCoordinateOperation[0]
            
            data = self.extractTargetCRSData(mapConversion.TargetCRS)
            data.update(self.extractMapConversionData(mapConversion))
            ArchIFC.IfcRoot.setObjIfcComplexAttributeValue(self, self.object, "RepresentationContexts", data)
        except:
            # This scenario occurs validly in IFC2X3, as the mapConversion does
            # not exist
            return

    def extractTargetCRSData(self, targetCRS):
        mappings = {
            "name": "Name",
            "description": "Description",
            "geodetic_datum": "GeodeticDatum",
            "vertical_datum": "VerticalDatum",
            "map_projection": "MapProjection",
            "map_zone": "MapZone"
        }
        data = {}
        for attributeName, ifcName in mappings.items():
            data[attributeName] = str(getattr(targetCRS, ifcName))

        if targetCRS.MapUnit.Prefix:
            data["map_unit"] = targetCRS.MapUnit.Prefix.title() + targetCRS.MapUnit.Name.lower()
        else:
            data["map_unit"] = targetCRS.MapUnit.Name.title()

        return data

    def extractMapConversionData(self, mapConversion):
        mappings = {
            "eastings": "Eastings",
            "northings": "Northings",
            "orthogonal_height": "OrthogonalHeight",
            "x_axis_abscissa": "XAxisAbscissa",
            "x_axis_ordinate": "XAxisOrdinate",
            "scale": "Scale"
        }
        data = {}
        for attributeName, ifcName in mappings.items():
            data[attributeName] = str(getattr(mapConversion, ifcName))

        data["true_north"] = str(self.calculateTrueNorthAngle(
            mapConversion.XAxisAbscissa, mapConversion.XAxisOrdinate))
        return data

    def calculateTrueNorthAngle(self, x, y):
        return round(math.degrees(math.atan2(y, x)) - 90, 6)
