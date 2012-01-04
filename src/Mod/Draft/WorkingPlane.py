#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2009, 2010                                              *
#*   Ken Cline <cline@frii.com>                                            *
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


import FreeCAD, FreeCADGui, math
from FreeCAD import Vector
from draftlibs import fcvec

__title__="FreeCAD Working Plane utility"
__author__ = "Ken Cline"
__url__ = "http://free-cad.sourceforge.net"

'''
This module provides a class called plane to assist in selecting and maintaining a working plane.
'''

class plane:
	'''A WorkPlane object'''

	def __init__(self):
		# keep track of active document.  Reset view when doc changes.
		self.doc = None
		# self.weak is true if the plane has been defined by self.setup or has been reset
		self.weak = True
		# u, v axes and position define plane, perpendicular axis is handy, though redundant.
		self.u = Vector(1,0,0)
		self.v = Vector(0,1,0)
		self.axis = Vector(0,0,1)
		self.position = Vector(0,0,0)
                # a placeholder for a stored state
                self.stored = None

        def __repr__(self):
                return "Workplane x="+str(fcvec.rounded(self.u))+" y="+str(fcvec.rounded(self.v))+" z="+str(fcvec.rounded(self.axis))

	def offsetToPoint(self, p, direction=None):
		'''
		Return the signed distance from p to the plane, such
		that p + offsetToPoint(p)*direction lies on the plane.
		direction defaults to -plane.axis
		'''

		'''
		A picture will help explain the computation:

                                                            p
                                                          //|
                                                        / / |
                              			      /  /  |
						    /  	/   |
						  /    /    |
		-------------------- plane -----c-----x-----a--------

                Here p is the specified point,
                     c is a point (in this case plane.position) on the plane
                     x is the intercept on the plane from p in the specified direction, and
                     a is the perpendicular intercept on the plane (i.e. along plane.axis)

	        Using vertival bars to denote the length operator,
                     |ap| = |cp| * cos(apc) = |xp| * cos(apx)
                so
                     |xp| = |cp| * cos(apc) / cos(apx)
                          = (cp . axis) / (direction . axis)
		'''
		if direction == None: direction = self.axis
		return self.axis.dot(self.position.sub(p))/self.axis.dot(direction)

	def projectPoint(self, p, direction=None):
		'''project point onto plane, default direction is orthogonal'''
		if not direction: direction = self.axis
		t = Vector(direction)
		t.multiply(self.offsetToPoint(p, direction))
		return p.add(t)

	def alignToPointAndAxis(self, point, axis, offset, upvec=None):
		self.doc = FreeCAD.ActiveDocument
		self.axis = axis;
		self.axis.normalize()
		if (fcvec.equals(axis, Vector(1,0,0))):
			self.u = Vector(0,1,0)
			self.v = Vector(0,0,1)
                elif (fcvec.equals(axis, Vector(-1,0,0))):
                        self.u = Vector(0,-1,0)
                        self.v = Vector(0,0,1)
                elif upvec:
                        self.v = upvec
                        self.v.normalize()
                        self.u = self.v.cross(self.axis)
		else:
			self.v = axis.cross(Vector(1,0,0))
			self.v.normalize()
			self.u = fcvec.rotate(self.v, -math.pi/2, self.axis)
		offsetVector = Vector(axis); offsetVector.multiply(offset)
		self.position = point.add(offsetVector)
		self.weak = False
		# FreeCAD.Console.PrintMessage("(position = " + str(self.position) + ")\n")
		# FreeCAD.Console.PrintMessage("Current workplane: x="+str(fcvec.rounded(self.u))+" y="+str(fcvec.rounded(self.v))+" z="+str(fcvec.rounded(self.axis))+"\n")

	def alignToCurve(self, shape, offset):
		if shape.ShapeType == 'Edge':
			#??? TODO: process curve here.  look at shape.edges[0].Curve
			return False
		elif shape.ShapeType == 'Wire':
			#??? TODO: determine if edges define a plane
			return False
		else:
			return False

	def alignToFace(self, shape, offset=0):
		# Set face to the unique selected face, if found
		if shape.ShapeType == 'Face':
			#we should really use face.tangentAt to get u and v here, and implement alignToUVPoint
			self.alignToPointAndAxis(shape.Faces[0].CenterOfMass, shape.Faces[0].normalAt(0,0), offset)
			return True
		else:
			return False

	def alignToSelection(self, offset):
		'''If selection uniquely defines a plane, align working plane to it.  Return success (bool)'''
		sex = FreeCADGui.Selection.getSelectionEx(FreeCAD.ActiveDocument.Name)
		if len(sex) == 0:
			return False
		elif len(sex) == 1:
                        if not sex[0].Object.isDerivedFrom("Part::Shape"):
                                return False
			return self.alignToCurve(sex[0].Object.Shape, offset) \
				or self.alignToFace(sex[0].Object.Shape, offset) \
				or (len(sex[0].SubObjects) == 1 and self.alignToFace(sex[0].SubObjects[0], offset))
		else:
			# len(sex) > 2, look for point and line, three points, etc.
			return False

	def setup(self, direction, point, upvec=None):
		'''If working plane is undefined, define it!'''
		if self.weak:
			self.alignToPointAndAxis(point, direction, 0, upvec)
			self.weak = True

	def reset(self):
		self.doc = None
                self.weak = True

        def getRotation(self):
                "returns a placement describing the working plane orientation ONLY"
                m = fcvec.getPlaneRotation(self.u,self.v,self.axis)
                return FreeCAD.Placement(m)

        def getPlacement(self):
                "returns the placement of the working plane"
                m = FreeCAD.Matrix(
                        self.u.x,self.v.x,self.axis.x,self.position.x,
                        self.u.y,self.v.y,self.axis.y,self.position.y,
                        self.u.z,self.v.z,self.axis.z,self.position.z,
                        0.0,0.0,0.0,1.0)
                return FreeCAD.Placement(m)

        def save(self):
                "stores the current plane state"
                self.stored = [self.u,self.v,self.axis,self.position,self.weak]

        def restore(self):
                "restores a previously saved plane state, if exists"
                if self.stored:
                        self.u = self.stored[0]
                        self.v = self.stored[1]
                        self.axis = self.stored[2]
                        self.position = self.stored[3]
                        self.weak = self.stored[4]
                        self.stored = None

        def getLocalCoords(self,point):
                "returns the coordinates of a given point on the working plane"
                xv = fcvec.project(point,self.u)
                x = xv.Length
                if xv.getAngle(self.u) > 1:
                        x = -x
                yv = fcvec.project(point,self.v)
                y = yv.Length
                if yv.getAngle(self.v) > 1:
                        y = -y
                zv = fcvec.project(point,self.axis)
                z = zv.Length
                if zv.getAngle(self.axis) > 1:
                        z = -z
                return Vector(x,y,z)

        def getGlobalCoords(self,point):
                "returns the global coordinates of the given point, taken relatively to this working plane"
                vx = fcvec.scale(self.u,point.x)
                vy = fcvec.scale(self.v,point.y)
                vz = fcvec.scale(self.axis,point.z)
                return (vx.add(vy)).add(vz)

        def getClosestAxis(self,point):
                "returns which of the workingplane axes is closest from the given vector"
                ax = point.getAngle(self.u)
                ay = point.getAngle(self.v)
                az = point.getAngle(self.axis)
                bx = point.getAngle(fcvec.neg(self.u))
                by = point.getAngle(fcvec.neg(self.v))
                bz = point.getAngle(fcvec.neg(self.axis))
                b = min(ax,ay,az,bx,by,bz)
                if b in [ax,bx]:
                        return "x"
                elif b in [ay,by]:
                        return "y"
                elif b in [az,bz]:
                        return "z"
                else:
                        return None
                
def getPlacementFromPoints(points):
        "returns a placement from a list of 3 or 4 vectors"
        pl = plane()
        pl.position = points[0]
        pl.u = (points[1].sub(points[0]).normalize())
        pl.v = (points[2].sub(points[0]).normalize())
        if len(points) == 4:
                pl.axis = (points[3].sub(points[0]).normalize())
        else:
                pl.axis = ((pl.u).cross(pl.v)).normalize()
        p = pl.getPlacement()
        del pl
        return p
