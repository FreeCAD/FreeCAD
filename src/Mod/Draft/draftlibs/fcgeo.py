#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2009, 2010                                              *
#*   Yorik van Havre <yorik@gmx.fr>, Ken Cline <cline@frii.com>            *  
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
__url__ = ["http://free-cad.sourceforge.net"]

"this file contains generic geometry functions for manipulating Part shapes"

import FreeCAD, Part, fcvec, math, cmath, FreeCADGui
from FreeCAD import Vector

NORM = Vector(0,0,1) # provisory normal direction for all geometry ops.

params = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft")
precision = params.GetInt("precision")

# Generic functions *********************************************************


def vec(edge):
	"vec(edge) or vec(line) -- returns a vector from an edge or a Part.line"
	# if edge is not straight, you'll get strange results!
	if isinstance(edge,Part.Shape):
		return edge.Vertexes[-1].Point.sub(edge.Vertexes[0].Point)
	elif isinstance(edge,Part.Line):
		return edge.EndPoint.sub(edge.StartPoint)
	else:
		return None

def edg(p1,p2):
	"edg(Vector,Vector) -- returns an edge from 2 vectors"
	if isinstance(p1,FreeCAD.Vector) and isinstance(p2,FreeCAD.Vector):
		if fcvec.equals(p1,p2): return None
		else: return Part.Line(p1,p2).toShape()

def getVerts(shape):
        "getVerts(shape) -- returns a list containing vectors of each vertex of the shape"
        p = []
        for v in shape.Vertexes:
                p.append(v.Point)
        return p

def v1(edge):
	"v1(edge) -- returns the first point of an edge"
	return edge.Vertexes[0].Point

def isNull(something):
        '''returns true if the given shape is null or the given placement is 0 or
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
	'''isPtOnEdge(Vector,edge) -- Tests if a point is on an edge'''
	if isinstance(edge.Curve,Part.Line) :
		orig = edge.Vertexes[0].Point
		if fcvec.isNull(pt.sub(orig).cross(vec(edge))) :
			return pt.sub(orig).Length <= vec(edge).Length and pt.sub(orig).dot(vec(edge)) >= 0
		else : 
			return False
		
	elif isinstance(edge.Curve,Part.Circle) :
		center = edge.Curve.Center
		axis   = edge.Curve.Axis ; axis.normalize()
		radius = edge.Curve.Radius
		if  round(pt.sub(center).dot(axis),precision) == 0 \
		and round(pt.sub(center).Length - radius,precision) == 0 :
			if len(edge.Vertexes) == 1 :
				return True # edge is a complete circle
			else :
				begin = edge.Vertexes[0].Point
				end   = edge.Vertexes[-1].Point
				if fcvec.isNull(pt.sub(begin)) or fcvec.isNull(pt.sub(end)) :
					return True
				else :
					# newArc = Part.Arc(begin,pt,end)
					# return fcvec.isNull(newArc.Center.sub(center)) \
					#    and fcvec.isNull(newArc.Axis-axis) \
					#    and round(newArc.Radius-radius,precision) == 0
					angle1 = fcvec.angle(begin.sub(center))
					angle2 = fcvec.angle(end.sub(center))
					anglept = fcvec.angle(pt.sub(center))
					if (angle1 < anglept) and (anglept < angle2):
						return True
	return False

def hasCurves(shape):
        "checks if the given shape has curves"
        for e in shape.Edges:
                if not isinstance(e.Curve,Part.Line):
                        return True
        return False

def isAligned(edge,axis="x"):
        "checks if the given edge or line is aligned to the given axis (x, y or z)"
        if axis == "x":
                if isinstance(edge,Part.Edge):
                        if len(edge.Vertexes) == 2:
                                if edge.Vertexes[0].X == edge.Vertexes[-1].X:
                                        return True
                elif isinstance(edge,Part.Line):
                        if edge.StartPoint.x == edge.EndPoint.x:
                                        return True
        elif axis == "y":
                if isinstance(edge,Part.Edge):
                        if len(edge.Vertexes) == 2:
                                if edge.Vertexes[0].Y == edge.Vertexes[-1].Y:
                                        return True
                elif isinstance(edge,Part.Line):
                        if edge.StartPoint.y == edge.EndPoint.y:
                                        return True
        elif axis == "z":
                if isinstance(edge,Part.Edge):
                        if len(edge.Vertexes) == 2:
                                if edge.Vertexes[0].Z == edge.Vertexes[-1].Z:
                                        return True
                elif isinstance(edge,Part.Line):
                        if edge.StartPoint.z == edge.EndPoint.z:
                                        return True
        return False

def hasOnlyWires(shape):
        "returns True if all the edges are inside a wire"
        ne = 0
        for w in shape.Wires:
                ne += len(w.Edges)
        if ne == len(shape.Edges):
                return True
        return False
                        
# edge functions *****************************************************************

def findEdge(anEdge,aList):
        '''findEdge(anEdge,aList): returns True if anEdge is found in aList of edges'''
        for e in range(len(aList)):
                if str(anEdge.Curve) == str(aList[e].Curve):
                        if fcvec.equals(anEdge.Vertexes[0].Point,aList[e].Vertexes[0].Point):
                                if fcvec.equals(anEdge.Vertexes[-1].Point,aList[e].Vertexes[-1].Point):
                                        return(e)
        return None
	

def findIntersection(edge1,edge2,infinite1=False,infinite2=False,ex1=False,ex2=False) :
	'''findIntersection(edge1,edge2,infinite1=False,infinite2=False):
	returns a list containing the intersection point(s) of 2 edges.
        You can also feed 4 points instead of edge1 and edge2'''

        pt1 = None
	
        if isinstance(edge1,FreeCAD.Vector) and isinstance(edge2,FreeCAD.Vector):
                # we got points directly
                pt1 = edge1
                pt2 = edge2
                pt3 = infinite1
                pt4 = infinite2
                infinite1 = ex1
                infinite2 = ex2

        elif isinstance(edge1.Curve,Part.Line) and isinstance(edge2.Curve,Part.Line) :	
		# we have 2 edges	
		pt1, pt2, pt3, pt4 = [edge1.Vertexes[0].Point,
                                      edge1.Vertexes[1].Point,
                                      edge2.Vertexes[0].Point,
                                      edge2.Vertexes[1].Point]

        if pt1:
                # first check if we don't already have coincident endpoints
                if (pt1 in [pt3,pt4]):
                        return [pt1]
                elif (pt2 in [pt3,pt4]):
                        return [pt2]
                
                #we have 2 straight lines		  
		if fcvec.isNull(pt2.sub(pt1).cross(pt3.sub(pt1)).cross(pt2.sub(pt4).cross(pt3.sub(pt4)))):
			vec1 = pt2.sub(pt1) ; vec2 = pt4.sub(pt3)
                        if fcvec.isNull(vec1) or fcvec.isNull(vec2):
                                return []
			vec1.normalize()  ; vec2.normalize()
			cross = vec1.cross(vec2)
			if not fcvec.isNull(cross) :
				k = ((pt3.z-pt1.z)*(vec2.x-vec2.y)+(pt3.y-pt1.y)*(vec2.z-vec2.x)+ \
					 (pt3.x-pt1.x)*(vec2.y-vec2.z))/(cross.x+cross.y+cross.z)
				vec1.scale(k,k,k)
				int = pt1.add(vec1)
				
				if  infinite1 == False and not isPtOnEdge(int,edge1) :
					return []
					
				if  infinite2 == False and not isPtOnEdge(int,edge2) :
					return []
					
				return [int]
			else :
				return [] # Lines have same direction
		else :
			return [] # Lines aren't on same plane
			
	elif isinstance(edge1.Curve,Part.Circle) and isinstance(edge2.Curve,Part.Line) \
	  or isinstance(edge1.Curve,Part.Line)   and isinstance(edge2.Curve,Part.Circle) :
	  	
	  	# deals with an arc or circle and a line
	  	
		edges = [edge1,edge2]
		for edge in edges :
			if isinstance(edge.Curve,Part.Line) :
				line = edge
			else :
				arc  = edge
				
		dirVec = vec(line) ; dirVec.normalize()
		pt1    = line.Vertexes[0].Point
		pt2    = line.Vertexes[1].Point
                pt3    = arc.Vertexes[0].Point
                pt4    = arc.Vertexes[-1].Point
		center = arc.Curve.Center

                # first check for coincident endpoints
                if (pt1 in [pt3,pt4]):
                        return [pt1]
                elif (pt2 in [pt3,pt4]):
                        return [pt2]
                
		if fcvec.isNull(pt1.sub(center).cross(pt2.sub(center)).cross(arc.Curve.Axis)) :
			# Line and Arc are on same plane
			
			dOnLine = center.sub(pt1).dot(dirVec)
			onLine  = Vector(dirVec) ; onLine.scale(dOnLine,dOnLine,dOnLine)
			toLine  = pt1.sub(center).add(onLine)
			
			if toLine.Length < arc.Curve.Radius :
				dOnLine = (arc.Curve.Radius**2 - toLine.Length**2)**(0.5)
				onLine  = Vector(dirVec) ; onLine.scale(dOnLine,dOnLine,dOnLine)
				int = [center.add(toLine).add(onLine)]
				onLine  = Vector(dirVec) ; onLine.scale(-dOnLine,-dOnLine,-dOnLine)
				int += [center.add(toLine).add(onLine)]
			elif round(toLine.Length-arc.Curve.Radius,precision) == 0 :
				int = [center.add(toLine)]
			else :
				return []
				
		else : # Line isn't on Arc's plane
			if dirVec.dot(arc.Curve.Axis) != 0 :
				toPlane  = Vector(arc.Curve.Axis) ; toPlane.normalize()
				d = pt1.dot(toPlane)
				dToPlane = center.sub(pt1).dot(toPlane)
				toPlane = Vector(pt1)
				toPlane.scale(dToPlane/d,dToPlane/d,dToPlane/d)
				ptOnPlane = toPlane.add(pt1)
				if round(ptOnPlane.sub(center).Length - arc.Curve.Radius,precision) == 0 :
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
		
	elif isinstance(edge1.Curve,Part.Circle) and isinstance(edge2.Curve,Part.Circle) :
		
		# deals with 2 arcs or circles

		cent1, cent2 = edge1.Curve.Center, edge2.Curve.Center
		rad1 , rad2  = edge1.Curve.Radius, edge2.Curve.Radius
		axis1, axis2 = edge1.Curve.Axis  , edge2.Curve.Axis
		c2c          = cent2.sub(cent1)
		
		if fcvec.isNull(axis1.cross(axis2)) :
			if round(c2c.dot(axis1),precision) == 0 :
				# circles are on same plane
				dc2c = c2c.Length ;
				if not fcvec.isNull(c2c): c2c.normalize()
				if round(rad1+rad2-dc2c,precision) < 0 \
				or round(rad1-dc2c-rad2,precision) > 0 or round(rad2-dc2c-rad1,precision) > 0 :
					return []
				else :
					norm = c2c.cross(axis1)
					if not fcvec.isNull(norm): norm.normalize()
					if fcvec.isNull(norm): x = 0
					else: x = (dc2c**2 + rad1**2 - rad2**2)/(2*dc2c)
					y = abs(rad1**2 - x**2)**(0.5)
					c2c.scale(x,x,x)
					if round(y,precision) != 0 :
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
				if round(pt.sub(cent2).Length-rad2,precision) == 0 :
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
	else :
		print "fcgeo: Unsupported curve type: (" + str(edge1.Curve) + ", " + str(edge2.Curve) + ")"

def geom(edge):
        "returns a Line, ArcOfCircle or Circle geom from the given edge"
        if isinstance(edge.Curve,Part.Line):
                return edge.Curve
        elif isinstance(edge.Curve,Part.Circle):
                if len(edge.Vertexes) == 1:
                        return Part.Circle(edge.Curve.Center,edge.Curve.Axis,edge.Curve.Radius)
                else:
                        ref = edge.Placement.multVec(Vector(1,0,0))
                        v1 = edge.Vertexes[0].Point
                        v2 = edge.Vertexes[-1].Point
                        c = edge.Curve.Center
                        cu = Part.Circle(edge.Curve.Center,edge.Curve.Axis,edge.Curve.Radius)
                        a1 = -fcvec.angle(v1.sub(c),ref)
                        a2 = -fcvec.angle(v2.sub(c),ref)
                        p= Part.ArcOfCircle(cu,a1,a2)
                        return p
        else:
                return edge.Curve

def mirror (point, edge):
	"finds mirror point relative to an edge"
	normPoint = point.add(findDistance(point, edge, False))
	if normPoint:
		normPoint_point = Vector.sub(point, normPoint)
		normPoint_refl = fcvec.neg(normPoint_point)
		refl = Vector.add(normPoint, normPoint_refl)
		return refl
	else:
		return None

def findClosest(basepoint,pointslist):
	'''
	findClosest(vector,list)
	in a list of 3d points, finds the closest point to the base point.
	an index from the list is returned.
	'''
	if not pointslist: return None
	smallest = 100000
	for n in range(len(pointslist)):
		new = basepoint.sub(pointslist[n]).Length
		if new < smallest:
			smallest = new
			npoint = n
	return npoint

def concatenate(shape):
	"concatenate(shape) -- turns several faces into one"
	edges = getBoundary(shape)
	edges = sortEdges(edges)
	try:
		wire=Part.Wire(edges)
		face=Part.Face(wire)
	except:
		print "fcgeo: Couldn't join faces into one"
		return(shape)
	else:
		if not wire.isClosed():	return(wire)
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
			if lut.has_key(hc): lut[hc]=lut[hc]+1
			else: lut[hc]=1
	# filter out the edges shared by more than one sub-face
	bound=[]
	for e in shape.Edges:
		if lut[e.hashCode()] == 1: bound.append(e)
	return bound

def sortEdges(lEdges, aVertex=None):
        "an alternative, more accurate version of Part.__sortEdges__"

        #There is no reason to limit this to lines only because every non-closed edge always
        #has exactly two vertices (wmayer)
        #for e in lEdges:
        #        if not isinstance(e.Curve,Part.Line):
        #                print "Warning: sortedges cannot treat wired containing curves yet."
        #                return lEdges
	
	def isSameVertex(V1, V2):
		''' Test if vertexes have same coordinates with precision 10E(-precision)'''
		if round(V1.X-V2.X,1)==0 and round(V1.Y-V2.Y,1)==0 and round(V1.Z-V2.Z,1)==0 :
			return True
		else :
			return False
	
	def lookfor(aVertex, inEdges):
		''' Look for (aVertex, inEdges) returns count, the position of the instance
			the position in the instance and the instance of the Edge'''
		count = 0
		linstances = [] #lists the instances of aVertex
		for i in range(len(inEdges)) :
			for j in range(2) :
				if isSameVertex(aVertex,inEdges[i].Vertexes[j-1]):
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
                                if isSameVertex(aVertex,result[3].Vertexes[0]):
                                        return lEdges
                                else:
                                        if isinstance(result[3].Curve,Part.Line):
                                                return [Part.Line(aVertex.Point,result[3].Vertexes[0].Point).toShape()]
                                        elif isinstance(result[3].Curve,Part.Circle):
                                                mp = findMidpoint(result[3])
                                                return [Part.Arc(aVertex.Point,mp,result[3].Vertexes[0].Point).toShape()]
                                        else:
                                                return lEdges
                                        
	olEdges = [] # ol stands for ordered list 
	if aVertex == None:
		for i in range(len(lEdges)*2) :
			if len(lEdges[i/2].Vertexes) > 1:
				result = lookfor(lEdges[i/2].Vertexes[i%2],lEdges)
				if result[0] == 1 :  # Have we found an end ?
					olEdges = sortEdges(lEdges, result[3].Vertexes[result[2]])
					return olEdges
		# if the wire is closed there is no end so choose 1st Vertex
		return sortEdges(lEdges, lEdges[0].Vertexes[0]) 
	else :
		result = lookfor(aVertex,lEdges)
		if result[0] != 0 :
			del lEdges[result[1]]
			next = sortEdges(lEdges, result[3].Vertexes[-((-result[2])^1)])
                        if isSameVertex(aVertex,result[3].Vertexes[0]):
                                olEdges += [result[3]] + next
                        else:
                                if isinstance(result[3].Curve,Part.Line):
                                        newedge = Part.Line(aVertex.Point,result[3].Vertexes[0].Point).toShape()
                                        olEdges += [newedge] + next
                                elif isinstance(result[3].Curve,Part.Circle):
                                        mp = findMidpoint(result[3])
                                        newedge = Part.Arc(aVertex.Point,mp,result[3].Vertexes[0].Point).toShape()
                                        olEdges += [newedge] + next
                                else:
                                        olEdges += [result[3]] + next                                        
			return olEdges
		else :
			return []

                
def superWire(edgeslist,closed=False):
        '''superWire(edges,[closed]): forces a wire between edges that don't necessarily
        have coincident endpoints. If closed=True, wire will always be closed'''
        def median(v1,v2):
                vd = v2.sub(v1)
                vd.scale(.5,.5,.5)
                return v1.add(vd)
        edges = sortEdges(edgeslist)
        print edges
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
                print i,prev,curr,next
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
                if isinstance(curr.Curve,Part.Line):
                        print "line",p1,p2
                        newedges.append(Part.Line(p1,p2).toShape())
                elif isinstance(curr.Curve,Part.Circle):
                        p3 = findMidpoint(curr)
                        print "arc",p1,p3,p2
                        newedges.append(Part.Arc(p1,p3,p2).toShape())
                else:
                        print "Cannot superWire edges that are not lines or arcs"
                        return None
        print newedges
        return Part.Wire(newedges)

def findMidpoint(edge):
	"calculates the midpoint of an edge"
	first = edge.Vertexes[0].Point
	last = edge.Vertexes[-1].Point
	if isinstance(edge.Curve,Part.Circle):
                center = edge.Curve.Center
		radius = edge.Curve.Radius
                if len(edge.Vertexes) == 1:
                        # Circle
                        dv = first.sub(center)
                        dv = fcvec.neg(dv)
                        return center.add(dv)
		axis = edge.Curve.Axis
		chord = last.sub(first)
		perp = chord.cross(axis)
		perp.normalize()
		ray = first.sub(center)
		apothem = ray.dot(perp)
		sagitta = radius - apothem
		startpoint = Vector.add(first, fcvec.scale(chord,0.5))
		endpoint = fcvec.scaleTo(perp,sagitta)
		return Vector.add(startpoint,endpoint)

	elif isinstance(edge.Curve,Part.Line):
		halfedge = fcvec.scale(last.sub(first),.5)
		return Vector.add(first,halfedge)

	else:
		return None

def complexity(obj):
	'''
	tests given object for shape complexity:
	1: line
	2: arc
	3: circle
	4: open wire with no arc
	5: closed wire
	6: wire with arcs
	7: faces
	8: faces with arcs
	'''
	shape = obj.Shape
	if shape.Faces:
		for e in shape.Edges:
			if (isinstance(e.Curve,Part.Circle)): return 8
		return 7
	if shape.Wires:
		for e in shape.Edges:
			if (isinstance(e.Curve,Part.Circle)): return 6
		for w in shape.Wires:
			if w.isClosed(): return 5
		return 4
	if (isinstance(shape.Edges[0].Curve,Part.Circle)):
		if len(shape.Vertexes) == 1:
			return 3
		return 2
	return 1

def findPerpendicular(point,edgeslist,force=None):
	'''
	findPerpendicular(vector,wire,[force]):
	finds the shortest perpendicular distance between a point and an edgeslist.
	If force is specified, only the edge[force] will be considered, and it will be
	considered infinite.
	The function will return a list	[vector_from_point_to_closest_edge,edge_index]
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

def offset(edge,vector):
	'''
	offset(edge,vector)
	returns a copy of the edge at a certain (vector) distance
	if the edge is an arc, the vector will be added at its first point
	and a complete circle will be returned
	'''
	if (not isinstance(edge,Part.Shape)) or (not isinstance(vector,FreeCAD.Vector)):
		return None
	if isinstance(edge.Curve,Part.Line):
		v1 = Vector.add(edge.Vertexes[0].Point, vector)
		v2 = Vector.add(edge.Vertexes[-1].Point, vector)
		return Part.Line(v1,v2).toShape()
	else:
		rad = edge.Vertexes[0].Point.sub(edge.Curve.Center)
		newrad = Vector.add(rad,vector).Length
		return Part.Circle(edge.Curve.Center,NORM,newrad).toShape()

def isReallyClosed(wire):
        "checks if a wire is really closed"
        if len(wire.Edges) == len(wire.Vertexes): return True
        v1 = wire.Vertexes[0].Point
        v2 = wire.Vertexes[-1].Point
        if fcvec.equals(v1,v2): return True
        return False

def getNormal(shape):
        "finds the normal of a shape, if possible"
        n = Vector(0,0,1)
        if shape.ShapeType == "Face":
                n = shape.normalAt(0.5,0.5)
        elif shape.ShapeType == "Edge":
                if isinstance(shape.Curve,Part.Circle):
                        n = shape.Curve.Axis
        else:
                for e in shape.Edges:
                        if isinstance(e.Curve,Part.Circle):
                                n = e.Curve.Axis
                                break
                        e1 = vec(shape.Edges[0])
                        for i in range(1,len(shape.Edges)):
                                e2 = vec(shape.Edges[i])
                                if 0.1 < abs(e1.getAngle(e2)) < 1.56:
                                        n = e1.cross(e2).normalize()
                                        break
        vdir = FreeCADGui.ActiveDocument.ActiveView.getViewDirection()
        if n.getAngle(vdir) < 0.78: n = fcvec.neg(n)
        return n

def offsetWire(wire,dvec,bind=False,occ=False):
        '''
        offsetWire(wire,vector,[bind]): offsets the given wire along the
        given vector. The vector will be applied at the first vertex of
        the wire. If bind is True (and the shape is open), the original
        wire and the offsetted one are bound by 2 edges, forming a face.
        '''
        edges = sortEdges(wire.Edges)
        norm = getNormal(wire)
        closed = isReallyClosed(wire)
        nedges = []
        if occ:
                l=abs(dvec.Length)
                if not l: return None
                if not wire.Wires:
                        wire = Part.Wire(edges)
                try:
                        off = wire.makeOffset(l)
                except:
                        return None
                else:
                        return off
        for i in range(len(edges)):
                curredge = edges[i]
                delta = dvec
                if i != 0:
                        angle = fcvec.angle(vec(edges[0]),vec(curredge),norm)
                        delta = fcvec.rotate(delta,angle,norm)
                nedge = offset(curredge,delta)
                nedges.append(nedge)
        nedges = connect(nedges,closed)
        if bind and not closed:
                e1 = Part.Line(edges[0].Vertexes[0].Point,nedges[0].Vertexes[0].Point).toShape()
                e2 = Part.Line(edges[-1].Vertexes[-1].Point,nedges[-1].Vertexes[-1].Point).toShape()
                alledges = edges.extend(nedges)
                alledges = alledges.extend([e1,e2])
                w = Part.Wire(alledges)
                return w
        else:
                return nedges

def connect(edges,closed=False):
        '''connects the edges in the given list by their intersections'''
        nedges = []
        for i in range(len(edges)):
                curr = edges[i]
                # print "fcgeo.connect edge ",i," : ",curr.Vertexes[0].Point,curr.Vertexes[-1].Point
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
                        # print "debug: fcgeo.connect prev : ",prev.Vertexes[0].Point,prev.Vertexes[-1].Point
                        i = findIntersection(curr,prev,True,True)
                        if i:
                                v1 = i[0]
                        else:
                                v1 = curr.Vertexes[0].Point
                else:
                        v1 = curr.Vertexes[0].Point
                if next:
                        # print "debug: fcgeo.connect next : ",next.Vertexes[0].Point,next.Vertexes[-1].Point
                        i = findIntersection(curr,next,True,True)
                        if i:
                                v2 = i[0]
                        else:
                                v2 = curr.Vertexes[-1].Point 
                else:
                        v2 = curr.Vertexes[-1].Point
                if isinstance(curr.Curve,Part.Line):
                        if v1 != v2:
                                nedges.append(Part.Line(v1,v2).toShape())
                elif isinstance(curr.Curve,Part.Circle):
                        if v1 != v2:
                                nedges.append(Part.Arc(v1,findMidPoint(curr),v2))
        try:
                return Part.Wire(nedges)
        except:
                return None

def findDistance(point,edge,strict=False):
	'''
	findDistance(vector,edge,[strict]) - Returns a vector from the point to its
	closest point on the edge. If strict is True, the vector will be returned
	only if its endpoint lies on the edge.
	'''
	if isinstance(point, FreeCAD.Vector):
		if isinstance(edge.Curve, Part.Line):
			segment = vec(edge)
			chord = edge.Vertexes[0].Point.sub(point)
			norm = segment.cross(chord)
			perp = segment.cross(norm)
			dist = fcvec.project(chord,perp)
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
		elif isinstance(edge.Curve, Part.Circle):
			ve1 = edge.Vertexes[0].Point
			if (len(edge.Vertexes) > 1):
				ve2 = edge.Vertexes[-1].Point
			else:
				ve2 = None
			center = edge.Curve.Center
			segment = center.sub(point)
			ratio = (segment.Length - edge.Curve.Radius) / segment.Length
			dist = fcvec.scale(segment,ratio)
			newpoint = Vector.add(point, dist)
			if (dist.Length == 0):
				return None
			if strict and ve2:
				ang1 = fcvec.angle(ve1.sub(center))
				ang2 = fcvec.angle(ve2.sub(center))
				angpt = fcvec.angle(newpoint.sub(center))
				if ((angpt <= ang2 and angpt >= ang1) or (angpt <= ang1 and angpt >= ang2)):
					return dist
				else:
					return None
			else:
				return dist
                elif isinstance(edge.Curve,Part.BSplineCurve):
                        try:
                                pr = edge.Curve.parameter(point)
                                np = edge.Curve.value(pr)
                                dist = np.sub(point)
                        except:
                                print "fcgeo: Unable to get curve parameter for point ",point
                                return None
                        else:
                                return dist
		else:
			print "fcgeo: Couldn't project point"
			return None
	else:
		print "fcgeo: Couldn't project point"
		return None


def angleBisection(edge1, edge2):
	"angleBisection(edge,edge) - Returns an edge that bisects the angle between the 2 edges."
	if isinstance(edge1.Curve, Part.Line) and isinstance(edge2.Curve, Part.Line):
		p1 = edge1.Vertexes[0].Point
		p2 = edge1.Vertexes[-1].Point
		p3 = edge2.Vertexes[0].Point
		p4 = edge2.Vertexes[-1].Point
		int = findIntersection(edge1, edge2, True, True)
		if int:
			line1Dir = p2.sub(p1)
			angleDiff = fcvec.angle(line1Dir, p4.sub(p3))
			ang = angleDiff * 0.5
			origin = int[0]
			line1Dir.normalize()
			dir = fcvec.rotate(line1Dir, ang)
			return Part.Line(origin,origin.add(dir)).toShape()
		else:
			diff = p3.sub(p1)
			origin = p1.add(fcvec.scale(diff, 0.5))
			dir = p2.sub(p1); dir.normalize()
			return Part.Line(origin,origin.add(dir)).toShape()
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

def isCoplanar(faces):
        "checks if all faces in the given list are coplanar"
        if len(faces) < 2:
                return True
        base =faces[0].normalAt(.5,.5)
        for i in range(1,len(faces)):
                normal = faces[i].normalAt(.5,.5)
                if (normal.getAngle(base) > .0001) and (normal.getAngle(base) < 3.1415):
                        return False
        return True

def findWires(edges):
        '''finds connected edges in the list, and returns a list of lists containing edges
        that can be connected'''
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
        if isinstance(edge.Curve,Part.Line):
                return vec(edge)
        elif isinstance(edge.Curve,Part.BSplineCurve):
                if not frompoint:
                        return None
                cp = edge.Curve.parameter(frompoint)
                return edge.Curve.tangent(cp)[0]
        elif isinstance(edge.Curve,Part.Circle):
                if not frompoint:
                        v1 = edge.Vertexes[0].Point.sub(edge.Curve.Center)
                else:
                        v1 = frompoint.sub(edge.Curve.Center)
                return v1.cross(edge.Curve.Axis)
        return None

def bind(w1,w2):
        '''bind(wire1,wire2): binds 2 wires by their endpoints and
        returns a face'''
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
                w3 = Part.Line(w1.Vertexes[0].Point,w2.Vertexes[0].Point).toShape()
                w4 = Part.Line(w1.Vertexes[-1].Point,w2.Vertexes[-1].Point).toShape()
                return Part.Face(Part.Wire(w1.Edges+[w3]+w2.Edges+[w4]))

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
        # print "lut:",lut
        # take edges shared by 2 faces
        sharedhedges = []
        for k,v in lut.iteritems():
                if len(v) == 2:
                        sharedhedges.append(k)
        # print len(sharedhedges)," shared edges:",sharedhedges
        # find those with same normals
        targethedges = []
        for hedge in sharedhedges:
                faces = lut[hedge]
                n1 = find(faces[0]).normalAt(0.5,0.5)
                n2 = find(faces[1]).normalAt(0.5,0.5)
                if n1 == n2:
                        targethedges.append(hedge)
        # print len(targethedges)," target edges:",targethedges
        # get target faces
        hfaces = []
        for hedge in targethedges:
                for f in lut[hedge]:
                        if not f in hfaces:
                                hfaces.append(f)

        # print len(hfaces)," target faces:",hfaces
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
        # print len(islands)," islands:",islands
        # make new faces from islands
        newfaces = []
        treated = []
        for isle in islands:
                treated.extend(isle)
                fset = []
                for i in isle: fset.append(find(i))
                bounds = getBoundary(fset)
                shp = Part.Wire(sortEdges(bounds))
                shp = Part.Face(shp)
                if shp.normalAt(0.5,0.5) != find(isle[0]).normalAt(0.5,0.5):
                        shp.reverse()
                newfaces.append(shp)
        # print "new faces:",newfaces
        # add remaining faces
        for f in faceset:
                if not f.hashCode() in treated:
                        newfaces.append(f)
        # print "final faces"
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
        if not isinstance(e.Curve,Part.Line):
            return False
    # if ok until now, let's do more advanced testing
    for f in shape.Faces:
        if len(f.Edges) != 4: return False
        for i in range(4):
            e1 = vec(f.Edges[i])
            if i < 3:
                e2 = vec(f.Edges[i+1])
            else: e2 = vec(f.Edges[0])
            rpi = [0.0,round(math.pi/2,precision)]
            if not round(e1.getAngle(e2),precision) in rpi:
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
    rotZ = fcvec.angle(vx)
    rotY = fcvec.angle(vx,FreeCAD.Vector(vx.x,vx.y,0))
    rotX = fcvec.angle(vy,FreeCAD.Vector(vy.x,vy.y,0))
    # getting height
    vz = None
    rpi = round(math.pi/2,precision)
    for i in range(1,6):
        for e in shape.Faces[i].Edges:
            if basepoint in [e.Vertexes[0].Point,e.Vertexes[1].Point]:
                vtemp = vec(e)
                # print vtemp
                if round(vtemp.getAngle(vx),precision) == rpi:
                    if round(vtemp.getAngle(vy),precision) == rpi:
                        vz = vtemp
    if not vz: return None
    mat = FreeCAD.Matrix()
    mat.move(plpoint)
    mat.rotateX(rotX)
    mat.rotateY(rotY)
    mat.rotateZ(rotZ)
    return [FreeCAD.Placement(mat),round(vx.Length,precision),round(vy.Length,precision),round(vz.Length,precision)]

def removeInterVertices(wire):
        '''removeInterVertices(wire) - remove unneeded vertices (those that
        are in the middle of a straight line) from a wire, returns a new wire.'''
        edges = sortEdges(wire.Edges)
        nverts = []
        def getvec(v1,v2):
                if not abs(round(v1.getAngle(v2),precision) in [0,round(math.pi,precision)]):
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
        if isinstance(edge.Curve,Part.Line):
                print "This edge is straight, cannot build an arc on it"
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
                        print "Couldn't make an arc out of this edge"
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
                        print "couldn't make a circle out of this edge"
   
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
	if isinstance(tan1.Curve, Part.Line) and isinstance(tan2.Curve, Part.Line) and isinstance(point, FreeCAD.Vector):
		return circlefrom2Lines1Point(tan1, tan2, point)
	elif isinstance(tan1.Curve, Part.Circle) and isinstance(tan2.Curve, Part.Line) and isinstance(point, FreeCAD.Vector):
		return circlefromCircleLinePoint(tan1, tan2, point)
	elif isinstance(tan2.Curve, Part.Circle) and isinstance(tan1.Curve, Part.Line) and isinstance(point, FreeCAD.Vector):
		return circlefromCircleLinePoint(tan2, tan1, point)
	elif isinstance(tan2.Curve, Part.Circle) and isinstance(tan1.Curve, Part.Circle) and isinstance(point, FreeCAD.Vector):
		return circlefrom2Circles1Point(tan2, tan1, point)

def circleFrom2tan1rad(tan1, tan2, rad):
	"circleFrom2tan1rad(edge, edge, float)"
	if isinstance(tan1.Curve, Part.Line) and isinstance(tan2.Curve, Part.Line):
		return circleFrom2LinesRadius(tan1, tan2, rad)
	elif isinstance(tan1.Curve, Part.Circle) and isinstance(tan2.Curve, Part.Line):
		return circleFromCircleLineRadius(tan1, tan2, rad)
	elif isinstance(tan1.Curve, Part.Line) and isinstance(tan2.Curve, Part.Circle):
		return circleFromCircleLineRadius(tan2, tan1, rad)
	elif isinstance(tan1.Curve, Part.Circle) and isinstance(tan2.Curve, Part.Circle):
		return circleFrom2CirclesRadius(tan1, tan2, rad)

def circleFrom1tan2pt(tan1, p1, p2):
	if isinstance(tan1.Curve, Part.Line) and isinstance(p1, FreeCAD.Vector) and isinstance(p2, FreeCAD.Vector):
		return circlefrom1Line2Points(tan1, p1, p2)
	if isinstance(tan1.Curve, Part.Line) and isinstance(p1, FreeCAD.Vector) and isinstance(p2, FreeCAD.Vector):
		return circlefrom1Circle2Points(tan1, p1, p2)

def circleFrom1tan1pt1rad(tan1, p1, rad):
	if isinstance(tan1.Curve, Part.Line) and isinstance(p1, FreeCAD.Vector):
		return circleFromPointLineRadius(p1, tan1, rad)
	if isinstance(tan1.Curve, Part.Circle) and isinstance(p1, FreeCAD.Vector):
		return circleFromPointCircleRadius(p1, tan1, rad)

def circleFrom3tan(tan1, tan2, tan3):
	tan1IsLine = isinstance(tan1.Curve, Part.Line)
	tan2IsLine = isinstance(tan2.Curve, Part.Line)
	tan3IsLine = isinstance(tan3.Curve, Part.Line)
	tan1IsCircle = isinstance(tan1.Curve, Part.Circle)
	tan2IsCircle = isinstance(tan2.Curve, Part.Circle)
	tan3IsCircle = isinstance(tan3.Curve, Part.Circle)
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
	edgeDir = vec(edge); edgeDir.normailze()
	projectedCen1 = Vector.add(s, fcvec.scale(edgeDir, projectedDist))
	projectedCen2 = Vector.add(s, fcvec.scale(edgeDir, -projectedDist))
	perpEdgeDir = edgeDir.cross(Vector(0,0,1))
	perpCen1 = Vector.add(projectedCen1, perpEdgeDir)
	perpCen2 = Vector.add(projectedCen2, perpEdgeDir)
	mid = findMidpoint(p1_p2)
	x = fcvec.crossproduct(vec(p1_p2)); x.normalize()
	perp_mid = Vector.add(mid, x)
	cen1 = findIntersection(edg(projectedCen1, perpCen1), edg(mid, perp_mid), True, True)
	cen2 = findIntersection(edg(projectedCen2, perpCen2), edg(mid, perp_mid), True, True)
	circles = []
	if cen1:
		radius = fcvec.dist(projectedCen1, cen1[0])
		circles.append(Part.Circle(cen1[0], NORM, radius))
	if cen2:
		radius = fcvec.dist(projectedCen2, cen2[0])
		circles.append(Part.Circle(cen2[0], NORM, radius))

	if circles: return circles
	else: return None

def circleFrom2LinesRadius (edge1, edge2, radius):
	"circleFrom2LinesRadius(edge,edge,radius)"
	int = findIntersection(edge1, edge2, True, True)
	if not int: return None
	int = int[0]
	bis12 = angleBisection(edge1,edge2)
	bis21 = Part.Line(bis12.Vertexes[0].Point,fcvec.rotate(vec(bis12), math.pi/2.0))
	ang12 = abs(fcvec.angle(vec(edge1),vec(edge2)))
	ang21 = math.pi - ang12
	dist12 = radius / math.sin(ang12 * 0.5)
	dist21 = radius / math.sin(ang21 * 0.5)
	circles = []
	cen = Vector.add(int, fcvec.scale(vec(bis12), dist12))
	circles.append(Part.Circle(cen, NORM, radius))
	cen = Vector.add(int, fcvec.scale(vec(bis12), -dist12))
	circles.append(Part.Circle(cen, NORM, radius))
	cen = Vector.add(int, fcvec.scale(vec(bis21), dist21))
	circles.append(Part.Circle(cen, NORM, radius))
	cen = Vector.add(int, fcvec.scale(vec(bis21), -dist21))
	circles.append(Part.Circle(cen, NORM, radius))
	return circles

def circleFrom3LineTangents (edge1, edge2, edge3):
	"circleFrom3LineTangents(edge,edge,edge)"
	def rot(ed):
		return Part.Line(v1(ed),v1(ed).add(fcvec.rotate(vec(ed),math.pi/2))).toShape()
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
			if fcvec.equals(cir.Center, int.Center):
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
		perpVec = fcvec.crossproduct(segment); perpVec.normalize()
		normPoint_c1 = fcvec.scale(perpVec, radius)
		normPoint_c2 = fcvec.scale(perpVec, -radius)
		center1 = point.add(normPoint_c1)
		center2 = point.add(normPoint_c2)
	elif dist.Length > 2 * radius:
		return None
	elif dist.Length == 2 * radius:
		normPoint = point.add(findDistance(point, edge, False))
		dummy = fcvec.scale(normPoint.sub(point), 0.5)
		cen = point.add(dummy)
		circ = Part.Circle(cen, NORM, radius)
		if circ:
			return [circ]
		else:
			return None
	else:
		normPoint = point.add(findDistance(point, edge, False))
		normDist = fcvec.dist(normPoint, point)
		dist = math.sqrt(radius**2 - (radius - normDist)**2)
		centerNormVec = fcvec.scaleTo(point.sub(normPoint), radius)
		edgeDir = edge.Vertexes[0].Point.sub(normPoint); edgeDir.normalize()
		center1 = centerNormVec.add(normPoint.add(fcvec.scale(edgeDir, dist)))
		center2 = centerNormVec.add(normPoint.add(fcvec.scale(edgeDir, -dist)))
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
	if fcvec.equals(p1, p2): return None

	p1_p2 = Part.Line(p1, p2).toShape()
	dist_p1p2 = fcvec.dist(p1, p1)
	mid = findMidpoint(p1_p2)
	if dist_p1p2 == 2*radius:
		circle = Part.Circle(mid, norm, radius)
		if circle: return [circle]
		else: return None
	dir = vec(p1_p2); dir.normalize()
	perpDir = dir.cross(Vector(0,0,1)); perpDir.normailze()
	dist = math.sqrt(radius**2 - (dist_p1p2 / 2.0)**2)
	cen1 = Vector.add(mid, fcvec.scale(perpDir, dist))
	cen2 = Vector.add(mid, fcvec.scale(perpDir, -dist))
	circles = []
	if cen1: circles.append(Part.Circle(cen1, norm, radius))
	if cen2: circles.append(Part.Circle(cen2, norm, radius))
	if circles: return circles
	else: return None



#############################33 to include







def outerSoddyCircle(circle1, circle2, circle3):
	'''
	Computes the outer soddy circle for three tightly packed circles.
	'''
	if isinstance(circle1.Curve, Part.Circle) and isinstance(circle2.Curve, Part.Circle) and isinstance(circle3.Curve, Part.Circle):
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

		# If the formula is not solveable, we return no circle.
		if (not z or not (1 / k4)):
			return None

		X = -z.real
		Y = -z.imag
		print "Outer Soddy circle: " + str(X) + " " + str(Y) + "\n"	# Debug

		# The Radius of the outer soddy circle can also be calculated with the following formula:
		# radiusOuter = abs(r1*r2*r3 / (r1*r2 + r1*r3 + r2*r3 - 2 * math.sqrt(r1*r2*r3 * (r1+r2+r3))))
		circ = Part.Circle(Vector(X, Y, A.z), norm, 1 / k4)
		return circ

	else:
		print "debug: outerSoddyCircle bad parameters!\n"
		# FreeCAD.Console.PrintMessage("debug: outerSoddyCircle bad parameters!\n")
		return None

def innerSoddyCircle(circle1, circle2, circle3):
	'''
	Computes the inner soddy circle for three tightly packed circles.
	'''
	if isinstance(circle1.Curve, Part.Circle) and isinstance(circle2.Curve, Part.Circle) and isinstance(circle3.Curve, Part.Circle):
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

		# If the formula is not solveable, we return no circle.
		if (not z or not (1 / k4)):
			return None

		X = z.real
		Y = z.imag
		print "Outer Soddy circle: " + str(X) + " " + str(Y) + "\n"	# Debug

		# The Radius of the inner soddy circle can also be calculated with the following formula:
		# radiusInner = abs(r1*r2*r3 / (r1*r2 + r1*r3 + r2*r3 + 2 * math.sqrt(r1*r2*r3 * (r1+r2+r3))))
		circ = Part.Circle(Vector(X, Y, A.z), norm, 1 / k4)
		return circ

	else:
		print "debug: innerSoddyCircle bad parameters!\n"
		# FreeCAD.Console.PrintMessage("debug: innerSoddyCircle bad parameters!\n")
		return None
 
def circleFrom3CircleTangents(circle1, circle2, circle3):
	'''
	http://en.wikipedia.org/wiki/Problem_of_Apollonius#Inversive_methods
	http://mathworld.wolfram.com/ApolloniusCircle.html
	http://mathworld.wolfram.com/ApolloniusProblem.html
	'''

	if isinstance(circle1.Curve, Part.Circle) and isinstance(circle2.Curve, Part.Circle) and isinstance(circle3.Curve, Part.Circle):
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
#				print str(outerSoddy) + "\n" # Debug

				innerSoddy = innerSoddyCircle(circle1, circle2, circle3)
#				print str(innerSoddy) + "\n" # Debug

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
			# Some circles are inside each other or an error has occured.
			return None

	else:
		print "debug: circleFrom3CircleTangents bad parameters!\n"
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

	if isinstance(circle1.Curve, Part.Circle) and isinstance(circle2.Curve, Part.Circle):
		if fcvec.equals(circle1.Curve.Center, circle2.Curve.Center):
			return None

		cen1_cen2 = Part.Line(circle1.Curve.Center, circle2.Curve.Center).toShape()
		cenDir = vec(cen1_cen2); cenDir.normalize()

		# Get the perpedicular vector.
		perpCenDir = cenDir.cross(Vector(0,0,1)); perpCenDir.normalize()

		# Get point on first circle
		p1 = Vector.add(circle1.Curve.Center, fcvec.scale(perpCenDir, circle1.Curve.Radius))

		centers = []
		# Calculate inner homothetic center
		# Get point on second circle
		p2_inner = Vector.add(circle1.Curve.Center, fcvec.scale(perpCenDir, -circle1.Curve.Radius))
		hCenterInner = fcvec.intersect(circle1.Curve.Center, circle2.Curve.Center, p1, p2_inner, True, True)
		if hCenterInner:
			centers.append(hCenterInner)

		# Calculate outer homothetic center (only exists of the circles have different radii)
		if circle1.Curve.Radius != circle2.Curve.Radius:
			# Get point on second circle
			p2_outer = Vector.add(circle1.Curve.Center, fcvec.scale(perpCenDir, circle1.Curve.Radius))
			hCenterOuter = fcvec.intersect(circle1.Curve.Center, circle2.Curve.Center, p1, p2_outer, True, True)
			if hCenterOuter:
				centers.append(hCenterOuter)

		if len(centers):
			return centers
		else:
			return None

	else:
		print "debug: findHomotheticCenterOfCircles bad parameters!\n"
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

	if isinstance(circle1.Curve, Part.Circle) and isinstance(circle2.Curve, Part.Circle):
		if fcvec.equals(circle1.Curve.Center, circle2.Curve.Center):
			return None
		r1 = circle1.Curve.Radius
		r2 = circle1.Curve.Radius
		cen1 = circle1.Curve.Center
		# dist .. the distance from cen1 to cen2.
		dist = fcvec.dist(cen1, circle2.Curve.Center)
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

		K = Vector.add(cen1, fcvec.scale(cenDir, k1))

		# K_ .. A point somewhere between K and J (actually with a distance of 1 unit from K).
		K_ = Vector,add(K, perpCenDir)

		radicalAxis = Part.Line(K, Vector.add(origin, dir))

		if radicalAxis:
			return radicalAxis
		else:
			return None
	else:
		print "debug: findRadicalAxis bad parameters!\n"
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

	if isinstance(circle1.Curve, Part.Circle) and isinstance(circle2.Curve, Part.Circle) and isinstance(circle2.Curve, Part.Circle):
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
		print "debug: findRadicalCenter bad parameters!\n"
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

	if isinstance(circle.Curve, Part.Circle) and isinstance(point, FreeCAD.Vector):
		cen = circle.Curve.Center
		rad = circle.Curve.Radius

		if fcvec.equals(cen, point):
			return None

		# Inverse the distance of the point
		# dist(cen -> P) = r^2 / dist(cen -> invP)

		dist = fcvec.dist(point, cen)
		invDist = rad**2 / d

		invPoint = Vector(0, 0, point.z)
		invPoint.x = cen.x + (point.x - cen.x) * invDist / dist;
		invPoint.y = cen.y + (point.y - cen.y) * invDist / dist;

		return invPoint

	else:
		print "debug: pointInversion bad parameters!\n"
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

	if isinstance(circle.Curve, Part.Circle) and isinstance(edge.Curve, Part.Line):
		nearest = circle.Curve.Center.add(findDistance(circle.Curve.Center, edge, False))
		if nearest:
			inversionPole = pointInversion(circle, nearest)
			if inversionPole:
				return inversionPole

	else:
		print "debug: circleInversionPole bad parameters!\n"
		FreeCAD.Console.PrintMessage("debug: circleInversionPole bad parameters!\n")
		return None

def circleInversion(circle, circle2):
	'''
	pointInversion(Circle, Circle)

	Circle inversion of a circle.
	'''
	if isinstance(circle.Curve, Part.Circle) and isinstance(circle2.Curve, Part.Circle):
		cen1 = circle.Curve.Center
		rad1 = circle.Curve.Radius

		if fcvec.equals(cen1, point):
			return None

		invCen2 = Inversion(circle, circle2.Curve.Center)

		pointOnCircle2 = Vector.add(circle2.Curve.Center, Vector(circle2.Curve.Radius, 0, 0))
		invPointOnCircle2 = Inversion(circle, pointOnCircle2)

		return Part.Circle(invCen2, norm, fcvec.dist(invCen2, invPointOnCircle2))

	else:
		print "debug: circleInversion bad parameters!\n"
		FreeCAD.Console.PrintMessage("debug: circleInversion bad parameters!\n")
		return None

