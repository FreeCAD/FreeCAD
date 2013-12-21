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
__url__ = "http://www.freecadweb.org"

# config
subtractiveTypes = ["IfcOpeningElement"] # elements that must be subtracted from their parents
SCHEMA = "http://www.steptools.com/support/stdev_docs/express/ifc2x3/ifc2x3_tc1.exp"
MAKETEMPFILES = False # if True, shapes are passed from ifcopenshell to freecad through temp files
# end config

if open.__module__ == '__builtin__':
    pyopen = open # because we'll redefine open below

def open(filename):
    "called when freecad opens a file"
    docname = os.path.splitext(os.path.basename(filename))[0]
    doc = FreeCAD.newDocument(docname)
    doc.Label = decode(docname)
    FreeCAD.ActiveDocument = doc
    getConfig()
    read(filename)
    return doc

def insert(filename,docname):
    "called when freecad wants to import a file"
    try:
        doc = FreeCAD.getDocument(docname)
    except:
        doc = FreeCAD.newDocument(docname)
    FreeCAD.ActiveDocument = doc
    getConfig()
    read(filename)
    return doc
    
def getConfig():
    "Gets Arch IFC import preferences"
    global CREATE_IFC_GROUPS, ASMESH, DEBUG, SKIP, PREFIX_NUMBERS, FORCE_PYTHON_PARSER, SEPARATE_OPENINGS
    CREATE_IFC_GROUPS = False
    IMPORT_IFC_FURNITURE = False
    DEBUG = False
    SKIP = ["IfcBuildingElementProxy","IfcFlowTerminal","IfcFurnishingElement"]
    ASMESH = ["IfcFurnishingElement"]
    PREFIX_NUMBERS = False
    FORCE_PYTHON_PARSER = False
    SEPARATE_OPENINGS = False
    p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Arch")
    CREATE_IFC_GROUPS = p.GetBool("createIfcGroups")
    FORCE_PYTHON_PARSER = p.GetBool("forceIfcPythonParser") 
    DEBUG = p.GetBool("ifcDebug")
    SEPARATE_OPENINGS = p.GetBool("ifcSeparateOpenings")
    PREFIX_NUMBERS = p.GetBool("ifcPrefixNumbers")
    skiplist = p.GetString("ifcSkip")
    if skiplist:
        SKIP = skiplist.split(",")
    asmeshlist = p.GetString("ifcAsMesh")
    if asmeshlist:
        ASMESH = asmeshlist.split(",")

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
    num_lines = sum(1 for line in pyopen(filename))
    
    processedIds = []
    
    if getIfcOpenShell() and not FORCE_PYTHON_PARSER:
        # use the IfcOpenShell parser
        
        # preparing IfcOpenShell
        if DEBUG: global ifcObjects,ifcParents
        ifcObjects = {} # a table to relate ifc id with freecad object
        ifcParents = {} # a table to relate ifc id with parent id
        if hasattr(IfcImport,"DISABLE_OPENING_SUBTRACTIONS") and SEPARATE_OPENINGS:
            IfcImport.Settings(IfcImport.DISABLE_OPENING_SUBTRACTIONS,True)
        else:
            SKIP.append("IfcOpeningElement")
        useShapes = False
        if hasattr(IfcImport,"USE_BREP_DATA"):
            IfcImport.Settings(IfcImport.USE_BREP_DATA,True)
            useShapes = True
        else:
            if DEBUG: print "Warning: IfcOpenShell version very old, unable to handle Brep data"

        # processing geometry
        if IfcImport.Init(filename):
            while True:

                obj = IfcImport.Get()
                if DEBUG: print "["+str(int((float(obj.id)/num_lines)*100))+"%] parsing ",obj.id,": ",obj.name," of type ",obj.type

                # retrieving name
                n = getName(obj)
            
                # skip types
                if obj.type in SKIP:
                    if DEBUG: print "skipping because type is in skip list"
                    nobj = None
                
                # check if object was already processed, to workaround an ifcopenshell bug
                elif obj.id in processedIds:
                    if DEBUG: print "skipping because this object was already processed"
                    
                else:
                    # build shape
                    shape = None
                    if useShapes:
                        shape = getShape(obj)

                    # walls
                    if obj.type in ["IfcWallStandardCase","IfcWall"]:
                        nobj = makeWall(obj.id,shape,n)
    
                    # windows
                    elif obj.type in ["IfcWindow","IfcDoor"]:
                        nobj = makeWindow(obj.id,shape,n)
    
                    # structs
                    elif obj.type in ["IfcBeam","IfcColumn","IfcSlab","IfcFooting"]:
                        nobj = makeStructure(obj.id,shape,obj.type,n)
                        
                    # roofs
                    elif obj.type in ["IfcRoof"]:
                        nobj = makeRoof(obj.id,shape,n)
                        
                    # furniture
                    elif obj.type in ["IfcFurnishingElement"]:
                        nobj = FreeCAD.ActiveDocument.addObject("Part::Feature",n)
                        nobj.Shape = shape
                        
                    # sites
                    elif obj.type in ["IfcSite"]:
                        nobj = makeSite(obj.id,shape,n)
                        
                    # spaces
                    elif obj.type in ["IfcSpace"]:
                        nobj = makeSpace(obj.id,shape,n)
                        
                    elif shape:
                        # treat as dumb parts
                        #if DEBUG: print "Fixme: Shape-containing object not handled: ",obj.id, " ", obj.type 
                        nobj = FreeCAD.ActiveDocument.addObject("Part::Feature",n)
                        nobj.Shape = shape
                        
                    else:
                        # treat as meshes
                        if DEBUG: print "Warning: Object without shape: ",obj.id, " ", obj.type 
                        me,pl = getMesh(obj)
                        nobj = FreeCAD.ActiveDocument.addObject("Mesh::Feature",n)
                        nobj.Mesh = me
                        nobj.Placement = pl
                    
                    # registering object number and parent
                    if obj.parent_id > 0:
                        ifcParents[obj.id] = [obj.parent_id,not (obj.type in subtractiveTypes)]
                    ifcObjects[obj.id] = nobj
                    processedIds.append(obj.id)
                    
                if not IfcImport.Next():
                    break

        # processing non-geometry and relationships
        parents_temp = dict(ifcParents)
        import ArchCommands

        while parents_temp:
            id, c = parents_temp.popitem()
            parent_id = c[0]
            additive = c[1]
            
            if (id <= 0) or (parent_id <= 0):
                # root dummy object
                parent = None

            elif parent_id in ifcObjects:
                parent = ifcObjects[parent_id]
                # check if parent is a subtraction, if yes parent to grandparent
                if parent_id in ifcParents:
                    if ifcParents[parent_id][1] == False:
                        grandparent_id = ifcParents[parent_id][0]
                        if grandparent_id in ifcObjects:
                            parent = ifcObjects[grandparent_id]
            else:
                # creating parent if needed
                parent_ifcobj = IfcImport.GetObject(parent_id)
                if DEBUG: print "["+str(int((float(parent_ifcobj.id)/num_lines)*100))+"%] parsing ",parent_ifcobj.id,": ",parent_ifcobj.name," of type ",parent_ifcobj.type
                n = getName(parent_ifcobj)
                if parent_ifcobj.id <= 0:
                    parent = None
                elif parent_ifcobj.type == "IfcBuildingStorey":
                    parent = Arch.makeFloor(name=n)
                    parent.Label = n
                elif parent_ifcobj.type == "IfcBuilding":
                    parent = Arch.makeBuilding(name=n)
                    parent.Label = n
                elif parent_ifcobj.type == "IfcSite":
                    parent = Arch.makeSite(name=n)
                    parent.Label = n
                elif parent_ifcobj.type == "IfcWindow":
                    parent = Arch.makeWindow(name=n)
                    parent.Label = n
                else:
                    if DEBUG: print "Fixme: skipping unhandled parent: ", parent_ifcobj.id, " ", parent_ifcobj.type
                    parent = None
                # registering object number and parent
                if parent_ifcobj.parent_id > 0:
                        ifcParents[parent_ifcobj.id] = [parent_ifcobj.parent_id,True]
                        parents_temp[parent_ifcobj.id] = [parent_ifcobj.parent_id,True]
                if parent and (not parent_ifcobj.id in ifcObjects):
                    ifcObjects[parent_ifcobj.id] = parent
            
            # attributing parent
            if parent and (id in ifcObjects):
                if ifcObjects[id]:
                    if additive:
                        ArchCommands.addComponents(ifcObjects[id],parent)
                    else:
                        ArchCommands.removeComponents(ifcObjects[id],parent)
                        
        IfcImport.CleanUp()
        
    else:
        # use only the internal python parser
        
        FreeCAD.Console.PrintWarning(str(translate("Arch","IfcOpenShell not found or disabled, falling back on internal parser.\n")))
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
       
        # getting walls
        for w in ifc.getEnt("IfcWallStandardCase"):
            nobj = makeWall(w)
            
        # getting windows and doors
        for w in (ifc.getEnt("IfcWindow") + ifc.getEnt("IfcDoor")):
            nobj = makeWindow(w)
            
        # getting structs
        for w in (ifc.getEnt("IfcSlab") + ifc.getEnt("IfcBeam") + ifc.getEnt("IfcColumn") \
                  + ifc.getEnt("IfcFooting")):
            nobj = makeStructure(w)
             
        # getting floors
        for f in ifc.getEnt("IfcBuildingStorey"):
            group(f,ifc,"Floor")
            
        # getting buildings
        for b in ifc.getEnt("IfcBuilding"):
            group(b,ifc,"Building")
            
        # getting sites
        for s in ifc.getEnt("IfcSite"):
            group(s,ifc,"Site")

    if DEBUG: print "done parsing. Recomputing..."        
    FreeCAD.ActiveDocument.recompute()
    t3 = time.time()
    if DEBUG: print "done processing IFC file in %s s" % ((t3-t1))
    
    return None
    
def getName(ifcobj):
    "Get a clean name from an ifc object"
    n = ifcobj.name
    if not n:
        n = ifcobj.type
    if PREFIX_NUMBERS:
        n = "ID"+str(ifcobj.id)+" "+n
    #for c in ",.!?;:":
    #    n = n.replace(c,"_")
    return n


def makeWall(entity,shape=None,name="Wall"):
    "makes a wall in the freecad document"
    try:
        if shape:
            # use ifcopenshell
            if isinstance(shape,Part.Shape):
                body = FreeCAD.ActiveDocument.addObject("Part::Feature",name+"_body")
                body.Shape = shape
            else:
                body = FreeCAD.ActiveDocument.addObject("Mesh::Feature",name+"_body")
                body.Mesh = shape
            wall = Arch.makeWall(body,name=name)
            wall.Label = name
            if DEBUG: print "made wall object ",entity,":",wall
            return wall
            
        # use internal parser
        if DEBUG: print "=====> making wall",entity.id
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
            return wall
        if DEBUG: print "error: skipping wall",entity.id
        return None
    except:
        if DEBUG: print "error: skipping wall",entity
        return None


def makeWindow(entity,shape=None,name="Window"):
    "makes a window in the freecad document"
    try:
        if shape:
            # use ifcopenshell
            if isinstance(shape,Part.Shape):
                window = Arch.makeWindow(name=name)
                window.Shape = shape
                window.Label = name
                if DEBUG: print "made window object  ",entity,":",window
                return window
            
        # use internal parser
        if DEBUG: print "=====> making window",entity.id
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
                        window = Arch.makeWindow(wire,width=b.Depth,name=objtype+str(entity.id))
        if window:
            if DEBUG: print "made window object  ",entity.id,":",window
            return window
        if DEBUG: print "error: skipping window",entity.id
        return None
    except:
        if DEBUG: print "error: skipping window",entity
        return None


def makeStructure(entity,shape=None,ifctype=None,name="Structure"):
    "makes a structure in the freecad document"
    try:
        if shape:
            # use ifcopenshell
            if isinstance(shape,Part.Shape):
                body = FreeCAD.ActiveDocument.addObject("Part::Feature",name+"_body")
                body.Shape = shape
            else:
                body = FreeCAD.ActiveDocument.addObject("Mesh::Feature",name+"_body")
                body.Mesh = shape
            structure = Arch.makeStructure(body,name=name)
            structure.Label = name
            if ifctype == "IfcBeam":
                structure.Role = "Beam"
            elif ifctype == "IfcColumn":
                structure.Role = "Column"
            elif ifctype == "IfcSlab":
                structure.Role = "Slab"
            elif ifctype == "IfcFooting":
                structure.Role = "Foundation"
            if DEBUG: print "made structure object  ",entity,":",structure," (type: ",ifctype,")"
            return structure
            
        # use internal parser
        if DEBUG: print "=====> making struct",entity.id
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
                        structure = Arch.makeStructure(wire,height=b.Depth,name=objtype+str(entity.id))
        if structure:
            if DEBUG: print "made structure object  ",entity.id,":",structure
            return structure
        if DEBUG: print "error: skipping structure",entity.id
        return None
    except:
        if DEBUG: print "error: skipping structure",entity
        return None


def makeSite(entity,shape=None,name="Site"):
    "makes a site in the freecad document"
    try:
        body = None
        if shape:
            # use ifcopenshell
            if isinstance(shape,Part.Shape):
                body = FreeCAD.ActiveDocument.addObject("Part::Feature",name+"_body")
                body.Shape = shape
            else:
                body = FreeCAD.ActiveDocument.addObject("Mesh::Feature",name+"_body")
                body.Mesh = shape
        site = Arch.makeSite(name=name)
        site.Label = name
        if body:
            site.Terrain = body
        if DEBUG: print "made site object  ",entity,":",site
        return site
    except:
        return None
        
def makeSpace(entity,shape=None,name="Space"):
    "makes a space in the freecad document"
    try:
        if shape:
            # use ifcopenshell
            if isinstance(shape,Part.Shape):
                space = Arch.makeSpace(name=name)
                space.Label = name
                body = FreeCAD.ActiveDocument.addObject("Part::Feature",name+"_body")
                body.Shape = shape
                space.Base = body
                body.ViewObject.hide()
                if DEBUG: print "made space object  ",entity,":",space
                return space
    except:
        return None


def makeRoof(entity,shape=None,name="Roof"):
    "makes a roof in the freecad document"
    try:
        if shape:
            # use ifcopenshell
            if isinstance(shape,Part.Shape):
                roof = Arch.makeRoof(name=name)
                roof.Label = name
                roof.Shape = shape
                if DEBUG: print "made roof object  ",entity,":",roof
                return roof
    except:
        return None

# geometry helpers ###################################################################

def getMesh(obj):
    "gets mesh and placement from an IfcOpenShell object"
    import Mesh
    meshdata = []
    print obj.mesh.faces
    print obj.mesh.verts
    f = obj.mesh.faces
    v = obj.mesh.verts
    for i in range(0, len(f), 3):
        face = []
        for j in range(3):
            vi = f[i+j]*3
            face.append([v[vi],v[vi+1],v[vi+2]])
        meshdata.append(face)
        print meshdata
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
    #print "retrieving shape from obj ",obj.id
    import Part
    sh=Part.Shape()
    try:
        if MAKETEMPFILES:
            import tempfile
            tf = tempfile.mkstemp(suffix=".brp")[1]
            of = pyopen(tf,"wb")
            of.write(obj.mesh.brep_data)
            of.close()
            sh = Part.read(tf)
            os.remove(tf)
        else:
            sh.importBrepFromString(obj.mesh.brep_data)
            #sh = Part.makeBox(2,2,2)
    except:
        print "Error: malformed shape"
        return None
    if not sh.Solids:
        # try to extract a solid shape
        if sh.Faces:
            try:
                if DEBUG: print "Malformed solid. Attempting to fix..."
                shell = Part.makeShell(sh.Faces)
                if shell:
                    solid = Part.makeSolid(shell)
                    if solid:
                        sh = solid
            except:
                if DEBUG: print "failed to retrieve solid from object ",obj.id
        else:
            if DEBUG: print "object ", obj.id, " doesn't contain any face"
    m = obj.matrix
    mat = FreeCAD.Matrix(m[0], m[3], m[6], m[9],
                         m[1], m[4], m[7], m[10],
                         m[2], m[5], m[8], m[11],
                         0, 0, 0, 1)
    sh.Placement = FreeCAD.Placement(mat)
    # if DEBUG: print "getting Shape from ",obj 
    #print "getting shape: ",sh,sh.Solids,sh.Volume,sh.isValid(),sh.isNull()
    #for v in sh.Vertexes: print v.Point
    return sh
    
# below is only used by the internal parser #########################################
 
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
    custom = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Arch").GetString("CustomIfcSchema","")
    if custom:
        if os.path.exists(custom):
            if DEBUG: print "Using custom schema: ",custom.split(os.sep)[-1]
            return custom
    p = None
    p = os.path.join(FreeCAD.ConfigGet("UserAppData"),SCHEMA.split(os.sep)[-1])
    if os.path.exists(p):
        return p
    import ArchCommands
    p = ArchCommands.download(SCHEMA)
    if p:
        return p
    return None 
    
def group(entity,ifc,mode=None):
    "gathers the children of the given entity"
    # only used by the internal parser
    
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
        
        groups = [['Wall',['IfcWallStandardCase'],[]],
                  ['Window',['IfcWindow','IfcDoor'],[]],
                  ['Structure',['IfcSlab','IfcFooting','IfcBeam','IfcColumn'],[]],
                  ['Floor',['IfcBuildingStorey'],[]],
                  ['Building',['IfcBuilding'],[]],
                  ['Furniture',['IfcFurnishingElement'],[]]]
        
        for e in elts:
            for g in groups:
                for t in g[1]:
                    if e.type.upper() == t.upper():
                        if hasattr(FreeCAD.ActiveDocument,g[0]+str(e.id)):
                            g[2].append(FreeCAD.ActiveDocument.getObject(g[0]+str(e.id)))
        print "groups:",groups

        comps = []
        if CREATE_IFC_GROUPS:
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

        label = entity.Name
        name = mode + str(entity.id)
        cell = None
        if mode == "Site":
            cell = Arch.makeSite(comps,name=name)
        elif mode == "Floor":
            cell = Arch.makeFloor(comps,name=name)
        elif mode == "Building":
            cell = Arch.makeBuilding(comps,name=name)
        if label and cell:
            cell.Label = label
    except:
        if DEBUG: print "error: skipping group ",entity.id
        
def getWire(entity,placement=None):
    "returns a wire (created in the freecad document) from the given entity"
    # only used by the internal parser
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
    # only used by the internal parser
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
    # only used by the internal parser
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
    
# EXPORT ##########################################################

def export(exportList,filename):
    "called when freecad exports a file"
    try:
        import ifcWriter
    except:
        print "importIFC: ifcWriter not found or unusable. Unable to export."
        return

    # creating base IFC project
    import Arch,Draft
    getConfig()
    ifcWriter.PRECISION = Draft.precision()
    p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Arch")
    scaling = p.GetFloat("IfcScalingFactor",1.0)
    application = "FreeCAD"
    ver = FreeCAD.Version()
    version = ver[0]+"."+ver[1]+" build"+ver[2]
    owner = FreeCAD.ActiveDocument.CreatedBy
    company = FreeCAD.ActiveDocument.Company
    project = FreeCAD.ActiveDocument.Name
    ifc = ifcWriter.IfcDocument(filename,project,owner,company,application,version)

    # get all children and reorder list to get buildings and floors processed first
    objectslist = Draft.getGroupContents(exportList,walls=True,addgroups=True)
    objectslist = Arch.pruneIncluded(objectslist)
    buildings = []
    floors = []
    others = []
    for obj in objectslist:
        otype = Draft.getType(obj)
        if otype == "Building":
            buildings.append(obj)
        elif otype == "Floor":
            floors.append(obj)
        else:
            others.append(obj)
    objectslist = buildings + floors + others
    if DEBUG: print "adding ", len(objectslist), " objects"

    # process objects
    for obj in objectslist:
        if DEBUG: print "adding ",obj.Label
        otype = Draft.getType(obj)
        name = str(obj.Label)
        parent = Arch.getHost(obj)
        gdata = Arch.getExtrusionData(obj,scaling)
        if not gdata:
            fdata = Arch.getBrepFacesData(obj,scaling)
            if not fdata:
                if obj.isDerivedFrom("Part::Feature"):
                    print "IFC export: error retrieving the shape of object ", obj.Name
                    continue
                    
        if otype == "Building":
            ifc.addBuilding( name=name )
            
        elif otype == "Floor":
            if parent:
                parent = ifc.findByName("IfcBuilding",str(parent.Label))
            ifc.addStorey( building=parent, name=name )

        elif otype == "Wall":
            if parent:
                parent = ifc.findByName("IfcBuildingStorey",str(parent.Label))
            if gdata:
                ifc.addWall( ifc.addExtrudedPolyline(gdata[0],gdata[1]), storey=parent, name=name )
            elif fdata:
                ifc.addWall( [ifc.addFacetedBrep(f) for f in fdata], storey=parent, name=name )
                
        elif otype == "Structure":
            if parent:
                parent = ifc.findByName("IfcBuildingStorey",str(parent.Label))
            role = "IfcBeam"
            if hasattr(obj,"Role"):
                if obj.Role == "Column":
                    role = "IfcColumn"
                elif obj.Role == "Slab":
                    role = "IfcSlab"
                elif obj.Role == "Foundation":
                    role = "IfcFooting"
            if gdata:
                ifc.addStructure( role, ifc.addExtrudedPolyline(gdata[0],gdata[1]), storey=parent, name=name )
            elif fdata:
                ifc.addStructure( role, [ifc.addFacetedBrep(f) for f in fdata], storey=parent, name=name )
                
        elif otype == "Window":
            if parent:
                p = ifc.findByName("IfcWallStandardCase",str(parent.Label))
                if not p:
                    p = ifc.findByName("IfcColumn",str(parent.Label))
                    if not p:
                        p = ifc.findByName("IfcBeam",str(parent.Label))
                        if not p:
                            p = ifc.findByName("IfcSlab",str(parent.Label))
                parent = p
            role = "IfcWindow"
            if hasattr(obj,"Role"):
                if obj.Role == "Door":
                    role = "IfcDoor"
            if gdata:
                ifc.addWindow( role, obj.Width*scaling, obj.Height*scaling, ifc.addExtrudedPolyline(gdata[0],gdata[1]), host=parent, name=name )
            elif fdata:
                ifc.addWindow( role, obj.Width*scaling, obj.Height*scaling, [ifc.addFacetedBrep(f) for f in fdata], host=parent, name=name )

        else:
            print "IFC export: object type ", otype, " is not supported yet."
            
    ifc.write()
