#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2009, 2010                                              *
#*   Yorik van Havre <yorik@uncreated.net>, Ken Cline <cline@frii.com>     *
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

__title__="FreeCAD Draft Workbench - Geometry library"
__author__ = "Yorik van Havre, Jacques-Antoine Gaudin, Ken Cline"
__url__ = ["http://www.freecadweb.org"]

## \defgroup DRAFTGEOMUTILS DraftGeomUtils
#  \ingroup DRAFT
#  \brief Shape manipulation utilities for the Draft workbench
#
# Shapes manipulation utilities

## \addtogroup DRAFTGEOMUTILS
#  @{

"this file contains generic geometry functions for manipulating Part shapes"

import FreeCAD, Part, DraftVecUtils, math, cmath
from FreeCAD import Vector

NORM = Vector(0,0,1) # provisory normal direction for all geometry ops.

params = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft")

# Generic functions *********************************************************

def precision():
    "precision(): returns the Draft precision setting"
    return params.GetInt("precision",6)

def vec(edge):
    "vec(edge) or vec(line): returns a vector from an edge or a Part.LineSegment"
    # if edge is not straight, you'll get strange results!
    if isinstance(edge,Part.Shape):
        return edge.Vertexes[-1].Point.sub(edge.Vertexes[0].Point)
    elif isinstance(edge,Part.LineSegment):
        return edge.EndPoint.sub(edge.StartPoint)
    else:
        return None

def edg(p1,p2):
    "edg(Vector,Vector): returns an edge from 2 vectors"
    if isinstance(p1,FreeCAD.Vector) and isinstance(p2,FreeCAD.Vector):
        if DraftVecUtils.equals(p1,p2): return None
        else: return Part.LineSegment(p1,p2).toShape()

def getVerts(shape):
    "getVerts(shape): returns a list containing vectors of each vertex of the shape"
    if not hasattr(shape,"Vertexes"):
        return []
    p = []
    for v in shape.Vertexes:
            p.append(v.Point)
    return p

def v1(edge):
    "v1(edge): returns the first point of an edge"
    return edge.Vertexes[0].Point

def isNull(something):
    '''isNull(object): returns true if the given shape is null or the given placement is null or
    if the given vector is (0,0,0)'''
    if isinstance(something,Part.Shape):
            return something.isNull()
    elif isinstance(something,FreeCAD.Vector):
            if something == Vector(0,0,0):
                    return True
            else:
                    return False
    elif isinstance(something,FreeCAD.Placement):
            if (something.Base == Vector(0,0,0)) and (something.Rotation.Q == (0,0,0,1)):
                    return True
            else:
                    return False

def isPtOnEdge(pt,edge) :
    '''isPtOnEdge(Vector,edge): Tests if a point is on an edge'''
    v = Part.Vertex(pt)
    try:
        d = v.distToShape(edge)
    except:
        return False
    else:
        if d:
            if round(d[0],precision()) == 0:
                return True
    return False

def hasCurves(shape):
    "hasCurve(shape): checks if the given shape has curves"
    for e in shape.Edges:
            if not isinstance(e.Curve,(Part.LineSegment,Part.Line)):
                    return True
    return False

def isAligned(edge,axis="x"):
    "isAligned(edge,axis): checks if the given edge or line is aligned to the given axis (x, y or z)"
    if axis == "x":
        if isinstance(edge,Part.Edge):
            if len(edge.Vertexes) == 2:
                if edge.Vertexes[0].X == edge.Vertexes[-1].X:
                    return True
        elif isinstance(edge,Part.LineSegment):
            if edge.StartPoint.x == edge.EndPoint.x:
                    return True
    elif axis == "y":
        if isinstance(edge,Part.Edge):
            if len(edge.Vertexes) == 2:
                if edge.Vertexes[0].Y == edge.Vertexes[-1].Y:
                    return True
        elif isinstance(edge,Part.LineSegment):
            if edge.StartPoint.y == edge.EndPoint.y:
                    return True
    elif axis == "z":
        if isinstance(edge,Part.Edge):
            if len(edge.Vertexes) == 2:
                if edge.Vertexes[0].Z == edge.Vertexes[-1].Z:
                    return True
        elif isinstance(edge,Part.LineSegment):
            if edge.StartPoint.z == edge.EndPoint.z:
                    return True
    return False

def getQuad(face):
    """getQuad(face): returns a list of 3 vectors (basepoint, Xdir, Ydir) if the face
    is a quad, or None if not."""
    if len(face.Edges) != 4:
        return None
    v1 = vec(face.Edges[0])
    v2 = vec(face.Edges[1])
    v3 = vec(face.Edges[2])
    v4 = vec(face.Edges[3])
    angles90 = [round(math.pi*0.5,precision()),round(math.pi*1.5,precision())]
    angles180 = [0,round(math.pi,precision()),round(math.pi*2,precision())]
    for ov in [v2,v3,v4]:
        if not (round(v1.getAngle(ov),precision()) in angles90+angles180):
            return None
    for ov in [v2,v3,v4]:
        if round(v1.getAngle(ov),precision()) in angles90:
            v1.normalize()
            ov.normalize()
            return [face.Edges[0].Vertexes[0].Point,v1,ov]

def areColinear(e1,e2):
    """areColinear(e1,e2): returns True if both edges are colinear"""
    if not isinstance(e1.Curve,(Part.LineSegment,Part.Line)):
        return False
    if not isinstance(e2.Curve,(Part.LineSegment,Part.Line)):
        return False
    v1 = vec(e1)
    v2 = vec(e2)
    a = round(v1.getAngle(v2),precision())
    if (a == 0) or (a == round(math.pi,precision())):
        v3 = e2.Vertexes[0].Point.sub(e1.Vertexes[0].Point)
        if DraftVecUtils.isNull(v3):
            return True
        else:
            a2 = round(v1.getAngle(v3),precision())
            if (a2 == 0) or (a2 == round(math.pi,precision())):
                return True
    return False

def hasOnlyWires(shape):
    "hasOnlyWires(shape): returns True if all the edges are inside a wire"
    ne = 0
    for w in shape.Wires:
        ne += len(w.Edges)
    if ne == len(shape.Edges):
        return True
    return False

def geomType(edge):
    "returns the type of geom this edge is based on"
    try:
        if isinstance(edge.Curve,(Part.LineSegment,Part.Line)):
            return "Line"
        elif isinstance(edge.Curve,Part.Circle):
            return "Circle"
        elif isinstance(edge.Curve,Part.BSplineCurve):
            return "BSplineCurve"
        elif isinstance(edge.Curve,Part.BezierCurve):
            return "BezierCurve"
        elif isinstance(edge.Curve,Part.Ellipse):
            return "Ellipse"
        else:
            return "Unknown"
    except:
        return "Unknown"

def isValidPath(shape):
    "isValidPath(shape): returns True if the shape can be used as an extrusion path"
    if shape.isNull():
        return False
    if shape.Faces:
        return False
    if len(shape.Wires) > 1:
        return False
    if shape.Wires:
        if shape.Wires[0].isClosed():
            return False
    if shape.isClosed():
        return False
    return True

# edge functions *****************************************************************

def findEdge(anEdge,aList):
    '''findEdge(anEdge,aList): returns True if anEdge is found in aList of edges'''
    for e in range(len(aList)):
        if str(anEdge.Curve) == str(aList[e].Curve):
            if DraftVecUtils.equals(anEdge.Vertexes[0].Point,aList[e].Vertexes[0].Point):
                if DraftVecUtils.equals(anEdge.Vertexes[-1].Point,aList[e].Vertexes[-1].Point):
                    return(e)
    return None


def findIntersection(edge1,edge2,infinite1=False,infinite2=False,ex1=False,ex2=False,dts=True,findAll=False) :
    '''findIntersection(edge1,edge2,infinite1=False,infinite2=False,dts=True):
    returns a list containing the intersection point(s) of 2 edges.
    You can also feed 4 points instead of edge1 and edge2. If dts is used,
    Shape.distToShape() is used, which can be buggy'''

    def getLineIntersections(pt1,pt2,pt3,pt4,infinite1,infinite2):
        if pt1:
            # first check if we don't already have coincident endpoints
            if (pt1 in [pt3,pt4]):
                    return [pt1]
            elif (pt2 in [pt3,pt4]):
                    return [pt2]
        norm1 = pt2.sub(pt1).cross(pt3.sub(pt1))
        norm2 = pt2.sub(pt4).cross(pt3.sub(pt4))
        if not DraftVecUtils.isNull(norm1):
            try:
                norm1.normalize()
            except:
                return []
        if not DraftVecUtils.isNull(norm2):
            try:
                norm2.normalize()
            except:
                return []
        if DraftVecUtils.isNull(norm1.cross(norm2)):
            vec1 = pt2.sub(pt1)
            vec2 = pt4.sub(pt3)
            if DraftVecUtils.isNull(vec1) or DraftVecUtils.isNull(vec2):
                return [] # One of the line has zero-length
            try:
                vec1.normalize()
                vec2.normalize()
            except:
                return []
            norm3 = vec1.cross(vec2)
            if not DraftVecUtils.isNull(norm3) :
                k = ((pt3.z-pt1.z)*(vec2.x-vec2.y)+(pt3.y-pt1.y)*(vec2.z-vec2.x)+ \
                     (pt3.x-pt1.x)*(vec2.y-vec2.z))/(norm3.x+norm3.y+norm3.z)
                vec1.scale(k,k,k)
                intp = pt1.add(vec1)

                if  infinite1 == False and not isPtOnEdge(intp,edge1) :
                    return []

                if  infinite2 == False and not isPtOnEdge(intp,edge2) :
                    return []

                return [intp]
            else :
                return [] # Lines have same direction
        else :
            return [] # Lines aren't on same plane

    # First, check bound boxes
    if isinstance(edge1,Part.Edge) and isinstance(edge2,Part.Edge) \
        and (not infinite1) and (not infinite2):
        if not edge1.BoundBox.intersect(edge2.BoundBox):
            return [] # bound boxes don't intersect

    # First, try to use distToShape if possible
    if dts and isinstance(edge1,Part.Edge) and isinstance(edge2,Part.Edge) \
            and (not infinite1) and (not infinite2):
        dist, pts, geom = edge1.distToShape(edge2)
        sol = []
        if round(dist,precision()) == 0:
            for p in pts:
                if not p in sol:
                    sol.append(p[0])
        return sol

    pt1 = None

    if isinstance(edge1,FreeCAD.Vector) and isinstance(edge2,FreeCAD.Vector):
        # we got points directly
        pt1 = edge1
        pt2 = edge2
        pt3 = infinite1
        pt4 = infinite2
        infinite1 = ex1
        infinite2 = ex2
        return getLineIntersections(pt1,pt2,pt3,pt4,infinite1,infinite2)

    elif (geomType(edge1) == "Line") and (geomType(edge2) == "Line") :
        # we have 2 straight lines
        pt1, pt2, pt3, pt4 = [edge1.Vertexes[0].Point,
                                      edge1.Vertexes[1].Point,
                                      edge2.Vertexes[0].Point,
                                      edge2.Vertexes[1].Point]
        return getLineIntersections(pt1,pt2,pt3,pt4,infinite1,infinite2)

    elif (geomType(edge1) == "Circle") and (geomType(edge2) == "Line") \
      or (geomType(edge1) == "Line") and (geomType(edge2) == "Circle") :

        # deals with an arc or circle and a line

        edges = [edge1,edge2]
        for edge in edges :
            if geomType(edge) == "Line":
                line = edge
            else :
                arc  = edge

        dirVec = vec(line) ; dirVec.normalize()
        pt1    = line.Vertexes[0].Point
        pt2    = line.Vertexes[1].Point
        pt3    = arc.Vertexes[0].Point
        pt4    = arc.Vertexes[-1].Point
        center = arc.Curve.Center

        int = []
        # first check for coincident endpoints
        if (pt1 in [pt3,pt4]):
            if findAll:
                int.append(pt1)
            else:
                return [pt1]
        elif (pt2 in [pt3,pt4]):
            if findAll:
                int.append(pt2)
            else:
                return [pt2]

        if DraftVecUtils.isNull(pt1.sub(center).cross(pt2.sub(center)).cross(arc.Curve.Axis)) :
            # Line and Arc are on same plane

            dOnLine = center.sub(pt1).dot(dirVec)
            onLine  = Vector(dirVec)
            onLine.scale(dOnLine,dOnLine,dOnLine)
            toLine  = pt1.sub(center).add(onLine)

            if toLine.Length < arc.Curve.Radius :
                dOnLine = (arc.Curve.Radius**2 - toLine.Length**2)**(0.5)
                onLine  = Vector(dirVec)
                onLine.scale(dOnLine,dOnLine,dOnLine)
                int += [center.add(toLine).add(onLine)]
                onLine  = Vector(dirVec)
                onLine.scale(-dOnLine,-dOnLine,-dOnLine)
                int += [center.add(toLine).add(onLine)]
            elif round(toLine.Length-arc.Curve.Radius,precision()) == 0 :
                int = [center.add(toLine)]
            else :
                return []

        else :
            # Line isn't on Arc's plane
            if dirVec.dot(arc.Curve.Axis) != 0 :
                toPlane  = Vector(arc.Curve.Axis) ; toPlane.normalize()
                d = pt1.dot(toPlane)
                if not d:
                    return []
                dToPlane = center.sub(pt1).dot(toPlane)
                toPlane = Vector(pt1)
                toPlane.scale(dToPlane/d,dToPlane/d,dToPlane/d)
                ptOnPlane = toPlane.add(pt1)
                if round(ptOnPlane.sub(center).Length - arc.Curve.Radius,precision()) == 0 :
                    int = [ptOnPlane]
                else :
                    return []
            else :
                return []

        if infinite1 == False :
            for i in range(len(int)-1,-1,-1) :
                if not isPtOnEdge(int[i],edge1) :
                    del int[i]
        if infinite2 == False :
            for i in range(len(int)-1,-1,-1) :
                if not isPtOnEdge(int[i],edge2) :
                    del int[i]
        return int

    elif (geomType(edge1) == "Circle") and (geomType(edge2) == "Circle") :

        # deals with 2 arcs or circles

        cent1, cent2 = edge1.Curve.Center, edge2.Curve.Center
        rad1 , rad2  = edge1.Curve.Radius, edge2.Curve.Radius
        axis1, axis2 = edge1.Curve.Axis  , edge2.Curve.Axis
        c2c          = cent2.sub(cent1)

        if cent1.sub(cent2).Length == 0:
            # circles are concentric
            return []

        if DraftVecUtils.isNull(axis1.cross(axis2)) :
            if round(c2c.dot(axis1),precision()) == 0 :
                # circles are on same plane
                dc2c = c2c.Length ;
                if not DraftVecUtils.isNull(c2c): c2c.normalize()
                if round(rad1+rad2-dc2c,precision()) < 0 \
                or round(rad1-dc2c-rad2,precision()) > 0 or round(rad2-dc2c-rad1,precision()) > 0 :
                    return []
                else :
                    norm = c2c.cross(axis1)
                    if not DraftVecUtils.isNull(norm): norm.normalize()
                    if DraftVecUtils.isNull(norm): x = 0
                    else: x = (dc2c**2 + rad1**2 - rad2**2)/(2*dc2c)
                    y = abs(rad1**2 - x**2)**(0.5)
                    c2c.scale(x,x,x)
                    if round(y,precision()) != 0 :
                        norm.scale(y,y,y)
                        int =  [cent1.add(c2c).add(norm)]
                        int += [cent1.add(c2c).sub(norm)]
                    else :
                        int = [cent1.add(c2c)]
            else :
                return [] # circles are on parallel planes
        else :
            # circles aren't on same plane
            axis1.normalize() ; axis2.normalize()
            U = axis1.cross(axis2)
            V = axis1.cross(U)
            dToPlane = c2c.dot(axis2)
            d        = V.add(cent1).dot(axis2)
            V.scale(dToPlane/d,dToPlane/d,dToPlane/d)
            PtOn2Planes = V.add(cent1)
            planeIntersectionVector = U.add(PtOn2Planes)
            intTemp = findIntersection(planeIntersectionVector,edge1,True,True)
            int = []
            for pt in intTemp :
                if round(pt.sub(cent2).Length-rad2,precision()) == 0 :
                    int += [pt]

        if infinite1 == False :
            for i in range(len(int)-1,-1,-1) :
                if not isPtOnEdge(int[i],edge1) :
                    del int[i]
        if infinite2 == False :
            for i in range(len(int)-1,-1,-1) :
                if not isPtOnEdge(int[i],edge2) :
                    del int[i]

        return int
    else:
        print("DraftGeomUtils: Unsupported curve type: (" + str(edge1.Curve) + ", " + str(edge2.Curve) + ")")
        return []

def wiresIntersect(wire1,wire2):
    "wiresIntersect(wire1,wire2): returns True if some of the edges of the wires are intersecting otherwise False"
    for e1 in wire1.Edges:
        for e2 in wire2.Edges:
            if findIntersection(e1,e2,dts=False):
                return True
    return False

def pocket2d(shape,offset):
    """pocket2d(shape,offset): return a list of wires obtained from offsetting the wires from the given shape
    by the given offset, and intersection if needed."""
    # find the outer wire
    l = 0
    outerWire = None
    innerWires = []
    for w in shape.Wires:
        if w.BoundBox.DiagonalLength > l:
            outerWire = w
            l = w.BoundBox.DiagonalLength
    if not outerWire:
        return []
    for w in shape.Wires:
        if w.hashCode() != outerWire.hashCode():
            innerWires.append(w)
    o = outerWire.makeOffset(-offset)
    if not o.Wires:
        return []
    offsetWires = o.Wires
    #print("base offset wires:",offsetWires)
    if not innerWires:
        return offsetWires
    for innerWire in innerWires:
        i = innerWire.makeOffset(offset)
        if len(innerWire.Edges) == 1:
            e = innerWire.Edges[0]
            if isinstance(e.Curve,Part.Circle):
                e = Part.makeCircle(e.Curve.Radius+offset,e.Curve.Center,e.Curve.Axis)
                i = Part.Wire(e)
        if i.Wires:
            #print("offsetting island ",innerWire," : ",i.Wires)
            for w in i.Wires:
                added = False
                #print("checking wire ",w)
                k = list(range(len(offsetWires)))
                for j in k:
                    #print("checking against existing wire ",j)
                    ow = offsetWires[j]
                    if ow:
                        if wiresIntersect(w,ow):
                            #print("intersect")
                            f1 = Part.Face(ow)
                            f2 = Part.Face(w)
                            f3  = f1.cut(f2)
                            #print("made new wires: ",f3.Wires)
                            offsetWires[j] = f3.Wires[0]
                            if len(f3.Wires) > 1:
                                #print("adding more")
                                offsetWires.extend(f3.Wires[1:])
                            added = True
                        else:
                            a = w.BoundBox
                            b = ow.BoundBox
                            if (a.XMin <= b.XMin) and (a.YMin <= b.YMin) and (a.ZMin <= b.ZMin) and (a.XMax >= b.XMax) and (a.YMax >= b.YMax) and (a.ZMax >= b.ZMax):
                                #print("this wire is bigger than the outer wire")
                                offsetWires[j] = None
                                added = True
                            #else:
                                #print("doesn't intersect")
                if not added:
                    #print("doesn't intersect with any other")
                    offsetWires.append(w)
    offsetWires = [o for o in offsetWires if o != None]
    return offsetWires

def orientEdge(edge, normal=None, make_arc=False):
    """Re-orients 'edge' such that it is in the x-y plane. If 'normal' is passed, this
    is used as the basis for the rotation, otherwise the Placement property of 'edge'
    is used"""
    import DraftVecUtils
    # This 'normalizes' the placement to the xy plane
    edge = edge.copy()
    xyDir = FreeCAD.Vector(0, 0, 1)
    base = FreeCAD.Vector(0,0,0)

    if normal:
        angle = DraftVecUtils.angle(normal, xyDir)*FreeCAD.Units.Radian
        axis  = normal.cross(xyDir)
    else:
        axis = edge.Placement.Rotation.Axis
        angle = -1*edge.Placement.Rotation.Angle*FreeCAD.Units.Radian
    if axis == Vector (0.0, 0.0, 0.0):
        axis = Vector (0.0, 0.0, 1.0)
    if angle:
        edge.rotate(base, axis, angle)
    if isinstance(edge.Curve,Part.Line):
        return Part.LineSegment(edge.Curve,edge.FirstParameter,edge.LastParameter)
    elif make_arc and isinstance(edge.Curve,Part.Circle) and not edge.Closed:
        return Part.ArcOfCircle(edge.Curve, edge.FirstParameter,
                                    edge.LastParameter,edge.Curve.Axis.z>0)
    elif make_arc and isinstance(edge.Curve,Part.Ellipse) and not edge.Closed:
        return Part.ArcOfEllipse(edge.Curve, edge.FirstParameter,
                                    edge.LastParameter,edge.Curve.Axis.z>0)
    return edge.Curve

def mirror (point, edge):
    "finds mirror point relative to an edge"
    normPoint = point.add(findDistance(point, edge, False))
    if normPoint:
        normPoint_point = Vector.sub(point, normPoint)
        normPoint_refl = normPoint_point.negative()
        refl = Vector.add(normPoint, normPoint_refl)
        return refl
    else:
        return None

def isClockwise(edge,ref=None):
    """Returns True if a circle-based edge has a clockwise direction"""
    if not geomType(edge) == "Circle":
        return True
    v1 = edge.Curve.tangent(edge.ParameterRange[0])[0]
    if DraftVecUtils.isNull(v1):
        return True
    # we take an arbitrary other point on the edge that has little chances to be aligned with the first one...
    v2 = edge.Curve.tangent(edge.ParameterRange[0]+0.01)[0]
    n = edge.Curve.Axis
    # if that axis points "the wrong way" from the reference, we invert it
    if not ref:
        ref = Vector(0,0,1)
    if n.getAngle(ref) > math.pi/2:
        n = n.negative()
    if DraftVecUtils.angle(v1,v2,n) < 0:
        return False
    if n.z < 0:
        return False
    return True

def isSameLine(e1,e2):
    """isSameLine(e1,e2): return True if the 2 edges are lines and have the same
    points"""
    if not isinstance(e1.Curve,Part.LineSegment):
        return False
    if not isinstance(e2.Curve,Part.LineSegment):
        return False
    if (DraftVecUtils.equals(e1.Vertexes[0].Point,e2.Vertexes[0].Point)) and \
       (DraftVecUtils.equals(e1.Vertexes[-1].Point,e2.Vertexes[-1].Point)):
           return True
    elif (DraftVecUtils.equals(e1.Vertexes[-1].Point,e2.Vertexes[0].Point)) and \
       (DraftVecUtils.equals(e1.Vertexes[0].Point,e2.Vertexes[-1].Point)):
           return True
    return False

def isWideAngle(edge):
    """returns True if the given edge is an arc with angle > 180 degrees"""
    if geomType(edge) != "Circle":
        return False
    r = edge.Curve.Radius
    total = 2*r*math.pi
    if edge.Length > total/2:
        return True
    return False

def findClosest(basepoint,pointslist):
    '''
    findClosest(vector,list)
    in a list of 3d points, finds the closest point to the base point.
    an index from the list is returned.
    '''
    npoint = None
    if not pointslist:
        return None
    smallest = 1000000
    for n in range(len(pointslist)):
        new = basepoint.sub(pointslist[n]).Length
        if new < smallest:
            smallest = new
            npoint = n
    return npoint

def concatenate(shape):
    "concatenate(shape) -- turns several faces into one"
    edges = getBoundary(shape)
    edges = Part.__sortEdges__(edges)
    try:
        wire=Part.Wire(edges)
        face=Part.Face(wire)
    except:
        print("DraftGeomUtils: Couldn't join faces into one")
        return(shape)
    else:
        if not wire.isClosed(): return(wire)
        else: return(face)

def getBoundary(shape):
    "getBoundary(shape) -- this function returns the boundary edges of a group of faces"
    # make a lookup-table where we get the number of occurrences
    # to each edge in the fused face
    if isinstance(shape,list):
            shape = Part.makeCompound(shape)
    lut={}
    for f in shape.Faces:
        for e in f.Edges:
            hc= e.hashCode()
            if hc in lut: lut[hc]=lut[hc]+1
            else: lut[hc]=1
    # filter out the edges shared by more than one sub-face
    bound=[]
    for e in shape.Edges:
        if lut[e.hashCode()] == 1: bound.append(e)
    return bound

def isLine(bsp):
    "returns True if the given BSpline curve is a straight line"
    step = bsp.LastParameter/10
    b = bsp.tangent(0)
    for i in range(10):
        if bsp.tangent(i*step) != b:
            return False
    return True


def sortEdges(edges):
    "Deprecated. Use Part.__sortEdges__ instead"

    raise DeprecationWarning("Deprecated. Use Part.__sortEdges__ instead")

    # Build a dictionary of edges according to their end points.
    # Each entry is a set of edges that starts, or ends, at the
    # given vertex hash.
    if len(edges) < 2:
        return edges
    sdict = dict()
    edict = dict()
    nedges = []
    for e in edges:
        if hasattr(e,"Length"):
            if e.Length != 0:
                sdict.setdefault( e.Vertexes[0].hashCode(), [] ).append(e)
                edict.setdefault( e.Vertexes[-1].hashCode(),[] ).append(e)
                nedges.append(e)
    if not nedges:
        print("DraftGeomUtils.sortEdges: zero-length edges")
        return edges
    # Find the start of the path.  The start is the vertex that appears
    # in the sdict dictionary but not in the edict dictionary, and has
    # only one edge ending there.
    startedge = None
    for v, se in sdict.items():
        if v not in edict and len(se) == 1:
            startedge = se
            break
    # The above may not find a start vertex; if the start edge is reversed,
    # the start vertex will appear in edict (and not sdict).
    if not startedge:
        for v, se in edict.items():
            if v not in sdict and len(se) == 1:
                startedge = se
                break
    # If we still have no start vertex, it was a closed path.  If so, start
    # with the first edge in the supplied list
    if not startedge:
        startedge = nedges[0]
        v = startedge.Vertexes[0].hashCode()
    # Now build the return list by walking the edges starting at the start
    # vertex we found.  We're done when we've visited each edge, so the
    # end check is simply the count of input elements (that works for closed
    # as well as open paths).
    ret = list()
    # store the hash code of the last edge, to avoid picking the same edge back
    eh = None
    for i in range(len(nedges)):
        try:
            eset = sdict[v]
            e = eset.pop()
            if not eset:
                del sdict[v]
            if e.hashCode() == eh:
                raise KeyError
            v = e.Vertexes[-1].hashCode()
            eh = e.hashCode()
        except KeyError:
            try:
                eset = edict[v]
                e = eset.pop()
                if not eset:
                    del edict[v]
                if e.hashCode() == eh:
                    raise KeyError
                v = e.Vertexes[0].hashCode()
                eh = e.hashCode()
                e = invert(e)
            except KeyError:
                print("DraftGeomUtils.sortEdges failed - running old version")
                return sortEdgesOld(edges)
        ret.append(e)
    # All done.
    return ret


def sortEdgesOld(lEdges, aVertex=None):
    "Deprecated. Use Part.__sortEdges__ instead"

    raise DeprecationWarning("Deprecated. Use Part.__sortEdges__ instead")

    #There is no reason to limit this to lines only because every non-closed edge always
    #has exactly two vertices (wmayer)
    #for e in lEdges:
    #        if not isinstance(e.Curve,Part.LineSegment):
    #                print("Warning: sortedges cannot treat wired containing curves yet.")
    #                return lEdges

    def lookfor(aVertex, inEdges):
        ''' Look for (aVertex, inEdges) returns count, the position of the instance
        the position in the instance and the instance of the Edge'''
        count = 0
        linstances = [] #lists the instances of aVertex
        for i in range(len(inEdges)) :
            for j in range(2) :
                if aVertex.Point == inEdges[i].Vertexes[j-1].Point:
                        instance = inEdges[i]
                        count += 1
                        linstances += [i,j-1,instance]
        return [count]+linstances

    if (len(lEdges) < 2):
        if aVertex == None:
            return lEdges
        else:
            result = lookfor(aVertex,lEdges)
            if result[0] != 0:
                if aVertex.Point == result[3].Vertexes[0].Point:
                    return lEdges
                else:
                    if geomType(result[3]) == "Line":
                        return [Part.LineSegment(aVertex.Point,result[3].Vertexes[0].Point).toShape()]
                    elif geomType(result[3]) == "Circle":
                        mp = findMidpoint(result[3])
                        return [Part.Arc(aVertex.Point,mp,result[3].Vertexes[0].Point).toShape()]
                    elif geomType(result[3]) == "BSplineCurve" or\
                        geomType(result[3]) == "BezierCurve":
                        if isLine(result[3].Curve):
                            return [Part.LineSegment(aVertex.Point,result[3].Vertexes[0].Point).toShape()]
                        else:
                            return lEdges
                    else:
                        return lEdges

    olEdges = [] # ol stands for ordered list
    if aVertex == None:
        for i in range(len(lEdges)*2) :
            if len(lEdges[i/2].Vertexes) > 1:
                result = lookfor(lEdges[i/2].Vertexes[i%2],lEdges)
                if result[0] == 1 :  # Have we found an end ?
                    olEdges = sortEdgesOld(lEdges, result[3].Vertexes[result[2]])
                    return olEdges
        # if the wire is closed there is no end so choose 1st Vertex
        # print("closed wire, starting from ",lEdges[0].Vertexes[0].Point)
        return sortEdgesOld(lEdges, lEdges[0].Vertexes[0])
    else :
        #print("looking ",aVertex.Point)
        result = lookfor(aVertex,lEdges)
        if result[0] != 0 :
            del lEdges[result[1]]
            next = sortEdgesOld(lEdges, result[3].Vertexes[-((-result[2])^1)])
            #print("result ",result[3].Vertexes[0].Point,"    ",result[3].Vertexes[1].Point, " compared to ",aVertex.Point)
            if aVertex.Point == result[3].Vertexes[0].Point:
                #print("keeping")
                olEdges += [result[3]] + next
            else:
                #print("inverting", result[3].Curve)
                if geomType(result[3]) == "Line":
                    newedge = Part.LineSegment(aVertex.Point,result[3].Vertexes[0].Point).toShape()
                    olEdges += [newedge] + next
                elif geomType(result[3]) == "Circle":
                    mp = findMidpoint(result[3])
                    newedge = Part.Arc(aVertex.Point,mp,result[3].Vertexes[0].Point).toShape()
                    olEdges += [newedge] + next
                elif geomType(result[3]) == "BSplineCurve" or \
                    geomType(result[3]) == "BezierCurve":
                    if isLine(result[3].Curve):
                        newedge = Part.LineSegment(aVertex.Point,result[3].Vertexes[0].Point).toShape()
                        olEdges += [newedge] + next
                    else:
                        olEdges += [result[3]] + next
                else:
                    olEdges += [result[3]] + next
            return olEdges
        else :
            return []


def invert(edge):
    '''invert(edge): returns an inverted copy of this edge'''
    if len(edge.Vertexes) == 1:
        return edge
    if geomType(edge) == "Line":
        return Part.LineSegment(edge.Vertexes[-1].Point,edge.Vertexes[0].Point).toShape()
    elif geomType(edge) == "Circle":
        mp = findMidpoint(edge)
        return Part.Arc(edge.Vertexes[-1].Point,mp,edge.Vertexes[0].Point).toShape()
    elif geomType(edge) in ["BSplineCurve","BezierCurve"]:
        if isLine(edge.Curve):
            return Part.LineSegment(edge.Vertexes[-1].Point,edge.Vertexes[0].Point).toShape()
    print("DraftGeomUtils.invert: unable to invert ",edge.Curve)
    return edge


def flattenWire(wire):
    '''flattenWire(wire): forces a wire to get completely flat
    along its normal.'''
    import WorkingPlane
    n = getNormal(wire)
    if not n:
        return
    o = wire.Vertexes[0].Point
    plane = WorkingPlane.plane()
    plane.alignToPointAndAxis(o,n,0)
    verts = [o]
    for v in wire.Vertexes[1:]:
        verts.append(plane.projectPoint(v.Point))
    if wire.isClosed():
        verts.append(o)
    w = Part.makePolygon(verts)
    return w

def findWires(edgeslist):
    return [ Part.Wire(e) for e in Part.sortEdges(edgeslist)]

def findWiresOld2(edgeslist):
    '''finds connected wires in the given list of edges'''

    def touches(e1,e2):
        if len(e1.Vertexes) < 2:
            return False
        if len(e2.Vertexes) < 2:
            return False
        if DraftVecUtils.equals(e1.Vertexes[0].Point,e2.Vertexes[0].Point):
            return True
        if DraftVecUtils.equals(e1.Vertexes[0].Point,e2.Vertexes[-1].Point):
            return True
        if DraftVecUtils.equals(e1.Vertexes[-1].Point,e2.Vertexes[0].Point):
            return True
        if DraftVecUtils.equals(e1.Vertexes[-1].Point,e2.Vertexes[-1].Point):
            return True
        return False

    edges = edgeslist[:]
    wires = []
    lost = []
    while edges:
        e = edges[0]
        if not wires:
            # create first group
            edges.remove(e)
            wires.append([e])
        else:
            found = False
            for w in wires:
                if not found:
                    for we in w:
                        if touches(e,we):
                            edges.remove(e)
                            w.append(e)
                            found = True
                            break
            if not found:
                if e in lost:
                    # we already tried this edge, and still nothing
                    edges.remove(e)
                    wires.append([e])
                    lost = []
                else:
                    # put to the end of the list
                    edges.remove(e)
                    edges.append(e)
                    lost.append(e)
    nwires = []
    for w in wires:
        try:
            wi = Part.Wire(w)
        except:
            print("couldn't join some edges")
        else:
            nwires.append(wi)
    return nwires

def superWire(edgeslist,closed=False):
        '''superWire(edges,[closed]): forces a wire between edges that don't necessarily
        have coincident endpoints. If closed=True, wire will always be closed'''
        def median(v1,v2):
                vd = v2.sub(v1)
                vd.scale(.5,.5,.5)
                return v1.add(vd)
        edges = Part.__sortEdges__(edgeslist)
        print(edges)
        newedges = []
        for i in range(len(edges)):
                curr = edges[i]
                if i == 0:
                        if closed:
                                prev = edges[-1]
                        else:
                                prev = None
                else:
                        prev = edges[i-1]
                if i == (len(edges)-1):
                        if closed:
                                next = edges[0]
                        else:
                                next = None
                else:
                        next = edges[i+1]
                print(i,prev,curr,next)
                if prev:
                        if curr.Vertexes[0].Point == prev.Vertexes[-1].Point:
                                p1 = curr.Vertexes[0].Point
                        else:
                                p1 = median(curr.Vertexes[0].Point,prev.Vertexes[-1].Point)
                else:
                        p1 = curr.Vertexes[0].Point
                if next:
                        if curr.Vertexes[-1].Point == next.Vertexes[0].Point:
                                p2 = next.Vertexes[0].Point
                        else:
                                p2 = median(curr.Vertexes[-1].Point,next.Vertexes[0].Point)
                else:
                        p2 = curr.Vertexes[-1].Point
                if geomType(curr) == "Line":
                        print("line",p1,p2)
                        newedges.append(Part.LineSegment(p1,p2).toShape())
                elif geomType(curr) == "Circle":
                        p3 = findMidpoint(curr)
                        print("arc",p1,p3,p2)
                        newedges.append(Part.Arc(p1,p3,p2).toShape())
                else:
                        print("Cannot superWire edges that are not lines or arcs")
                        return None
        print(newedges)
        return Part.Wire(newedges)

def findMidpoint(edge):
    "calculates the midpoint of an edge"
    first = edge.Vertexes[0].Point
    last = edge.Vertexes[-1].Point
    if geomType(edge) == "Circle":
        center = edge.Curve.Center
        radius = edge.Curve.Radius
        if len(edge.Vertexes) == 1:
                # Circle
                dv = first.sub(center)
                dv = dv.negative()
                return center.add(dv)
        axis = edge.Curve.Axis
        chord = last.sub(first)
        perp = chord.cross(axis)
        perp.normalize()
        ray = first.sub(center)
        apothem = ray.dot(perp)
        sagitta = radius - apothem
        startpoint = Vector.add(first, chord.multiply(0.5))
        endpoint = DraftVecUtils.scaleTo(perp,sagitta)
        return Vector.add(startpoint,endpoint)

    elif geomType(edge) == "Line":
        halfedge = (last.sub(first)).multiply(.5)
        return Vector.add(first,halfedge)

    else:
        return None


def findPerpendicular(point,edgeslist,force=None):
    '''
    findPerpendicular(vector,wire,[force]):
    finds the shortest perpendicular distance between a point and an edgeslist.
    If force is specified, only the edge[force] will be considered, and it will be
    considered infinite.
    The function will return a list [vector_from_point_to_closest_edge,edge_index]
    or None if no perpendicular vector could be found.
    '''
    if not isinstance(edgeslist,list):
        try:
            edgeslist = edgeslist.Edges
        except:
            return None
    if (force == None):
        valid = None
        for edge in edgeslist:
            dist = findDistance(point,edge,strict=True)
            if dist:
                if not valid: valid = [dist,edgeslist.index(edge)]
                else:
                    if (dist.Length < valid[0].Length):
                        valid = [dist,edgeslist.index(edge)]
        return valid
    else:
        edge = edgeslist[force]
        dist = findDistance(point,edge)
        if dist: return [dist,force]
        else: return None
        return None

def offset(edge,vector,trim=False):
    '''
    offset(edge,vector)
    returns a copy of the edge at a certain (vector) distance
    if the edge is an arc, the vector will be added at its first point
    and a complete circle will be returned
    '''
    if (not isinstance(edge,Part.Shape)) or (not isinstance(vector,FreeCAD.Vector)):
        return None
    if geomType(edge) == "Line":
        v1 = Vector.add(edge.Vertexes[0].Point, vector)
        v2 = Vector.add(edge.Vertexes[-1].Point, vector)
        return Part.LineSegment(v1,v2).toShape()
    elif geomType(edge) == "Circle":
        rad = edge.Vertexes[0].Point.sub(edge.Curve.Center)
        curve = Part.Circle(edge.Curve)
        curve.Radius = Vector.add(rad,vector).Length
        if trim:
            return Part.ArcOfCircle(curve,edge.FirstParameter,edge.LastParameter).toShape()
        else:
            return curve.toShape()
    else:
        return None

def isReallyClosed(wire):
    "checks if a wire is really closed"
    if len(wire.Edges) == len(wire.Vertexes): return True
    v1 = wire.Vertexes[0].Point
    v2 = wire.Vertexes[-1].Point
    if DraftVecUtils.equals(v1,v2): return True
    return False

def getNormal(shape):
        "finds the normal of a shape, if possible"
        n = Vector(0,0,1)
        if shape.isNull():
            return n
        if (shape.ShapeType == "Face") and hasattr(shape,"normalAt"):
                n = shape.copy().normalAt(0.5,0.5)
        elif shape.ShapeType == "Edge":
                if geomType(shape.Edges[0]) in ["Circle","Ellipse"]:
                        n = shape.Edges[0].Curve.Axis
        else:
                for e in shape.Edges:
                        if geomType(e) in ["Circle","Ellipse"]:
                                n = e.Curve.Axis
                                break
                        e1 = vec(shape.Edges[0])
                        for i in range(1,len(shape.Edges)):
                                e2 = vec(shape.Edges[i])
                                if 0.1 < abs(e1.getAngle(e2)) < 3.14:
                                        n = e1.cross(e2).normalize()
                                        break
        if FreeCAD.GuiUp:
            import Draft
            vdir = Draft.get3DView().getViewDirection()
            if n.getAngle(vdir) < 0.78:
                n = n.negative()
        if not n.Length:
            return None
        return n

def getRotation(v1,v2=FreeCAD.Vector(0,0,1)):
    '''Get the rotation Quaternion between 2 vectors'''
    if (v1.dot(v2) > 0.999999) or (v1.dot(v2) < -0.999999):
        # vectors are opposite
        return None
    axis = v1.cross(v2)
    axis.normalize()
    #angle = math.degrees(math.sqrt((v1.Length ^ 2) * (v2.Length ^ 2)) + v1.dot(v2))
    angle = math.degrees(DraftVecUtils.angle(v1,v2,axis))
    return FreeCAD.Rotation(axis,angle)

def calculatePlacement(shape):
    '''calculatePlacement(shape): if the given shape is planar, this function
    returns a placement located at the center of gravity of the shape, and oriented
    towards the shape's normal. Otherwise, it returns a null placement.'''
    if not isPlanar(shape):
        return FreeCAD.Placement()
    pos = shape.BoundBox.Center
    norm = getNormal(shape)
    pla = FreeCAD.Placement()
    pla.Base = pos
    r =  getRotation(norm)
    if r:
        pla.Rotation = r
    return pla

def offsetWire(wire,dvec,bind=False,occ=False):
    '''
    offsetWire(wire,vector,[bind]): offsets the given wire along the
    given vector. The vector will be applied at the first vertex of
    the wire. If bind is True (and the shape is open), the original
    wire and the offsetted one are bound by 2 edges, forming a face.
    '''
    edges = Part.__sortEdges__(wire.Edges)
    norm = getNormal(wire)
    closed = isReallyClosed(wire)
    nedges = []
    if occ:
        l=abs(dvec.Length)
        if not l: return None
        if wire.Wires:
            wire = wire.Wires[0]
        else:
            wire = Part.Wire(edges)
        try:
            off = wire.makeOffset(l)
        except:
            return None
        else:
            return off

    # vec of first edge depends on its geometry
    e = edges[0]
    if isinstance(e.Curve,Part.Circle):
        firstVec = e.tangentAt(e.FirstParameter)
    else:
        firstVec = vec(e)

    for i in range(len(edges)):
        curredge = edges[i]
        delta = dvec
        if i != 0:
            if isinstance(curredge.Curve,Part.Circle):
                v = curredge.tangentAt(curredge.FirstParameter)
            else:
                v = vec(curredge)
            angle = DraftVecUtils.angle(firstVec,v,norm)			# use vec deduced depending on geometry instead of - angle = DraftVecUtils.angle(vec(edges[0]),v,norm)
            delta = DraftVecUtils.rotate(delta,angle,norm)
        #print("edge ",i,": ",curredge.Curve," ",curredge.Orientation," parameters:",curredge.ParameterRange," vector:",delta)
        nedge = offset(curredge,delta,trim=True)
        if not nedge:
            return None
        nedges.append(nedge)
    nedges = connect(nedges,closed)
    if bind and not closed:
        e1 = Part.LineSegment(edges[0].Vertexes[0].Point,nedges[0].Vertexes[0].Point).toShape()
        e2 = Part.LineSegment(edges[-1].Vertexes[-1].Point,nedges[-1].Vertexes[-1].Point).toShape()
        alledges = edges.extend(nedges)
        alledges = alledges.extend([e1,e2])
        w = Part.Wire(alledges)
        return w
    else:
        return nedges

def connect(edges,closed=False):
        '''connects the edges in the given list by their intersections'''
        nedges = []
        v2 = None

        for i in range(len(edges)):
            curr = edges[i]
            #print("debug: DraftGeomUtils.connect edge ",i," : ",curr.Vertexes[0].Point,curr.Vertexes[-1].Point)
            if i > 0:
                prev = edges[i-1]
            else:
                if closed:
                    prev = edges[-1]
                else:
                    prev = None
            if i < (len(edges)-1):
                next = edges[i+1]
            else:
                if closed: next = edges[0]
                else:
                    next = None
            if prev:
              #print("debug: DraftGeomUtils.connect prev : ",prev.Vertexes[0].Point,prev.Vertexes[-1].Point)

              # If prev v2 had been calculated, do not calculate again, just use it as current v1 - avoid chance of slight difference in result
              if v2:
                v1 = v2

              else:
                i = findIntersection(curr,prev,True,True)
                if i:
                    v1 = i[DraftVecUtils.closest(curr.Vertexes[0].Point,i)]
                else:
                    v1 = curr.Vertexes[0].Point
            else:
                v1 = curr.Vertexes[0].Point
            if next:
                #print("debug: DraftGeomUtils.connect next : ",next.Vertexes[0].Point,next.Vertexes[-1].Point)
                i = findIntersection(curr,next,True,True)
                if i:
                    v2 = i[DraftVecUtils.closest(curr.Vertexes[-1].Point,i)]
                else:
                    v2 = curr.Vertexes[-1].Point
            else:
                v2 = curr.Vertexes[-1].Point
            if geomType(curr) == "Line":
                if v1 != v2:
                    nedges.append(Part.LineSegment(v1,v2).toShape())
            elif geomType(curr) == "Circle":
                if v1 != v2:
                    nedges.append(Part.Arc(v1,findMidpoint(curr),v2).toShape())
        try:
            return Part.Wire(nedges)
        except:
            print("DraftGeomUtils.connect: unable to connect edges")
            for e in nedges:
                print(e.Curve, " ",e.Vertexes[0].Point, " ", e.Vertexes[-1].Point)
            return None

def findDistance(point,edge,strict=False):
    '''
    findDistance(vector,edge,[strict]) - Returns a vector from the point to its
    closest point on the edge. If strict is True, the vector will be returned
    only if its endpoint lies on the edge. Edge can also be a list of 2 points.
    '''
    if isinstance(point, FreeCAD.Vector):
        if isinstance(edge,list):
            segment = edge[1].sub(edge[0])
            chord = edge[0].sub(point)
            norm = segment.cross(chord)
            perp = segment.cross(norm)
            dist = DraftVecUtils.project(chord,perp)
            if not dist: return None
            newpoint = point.add(dist)
            if (dist.Length == 0):
                return None
            if strict:
                s1 = newpoint.sub(edge[0])
                s2 = newpoint.sub(edge[1])
                if (s1.Length <= segment.Length) and (s2.Length <= segment.Length):
                    return dist
                else:
                    return None
            else: return dist
        elif geomType(edge) == "Line":
            segment = vec(edge)
            chord = edge.Vertexes[0].Point.sub(point)
            norm = segment.cross(chord)
            perp = segment.cross(norm)
            dist = DraftVecUtils.project(chord,perp)
            if not dist: return None
            newpoint = point.add(dist)
            if (dist.Length == 0):
                return None
            if strict:
                s1 = newpoint.sub(edge.Vertexes[0].Point)
                s2 = newpoint.sub(edge.Vertexes[-1].Point)
                if (s1.Length <= segment.Length) and (s2.Length <= segment.Length):
                    return dist
                else:
                    return None
            else: return dist
        elif geomType(edge) == "Circle":
            ve1 = edge.Vertexes[0].Point
            if (len(edge.Vertexes) > 1):
                ve2 = edge.Vertexes[-1].Point
            else:
                ve2 = None
            center = edge.Curve.Center
            segment = center.sub(point)
            if segment.Length == 0:
                return None
            ratio = (segment.Length - edge.Curve.Radius) / segment.Length
            dist = segment.multiply(ratio)
            newpoint = Vector.add(point, dist)
            if (dist.Length == 0):
                return None
            if strict and ve2:
                ang1 = DraftVecUtils.angle(ve1.sub(center))
                ang2 = DraftVecUtils.angle(ve2.sub(center))
                angpt = DraftVecUtils.angle(newpoint.sub(center))
                if ((angpt <= ang2 and angpt >= ang1) or (angpt <= ang1 and angpt >= ang2)):
                    return dist
                else:
                    return None
            else:
                return dist
        elif geomType(edge) == "BSplineCurve" or \
            geomType(edge) == "BezierCurve":
            try:
                    pr = edge.Curve.parameter(point)
                    np = edge.Curve.value(pr)
                    dist = np.sub(point)
            except:
                    print("DraftGeomUtils: Unable to get curve parameter for point ",point)
                    return None
            else:
                    return dist
        else:
            print("DraftGeomUtils: Couldn't project point")
            return None
    else:
        print("DraftGeomUtils: Couldn't project point")
        return None


def angleBisection(edge1, edge2):
    "angleBisection(edge,edge) - Returns an edge that bisects the angle between the 2 edges."
    if (geomType(edge1) == "Line") and (geomType(edge2) == "Line"):
        p1 = edge1.Vertexes[0].Point
        p2 = edge1.Vertexes[-1].Point
        p3 = edge2.Vertexes[0].Point
        p4 = edge2.Vertexes[-1].Point
        int = findIntersection(edge1, edge2, True, True)
        if int:
            line1Dir = p2.sub(p1)
            angleDiff = DraftVecUtils.angle(line1Dir, p4.sub(p3))
            ang = angleDiff * 0.5
            origin = int[0]
            line1Dir.normalize()
            dir = DraftVecUtils.rotate(line1Dir, ang)
            return Part.LineSegment(origin,origin.add(dir)).toShape()
        else:
            diff = p3.sub(p1)
            origin = p1.add(diff.multiply(0.5))
            dir = p2.sub(p1); dir.normalize()
            return Part.LineSegment(origin,origin.add(dir)).toShape()
    else:
        return None

def findClosestCircle(point,circles):
    "findClosestCircle(Vector, list of circles) -- returns the circle with closest center"
    dist = 1000000
    closest = None
    for c in circles:
        if c.Center.sub(point).Length < dist:
            dist = c.Center.sub(point).Length
            closest = c
    return closest

def isCoplanar(faces,tolerance=0):
    "isCoplanar(faces,[tolerance]): checks if all faces in the given list are coplanar. Tolerance is the max deviation to be considered coplanar"
    if len(faces) < 2:
        return True
    base =faces[0].normalAt(0,0)
    for i in range(1,len(faces)):
        for v in faces[i].Vertexes:
            chord = v.Point.sub(faces[0].Vertexes[0].Point)
            dist = DraftVecUtils.project(chord,base)
            if round(dist.Length,precision()) > tolerance:
                return False
    return True

def isPlanar(shape):
    "checks if the given shape is planar"
    if len(shape.Vertexes) <= 3:
        return True
    n = getNormal(shape)
    for p in shape.Vertexes[1:]:
        pv = p.Point.sub(shape.Vertexes[0].Point)
        rv = DraftVecUtils.project(pv,n)
        if not DraftVecUtils.isNull(rv):
            return False
    return True

def findWiresOld(edges):
        '''finds connected edges in the list, and returns a list of lists containing edges
        that can be connected'''
        raise DeprecationWarning("This function shouldn't be called anymore - use findWires() instead")
        def verts(shape):
                return [shape.Vertexes[0].Point,shape.Vertexes[-1].Point]
        def group(shapes):
                shapesIn = shapes[:]
                shapesOut = [shapesIn.pop()]
                changed = False
                for s in shapesIn:
                        if len(s.Vertexes) < 2:
                                continue
                        else:
                                clean = True
                                for v in verts(s):
                                        for i in range(len(shapesOut)):
                                                if clean and (v in verts(shapesOut[i])):
                                                        shapesOut[i] = Part.Wire(shapesOut[i].Edges+s.Edges)
                                                        changed = True
                                                        clean = False
                                if clean:
                                        shapesOut.append(s)
                return(changed,shapesOut)
        working = True
        edgeSet = edges
        while working:
                result = group(edgeSet)
                working = result[0]
                edgeSet = result[1]
        return result[1]

def getTangent(edge,frompoint=None):
        '''
        returns the tangent to an edge. If from point is given, it is used to
        calculate the tangent (only useful for an arc of course).
        '''
        if geomType(edge) == "Line":
                return vec(edge)
        elif geomType(edge) == "BSplineCurve" or \
            geomType(edge) == "BezierCurve":
                if not frompoint:
                        return None
                cp = edge.Curve.parameter(frompoint)
                return edge.Curve.tangent(cp)[0]
        elif geomType(edge) == "Circle":
                if not frompoint:
                        v1 = edge.Vertexes[0].Point.sub(edge.Curve.Center)
                else:
                        v1 = frompoint.sub(edge.Curve.Center)
                return v1.cross(edge.Curve.Axis)
        return None

def bind(w1,w2):
    '''bind(wire1,wire2): binds 2 wires by their endpoints and
    returns a face'''
    if (not w1) or (not w2):
        print("DraftGeomUtils: unable to bind wires")
        return None
    if w1.isClosed() and w2.isClosed():
        d1 = w1.BoundBox.DiagonalLength
        d2 = w2.BoundBox.DiagonalLength
        if d1 > d2:
            #w2.reverse()
            return Part.Face([w1,w2])
        else:
            #w1.reverse()
            return Part.Face([w2,w1])
    else:
        try:
            w3 = Part.LineSegment(w1.Vertexes[0].Point,w2.Vertexes[0].Point).toShape()
            w4 = Part.LineSegment(w1.Vertexes[-1].Point,w2.Vertexes[-1].Point).toShape()
            return Part.Face(Part.Wire(w1.Edges+[w3]+w2.Edges+[w4]))
        except:
            print("DraftGeomUtils: unable to bind wires")
            return None

def cleanFaces(shape):
        "removes inner edges from coplanar faces"
        faceset = shape.Faces
        def find(hc):
                "finds a face with the given hashcode"
                for f in faceset:
                        if f.hashCode() == hc:
                                return f

        def findNeighbour(hface,hfacelist):
                "finds the first neighbour of a face in a list, and returns its index"
                eset = []
                for e in find(hface).Edges:
                        eset.append(e.hashCode())
                for i in range(len(hfacelist)):
                        for ee in find(hfacelist[i]).Edges:
                                if ee.hashCode() in eset:
                                        return i
                return None

        # build lookup table
        lut = {}
        for face in faceset:
                for edge in face.Edges:
                        if edge.hashCode() in lut:
                                lut[edge.hashCode()].append(face.hashCode())
                        else:
                                lut[edge.hashCode()] = [face.hashCode()]
        # print("lut:",lut)
        # take edges shared by 2 faces
        sharedhedges = []
        for k,v in lut.items():
                if len(v) == 2:
                        sharedhedges.append(k)
        # print(len(sharedhedges)," shared edges:",sharedhedges)
        # find those with same normals
        targethedges = []
        for hedge in sharedhedges:
                faces = lut[hedge]
                n1 = find(faces[0]).normalAt(0.5,0.5)
                n2 = find(faces[1]).normalAt(0.5,0.5)
                if n1 == n2:
                        targethedges.append(hedge)
        # print(len(targethedges)," target edges:",targethedges)
        # get target faces
        hfaces = []
        for hedge in targethedges:
                for f in lut[hedge]:
                        if not f in hfaces:
                                hfaces.append(f)

        # print(len(hfaces)," target faces:",hfaces)
        # sort islands
        islands = [[hfaces.pop(0)]]
        currentisle = 0
        currentface = 0
        found = True
        while hfaces:
                if not found:
                        if len(islands[currentisle]) > (currentface + 1):
                                currentface += 1
                                found = True
                        else:
                                islands.append([hfaces.pop(0)])
                                currentisle += 1
                                currentface = 0
                                found = True
                else:
                        f = findNeighbour(islands[currentisle][currentface],hfaces)
                        if f != None:
                                islands[currentisle].append(hfaces.pop(f))
                        else:
                                found = False
        # print(len(islands)," islands:",islands)
        # make new faces from islands
        newfaces = []
        treated = []
        for isle in islands:
                treated.extend(isle)
                fset = []
                for i in isle: fset.append(find(i))
                bounds = getBoundary(fset)
                shp = Part.Wire(Part.__sortEdges__(bounds))
                shp = Part.Face(shp)
                if shp.normalAt(0.5,0.5) != find(isle[0]).normalAt(0.5,0.5):
                        shp.reverse()
                newfaces.append(shp)
        # print("new faces:",newfaces)
        # add remaining faces
        for f in faceset:
                if not f.hashCode() in treated:
                        newfaces.append(f)
        # print("final faces")
        # finishing
        fshape = Part.makeShell(newfaces)
        if shape.isClosed():
                fshape = Part.makeSolid(fshape)
        return fshape


def isCubic(shape):
    '''isCubic(shape): verifies if a shape is cubic, that is, has
    8 vertices, 6 faces, and all angles are 90 degrees.'''
    # first we try fast methods
    if len(shape.Vertexes) != 8:
        return False
    if len(shape.Faces) != 6:
        return False
    if len(shape.Edges) != 12:
        return False
    for e in shape.Edges:
        if geomType(e) != "Line":
            return False
    # if ok until now, let's do more advanced testing
    for f in shape.Faces:
        if len(f.Edges) != 4: return False
        for i in range(4):
            e1 = vec(f.Edges[i])
            if i < 3:
                e2 = vec(f.Edges[i+1])
            else: e2 = vec(f.Edges[0])
            rpi = [0.0,round(math.pi/2,precision())]
            if not round(e1.getAngle(e2),precision()) in rpi:
                return False
    return True

def getCubicDimensions(shape):
    '''getCubicDimensions(shape): returns a list containing the placement,
    the length, the width and the height of a cubic shape. If not cubic, nothing
    is returned. The placement point is the lowest corner of the shape.'''
    if not isCubic(shape): return None
    # determine lowest face, which will be our base
    z = [10,1000000000000]
    for i in range(len(shape.Faces)):
        if shape.Faces[i].CenterOfMass.z < z[1]:
            z = [i,shape.Faces[i].CenterOfMass.z]
    if z[0] > 5: return None
    base = shape.Faces[z[0]]
    basepoint = base.Edges[0].Vertexes[0].Point
    plpoint = base.CenterOfMass
    basenorm = base.normalAt(0.5,0.5)
    # getting length and width
    vx = vec(base.Edges[0])
    vy = vec(base.Edges[1])
    # getting rotations
    rotZ = DraftVecUtils.angle(vx)
    rotY = DraftVecUtils.angle(vx,FreeCAD.Vector(vx.x,vx.y,0))
    rotX = DraftVecUtils.angle(vy,FreeCAD.Vector(vy.x,vy.y,0))
    # getting height
    vz = None
    rpi = round(math.pi/2,precision())
    for i in range(1,6):
        for e in shape.Faces[i].Edges:
            if basepoint in [e.Vertexes[0].Point,e.Vertexes[1].Point]:
                vtemp = vec(e)
                # print(vtemp)
                if round(vtemp.getAngle(vx),precision()) == rpi:
                    if round(vtemp.getAngle(vy),precision()) == rpi:
                        vz = vtemp
    if not vz: return None
    mat = FreeCAD.Matrix()
    mat.move(plpoint)
    mat.rotateX(rotX)
    mat.rotateY(rotY)
    mat.rotateZ(rotZ)
    return [FreeCAD.Placement(mat),round(vx.Length,precision()),round(vy.Length,precision()),round(vz.Length,precision())]

def removeInterVertices(wire):
        '''removeInterVertices(wire) - remove unneeded vertices (those that
        are in the middle of a straight line) from a wire, returns a new wire.'''
        edges = Part.__sortEdges__(wire.Edges)
        nverts = []
        def getvec(v1,v2):
                if not abs(round(v1.getAngle(v2),precision()) in [0,round(math.pi,precision())]):
                        nverts.append(edges[i].Vertexes[-1].Point)
        for i in range(len(edges)-1):
                vA = vec(edges[i])
                vB = vec(edges[i+1])
                getvec(vA,vB)
        vA = vec(edges[-1])
        vB = vec(edges[0])
        getvec(vA,vB)
        if nverts:
                if wire.isClosed():
                        nverts.append(nverts[0])
                w = Part.makePolygon(nverts)
                return w
        else:
                return wire

def arcFromSpline(edge):
        """arcFromSpline(edge): turns the given edge into an arc, by taking
        its first point, midpoint and endpoint. Works best with bspline
        segments such as those from imported svg files. Use this only
        if you are sure your edge is really an arc..."""
        if geomType(edge) == "Line":
                print("This edge is straight, cannot build an arc on it")
                return None
        if len(edge.Vertexes) > 1:
                # 2-point arc
                p1 = edge.Vertexes[0].Point
                p2 = edge.Vertexes[-1].Point
                ml = edge.Length/2
                p3 = edge.valueAt(ml)
                try:
                        return Part.Arc(p1,p3,p2).toShape()
                except:
                        print("Couldn't make an arc out of this edge")
                        return None
        else:
                # circle
                p1 = edge.Vertexes[0].Point
                ml = edge.Length/2
                p2 = edge.valueAt(ml)
                ray = p2.sub(p1)
                ray.scale(.5,.5,.5)
                center = p1.add(ray)
                radius = ray.Length
                try:
                        return Part.makeCircle(radius,center)
                except:
                        print("couldn't make a circle out of this edge")

# Fillet code graciously donated by Jacques-Antoine Gaudin

def fillet(lEdges,r,chamfer=False):
    '''fillet(lEdges,r,chamfer=False): Take a list of two Edges & a float as argument,
    Returns a list of sorted edges describing a round corner'''

    def getCurveType(edge,existingCurveType = None):
            '''Builds or completes a dictionary containing edges with keys "Arc" and "Line"'''
            if not existingCurveType :
                    existingCurveType = { 'Line' : [], 'Arc' : [] }
            if issubclass(type(edge.Curve),Part.LineSegment) :
                    existingCurveType['Line'] += [edge]
            elif issubclass(type(edge.Curve),Part.Line) :
                    existingCurveType['Line'] += [edge]
            elif issubclass(type(edge.Curve),Part.Circle) :
                    existingCurveType['Arc']  += [edge]
            else :
                    raise ValueError("Edge's curve must be either Line or Arc")
            return existingCurveType

    rndEdges = lEdges[0:2]
    rndEdges = Part.__sortEdges__(rndEdges)

    if len(rndEdges) < 2 :
        return rndEdges

    if r <= 0 :
        print("DraftGeomUtils.fillet : Error : radius is negative.")
        return rndEdges

    curveType = getCurveType(rndEdges[0])
    curveType = getCurveType(rndEdges[1],curveType)

    lVertexes = rndEdges[0].Vertexes + [rndEdges[1].Vertexes[-1]]

    if len(curveType['Line']) == 2:

        # Deals with 2-line-edges lists --------------------------------------

        U1 = lVertexes[0].Point.sub(lVertexes[1].Point) ; U1.normalize()
        U2 = lVertexes[2].Point.sub(lVertexes[1].Point) ; U2.normalize()
        alpha = U1.getAngle(U2)

        if chamfer:
            # correcting r value so the size of the chamfer = r
            beta = math.pi - alpha/2
            r = (r/2)/math.cos(beta)

        if round(alpha,precision()) == 0 or round(alpha - math.pi,precision()) == 0: # Edges have same direction
            print("DraftGeomUtils.fillet : Warning : edges have same direction. Did nothing")
            return rndEdges

        dToCenter = r / math.sin(alpha/2.)
        dToTangent = (dToCenter**2-r**2)**(0.5)
        dirVect = Vector(U1) ; dirVect.scale(dToTangent,dToTangent,dToTangent)
        arcPt1 = lVertexes[1].Point.add(dirVect)

        dirVect = U2.add(U1) ; dirVect.normalize()
        dirVect.scale(dToCenter-r,dToCenter-r,dToCenter-r)
        arcPt2 = lVertexes[1].Point.add(dirVect)

        dirVect = Vector(U2) ; dirVect.scale(dToTangent,dToTangent,dToTangent)
        arcPt3 = lVertexes[1].Point.add(dirVect)

        if (dToTangent>lEdges[0].Length) or (dToTangent>lEdges[1].Length) :
            print("DraftGeomUtils.fillet : Error : radius value ", r," is too high")
            return rndEdges
        if chamfer:
            rndEdges[1]   =  Part.Edge(Part.LineSegment(arcPt1,arcPt3))
        else:
            rndEdges[1]   =  Part.Edge(Part.Arc(arcPt1,arcPt2,arcPt3))

        if lVertexes[0].Point == arcPt1:
            # fillet consumes entire first edge
            rndEdges.pop(0)
        else:
            rndEdges[0]   =  Part.Edge(Part.LineSegment(lVertexes[0].Point,arcPt1))

        if lVertexes[2].Point != arcPt3:
            # fillet does not consume entire second edge
            rndEdges     += [Part.Edge(Part.LineSegment(arcPt3,lVertexes[2].Point))]

        return rndEdges

    elif len(curveType['Arc']) == 1 :

        # Deals with lists containing an arc and a line ----------------------------------

        if lEdges[0] in curveType['Arc'] :
            lineEnd = lVertexes[2] ; arcEnd = lVertexes[0] ; arcFirst = True
        else :
            lineEnd = lVertexes[0] ; arcEnd = lVertexes[2] ; arcFirst = False
        arcCenter = curveType['Arc'][0].Curve.Center
        arcRadius = curveType['Arc'][0].Curve.Radius
        arcAxis   = curveType['Arc'][0].Curve.Axis
        arcLength = curveType['Arc'][0].Length

        U1 = lineEnd.Point.sub(lVertexes[1].Point) ; U1.normalize()
        toCenter = arcCenter.sub(lVertexes[1].Point)
        if arcFirst : # make sure the tangent points towards the arc
            T = arcAxis.cross(toCenter)
        else :
            T = toCenter.cross(arcAxis)

        projCenter = toCenter.dot(U1)
        if round(abs(projCenter),precision()) > 0 :
            normToLine = U1.cross(T).cross(U1)
        else :
            normToLine = Vector(toCenter)
        normToLine.normalize()

        dCenterToLine = toCenter.dot(normToLine) - r

        if  round(projCenter,precision()) > 0 :
            newRadius = arcRadius - r
        elif round(projCenter,precision()) < 0 or (round(projCenter,precision()) == 0 and U1.dot(T) > 0):
            newRadius = arcRadius + r
        else :
            print("DraftGeomUtils.fillet : Warning : edges are already tangent. Did nothing")
            return rndEdges

        toNewCent = newRadius**2-dCenterToLine**2
        if toNewCent > 0 :
            toNewCent = abs(abs(projCenter) - toNewCent**(0.5))
        else :
            print("DraftGeomUtils.fillet : Error : radius value ", r," is too high")
            return rndEdges

        U1.scale(toNewCent,toNewCent,toNewCent)
        normToLine.scale(r,r,r)
        newCent = lVertexes[1].Point.add(U1).add(normToLine)

        arcPt1= lVertexes[1].Point.add(U1)
        arcPt2= lVertexes[1].Point.sub(newCent); arcPt2.normalize()
        arcPt2.scale(r,r,r) ; arcPt2 = arcPt2.add(newCent)
        if newRadius == arcRadius - r :
            arcPt3= newCent.sub(arcCenter)
        else :
            arcPt3= arcCenter.sub(newCent)
        arcPt3.normalize()
        arcPt3.scale(r,r,r) ; arcPt3 = arcPt3.add(newCent)
        arcPt = [arcPt1,arcPt2,arcPt3]


        # Warning : In the following I used a trick for calling the right element
        # in arcPt or V : arcFirst is a boolean so - not arcFirst is -0 or -1
        # list[-1] is the last element of a list and list[0] the first
        # this way I don't have to proceed tests to know the position of the arc

        myTrick = not arcFirst

        V  = [arcPt3]
        V += [arcEnd.Point]

        toCenter.scale(-1,-1,-1)

        delLength = arcRadius * V[0].sub(arcCenter).getAngle(toCenter)
        if delLength > arcLength or toNewCent > curveType['Line'][0].Length:
            print("DraftGeomUtils.fillet : Error : radius value ", r," is too high")
            return rndEdges

        arcAsEdge = arcFrom2Pts(V[-arcFirst],V[-myTrick],arcCenter,arcAxis)

        V = [lineEnd.Point,arcPt1]
        lineAsEdge = Part.Edge(Part.LineSegment(V[-arcFirst],V[myTrick]))

        rndEdges[not arcFirst]   =  arcAsEdge
        rndEdges[arcFirst]       =  lineAsEdge
        if chamfer:
            rndEdges[1:1] = [Part.Edge(Part.LineSegment(arcPt[- arcFirst],arcPt[- myTrick]))]
        else:
            rndEdges[1:1] = [Part.Edge(Part.Arc(arcPt[- arcFirst],arcPt[1],arcPt[- myTrick]))]

        return rndEdges

    elif len(curveType['Arc']) == 2 :

        # Deals with lists of 2 arc-edges --------------------------------------------

        arcCenter, arcRadius, arcAxis, arcLength, toCenter, T, newRadius = [], [], [], [], [], [], []
        for i in range(2) :
            arcCenter += [curveType['Arc'][i].Curve.Center]
            arcRadius += [curveType['Arc'][i].Curve.Radius]
            arcAxis   += [curveType['Arc'][i].Curve.Axis]
            arcLength += [curveType['Arc'][i].Length]
            toCenter  += [arcCenter[i].sub(lVertexes[1].Point)]
        T += [arcAxis[0].cross(toCenter[0])]
        T += [toCenter[1].cross(arcAxis[1])]
        CentToCent = toCenter[1].sub(toCenter[0])
        dCentToCent = CentToCent.Length

        sameDirection = (arcAxis[0].dot(arcAxis[1]) > 0)
        TcrossT = T[0].cross(T[1])
        if sameDirection :
            if   round(TcrossT.dot(arcAxis[0]),precision()) > 0 :
                newRadius += [arcRadius[0]+r]
                newRadius += [arcRadius[1]+r]
            elif round(TcrossT.dot(arcAxis[0]),precision()) < 0 :
                newRadius += [arcRadius[0]-r]
                newRadius += [arcRadius[1]-r]
            elif T[0].dot(T[1]) > 0 :
                newRadius += [arcRadius[0]+r]
                newRadius += [arcRadius[1]+r]
            else :
                print("DraftGeomUtils.fillet : Warning : edges are already tangent. Did nothing")
                return rndEdges
        elif not sameDirection :
            if   round(TcrossT.dot(arcAxis[0]),precision()) > 0 :
                newRadius += [arcRadius[0]+r]
                newRadius += [arcRadius[1]-r]
            elif round(TcrossT.dot(arcAxis[0]),precision()) < 0 :
                newRadius += [arcRadius[0]-r]
                newRadius += [arcRadius[1]+r]
            elif T[0].dot(T[1]) > 0 :
                if arcRadius[0] > arcRadius[1] :
                    newRadius += [arcRadius[0]-r]
                    newRadius += [arcRadius[1]+r]
                elif arcRadius[1] > arcRadius[0] :
                    newRadius += [arcRadius[0]+r]
                    newRadius += [arcRadius[1]-r]
                else :
                    print("DraftGeomUtils.fillet : Warning : arcs are coincident. Did nothing")
                    return rndEdges
            else :
                print("DraftGeomUtils.fillet : Warning : edges are already tangent. Did nothing")
                return rndEdges

        if newRadius[0]+newRadius[1] < dCentToCent or \
           newRadius[0]-newRadius[1] > dCentToCent or \
           newRadius[1]-newRadius[0] > dCentToCent :
            print("DraftGeomUtils.fillet : Error : radius value ", r," is too high")
            return rndEdges

        x = (dCentToCent**2+newRadius[0]**2-newRadius[1]**2)/(2*dCentToCent)
        y = (newRadius[0]**2-x**2)**(0.5)

        CentToCent.normalize() ; toCenter[0].normalize() ; toCenter[1].normalize()
        if abs(toCenter[0].dot(toCenter[1])) != 1 :
            normVect = CentToCent.cross(CentToCent.cross(toCenter[0]))
        else :
            normVect = T[0]
        normVect.normalize()
        CentToCent.scale(x,x,x) ; normVect.scale(y,y,y)
        newCent = arcCenter[0].add(CentToCent.add(normVect))
        CentToNewCent = [newCent.sub(arcCenter[0]),newCent.sub(arcCenter[1])]
        for i in range(2) :
            CentToNewCent[i].normalize()
            if newRadius[i] == arcRadius[i]+r :
                CentToNewCent[i].scale(-r,-r,-r)
            else :
                CentToNewCent[i].scale(r,r,r)
        toThirdPt = lVertexes[1].Point.sub(newCent) ; toThirdPt.normalize()
        toThirdPt.scale(r,r,r)
        arcPt1 = newCent.add(CentToNewCent[0])
        arcPt2 = newCent.add(toThirdPt)
        arcPt3 = newCent.add(CentToNewCent[1])
        arcPt = [arcPt1,arcPt2,arcPt3]

        arcAsEdge = []
        for i in range(2) :
            toCenter[i].scale(-1,-1,-1)
            delLength = arcRadius[i] * arcPt[-i].sub(arcCenter[i]).getAngle(toCenter[i])
            if delLength > arcLength[i] :
                print("DraftGeomUtils.fillet : Error : radius value ", r," is too high")
                return rndEdges
            V = [arcPt[-i],lVertexes[-i].Point]
            arcAsEdge += [arcFrom2Pts(V[i-1],V[-i],arcCenter[i],arcAxis[i])]

        rndEdges[0]   =  arcAsEdge[0]
        rndEdges[1]   =  arcAsEdge[1]
        if chamfer:
            rndEdges[1:1] = [Part.Edge(Part.LineSegment(arcPt[0],arcPt[2]))]
        else:
            rndEdges[1:1] = [Part.Edge(Part.Arc(arcPt[0],arcPt[1],arcPt[2]))]

        return rndEdges

def filletWire(aWire,r,chamfer=False):
    ''' Fillets each angle of a wire with r as radius value
    if chamfer is true, a chamfer is made instead and r is the
    size of the chamfer'''

    edges = aWire.Edges
    edges = Part.__sortEdges__(edges)
    filEdges = [edges[0]]
    for i in range(len(edges)-1):
        result = fillet([filEdges[-1],edges[i+1]],r,chamfer)
        if len(result)>2:
            filEdges[-1:] = result[0:3]
        else :
            filEdges[-1:] = result[0:2]
    if isReallyClosed(aWire):
        result = fillet([filEdges[-1],filEdges[0]],r,chamfer)
        if len(result)>2:
            filEdges[-1:] = result[0:2]
            filEdges[0]   = result[2]
    return Part.Wire(filEdges)

def getCircleFromSpline(edge):
    "returns a circle-based edge from a bspline-based edge"
    if geomType(edge) != "BSplineCurve":
        return None
    if len(edge.Vertexes) != 1:
        return None
    # get 2 points
    p1 = edge.Curve.value(0)
    p2 = edge.Curve.value(math.pi/2)
    # get 2 tangents
    t1 = edge.Curve.tangent(0)[0]
    t2 = edge.Curve.tangent(math.pi/2)[0]
    # get normal
    n = p1.cross(p2)
    if DraftVecUtils.isNull(n):
        return None
    # get rays
    r1 = DraftVecUtils.rotate(t1,math.pi/2,n)
    r2 = DraftVecUtils.rotate(t2,math.pi/2,n)
    # get center (intersection of rays)
    i = findIntersection(p1,p1.add(r1),p2,p2.add(r2),True,True)
    if not i:
        return None
    c = i[0]
    r = (p1.sub(c)).Length
    circle = Part.makeCircle(r,c,n)
    #print(circle.Curve)
    return circle

def curvetowire(obj,steps):
    points = obj.copy().discretize(steps)
    p0 = points[0]
    edgelist = []
    for p in points[1:]:
        edge = Part.makeLine((p0.x,p0.y,p0.z),(p.x,p.y,p.z))
        edgelist.append(edge)
        p0 = p
    return edgelist

def cleanProjection(shape,tessellate=True,seglength=.05):
    "returns a valid compound of edges, by recreating them"
    # this is because the projection algorithm somehow creates wrong shapes.
    # they display fine, but on loading the file the shape is invalid
    # Now with tanderson's fix to ProjectionAlgos, that isn't the case, but this
    # can be used for tessellating ellipses and splines for DXF output-DF
    oldedges = shape.Edges
    newedges = []
    for e in oldedges:
        try:
            if geomType(e) == "Line":
                newedges.append(e.Curve.toShape())
            elif geomType(e) == "Circle":
                if len(e.Vertexes) > 1:
                    mp = findMidpoint(e)
                    a = Part.Arc(e.Vertexes[0].Point,mp,e.Vertexes[-1].Point).toShape()
                    newedges.append(a)
                else:
                    newedges.append(e.Curve.toShape())
            elif geomType(e) == "Ellipse":
                if tessellate:
                    newedges.append(Part.Wire(curvetowire(e, seglength)))
                else:
                    if len(e.Vertexes) > 1:
                        a = Part.Arc(e.Curve,e.FirstParameter,e.LastParameter).toShape()
                        newedges.append(a)
                    else:
                        newedges.append(e.Curve.toShape())
            elif geomType(e) == "BSplineCurve" or \
                 geomType(e) == "BezierCurve":
                if tessellate:
                    newedges.append(Part.Wire(curvetowire(e,seglength)))
                else:
                    if isLine(e.Curve):
                        l = Part.LineSegment(e.Vertexes[0].Point,e.Vertexes[-1].Point).toShape()
                        newedges.append(l)
                    else:
                        newedges.append(e.Curve.toShape(e.FirstParameter,e.LastParameter))
            else:
                newedges.append(e)
        except:
            print("Debug: error cleaning edge ",e)
    return Part.makeCompound(newedges)

def curvetosegment(curve,seglen):
    points = curve.discretize(seglen)
    p0 = points[0]
    edgelist = []
    for p in points[1:]:
        edge = Part.makeLine((p0.x,p0.y,p0.z),(p.x,p.y,p.z))
        edgelist.append(edge)
        p0 = p
    return edgelist

def tessellateProjection(shape,seglen):
    ''' Returns projection with BSplines and Ellipses broken into line segments.
        Useful for exporting projected views to *dxf files.'''
    oldedges = shape.Edges
    newedges = []
    for e in oldedges:
        try:
            if geomType(e) == "Line":
                newedges.append(e.Curve.toShape())
            elif geomType(e) == "Circle":
                newedges.append(e.Curve.toShape())
            elif geomType(e) == "Ellipse":
                newedges.append(Part.Wire(curvetosegment(e,seglen)))
            elif geomType(e) == "BSplineCurve":
                newedges.append(Part.Wire(curvetosegment(e,seglen)))
            else:
                newedges.append(e)
        except:
            print("Debug: error cleaning edge ",e)
    return Part.makeCompound(newedges)


def rebaseWire(wire,vidx):

    """rebaseWire(wire,vidx): returns a new wire which is a copy of the
    current wire, but where the first vertex is the vertex indicated by the given
    index vidx, starting from 1. 0 will return an exact copy of the wire."""

    if vidx < 1:
        return wire
    if vidx > len(wire.Vertexes):
        #print("Vertex index above maximum\n")
        return wire
    #This can be done in one step
    return Part.Wire(wire.Edges[vidx-1:] + wire.Edges[:vidx-1])


def removeSplitter(shape):
    """an alternative, shared edge-based version of Part.removeSplitter. Returns a
    face or None if the operation failed"""
    lut = {}
    for f in shape.Faces:
        for e in f.Edges:
            h = e.hashCode()
            if h in lut:
                lut[h].append(e)
            else:
                lut[h] = [e]
    edges = [e[0] for e in lut.values() if len(e) == 1]
    try:
        face = Part.Face(Part.Wire(edges))
    except:
        # operation failed
        return None
    else:
        if face.isValid():
            return face
    return None


# circle functions *********************************************************


def getBoundaryAngles(angle,alist):
        '''returns the 2 closest angles from the list that
        encompass the given angle'''
        negs = True
        while negs:
                negs = False
                for i in range(len(alist)):
                        if alist[i] < 0:
                                alist[i] = 2*math.pi + alist[i]
                                negs = True
                if angle < 0:
                        angle = 2*math.pi + angle
                        negs = True
        lower = None
        for a in alist:
                if a < angle:
                        if lower == None:
                                lower = a
                        else:
                                if a > lower:
                                        lower = a
        if lower == None:
                lower = 0
                for a in alist:
                        if a > lower:
                                lower = a
        higher = None
        for a in alist:
                if a > angle:
                        if higher == None:
                                higher = a
                        else:
                                if a < higher:
                                        higher = a
        if higher == None:
                higher = 2*math.pi
                for a in alist:
                        if a < higher:
                                higher = a
        return (lower,higher)


def circleFrom2tan1pt(tan1, tan2, point):
    "circleFrom2tan1pt(edge, edge, Vector)"
    if (geomType(tan1) == "Line") and (geomType(tan2) == "Line") and isinstance(point, FreeCAD.Vector):
        return circlefrom2Lines1Point(tan1, tan2, point)
    elif (geomType(tan1) == "Circle") and (geomType(tan2) == "Line") and isinstance(point, FreeCAD.Vector):
        return circlefromCircleLinePoint(tan1, tan2, point)
    elif (geomType(tan2) == "Circle") and (geomType(tan1) == "Line") and isinstance(point, FreeCAD.Vector):
        return circlefromCircleLinePoint(tan2, tan1, point)
    elif (geomType(tan2) == "Circle") and (geomType(tan1) == "Circle") and isinstance(point, FreeCAD.Vector):
        return circlefrom2Circles1Point(tan2, tan1, point)

def circleFrom2tan1rad(tan1, tan2, rad):
    "circleFrom2tan1rad(edge, edge, float)"
    if (geomType(tan1) == "Line") and (geomType(tan2) == "Line"):
        return circleFrom2LinesRadius(tan1, tan2, rad)
    elif (geomType(tan1) == "Circle") and (geomType(tan2) == "Line"):
        return circleFromCircleLineRadius(tan1, tan2, rad)
    elif (geomType(tan1) == "Line") and (geomType(tan2) == "Circle"):
        return circleFromCircleLineRadius(tan2, tan1, rad)
    elif (geomType(tan1) == "Circle") and (geomType(tan2) == "Circle"):
        return circleFrom2CirclesRadius(tan1, tan2, rad)

def circleFrom1tan2pt(tan1, p1, p2):
    if (geomType(tan1) == "Line") and isinstance(p1, FreeCAD.Vector) and isinstance(p2, FreeCAD.Vector):
        return circlefrom1Line2Points(tan1, p1, p2)
    if (geomType(tan1) == "Line") and isinstance(p1, FreeCAD.Vector) and isinstance(p2, FreeCAD.Vector):
        return circlefrom1Circle2Points(tan1, p1, p2)

def circleFrom1tan1pt1rad(tan1, p1, rad):
    if (geomType(tan1) == "Line") and isinstance(p1, FreeCAD.Vector):
        return circleFromPointLineRadius(p1, tan1, rad)
    if (geomType(tan1) == "Circle") and isinstance(p1, FreeCAD.Vector):
        return circleFromPointCircleRadius(p1, tan1, rad)

def circleFrom3tan(tan1, tan2, tan3):
    tan1IsLine = (geomType(tan1) == "Line")
    tan2IsLine = (geomType(tan2) == "Line")
    tan3IsLine = (geomType(tan3) == "Line")
    tan1IsCircle = (geomType(tan1) == "Circle")
    tan2IsCircle = (geomType(tan2) == "Circle")
    tan3IsCircle = (geomType(tan3) == "Circle")
    if tan1IsLine and tan2IsLine and tan3IsLine:
        return circleFrom3LineTangents(tan1, tan2, tan3)
    elif tan1IsCircle and tan2IsCircle and tan3IsCircle:
        return circleFrom3CircleTangents(tan1, tan2, tan3)
    elif (tan1IsCircle and tan2IsLine and tan3IsLine):
        return circleFrom1Circle2Lines(tan1, tan2, tan3)
    elif (tan1IsLine and tan2IsCircle and tan3IsLine):
        return circleFrom1Circle2Lines(tan2, tan1, tan3)
    elif (tan1IsLine and tan2IsLine and tan3IsCircle):
        return circleFrom1Circle2Lines(tan3, tan1, tan2)
    elif (tan1IsLine and tan2IsCircle and tan3IsCircle):
        return circleFrom2Circle1Lines(tan2, tan3, tan1)
    elif (tan1IsCircle and tan2IsLine and tan3IsCircle):
        return circleFrom2Circle1Lines(tan1, tan3, tan2)
    elif (tan1IsCircle and tan2IsCircle and tan3IsLine):
        return circleFrom2Circle1Lines(tan1, tan2, tan3)

def circlefrom2Lines1Point(edge1, edge2, point):
    "circlefrom2Lines1Point(edge, edge, Vector)"
    bis = angleBisection(edge1, edge2)
    if not bis: return None
    mirrPoint = mirror(point, bis)
    return circlefrom1Line2Points(edge1, point, mirrPoint)

def circlefrom1Line2Points(edge, p1, p2):
    "circlefrom1Line2Points(edge, Vector, Vector)"
    p1_p2 = edg(p1, p2)
    s = findIntersection(edge, p1_p2, True, True)
    if not s: return None
    s = s[0]
    v1 = p1.sub(s)
    v2 = p2.sub(s)
    projectedDist = math.sqrt(abs(v1.dot(v2)))
    edgeDir = vec(edge); edgeDir.normalize()
    projectedCen1 = Vector.add(s, Vector(edgeDir).multiply(projectedDist))
    projectedCen2 = Vector.add(s, Vector(edgeDir).multiply(-projectedDist))
    perpEdgeDir = edgeDir.cross(Vector(0,0,1))
    perpCen1 = Vector.add(projectedCen1, perpEdgeDir)
    perpCen2 = Vector.add(projectedCen2, perpEdgeDir)
    mid = findMidpoint(p1_p2)
    x = DraftVecUtils.crossproduct(vec(p1_p2)); x.normalize()
    perp_mid = Vector.add(mid, x)
    cen1 = findIntersection(edg(projectedCen1, perpCen1), edg(mid, perp_mid), True, True)
    cen2 = findIntersection(edg(projectedCen2, perpCen2), edg(mid, perp_mid), True, True)
    circles = []
    if cen1:
        radius = DraftVecUtils.dist(projectedCen1, cen1[0])
        circles.append(Part.Circle(cen1[0], NORM, radius))
    if cen2:
        radius = DraftVecUtils.dist(projectedCen2, cen2[0])
        circles.append(Part.Circle(cen2[0], NORM, radius))

    if circles: return circles
    else: return None

def circleFrom2LinesRadius (edge1, edge2, radius):
    "circleFrom2LinesRadius(edge,edge,radius)"
    int = findIntersection(edge1, edge2, True, True)
    if not int: return None
    int = int[0]
    bis12 = angleBisection(edge1,edge2)
    bis21 = Part.LineSegment(bis12.Vertexes[0].Point,DraftVecUtils.rotate(vec(bis12), math.pi/2.0))
    ang12 = abs(DraftVecUtils.angle(vec(edge1),vec(edge2)))
    ang21 = math.pi - ang12
    dist12 = radius / math.sin(ang12 * 0.5)
    dist21 = radius / math.sin(ang21 * 0.5)
    circles = []
    cen = Vector.add(int, vec(bis12).multiply(dist12))
    circles.append(Part.Circle(cen, NORM, radius))
    cen = Vector.add(int, vec(bis12).multiply(-dist12))
    circles.append(Part.Circle(cen, NORM, radius))
    cen = Vector.add(int, vec(bis21).multiply(dist21))
    circles.append(Part.Circle(cen, NORM, radius))
    cen = Vector.add(int, vec(bis21).multiply(-dist21))
    circles.append(Part.Circle(cen, NORM, radius))
    return circles

def circleFrom3LineTangents (edge1, edge2, edge3):
    "circleFrom3LineTangents(edge,edge,edge)"
    def rot(ed):
        return Part.LineSegment(v1(ed),v1(ed).add(DraftVecUtils.rotate(vec(ed),math.pi/2))).toShape()
    bis12 = angleBisection(edge1,edge2)
    bis23 = angleBisection(edge2,edge3)
    bis31 = angleBisection(edge3,edge1)
    intersections = []
    int = findIntersection(bis12, bis23, True, True)
    if int:
        radius = findDistance(int[0],edge1).Length
        intersections.append(Part.Circle(int[0],NORM,radius))
    int = findIntersection(bis23, bis31, True, True)
    if int:
        radius = findDistance(int[0],edge1).Length
        intersections.append(Part.Circle(int[0],NORM,radius))
    int = findIntersection(bis31, bis12, True, True)
    if int:
        radius = findDistance(int[0],edge1).Length
        intersections.append(Part.Circle(int[0],NORM,radius))
    int = findIntersection(rot(bis12), rot(bis23), True, True)
    if int:
        radius = findDistance(int[0],edge1).Length
        intersections.append(Part.Circle(int[0],NORM,radius))
    int = findIntersection(rot(bis23), rot(bis31), True, True)
    if int:
        radius = findDistance(int[0],edge1).Length
        intersections.append(Part.Circle(int[0],NORM,radius))
    int = findIntersection(rot(bis31), rot(bis12), True, True)
    if int:
        radius = findDistance(int[0],edge1).Length
        intersections.append(Part.Circle(int[0],NORM,radius))
    circles = []
    for int in intersections:
        exists = False
        for cir in circles:
            if DraftVecUtils.equals(cir.Center, int.Center):
                exists = True
                break
        if not exists:
            circles.append(int)
    if circles:
        return circles
    else:
        return None

def circleFromPointLineRadius (point, edge, radius):
    "circleFromPointLineRadius (point, edge, radius)"
    dist = findDistance(point, edge, False)
    center1 = None
    center2 = None
    if dist.Length == 0:
        segment = vec(edge)
        perpVec = DraftVecUtils.crossproduct(segment); perpVec.normalize()
        normPoint_c1 = Vector(perpVec).multiply(radius)
        normPoint_c2 = Vector(perpVec).multiply(-radius)
        center1 = point.add(normPoint_c1)
        center2 = point.add(normPoint_c2)
    elif dist.Length > 2 * radius:
        return None
    elif dist.Length == 2 * radius:
        normPoint = point.add(findDistance(point, edge, False))
        dummy = (normPoint.sub(point)).multiply(0.5)
        cen = point.add(dummy)
        circ = Part.Circle(cen, NORM, radius)
        if circ:
            return [circ]
        else:
            return None
    else:
        normPoint = point.add(findDistance(point, edge, False))
        normDist = DraftVecUtils.dist(normPoint, point)
        dist = math.sqrt(radius**2 - (radius - normDist)**2)
        centerNormVec = DraftVecUtils.scaleTo(point.sub(normPoint), radius)
        edgeDir = edge.Vertexes[0].Point.sub(normPoint); edgeDir.normalize()
        center1 = centerNormVec.add(normPoint.add(Vector(edgeDir).multiply(dist)))
        center2 = centerNormVec.add(normPoint.add(Vector(edgeDir).multiply(-dist)))
    circles = []
    if center1:
        circ = Part.Circle(center1, NORM, radius)
        if circ:
            circles.append(circ)
    if center2:
        circ = Part.Circle(center2, NORM, radius)
        if circ:
            circles.append(circ)

    if len(circles):
        return circles
    else:
        return None

def circleFrom2PointsRadius(p1, p2, radius):
    "circleFrom2PointsRadiust(Vector, Vector, radius)"
    if DraftVecUtils.equals(p1, p2): return None

    p1_p2 = Part.LineSegment(p1, p2).toShape()
    dist_p1p2 = DraftVecUtils.dist(p1, p1)
    mid = findMidpoint(p1_p2)
    if dist_p1p2 == 2*radius:
        circle = Part.Circle(mid, NORM, radius)
        if circle: return [circle]
        else: return None
    dir = vec(p1_p2); dir.normalize()
    perpDir = dir.cross(Vector(0,0,1)); perpDir.normalize()
    dist = math.sqrt(radius**2 - (dist_p1p2 / 2.0)**2)
    cen1 = Vector.add(mid, Vector(perpDir).multiply(dist))
    cen2 = Vector.add(mid, Vector(perpDir).multiply(-dist))
    circles = []
    if cen1: circles.append(Part.Circle(cen1, NORM, radius))
    if cen2: circles.append(Part.Circle(cen2, NORM, radius))
    if circles: return circles
    else: return None


def arcFrom2Pts(firstPt,lastPt,center,axis=None):

    '''Builds an arc with center and 2 points, can be oriented with axis'''

    radius1  = firstPt.sub(center).Length
    radius2  = lastPt.sub(center).Length
    if round(radius1-radius2,4) != 0 : # (PREC = 4 = same as Part Module),  Is it possible ?
        return None

    thirdPt = Vector(firstPt.sub(center).add(lastPt).sub(center))
    thirdPt.normalize()
    thirdPt.scale(radius1,radius1,radius1)
    thirdPt = thirdPt.add(center)
    newArc = Part.Edge(Part.Arc(firstPt,thirdPt,lastPt))
    if not axis is None and newArc.Curve.Axis.dot(axis) < 0 :
        thirdPt = thirdPt.sub(center)
        thirdPt.scale(-1,-1,-1)
        thirdPt = thirdPt.add(center)
        newArc = Part.Edge(Part.Arc(firstPt,thirdPt,lastPt))
    return newArc


#############################33 to include







def outerSoddyCircle(circle1, circle2, circle3):
    '''
    Computes the outer soddy circle for three tightly packed circles.
    '''
    if (geomType(circle1) == "Circle") and (geomType(circle2) == "Circle") \
    and (geomType(circle3) == "Circle"):
        # Original Java code Copyright (rc) 2008 Werner Randelshofer
        # Converted to python by Martin Buerbaum 2009
        # http://www.randelshofer.ch/treeviz/
        # Either Creative Commons Attribution 3.0, the MIT license, or the GNU Lesser General License LGPL.

        A = circle1.Curve.Center
        B = circle2.Curve.Center
        C = circle3.Curve.Center

        ra = circle1.Curve.Radius
        rb = circle2.Curve.Radius
        rc = circle3.Curve.Radius

        # Solution using Descartes' theorem, as described here:
        # http://en.wikipedia.org/wiki/Descartes%27_theorem
        k1 = 1 / ra
        k2 = 1 / rb
        k3 = 1 / rc
        k4 = abs(k1 + k2 + k3 - 2 * math.sqrt(k1 * k2 + k2 * k3 + k3 * k1))

        q1 = (k1 + 0j) * (A.x + A.y * 1j)
        q2 = (k2 + 0j) * (B.x + B.y * 1j)
        q3 = (k3 + 0j) * (C.x + C.y * 1j)

        temp = ((q1 * q2) + (q2 * q3) + (q3 * q1))
        q4 = q1 + q2 + q3 - ((2 + 0j) * cmath.sqrt(temp) )

        z = q4 / (k4 + 0j)

        # If the formula is not solvable, we return no circle.
        if (not z or not (1 / k4)):
            return None

        X = -z.real
        Y = -z.imag
        print("Outer Soddy circle: " + str(X) + " " + str(Y) + "\n") # Debug

        # The Radius of the outer soddy circle can also be calculated with the following formula:
        # radiusOuter = abs(r1*r2*r3 / (r1*r2 + r1*r3 + r2*r3 - 2 * math.sqrt(r1*r2*r3 * (r1+r2+r3))))
        circ = Part.Circle(Vector(X, Y, A.z), norm, 1 / k4)
        return circ

    else:
        print("debug: outerSoddyCircle bad parameters!\n")
        # FreeCAD.Console.PrintMessage("debug: outerSoddyCircle bad parameters!\n")
        return None

def innerSoddyCircle(circle1, circle2, circle3):
    '''
    Computes the inner soddy circle for three tightly packed circles.
    '''
    if (geomType(circle1) == "Circle") and (geomType(circle2) == "Circle") \
    and (geomType(circle3) == "Circle"):
        # Original Java code Copyright (rc) 2008 Werner Randelshofer
        # Converted to python by Martin Buerbaum 2009
        # http://www.randelshofer.ch/treeviz/

        A = circle1.Curve.Center
        B = circle2.Curve.Center
        C = circle3.Curve.Center

        ra = circle1.Curve.Radius
        rb = circle2.Curve.Radius
        rc = circle3.Curve.Radius

        # Solution using Descartes' theorem, as described here:
        # http://en.wikipedia.org/wiki/Descartes%27_theorem
        k1 = 1 / ra
        k2 = 1 / rb
        k3 = 1 / rc
        k4 = abs(k1 + k2 + k3 + 2 * math.sqrt(k1 * k2 + k2 * k3 + k3 * k1))

        q1 = (k1 + 0j) * (A.x + A.y * 1j)
        q2 = (k2 + 0j) * (B.x + B.y * 1j)
        q3 = (k3 + 0j) * (C.x + C.y * 1j)

        temp = ((q1 * q2) + (q2 * q3) + (q3 * q1))
        q4 = q1 + q2 + q3 + ((2 + 0j) * cmath.sqrt(temp) )

        z = q4 / (k4 + 0j)

        # If the formula is not solvable, we return no circle.
        if (not z or not (1 / k4)):
            return None

        X = z.real
        Y = z.imag
        print("Outer Soddy circle: " + str(X) + " " + str(Y) + "\n") # Debug

        # The Radius of the inner soddy circle can also be calculated with the following formula:
        # radiusInner = abs(r1*r2*r3 / (r1*r2 + r1*r3 + r2*r3 + 2 * math.sqrt(r1*r2*r3 * (r1+r2+r3))))
        circ = Part.Circle(Vector(X, Y, A.z), norm, 1 / k4)
        return circ

    else:
        print("debug: innerSoddyCircle bad parameters!\n")
        # FreeCAD.Console.PrintMessage("debug: innerSoddyCircle bad parameters!\n")
        return None

def circleFrom3CircleTangents(circle1, circle2, circle3):
    '''
    http://en.wikipedia.org/wiki/Problem_of_Apollonius#Inversive_methods
    http://mathworld.wolfram.com/ApolloniusCircle.html
    http://mathworld.wolfram.com/ApolloniusProblem.html
    '''

    if (geomType(circle1) == "Circle") and (geomType(circle2) == "Circle") \
    and (geomType(circle3) == "Circle"):
        int12 = findIntersection(circle1, circle2, True, True)
        int23 = findIntersection(circle2, circle3, True, True)
        int31 = findIntersection(circle3, circle1, True, True)

        if int12 and int23 and int31:
            if len(int12) == 1 and len(int23) == 1 and len(int31) == 1:
                # Only one intersection with each circle.
                # => "Soddy Circle" - 2 solutions.
                # http://en.wikipedia.org/wiki/Problem_of_Apollonius#Mutually_tangent_given_circles:_Soddy.27s_circles_and_Descartes.27_theorem
                # http://mathworld.wolfram.com/SoddyCircles.html
                # http://mathworld.wolfram.com/InnerSoddyCenter.html
                # http://mathworld.wolfram.com/OuterSoddyCenter.html

                r1 = circle1.Curve.Radius
                r2 = circle2.Curve.Radius
                r3 = circle3.Curve.Radius
                outerSoddy = outerSoddyCircle(circle1, circle2, circle3)
#               print(str(outerSoddy) + "\n") # Debug

                innerSoddy = innerSoddyCircle(circle1, circle2, circle3)
#               print(str(innerSoddy) + "\n") # Debug

                circles = []
                if outerSoddy:
                    circles.append(outerSoddy)
                if innerSoddy:
                    circles.append(innerSoddy)
                return circles

            # @todo Calc all 6 homothetic centers.
            # @todo Create 3 lines from the inner and 4 from the outer h. center.
            # @todo Calc. the 4 inversion poles of these lines for each circle.
            # @todo Calc. the radical center of the 3 circles.
            # @todo Calc. the intersection points (max. 8) of 4 lines (trough each inversion pole and the radical center) with the circle.
            #       This gives us all the tangent points.
        else:
            # Some circles are inside each other or an error has occurred.
            return None

    else:
        print("debug: circleFrom3CircleTangents bad parameters!\n")
        # FreeCAD.Console.PrintMessage("debug: circleFrom3CircleTangents bad parameters!\n")
        return None


def linearFromPoints (p1, p2):
    '''
    Calculate linear equation from points.
    Calculate the slope and offset parameters of the linear equation of a line defined by two points.

    Linear equation:
    y = m * x + b
    m = dy / dx
    m ... Slope
    b ... Offset (point where the line intersects the y axis)
    dx/dy ... Delta x and y. Using both as a vector results in a non-offset direction vector.
    '''
    if isinstance(p1, Vector) and isinstance(p2, Vector):
        line = {}
        line['dx'] = (p2.x - p1.x)
        line['dy'] = (p2.y - p1.y)
        line['slope'] = line['dy'] / line['dx']
        line['offset'] = p1.y - slope * p1.x
        return line
    else:
        return None


def determinant (mat,n):
    '''
    determinant(matrix,int) - Determinat function. Returns the determinant
    of a n-matrix. It recursively expands the minors.
    '''
    matTemp = [[0.0,0.0,0.0],[0.0,0.0,0.0],[0.0,0.0,0.0]]
    if (n > 1):
        if n == 2:
            d = mat[0][0] * mat[1][1] - mat[1][0] * mat[0][1]
        else:
            d = 0.0
            for j1 in range(n):
                # Create minor
                for i in range(1, n):
                    j2 = 0
                    for j in range(n):
                        if j == j1:
                            continue
                        matTemp[i-1][j2] = mat[i][j]
                        j2 += 1
                d += (-1.0)**(1.0 + j1 + 1.0) * mat[0][j1] * determinant(matTemp, n-1)
        return d
    else:
        return 0


def findHomotheticCenterOfCircles(circle1, circle2):
    '''
    findHomotheticCenterOfCircles(circle1, circle2)
    Calculates the homothetic center(s) of two circles.

    http://en.wikipedia.org/wiki/Homothetic_center
    http://mathworld.wolfram.com/HomotheticCenter.html
    '''

    if (geomType(circle1) == "Circle") and (geomType(circle2) == "Circle"):
        if DraftVecUtils.equals(circle1.Curve.Center, circle2.Curve.Center):
            return None

        cen1_cen2 = Part.LineSegment(circle1.Curve.Center, circle2.Curve.Center).toShape()
        cenDir = vec(cen1_cen2); cenDir.normalize()

        # Get the perpedicular vector.
        perpCenDir = cenDir.cross(Vector(0,0,1)); perpCenDir.normalize()

        # Get point on first circle
        p1 = Vector.add(circle1.Curve.Center, Vector(perpCenDir).multiply(circle1.Curve.Radius))

        centers = []
        # Calculate inner homothetic center
        # Get point on second circle
        p2_inner = Vector.add(circle1.Curve.Center, Vector(perpCenDir).multiply(-circle1.Curve.Radius))
        hCenterInner = DraftVecUtils.intersect(circle1.Curve.Center, circle2.Curve.Center, p1, p2_inner, True, True)
        if hCenterInner:
            centers.append(hCenterInner)

        # Calculate outer homothetic center (only exists of the circles have different radii)
        if circle1.Curve.Radius != circle2.Curve.Radius:
            # Get point on second circle
            p2_outer = Vector.add(circle1.Curve.Center, Vector(perpCenDir).multiply(circle1.Curve.Radius))
            hCenterOuter = DraftVecUtils.intersect(circle1.Curve.Center, circle2.Curve.Center, p1, p2_outer, True, True)
            if hCenterOuter:
                centers.append(hCenterOuter)

        if len(centers):
            return centers
        else:
            return None

    else:
        print("debug: findHomotheticCenterOfCircles bad parameters!\n")
        FreeCAD.Console.PrintMessage("debug: findHomotheticCenterOfCirclescleFrom3tan bad parameters!\n")
        return None


def findRadicalAxis(circle1, circle2):
    '''
    Calculates the radical axis of two circles.
    On the radical axis (also called power line) of two circles any
    tangents drawn from a point on the axis to both circles have the same length.

    http://en.wikipedia.org/wiki/Radical_axis
    http://mathworld.wolfram.com/RadicalLine.html

    @sa findRadicalCenter
    '''

    if (geomType(circle1) == "Circle") and (geomType(circle2) == "Circle"):
        if DraftVecUtils.equals(circle1.Curve.Center, circle2.Curve.Center):
            return None
        r1 = circle1.Curve.Radius
        r2 = circle1.Curve.Radius
        cen1 = circle1.Curve.Center
        # dist .. the distance from cen1 to cen2.
        dist = DraftVecUtils.dist(cen1, circle2.Curve.Center)
        cenDir = cen1.sub(circle2.Curve.Center); cenDir.normalize()

        # Get the perpedicular vector.
        perpCenDir = cenDir.cross(Vector(0,0,1)); perpCenDir.normalize()

        # J ... The radical center.
        # K ... The point where the cadical axis crosses the line of cen1->cen2.
        # k1 ... Distance from cen1 to K.
        # k2 ... Distance from cen2 to K.
        # dist = k1 + k2

        k1 = (dist + (r1^2 - r2^2) / dist) / 2.0
        #k2 = dist - k1

        K = Vector.add(cen1, cenDir.multiply(k1))

        # K_ .. A point somewhere between K and J (actually with a distance of 1 unit from K).
        K_ = Vector,add(K, perpCenDir)

        radicalAxis = Part.LineSegment(K, Vector.add(origin, dir))

        if radicalAxis:
            return radicalAxis
        else:
            return None
    else:
        print("debug: findRadicalAxis bad parameters!\n")
        FreeCAD.Console.PrintMessage("debug: findRadicalAxis bad parameters!\n")
        return None



def findRadicalCenter(circle1, circle2, circle3):
    '''
    findRadicalCenter(circle1, circle2, circle3):
    Calculates the radical center (also called the power center) of three circles.
    It is the intersection point of the three radical axes of the pairs of circles.

    http://en.wikipedia.org/wiki/Power_center_(geometry)
    http://mathworld.wolfram.com/RadicalCenter.html

    @sa findRadicalAxis
    '''

    if (geomType(circle1) == "Circle") and (geomType(circle2) == "Circle"):
        radicalAxis12 = findRadicalAxis(circle1, circle2)
        radicalAxis23 = findRadicalAxis(circle1, circle2)

        if not radicalAxis12 or not radicalAxis23:
            # No radical center could be calculated.
            return None

        int = findIntersection(radicalAxis12, radicalAxis23, True, True)

        if int:
            return int
        else:
            # No radical center could be calculated.
            return None
    else:
        print("debug: findRadicalCenter bad parameters!\n")
        FreeCAD.Console.PrintMessage("debug: findRadicalCenter bad parameters!\n")
        return None

def pointInversion(circle, point):
    '''
    pointInversion(Circle, Vector)

    Circle inversion of a point.
    Will calculate the inversed point an return it.
    If the given point is equal to the center of the circle "None" will be returned.

    See also:
    http://en.wikipedia.org/wiki/Inversive_geometry
    '''

    if (geomType(circle) == "Circle") and isinstance(point, FreeCAD.Vector):
        cen = circle.Curve.Center
        rad = circle.Curve.Radius

        if DraftVecUtils.equals(cen, point):
            return None

        # Inverse the distance of the point
        # dist(cen -> P) = r^2 / dist(cen -> invP)

        dist = DraftVecUtils.dist(point, cen)
        invDist = rad**2 / d

        invPoint = Vector(0, 0, point.z)
        invPoint.x = cen.x + (point.x - cen.x) * invDist / dist;
        invPoint.y = cen.y + (point.y - cen.y) * invDist / dist;

        return invPoint

    else:
        print("debug: pointInversion bad parameters!\n")
        FreeCAD.Console.PrintMessage("debug: pointInversion bad parameters!\n")
        return None

def polarInversion(circle, edge):
    '''
    polarInversion(circle, edge):
    Returns the inversion pole of a line.
    edge ... The polar.
    i.e. The nearest point on the line is inversed.

    http://mathworld.wolfram.com/InversionPole.html
    '''

    if (geomType(circle) == "Circle") and (geomType(edge) == "Line"):
        nearest = circle.Curve.Center.add(findDistance(circle.Curve.Center, edge, False))
        if nearest:
            inversionPole = pointInversion(circle, nearest)
            if inversionPole:
                return inversionPole

    else:
        print("debug: circleInversionPole bad parameters!\n")
        FreeCAD.Console.PrintMessage("debug: circleInversionPole bad parameters!\n")
        return None

def circleInversion(circle, circle2):
    '''
    pointInversion(Circle, Circle)

    Circle inversion of a circle.
    '''
    if (geomType(circle) == "Circle") and (geomType(circle2) == "Circle"):
        cen1 = circle.Curve.Center
        rad1 = circle.Curve.Radius

        if DraftVecUtils.equals(cen1, point):
            return None

        invCen2 = Inversion(circle, circle2.Curve.Center)

        pointOnCircle2 = Vector.add(circle2.Curve.Center, Vector(circle2.Curve.Radius, 0, 0))
        invPointOnCircle2 = Inversion(circle, pointOnCircle2)

        return Part.Circle(invCen2, norm, DraftVecUtils.dist(invCen2, invPointOnCircle2))

    else:
        print("debug: circleInversion bad parameters!\n")
        FreeCAD.Console.PrintMessage("debug: circleInversion bad parameters!\n")
        return None

##  @}
