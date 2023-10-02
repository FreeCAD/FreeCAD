#***************************************************************************
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

__title__ = "FreeCAD OpenSCAD Workbench - Parametric Features"
__author__ = "Sebastian Hoogen"
__url__ = ["https://www.freecad.org"]

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

    def dumps(self):
#        return {'ObjectName' : self.Object.Name}
        return None

    def loads(self,state):
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


class Resize:
    def __init__(self,obj,target,vector):
        import FreeCAD
        #self.Obj = obj
        self.Target = target
        self.Vector = vector
        #obj.addProperty("App::PropertyPythonObject","Object","Resize", \
        #                "Object to be resized").Object = target
        obj.addProperty("Part::PropertyPartShape","Shape","Resize", "Shape of the Resize")
        obj.addProperty("App::PropertyVector","Vector","Resize",
                        " Resize Vector").Vector = FreeCAD.Vector(vector)
        obj.Proxy = self

    def onChanged(self, fp, prop):
        if prop in ['Object','Vector']:
            self.createGeometry(fp)

    def execute(self, fp):
        self.createGeometry(fp)

    def createGeometry(self, fp):
        import FreeCAD
        mat = FreeCAD.Matrix()
        mat.A11 = self.Vector[0]
        mat.A22 = self.Vector[1]
        mat.A33 = self.Vector[2]
        fp.Shape = self.Target.Shape.transformGeometry(mat)

    def dumps(self):
        return None

    def loads(self,state):
        return None


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
        if fp.Matrix and fp.Base:
            sh = fp.Base.Shape#.copy()
            m = sh.Placement.toMatrix().multiply(fp.Matrix)
            fp.Shape = sh.transformGeometry(m)
        #else:
            #FreeCAD.Console.PrintMessage('base %s\nmat %s/n' % (fp.Base,fp.Matrix))


class ImportObject:
    def __init__(self, obj,child=None):
        obj.addProperty("App::PropertyLink", "Base", "Base",
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
    def __init__(self, obj, child=None):
        obj.addProperty("App::PropertyLink", "Base", "Base",
                        "The base object that must be refined")
        obj.Proxy = self
        obj.Base = child

    def onChanged(self, fp, prop):
        "Do something when a property has changed"
        pass

    def execute(self, fp):
        if fp.Base and fp.Base.Shape.isValid():
            import OpenSCADUtils
            sh = fp.Base.Shape.removeSplitter()
            fp.Shape = OpenSCADUtils.applyPlacement(sh)

class IncreaseTolerance:
    '''increase the tolerance of every vertex
    in the current implementation its' placement is linked'''
    def __init__(self,obj,child,tolerance=0):
        obj.addProperty("App::PropertyLink", "Base", "Base",
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
                    vertex.Tolerance = max(vertex.Tolerance, fp.Vertex.Value)
                for edge in sh.Edges:
                    edge.Tolerance = max(edge.Tolerance, fp.Edge.Value)
                for face in sh.Faces:
                    face.Tolerance = max(face.Tolerance, fp.Face.Value)

            fp.Shape = sh
            fp.Placement = sh.Placement


class GetWire:
    '''return the first wire from a given shape'''
    def __init__(self, obj, child=None):
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
            import FreeCAD
            import Part
            #from draftlibs import fcgeo
            plm = fp.Placement
            wires = []
            faces = []
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
                if ir == 0: #top face
                    face.reverse()
                wires.append(shape)
                faces.append(face)
            #shellperi = Part.makeRuledSurface(*wires)
            shellperi = Part.makeLoft(wires)
            shell = Part.Shell(shellperi.Faces+faces)
            fp.Shape = Part.Solid(shell)
            fp.Placement = plm

class Twist:
    def __init__(self, obj, child=None, h=1.0, angle=0.0, scale=[1.0,1.0]):
        import FreeCAD
        obj.addProperty("App::PropertyLink","Base","Base",
                        "The base object that must be transformed")
        obj.addProperty("App::PropertyQuantity","Angle","Base","Twist Angle")
        obj.Angle = FreeCAD.Units.Angle # assign the Angle unit
        obj.addProperty("App::PropertyDistance","Height","Base","Height of the Extrusion")
        obj.addProperty("App::PropertyFloatList","Scale","Base","Scale to apply during the Extrusion")

        obj.Base = child
        obj.Angle = angle
        obj.Height = h
        obj.Scale = scale
        obj.Proxy = self

    def execute(self, fp):
        self.createGeometry(fp)

    def onChanged(self, fp, prop):
        if prop in ["Angle","Height","Scale"]:
            self.createGeometry(fp)

    def createGeometry(self, fp):
        import FreeCAD
        import Part
        import math
        import sys
        if fp.Base and fp.Height and fp.Base.Shape.isValid():
            solids = []
            for lower_face in fp.Base.Shape.Faces:
                upper_face = lower_face.copy()
                face_transform = FreeCAD.Matrix()
                face_transform.rotateZ(math.radians(fp.Angle.Value))
                face_transform.scale(fp.Scale[0], fp.Scale[1], 1.0)
                face_transform.move(FreeCAD.Vector(0,0,fp.Height.Value))
                upper_face.transformShape(face_transform, False, True) # True to check for non-uniform scaling

                spine = Part.makePolygon([(0,0,0),(0,0,fp.Height.Value)])
                if fp.Angle.Value == 0.0:
                    auxiliary_spine = None
                else:
                    num_revolutions = abs(fp.Angle.Value)/360.0
                    pitch = fp.Height.Value / num_revolutions
                    height = fp.Height.Value
                    radius = 1.0
                    if fp.Angle.Value < 0.0:
                        left_handed = True
                    else:
                        left_handed = False

                    auxiliary_spine = Part.makeHelix(pitch, height, radius, 0.0, left_handed)

                faces = [lower_face,upper_face]
                for wire1,wire2 in zip(lower_face.Wires,upper_face.Wires):
                    pipe_shell = Part.BRepOffsetAPI.MakePipeShell(spine)
                    pipe_shell.setSpineSupport(spine)
                    pipe_shell.add(wire1)
                    pipe_shell.add(wire2)
                    if auxiliary_spine:
                        pipe_shell.setAuxiliarySpine(auxiliary_spine,True,0)
                    assert(pipe_shell.isReady())
                    pipe_shell.build()
                    faces.extend(pipe_shell.shape().Faces)
                try:
                    fullshell = Part.Shell(faces)
                    solid=Part.Solid(fullshell)
                    if solid.Volume < 0:
                        solid.reverse()
                    assert(solid.Volume >= 0)
                    solids.append(solid)
                except Part.OCCError:
                    solids.append(Part.Compound(faces))
                fp.Shape=Part.Compound(solids)



class PrismaticToroid:
    def __init__(self, obj,child=None,angle=360.0,n=3):
        obj.addProperty("App::PropertyLink","Base","Base",
                        "The 2D face that will be swept")
        obj.addProperty("App::PropertyAngle","Angle","Base","Angle to sweep through")
        obj.addProperty("App::PropertyInteger","Segments","Base","Number of segments per 360° (OpenSCAD's \"$fn\")")

        obj.Base = child
        obj.Angle =  angle
        obj.Segments = n
        obj.Proxy = self

    def execute(self, fp):
        self.createGeometry(fp)

    def onChanged(self, fp, prop):
        if prop in ["Angle","Segments"]:
            self.createGeometry(fp)

    def createGeometry(self,fp):
        import FreeCAD
        import Part
        import math
        import sys
        if fp.Base and fp.Angle and fp.Segments and fp.Base.Shape.isValid():
            solids = []
            min_sweep_angle_per_segment = 360.0 / fp.Segments # This is how OpenSCAD defines $fn
            num_segments = math.floor(abs(fp.Angle) / min_sweep_angle_per_segment)
            num_ribs = num_segments + 1
            sweep_angle_per_segment = fp.Angle / num_segments # Always >= min_sweep_angle_per_segment

            # From the OpenSCAD documentation:
            # The 2D shape must lie completely on either the right (recommended) or the left side of the Y-axis.
            # More precisely speaking, every vertex of the shape must have either x >= 0 or x <= 0. If the shape
            # spans the X axis a warning appears in the console windows and the rotate_extrude() is ignored. If
            # the 2D shape touches the Y axis, i.e. at x=0, it must be a line that touches, not a point.

            for start_face in fp.Base.Shape.Faces:
                ribs = []
                end_face = start_face
                for rib in range(num_ribs):
                    angle = rib * sweep_angle_per_segment
                    intermediate_face = start_face.copy()
                    face_transform = FreeCAD.Matrix()
                    face_transform.rotateY (math.radians (angle))
                    intermediate_face.transformShape (face_transform)
                    if rib == num_ribs-1:
                        end_face = intermediate_face

                    edges = []
                    for edge in intermediate_face.OuterWire.Edges:
                        if edge.BoundBox.XMin != 0.0 or edge.BoundBox.XMax != 0.0:
                            edges.append(edge)

                    ribs.append(Part.Wire(edges))

                faces = []
                shell = Part.makeShellFromWires (ribs)
                for face in shell.Faces:
                    faces.append(face)

                if abs(fp.Angle) < 360.0 and faces:
                    if fp.Angle > 0:
                        faces.append(start_face.reversed()) # Reversed so the normal faces out of the shell
                        faces.append(end_face)
                    else:
                        faces.append(start_face)
                        faces.append(end_face.reversed()) # Reversed so the normal faces out of the shell

                try:
                    shell = Part.makeShell(faces)
                    shell.sewShape()
                    shell.fix(1e-7,1e-7,1e-7)
                    clean_shell = shell.removeSplitter()
                    solid = Part.makeSolid (clean_shell)
                    if solid.Volume < 0:
                        solid.reverse()
                    solids.append(solid)
                except Part.OCCError:
                    FreeCAD.Console.PrintWarning("Could not create solid: creating compound instead")
                    solids.append(Part.Compound(faces))
            fp.Shape = Part.Compound(solids)

class OffsetShape:
    def __init__(self, obj,child=None,offset=1.0):
        obj.addProperty("App::PropertyLink","Base","Base",
                        "The base object that must be transformed")
        obj.addProperty("App::PropertyDistance","Offset","Base","Offset outwards")

        obj.Base = child
        obj.Offset = offset
        obj.Proxy = self

    def execute(self, fp):
        self.createGeometry(fp)

    def onChanged(self, fp, prop):
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
        import Part
        import OpenSCAD.OpenSCADUtils
        shape = OpenSCAD.OpenSCADUtils.process_ObjectsViaOpenSCADShape(fp.Document,fp.Children,\
                fp.Operation, maxmeshpoints=maxmeshpoints)
        if shape:
            fp.Shape = shape
        else:
            raise ValueError

def makeSurfaceVolume(filename):
    import FreeCAD
    import Part
    import sys
    coords = []
    with open(filename) as f1:
        min_z = sys.float_info.max
        for line in f1.readlines():
            sline = line.strip()
            if sline and not sline.startswith('#'):
                ycoord = len(coords)
                lcoords = []
                for xcoord, num in enumerate(sline.split()):
                    fnum = float(num)
                    lcoords.append(FreeCAD.Vector(float(xcoord),float(ycoord),fnum))
                    min_z = min(fnum,min_z)
                coords.append(lcoords)

    num_rows = len(coords)
    if num_rows == 0:
        FreeCAD.Console.PrintWarning(f"No data found in surface file {filename}")
        return None,0,0
    num_cols = len(coords[0])

    # OpenSCAD does not spline this surface, so neither do we: just create a
    # bunch of faces,
    # using four triangles per quadrilateral
    faces = []
    for row in range(num_rows - 1):
        for col in range(num_cols - 1):
            a = coords[row + 0][col + 0]
            b = coords[row + 0][col + 1]
            c = coords[row + 1][col + 1]
            d = coords[row + 1][col + 0]
            centroid = 0.25 * (a + b + c + d)
            ab = Part.makeLine(a,b)
            bc = Part.makeLine(b,c)
            cd = Part.makeLine(c,d)
            da = Part.makeLine(d,a)

            diag_a = Part.makeLine(a, centroid)
            diag_b = Part.makeLine(b, centroid)
            diag_c = Part.makeLine(c, centroid)
            diag_d = Part.makeLine(d, centroid)

            wire1 = Part.Wire([ab,diag_a,diag_b])
            wire2 = Part.Wire([bc,diag_b,diag_c])
            wire3 = Part.Wire([cd,diag_c,diag_d])
            wire4 = Part.Wire([da,diag_d,diag_a])

            try:
                face = Part.Face(wire1)
                faces.append(face)
                face = Part.Face(wire2)
                faces.append(face)
                face = Part.Face(wire3)
                faces.append(face)
                face = Part.Face(wire4)
                faces.append(face)
            except Exception:
                FreeCAD.Console.PrintWarning("Failed to create the face from {},{},{},{}".format(coords[row + 0][col + 0],\
                    coords[row + 0][col + 1],coords[row + 1][col + 1],coords[row + 1][col + 0]))

    last_row = num_rows - 1
    last_col = num_cols - 1

    # Create the face to close off the y-min border: OpenSCAD places the lower
    # surface of the shell
    # at 1 unit below the lowest coordinate in the surface
    lines = []
    corner1 = FreeCAD.Vector(coords[0][0].x, coords[0][0].y, min_z - 1)
    lines.append(Part.makeLine(corner1,coords[0][0]))
    for col in range(num_cols - 1):
        a = coords[0][col]
        b = coords[0][col + 1]
        lines.append(Part.makeLine(a, b))
    corner2 = FreeCAD.Vector(coords[0][last_col].x, coords[0][last_col].y, min_z - 1)
    lines.append(Part.makeLine(corner2,coords[0][last_col]))
    lines.append(Part.makeLine(corner1,corner2))
    wire = Part.Wire(lines)
    face = Part.Face(wire)
    faces.append(face)

    # Create the face to close off the y-max border
    lines = []
    corner1 = FreeCAD.Vector(coords[last_row][0].x, coords[last_row][0].y, min_z - 1)
    lines.append(Part.makeLine(corner1,coords[last_row][0]))
    for col in range(num_cols - 1):
        a = coords[last_row][col]
        b = coords[last_row][col + 1]
        lines.append(Part.makeLine(a, b))
    corner2 = FreeCAD.Vector(coords[last_row][last_col].x, coords[last_row][last_col].y, min_z - 1)
    lines.append(Part.makeLine(corner2,coords[last_row][last_col]))
    lines.append(Part.makeLine(corner1,corner2))
    wire = Part.Wire(lines)
    face = Part.Face(wire)
    faces.append(face)

    # Create the face to close off the x-min border
    lines = []
    corner1 = FreeCAD.Vector(coords[0][0].x, coords[0][0].y, min_z - 1)
    lines.append(Part.makeLine(corner1,coords[0][0]))
    for row in range(num_rows - 1):
        a = coords[row][0]
        b = coords[row + 1][0]
        lines.append(Part.makeLine(a, b))
    corner2 = FreeCAD.Vector(coords[last_row][0].x, coords[last_row][0].y, min_z - 1)
    lines.append(Part.makeLine(corner2,coords[last_row][0]))
    lines.append(Part.makeLine(corner1,corner2))
    wire = Part.Wire(lines)
    face = Part.Face(wire)
    faces.append(face)

    # Create the face to close off the x-max border
    lines = []
    corner1 = FreeCAD.Vector(coords[0][last_col].x, coords[0][last_col].y, min_z - 1)
    lines.append(Part.makeLine(corner1,coords[0][last_col]))
    for row in range(num_rows - 1):
        a = coords[row][last_col]
        b = coords[row + 1][last_col]
        lines.append(Part.makeLine(a, b))
    corner2 = FreeCAD.Vector(coords[last_row][last_col].x, coords[last_row][last_col].y, min_z - 1)
    lines.append(Part.makeLine(corner2,coords[last_row][last_col]))
    lines.append(Part.makeLine(corner1,corner2))
    wire = Part.Wire(lines)
    face = Part.Face(wire)
    faces.append(face)

    # Create a bottom surface to close off the shell
    a = FreeCAD.Vector(coords[0][0].x, coords[0][0].y, min_z - 1)
    b = FreeCAD.Vector(coords[0][last_col].x, coords[0][last_col].y, min_z - 1)
    c = FreeCAD.Vector(coords[last_row][last_col].x, coords[last_row][last_col].y, min_z - 1)
    d = FreeCAD.Vector(coords[last_row][0].x, coords[last_row][0].y, min_z - 1)
    ab = Part.makeLine(a,b)
    bc = Part.makeLine(b,c)
    cd = Part.makeLine(c,d)
    da = Part.makeLine(d,a)
    wire = Part.Wire([ab,bc,cd,da])
    face = Part.Face(wire)
    faces.append(face)

    s = Part.Shell(faces)
    solid = Part.Solid(s)
    return solid,last_col,last_row
