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

# if you already have another version of IfcOpenShell:
# adding here the path to the ifcwrap folder of the ifcopenshell build. That
# folder must also contain an __init__.py file. This is to differentiate with
# systemwide-installed IfcOpenShell, which has the same name.
# if you have such setup, uncomment the following 2 lines and comment out the
# third one.
#sys.path.append("/home/yorik/Sources/build/ifcopenshell-dev")
#from ifcwrap import IfcImport

try:
    import IfcImport as ifcw
except:
    import ifc_wrapper as ifcw
else:
    print "error: IfcOpenShell not found!"
    sys.exit()
    
# checking that we got the right importer, with export capabilities
if (not hasattr(ifcw,"IfcFile")) and (not hasattr(ifcw,"file")):
    print "Wrong version of IfcOpenShell"
    sys.exit()
    
PRECISION = 8 # rounding value, in number of digits
APPLYFIX = True # if true, the ifcopenshell bug-fixing function is applied when saving files

# Basic functions #################################################################


class _tempEntityHolder:
    """a temporary object to store entity references
    to be made into something nicer later..."""
    def __init__(self):
        self.refs = []
        
holder = _tempEntityHolder()

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
    if hasattr(entity,"get_argument_count"):
        l = entity.get_argument_count()
    else:
        l = len(entity)
    for i in range(l):
        ents[i] = entity.get_argument_name(i)
    return ents
    
def getTuple(vec):
    """getTuple(vec): returns a tuple from other coordinate
    structures: tuple, list, 3d vector, or occ vertex"""
    def fmt(t):
        t = float(t)
        t = round(t,PRECISION)
        return t
    if isinstance(vec,tuple):
        return tuple([fmt(v) for v in vec])
    elif isinstance(vec,list):
        return tuple([fmt(v) for v in vec])
    elif hasattr(vec,"x") and hasattr(vec,"y") and hasattr(vec,"z"):
        return (fmt(vec.x),fmt(vec.y),fmt(vec.z))
    elif hasattr(vec,"X") and hasattr(vec,"Y") and hasattr(vec,"Z"):
        return (fmt(vec.X),fmt(vec.Y),fmt(vec.Z))
        
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
    if hasattr(ifcw,"Entity"):
        entity = ifcw.Entity(ifcname)
    else:
        entity = ifcw.entity_instance(ifcname)
    if ifcdoc:
        ifcdoc.add(entity)
    # this is a temporary hack while ifcopenshell has no ref counting
    holder.refs.append(entity)
    if not isinstance(arguments,list):
        arguments = [arguments]
    for i in range(len(arguments)):
        arg = arguments[i]
        if isinstance(arg,tuple):
            if len(arg) in [2,3]:
                if hasattr(ifcw,"Doubles"):
                    arg = ifcw.Doubles(arg)
                else:
                    arg = ifcw.doubles(arg)
        entity.set_argument(i,arg)
    return entity
    

# IfcDocument Object #################################################################


class IfcDocument(object):
    """IfcDocument([filepath,name,owner,organization,application,version])
    Creates an empty IFC document."""
    
    def __init__(self,filepath="",name="",owner="",organization="",application="Python IFC exporter",version="0.0"):
        if hasattr(ifcw,"IfcFile"):
            self._fileobject = ifcw.IfcFile()
        else:
            self._fileobject = ifcw.file()
        self._person = create(self._fileobject,"IfcPerson",[None,None,"",None,None,None,None,None])
        self._org = create(self._fileobject,"IfcOrganization",[None,"",None,None,None])
        pno = create(self._fileobject,"IfcPersonAndOrganization",[self._person,self._org,None])
        app = create(self._fileobject,"IfcApplication",[self._org,version,application,uid()])
        self._owner = create(self._fileobject,"IfcOwnerHistory",[pno,app,None,"ADDED",None,pno,app,now()])
        axp = self.addPlacement(local=False)
        dim0 = create(self._fileobject,"IfcDirection",getTuple((0,1,0)))
        self._repcontext = create(self._fileobject,"IfcGeometricRepresentationContext",['Plan','Model',3,1.E-05,axp,dim0])
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
        self.Site = None
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
                self._person.set_argument(2,str(value))
            elif key == "Organization":
                self._org.set_argument(1,str(value))
            elif key == "Name":
                self.Project.set_argument(2,str(value))
        self.__dict__.__setitem__(key,value)
        
    def findByName(self,ifctype,name):
        "finds an entity of a given ifctype by name"
        objs = self._fileobject.by_type(ifctype)
        for obj in objs:
            if hasattr(obj,"get_argument_count"):
                l = obj.get_argument_count()
            else:
                l = len(obj)
            for i in range(l):
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
                if APPLYFIX:
                    print ("IfcWriter: Applying fix...")
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
                if "(=IFC" in l:
                    # bug 1: adding an ifc entity without ID adds an unwanted = sign
                    l = l.replace("(=IFC","(IFC")
                #elif ("FACEBOUND" in l) or ("FACEOUTERBOUND" in l): # FIXED
                    # bug 2: booleans are exported as ints
                    #l = l.replace(",1);",",.T.);")
                    #l = l.replace(",0);",",.F.);")
                #elif "FILE_DESCRIPTION" in l: # FIXED
                    # bug 3: incomplete file description header
                    #l = l.replace("ViewDefinition []","ViewDefinition [CoordinationView_V2.0]")
                #elif "FILE_NAME" in l: # FIXED
                    # bug 4: incomplete file name entry
                    #l = l.replace("FILE_NAME('','',(''),('',''),'IfcOpenShell','IfcOpenShell','');","FILE_NAME('"+path+"','"+now(string=True)+"',('"+self.Owner+"'),('',''),'IfcOpenShell','IfcOpenShell','');")
                elif "IFCSIUNIT" in l:
                    # bug 5: no way to insert * character
                    l = l.replace("IFCSIUNIT(#13,","IFCSIUNIT(*,")
                lines.append(l)
            f.close()
            f = open(path,"wb")
            for l in lines:
                f.write(l)
            f.close()

    def union(self,solids):
        """union(solids): creates a boolean union between all the solids of the list"""
        if len(solids) == 1:
            return solids[0]
        else:
            s1 = solids.pop(0)
            s2 = solids.pop(0)
            base = create(self._fileobject,"IfcBooleanResult",["UNION",s1,s2])
            for s in solids:
                base = create(self._fileobject,"IfcBooleanResult",["UNION",base,s])
            return base

    def addPlacement(self,reference=None,origin=(0,0,0),xaxis=(1,0,0),zaxis=(0,0,1),local=True,flat=False):
        """addPlacement([reference,origin,xaxis,zaxis,local]): adds a placement. origin,
        xaxis and zaxis can be either tuples or 3d vectors. If local is False, a global
        placement is returned, otherwise a local one."""
        if flat:
            xvc = create(self._fileobject,"IfcDirection",getTuple(xaxis)[:2])
            ovc = create(self._fileobject,"IfcCartesianPoint",getTuple(origin)[:2])
            gpl = create(self._fileobject,"IfcAxis2Placement2D",[ovc,xvc])
        else:
            xvc = create(self._fileobject,"IfcDirection",getTuple(xaxis))
            zvc = create(self._fileobject,"IfcDirection",getTuple(zaxis))
            ovc = create(self._fileobject,"IfcCartesianPoint",getTuple(origin))
            gpl = create(self._fileobject,"IfcAxis2Placement3D",[ovc,zvc,xvc])
        if local:
            lpl = create(self._fileobject,"IfcLocalPlacement",[reference,gpl])
            return lpl
        else:
            return gpl
            
    def addSite(self,placement=None,name="Site",description=None,latitude=None,longitude=None,elevation=None,landtitlenumber=None,address=None):
        """makeSite(ifcdoc,project,owner,[placement,name,description]): creates a site
        in the given ifc document"""
        if self.Site:
            return
        if not placement:
            placement = self.addPlacement()
        self.Site = create(self._fileobject,"IfcSite",[uid(),self._owner,str(name),description,None,placement,None,None,"ELEMENT",latitude,longitude,elevation,landtitlenumber,address])
        self._relate(self.Project,self.Site)
                    
    def addBuilding(self,placement=None,name="Default building",description=None):
        """addBuilding([placement,name,description]): adds a building"""
        if not placement:
            placement = self.addPlacement()
        if not self.Site:
            self.addSite()
        bdg = create(self._fileobject,"IfcBuilding",[uid(),self._owner,str(name),description,None,placement,None,None,"ELEMENT",None,None,None])
        self._relate(self.Site,bdg)
        self.Buildings.append(bdg)
        return bdg
        
    def addStorey(self,building=None,placement=None,name="Default storey",description=None):
        """addStorey([building,placement,name,description]): adds a storey"""
        if not placement:
            placement = self.addPlacement()
        sto = create(self._fileobject,"IfcBuildingStorey",[uid(),self._owner,str(name),description,None,placement,None,None,"ELEMENT",None])
        if not building:
            if self.Buildings:
                building = self.Buildings[0]
            else:
                building = self.addBuilding()
        self._relate(building,sto)
        self.Storeys.append(sto)
        return sto
        
    def addGroup(self,entities,name="Default group",description=None):
        """addGroup(entities,[name,description]): adds a group with the given entities"""
        if not isinstance(entities,list):
            entities = [entities]
        gro = create(self._fileobject,"IfcGroup",[uid(),self._owner,str(name),description,None])
        rel = create(self._fileobject,"IfcRelAssignsToGroup",[uid(),self._owner,str(name)+"-relation",None,entities,"PRODUCT",gro])
        return gro
        
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
            if entities[0].is_a("IfcOpeningElement"):
                create(self._fileobject,"IfcRelVoidsElement",[uid(),self._owner,'Opening','',container,entities[0]])
            else:
                create(self._fileobject,"IfcRelAggregates",[uid(),self._owner,'Relationship','',container,entities])

    def addProduct(self,elttype,shapes,storey=None,placement=None,name="Unnamed element",description=None,extra=None):
        """addProduct(elttype,representations,[storey,placement,name,description,extra]): creates an element of the given type
        (IfcWall, IfcBeam, etc...) with the given attributes, plus the given extra attributes."""
        elttype = str(elttype)
        if not extra:
            extra = []
        if not description:
            description = None
        if not placement:
            placement = self.addPlacement()
        representations = self.addRepresentations(shapes)
        prd = create(self._fileobject,"IfcProductDefinitionShape",[None,None,representations])
        try:
            elt = create(self._fileobject,elttype,[uid(),self._owner,name,description,None,placement,prd,None]+extra)
        except:
            print "unable to create an ",elttype, " with attributes: ",[uid(),self._owner,str(name),description,None,placement,prd,None]+extra
            try:
                if hasattr(ifcw,"Entity"):
                    o = ifcw.Entity(elttype)
                else:
                    o = ifcw.entity_instance(elttype)
                print "supported attributes are: "
                print getPropertyNames(o)
            except:
                print "unable to create an element of type '"+elttype+"'"
            print "WARNING: skipping object '"+name+"' of type "+elttype
            return None
        self.BuildingProducts.append(elt)
        if not storey:
            if self.Storeys:
                storey = self.Storeys[0]
            else:
                storey = self.addStorey()
        self._relate(storey,elt)
        return elt

    def addRepresentations(self,shapes):
        """addRepresentations(shapes,[solidType]): creates a representation from the given shape"""
        solidType = "Brep"
        if not isinstance(shapes,list):
            if shapes.is_a("IfcExtrudedAreaSolid"):
                solidType = "SweptSolid"
            shapes = [shapes]
        reps = [create(self._fileobject,"IfcShapeRepresentation",[self._repcontext,'Body',solidType,[shape for shape in shapes]])]
        return reps

    def addColor(self,rgb,rep):
        """addColor(rgb,rep): adds a RGB color definition tuple (float,float,float) to a given representation"""
        col = create(self._fileobject,"IfcColourRgb",[None]+list(rgb))
        ssr = create(self._fileobject,"IfcSurfaceStyleRendering",[col,None,None,None,None,None,None,None,"FLAT"])
        iss = create(self._fileobject,"IfcSurfaceStyle",[None,"BOTH",[ssr]])
        psa = create(self._fileobject,"IfcPresentationStyleAssignment",[[iss]])
        isi = create(self._fileobject,"IfcStyledItem",[rep,[psa],None])
        return isi
        
    def addProfile(self,ifctype,data,curvetype="AREA"):
        """addProfile(ifctype,data): creates a 2D profile of the given type, with the given
        data as arguments, which must be formatted correctly according to the type."""
        
        # Expected ifctype and corresponding data formatting:
        # IfcPolyLine: [ (0,0,0), (2,1,0), (3,3,0) ] # list of points
        # IfcCompositeCurve: [ ["line",[ (0,0,0), (2,1,0) ] ], # list of points
        #                      ["arc", (0,0,0), 15, [0.76, 3.1416], True, "PARAMETER"] # center, radius, [trim1, trim2], SameSense, trimtype
        #                      ... ]
        # IfcCircleProfileDef: [ (0,0,0), 15 ] # center, radius
        # IfcEllipseProfileDef: [ (0,0,0), 15, 7 ] # center, radiusX, radiusY
        
        if ifctype == "IfcPolyline":
            pts = [create(self._fileobject,"IfcCartesianPoint",getTuple(p)[:2]) for p in data]
            pol = create(self._fileobject,"IfcPolyline",[pts])
            profile = create(self._fileobject,"IfcArbitraryClosedProfileDef",[curvetype,None,pol])
        elif ifctype == "IfcCompositeCurve":
            curves = []
            for curve in data:
                cur = None
                if curve[0] == "line":
                    pts = [create(self._fileobject,"IfcCartesianPoint",getTuple(p)[:2]) for p in curve[1]]
                    cur = create(self._fileobject,"IfcPolyline",[pts])
                elif curve[0] == "arc":
                    pla = self.addPlacement(origin=curve[1],local=False,flat=True)
                    cir = create(self._fileobject,"IfcCircle",[pla,curve[2]])
                    if curve[5] == "CARTESIAN":
                        # BUGGY! Impossible to add cartesian points as "embedded" entity
                        trim1 = create(None,"IfcCartesianPoint",getTuple(curve[3][0])[:2])
                        trim2 = create(None,"IfcCartesianPoint",getTuple(curve[3][1])[:2])
                    else:
                        trim1 = create(None,"IfcParameterValue",[curve[3][0]])
                        trim2 = create(None,"IfcParameterValue",[curve[3][1]])
                    cur = create(self._fileobject,"IfcTrimmedCurve",[cir,[trim1],[trim2],curve[4],curve[5]])
                if cur:
                    seg = create(self._fileobject,"IfcCompositeCurveSegment",["CONTINUOUS",True,cur])
                    curves.append(seg)
            ccu = create(self._fileobject,"IfcCompositeCurve",[curves,False])
            profile = create(self._fileobject,"IfcArbitraryClosedProfileDef",[curvetype,None,ccu])
        else:
            if not isinstance(data,list):
                data = [data]
            p = self.addPlacement(local=False,flat=True)
            profile = create(self._fileobject,ifctype,[curvetype,None,p]+data)
        return profile

    def addExtrusion(self,profile,extrusion,placement=None):
        """addExtrusion(profile,extrusion,[placement]): makes an
        extrusion of the given polyline with the given extrusion vector"""
        if not placement:
            placement = self.addPlacement(local=False)
        value,norm = getValueAndDirection(extrusion)
        edir = create(self._fileobject,"IfcDirection",[norm])
        solid = create(self._fileobject,"IfcExtrudedAreaSolid",[profile,placement,edir,value])
        return solid
        
    def addExtrudedPolyline(self,points,extrusion,placement=None,color=None):
        """addExtrudedPolyline(points,extrusion,[placement,color]): makes an extruded polyline
        from the given points and the given extrusion vector"""
        pol = self.addProfile("IfcPolyline",points)
        if not placement:
            placement = self.addPlacement(local=False)
        exp = self.addExtrusion(pol,extrusion,placement)
        if color:
            self.addColor(color,exp)
        return exp

    def addExtrudedCircle(self,data,extrusion,placement=None,color=None):
        """addExtrudedCircle(data,extrusion,[placement,color]): makes an extruded circle
        from the given data (center,radius) and the given extrusion vector"""
        cir = self.addProfile("IfcCircleProfileDef",data[1])
        if not placement:
            placement = self.addPlacement(origin=data[0],local=False)
        exp = self.addExtrusion(cir,extrusion,placement)
        if color:
            self.addColor(color,exp)
        return exp
        
    def addExtrudedEllipse(self,data,extrusion,placement=None,color=None):
        """addExtrudedEllipse(data,extrusion,[placement,color]): makes an extruded ellipse
        from the given data (center,radiusx,radiusy) and the given extrusion vector"""
        cir = self.addProfile("IfcEllipseProfileDef",[data[1],data[2]])
        if not placement:
            placement = self.addPlacement(origin=data[0],local=False)
        exp = self.addExtrusion(cir,extrusion,placement)
        if color:
            self.addColor(color,exp)
        return exp
        
    def addExtrudedCompositeCurve(self,curves,extrusion,placement=None,color=None):
        """addExtrudedCompositeCurve(curves,extrusion,[placement,color]): makes an extruded polyline
        from the given curves and the given extrusion vector"""
        if not placement:
            placement = self.addPlacement(local=False)
        ccu = self.addProfile("IfcCompositeCurve",curves)
        exp = self.addExtrusion(ccu,extrusion,placement)
        if color:
            self.addColor(color,exp)
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

    def addFacetedBrep(self,faces,color=None):
        """addFacetedBrep(self,faces,[color]): creates a faceted brep object from the given list
        of faces (each face is a list of lists of points, inner wires are reversed)"""
        self.fpoints = []
        self.frefs = []
        #print "adding ",len(faces)," faces"
        #print faces
        ifaces = [self.addFace(face) for face in faces]
        sh = create(self._fileobject,"IfcClosedShell",[ifaces])
        brp = create(self._fileobject,"IfcFacetedBrep",[sh])
        if color:
            self.addColor(color,brp)
        return brp
        

# EXAMPLE #################################################################
    
def example():
        
    ifc = IfcDocument("/home/yorik/test2.ifc")
    ifc.Name = "Test Project"
    ifc.Owner = "Yorik van Havre"
    ifc.Organization = "FreeCAD"
    w1 = ifc.addProduct( "IfcWall", ifc.addExtrudedPolyline([(0,0,0),(0,200,0),(5000,200,0),(5000,0,0),(0,0,0)], (0,0,3500)) )
    ifc.addProduct( "IfcWall", ifc.addExtrudedPolyline([(0,200,0),(0,2000,0),(200,2000,0),(200,200,0),(0,200,0)],(0,0,3500)) )
    ifc.addProduct( "IfcWall", ifc.addExtrudedPolyline([(0,2000,0),(0,2200,0),(5000,2200,0),(5000,2000,0),(0,2000,0)],(0,0,3500)) )
    ifc.addProduct( "IfcWall", ifc.addExtrudedPolyline([(5000,200,0),(5000,2000,0),(4800,2000,0),(4800,200,0),(5000,200,0)],(0,0,3500)) )
    ifc.addProduct( "IfcWall", ifc.addFacetedBrep([[[(0,0,0),(100,0,0),(100,-1000,0),(0,-1000,0)]],
                                    [[(0,0,0),(100,0,0),(100,0,1000),(0,0,1000)]],
                                    [[(0,0,0),(0,0,1000),(0,-1000,1000),(0,-1000,0)]],
                                    [[(0,-1000,0),(0,-1000,1000),(100,-1000,1000),(100,-1000,0)]],
                                    [[(100,-1000,0),(100,-1000,1000),(100,0,1000),(100,0,0)]],
                                    [[(0,0,1000),(0,-1000,1000),(100,-1000,1000),(100,0,1000)]]]) )
    ifc.addProduct( "IfcColumn", ifc.addExtrudedPolyline([(0,0,0),(0,-200,0),(-500,-200,0),(-500,0,0),(0,0,0)], (0,0,3500)) )
    ifc.addProduct( "IfcDoor", ifc.addExtrudedPolyline([(200,200,0),(200,400,0),(400,400,0),(400,200,0),(200,200,0)], (0,0,200)), w1, [200, 200] )
    ifc.write()
    
    print dir(ifc._fileobject)
    print ifc._fileobject.by_type("IfcDoor")
    w = ifc._fileobject.by_type("IfcDoor")[0]
    print w
    print dir(w)
    print w.is_a("IfcDoor")
    for i in range(w.get_argument_count()):
        print i,": ",w.get_argument_name(i)," : ",w.get_argument(i)

