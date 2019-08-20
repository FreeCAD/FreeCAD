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



def buildRelationships(ifcfile,root_element):
    """Builds different tables from an IFC file"""

    # building relations tables
    # TODO use inverse attributes, see https://forum.freecadweb.org/viewtopic.php?f=39&t=37892
    # done for properties

    objects = {} # { id:object, ... }
    prodrepr = {} # product/representations table
    additions = {} # { host:[child,...], ... }
    groups = {} # { host:[child,...], ... }     # used in structural IFC
    subtractions = [] # [ [opening,host], ... ]
    colors = {} # { id:(r,g,b) }
    shapes = {} # { id:shaoe } only used for merge mode
    structshapes = {} # { id:shaoe } only used for merge mode
    mattable = {} # { objid:matid }
    sharedobjects = {} # { representationmapid:object }
    parametrics = [] # a list of imported objects whose parametric relationships need processing after all objects have been created
    profiles = {} # to store reused extrusion profiles {ifcid:fcobj,...}
    style_material_id = {}  # { style_entity_id: material_id) }

    # gather easy entity types
    sites = ifcfile.by_type("IfcSite")
    buildings = ifcfile.by_type("IfcBuilding")
    floors = ifcfile.by_type("IfcBuildingStorey")
    products = ifcfile.by_type(root_element)
    openings = ifcfile.by_type("IfcOpeningElement")
    annotations = ifcfile.by_type("IfcAnnotation")
    materials = ifcfile.by_type("IfcMaterial")

    for r in ifcfile.by_type("IfcRelContainedInSpatialStructure"):
        additions.setdefault(r.RelatingStructure.id(),[]).extend([e.id() for e in r.RelatedElements])
    for r in ifcfile.by_type("IfcRelAggregates"):
        additions.setdefault(r.RelatingObject.id(),[]).extend([e.id() for e in r.RelatedObjects])
    for r in ifcfile.by_type("IfcRelAssignsToGroup"):
        groups.setdefault(r.RelatingGroup.id(),[]).extend([e.id() for e in r.RelatedObjects])
    for r in ifcfile.by_type("IfcRelVoidsElement"):
        subtractions.append([r.RelatedOpeningElement.id(), r.RelatingBuildingElement.id()])
    for r in ifcfile.by_type("IfcRelAssociatesMaterial"):
        for o in r.RelatedObjects:
            if r.RelatingMaterial.is_a("IfcMaterial"):
                mattable[o.id()] = r.RelatingMaterial.id()
            elif r.RelatingMaterial.is_a("IfcMaterialLayer"):
                mattable[o.id()] = r.RelatingMaterial.Material.id()
            elif r.RelatingMaterial.is_a("IfcMaterialLayerSet"):
                mattable[o.id()] = r.RelatingMaterial.MaterialLayers[0].Material.id()
            elif r.RelatingMaterial.is_a("IfcMaterialLayerSetUsage"):
                mattable[o.id()] = r.RelatingMaterial.ForLayerSet.MaterialLayers[0].Material.id()
    for p in ifcfile.by_type("IfcProduct"):
        if hasattr(p,"Representation"):
            if p.Representation:
                for it in p.Representation.Representations:
                    for it1 in it.Items:
                        prodrepr.setdefault(p.id(),[]).append(it1.id())
                        if it1.is_a("IfcBooleanResult"):
                            prodrepr.setdefault(p.id(),[]).append(it1.FirstOperand.id())
                        elif it.Items[0].is_a("IfcMappedItem"):
                            prodrepr.setdefault(p.id(),[]).append(it1.MappingSource.MappedRepresentation.id())
                            if it1.MappingSource.MappedRepresentation.is_a("IfcShapeRepresentation"):
                                for it2 in it1.MappingSource.MappedRepresentation.Items:
                                    prodrepr.setdefault(p.id(),[]).append(it2.id())
    # colors
    style_color_rgb = {}  # { style_entity_id: (r,g,b) }
    for r in ifcfile.by_type("IfcStyledItem"):
        if r.Styles:
            if r.Styles[0].is_a("IfcPresentationStyleAssignment"):
                for style1 in r.Styles[0].Styles:
                    if style1.is_a("IfcSurfaceStyle"):
                        for style2 in style1.Styles:
                            if style2.is_a("IfcSurfaceStyleRendering"):
                                if style2.SurfaceColour:
                                    c = style2.SurfaceColour
                                    style_color_rgb[r.id()] = (c.Red,c.Green,c.Blue)
        # Nova
        if r.Item:
            for p in prodrepr.keys():
                if r.Item.id() in prodrepr[p]:
                    style_material_id[r.id()] = p

    # Allplan, ArchiCAD
    for m in ifcfile.by_type("IfcMaterialDefinitionRepresentation"):
        for it in m.Representations:
            if it.Items:
                style_material_id[it.Items[0].id()] = m.RepresentedMaterial.id()

    # create colors out of style_color_rgb and style_material_id
    for k in style_material_id:
        if k in style_color_rgb:
            colors[style_material_id[k]] = style_color_rgb[k]

    # remove any leftover annotations from products
    tp = []
    for product in products:
        if product.is_a("IfcGrid") and not (product in annotations):
            annotations.append(product)
        elif not (product in annotations):
            tp.append(product)
    products = sorted(tp,key=lambda prod: prod.id())

    return objects,prodrepr,additions,groups,subtractions,colors,shapes, \
           structshapes,mattable,sharedobjects,parametrics,profiles, \
           sites,buildings,floors,products,openings,annotations,materials, \
           style_material_id


