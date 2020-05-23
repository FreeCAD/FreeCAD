# ***************************************************************************
# *   Copyright (c) 2009, 2010 Yorik van Havre <yorik@uncreated.net>        *
# *   Copyright (c) 2009, 2010 Ken Cline <cline@frii.com>                   *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************
"""Define geometry functions for manipulating shapes in the Draft Workbench.

These functions are used by different object creation functions
of the Draft Workbench, both in `Draft.py` and `DraftTools.py`.
They operate on the internal shapes (`Part::TopoShape`) of different objects
and on their subelements, that is, vertices, edges, and faces.
"""
## \defgroup DRAFTGEOMUTILS DraftGeomUtils
#  \ingroup UTILITIES
#  \brief Shape manipulation utilities for the Draft workbench
#
# Shapes manipulation utilities

## \addtogroup DRAFTGEOMUTILS
#  @{
import cmath
import math

import FreeCAD
import Part
import DraftVecUtils
from FreeCAD import Vector

__title__ = "FreeCAD Draft Workbench - Geometry library"
__author__ = "Yorik van Havre, Jacques-Antoine Gaudin, Ken Cline"
__url__ = ["https://www.freecadweb.org"]

NORM = Vector(0, 0, 1)  # provisory normal direction for all geometry ops.

params = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft")

# Generic functions *********************************************************


from draftgeoutils.general import precision


from draftgeoutils.general import vec


from draftgeoutils.general import edg


from draftgeoutils.general import getVerts


from draftgeoutils.general import v1


from draftgeoutils.general import isNull


from draftgeoutils.general import isPtOnEdge


from draftgeoutils.general import hasCurves


from draftgeoutils.general import isAligned


from draftgeoutils.general import getQuad


from draftgeoutils.general import areColinear


from draftgeoutils.general import hasOnlyWires


from draftgeoutils.general import geomType


from draftgeoutils.general import isValidPath


# edge functions *************************************************************


from draftgeoutils.edges import findEdge


from draftgeoutils.intersections import findIntersection


from draftgeoutils.intersections import wiresIntersect



def pocket2d(shape, offset):
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


from draftgeoutils.edges import orientEdge


def mirror(point, edge):
    """Find mirror point relative to an edge."""
    normPoint = point.add(findDistance(point, edge, False))
    if normPoint:
        normPoint_point = Vector.sub(point, normPoint)
        normPoint_refl = normPoint_point.negative()
        refl = Vector.add(normPoint, normPoint_refl)
        return refl
    else:
        return None


from draftgeoutils.arcs import isClockwise


from draftgeoutils.edges import isSameLine


from draftgeoutils.arcs import isWideAngle


def findClosest(basepoint, pointslist):
    """
    findClosest(vector,list)
    in a list of 3d points, finds the closest point to the base point.
    an index from the list is returned.
    """
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


from draftgeoutils.faces import concatenate


from draftgeoutils.faces import getBoundary


from draftgeoutils.edges import isLine


from draftgeoutils.sort_edges import sortEdges


from draftgeoutils.sort_edges import sortEdgesOld


from draftgeoutils.edges import invert


from draftgeoutils.wires import flattenWire


from draftgeoutils.wires import findWires


from draftgeoutils.wires import findWiresOld2


from draftgeoutils.wires import superWire


from draftgeoutils.edges import findMidpoint


from draftgeoutils.geometry import findPerpendicular


def offset(edge, vector, trim=False):
    """
    offset(edge,vector)
    returns a copy of the edge at a certain (vector) distance
    if the edge is an arc, the vector will be added at its first point
    and a complete circle will be returned
    """
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


from draftgeoutils.wires import isReallyClosed


from draftgeoutils.geometry import getSplineNormal


from draftgeoutils.geometry import getNormal


from draftgeoutils.geometry import getRotation


from draftgeoutils.geometry import calculatePlacement


def offsetWire(wire, dvec, bind=False, occ=False,
               widthList=None, offsetMode=None, alignList=[],
               normal=None, basewireOffset=0):  # offsetMode="BasewireMode" or None
    """
    offsetWire(wire,vector,[bind]): offsets the given wire along the given
    vector. The vector will be applied at the first vertex of the wire. If bind
    is True (and the shape is open), the original wire and the offsetted one
    are bound by 2 edges, forming a face.

        If widthList is provided (values only, not lengths - i.e. no unit),
        each value will be used to offset each corresponding edge in the wire.

        (The 1st value overrides 'dvec' for 1st segment of wire;
         if a value is zero, value of 'widthList[0]' will follow;
         if widthList[0]' == 0, but dvec still provided, dvec will be followed)

        If alignList is provided,
        each value will be used to offset each corresponding edge in the wire with corresponding index.

        OffsetWire() is now aware of width and align per edge (Primarily for use with ArchWall based on Sketch object )

        'dvec' vector to offset is now derived (and can be ignored) in this function if widthList and alignList are provided - 'dvec' to be obsolete in future ?

        'basewireOffset' corresponds to 'offset' in ArchWall which offset the basewire before creating the wall outline
    """

    # Accept 'wire' as a list of edges (use the list directly), or previously as a wire or a face (Draft Wire with MakeFace True or False supported)

    if isinstance(wire,Part.Wire) or isinstance(wire,Part.Face):
        edges = wire.Edges  # Seems has repeatedly sortEdges, remark out here - edges = Part.__sortEdges__(wire.Edges)
    elif isinstance(wire, list):
        if isinstance(wire[0],Part.Edge):
            edges = wire.copy()
            wire = Part.Wire( Part.__sortEdges__(edges) )  # How to avoid __sortEdges__ again?  Make getNormal directly tackle edges ?
    else:
        print ("Either Part.Wire or Part.Edges should be provided, returning None ")
        return None

    # For sketch with a number of wires, getNormal() may result in different direction for each wire
    # The 'normal' parameter, if provided e.g. by ArchWall, allows normal over different wires e.g. in a Sketch be consistent (over different calls of this function)
    if normal:
        norm = normal
    else:
        norm = getNormal(wire)  # norm = Vector(0, 0, 1)

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

    # Make a copy of alignList - to avoid changes in this function become starting input of next call of this function ?
    # https://www.dataquest.io/blog/tutorial-functions-modify-lists-dictionaries-python/
    # alignListC = alignList.copy()  # Only Python 3
    alignListC = list(alignList)  # Python 2 and 3

    # Check the direction / offset of starting edge
    firstDir = None
    try:
        if alignListC[0] == 'Left':
            firstDir = 1
            firstAlign = 'Left'
        elif alignListC[0] == 'Right':
            firstDir = -1
            firstAlign = 'Right'
        elif alignListC[0] == 'Center':
            firstDir = 1
            firstAlign = 'Center'
    except:
        pass  # Should no longer happen for ArchWall - as aligns are 'filled in' by ArchWall

    # If not provided by alignListC checked above, check the direction of offset in dvec (not 'align') 
    if not firstDir:  ## TODO Should check if dvec is provided or not ('legacy/backward-compatible' mode)
        if isinstance(e.Curve,Part.Circle):  # need to test against Part.Circle, not Part.ArcOfCircle
            v0 = e.Vertexes[0].Point.sub(e.Curve.Center)
        else:
            v0 = vec(e).cross(norm)
        # check against dvec provided for the offset direction - would not know if dvec is vector of width (Left/Right Align) or width/2 (Center Align)
        dvec0 = DraftVecUtils.scaleTo(v0,dvec.Length)
        if DraftVecUtils.equals(dvec0,dvec):  # if dvec0 == dvec:
            firstDir = 1  # "Left Offset" (Left Align or 'left offset' in Centre Align)
            firstAlign = 'Left'
            alignListC.append('Left')
        elif DraftVecUtils.equals(dvec0,dvec.negative()):  # elif dvec0 == dvec.negative():
            firstDir = -1  # "Right Offset" (Right Align or 'right offset' in Centre Align)
            firstAlign = 'Right'
            alignListC.append('Right')
        else:
            print (" something wrong with firstDir ")
            firstAlign = 'Left'
            alignListC.append('Left')

    for i in range(len(edges)):
        # make a copy so it do not reverse the self.baseWires edges pointed to by _Wall.getExtrusionData() ?
        curredge = edges[i].copy()

        # record first edge's Orientation, Dir, Align and set Delta
        if i == 0:
            firstOrientation = curredge.Vertexes[0].Orientation			# TODO Could be edge.Orientation in fact	# "Forward" or "Reversed"
            curOrientation = firstOrientation
            curDir = firstDir
            curAlign = firstAlign
            delta = dvec

        # record current edge's Orientation, and set Delta
        if i != 0:  #else:
            if isinstance(curredge.Curve,Part.Circle):				# TODO Should also calculate 1st edge direction above
                delta = curredge.Vertexes[0].Point.sub(curredge.Curve.Center)
            else:
                delta = vec(curredge).cross(norm)
            curOrientation = curredge.Vertexes[0].Orientation			# TODO Could be edge.Orientation in fact

        # Consider individual edge width
        if widthList:  # ArchWall should now always provide widthList
            try:
                if widthList[i] > 0:
                    delta = DraftVecUtils.scaleTo(delta, widthList[i])
                elif dvec:
                    delta = DraftVecUtils.scaleTo(delta, dvec.Length)
                else:
                    #just hardcoded default value as ArchWall would provide if dvec is not provided either
                    delta = DraftVecUtils.scaleTo(delta, 200)
            except:
                if dvec:
                    delta = DraftVecUtils.scaleTo(delta, dvec.Length)
                else:
                    #just hardcoded default value as ArchWall would provide if dvec is not provided either
                    delta = DraftVecUtils.scaleTo(delta, 200)
        else:
            delta = DraftVecUtils.scaleTo(delta,dvec.Length)

        # Consider individual edge Align direction - ArchWall should now always provide alignList
        if i == 0:
            if alignListC[0] == 'Center':
                delta = DraftVecUtils.scaleTo(delta, delta.Length/2)
            # No need to do anything for 'Left' and 'Right' as original dvec have set both the direction and amount of offset correct
            # elif alignListC[i] == 'Left':  #elif alignListC[i] == 'Right':
        if i != 0:
            try:
                if alignListC[i] == 'Left':
                    curDir = 1
                    curAlign = 'Left'
                elif alignListC[i] == 'Right':
                    curDir = -1
                    curAlign = 'Right'
                    delta = delta.negative()
                elif alignListC[i] == 'Center':
                    curDir = 1
                    curAlign = 'Center'
                    delta = DraftVecUtils.scaleTo(delta, delta.Length/2)
            except:
                curDir = firstDir
                curAlign = firstAlign
                if firstAlign == 'Right':
                    delta = delta.negative()
                elif firstAlign == 'Center':
                    delta = DraftVecUtils.scaleTo(delta, delta.Length/2)

        # Consider whether generating the 'offset wire' or the 'base wire'
        if offsetMode is None:
            # Consider if curOrientation and/or curDir match their firstOrientation/firstDir - to determine whether and how to offset the current edge 
            if (curOrientation == firstOrientation) != (curDir == firstDir):	# i.e. xor
                if curAlign in ['Left', 'Right']:
                    nedge = curredge
                elif curAlign == 'Center':
                    delta = delta.negative()
                    nedge = offset(curredge,delta,trim=True)
            else:
                # if curAlign in ['Left', 'Right']: # elif curAlign == 'Center': # Both conditions same result..
                if basewireOffset:  # ArchWall has an Offset properties for user to offset the basewire before creating the base profile of wall (not applicable to 'Center' align)
                    delta = DraftVecUtils.scaleTo(delta, delta.Length+basewireOffset)
                nedge = offset(curredge,delta,trim=True)

            if curOrientation == "Reversed":  # TODO arc always in counter-clockwise directinon ... ( not necessarily 'reversed')
                if not isinstance(curredge.Curve,Part.Circle):  # need to test against Part.Circle, not Part.ArcOfCircle
                    # if not arc/circle, assume straight line, reverse it
                    nedge = Part.Edge(nedge.Vertexes[1],nedge.Vertexes[0])
                else:
                    # if arc/circle 
                    #Part.ArcOfCircle(edge.Curve, edge.FirstParameter,edge.LastParameter,edge.Curve.Axis.z>0)
                    midParameter = nedge.FirstParameter + (nedge.LastParameter - nedge.FirstParameter)/2
                    midOfArc = nedge.valueAt(midParameter)
                    nedge = Part.ArcOfCircle(nedge.Vertexes[1].Point, midOfArc, nedge.Vertexes[0].Point).toShape()
                    # TODO any better solution than to calculate midpoint of arc to reverse ?

        elif offsetMode in ["BasewireMode"]:
            if not ( (curOrientation == firstOrientation) != (curDir == firstDir) ):
                if curAlign in ['Left', 'Right']:
                    if basewireOffset:  # ArchWall has an Offset properties for user to offset the basewire before creating the base profile of wall (not applicable to 'Center' align)
                        delta = DraftVecUtils.scaleTo(delta, basewireOffset)
                        nedge = offset(curredge,delta,trim=True)
                    else:
                        nedge = curredge
                elif curAlign == 'Center':
                    delta = delta.negative()
                    nedge = offset(curredge,delta,trim=True)
            else:
                if curAlign in ['Left', 'Right']:
                    if basewireOffset:  # ArchWall has an Offset properties for user to offset the basewire before creating the base profile of wall (not applicable to 'Center' align)
                        delta = DraftVecUtils.scaleTo(delta, delta.Length+basewireOffset)
                    nedge = offset(curredge,delta,trim=True)

                elif curAlign == 'Center':
                    nedge = offset(curredge,delta,trim=True)
            if curOrientation == "Reversed":
                if not isinstance(curredge.Curve,Part.Circle):  # need to test against Part.Circle, not Part.ArcOfCircle
                    # if not arc/circle, assume straight line, reverse it
                    nedge = Part.Edge(nedge.Vertexes[1],nedge.Vertexes[0])
                else:
                    # if arc/circle
                    #Part.ArcOfCircle(edge.Curve, edge.FirstParameter,edge.LastParameter,edge.Curve.Axis.z>0)
                    midParameter = nedge.FirstParameter + (nedge.LastParameter - nedge.FirstParameter)/2
                    midOfArc = nedge.valueAt(midParameter)
                    nedge = Part.ArcOfCircle(nedge.Vertexes[1].Point, midOfArc, nedge.Vertexes[0].Point).toShape()
                    # TODO any better solution than to calculate midpoint of arc to reverse ?
        else:
            print(" something wrong ")
            return
        if not nedge:
            return None
        nedges.append(nedge)

    if len(edges) >1:
        nedges = connect(nedges,closed)
    else:
        nedges = Part.Wire(nedges[0])

    if bind and not closed:
        e1 = Part.LineSegment(edges[0].Vertexes[0].Point,nedges[0].Vertexes[0].Point).toShape()
        e2 = Part.LineSegment(edges[-1].Vertexes[-1].Point,nedges[-1].Vertexes[-1].Point).toShape()
        alledges = edges.extend(nedges)
        alledges = alledges.extend([e1,e2])
        w = Part.Wire(alledges)
        return w
    else:
        return nedges


from draftgeoutils.intersections import connect


from draftgeoutils.geometry import findDistance


def angleBisection(edge1, edge2):
    """angleBisection(edge,edge) - Returns an edge that bisects the angle between the 2 edges."""
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

def findClosestCircle(point, circles):
    """Return the circle with closest center."""
    dist = 1000000
    closest = None
    for c in circles:
        if c.Center.sub(point).Length < dist:
            dist = c.Center.sub(point).Length
            closest = c
    return closest


from draftgeoutils.faces import isCoplanar


from draftgeoutils.geometry import isPlanar


from draftgeoutils.wires import findWiresOld


def getTangent(edge, frompoint=None):
        """
        returns the tangent to an edge. If from point is given, it is used to
        calculate the tangent (only useful for an arc of course).
        """
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


from draftgeoutils.faces import bind


from draftgeoutils.faces import cleanFaces


def isCubic(shape):
    """isCubic(shape): verifies if a shape is cubic, that is, has
    8 vertices, 6 faces, and all angles are 90 degrees."""
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
    """getCubicDimensions(shape): returns a list containing the placement,
    the length, the width and the height of a cubic shape. If not cubic, nothing
    is returned. The placement point is the lowest corner of the shape."""
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
        """removeInterVertices(wire) - remove unneeded vertices (those that
        are in the middle of a straight line) from a wire, returns a new wire."""
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


def fillet(lEdges, r, chamfer=False):
    """fillet(lEdges,r,chamfer=False): Take a list of two Edges & a float as argument,
    Returns a list of sorted edges describing a round corner"""
    # Fillet code graciously donated by Jacques-Antoine Gaudin

    def getCurveType(edge, existingCurveType=None):
            """Builds or completes a dictionary containing edges with keys "Arc" and 'Line'"""
            if not existingCurveType:
                    existingCurveType = { 'Line' : [], 'Arc' : [] }
            if issubclass(type(edge.Curve),Part.LineSegment):
                    existingCurveType['Line'] += [edge]
            elif issubclass(type(edge.Curve),Part.Line):
                    existingCurveType['Line'] += [edge]
            elif issubclass(type(edge.Curve),Part.Circle):
                    existingCurveType['Arc']  += [edge]
            else:
                    raise ValueError("Edge's curve must be either Line or Arc")
            return existingCurveType

    rndEdges = lEdges[0:2]
    rndEdges = Part.__sortEdges__(rndEdges)

    if len(rndEdges) < 2:
        return rndEdges

    if r <= 0:
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
        # Deals with lists containing an arc and a line ----------------------
        if lEdges[0] in curveType['Arc']:
            lineEnd = lVertexes[2] ; arcEnd = lVertexes[0] ; arcFirst = True
        else:
            lineEnd = lVertexes[0] ; arcEnd = lVertexes[2] ; arcFirst = False
        arcCenter = curveType['Arc'][0].Curve.Center
        arcRadius = curveType['Arc'][0].Curve.Radius
        arcAxis   = curveType['Arc'][0].Curve.Axis
        arcLength = curveType['Arc'][0].Length

        U1 = lineEnd.Point.sub(lVertexes[1].Point) ; U1.normalize()
        toCenter = arcCenter.sub(lVertexes[1].Point)
        if arcFirst : # make sure the tangent points towards the arc
            T = arcAxis.cross(toCenter)
        else:
            T = toCenter.cross(arcAxis)

        projCenter = toCenter.dot(U1)
        if round(abs(projCenter),precision()) > 0:
            normToLine = U1.cross(T).cross(U1)
        else:
            normToLine = Vector(toCenter)
        normToLine.normalize()

        dCenterToLine = toCenter.dot(normToLine) - r

        if  round(projCenter,precision()) > 0:
            newRadius = arcRadius - r
        elif round(projCenter,precision()) < 0 or (round(projCenter,precision()) == 0 and U1.dot(T) > 0):
            newRadius = arcRadius + r
        else:
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
        # Deals with lists of 2 arc-edges -----------------------------------
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
            else:
                print("DraftGeomUtils.fillet : Warning : edges are already tangent. Did nothing")
                return rndEdges
        elif not sameDirection:
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


def filletWire(aWire, r, chamfer=False):
    """Fillets each angle of a wire with r as radius value
    if chamfer is true, a chamfer is made instead and r is the
    size of the chamfer"""

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
    """Return a circle-based edge from a bspline-based edge."""
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


from draftgeoutils.wires import curvetowire


def cleanProjection(shape, tessellate=True, seglength=0.05):
    """Return a valid compound of edges, by recreating them."""
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


from draftgeoutils.wires import curvetosegment


def tessellateProjection(shape, seglen):
    """Returns projection with BSplines and Ellipses broken into line segments.
        Useful for exporting projected views to *dxf files."""
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


from draftgeoutils.wires import rebaseWire


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


def getBoundaryAngles(angle, alist):
        """returns the 2 closest angles from the list that
        encompass the given angle"""
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
                        if lower is None:
                                lower = a
                        else:
                                if a > lower:
                                        lower = a
        if lower is None:
                lower = 0
                for a in alist:
                        if a > lower:
                                lower = a
        higher = None
        for a in alist:
                if a > angle:
                        if higher is None:
                                higher = a
                        else:
                                if a < higher:
                                        higher = a
        if higher is None:
                higher = 2*math.pi
                for a in alist:
                        if a < higher:
                                higher = a
        return (lower,higher)


def circleFrom2tan1pt(tan1, tan2, point):
    """circleFrom2tan1pt(edge, edge, Vector)"""
    if (geomType(tan1) == "Line") and (geomType(tan2) == "Line") and isinstance(point, FreeCAD.Vector):
        return circlefrom2Lines1Point(tan1, tan2, point)
    elif (geomType(tan1) == "Circle") and (geomType(tan2) == "Line") and isinstance(point, FreeCAD.Vector):
        return circlefromCircleLinePoint(tan1, tan2, point)
    elif (geomType(tan2) == "Circle") and (geomType(tan1) == "Line") and isinstance(point, FreeCAD.Vector):
        return circlefromCircleLinePoint(tan2, tan1, point)
    elif (geomType(tan2) == "Circle") and (geomType(tan1) == "Circle") and isinstance(point, FreeCAD.Vector):
        return circlefrom2Circles1Point(tan2, tan1, point)


def circleFrom2tan1rad(tan1, tan2, rad):
    """circleFrom2tan1rad(edge, edge, float)"""
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
    """circlefrom2Lines1Point(edge, edge, Vector)"""
    bis = angleBisection(edge1, edge2)
    if not bis: return None
    mirrPoint = mirror(point, bis)
    return circlefrom1Line2Points(edge1, point, mirrPoint)


def circlefrom1Line2Points(edge, p1, p2):
    """circlefrom1Line2Points(edge, Vector, Vector)"""
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


def circleFrom2LinesRadius(edge1, edge2, radius):
    """circleFrom2LinesRadius(edge,edge,radius)"""
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


def circleFrom3LineTangents(edge1, edge2, edge3):
    """circleFrom3LineTangents(edge,edge,edge)"""
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

def circleFromPointLineRadius(point, edge, radius):
    """circleFromPointLineRadius (point, edge, radius)"""
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
    """circleFrom2PointsRadiust(Vector, Vector, radius)"""
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


from draftgeoutils.arcs import arcFrom2Pts


#############################33 to include







def outerSoddyCircle(circle1, circle2, circle3):
    """Compute the outer soddy circle for three tightly packed circles."""
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
    """Compute the inner soddy circle for three tightly packed circles."""
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
    """
    http://en.wikipedia.org/wiki/Problem_of_Apollonius#Inversive_methods
    http://mathworld.wolfram.com/ApolloniusCircle.html
    http://mathworld.wolfram.com/ApolloniusProblem.html
    """

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
            # @todo Calc. the intersection points (max. 8) of 4 lines (through each inversion pole and the radical center) with the circle.
            #       This gives us all the tangent points.
        else:
            # Some circles are inside each other or an error has occurred.
            return None

    else:
        print("debug: circleFrom3CircleTangents bad parameters!\n")
        # FreeCAD.Console.PrintMessage("debug: circleFrom3CircleTangents bad parameters!\n")
        return None


def linearFromPoints(p1, p2):
    """Calculate linear equation from points.

    Calculate the slope and offset parameters of the linear equation of a line defined by two points.

    Linear equation:
    y = m * x + b
    m = dy / dx
    m ... Slope
    b ... Offset (point where the line intersects the y axis)
    dx/dy ... Delta x and y. Using both as a vector results in a non-offset direction vector.
    """
    if isinstance(p1, Vector) and isinstance(p2, Vector):
        line = {}
        line['dx'] = (p2.x - p1.x)
        line['dy'] = (p2.y - p1.y)
        line['slope'] = line['dy'] / line['dx']
        line['offset'] = p1.y - slope * p1.x
        return line
    else:
        return None


def determinant(mat, n):
    """
    determinant(matrix,int) - Determinat function. Returns the determinant
    of a n-matrix. It recursively expands the minors.
    """
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
    """Calculate the homothetic center(s) of two circles.

    http://en.wikipedia.org/wiki/Homothetic_center
    http://mathworld.wolfram.com/HomotheticCenter.html
    """

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
        FreeCAD.Console.PrintMessage("debug: findHomotheticCenterOfCirclescleFrom3tan bad parameters!\n")
        return None


def findRadicalAxis(circle1, circle2):
    """Calculate the radical axis of two circles.

    On the radical axis (also called power line) of two circles any
    tangents drawn from a point on the axis to both circles have the same length.

    http://en.wikipedia.org/wiki/Radical_axis
    http://mathworld.wolfram.com/RadicalLine.html

    @sa findRadicalCenter
    """
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
        FreeCAD.Console.PrintMessage("debug: findRadicalAxis bad parameters!\n")
        return None


def findRadicalCenter(circle1, circle2, circle3):
    """
    findRadicalCenter(circle1, circle2, circle3):
    Calculates the radical center (also called the power center) of three circles.
    It is the intersection point of the three radical axes of the pairs of circles.

    http://en.wikipedia.org/wiki/Power_center_(geometry)
    http://mathworld.wolfram.com/RadicalCenter.html

    @sa findRadicalAxis
    """
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
        FreeCAD.Console.PrintMessage("debug: findRadicalCenter bad parameters!\n")
        return None


def pointInversion(circle, point):
    """Circle inversion of a point.

    pointInversion(Circle, Vector)

    Will calculate the inversed point an return it.
    If the given point is equal to the center of the circle "None" will be returned.

    See also:
    http://en.wikipedia.org/wiki/Inversive_geometry
    """
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
        FreeCAD.Console.PrintMessage("debug: pointInversion bad parameters!\n")
        return None


def polarInversion(circle, edge):
    """Return the inversion pole of a line.

    polarInversion(circle, edge):

    edge ... The polar.
    i.e. The nearest point on the line is inversed.

    http://mathworld.wolfram.com/InversionPole.html
    """

    if (geomType(circle) == "Circle") and (geomType(edge) == "Line"):
        nearest = circle.Curve.Center.add(findDistance(circle.Curve.Center, edge, False))
        if nearest:
            inversionPole = pointInversion(circle, nearest)
            if inversionPole:
                return inversionPole
    else:
        FreeCAD.Console.PrintMessage("debug: circleInversionPole bad parameters!\n")
        return None


def circleInversion(circle, circle2):
    """
    pointInversion(Circle, Circle)

    Circle inversion of a circle.
    """
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
        FreeCAD.Console.PrintMessage("debug: circleInversion bad parameters!\n")
        return None

##  @}
