#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2011                                                    *  
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

import ifcReader, FreeCAD, Arch, Draft, os, sys, time, Part, DraftVecUtils
from DraftTools import translate

__title__="FreeCAD IFC importer"
__author__ = "Yorik van Havre"
__url__ = "http://free-cad.sourceforge.net"

# config
DEBUG = True
SCHEMA = "http://www.steptools.com/support/stdev_docs/express/ifc2x3/ifc2x3_tc1.exp"
SKIP = ["IfcOpeningElement","IfcSpace"]
# end config

if open.__module__ == '__builtin__':
    pyopen = open # because we'll redefine open below

def open(filename):
    "called when freecad opens a file"
    docname = os.path.splitext(os.path.basename(filename))[0]
    doc = FreeCAD.newDocument(docname)
    doc.Label = decode(docname)
    FreeCAD.ActiveDocument = doc
    global createIfcGroups, useIfcOpenShell, importIfcFurniture
    createIfcGroups = useIfcOpenShell = importIfcFurniture = False
    p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Arch")
    useIfcOpenShell = p.GetBool("useIfcOpenShell")
    createIfcGroups = p.GetBool("createIfcGroups")
    importIfcFurniture = p.GetBool("importIfcFurniture")
    if not importIfcFurniture:
        SKIP.append("IfcFurnishingElement")
    read(filename)
    return doc

def insert(filename,docname):
    "called when freecad wants to import a file"
    try:
        doc = FreeCAD.getDocument(docname)
    except:
        doc = FreeCAD.newDocument(docname)
    FreeCAD.ActiveDocument = doc
    global createIfcGroups, useIfcOpenShell, importIfcFurniture
    createIfcGroups = useIfcOpenShell = importIfcFurniture = False
    p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Arch")
    useIfcOpenShell = p.GetBool("useIfcOpenShell")
    createIfcGroups = p.GetBool("createIfcGroups")
    importIfcFurniture = p.GetBool("importIfcFurniture")
    if not importIfcFurniture:
        SKIP.append("IfcFurnishingElement")    
    read(filename)
    return doc

def decode(name):
    "decodes encoded strings"
    try:
        decodedName = (name.decode("utf8"))
    except UnicodeDecodeError:
        try:
            decodedName = (name.decode("latin1"))
        except UnicodeDecodeError:
            FreeCAD.Console.PrintError(str(translate("Arch", "Error: Couldn't determine character encoding\n")))
            decodedName = name
    return decodedName

def getSchema():
    "retrieves the express schema"
    p = None
    p = os.path.join(FreeCAD.ConfigGet("UserAppData"),SCHEMA.split('/')[-1])
    if os.path.exists(p):
        return p
    import ArchCommands
    p = ArchCommands.download(SCHEMA)
    if p:
        return p
    return None

def getIfcOpenShell():
    "locates and imports ifcopenshell"
    try:
        global IfcImport
        import IfcImport
    except:
        FreeCAD.Console.PrintMessage(str(translate("Arch","Couldn't locate IfcOpenShell\n")))
        return False
    else:
        return True

def read(filename):
    "Parses an IFC file"

    # parsing the IFC file
    t1 = time.time()
    schema=getSchema()
    if schema:
        if DEBUG: global ifc
        if DEBUG: print "opening",filename,"..."
        ifc = ifcReader.IfcDocument(filename,schema=schema,debug=DEBUG)
    else:
        FreeCAD.Console.PrintWarning(str(translate("Arch","IFC Schema not found, IFC import disabled.\n")))
        return None
    t2 = time.time()
    if DEBUG: print "Successfully loaded",ifc,"in %s s" % ((t2-t1))
    
    if useIfcOpenShell and getIfcOpenShell():
        # use the IfcOpenShell parser
        
        useShapes = False
        if hasattr(IfcImport,"USE_BREP_DATA"):
            IfcImport.Settings(IfcImport.USE_BREP_DATA,True)
            useShapes = True
        if IfcImport.Init(filename):
            while True:

                obj = IfcImport.Get()
                if DEBUG: print "parsing ",obj.id,": ",obj.name," of type ",obj.type
                meshdata = []

                # retrieving name
                n = obj.name
                if not n:
                    n = "Unnamed"

                # build shape
                shape = None
                if useShapes:
                    shape = getShape(obj)

                # skip types
                if obj.type in SKIP:
                    pass

                # walls
                elif obj.type == "IfcWallStandardCase":
                    makeWall(ifc.Entities[obj.id],shape)

                # windows
                elif obj.type in ["IfcWindow","IfcDoor"]:
                    makeWindow(ifc.Entities[obj.id],shape)

                # structs
                elif obj.type in ["IfcBeam","IfcColumn","IfcSlab"]:
                    makeStructure(ifc.Entities[obj.id],shape)

                # furniture
                elif obj.type == "IfcFurnishingElement":
                    nobj = FreeCAD.ActiveDocument.addObject("Part::Feature","Furniture")
                    nobj.Shape = shape
                    
                elif shape:
                    # treat as dumb parts
                    nobj = FreeCAD.ActiveDocument.addObject("Part::Feature",n)
                    nobj.Shape = shape
                    
                else:
                    # treat as meshes
                    me,pl = getMesh(obj)
                    nobj = FreeCAD.ActiveDocument.addObject("Mesh::Feature",n)
                    nobj.Mesh = me
                    nobj.Placement = pl
                    
                if not IfcImport.Next():
                    break

        IfcImport.CleanUp()
        
    else:
        # use only the internal python parser
       
        # getting walls
        for w in ifc.getEnt("IfcWallStandardCase"):
            makeWall(w)
            
        # getting windows and doors
        for w in (ifc.getEnt("IfcWindow") + ifc.getEnt("IfcDoor")):
            makeWindow(w)
            
        # getting structs
        for w in (ifc.getEnt("IfcSlab") + ifc.getEnt("IfcBeam") + ifc.getEnt("IfcColumn")):
            makeStructure(w)
            
    order(ifc)
    FreeCAD.ActiveDocument.recompute()
    t3 = time.time()
    if DEBUG: print "done processing",ifc,"in %s s" % ((t3-t1))
    
    return None
    
def order(ifc):
    "orders the already generated elements by building and by floor"
    
    # getting floors
    for f in ifc.getEnt("IfcBuildingStorey"):
        group(f,"Floor")
    # getting buildings
    for b in ifc.getEnt("IfcBuilding"):
        group(b,"Building")
    # getting sites
    for s in ifc.getEnt("IfcSite"):
        group(s,"Site")

def group(entity,mode=None):
    "gathers the children of the given entity"
    
    try:
        if DEBUG: print "=====> making group",entity.id
        placement = None
        placement = getPlacement(entity.ObjectPlacement)
        if DEBUG: print "got cell placement",entity.id,":",placement
        subelements = ifc.find("IFCRELCONTAINEDINSPATIALSTRUCTURE","RelatingStructure",entity)
        subelements.extend(ifc.find("IFCRELAGGREGATES","RelatingObject",entity))
        elts = []
        for s in subelements:
            if hasattr(s,"RelatedElements"):
                s = s.RelatedElements
                if not isinstance(s,list): s = [s]
                elts.extend(s)
            elif hasattr(s,"RelatedObjects"):
                s = s.RelatedObjects
                if not isinstance(s,list): s = [s]
                elts.extend(s)
            elif hasattr(s,"RelatedObject"):
                s = s.RelatedObject
                if not isinstance(s,list): s = [s]
                elts.extend(s)
        print "found dependent elements: ",elts
        
        groups = [['Wall','IfcWallStandardCase',[]],
                  ['Window','IfcWindow',[]],
                  ['Door','IfcDoor',[]],
                  ['Slab','IfcSlab',[]],
                  ['Beam','IfcBeam',[]],
                  ['Column','IfcColumn',[]],
                  ['Floor','IfcBuildingStorey',[]],
                  ['Building','IfcBuilding',[]],
                  ['Furniture','IfcFurnishingElement',[]]]
        
        for e in elts:
            for g in groups:
                if e.type.upper() == g[1].upper():
                    o = FreeCAD.ActiveDocument.getObject(g[0] + str(e.id))
                    if o:
                        g[2].append(o)
        print "groups:",groups

        comps = []
        if createIfcGroups:
            if DEBUG: print "creating subgroups"
            for g in groups:
                if g[2]:
                    if g[0] in ['Building','Floor']:
                        comps.extend(g[2])
                    else:
                        fcg = FreeCAD.ActiveDocument.addObject("App::DocumentObjectGroup",g[0]+"s")
                        for o in g[2]:
                            fcg.addObject(o)
                        comps.append(fcg)
        else:
            for g in groups:
                comps.extend(g[2])

        name = mode + str(entity.id)
        if mode == "Site":
            cell = Arch.makeSite(comps,name=name)
        elif mode == "Floor":
            cell = Arch.makeFloor(comps,name=name)
        elif mode == "Building":
            cell = Arch.makeBuilding(comps,name=name)
    except:
        if DEBUG: print "error: skipping group ",entity.id        

def makeWall(entity,shape=None):
    "makes a wall in the freecad document"
    try:
        if DEBUG: print "=====> making wall",entity.id
        if shape:
            sh = FreeCAD.ActiveDocument.addObject("Part::Feature","WallBody")
            sh.Shape = shape
            wall = Arch.makeWall(sh,name="Wall"+str(entity.id))
            if DEBUG: print "made wall object  ",entity.id,":",wall
            return
        placement = wall = wire = body = width = height = None
        placement = getPlacement(entity.ObjectPlacement)
        if DEBUG: print "got wall placement",entity.id,":",placement
        width = entity.getProperty("Width")
        height = entity.getProperty("Height")
        if width and height:
                if DEBUG: print "got width, height ",entity.id,":",width,"/",height
                for r in entity.Representation.Representations:
                    if r.RepresentationIdentifier == "Axis":
                        wire = getWire(r.Items,placement)
                        wall = Arch.makeWall(wire,width,height,align="Center",name="Wall"+str(entity.id))
        else:
                if DEBUG: print "no height or width properties found..."
                for r in entity.Representation.Representations:
                    if r.RepresentationIdentifier == "Body":
                        for b in r.Items:
                            if b.type == "IFCEXTRUDEDAREASOLID":
                                norm = getVector(b.ExtrudedDirection)
                                norm.normalize()
                                wire = getWire(b.SweptArea,placement)
                                wall = Arch.makeWall(wire,width=0,height=b.Depth,name="Wall"+str(entity.id))
                                wall.Normal = norm
        if wall:
            if DEBUG: print "made wall object  ",entity.id,":",wall
    except:
        if DEBUG: print "error: skipping wall",entity.id

def makeWindow(entity,shape=None):
    "makes a window in the freecad document"
    try:
        typ = "Window" if entity.type == "IFCWINDOW" else "Door"
        if DEBUG: print "=====> making window",entity.id
        if shape:
            window = Arch.makeWindow(name=typ+str(entity.id))
            window.Shape = shape
            if DEBUG: print "made window object  ",entity.id,":",window
            return
        placement = window = wire = body = width = height = None
        placement = getPlacement(entity.ObjectPlacement)
        if DEBUG: print "got window placement",entity.id,":",placement
        width = entity.getProperty("Width")
        height = entity.getProperty("Height")
        for r in entity.Representation.Representations:
            if r.RepresentationIdentifier == "Body":
                for b in r.Items:
                    if b.type == "IFCEXTRUDEDAREASOLID":
                        wire = getWire(b.SweptArea,placement)
                        window = Arch.makeWindow(wire,width=b.Depth,name=typ+str(entity.id))
        if window:
            if DEBUG: print "made window object  ",entity.id,":",window
    except:
        if DEBUG: print "error: skipping window",entity.id

def makeStructure(entity,shape=None):
    "makes a structure in the freecad document"
    try:
        if entity.type == "IFCSLAB":
            typ = "Slab"
        elif entity.type == "IFCBEAM":
            typ = "Beam"
        else:
            typ = "Column"
        
        if DEBUG: print "=====> making struct",entity.id
        if shape:
            sh = FreeCAD.ActiveDocument.addObject("Part::Feature","StructureBody")
            sh.Shape = shape
            structure = Arch.makeStructure(sh,name=typ+str(entity.id))
            if DEBUG: print "made structure object  ",entity.id,":",structure
            return
        placement = structure = wire = body = width = height = None
        placement = getPlacement(entity.ObjectPlacement)
        if DEBUG: print "got window placement",entity.id,":",placement
        width = entity.getProperty("Width")
        height = entity.getProperty("Height")
        for r in entity.Representation.Representations:
            if r.RepresentationIdentifier == "Body":
                for b in r.Items:
                    if b.type == "IFCEXTRUDEDAREASOLID":
                        wire = getWire(b.SweptArea,placement)
                        structure = Arch.makeStructure(wire,height=b.Depth,name=typ+str(entity.id))
        if structure:
            if DEBUG: print "made structure object  ",entity.id,":",structure
    except:
        if DEBUG: print "error: skipping structure",entity.id

        
# geometry helpers ###################################################################

def getMesh(obj):
    "gets mesh and placement from an IfcOpenShell object"
    import Mesh
    f = obj.mesh.faces
    v = obj.mesh.verts
    for i in range(0, len(f), 3):
        face = []
        for j in range(3):
            vi = f[i+j]*3
            face.append([v[vi],v[vi+1],v[vi+2]])
        meshdata.append(face)
    me = Mesh.Mesh(meshdata)
    # get transformation matrix
    m = obj.matrix
    mat = FreeCAD.Matrix(m[0], m[3], m[6], m[9],
                         m[1], m[4], m[7], m[10],
                         m[2], m[5], m[8], m[11],
                         0, 0, 0, 1)
    pl = FreeCAD.Placement(mat)
    return me,pl

def getShape(obj):
    "gets a shape from an IfcOpenShell object"
    import StringIO
    sh=Part.Shape()
    sh.importBrep(StringIO.StringIO(obj.mesh.brep_data))
    m = obj.matrix
    mat = FreeCAD.Matrix(m[0], m[3], m[6], m[9],
                         m[1], m[4], m[7], m[10],
                         m[2], m[5], m[8], m[11],
                         0, 0, 0, 1)
    sh.Placement = FreeCAD.Placement(mat)
    if DEBUG: print "getting Shape from ",obj 
    return sh
        
def getWire(entity,placement=None):
    "returns a wire (created in the freecad document) from the given entity"
    if DEBUG: print "making Wire from :",entity
    if not entity: return None
    if entity.type == "IFCPOLYLINE":
        pts = []
        for p in entity.Points:
            pts.append(getVector(p))
        return Draft.getWire(pts,placement=placement)
    elif entity.type == "IFCARBITRARYCLOSEDPROFILEDEF":
        pts = []
        for p in entity.OuterCurve.Points:
            pts.append(getVector(p))
        return Draft.getWire(pts,closed=True,placement=placement)

def getPlacement(entity):
    "returns a placement from the given entity"
    if DEBUG: print "getting placement ",entity
    if not entity: return None
    pl = None
    if entity.type == "IFCAXIS2PLACEMENT3D":
        x = getVector(entity.RefDirection)
        z = getVector(entity.Axis)
        y = z.cross(x)
        loc = getVector(entity.Location)
        m = DraftVecUtils.getPlaneRotation(x,y,z)
        pl = FreeCAD.Placement(m)
        pl.move(loc)
    elif entity.type == "IFCLOCALPLACEMENT":
        pl = getPlacement(entity.PlacementRelTo)
        relpl = getPlacement(entity.RelativePlacement)
        if pl and relpl:
            pl = relpl.multiply(pl)
        elif relpl:
            pl = relpl
    elif entity.type == "IFCCARTESIANPOINT":
        loc = getVector(entity)
        pl = FreeCAD.Placement()
        pl.move(loc)
    if DEBUG: print "made placement for",entity.id,":",pl
    return pl

def getVector(entity):
    "returns a vector from the given entity"
    if DEBUG: print "getting point from",entity
    if entity.type == "IFCDIRECTION":
        if len(entity.DirectionRatios) == 3:
            return FreeCAD.Vector(tuple(entity.DirectionRatios))
        else:
            return FreeCAD.Vector(tuple(entity.DirectionRatios+[0]))
    elif entity.type == "IFCCARTESIANPOINT":
        if len(entity.Coordinates) == 3:
            return FreeCAD.Vector(tuple(entity.Coordinates))
        else:
            return FreeCAD.Vector(tuple(entity.Coordinates+[0]))
    return None
