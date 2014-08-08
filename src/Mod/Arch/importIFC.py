#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2014                                                    *  
#*   Yorik van Havre <yorik@uncreated.net>                                 *  
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

__title__=   "FreeCAD IFC importer - Enhanced ifcopenshell-only version"
__author__ = "Yorik van Havre"
__url__ =    "http://www.freecadweb.org"

import os,time,tempfile,uuid,FreeCAD,Part,Draft,Arch

if open.__module__ == '__builtin__':
    pyopen = open # because we'll redefine open below

typesmap = { "Site":       ["IfcSite"],
             "Building":   ["IfcBuilding"], 
             "Floor":      ["IfcBuildingStorey"],
             "Structure":  ["IfcBeam", "IfcBeamStandardCase", "IfcColumn", "IfcColumnStandardCase", "IfcSlab", "IfcFooting", "IfcPile", "IfcTendon"],
             "Wall":       ["IfcWall", "IfcWallStandardCase", "IfcCurtainWall"],
             "Window":     ["IfcWindow", "IfcWindowStandardCase", "IfcDoor", "IfcDoorStandardCase"],
             "Roof":       ["IfcRoof"],
             "Stairs":     ["IfcStair", "IfcStairFlight", "IfcRamp", "IfcRampFlight"],
             "Space":      ["IfcSpace"],
             "Rebar":      ["IfcReinforcingBar"]
           }

ifctemplate = """ISO-10303-21;
HEADER;
FILE_DESCRIPTION(('ViewDefinition [CoordinationView]'),'2;1');
FILE_NAME('$filename','$timestamp',('$owner','$email'),('$company'),'IfcOpenShell','IfcOpenShell','');
FILE_SCHEMA(('IFC2X3'));
ENDSEC;
DATA;
#1=IFCPERSON($,$,'$owner',$,$,$,$,$);
#2=IFCORGANIZATION($,'$company',$,$,$);
#3=IFCPERSONANDORGANIZATION(#1,#2,$);
#4=IFCAPPLICATION(#2,'$version','FreeCAD','118df2cf_ed21_438e_a41');
#5=IFCOWNERHISTORY(#3,#4,$,.ADDED.,$,#3,#4,$now);
#6=IFCDIRECTION((1.,0.,0.));
#7=IFCDIRECTION((0.,0.,1.));
#8=IFCCARTESIANPOINT((0.,0.,0.));
#9=IFCAXIS2PLACEMENT3D(#8,#7,#6);
#10=IFCDIRECTION((0.,1.,0.));
#11=IFCGEOMETRICREPRESENTATIONCONTEXT('Plan','Model',3,1.E-05,#9,#10);
#12=IFCDIMENSIONALEXPONENTS(0,0,0,0,0,0,0);
#13=IFCSIUNIT(*,.LENGTHUNIT.,.MILLI.,.METRE.);
#14=IFCSIUNIT(*,.AREAUNIT.,$,.SQUARE_METRE.);
#15=IFCSIUNIT(*,.VOLUMEUNIT.,$,.CUBIC_METRE.);
#16=IFCSIUNIT(*,.PLANEANGLEUNIT.,$,.RADIAN.);
#17=IFCMEASUREWITHUNIT(IFCPLANEANGLEMEASURE(0.01745),#16);
#18=IFCCONVERSIONBASEDUNIT(#12,.PLANEANGLEUNIT.,'DEGREE',#17);
#19=IFCUNITASSIGNMENT((#13,#14,#15,#18));
#20=IFCPROJECT('$projectid',#5,'$project',$,$,$,$,(#11),#19);
ENDSEC;
END-ISO-10303-21;
"""

ifctypes = ["IfcSite", "IfcBuilding", "IfcBuildingStorey", "IfcBeam", "IfcBeamStandardCase",
            "IfcChimney", "IfcColumn", "IfcColumnStandardCase", "IfcCovering", "IfcCurtainWall",
            "IfcDoor", "IfcDoorStandardCase", "IfcMember", "IfcMemberStandardCase", "IfcPlate",
            "IfcPlateStandardCase", "IfcRailing", "IfcRamp", "IfcRampFlight", "IfcRoof",
            "IfcSlab", "IfcStair", "IfcStairFlight", "IfcWall","IfcSpace",
            "IfcWallStandardCase", "IfcWindow", "IfcWindowStandardCase", "IfcBuildingElementProxy",
            "IfcPile", "IfcFooting", "IfcReinforcingBar", "IfcTendon", "IfcGroup"]


def explore(filename=None):
    "opens a dialog showing the contents of an IFC file"
    
    if not filename:
        from PySide import QtGui
        filename = QtGui.QFileDialog.getOpenFileName(QtGui.qApp.activeWindow(),'IFC files','*.ifc')
        if filename:
            filename = filename[0]
    if filename:
        import importIFClegacy
        importIFClegacy.getConfig()
        schema=importIFClegacy.getSchema()
        importIFClegacy.DEBUG = DEBUG
        d = importIFClegacy.explorer(filename,schema)
        d.show()
        return d
    return


def open(filename,skip=[]):
    "opens an IFC file in a new document"
    
    docname = os.path.splitext(os.path.basename(filename))[0]
    doc = FreeCAD.newDocument(docname)
    doc.Label = docname
    doc = insert(filename,doc.Name,skip)
    return doc


def insert(filename,docname,skip=[]):
    "imports the contents of an IFC file"
    
    try:
        import ifcopenshell
    except:
        if DEBUG: print "using legacy importer"
        import importIFClegacy
        return importIFClegacy.insert(filename,docname,skip)

    p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Arch")
    DEBUG = p.GetBool("ifcDebug",False)
    PREFIX_NUMBERS = p.GetBool("ifcPrefixNumbers",False)
    SKIP = p.GetString("ifcSkip","")
    SEPARATE_OPENINGS = p.GetBool("ifcSeparateOpenings",False)
    SCALE = p.GetFloat("IfcScalingFactor",1.0)

    if DEBUG: print "opening ",filename,"..."
    try:
        doc = FreeCAD.getDocument(docname)
    except:
        doc = FreeCAD.newDocument(docname)
    FreeCAD.ActiveDocument = doc
    
    global ifcfile # keeping global for debugging purposes
    ifcopenshell.clean()
    ifcfile = ifcopenshell.open(filename)
    shape_attributes = ifcopenshell.SEW_SHELLS
    if SEPARATE_OPENINGS: shape_attributes += ifcopenshell.DISABLE_OPENING_SUBTRACTIONS
    sites = ifcfile.by_type("IfcSite")
    buildings = ifcfile.by_type("IfcBuilding")
    floors = ifcfile.by_type("IfcBuildingStorey")
    products = ifcfile.by_type("IfcProduct")
    openings = ifcfile.by_type("IfcOpeningElement")

    # building relations tables
    objects = {} # { id:object, ... }
    additions = {} # { host:[child,...], ... }
    subtractions = [] # [ [opening,host], ... ]
    for r in ifcfile.by_type("IfcRelContainedInSpatialStructure"):
        additions.setdefault(r.RelatingStructure.id(),[]).extend([e.id() for e in r.RelatedElements])
    for r in ifcfile.by_type("IfcRelAggregates"):
        additions.setdefault(r.RelatingObject.id(),[]).extend([e.id() for e in r.RelatedObjects])
    for r in ifcfile.by_type("IfcRelVoidsElement"):
        subtractions.append([r.RelatedOpeningElement.id(), r.RelatingBuildingElement.id()])

    # products
    for product in products:
        pid = product.id()
        guid = product.GlobalId
        ptype = product.is_a()
        name = product.Name or str(ptype[3:])
        if PREFIX_NUMBERS: name = "ID" + str(pid) + " " + name
        obj = None
        baseobj = None
        
        if (ptype == "IfcOpeningElement") and (not SEPARATE_OPENINGS): break
        if pid in skip: break
        if ptype in SKIP: break
        
        brep = ifcopenshell.create_shape(product,shape_attributes)
        if brep:
            shape = Part.Shape()
            shape.importBrepFromString(brep)
            if SCALE != 1:
                shape.scale(SCALE)
            if not shape.isNull():
                baseobj = FreeCAD.ActiveDocument.addObject("Part::Feature",name+"_body")
                baseobj.Shape = shape
        for freecadtype,ifctypes in typesmap.iteritems():
            if ptype in ifctypes:
                obj = getattr(Arch,"make"+freecadtype)(baseobj=baseobj,name=name)
                obj.Label = name
                # setting uid
                if hasattr(obj,"IfcAttributes"):
                    a = obj.IfcAttributes
                    a["IfcUID"] = str(guid)
                    obj.IfcAttributes = a
                break
        if not obj:
            obj = baseobj
        if obj:
            sh = baseobj.Shape.ShapeType if hasattr(baseobj,"Shape") else "None"
            sols = str(baseobj.Shape.Solids) if hasattr(baseobj,"Shape") else ""
            if DEBUG: print "creating object ",pid," : ",ptype, " with shape: ",sh," ",sols
            objects[pid] = obj

    # subtractions
    if SEPARATE_OPENINGS:
        for subtraction in subtractions:
            if (subtraction[0] in objects.keys()) and (subtraction[1] in objects.keys()):
                    Arch.removeComponents(objects[subtraction[0]],objects[subtraction[1]])

    # additions
    for host,children in additions.iteritems():
        if host in objects.keys():
            cobs = [objects[child] for child in children if child in objects.keys()]
            if cobs:
                Arch.addComponents(cobs,objects[host])

    FreeCAD.ActiveDocument.recompute()
    
    # cleaning bad shapes
    for obj in objects.values():
        if obj.isDerivedFrom("Part::Feature"):
            if obj.Shape.isNull():
                Arch.rebuildArchShape(obj)
    FreeCAD.ActiveDocument.recompute()
    
    if FreeCAD.GuiUp:
        import FreeCADGui
        FreeCADGui.SendMsgToActiveView("ViewFit")
    return doc


def export(exportList,filename):
    "exports FreeCAD contents to an IFC file"
    
    try:
        global ifcopenshell
        import ifcopenshell
    except:
        if DEBUG: print "using legacy exporter"
        import importIFClegacy
        return importIFClegacy.export(exportList,filename)

    p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Arch")
    FORCEBREP = p.GetBool("ifcExportAsBrep",False)
    DEBUG = p.GetBool("ifcDebug",False)
    version = FreeCAD.Version()
    owner = FreeCAD.ActiveDocument.CreatedBy
    email = ''
    if ("@" in owner) and ("<" in owner):
        s = owner.split("<")
        owner = s[0]
        email = s[1].strip(">")
    global ifctemplate
    ifctemplate = ifctemplate.replace("$version",version[0]+"."+version[1]+" build "+version[2])
    ifctemplate = ifctemplate.replace("$owner",owner)
    ifctemplate = ifctemplate.replace("$company",FreeCAD.ActiveDocument.Company)
    ifctemplate = ifctemplate.replace("$email",email)
    ifctemplate = ifctemplate.replace("$now",str(int(time.time())))
    ifctemplate = ifctemplate.replace("$projectid",FreeCAD.ActiveDocument.Uid[:22].replace("-","_"))
    ifctemplate = ifctemplate.replace("$project",FreeCAD.ActiveDocument.Name)
    ifctemplate = ifctemplate.replace("$filename",filename)
    ifctemplate = ifctemplate.replace("$timestamp",str(time.strftime("%Y-%m-%dT%H:%M:%S", time.gmtime())))
    template = tempfile.mkstemp(suffix=".ifc")[1]
    of = pyopen(template,"wb")
    of.write(ifctemplate)
    of.close()
    global ifcfile
    ifcfile = ifcopenshell.open(template)
    history = ifcfile.by_type("IfcOwnerHistory")[0]
    context = ifcfile.by_type("IfcGeometricRepresentationContext")[0]
    project = ifcfile.by_type("IfcProject")[0]
    objectslist = Draft.getGroupContents(exportList,walls=True,addgroups=True)
    objectslist = Arch.pruneIncluded(objectslist)
    products = {}
    count = 1
    
    # products
    for obj in objectslist:
        
        # getting generic data
        name = str(obj.Label)
        description = str(obj.Description) if hasattr(obj,"Description") else ""
            
        # getting uid
        uid = None
        if hasattr(obj,"IfcAttributes"):
            if "IfcUID" in obj.IfcAttributes.keys():
                uid = str(obj.IfcAttributes["IfcUID"])
        if not uid:
            uid = ifcopenshell.guid.compress(uuid.uuid1().hex)
            
        # setting the IFC type + name conversions
        if hasattr(obj,"Role"):
            ifctype = obj.Role.replace(" ","")
        else:
            ifctype = Draft.getType(obj)
        if ifctype == "Foundation":
            ifctype = "Footing"
        elif ifctype == "Floor":
            ifctype = "BuildingStorey"
        elif ifctype == "Rebar":
            ifctype = "ReinforcingBar"
        ifctype = "Ifc" + ifctype
        if ifctype == "IfcGroup":
            continue
        if not ifctype in ifctypes:
            ifctype = "IfcBuildingElementProxy"
        
        # getting the "Force BREP" flag
        brepflag = False
        if hasattr(obj,"IfcAttributes"):
            if "FlagForceBrep" in obj.IfcAttributes.keys():
                if obj.IfcAttributes["FlagForceBrep"] == "True":
                    brepflag = True
                    
        # getting the representation
        representation,placement,shapetype = getRepresentation(ifcfile,context,obj,forcebrep=(brepflag or FORCEBREP))
        
        if DEBUG: print str(count).ljust(3)," : ", ifctype, " (",shapetype,") : ",name

        # setting the arguments
        args = [uid,history,name,description,None,placement,representation,None]
        if ifctype in ["IfcSlab","IfcFooting","IfcRoof"]:
            args = args + ["NOTDEFINED"]
        elif ifctype in ["IfcWindow","IfcDoor"]:
            args = args + [obj.Width.Value, obj.Height.Value]
        elif ifctype == "IfcSpace":
            args = args + ["ELEMENT","INTERNAL",obj.Shape.BoundBox.ZMin]
        elif ifctype == "IfcBuildingElementProxy":
            args = args + ["ELEMENT"]
        elif ifctype == "IfcSite":
            latitude = None
            longitude = None
            elevation = None
            landtitlenumber = None
            address = None
            args = args + ["ELEMENT",latitude,longitude,elevation,landtitlenumber,address]
        elif ifctype == "IfcBuilding":
            args = args + ["ELEMENT",None,None,None]
        elif ifctype == "IfcBuildingStorey":
            args = args + ["ELEMENT",None]
            
        # creating the product
        product = getattr(ifcfile,"create"+ifctype)(*args)
        products[obj.Name] = product
            
        # additions
        if hasattr(obj,"Additions") and (shapetype == "extrusion"):
            for o in obj.Additions:
                r2,p2,c2 = getRepresentation(ifcfile,context,o,forcebrep=True)
                if DEBUG: print "      adding ",c2," : ",str(o.Label)
                prod2 = ifcfile.createIfcBuildingElementProxy(ifcopenshell.guid.compress(uuid.uuid1().hex),history,str(o.Label),None,None,p2,r2,None,"ELEMENT")
                ifcfile.createIfcRelAggregates(ifcopenshell.guid.compress(uuid.uuid1().hex),history,'Addition','',product,[prod2])
        
        # subtractions
        if hasattr(obj,"Subtractions") and (shapetype == "extrusion"):
            for o in obj.Subtractions:
                r2,p2,c2 = getRepresentation(ifcfile,context,o,forcebrep=True,subtraction=True)
                if DEBUG: print "      subtracting ",c2," : ",str(o.Label)
                prod2 = ifcfile.createIfcOpeningElement(ifcopenshell.guid.compress(uuid.uuid1().hex),history,str(o.Label),None,None,p2,r2,None)
                ifcfile.createIfcRelVoidsElement(ifcopenshell.guid.compress(uuid.uuid1().hex),history,'Subtraction','',product,prod2)
    
        count += 1
        
    # relationships
    sites = []
    buildings = []
    floors = []
    for site in Draft.getObjectsOfType(objectslist,"Site"):
        for building in Draft.getObjectsOfType(site.Group,"Building"):
            for floor in Draft.getObjectsOfType(building.Group,"Floor"):
                children = Draft.getGroupContents(floor,walls=True)
                children = Arch.pruneIncluded(children)
                children = [products[c.Name] for c in children if c.Name in products.keys()]
                floor = products[floor.Name]
                ifcfile.createIfcRelContainedInSpatialStructure(ifcopenshell.guid.compress(uuid.uuid1().hex),history,'StoreyLink','',children,floor)
                floors.append(floor)
            building = products[building.Name]
            if floors:
                ifcfile.createIfcRelAggregates(ifcopenshell.guid.compress(uuid.uuid1().hex),history,'BuildingLink','',building,floors)                
            buildings.append(building)
        site = products[site.Name]
        if buildings:
            ifcfile.createIfcRelAggregates(ifcopenshell.guid.compress(uuid.uuid1().hex),history,'SiteLink','',site,buildings)
        sites.append(site)
    if not sites:
        if DEBUG: print "adding default site"
        sites = [ifcfile.createIfcSite(ifcopenshell.guid.compress(uuid.uuid1().hex),history,"Default Site",'',None,None,None,None,"ELEMENT",None,None,None,None,None)]
    ifcfile.createIfcRelAggregates(ifcopenshell.guid.compress(uuid.uuid1().hex),history,'ProjectLink','',project,sites)
    if not buildings:
        if DEBUG: print "adding default building"
        buildings = [ifcfile.createIfcBuilding(ifcopenshell.guid.compress(uuid.uuid1().hex),history,"Default Building",'',None,None,None,None,"ELEMENT",None,None,None)]
        ifcfile.createIfcRelAggregates(ifcopenshell.guid.compress(uuid.uuid1().hex),history,'SiteLink','',sites[0],buildings)
        ifcfile.createIfcRelContainedInSpatialStructure(ifcopenshell.guid.compress(uuid.uuid1().hex),history,'BuildingLink','',products.values(),buildings[0])


    if DEBUG: print "writing ",filename,"..."
    ifcfile.write(filename)


def getRepresentation(ifcfile,context,obj,forcebrep=False,subtraction=False,tessellation=1):
    """returns an IfcShapeRepresentation object or None"""
    
    import Part,math,DraftGeomUtils,DraftVecUtils
    shapes = []
    placement = None
    productdef = None
    shapetype = "no shape"
        
    if not forcebrep:
        profile = None
        if hasattr(obj,"Proxy"):
            if hasattr(obj.Proxy,"getProfiles"):
                p = obj.Proxy.getProfiles(obj,noplacement=True)
                extrusionv = obj.Proxy.getExtrusionVector(obj,noplacement=True)
                if (len(p) == 1) and extrusionv:
                    p = p[0]
                    r = obj.Proxy.getPlacement(obj)
                    
                    if len(p.Edges) == 1:
                        
                        pxvc = ifcfile.createIfcDirection((1.0,0.0))
                        povc = ifcfile.createIfcCartesianPoint((0.0,0.0))
                        pt = ifcfile.createIfcAxis2Placement2D(povc,pxvc)
                        
                        # extruded circle
                        if isinstance(p.Edges[0].Curve,Part.Circle):
                            profile = ifcfile.createIfcCircleProfileDef("AREA",None,pt, p.Edges[0].Curve.Radius)
                            
                        # extruded ellipse
                        elif isinstance(p.Edges[0].Curve,Part.Ellipse):
                            profile = ifcfile.createIfcEllipseProfileDef("AREA",None,pt, p.Edges[0].Curve.MajorRadius, p.Edges[0].Curve.MinorRadius)     
                            
                    else:
                        curves = False
                        for e in p.Edges:
                            if isinstance(e.Curve,Part.Circle):
                                curves = True
                                
                        # extruded polyline
                        if not curves:
                            w = Part.Wire(DraftGeomUtils.sortEdges(p.Edges))
                            pts = [ifcfile.createIfcCartesianPoint(tuple(v.Point)[:2]) for v in w.Vertexes+[w.Vertexes[0]]]
                            pol = ifcfile.createIfcPolyline(pts)
                            
                        # extruded composite curve
                        else:
                            segments = []
                            last = None
                            edges = DraftGeomUtils.sortEdges(p.Edges)
                            for e in edges:
                                if isinstance(e.Curve,Part.Circle):
                                    follow = True
                                    if last:
                                        if not DraftVecUtils.equals(last,e.Vertexes[0].Point):
                                            follow = False
                                            last = e.Vertexes[0].Point
                                        else:
                                            last = e.Vertexes[-1].Point
                                    else:
                                        last = e.Vertexes[-1].Point
                                    p1 = math.degrees(-DraftVecUtils.angle(e.Vertexes[0].Point.sub(e.Curve.Center)))
                                    p2 = math.degrees(-DraftVecUtils.angle(e.Vertexes[-1].Point.sub(e.Curve.Center)))
                                    da = DraftVecUtils.angle(e.valueAt(e.FirstParameter+0.1).sub(e.Curve.Center),e.Vertexes[0].Point.sub(e.Curve.Center))
                                    if p1 < 0: 
                                        p1 = 360 + p1
                                    if p2 < 0:
                                        p2 = 360 + p2
                                    if da > 0:
                                        follow = not(follow)
                                    xvc =       ifcfile.createIfcDirection((1.0,0.0))
                                    ovc =       ifcfile.createIfcCartesianPoint(tuple(e.Curve.Center)[:2])
                                    plc =       ifcfile.createIfcAxis2Placement2D(ovc,xvc)
                                    cir =       ifcfile.createIfcCircle(plc,e.Curve.Radius)
                                    curve =     ifcfile.createIfcTrimmedCurve(cir,[ifcfile.create_entity("IfcParameterValue",p1)],[ifcfile.create_entity("IfcParameterValue",p2)],follow,"PARAMETER")
                                    
                                else:
                                    verts = [vertex.Point for vertex in e.Vertexes]
                                    if last:
                                        if not DraftVecUtils.equals(last,verts[0]):
                                            verts.reverse()
                                            last = e.Vertexes[0].Point
                                        else:
                                            last = e.Vertexes[-1].Point
                                    else:
                                        last = e.Vertexes[-1].Point
                                    pts =     [ifcfile.createIfcCartesianPoint(tuple(v)[:2]) for v in verts]
                                    curve =   ifcfile.createIfcPolyline(pts)
                                segment = ifcfile.createIfcCompositeCurveSegment("CONTINUOUS",True,curve)
                                segments.append(segment)
                                
                            pol = ifcfile.createIfcCompositeCurve(segments,False)
                        profile = ifcfile.createIfcArbitraryClosedProfileDef("AREA",None,pol)
                        
        if profile:
            xvc =       ifcfile.createIfcDirection(tuple(r.Rotation.multVec(FreeCAD.Vector(1,0,0))))
            zvc =       ifcfile.createIfcDirection(tuple(r.Rotation.multVec(FreeCAD.Vector(0,0,1))))
            ovc =       ifcfile.createIfcCartesianPoint(tuple(r.Base))
            lpl =       ifcfile.createIfcAxis2Placement3D(ovc,zvc,xvc)
            edir =      ifcfile.createIfcDirection(tuple(FreeCAD.Vector(extrusionv).normalize()))
            shape =     ifcfile.createIfcExtrudedAreaSolid(profile,lpl,edir,extrusionv.Length)
            shapes.append(shape)
            solidType = "SweptSolid"
            shapetype = "extrusion"
                
    if not shapes:
        
        # brep representation
        fcshape = None
        solidType = "Brep"
        if subtraction:
            if hasattr(obj,"Proxy"):
                if hasattr(obj.Proxy,"getSubVolume"):
                    fcshape = obj.Proxy.getSubVolume(obj)
        if not fcshape:
            if hasattr(obj,"Shape"):
                if obj.Shape:
                    if not obj.Shape.isNull():
                        fcshape = obj.Shape
            elif hasattr(obj,"Terrain"):
                if obj.Terrain:
                    if hasattr(obj.Terrain,"Shape"):
                        if obj.Terrain.Shape:
                            if not obj.Terrain.Shape.isNull():
                                    fcshape = obj.Terrain.Shape
        if fcshape:
            solids = []
            if fcshape.Solids:
                dataset = fcshape.Solids
            else:
                dataset = fcshape.Shells
                print "Warning! object contains no solids"
            for fcsolid in dataset:
                faces = []
                curves = False
                for fcface in fcsolid.Faces:
                    for e in fcface.Edges:
                        if not isinstance(e.Curve,Part.Line):
                            curves = True
                if curves:
                    tris = fcsolid.tessellate(tessellation)
                    for tri in tris[1]:
                        pts =   [ifcfile.createIfcCartesianPoint(tuple(tris[0][i])) for i in tri]
                        loop =  ifcfile.createIfcPolyLoop(pts)
                        bound = ifcfile.createIfcFaceOuterBound(loop,True)
                        face =  ifcfile.createIfcFace([bound])
                        faces.append(face)
                else:
                    for fcface in fcsolid.Faces:
                        loops = []
                        verts = [v.Point for v in Part.Wire(DraftGeomUtils.sortEdges(fcface.OuterWire.Edges)).Vertexes]
                        c = fcface.CenterOfMass
                        v1 = verts[0].sub(c)
                        v2 = verts[1].sub(c)
                        n = fcface.normalAt(0,0)
                        if DraftVecUtils.angle(v2,v1,n) >= 0:
                            verts.reverse() # inverting verts order if the direction is couterclockwise
                        pts =   [ifcfile.createIfcCartesianPoint(tuple(v)) for v in verts]
                        loop =  ifcfile.createIfcPolyLoop(pts)
                        bound = ifcfile.createIfcFaceOuterBound(loop,True)
                        loops.append(bound)
                        for wire in fcface.Wires:
                            if wire.hashCode() != fcface.OuterWire.hashCode():
                                verts = [v.Point for v in Part.Wire(DraftGeomUtils.sortEdges(wire.Edges)).Vertexes]
                                v1 = verts[0].sub(c)
                                v2 = verts[1].sub(c)
                                if DraftVecUtils.angle(v2,v1,DraftVecUtils.neg(n)) >= 0:
                                    verts.reverse()
                                pts =   [ifcfile.createIfcCartesianPoint(tuple(v)) for v in verts]
                                loop =  ifcfile.createIfcPolyLoop(pts)
                                bound = ifcfile.createIfcFaceBound(loop,True)
                                loops.append(bound)
                        face =  ifcfile.createIfcFace(loops)
                        faces.append(face)
                shell = ifcfile.createIfcClosedShell(faces)
                shape = ifcfile.createIfcFacetedBrep(shell)
                shapes.append(shape)
                shapetype = "brep"

    if shapes:
        
        if FreeCAD.GuiUp and (not subtraction) and hasattr(obj.ViewObject,"ShapeColor"):
            rgb = obj.ViewObject.ShapeColor
            col = ifcfile.createIfcColourRgb(None,rgb[0],rgb[1],rgb[2])
            ssr = ifcfile.createIfcSurfaceStyleRendering(col,None,None,None,None,None,None,None,"FLAT")
            iss = ifcfile.createIfcSurfaceStyle(None,"BOTH",[ssr])
            psa = ifcfile.createIfcPresentationStyleAssignment([iss])
            for shape in shapes:
                isi = ifcfile.createIfcStyledItem(shape,[psa],None)
                
                
        xvc = ifcfile.createIfcDirection((1.0,0.0,0.0))
        zvc = ifcfile.createIfcDirection((0.0,0.0,1.0))
        ovc = ifcfile.createIfcCartesianPoint((0.0,0.0,0.0))
        gpl = ifcfile.createIfcAxis2Placement3D(ovc,zvc,xvc)
        placement = ifcfile.createIfcLocalPlacement(None,gpl)
        representation = ifcfile.createIfcShapeRepresentation(context,'Body',solidType,shapes)
        productdef = ifcfile.createIfcProductDefinitionShape(None,None,[representation])
        
    return productdef,placement,shapetype
