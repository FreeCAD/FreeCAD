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

import ifcReader, FreeCAD, Arch, Draft, os, sys, time
from draftlibs import fcvec

__title__="FreeCAD IFC importer"
__author__ = "Yorik van Havre"
__url__ = "http://free-cad.sourceforge.net"

DEBUG = True

def open(filename):
    "called when freecad opens a file"
    docname = os.path.splitext(os.path.basename(filename))[0]
    doc = FreeCAD.newDocument(docname)
    doc.Label = decode(docname)
    FreeCAD.ActiveDocument = doc
    p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Arch")
    p = p.GetBool("useIfcOpenShell")
    if p:
        readOpenShell(filename)
    else:
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
            print "ifc: error: couldn't determine character encoding"
            decodedName = name
    return decodedName

def getSchema():
    "retrieves the express schema"
    default = "IFC2X3_TC1.exp"
    p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Arch")
    p = p.GetString("IfcSchema")
    if p:
        if os.path.exists(p):
            return p
    p = os.path.join(FreeCAD.ConfigGet("UserAppData"),default)
    if os.path.exists(p):
        return p
    return None

def getIfcOpenShell():
    "locates and imports ifcopenshell"
    p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Arch")
    p = p.GetString("ifcopenshell")
    if p:
        try:
            sys.path.append(os.path.dirname(p))
            global IfcOpenShell
            IfcOpenShell = None
            import IfcImport as IfcOpenShell
        except:
            print "Couldn't import IfcOpenShell"

def readOpenShell(filename):
    import Mesh
    getIfcOpenShell()
    if IfcOpenShell:
        if IfcOpenShell.Init(filename):
            while True:
                obj = IfcOpenShell.Get()
                print "parsing ",obj.guid,": ",obj.name," of type ",obj.type
                meshdata = []
                n = obj.name
                if not n: n = "Unnamed"
                f = obj.mesh.faces
                v = obj.mesh.verts
                m = obj.matrix
                print "verts: ",len(v)," faces: ",len(f)
                mat = FreeCAD.Matrix(m[0], m[3], m[6], m[9],
                                     m[1], m[4], m[7], m[10],
                                     m[2], m[5], m[8], m[11],
                                     0, 0, 0, 1)
                for i in range(0, len(f), 3):
                    print "face ",f[i],f[i+1],f[i+2]
                    face = []
                    print "i:",i
                    for j in range(3):
                        vi = f[i+j]*3
                        print "vi:",vi
                        face.append([v[vi],v[vi+1],v[vi+2]])
                    meshdata.append(face)
                newmesh = Mesh.Mesh(meshdata)
                mobj = FreeCAD.ActiveDocument.addObject("Mesh::Feature",n)
                mobj.Mesh = newmesh
                mobj.Placement = FreeCAD.Placement(mat)
                if not IfcOpenShell.Next():
                    break
    IfcOpenShell.CleanUp()
    return None
    
def read(filename):
    "processes an ifc file and add its objects to the given document"
    t1 = time.time()
    schema=getSchema()
    if schema:
        if DEBUG: global ifc
        if DEBUG: print "opening",filename,"..."
        ifc = ifcReader.IfcDocument(filename,schema=schema,debug=DEBUG)
        t2 = time.time()
        if DEBUG: print "Successfully loaded",ifc,"in %s s" % ((t2-t1))
        # getting walls
        for w in ifc.getEnt("IFCWALLSTANDARDCASE"):
            makeWall(w)
        # getting floors
        for f in ifc.getEnt("IFCBUILDINGSTOREY"):
            makeCell(f,"Floor")
        # getting buildings
        for b in ifc.getEnt("IFCBUILDING"):
            makeCell(b,"Building")
        FreeCAD.ActiveDocument.recompute()
        t3 = time.time()
        if DEBUG: print "done processing",ifc,"in %s s" % ((t3-t1))
    else:
        FreeCAD.Console.PrintWarning("IFC Schema not found, IFC import disabled. See Arch Preferences to get a schema")

def makeCell(entity,mode="Cell"):
    "makes a cell in the freecad document"
    try:
        if DEBUG: print "=====> making cell",entity.id
        placement = None
        placement = getPlacement(entity.ObjectPlacement)
        if DEBUG: print "got cell placement",entity.id,":",placement
        subelements = ifc.find("IFCRELCONTAINEDINSPATIALSTRUCTURE","RelatingStructure",entity)
        subelements.extend(ifc.find("IFCRELAGGREGATES","RelatingObject",entity))
        fcelts = []
        for s in subelements:
            if hasattr(s,"RelatedElements"):
                s = s.RelatedElements
                if not isinstance(s,list): s = [s]
                for r in s:
                    if r.type == "IFCWALLSTANDARDCASE":
                        o = FreeCAD.ActiveDocument.getObject("Wall"+str(r.id))
                        if o: fcelts.append(o)
            elif hasattr(s,"RelatedObjects"):
                s = s.RelatedObjects
                if not isinstance(s,list): s = [s]
                for r in s:
                    if r.type == "IFCBUILDINGSTOREY":
                        o = FreeCAD.ActiveDocument.getObject("Floor"+str(r.id))
                        if o: fcelts.append(o)
        name = mode+str(entity.id)
        if mode == "Site":
            cell = Arch.makeSite(fcelts,name=name)
        elif mode == "Floor":
            cell = Arch.makeFloor(fcelts,join=True,name=name)
        elif mode == "Building":
            cell = Arch.makeBuilding(fcelts,name=name)
        else:
            cell = Arch.makeCell(fcelts,join=True,name=name)
        cell.CellType = type
    except:
        if DEBUG: print "error: skipping cell",entity.id        

def makeWall(entity):
    "makes a wall in the freecad document"
    try:
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
                        wire = makeWire(r.Items,placement)
                        wall = Arch.makeWall(wire,width,height,align="Center",name="Wall"+str(entity.id))
        else:
                if DEBUG: print "no height or width properties found..."
                for r in entity.Representation.Representations:
                    if r.RepresentationIdentifier == "Body":
                        for b in r.Items:
                            if b.type == "IFCEXTRUDEDAREASOLID":
                                norm = getVector(b.ExtrudedDirection)
                                norm.normalize()
                                wire = makeWire(b.SweptArea,placement)
                                wall = Arch.makeWall(wire,width=0,height=b.Depth,name="Wall"+str(entity.id))
                                wall.Normal = norm
        if wall:
            if DEBUG: print "made wall object  ",entity.id,":",wall
    except:
        if DEBUG: print "error: skipping wall",entity.id

def makeWire(entity,placement=None):
    "makes a wire in the freecad document"
    if DEBUG: print "making Wire from :",entity
    if not entity: return None
    if entity.type == "IFCPOLYLINE":
        pts = []
        for p in entity.Points:
            pts.append(getVector(p))
        return Draft.makeWire(pts,placement=placement)
    elif entity.type == "IFCARBITRARYCLOSEDPROFILEDEF":
        pts = []
        for p in entity.OuterCurve.Points:
            pts.append(getVector(p))
        return Draft.makeWire(pts,closed=True,placement=placement)

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
        m = fcvec.getPlaneRotation(x,y,z)
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
