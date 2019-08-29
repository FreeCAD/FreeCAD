#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2012 Sebastian Hoogen <github@sebastianhoogen.de>       *
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

__title__="FreeCAD OpenSCAD Workbench - Parametric Features"
__author__ = "Sebastian Hoogen"
__url__ = ["http://www.freecadweb.org"]

try:
    long
except NameError:
    long = int

'''
This Script includes python Features to represent OpenSCAD Operations
'''
class ViewProviderTree:
    "A generic View Provider for Elements with Children"
        
    def __init__(self, obj):
        obj.Proxy = self
        self.Object = obj.Object
        
    def attach(self, obj):
        self.Object = obj.Object
        return

    def updateData(self, fp, prop):
        return

    def getDisplayModes(self,obj):
        modes=[]
        return modes

    def setDisplayMode(self,mode):
        return mode

    def onChanged(self, vp, prop):
        return

    def __getstate__(self):
#        return {'ObjectName' : self.Object.Name}
        return None

    def __setstate__(self,state):
        if state is not None:
            import FreeCAD
            doc = FreeCAD.ActiveDocument #crap
            self.Object = doc.getObject(state['ObjectName'])

    def claimChildren(self):
        objs = []
        if hasattr(self.Object.Proxy,"Base"):
            objs.append(self.Object.Proxy.Base)
        if hasattr(self.Object,"Base"):
            objs.append(self.Object.Base)
        if hasattr(self.Object,"Objects"):
            objs.extend(self.Object.Objects)
        if hasattr(self.Object,"Components"):
            objs.extend(self.Object.Components)
        if hasattr(self.Object,"Children"):
            objs.extend(self.Object.Children)

        return objs
   
    def getIcon(self):
        import OpenSCAD_rc
        if isinstance(self.Object.Proxy,RefineShape):
            return(":/icons/OpenSCAD_RefineShapeFeature.svg")
        if isinstance(self.Object.Proxy,IncreaseTolerance):
            return(":/icons/OpenSCAD_IncreaseToleranceFeature.svg")
        if isinstance(self.Object.Proxy,MatrixTransform):
            return """/* XPM */
static char * matrix_xpm[] = {
"16 16 3 1",
" 	c #0079FF",
".	c #FFFFFF",
"+	c #000000",
"   .........   .",
" ............. .",
" .  .  .  .  . .",
" .  .  .  .  . .",
" ............. .",
" .  .  .  .  . .",
" .  .  .  .  . .",
" ............. .",
" .  .  .  .  . .",
" .  .  .  .  . .",
" ............. .",
" ...........+. .",
" ..+..+..+..+. .",
" ............. .",
"   .........   .",
"................"};"""
        else:
            return """/* XPM */
static char * openscadlogo_xpm[] = {
"16 16 33 1",
" 	c None",
".	c #61320B",
"+	c #5D420B",
"@	c #4F4C09",
"#	c #564930",
"$	c #754513",
"%	c #815106",
"&	c #666509",
"*	c #875F55",
"=	c #6E7000",
"-	c #756A53",
";	c #717037",
">	c #946637",
",	c #92710E",
"'	c #797A0A",
")	c #7C7720",
"!	c #8A8603",
"~	c #88886F",
"{	c #AF8181",
"]	c #999908",
"^	c #BB8D8D",
"/	c #AAAA00",
"(	c #A9A880",
"_	c #B5B419",
":	c #C1A9A9",
"<	c #B1B19A",
"[	c #BEBE00",
"}	c #B9B8B4",
"|	c #CACC00",
"1	c #D4D4BC",
"2	c #DBD2D0",
"3	c #EEEEED",
"4	c #FDFFFC",
"4444444444444444",
"4444443113444444",
"4444<;']]!;<^^24",
"444(&@!]]]=&#^{3",
"44<']')@++)!&*{^",
"44)]/[|//[/]'@{{",
"42=/_|||||[]!&*{",
"4(&][|||||[/'@#}",
"3-..,|||||[)&&~4",
"^*$%.!|||[!+/](4",
"^{%%%._[[_&/[_14",
":{>%%.!//])_[_44",
"2{{%%+!]!!)]]344",
"4:{{#@&=&&@#3444",
"44224}~--~}44444",
"4444444444444444"};
"""

class OpenSCADPlaceholder:
    def __init__(self,obj,children=None,arguments=None):
        obj.addProperty("App::PropertyLinkList",'Children','OpenSCAD',"Base Objects")
        obj.addProperty("App::PropertyString",'Arguments','OpenSCAD',"Arguments")
        obj.Proxy = self
        if children:
            obj.Children = children
        if arguments:
            obj.Arguments = arguments
             
    def execute(self,fp):
        import Part
        fp.Shape = Part.Compound([]) #empty Shape

class MatrixTransform:
    def __init__(self, obj,matrix=None,child=None):
        obj.addProperty("App::PropertyLink","Base","Base",
                        "The base object that must be tranfsformed")
        obj.addProperty("App::PropertyMatrix","Matrix","Matrix", "Transformation Matrix")
        obj.Proxy = self
        obj.Matrix = matrix
        obj.Base = child

    def onChanged(self, fp, prop):
        "Do something when a property has changed"
        pass

    def updateProperty(self, fp, prop, value):
        epsilon = 0.0001
        if abs(getattr(fp, prop) - value) > epsilon:
            setattr(fp, prop, value)

    def execute(self, fp):
        pass
        if fp.Matrix and fp.Base:
            sh=fp.Base.Shape#.copy()
            m=sh.Placement.toMatrix().multiply(fp.Matrix)
            fp.Shape = sh.transformGeometry(m)
        #else:
            #FreeCAD.Console.PrintMessage('base %s\nmat %s/n' % (fp.Base,fp.Matrix))

class ImportObject:
    def __init__(self, obj,child=None):
        obj.addProperty("App::PropertyLink","Base","Base",
                        "The base object that must be tranfsformed")
        obj.Proxy = self
        obj.Base = child

    def onChanged(self, fp, prop):
        "Do something when a property has changed"
        pass

    def execute(self, fp):
        pass
#        if fp.Base:
#            fp.Shape = fp.Base.Shape.copy()

class RefineShape:
    '''return a refined shape'''
    def __init__(self, obj,child=None):
        obj.addProperty("App::PropertyLink","Base","Base",
                        "The base object that must be refined")
        obj.Proxy = self
        obj.Base = child

    def onChanged(self, fp, prop):
        "Do something when a property has changed"
        pass

    def execute(self, fp):
        if fp.Base and fp.Base.Shape.isValid():
            import OpenSCADUtils
            sh=fp.Base.Shape.removeSplitter()
            fp.Shape=OpenSCADUtils.applyPlacement(sh)

class IncreaseTolerance:
    '''increase the tolerance of every vertex
    in the current implementation its' placement is linked'''
    def __init__(self,obj,child,tolerance=0):
        obj.addProperty("App::PropertyLink","Base","Base",
                        "The base object that wire must be extracted")
        obj.addProperty("App::PropertyDistance","Vertex","Tolerance","Vertexes tolerance (0 default)")
        obj.addProperty("App::PropertyDistance","Edge","Tolerance","Edges tolerance (0 default)")
        obj.addProperty("App::PropertyDistance","Face","Tolerance","Faces tolerance (0 default)")
        obj.Base = child
        obj.Vertex = tolerance
        obj.Edge = tolerance
        obj.Face = tolerance
        obj.Proxy = self

    def onChanged(self, fp, prop):
        # Tolerance property left for backward compatibility
        if prop in ["Vertex", "Edge", "Face", "Tolerance"]:
            self.createGeometry(fp)

    def execute(self, fp):
        self.createGeometry(fp)

    def createGeometry(self,fp):
        if fp.Base:
            sh=fp.Base.Shape.copy()
            # Check if property Tolerance exist and preserve support for backward compatibility
            if hasattr(fp, "Tolerance") and fp.Proxy.__module__ == "OpenSCADFeatures":
                for vertex in sh.Vertexes:
                    vertex.Tolerance = max(vertex.Tolerance,fp.Tolerance.Value)
            # New properties
            else:
                for vertex in sh.Vertexes:
                    vertex.Tolerance = max(vertex.Tolerance,fp.Vertex.Value)
                for edge in sh.Edges:
                    edge.Tolerance = max(edge.Tolerance,fp.Edge.Value)
                for face in sh.Faces:
                    face.Tolerance = max(face.Tolerance,fp.Face.Value)

            fp.Shape = sh
            fp.Placement = sh.Placement


class GetWire:
    '''return the first wire from a given shape'''
    def __init__(self, obj,child=None):
        obj.addProperty("App::PropertyLink","Base","Base",
                        "The base object that wire must be extracted")
        obj.Proxy = self
        obj.Base = child

    def onChanged(self, fp, prop):
        "Do something when a property has changed"
        pass

    def execute(self, fp):
        if fp.Base:
            import Part
            #fp.Shape=fp.Base.Shape.Wires[0]
            fp.Shape=Part.Wire(fp.Base.Shape.Wires[0]) # works with 0.13 stable
            #sh = fp.Base.Shape.Wires[0].copy; sh.transformSahpe(fp.Base.Shape.Placement.toMatrix()); fp.Shape = sh #untested

class Frustum:
    def __init__(self, obj,r1=1,r2=2,n=3,h=4):
        obj.addProperty("App::PropertyInteger","FacesNumber","Base","Number of faces")
        obj.addProperty("App::PropertyDistance","Radius1","Base","Radius of lower the inscribed control circle")
        obj.addProperty("App::PropertyDistance","Radius2","Base","Radius of upper the inscribed control circle")
        obj.addProperty("App::PropertyDistance","Height","Base","Height of the Frustum")

        obj.FacesNumber = n
        obj.Radius1 = r1
        obj.Radius2=  r2
        obj.Height= h
        obj.Proxy = self

    def execute(self, fp):
        self.createGeometry(fp)

    def onChanged(self, fp, prop):
        if prop in ["FacesNumber","Radius1","Radius2","Height"]:
            self.createGeometry(fp)

    def createGeometry(self,fp):
        if all((fp.Radius1,fp.Radius2,fp.FacesNumber,fp.Height)):
            import math
            import FreeCAD,Part
            #from draftlibs import fcgeo
            plm = fp.Placement
            wires=[]
            faces=[]
            for ir,r in enumerate((fp.Radius1,fp.Radius2)):
                angle = (math.pi*2)/fp.FacesNumber
                pts = [FreeCAD.Vector(r.Value,0,ir*fp.Height.Value)]
                for i in range(fp.FacesNumber-1):
                    ang = (i+1)*angle
                    pts.append(FreeCAD.Vector(r.Value*math.cos(ang),\
                            r.Value*math.sin(ang),ir*fp.Height.Value))
                pts.append(pts[0])
                shape = Part.makePolygon(pts)
                face = Part.Face(shape)
                if ir==1: #top face
                    face.reverse()
                wires.append(shape)
                faces.append(face)
            #shellperi=Part.makeRuledSurface(*wires)
            shellperi=Part.makeLoft(wires)
            shell=Part.Shell(shellperi.Faces+faces)
            fp.Shape = Part.Solid(shell)
            fp.Placement = plm

class Twist:
    def __init__(self, obj,child=None,h=1.0,angle=0.0):
        obj.addProperty("App::PropertyLink","Base","Base",
                        "The base object that must be tranfsformed")
        obj.addProperty("App::PropertyAngle","Angle","Base","Twist Angle in degrees") #degree or rad
        obj.addProperty("App::PropertyDistance","Height","Base","Height of the Extrusion")

        obj.Base = child
        obj.Angle =  angle
        obj.Height = h
        obj.Proxy = self

    def execute(self, fp):
        self.createGeometry(fp)

    def onChanged(self, fp, prop):
        pass
        #if prop in ["Angle","Height"]:
        #    self.createGeometry(fp)

    def createGeometry(self,fp):
        import FreeCAD,Part,math,sys
        #tangle = -twist #openscad uses degrees clockwise
        if fp.Base and fp.Angle and fp.Height and \
            fp.Base.Shape.isValid():
        #wire=fp.Base.Shape.Wires[0].transformGeometry(fp.Base.Placement.toMatrix()) 
            solids=[]
            for faceb in fp.Base.Shape.Faces:
            #fp.Base.Shape.Faces[0].check()

            #faceb=fp.Base.Shape.Faces[0]
            #faceb=fp.Base.Shape.removeSplitter().Faces[0]
                faceu=faceb.copy()
                facetransform=FreeCAD.Matrix()
                facetransform.rotateZ(math.radians(fp.Angle.Value))
                facetransform.move(FreeCAD.Vector(0,0,fp.Height.Value))
                faceu.transformShape(facetransform)
                step = 2 + abs(int(fp.Angle.Value // 90)) #resolution in z direction
                # print abs(int(fp.Angle.Value // 90)) #resolution in z direction
                # print step
                zinc = fp.Height.Value/(step-1.0)
                angleinc = math.radians(fp.Angle.Value)/(step-1.0)
                spine = Part.makePolygon([(0,0,i*zinc) \
                        for i in range(step)])
                auxspine = Part.makePolygon([(math.cos(i*angleinc),\
                        math.sin(i*angleinc),i*fp.Height.Value/(step-1)) \
                        for i in range(step)])
                faces=[faceb,faceu]
                for wire in faceb.Wires:
                    pipeshell=Part.BRepOffsetAPI.MakePipeShell(spine)
                    pipeshell.setSpineSupport(spine)
                    pipeshell.add(wire)
                    # Was before function change
                    # pipeshell.setAuxiliarySpine(auxspine,True,False)
                    if sys.version_info.major < 3:
                        pipeshell.setAuxiliarySpine(auxspine,True,long(0))
                    else:
                        pipeshell.setAuxiliarySpine(auxspine,True,0)
                    print(pipeshell.getStatus())
                    assert(pipeshell.isReady())
                    #fp.Shape=pipeshell.makeSolid()
                    pipeshell.build()
                    faces.extend(pipeshell.shape().Faces)
                try:
                    fullshell=Part.Shell(faces)
                    solid=Part.Solid(fullshell)
                    if solid.Volume < 0:
                        solid.reverse()
                    assert(solid.Volume >= 0)
                    solids.append(solid)
                except Part.OCCError:
                    solids.append(Part.Compound(faces))
                fp.Shape=Part.Compound(solids)

class OffsetShape:
    def __init__(self, obj,child=None,offset=1.0):
        obj.addProperty("App::PropertyLink","Base","Base",
                        "The base object that must be tranfsformed")
        obj.addProperty("App::PropertyDistance","Offset","Base","Offset outwards")

        obj.Base = child
        obj.Offset = offset
        obj.Proxy = self

    def execute(self, fp):
        self.createGeometry(fp)

    def onChanged(self, fp, prop):
        pass
        if prop in ["Offset"]:
            self.createGeometry(fp)

    def createGeometry(self,fp):
        if fp.Base and fp.Offset:
            fp.Shape=fp.Base.Shape.makeOffsetShape(fp.Offset.Value,1e-6)

class CGALFeature:
    def __init__(self,obj,opname=None,children=None,arguments=None):
        obj.addProperty("App::PropertyLinkList",'Children','OpenSCAD',"Base Objects")
        obj.addProperty("App::PropertyString",'Arguments','OpenSCAD',"Arguments")
        obj.addProperty("App::PropertyString",'Operation','OpenSCAD',"Operation")
        obj.Proxy = self
        if opname:
            obj.Operation = opname
        if children:
            obj.Children = children
        if arguments:
            obj.Arguments = arguments

    def execute(self,fp):
        #arguments are ignored
        maxmeshpoints = None #TBD: add as property
        import Part,OpenSCADUtils
        shape = OpenSCADUtils.process_ObjectsViaOpenSCADShape(fp.Document,fp.Children,\
                fp.Operation, maxmeshpoints=maxmeshpoints)
        if shape:
            fp.Shape = shape
        else:
            raise ValueError

def makeSurfaceVolume(filename):
    import FreeCAD,Part
    f1=open(filename)
    coords=[]
    miny=1
    for line in f1.readlines():
        sline=line.strip()
        if sline and not sline.startswith('#'):
            ycoord=len(coords)
            lcoords=[]
            for xcoord, num in enumerate(sline.split()):
                fnum=float(num)
                lcoords.append(FreeCAD.Vector(float(xcoord),float(ycoord),fnum))
                miny=min(fnum,miny)
            coords.append(lcoords)
    s=Part.BSplineSurface()
    s.interpolate(coords)
    plane=Part.makePlane(len(coords[0])-1,len(coords)-1,FreeCAD.Vector(0,0,miny-1))
    l1=Part.makeLine(plane.Vertexes[0].Point,s.value(0,0))
    l2=Part.makeLine(plane.Vertexes[1].Point,s.value(1,0))
    l3=Part.makeLine(plane.Vertexes[2].Point,s.value(0,1))
    l4=Part.makeLine(plane.Vertexes[3].Point,s.value(1,1))
    f0=plane.Faces[0]
    f0.reverse()
    f1=Part.Face(Part.Wire([plane.Edges[0],l1.Edges[0],s.vIso(0).toShape(),l2.Edges[0]]))
    f2=Part.Face(Part.Wire([plane.Edges[1],l3.Edges[0],s.uIso(0).toShape(),l1.Edges[0]]))
    f3=Part.Face(Part.Wire([plane.Edges[2],l4.Edges[0],s.vIso(1).toShape(),l3.Edges[0]]))
    f4=Part.Face(Part.Wire([plane.Edges[3],l2.Edges[0],s.uIso(1).toShape(),l4.Edges[0]]))
    f5=s.toShape().Faces[0]
    solid=Part.Solid(Part.Shell([f0,f1,f2,f3,f4,f5]))
    return solid,(len(coords[0])-1)/2.0,(len(coords)-1)/2.0
