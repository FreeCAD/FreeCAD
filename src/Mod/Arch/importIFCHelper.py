import FreeCAD
import Arch
import ArchIFC
import math
import six


class ProjectImporter:
    """A helper class to create a FreeCAD Arch Project object"""

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


def getRelProperties(ifcfile):
    """Builds and returns a dictionary of {object:[properties]} from an IFC file"""

    # this method no longer used by this importer module
    # but this relation table might be useful anyway for other purposes

    properties = {} # { objid : { psetid : [propertyid, ... ], ... }, ... }
    for r in ifcfile.by_type("IfcRelDefinesByProperties"):
        for obj in r.RelatedObjects:
            if not obj.id() in properties:
                properties[obj.id()] = {}
            psets = {}
            props = []
            if r.RelatingPropertyDefinition.is_a("IfcPropertySet"):
                props.extend([prop.id() for prop in r.RelatingPropertyDefinition.HasProperties])
                psets[r.RelatingPropertyDefinition.id()] = props
                properties[obj.id()].update(psets)
    return properties


def getIfcPropertySets(ifcfile, pid):
    """Returns a dicionary of {pset_id:[prop_id, prop_id...]} for an IFC object"""

    # get psets for this pid
    psets = {}
    for rel in ifcfile[pid].IsDefinedBy:
        # the following if condition is needed in IFC2x3 only
        # https://forum.freecadweb.org/viewtopic.php?f=39&t=37892#p322884
        if rel.is_a('IfcRelDefinesByProperties'):
            props = []
            if rel.RelatingPropertyDefinition.is_a("IfcPropertySet"):
                props.extend([prop.id() for prop in rel.RelatingPropertyDefinition.HasProperties])
                psets[rel.RelatingPropertyDefinition.id()] = props
    return psets


def getScaling(ifcfile):
    """returns a scaling factor from file units to mm"""

    def getUnit(unit):
        if unit.Name == "METRE":
            if unit.Prefix == "KILO":
                return 1000000.0
            elif unit.Prefix == "HECTO":
                return 100000.0
            elif unit.Prefix == "DECA":
                return 10000.0
            elif not unit.Prefix:
                return 1000.0
            elif unit.Prefix == "DECI":
                return 100.0
            elif unit.Prefix == "CENTI":
                return 10.0
        return 1.0

    ua = ifcfile.by_type("IfcUnitAssignment")
    if not ua:
        return 1.0
    ua = ua[0]
    for u in ua.Units:
        if u.UnitType == "LENGTHUNIT":
            if u.is_a("IfcConversionBasedUnit"):
                f =  getUnit(u.ConversionFactor.UnitComponent)
                return f * u.ConversionFactor.ValueComponent.wrappedValue
            elif u.is_a("IfcSIUnit") or u.is_a("IfcUnit"):
                return getUnit(u)
    return 1.0


def getRotation(entity):
    """returns a FreeCAD rotation from an IfcProduct with a IfcMappedItem representation"""
    try:
        u = FreeCAD.Vector(entity.Axis1.DirectionRatios)
        v = FreeCAD.Vector(entity.Axis2.DirectionRatios)
        w = FreeCAD.Vector(entity.Axis3.DirectionRatios)
    except AttributeError:
        return FreeCAD.Rotation()
    import WorkingPlane
    p = WorkingPlane.plane(u=u,v=v,w=w)
    return p.getRotation().Rotation


def getPlacement(entity,scaling=1000):
    """returns a placement from the given entity"""

    if not entity:
        return None
    import DraftVecUtils
    pl = None
    if entity.is_a("IfcAxis2Placement3D"):
        x = getVector(entity.RefDirection,scaling)
        z = getVector(entity.Axis,scaling)
        if x and z:
            y = z.cross(x)
            m = DraftVecUtils.getPlaneRotation(x,y,z)
            pl = FreeCAD.Placement(m)
        else:
            pl = FreeCAD.Placement()
        loc = getVector(entity.Location,scaling)
        if loc:
            pl.move(loc)
    elif entity.is_a("IfcLocalPlacement"):
        pl = getPlacement(entity.PlacementRelTo,1) # original placement
        relpl = getPlacement(entity.RelativePlacement,1) # relative transf
        if pl and relpl:
            pl = pl.multiply(relpl)
        elif relpl:
            pl = relpl
    elif entity.is_a("IfcCartesianPoint"):
        loc = getVector(entity,scaling)
        pl = FreeCAD.Placement()
        pl.move(loc)
    if pl:
        pl.Base = FreeCAD.Vector(pl.Base).multiply(scaling)
    return pl


def getVector(entity,scaling=1000):
    """returns a vector from the given entity"""

    if not entity:
        return None
    v = None
    if entity.is_a("IfcDirection"):
        if len(entity.DirectionRatios) == 3:
            v= FreeCAD.Vector(tuple(entity.DirectionRatios))
        else:
            v = FreeCAD.Vector(tuple(entity.DirectionRatios+[0]))
    elif entity.is_a("IfcCartesianPoint"):
        if len(entity.Coordinates) == 3:
            v = FreeCAD.Vector(tuple(entity.Coordinates))
        else:
            v = FreeCAD.Vector(tuple(entity.Coordinates+[0]))
    #if v:
    #    v.multiply(scaling)
    return v


def get2DShape(representation,scaling=1000):
    """Returns a shape from a 2D IfcShapeRepresentation"""

    import Part
    import DraftVecUtils
    import Draft

    def getPolyline(ent):
        pts = []
        for p in ent.Points:
            c = p.Coordinates
            c = FreeCAD.Vector(c[0],c[1],c[2] if len(c) > 2 else 0)
            c.multiply(scaling)
            pts.append(c)
        return Part.makePolygon(pts)

    def getLine(ent):
        pts = []
        p1 = getVector(ent.Pnt)
        p1.multiply(scaling)
        pts.append(p1)
        p2 = getVector(ent.Dir)
        p2.multiply(scaling)
        p2 = p1.add(p2)
        pts.append(p2)
        return Part.makePolygon(pts)

    def getCircle(ent):
        c = ent.Position.Location.Coordinates
        c = FreeCAD.Vector(c[0],c[1],c[2] if len(c) > 2 else 0)
        c.multiply(scaling)
        r = ent.Radius*scaling
        return Part.makeCircle(r,c)

    def getCurveSet(ent):
        result = []
        if ent.is_a() in ["IfcGeometricCurveSet","IfcGeometricSet"]:
            elts = ent.Elements
        elif ent.is_a() in ["IfcLine","IfcPolyline","IfcCircle","IfcTrimmedCurve"]:
            elts = [ent]
        for el in elts:
            if el.is_a("IfcPolyline"):
                result.append(getPolyline(el))
            elif el.is_a("IfcLine"):
                result.append(getLine(el))
            elif el.is_a("IfcCircle"):
                result.append(getCircle(el))
            elif el.is_a("IfcTrimmedCurve"):
                base = el.BasisCurve
                t1 = el.Trim1[0].wrappedValue
                t2 = el.Trim2[0].wrappedValue
                if not el.SenseAgreement:
                    t1,t2 = t2,t1
                if base.is_a("IfcPolyline"):
                    bc = getPolyline(base)
                    result.append(bc)
                elif base.is_a("IfcCircle"):
                    bc = getCircle(base)
                    e = Part.ArcOfCircle(bc.Curve,math.radians(t1),math.radians(t2)).toShape()
                    d = base.Position.RefDirection.DirectionRatios
                    v = FreeCAD.Vector(d[0],d[1],d[2] if len(d) > 2 else 0)
                    a = -DraftVecUtils.angle(v)
                    e.rotate(bc.Curve.Center,FreeCAD.Vector(0,0,1),math.degrees(a))
                    result.append(e)
            elif el.is_a("IfcCompositeCurve"):
                for base in el.Segments:
                    if base.ParentCurve.is_a("IfcPolyline"):
                        bc = getPolyline(base.ParentCurve)
                        result.append(bc)
                    elif base.ParentCurve.is_a("IfcCircle"):
                        bc = getCircle(base.ParentCurve)
                        e = Part.ArcOfCircle(bc.Curve,math.radians(t1),math.radians(t2)).toShape()
                        d = base.Position.RefDirection.DirectionRatios
                        v = FreeCAD.Vector(d[0],d[1],d[2] if len(d) > 2 else 0)
                        a = -DraftVecUtils.angle(v)
                        e.rotate(bc.Curve.Center,FreeCAD.Vector(0,0,1),math.degrees(a))
                        result.append(e)

        return result

    result = []
    if representation.is_a("IfcShapeRepresentation"):
        for item in representation.Items:
            if item.is_a() in ["IfcGeometricCurveSet","IfcGeometricSet"]:
                result = getCurveSet(item)
            elif item.is_a("IfcMappedItem"):
                preresult = setRepresentation(item.MappingSource.MappedRepresentation,scaling)
                pla = getPlacement(item.MappingSource.MappingOrigin,scaling)
                rot = getRotation(item.MappingTarget)
                if pla:
                    if rot.Angle:
                        pla.Rotation = rot
                    for r in preresult:
                        #r.Placement = pla
                        result.append(r)
                else:
                    result = preresult
            elif item.is_a("IfcTextLiteral"):
                t = Draft.makeText([item.Literal],point=getPlacement(item.Placement,scaling).Base)
                return t # dirty hack... Object creation should not be done here
    elif representation.is_a() in ["IfcPolyline","IfcCircle","IfcTrimmedCurve"]:
        result = getCurveSet(representation)
    return result


def getIfcProperties(ifcfile, pid, psets, d):
    """builds valid property values for FreeCAD"""

    for pset in psets.keys():
        #print("reading pset: ",pset)
        psetname = ifcfile[pset].Name
        if six.PY2:
            psetname = psetname.encode("utf8")
        for prop in psets[pset]:
            e = ifcfile[prop]
            pname = e.Name
            if six.PY2:
                pname = pname.encode("utf8")
            if e.is_a("IfcPropertySingleValue"):
                if e.NominalValue:
                    ptype = e.NominalValue.is_a()
                    if ptype in ['IfcLabel','IfcText','IfcIdentifier','IfcDescriptiveMeasure']:
                        pvalue = e.NominalValue.wrappedValue
                        if six.PY2:
                            pvalue = pvalue.encode("utf8")
                    else:
                        pvalue = str(e.NominalValue.wrappedValue)
                    if hasattr(e.NominalValue,'Unit'):
                        if e.NominalValue.Unit:
                            pvalue += e.NominalValue.Unit
                    d[pname+";;"+psetname] = ptype+";;"+pvalue
                #print("adding property: ",pname,ptype,pvalue," pset ",psetname)
    return d
