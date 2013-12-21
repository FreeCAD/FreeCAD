#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2013                                                    *  
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

# this is a python convenience interface to IfcOpenShell, made to ease the
# creation of IFC files from scratch.
# currently using a test build of IfcOpenShell with export capabilities from
# https://github.com/aothms/IfcOpenShell

# see examples on how to use this module at the bottom of this file

import sys, uuid, time, math

# adding here the path to the ifcwrap folder of the ifcopenshell build. That
# folder must also contain an __init__.py file. This is to differentiate with
# systemwide-installed IfcOpenShell, which has the same name.

sys.path.append("/home/yorik/Sources/build/ifcopenshell-dev")
from ifcwrap import IfcImport as IfcExport

# checking that we got the right importer, with export capabilities
if not hasattr(IfcExport,"IfcFile"):
    print "Wrong version of IfcOpenShell"
    sys.exit()
    
PRECISION = 4 # rounding value, in number of digits

# Basic functions #################################################################


class _tempEntityHolder:
    """a temporary object to store entity references
    to be made into something nicer later..."""
    def __init__(self):
        self.refs = []
        
holder = _tempEntityHolder()

def new():
    """new(): returns a new empty ifc file holder"""
    fil = IfcExport.IfcFile()
    return fil

def uid():
    """returns a suitable GlobalID"""
    u = str(uuid.uuid4())[:22]
    u = u.replace("-","_")
    return u
    
def now(string=False):
    "returns a suitable Ifc Time"
    if string:
        return time.strftime("%Y-%m-%dT%H:%M:%S", time.gmtime())
    else:
        return int(time.time())

def getPropertyNames(entity):
    """getPropertyNames(entity): Returns a dictionary with 
    the numbers and names of the pythonproperties available for
    this entity"""
    ents = {}
    for i in range(entity.get_argument_count()):
        ents[i] = entity.get_argument_name(i)
    return ents
    
def getTuple(vec):
    """getTuple(vec): returns a tuple from other coordinate
    structures: tuple, list, 3d vector, or occ vertex"""
    if isinstance(vec,tuple):
        return vec
    elif isinstance(vec,list):
        return tuple(vec)
    elif hasattr(vec,"x") and hasattr(vec,"y") and hasattr(vec,"z"):
        return (vec.x,vec.y,vec.z)
    elif hasattr(vec,"X") and hasattr(vec,"Y") and hasattr(vec,"Z"):
        return (vec.X,vec.Y,vec.Z)
        
def getValueAndDirection(vec):
    """getValueAndDirection(vec): returns a length and a tuple
    representing a normalized vector from a tuple"""
    vec = getTuple(vec)
    length = round(math.sqrt(vec[0]**2 + vec[1]**2 + vec[2]**2),PRECISION)
    ratio = 1/length
    x = round(vec[0]*ratio,PRECISION)
    y = round(vec[1]*ratio,PRECISION)
    z = round(vec[2]*ratio,PRECISION)
    normal = (x,y,z)
    return length,normal

def create(ifcdoc=None,ifcname=None,arguments=[]):
    """create(ifcdoc,ifcname,[arguments]):creates an entity 
    of the given name in the given document and optionally 
    gives it an ordered list of arguments"""
    entity = IfcExport.Entity(ifcname)
    if ifcdoc:
        ifcdoc.add(entity)
    # this is a temporary hack while ifcopenshell has no ref counting
    holder.refs.append(entity)
    if not isinstance(arguments,list):
        arguments = [arguments]
    for i in range(len(arguments)):
        arg = arguments[i]
        if isinstance(arg,tuple):
            if len(arg) == 3:
                arg = IfcExport.Doubles(arg)
        entity.set_argument(i,arg)
    return entity
    

# Convenience tools #################################################################


def makeOwner(ifcdoc,person,organization="Undefined",application="Undefined",version=0.0):
    """makeOwner(ifcdoc,person,[organization,application,version]): 
    creates an owner in the given ifc document"""
    per = create(ifcdoc,"IfcPerson",[None,None,person,None,None,None,None,None])
    org = create(ifcdoc,"IfcOrganization",[None,organization,None,None,None])
    pno = create(ifcdoc,"IfcPersonAndOrganization",[per,org,None])
    app = create(ifcdoc,"IfcApplication",[org,str(version),application,uid()])
    own = create(ifcdoc,"IfcOwnerHistory",[pno,app,None,"ADDED",None,pno,app,now()])
    return own

def makePlacement(ifcdoc,reference=None,origin=(0,0,0),xaxis=(1,0,0),zaxis=(0,0,1),local=True):
    """makePlacement(ifcdoc,[reference,origin,xaxis,zaxis]):
    creates a local placement in the given ifc document. origin,
    xaxis and zaxis can be either a tuple or a 3d vector"""
    xvc = create(ifcdoc,"IfcDirection",getTuple(xaxis))
    zvc = create(ifcdoc,"IfcDirection",getTuple(zaxis))
    ovc = create(ifcdoc,"IfcCartesianPoint",getTuple(origin))
    gpl = create(ifcdoc,"IfcAxis2Placement3D",[ovc,zvc,xvc])
    if local:
        lpl = create(ifcdoc,"IfcLocalPlacement",[reference,gpl])
        return lpl
    else:
        return gpl

def makeContext(ifcdoc,placement=None):
    """makeContext(ifcdoc,[placement]): creates a geometric representation context
    in the given ifc document"""
    if not placement:
        placement = makePlacement(ifcdoc)
    rep = create(ifcdoc,"IfcGeometricRepresentationContext",
                        [None,'Model',3,1.E-05,placement,None])
    return rep

def makeProject(ifcdoc,context,owner,name="Default project",description=None):
    """makeProject(ifcdoc,context,owner,[name,description]): creates a project
    in the given ifc document"""
    
    # 0: 'GlobalId', 1: 'OwnerHistory', 2: 'Name', 3: 'Description', 
    # 4: 'ObjectType', 5: 'LongName', 6: 'Phase', 7: 'RepresentationContexts', 
    # 8: 'UnitsInContext'
    
    dim1 = create(ifcdoc,"IfcDimensionalExponents",[0,0,0,0,0,0,0])
    dim2 = create(ifcdoc,"IfcSIUnit",[dim1,"LENGTHUNIT","MILLI","METRE"])
    dim3 = create(ifcdoc,"IfcSIUnit",[dim1,"AREAUNIT",None,"SQUARE_METRE"])
    dim4 = create(ifcdoc,"IfcSIUnit",[dim1,"VOLUMEUNIT",None,"CUBIC_METRE"])
    dim6 = create(ifcdoc,"IfcSIUnit",[dim1,"PLANEANGLEUNIT",None,"RADIAN"])
    dim7 = create(ifcdoc,"IfcPlaneAngleMeasure",[1.745E-2])
    dim8 = create(ifcdoc,"IfcMeasureWithUnit",[dim7,dim6])
    dim9 = create(ifcdoc,"IfcConversionBasedUnit",[dim1,"PLANEANGLEUNIT","DEGREE",dim8])
    units = create(ifcdoc,"IfcUnitAssignment",[[dim2,dim3,dim4,dim9]])
    pro = create(ifcdoc,"IfcProject",[uid(),owner,name,description,
                                     None,None,None,[context],units])
    return pro

def makeSite(ifcdoc,project,owner,placement=None,name="Default site",description=None):
    """makeSite(ifcdoc,project,owner,[placement,name,description]): creates a site
    in the given ifc document"""
    
    # 0: 'GlobalId', 1: 'OwnerHistory', 2: 'Name', 3: 'Description', 
    # 4: 'ObjectType', 5: 'ObjectPlacement', 6: 'Representation', 7: 'LongName', 
    # 8: 'CompositionType', 9: 'RefLatitude', 10: 'RefLongitude', 11: 'RefElevation', 
    # 12: 'LandTitleNumber', 13: 'SiteAddress'
    
    if not placement:
        placement = makePlacement(ifcdoc)
    sit = create(ifcdoc,"IfcSite",[uid(),owner,name,description,None,placement,
                                   None,None,"ELEMENT",None,None,None,None,None])
    rel = create(ifcdoc,"IfcRelAggregates",[uid(),owner,'ProjectContainer',
                                            'Project-site relationship',project,[sit]])
    return sit

def makeBuilding(ifcdoc,site,owner,placement=None,name="Default building",description=None):
    """makeBuilding(ifcdoc,site,owner,[placement,name,description]): creates a building
    in the given ifc document"""
    
    # 0: 'GlobalId', 1: 'OwnerHistory', 2: 'Name', 3: 'Description', 
    # 4: 'ObjectType', 5: 'ObjectPlacement', 6: 'Representation', 7: 'LongName', 
    # 8: 'CompositionType', 9: 'ElevationOfRefHeight', 10: 'ElevationOfTerrain', 11: 'BuildingAddress'
    
    if not placement:
        placement = makePlacement(ifcdoc)
    bdg = create(ifcdoc,"IfcBuilding",[uid(),owner,name,description,
                                   None,placement,None,None,"ELEMENT",None,None,None])
    rel = create(ifcdoc,"IfcRelAggregates",[uid(),owner,'SiteContainer',
                                            'Site-Building relationship',site,[bdg]])
    return bdg

def makeStorey(ifcdoc,building,owner,placement=None,name="Default storey",description=None):
    """makeStorey(ifcdoc,building,owner,[placement,name,description]): creates a storey
    in the given ifc document"""
    
    # 0: 'GlobalId', 1: 'OwnerHistory', 2: 'Name', 3: 'Description', 
    # 4: 'ObjectType', 5: 'ObjectPlacement', 6: 'Representation', 7: 'LongName', 
    # 8: 'CompositionType', 9: 'Elevation
    
    if not placement:
        placement = makePlacement(ifcdoc)
    sto = create(ifcdoc,"IfcBuildingStorey",[uid(),owner,name,description,None,placement,
                                             None,None,"ELEMENT",None])
    rel = create(ifcdoc,"IfcRelAggregates",[uid(),owner,'BuildingContainer',
                                            'Building-Stories relationship',building,[sto]])
    return sto

def makeWall(ifcdoc,storey,owner,context,shape,placement=None,name="Default wall",description=None):
    """makeWall(ifcdoc,storey,owner,shape,[placement,name,description]): creates a wall
    in the given ifc document"""
    if not placement:
        placement = makePlacement(ifcdoc)
    rep = create(ifcdoc,"IfcShapeRepresentation",[context,'Body','SweptSolid',[shape]])
    prd = create(ifcdoc,"IfcProductDefinitionShape",[None,None,[rep]])
    wal = create(ifcdoc,"IfcWallStandardCase",[uid(),owner,name,description,
                                               None,placement,prd,None])
    return wal

def makePolyline(ifcdoc,points):
    """makePolyline(ifcdoc,points): creates a polyline with the given
    points in the given ifc document"""
    pts = [create(ifcdoc,"IfcCartesianPoint",getTuple(p)) for p in points]
    pol = create(ifcdoc,"IfcPolyline",[pts])
    return pol

def makeExtrusion(ifcdoc,polyline,extrusion,placement=None):
    """makeExtrusion(ifcdoc,polyline,extrusion,[placement]): makes an
    extrusion of the given polyline with the given extrusion vector in
    the given ifc document"""
    if not placement:
        placement = makePlacement(ifcdoc,local=False)
    value,norm = getValueAndDirection(extrusion)
    edir = create(ifcdoc,"IfcDirection",norm)
    area = create(ifcdoc,"IfcArbitraryClosedProfileDef",["AREA",None,polyline])
    solid = create(ifcdoc,"IfcExtrudedAreaSolid",[area,placement,edir,value])
    return solid
    
def relate(ifcdoc,container,owner,entities):
    """relate(ifcdoc,container,owner,entities): relates the given entities to the given
    container"""
    if container.is_a("IfcBuildingStorey"):
        rel = create(ifcdoc,"IfcRelContainedInSpatialStructure",[uid(),owner,'StoreyLink',
                     'Storey-content relationship',entities, container])
    else:
        print "not implemented!"


# IfcDocument Object #################################################################


class IfcDocument(object):
    """IfcDocument([filepath,name,owner,organization,application,version])
    Creates an empty IFC document."""
    
    def __init__(self,filepath="",name="",owner="",organization="",application="Python IFC exporter",version="0.0"):
        self._fileobject = IfcExport.IfcFile()
        self._person = create(self._fileobject,"IfcPerson",[None,None,"",None,None,None,None,None])
        self._org = create(self._fileobject,"IfcOrganization",[None,"",None,None,None])
        pno = create(self._fileobject,"IfcPersonAndOrganization",[self._person,self._org,None])
        app = create(self._fileobject,"IfcApplication",[self._org,version,application,uid()])
        self._owner = create(self._fileobject,"IfcOwnerHistory",[pno,app,None,"ADDED",None,pno,app,now()])
        axp = self.addPlacement(local=False)
        dim0 = create(self._fileobject,"IfcDirection",getTuple((0,1,0)))
        self._repcontext = create(self._fileobject,"IfcGeometricRepresentationContext",['Plan','Model',3,1.E-05,axp,dim0])
        placement = create(self._fileobject,"IfcLocalPlacement",[None,axp])
        dim1 = create(self._fileobject,"IfcDimensionalExponents",[0,0,0,0,0,0,0])
        dim2 = create(self._fileobject,"IfcSIUnit",[dim1,"LENGTHUNIT","MILLI","METRE"])
        dim3 = create(self._fileobject,"IfcSIUnit",[dim1,"AREAUNIT",None,"SQUARE_METRE"])
        dim4 = create(self._fileobject,"IfcSIUnit",[dim1,"VOLUMEUNIT",None,"CUBIC_METRE"])
        dim6 = create(self._fileobject,"IfcSIUnit",[dim1,"PLANEANGLEUNIT",None,"RADIAN"])
        dim7 = create(None,"IfcPlaneAngleMeasure",[1.745E-2])
        dim8 = create(self._fileobject,"IfcMeasureWithUnit",[dim7,dim6])
        dim9 = create(self._fileobject,"IfcConversionBasedUnit",[dim1,"PLANEANGLEUNIT","DEGREE",dim8])
        units = create(self._fileobject,"IfcUnitAssignment",[[dim2,dim3,dim4,dim9]])
        self.Project = create(self._fileobject,"IfcProject",[uid(),self._owner,None,None,None,None,None,[self._repcontext],units])
        self._site = create(self._fileobject,"IfcSite",[uid(),self._owner,"Site",None,None,placement,None,None,"ELEMENT",None,None,None,None,None])
        self._relate(self.Project,self._site)
        self._storeyRelations = {}
        self.BuildingProducts = []
        self.Storeys = []
        self.Buildings = []
        self.FilePath = filepath
        self.Owner = owner
        self.Organization = organization
        self.Name = name

    def __repr__(self):
        return "IFC document " + self.Name #+ " containing " + str(len(holder)) + " entities"
        
    def __setattr__(self,key,value):
        if value:
            if key == "Owner":
                print value
                self._person.set_argument(2,value)
            elif key == "Organization":
                self._org.set_argument(1,value)
            elif key == "Name":
                self.Project.set_argument(2,value)
        self.__dict__.__setitem__(key,value)
        
    def findByName(self,ifctype,name):
        "finds an entity of a given ifctype by name"
        objs = self._fileobject.by_type(ifctype)
        for obj in objs:
            for i in range(obj.get_argument_count()):
                if obj.get_argument_name(i) == "Name":
                    if obj.get_argument(i) == name:
                        return obj
        return None

    def write(self,fp=None):
        "writes the document to its file"
        if fp:
            path = fp
        else:
            path = self.FilePath
        if path:
            try:
                self._fileobject.write(path)
                self._fix(path)
            except:
                print ("IfcWriter: Error writing to "+path)
            else:
                print ("IfcWriter: Successfully written to "+path)
        else:
            print ("IfcWriter: Error: File path is not defined, unable to save")

    def _fix(self,path):
        "dirty hack to fix bugs in ifcopenshell"
        import os
        if os.path.exists(path):
            f = open(path,"rb")
            lines = []
            for l in f.readlines():
                if "IFCPLANEANGLEMEASURE" in l:
                    # bug 1: adding ifcPlaneAngleMeasure in a ifcMeasureWithUnit adds an unwanted = sign
                    l = l.replace("=IFCPLANEANGLEMEASURE","IFCPLANEANGLEMEASURE")
                elif ("FACEBOUND" in l) or ("FACEOUTERBOUND" in l):
                    # bug 2: booleans are exported as ints
                    l = l.replace(",1);",",.T.);")
                    l = l.replace(",0);",",.F.);")
                elif "FILE_DESCRIPTION" in l:
                    # bug 3: incomplete file description header
                    l = l.replace("ViewDefinition []","ViewDefinition [CoordinationView_V2.0]")
                elif "FILE_NAME" in l:
                    # bug 4: incomplete file name entry
                    l = l.replace("FILE_NAME('','',(''),('',''),'IfcOpenShell','IfcOpenShell','');","FILE_NAME('"+path+"','"+now(string=True)+"',('"+self.Owner+"'),('',''),'IfcOpenShell','IfcOpenShell','');")
                lines.append(l)
            f.close()
            f = open(path,"wb")
            for l in lines:
                f.write(l)
            f.close()

    def addPlacement(self,reference=None,origin=(0,0,0),xaxis=(1,0,0),zaxis=(0,0,1),local=True):
        """addPlacement([reference,origin,xaxis,zaxis,local]): adds a placement. origin,
        xaxis and zaxis can be either tuples or 3d vectors. If local is False, a global
        placement is returned, otherwise a local one."""
        xvc = create(self._fileobject,"IfcDirection",getTuple(xaxis))
        zvc = create(self._fileobject,"IfcDirection",getTuple(zaxis))
        ovc = create(self._fileobject,"IfcCartesianPoint",getTuple(origin))
        gpl = create(self._fileobject,"IfcAxis2Placement3D",[ovc,zvc,xvc])
        if local:
            lpl = create(self._fileobject,"IfcLocalPlacement",[reference,gpl])
            return lpl
        else:
            return gpl
            
    def addBuilding(self,placement=None,name="Default building",description=None):
        """addBuilding([placement,name,description]): adds a building"""
        if not placement:
            placement = self.addPlacement()
        bdg = create(self._fileobject,"IfcBuilding",[uid(),self._owner,name,description,None,placement,None,None,"ELEMENT",None,None,None])
        self._relate(self._site,bdg)
        self.Buildings.append(bdg)
        return bdg
        
    def addStorey(self,building=None,placement=None,name="Default storey",description=None):
        """addStorey([building,placement,name,description]): adds a storey"""
        if not placement:
            placement = self.addPlacement()
        sto = create(self._fileobject,"IfcBuildingStorey",[uid(),self._owner,name,description,None,placement,None,None,"ELEMENT",None])
        if not building:
            if self.Buildings:
                building = self.Buildings[0]
            else:
                building = self.addBuilding()
        self._relate(building,sto)
        self.Storeys.append(sto)
        return sto
            
    def _relate(self,container,entities):
        """relate(container,entities): relates the given entities to the given
        container"""
        if not isinstance(entities,list):
            entities = [entities]
        if container.is_a("IfcBuildingStorey"):
            sid = container.get_argument(0)
            if sid in self._storeyRelations:
                prods = self._storeyRelations[sid].get_argument(4)
                self._storeyRelations[sid].set_argument(4,prods+entities)
            else:
                rel = create(self._fileobject,"IfcRelContainedInSpatialStructure",[uid(),self._owner,'StoreyLink','',entities,container])
                self._storeyRelations[sid] = rel
        else:
            create(self._fileobject,"IfcRelAggregates",[uid(),self._owner,'Relationship','',container,entities])

    def addWall(self,shapes,storey=None,placement=None,name="Default wall",description=None):
        """addWall(shapes,[storey,placement,name,description]): creates a wall from the given representation shape(s)"""
        if not placement:
            placement = self.addPlacement()
        if not isinstance(shapes,list):
            shapes = [shapes]
        reps = [create(self._fileobject,"IfcShapeRepresentation",[self._repcontext,'Body','SweptSolid',[shape]]) for shape in shapes]
        prd = create(self._fileobject,"IfcProductDefinitionShape",[None,None,reps])
        wal = create(self._fileobject,"IfcWallStandardCase",[uid(),self._owner,name,description,None,placement,prd,None])
        self.BuildingProducts.append(wal)
        if not storey:
            if self.Storeys:
                storey = self.Storeys[0]
            else:
                storey = self.addStorey()
        self._relate(storey,wal)
        return wal
        
    def addStructure(self,ifctype,shapes,storey=None,placement=None,name="Default Structure",description=None):
        """addStructure(ifctype,shapes,[storey,placement,name,description]): creates a structure 
        from the given representation shape(s). Ifctype is the type of structural object (IfcBeam, IfcColumn, etc)"""
        if not placement:
            placement = self.addPlacement()
        if not isinstance(shapes,list):
            shapes = [shapes]
        reps = [create(self._fileobject,"IfcShapeRepresentation",[self._repcontext,'Body','SweptSolid',[shape]]) for shape in shapes]
        prd = create(self._fileobject,"IfcProductDefinitionShape",[None,None,reps])
        if ifctype in ["IfcSlab","IfcFooting"]:
            stt = create(self._fileobject,ifctype,[uid(),self._owner,name,description,None,placement,prd,None,"NOTDEFINED"])
        else:
            stt = create(self._fileobject,ifctype,[uid(),self._owner,name,description,None,placement,prd,None])
        self.BuildingProducts.append(stt)
        if not storey:
            if self.Storeys:
                storey = self.Storeys[0]
            else:
                storey = self.addStorey()
        self._relate(storey,stt)
        return stt
        
    def addWindow(self,ifctype,width,height,shapes,host=None,placement=None,name="Default Window",description=None):
        """addWindow(ifctype,width,height,shapes,[host,placement,name,description]): creates a window 
        from the given representation shape(s). Ifctype is the type of window object (IfcWindow, IfcDoor, etc)"""
        if not placement:
            placement = self.addPlacement()
        if not isinstance(shapes,list):
            shapes = [shapes]
        reps = [create(self._fileobject,"IfcShapeRepresentation",[self._repcontext,'Body','SweptSolid',[shape]]) for shape in shapes]
        prd = create(self._fileobject,"IfcProductDefinitionShape",[None,None,reps])
        win = create(self._fileobject,ifctype,[uid(),self._owner,name,description,None,placement,prd,None,float(height),float(width)])
        self.BuildingProducts.append(win)
        if host:
            self._relate(host,win)
        return win
    
    def addPolyline(self,points):
        """addPolyline(points): creates a polyline from the given points"""
        pts = [create(self._fileobject,"IfcCartesianPoint",getTuple(p)) for p in points]
        pol = create(self._fileobject,"IfcPolyline",[pts])
        return pol
    
    def addExtrusion(self,polyline,extrusion,placement=None):
        """addExtrusion(polyline,extrusion,[placement]): makes an
        extrusion of the given polyline with the given extrusion vector"""
        if not placement:
            placement = self.addPlacement(local=False)
        value,norm = getValueAndDirection(extrusion)
        edir = create(self._fileobject,"IfcDirection",[norm])
        area = create(self._fileobject,"IfcArbitraryClosedProfileDef",["AREA",None,polyline])
        solid = create(self._fileobject,"IfcExtrudedAreaSolid",[area,placement,edir,value])
        return solid
        
    def addExtrudedPolyline(self,points,extrusion,placement=None):
        """addExtrudedPolyline(points,extrusion,[placement]): makes an extruded polyline
        from the given points and the given extrusion vector"""
        pol = self.addPolyline(points)
        exp = self.addExtrusion(pol,extrusion,placement)
        return exp
        
    def addFace(self,face):
        """addFace(face): creates a face from the given face data (a list of lists of points).
        The first is the outer wire, the next are optional inner wires. They must be reversed in order"""
        ifb = []
        idx = 0
        for f in face:
            pts = []
            for p in f:
                #print p
                if p in self.fpoints:
                    #print self.fpoints.index(p)
                    #print self.frefs
                    pts.append(self.frefs[self.fpoints.index(p)])
                else:
                    pt = create(self._fileobject,"IfcCartesianPoint",getTuple(p))
                    pts.append(pt)
                    self.fpoints.append(p)
                    self.frefs.append(pt)
            #print pts
            loop = create(self._fileobject,"IfcPolyLoop",[pts])
            if idx == 0:
                fb = create(self._fileobject,"IfcFaceOuterBound",[loop,True])
            else:
                fb = create(self._fileobject,"IfcFaceBound",[loop,True])
            ifb.append(fb)
            idx += 1
        iface = create(self._fileobject,"IfcFace",[ifb])
        return iface

    def addFacetedBrep(self,faces):
        """addFacetedBrep(self,faces): creates a faceted brep object from the given list
        of faces (each face is a list of lists of points, inner wires are reversed)"""
        self.fpoints = []
        self.frefs = []
        #print "adding ",len(faces)," faces"
        #print faces
        ifaces = [self.addFace(face) for face in faces]
        sh = create(self._fileobject,"IfcClosedShell",[ifaces])
        brp = create(self._fileobject,"IfcFacetedBrep",[sh])
        return brp
        

# EXAMPLES #################################################################

def example1():

    "creation of a new file using manual, step-by-step procedure"
    
    f = new()
    own = makeOwner(f,"Yorik van Havre")
    plac = makePlacement(f)
    grep = makeContext(f,plac)
    proj = makeProject(f,grep,own)
    site = makeSite(f,proj,own)
    bldg = makeBuilding(f,site,own)
    stor = makeStorey(f,bldg,own)
    poly = makePolyline(f,[(0,0,0),(0,200,0),(5000,200,0),(5000,0,0),(0,0,0)])
    solid = makeExtrusion(f,poly,(0,0,3500))
    wall = makeWall(f,stor,own,grep,solid)
    poly2 = makePolyline(f,[(0,200,0),(0,2000,0),(200,2000,0),(200,200,0),(0,200,0)])
    solid2 = makeExtrusion(f,poly2,(0,0,3500))
    wall2 = makeWall(f,stor,own,grep,solid2)
    poly3 = makePolyline(f,[(0,2000,0),(0,2200,0),(5000,2200,0),(5000,2000,0),(0,2000,0)])
    solid3 = makeExtrusion(f,poly3,(0,0,3500))
    wall3 = makeWall(f,stor,own,grep,solid3)
    poly4 = makePolyline(f,[(5000,200,0),(5000,2000,0),(4800,2000,0),(4800,200,0),(5000,200,0)])
    solid4 = makeExtrusion(f,poly4,(0,0,3500))
    wall4 = makeWall(f,stor,own,grep,solid4)
    relate(f,stor,own,[wall,wall2,wall3,wall4])
    f.write("/home/yorik/test1.ifc")
    
    print dir(f)
    print f.by_type("IfcWallStandardCase")
    w = f.by_type("IfcWallStandardCase")[0]
    print w
    print dir(w)
    print w.is_a("IfcWallStandardCase")
    
def example2():
    
    "creation of a new file using advanced IfcDocument object"
    
    ifc = IfcDocument("/home/yorik/test2.ifc")
    ifc.Name = "Test Project"
    ifc.Owner = "Yorik van Havre"
    ifc.Organization = "FreeCAD"
    w1 = ifc.addWall( ifc.addExtrudedPolyline([(0,0,0),(0,200,0),(5000,200,0),(5000,0,0),(0,0,0)], (0,0,3500)) )
    ifc.addWall( ifc.addExtrudedPolyline([(0,200,0),(0,2000,0),(200,2000,0),(200,200,0),(0,200,0)],(0,0,3500)) )
    ifc.addWall( ifc.addExtrudedPolyline([(0,2000,0),(0,2200,0),(5000,2200,0),(5000,2000,0),(0,2000,0)],(0,0,3500)) )
    ifc.addWall( ifc.addExtrudedPolyline([(5000,200,0),(5000,2000,0),(4800,2000,0),(4800,200,0),(5000,200,0)],(0,0,3500)) )
    ifc.addWall( ifc.addFacetedBrep([[[(0,0,0),(100,0,0),(100,-1000,0),(0,-1000,0)]],
                                    [[(0,0,0),(100,0,0),(100,0,1000),(0,0,1000)]],
                                    [[(0,0,0),(0,0,1000),(0,-1000,1000),(0,-1000,0)]],
                                    [[(0,-1000,0),(0,-1000,1000),(100,-1000,1000),(100,-1000,0)]],
                                    [[(100,-1000,0),(100,-1000,1000),(100,0,1000),(100,0,0)]],
                                    [[(0,0,1000),(0,-1000,1000),(100,-1000,1000),(100,0,1000)]]]) )
    ifc.addStructure( "IfcColumn", ifc.addExtrudedPolyline([(0,0,0),(0,-200,0),(-500,-200,0),(-500,0,0),(0,0,0)], (0,0,3500)) )
    ifc.addWindow( "IfcDoor", 200, 200, ifc.addExtrudedPolyline([(200,200,0),(200,400,0),(400,400,0),(400,200,0),(200,200,0)], (0,0,200)), w1 )
    ifc.write()
    
    print dir(ifc._fileobject)
    print ifc._fileobject.by_type("IfcDoor")
    w = ifc._fileobject.by_type("IfcDoor")[0]
    print w
    print dir(w)
    print w.is_a("IfcDoor")
    for i in range(w.get_argument_count()):
        print i,": ",w.get_argument_name(i)," : ",w.get_argument(i)

