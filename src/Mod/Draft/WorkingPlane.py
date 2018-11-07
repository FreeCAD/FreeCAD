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


import FreeCAD, math, DraftVecUtils
from FreeCAD import Vector

__title__="FreeCAD Working Plane utility"
__author__ = "Ken Cline"
__url__ = "http://www.freecadweb.org"

## @package WorkingPlane
#  \ingroup DRAFT
#  \brief This module handles the Working Plane and grid of the Draft module.
#
#  This module contains the plane class which provides a virtual plane in FreeCAD
#  and a couple of utility functions

'''
This module provides a class called plane to assist in selecting and maintaining a working plane.
'''

class plane:
    '''A WorkPlane object'''

    def __init__(self,u=Vector(1,0,0),v=Vector(0,1,0),w=Vector(0,0,1),pos=Vector(0,0,0)):
        # keep track of active document.  Reset view when doc changes.
        self.doc = None
        # self.weak is true if the plane has been defined by self.setup or has been reset
        self.weak = True
        # u, v axes and position define plane, perpendicular axis is handy, though redundant.
        self.u = u
        self.v = v
        self.axis = w
        self.position = pos
        # a placeholder for a stored state
        self.stored = None

    def __repr__(self):
        return "Workplane x="+str(DraftVecUtils.rounded(self.u))+" y="+str(DraftVecUtils.rounded(self.v))+" z="+str(DraftVecUtils.rounded(self.axis))

    def copy(self):
        return plane(u=self.u,v=self.v,w=self.axis,pos=self.position)

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
                            /   /   |
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
        return direction.dot(self.position.sub(p))

    def projectPoint(self, p, direction=None):
        '''project point onto plane, default direction is orthogonal'''
        if not direction:
            direction = self.axis
        lp = self.getLocalCoords(p)
        gp = self.getGlobalCoords(Vector(lp.x,lp.y,0))
        a = direction.getAngle(gp.sub(p))
        if a > math.pi/2:
            direction = direction.negative()
            a = math.pi - a
        ld = self.getLocalRot(direction)
        gd = self.getGlobalRot(Vector(ld.x,ld.y,0))
        hyp = abs(math.tan(a) * lp.z)
        return gp.add(DraftVecUtils.scaleTo(gd,hyp))
                
    def projectPointOld(self, p, direction=None):
        '''project point onto plane, default direction is orthogonal. Obsolete'''
        if not direction:
            direction = self.axis
        t = Vector(direction)
        #t.normalize()
        a = round(t.getAngle(self.axis),DraftVecUtils.precision())
        pp = round((math.pi)/2,DraftVecUtils.precision())
        if a == pp:
            return p
        t.multiply(self.offsetToPoint(p, direction))
        return p.add(t)

    def alignToPointAndAxis(self, point, axis, offset=0, upvec=None):
        self.doc = FreeCAD.ActiveDocument
        self.axis = axis;
        self.axis.normalize()
        if axis.getAngle(Vector(1,0,0)) < 0.00001:
            self.axis = Vector(1,0,0)
            self.u = Vector(0,1,0)
            self.v = Vector(0,0,1)
        elif axis.getAngle(Vector(-1,0,0)) < 0.00001:
            self.axis = Vector(-1,0,0)
            self.u = Vector(0,-1,0)
            self.v = Vector(0,0,1)
        elif upvec:
            self.v = upvec
            self.v.normalize()
            self.u = self.v.cross(self.axis)
        else:
            self.v = axis.cross(Vector(1,0,0))
            self.v.normalize()
            self.u = DraftVecUtils.rotate(self.v, -math.pi/2, self.axis)
        offsetVector = Vector(axis); offsetVector.multiply(offset)
        self.position = point.add(offsetVector)
        self.weak = False
        # FreeCAD.Console.PrintMessage("(position = " + str(self.position) + ")\n")
        # FreeCAD.Console.PrintMessage("Current workplane: x="+str(DraftVecUtils.rounded(self.u))+" y="+str(DraftVecUtils.rounded(self.v))+" z="+str(DraftVecUtils.rounded(self.axis))+"\n")

    def alignToPointAndAxis_SVG(self, point, axis, offset):
        # based on cases table
        self.doc = FreeCAD.ActiveDocument
        self.axis = axis;
        self.axis.normalize()
        ref_vec = Vector(0.0, 1.0, 0.0)

        if ((abs(axis.x) > abs(axis.y)) and (abs(axis.y) > abs(axis.z))):
            ref_vec = Vector(0.0, 0., 1.0)
            self.u = axis.negative().cross(ref_vec)
            self.u.normalize()
            self.v = DraftVecUtils.rotate(self.u, math.pi/2, self.axis)
            #projcase = "Case new"
        
        elif ((abs(axis.y) > abs(axis.z)) and (abs(axis.z) >= abs(axis.x))):
            ref_vec = Vector(1.0, 0.0, 0.0)
            self.u = axis.negative().cross(ref_vec)
            self.u.normalize()
            self.v = DraftVecUtils.rotate(self.u, math.pi/2, self.axis)
            #projcase = "Y>Z, View Y"

        elif ((abs(axis.y) >= abs(axis.x)) and (abs(axis.x) > abs(axis.z))):
            ref_vec = Vector(0.0, 0., 1.0)
            self.u = axis.cross(ref_vec)
            self.u.normalize()
            self.v = DraftVecUtils.rotate(self.u, math.pi/2, self.axis)
            #projcase = "ehem. XY, Case XY"

        elif ((abs(axis.x) > abs(axis.z)) and (abs(axis.z) >= abs(axis.y))):
            self.u = axis.cross(ref_vec)
            self.u.normalize()
            self.v = DraftVecUtils.rotate(self.u, math.pi/2, self.axis)
            #projcase = "X>Z, View X"

        elif ((abs(axis.z) >= abs(axis.y)) and (abs(axis.y) > abs(axis.x))):
            ref_vec = Vector(1.0, 0., 0.0)
            self.u = axis.cross(ref_vec)
            self.u.normalize()
            self.v = DraftVecUtils.rotate(self.u, math.pi/2, self.axis)
            #projcase = "Y>X, Case YZ"

        else:
            self.u = axis.negative().cross(ref_vec)
            self.u.normalize()
            self.v = DraftVecUtils.rotate(self.u, math.pi/2, self.axis)
            #projcase = "else"

        #spat_vec = self.u.cross(self.v)
        #spat_res = spat_vec.dot(axis)
        #FreeCAD.Console.PrintMessage(projcase + " spat Prod = " + str(spat_res) + "\n")
        
        offsetVector = Vector(axis); offsetVector.multiply(offset)
        self.position = point.add(offsetVector)
        self.weak = False
        # FreeCAD.Console.PrintMessage("(position = " + str(self.position) + ")\n")
        # FreeCAD.Console.PrintMessage("Current workplane: x="+str(DraftVecUtils.rounded(self.u))+" y="+str(DraftVecUtils.rounded(self.v))+" z="+str(DraftVecUtils.rounded(self.axis))+"\n")


    def alignToCurve(self, shape, offset):
        if shape.ShapeType == 'Edge':
            #??? TODO: process curve here.  look at shape.edges[0].Curve
            return False
        elif shape.ShapeType == 'Wire':
            #??? TODO: determine if edges define a plane
            return False
        else:
            return False

    def alignToEdges(self,edges):
        # use a list of edges to find a plane position
        if len(edges) > 2:
            return False
        # for axes systems, we suppose the 2 first edges are parallel
        # ??? TODO: exclude other cases first
        v1 = edges[0].Vertexes[-1].Point.sub(edges[0].Vertexes[0].Point)
        v2 = edges[1].Vertexes[0].Point.sub(edges[0].Vertexes[0].Point)
        v3 = v1.cross(v2)
        v1.normalize()
        v2.normalize()
        v3.normalize()
        #print v1,v2,v3
        self.u = v1
        self.v = v2
        self.axis = v3

    def alignToFace(self, shape, offset=0):
        # Set face to the unique selected face, if found
        if shape.ShapeType == 'Face':
            self.alignToPointAndAxis(shape.Faces[0].CenterOfMass, shape.Faces[0].normalAt(0,0), offset)
            import DraftGeomUtils
            q = DraftGeomUtils.getQuad(shape)
            if q:
                self.u = q[1]
                self.v = q[2]
                if not DraftVecUtils.equals(self.u.cross(self.v),self.axis):
                    self.u = q[2]
                    self.v = q[1]
                if DraftVecUtils.equals(self.u,Vector(0,0,1)):
                    # the X axis is vertical: rotate 90 degrees
                    self.u,self.v = self.v.negative(),self.u
                elif DraftVecUtils.equals(self.u,Vector(0,0,-1)):
                    self.u,self.v = self.v,self.u.negative()
                    
            self.weak = False
            return True
        else:
            return False

    def alignTo3Points(self,p1,p2,p3,offset=0):
        import Part
        w = Part.makePolygon([p1,p2,p3,p1])
        f = Part.Face(w)
        return self.alignToFace(f,offset)

    def alignToSelection(self, offset):
        '''If selection uniquely defines a plane, align working plane to it.  Return success (bool)'''
        import FreeCADGui
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

    def setup(self, direction=None, point=None, upvec=None):
        '''If working plane is undefined, define it!'''
        if self.weak:
            if direction and point:
                self.alignToPointAndAxis(point, direction, 0, upvec)
            else:
                try:
                    import FreeCADGui
                    from pivy import coin
                    rot = FreeCADGui.ActiveDocument.ActiveView.getCameraNode().getField("orientation").getValue()
                    upvec = Vector(rot.multVec(coin.SbVec3f(0,1,0)).getValue())
                    vdir = FreeCADGui.ActiveDocument.ActiveView.getViewDirection()
                    if (vdir.getAngle(self.axis) > 0.001) and (vdir.getAngle(self.axis) < 3.14159):
                        # don't change the WP if it is already perpendicular to the current view
                        self.alignToPointAndAxis(Vector(0,0,0), vdir.negative(), 0, upvec)
                except:
                    pass
            self.weak = True

    def reset(self):
        self.doc = None
        self.weak = True

    def getRotation(self):
        "returns a placement describing the working plane orientation ONLY"
        m = DraftVecUtils.getPlaneRotation(self.u,self.v,self.axis)
        p = FreeCAD.Placement(m)
        # Arch active container
        if FreeCAD.GuiUp:
            import FreeCADGui
            if FreeCADGui.ActiveDocument.ActiveView:
                a = FreeCADGui.ActiveDocument.ActiveView.getActiveObject("Arch")
                if a:
                    p = a.Placement.inverse().multiply(p)
        return p

    def getPlacement(self,rotated=False):
        "returns the placement of the working plane"
        if rotated:
            m = FreeCAD.Matrix(
                self.u.x,self.axis.x,-self.v.x,self.position.x,
                self.u.y,self.axis.y,-self.v.y,self.position.y,
                self.u.z,self.axis.z,-self.v.z,self.position.z,
                0.0,0.0,0.0,1.0)
        else:
            m = FreeCAD.Matrix(
                self.u.x,self.v.x,self.axis.x,self.position.x,
                self.u.y,self.v.y,self.axis.y,self.position.y,
                self.u.z,self.v.z,self.axis.z,self.position.z,
                0.0,0.0,0.0,1.0)
        p = FreeCAD.Placement(m)
        # Arch active container if based on App Part
        #if FreeCAD.GuiUp:
        #    import FreeCADGui
        #    a = FreeCADGui.ActiveDocument.ActiveView.getActiveObject("Arch")
        #    if a:
        #        p = a.Placement.inverse().multiply(p)
        return p

    def getNormal(self):
        n = self.axis
        # Arch active container if based on App Part
        #if FreeCAD.GuiUp:
        #    import FreeCADGui
        #    a = FreeCADGui.ActiveDocument.ActiveView.getActiveObject("Arch")
        #    if a:
        #        n = a.Placement.inverse().Rotation.multVec(n)
        return n

    def setFromPlacement(self,pl,rebase=False):
        "sets the working plane from a placement (rotaton ONLY, unless rebase=True)"
        rot = FreeCAD.Placement(pl).Rotation
        self.u = rot.multVec(FreeCAD.Vector(1,0,0))
        self.v = rot.multVec(FreeCAD.Vector(0,1,0))
        self.axis = rot.multVec(FreeCAD.Vector(0,0,1))
        if rebase:
            self.position = pl.Base
        
    def inverse(self):
        "inverts the direction of the working plane"
        self.u = self.u.negative()
        self.axis = self.axis.negative()

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
        pt = point.sub(self.position)
        xv = DraftVecUtils.project(pt,self.u)
        x = xv.Length
        if xv.getAngle(self.u) > 1:
                x = -x
        yv = DraftVecUtils.project(pt,self.v)
        y = yv.Length
        if yv.getAngle(self.v) > 1:
                y = -y
        zv = DraftVecUtils.project(pt,self.axis)
        z = zv.Length
        if zv.getAngle(self.axis) > 1:
                z = -z
        return Vector(x,y,z)

    def getGlobalCoords(self,point):
        "returns the global coordinates of the given point, taken relatively to this working plane"
        vx = Vector(self.u).multiply(point.x)
        vy = Vector(self.v).multiply(point.y)
        vz = Vector(self.axis).multiply(point.z)
        pt = (vx.add(vy)).add(vz)
        return pt.add(self.position)

    def getLocalRot(self,point):
        "Same as getLocalCoords, but discards the WP position"
        xv = DraftVecUtils.project(point,self.u)
        x = xv.Length
        if xv.getAngle(self.u) > 1:
                x = -x
        yv = DraftVecUtils.project(point,self.v)
        y = yv.Length
        if yv.getAngle(self.v) > 1:
                y = -y
        zv = DraftVecUtils.project(point,self.axis)
        z = zv.Length
        if zv.getAngle(self.axis) > 1:
                z = -z
        return Vector(x,y,z)

    def getGlobalRot(self,point):
        "Same as getGlobalCoords, but discards the WP position"
        vx = Vector(self.u).multiply(point.x)
        vy = Vector(self.v).multiply(point.y)
        vz = Vector(self.axis).multiply(point.z)
        pt = (vx.add(vy)).add(vz)
        return pt
        
    def getClosestAxis(self,point):
        "returns which of the workingplane axes is closest from the given vector"
        ax = point.getAngle(self.u)
        ay = point.getAngle(self.v)
        az = point.getAngle(self.axis)
        bx = point.getAngle(self.u.negative())
        by = point.getAngle(self.v.negative())
        bz = point.getAngle(self.axis.negative())
        b = min(ax,ay,az,bx,by,bz)
        if b in [ax,bx]:
            return "x"
        elif b in [ay,by]:
            return "y"
        elif b in [az,bz]:
            return "z"
        else:
            return None
            
    def isGlobal(self):
        "returns True if the plane axes are equal to the global axes"
        if self.u != Vector(1,0,0):
            return False
        if self.v != Vector(0,1,0):
            return False
        if self.axis != Vector(0,0,1):
            return False
        return True
        
    def isOrtho(self):
        "returns True if the plane axes are following the global axes"
        if round(self.u.getAngle(Vector(0,1,0)),6) in [0,-1.570796,1.570796,-3.141593,3.141593,-4.712389,4.712389,6.283185]:
            if round(self.v.getAngle(Vector(0,1,0)),6) in [0,-1.570796,1.570796,-3.141593,3.141593,-4.712389,4.712389,6.283185]:
                if round(self.axis.getAngle(Vector(0,1,0)),6) in [0,-1.570796,1.570796,-3.141593,3.141593,-4.712389,4.712389,6.283185]:
                    return True
        return False
        
    def getDeviation(self):
        "returns the deviation angle between the u axis and the horizontal plane"
        proj = Vector(self.u.x,self.u.y,0)
        if self.u.getAngle(proj) == 0:
            return 0
        else:
            norm = proj.cross(self.u)
            return DraftVecUtils.angle(self.u,proj,norm)
                
def getPlacementFromPoints(points):
    "returns a placement from a list of 3 or 4 vectors"
    pl = plane()
    try:
            pl.position = points[0]
            pl.u = (points[1].sub(points[0]).normalize())
            pl.v = (points[2].sub(points[0]).normalize())
            if len(points) == 4:
                    pl.axis = (points[3].sub(points[0]).normalize())
            else:
                    pl.axis = ((pl.u).cross(pl.v)).normalize()
    except:
            return None
    p = pl.getPlacement()
    del pl
    return p
    
def getPlacementFromFace(face,rotated=False):
    "returns a placement from a face"
    pl = plane()
    try:
        pl.alignToFace(face)
    except:
        return None
    p = pl.getPlacement(rotated)
    del pl
    return p
