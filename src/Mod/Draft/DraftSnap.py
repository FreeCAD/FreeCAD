#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2011                                                    *  
#*   Yorik van Havre <yorik@uncreated.net>                                 *  
#*                                                                         *
#*   This program is free software; you can redistribute it and/or modify  *
#*   it under the terms of the GNU General Public License (GPL)            *
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

__title__="FreeCAD Draft Snap tools"
__author__ = "Yorik van Havre"
__url__ = "http://free-cad.sourceforge.net"


import FreeCAD, FreeCADGui, math, Draft, DraftGui, DraftTrackers, Part, SketcherGui
from DraftGui import todo
from draftlibs import fcvec,fcgeo
from FreeCAD import Vector
from pivy import coin
from PyQt4 import QtCore,QtGui

class Snapper:
    """The Snapper objects contains all the functionality used by draft
    and arch module to manage object snapping. It is responsible for
    finding snap points and displaying snap markers. Usually You
    only need to invoke it's snap() function, all the rest is taken
    care of."""

    def __init__(self):
        self.lastObj = [None,None]
        self.views = []
        self.maxEdges = 0
        self.radius = 0
        if Draft.getParam("maxSnap"):
            self.maxEdges = Draft.getParam("maxSnapEdges")

        # we still have no 3D view when the draft module initializes
        self.tracker = None
        self.extLine = None
        self.grid = None
        self.constraintAxis = None
        
        # the snapmarker has "dot","circle" and "square" available styles
        self.mk = {'passive':'circle',
                   'extension':'circle',
                   'parallel':'circle',
                   'grid':'circle',
                   'endpoint':'dot',
                   'midpoint':'square',
                   'perpendicular':'square',
                   'angle':'square',
                   'center':'dot',
                   'ortho':'square',
                   'intersection':'circle'}
        self.cursors = {'passive':None,
                        'extension':':/icons/Constraint_Parallel.svg',
                        'parallel':':/icons/Constraint_Parallel.svg',
                        'grid':':/icons/Constraint_PointOnPoint.svg',
                        'endpoint':':/icons/Constraint_PointOnPoint.svg',
                        'midpoint':':/icons/Constraint_PointOnObject.svg',
                        'perpendicular':':/icons/Constraint_PointToObject.svg',
                        'angle':':/icons/Constraint_ExternalAngle.svg',
                        'center':':/icons/Constraint_Concentric.svg',
                        'ortho':':/icons/Constraint_Perpendicular.svg',
                        'intersection':':/icons/Constraint_Tangent.svg'}
        
    def constrain(self,point,basepoint=None,axis=None):
        '''constrain(point,basepoint=None,axis=None: Returns a
        constrained point. Axis can be "x","y" or "z" or a custom vector. If None,
        the closest working plane axis will be picked.
        Basepoint is the base point used to figure out from where the point
        must be constrained. If no basepoint is given, the current point is
        used as basepoint.'''

        point = Vector(point)        
        dvec = point.sub(basepoint)

        # setting constraint axis
        if isinstance(axis,FreeCAD.Vector):
            self.constraintAxis = axis
        elif axis == "x":
            self.constraintAxis = FreeCAD.DraftWorkingPlane.u
        elif axis == "y":
            self.constraintAxis = FreeCAD.DraftWorkingPlane.v
        elif axis == "z":
            self.constraintAxis = FreeCAD.DraftWorkingPlane.axis
        else:
            self.constraintAxis = None

        # setting basepoint
        if not basepoint:
            pass
        
        affinity = FreeCAD.DraftWorkingPlane.getClosestAxis(dvec)
        if (not self.constraintAxis) or mobile:
            if affinity == "x":
                dv = fcvec.project(dvec,FreeCAD.DraftWorkingPlane.u)
                point = last.add(dv)
                if sym:
                    l = dv.Length
                    if dv.getAngle(FreeCAD.DraftWorkingPlane.u) > 1:
                        l = -l
                    point = last.add(FreeCAD.DraftWorkingPlane.getGlobalCoords(Vector(l,l,l)))
                target.constrain = 0 #x direction
                target.ui.xValue.setEnabled(True)
                target.ui.yValue.setEnabled(False)
                target.ui.zValue.setEnabled(False)
                target.ui.xValue.setFocus()
            elif affinity == "y":
                dv = fcvec.project(dvec,FreeCAD.DraftWorkingPlane.v)
                point = last.add(dv)
                if sym:
                    l = dv.Length
                    if dv.getAngle(FreeCAD.DraftWorkingPlane.v) > 1:
                        l = -l
                    point = last.add(FreeCAD.DraftWorkingPlane.getGlobalCoords(Vector(l,l,l)))
                target.constrain = 1 #y direction
                target.ui.xValue.setEnabled(False)
                target.ui.yValue.setEnabled(True)
                target.ui.zValue.setEnabled(False)
                target.ui.yValue.setFocus()
            elif affinity == "z":
                dv = fcvec.project(dvec,FreeCAD.DraftWorkingPlane.axis)
                point = last.add(dv)
                if sym:
                    l = dv.Length
                    if dv.getAngle(FreeCAD.DraftWorkingPlane.axis) > 1:
                        l = -l
                    point = last.add(FreeCAD.DraftWorkingPlane.getGlobalCoords(Vector(l,l,l)))
                target.constrain = 2 #z direction
                target.ui.xValue.setEnabled(False)
                target.ui.yValue.setEnabled(False)
                target.ui.zValue.setEnabled(True)
                target.ui.zValue.setFocus()
            else: target.constrain = 3
        elif (target.constrain == 0):
            dv = fcvec.project(dvec,FreeCAD.DraftWorkingPlane.u)
            point = last.add(dv)
            if sym:
                l = dv.Length
                if dv.getAngle(FreeCAD.DraftWorkingPlane.u) > 1:
                    l = -l
                point = last.add(FreeCAD.DraftWorkingPlane.getGlobalCoords(Vector(l,l,l)))
        elif (target.constrain == 1):
            dv = fcvec.project(dvec,FreeCAD.DraftWorkingPlane.v)
            point = last.add(dv)
            if sym:
                l = dv.Length
                if dv.getAngle(FreeCAD.DraftWorkingPlane.u) > 1:
                    l = -l
                point = last.add(FreeCAD.DraftWorkingPlane.getGlobalCoords(Vector(l,l,l)))
        elif (target.constrain == 2):
            dv = fcvec.project(dvec,FreeCAD.DraftWorkingPlane.axis)
            point = last.add(dv)
            if sym:
                l = dv.Length
                if dv.getAngle(FreeCAD.DraftWorkingPlane.u) > 1:
                    l = -l
                    point = last.add(FreeCAD.DraftWorkingPlane.getGlobalCoords(Vector(l,l,l)))			
        return point        

    def snap(self,screenpos,lastpoint=None,active=True,constrain=None):
        """snap(screenpos,lastpoint=None,active=True,constrain=None): returns a snapped
        point from the given (x,y) screenpos (the position of the mouse cursor), active is to
        activate active point snapping or not (passive), lastpoint is an optional
        other point used to draw an imaginary segment and get additional snap locations. Constrain can
        be set to 0 (horizontal) or 1 (vertical), for more additional snap locations."""

        # type conversion if needed
        if not isinstance(screenpos,tuple):
            screenpos = tuple(screenpos)

        # setup trackers if needed
        if not self.tracker:
            self.tracker = DraftTrackers.snapTracker()
        if not self.extLine:
            self.extLine = DraftTrackers.lineTracker(dotted=True)
        if (not self.grid) and Draft.getParam("grid"):
            self.grid = DraftTrackers.gridTracker()

        # getting current snap Radius
        if not self.radius:
            self.radius =  self.getScreenDist(Draft.getParam("snapRange"),screenpos)

        # set the grid
        if Draft.getParam("grid"):
            self.grid.set()
        
        # checking if alwaySnap setting is on
        oldActive = False
        if Draft.getParam("alwaysSnap"):
            oldActive = active
            active = True

        self.setCursor('passive')
        self.tracker.off()

        point = FreeCADGui.ActiveDocument.ActiveView.getPoint(screenpos[0],screenpos[1])
            
        # checking if parallel to one of the edges of the last objects
        point = self.snapToExtensions(point,lastpoint)

        # check if we snapped to something
        info = FreeCADGui.ActiveDocument.ActiveView.getObjectInfo((screenpos[0],screenpos[1]))

        if not info:
            
            # nothing has been snapped, check fro grid snap
            point = self.snapToGrid(point)
            return point

        else:

            # we have an object to snap to
            snaps = []

            obj = FreeCAD.ActiveDocument.getObject(info['Object'])
            if not obj:
                return point
                
            if hasattr(obj.ViewObject,"Selectable"):
                if not obj.ViewObject.Selectable:
                    return point
                
            if not active:
                
                # passive snapping
                snaps = [snapToVertex(info)]

            else:
                
                # active snapping
                comp = info['Component']
                if obj.isDerivedFrom("Part::Feature"):
                    if (not self.maxEdges) or (len(obj.Edges) <= self.maxEdges):
                        if "Edge" in comp:
                            # we are snapping to an edge
                            edge = obj.Shape.Edges[int(comp[4:])-1]
                            snaps.extend(self.snapToEndpoints(edge))
                            snaps.extend(self.snapToMidpoint(edge))
                            snaps.extend(self.snapToPerpendicular(edge,lastpoint))
                            snaps.extend(self.snapToOrtho(edge,lastpoint,constrain))
                            snaps.extend(self.snapToIntersection(edge))

                            if isinstance (edge.Curve,Part.Circle):
                                # the edge is an arc, we have extra options
                                snaps.extend(self.snapToAngles(edge))
                                snaps.extend(self.snapToCenter(edge))

                        elif "Vertex" in comp:
                            # directly snapped to a vertex
                            snaps.append(self.snapToVertex(info,active=True))
                        elif comp == '':
                            # workaround for the new view provider
                            snaps.append(self.snapToVertex(info,active=True))
                        else:
                            # all other cases (face, etc...) default to passive snap
                            snapArray = [self.snapToVertex(info)]
                            
                elif Draft.getType(obj) == "Dimension":
                    # for dimensions we snap to their 3 points
                    for pt in [obj.Start,obj.End,obj.Dimline]:
                        snaps.append([pt,'endpoint',pt])
                        
                elif Draft.getType(obj) == "Mesh":
                    # for meshes we only snap to vertices
                    snaps.extend(self.snapToEndpoints(obj.Mesh))

            # updating last objects list
            if not self.lastObj[0]:
                self.lastObj[0] = obj.Name
                self.lastObj[1] = obj.Name
            if (self.lastObj[1] != obj.Name):
                self.lastObj[0] = lastObj[1]
                self.lastObj[1] = obj.Name

            # calculating the nearest snap point
            shortest = 1000000000000000000
            origin = Vector(info['x'],info['y'],info['z'])
            winner = [Vector(0,0,0),None,Vector(0,0,0)]
            for snap in snaps:
                # if snap[0] == None: print "debug: Snapper: 'i[0]' is 'None'"
                delta = snap[0].sub(origin)
                if delta.Length < shortest:
                    shortest = delta.Length
                    winner = snap
            if self.radius != 0:
                dv = point.sub(winner[2])
                if (not oldActive) and (dv.Length > self.radius):
                    winner = snapToVertex(info)

            # setting the cursors
            self.tracker.setCoords(winner[2])
            self.tracker.setMarker(self.mk[winner[1]])
            self.tracker.on()
            self.setCursor(winner[1])

            # return the final point
            return winner[2]
        
    def snapToExtensions(self,point,last):
        "returns a point snapped to extension or parallel line to last object, if any"
        for o in [self.lastObj[1],self.lastObj[0]]:
            if o:
                ob = FreeCAD.ActiveDocument.getObject(o)
                if ob:
                    edges = ob.Shape.Edges
                    if (not self.maxEdges) or (len(edges) <= self.maxEdges):
                        for e in edges:
                            if isinstance(e.Curve,Part.Line):
                                np = self.getPerpendicular(e,point)
                                if (np.sub(point)).Length < self.radius:
                                    self.tracker.setCoords(np)
                                    self.tracker.setMarker(self.mk['extension'])
                                    self.tracker.on()
                                    self.extLine.p1(e.Vertexes[0].Point)
                                    self.extLine.p2(np)
                                    self.extLine.on()
                                    self.setCursor('extension')
                                    return np
                                else:
                                    if last:
                                        de = Part.Line(last,last.add(fcgeo.vec(e))).toShape()  
                                        np = self.getPerpendicular(de,point)
                                        if (np.sub(point)).Length < self.radius:
                                            self.tracker.setCoords(np)
                                            self.tracker.setMarker(self.mk['parallel'])
                                            self.tracker.on()
                                            self.setCursor('extension')
                                            return np
        return point

    def snapToGrid(self,point):
        "returns a grid snap point if available"
        if self.grid:
            np = self.grid.getClosestNode(point)
            if np:
                if self.radius != 0:
                    dv = point.sub(np)
                    if dv.Length <= self.radius:
                        self.tracker.setCoords(np)
                        self.tracker.setMarker(self.mk['grid'])
                        self.tracker.on()
                        self.setCursor('grid')
                        return np
        return point

    def snapToEndpoints(self,shape):
        "returns a list of enpoints snap locations"
        snaps = []
        if hasattr(shape,"Vertexes"):
            for v in shape.Vertexes:
                snaps.append([v.Point,'endpoint',v.Point])
        elif hasattr(shape,"Point"):
            snaps.append([shape.Point,'endpoint',shape.Point])
        elif hasattr(shape,"Points"):
            for v in shape.Points:
                snaps.append([v.Vector,'endpoint',v.Vector])
        return snaps

    def snapToMidpoint(self,shape):
        "returns a list of midpoints snap locations"
        snaps = []
        if isinstance(shape,Part.Edge):
            mp = fcgeo.findMidpoint(shape)
            if mp:
                snaps.append([mp,'midpoint',mp])
        return snaps

    def snapToPerpendicular(self,shape,last):
        "returns a list of perpendicular snap locations"
        snaps = []
        if last:
            if isinstance(shape,Part.Edge):
                if isinstance(shape.Curve,Part.Line):
                    np = self.getPerpendicular(shape,last)
                elif isinstance(shape.Curve,Part.Circle):
                    dv = last.sub(shape.Curve.Center)
                    dv = fcvec.scaleTo(dv,shape.Curve.Radius)
                    np = (shape.Curve.Center).add(dv)
                snaps.append([np,'perpendicular',np])
        return snaps

    def snapToOrtho(self,shape,last,constrain):
        "returns a list of ortho snap locations"
        snaps = []
        if constrain != None:
            if isinstance(shape,Part.Edge):
                if last:
                    if isinstance(shape.Curve,Part.Line):
                        p1 = shape.Vertexes[0].Point
                        p2 = shape.Vertexes[-1].Point
                        if (constrain == 0):
                            if ((last.y > p1.y) and (last.y < p2.y) or (last.y > p2.y) and (last.y < p1.y)):
                                pc = (last.y-p1.y)/(p2.y-p1.y)
                                cp = (Vector(p1.x+pc*(p2.x-p1.x),p1.y+pc*(p2.y-p1.y),p1.z+pc*(p2.z-p1.z)))
                                snaps.append([cp,'ortho',cp])
                        elif (constrain == 1):
                            if ((last.x > p1.x) and (last.x < p2.x) or (last.x > p2.x) and (last.x < p1.x)):
                                pc = (last.x-p1.x)/(p2.x-p1.x)
                                cp = (Vector(p1.x+pc*(p2.x-p1.x),p1.y+pc*(p2.y-p1.y),p1.z+pc*(p2.z-p1.z)))
                                snaps.append([cp,'ortho',cp])
        return snaps

    def snapToAngles(self,shape):
        "returns a list of angle snap locations"
        snaps = []
        rad = shape.Curve.Radius
        pos = shape.Curve.Center
        for i in [0,30,45,60,90,120,135,150,180,210,225,240,270,300,315,330]:
            ang = math.radians(i)
            cur = Vector(math.sin(ang)*rad+pos.x,math.cos(ang)*rad+pos.y,pos.z)
            snaps.append([cur,'angle',cur])
        return snaps

    def snapToCenter(self,shape):
        "returns a list of center snap locations"
        snaps = []
        rad = shape.Curve.Radius
        pos = shape.Curve.Center
        for i in [15,37.5,52.5,75,105,127.5,142.5,165,195,217.5,232.5,255,285,307.5,322.5,345]:
            ang = math.radians(i)
            cur = Vector(math.sin(ang)*rad+pos.x,math.cos(ang)*rad+pos.y,pos.z)
            snaps.append([cur,'center',pos])
        return snaps

    def snapToIntersection(self,shape):
        "returns a list of intersection snap locations"
        snaps = []
        # get the stored objects to calculate intersections
        intedges = []
        if self.lastObj[0]:
            obj = FreeCAD.ActiveDocument.getObject(self.lastObj[0])
            if obj:
                if obj.isDerivedFrom("Part::Feature"):
                    if (not self.maxEdges) or (len(obj.Shape.Edges) <= self.maxEdges):
                        for e in obj.Shape.Edges:
                            # get the intersection points
                            pt = fcgeo.findIntersection(e,shape)
                            if pt:
                                for p in pt:
                                    snaps.append([p,'intersection',p])
        return snaps

    def snapToVertex(self,info,active=False):
        p = Vector(info['x'],info['y'],info['z'])
        if active:
            return [p,'endpoint',p]
        else:
            return [p,'passive',p]
        
    def getScreenDist(self,dist,cursor):
        "returns a distance in 3D space from a screen pixels distance"
        print cursor
        p1 = FreeCADGui.ActiveDocument.ActiveView.getPoint(cursor)
        p2 = FreeCADGui.ActiveDocument.ActiveView.getPoint((cursor[0]+dist,cursor[1]))
        return (p2.sub(p1)).Length

    def getPerpendicular(self,edge,pt):
        "returns a point on an edge, perpendicular to the given point"
        dv = pt.sub(edge.Vertexes[0].Point)
        nv = fcvec.project(dv,fcgeo.vec(edge))
        np = (edge.Vertexes[0].Point).add(nv)
        return np

    def setCursor(self,mode=None):        
        if not mode:
            for v in self.views:
                v.unsetCursor()
            self.views = []
        else:
            if not self.views:
                mw = DraftGui.getMainWindow()
                self.views = mw.findChildren(QtGui.QWidget,"QtGLArea")
            baseicon = QtGui.QPixmap(":/icons/Draft_Cursor.svg")
            newicon = QtGui.QPixmap(32,24)
            newicon.fill(QtCore.Qt.transparent)
            qp = QtGui.QPainter()
            qp.begin(newicon)
            qp.drawPixmap(0,0,baseicon)
            if not (mode == 'passive'):
                tp = QtGui.QPixmap(self.cursors[mode]).scaledToWidth(16)
                qp.drawPixmap(QtCore.QPoint(16, 8), tp);
            qp.end()
            cur = QtGui.QCursor(newicon,8,8)
            for v in self.views:
                v.setCursor(cur)
    
    def off(self):
        "finishes snapping"
        if self.tracker:
            self.tracker.off()
        if self.extLine:
            self.extLine.off()
        if self.grid:
            self.grid.off()
        self.radius = 0
        self.setCursor()

    def constrainOff(self):
        pass
               
# deprecated ##################################################################

# last snapped objects, for quick intersection calculation
lastObj = [0,0]
        
def snapPoint(target,point,cursor,ctrl=False):
    '''
    Snap function used by the Draft tools
    
    Currently has two modes: passive and active. Pressing CTRL while 
    clicking puts you in active mode:
    
    - In passive mode (an open circle appears), your point is
    snapped to the nearest point on any underlying geometry.
    
    - In active mode (ctrl pressed, a filled circle appears), your point
    can currently be snapped to the following points:
        - Nodes and midpoints of all Part shapes
        - Nodes and midpoints of lines/wires
        - Centers and quadrant points of circles
        - Endpoints of arcs
        - Intersection between line, wires segments, arcs and circles
        - When constrained (SHIFT pressed), Intersections between
        constraining axis and lines/wires
    '''
        
    def getConstrainedPoint(edge,last,constrain):
        "check for constrained snappoint"
        p1 = edge.Vertexes[0].Point
        p2 = edge.Vertexes[-1].Point
        ar = []
        if (constrain == 0):
            if ((last.y > p1.y) and (last.y < p2.y) or (last.y > p2.y) and (last.y < p1.y)):
                pc = (last.y-p1.y)/(p2.y-p1.y)
                cp = (Vector(p1.x+pc*(p2.x-p1.x),p1.y+pc*(p2.y-p1.y),p1.z+pc*(p2.z-p1.z)))
                ar.append([cp,1,cp]) # constrainpoint
        if (constrain == 1):
            if ((last.x > p1.x) and (last.x < p2.x) or (last.x > p2.x) and (last.x < p1.x)):
                pc = (last.x-p1.x)/(p2.x-p1.x)
                cp = (Vector(p1.x+pc*(p2.x-p1.x),p1.y+pc*(p2.y-p1.y),p1.z+pc*(p2.z-p1.z)))
                ar.append([cp,1,cp]) # constrainpoint
        return ar

    def getPassivePoint(info):
        "returns a passive snap point"
        cur = Vector(info['x'],info['y'],info['z'])
        return [cur,2,cur]

    def getScreenDist(dist,cursor):
        "returns a 3D distance from a screen pixels distance"
        p1 = FreeCADGui.ActiveDocument.ActiveView.getPoint(cursor)
        p2 = FreeCADGui.ActiveDocument.ActiveView.getPoint((cursor[0]+dist,cursor[1]))
        return (p2.sub(p1)).Length

    def getGridSnap(target,point):
        "returns a grid snap point if available"
        if target.grid:
            return target.grid.getClosestNode(point)
        return None

    def getPerpendicular(edge,last):
        "returns a point on an edge, perpendicular to the given point"
        dv = last.sub(edge.Vertexes[0].Point)
        nv = fcvec.project(dv,fcgeo.vec(edge))
        np = (edge.Vertexes[0].Point).add(nv)
        return np

    # checking if alwaySnap setting is on
    extractrl = False
    if Draft.getParam("alwaysSnap"):
        extractrl = ctrl
        ctrl = True                

    # setting Radius
    radius =  getScreenDist(Draft.getParam("snapRange"),cursor)
	
    # checking if parallel to one of the edges of the last objects
    target.snap.off()
    target.extsnap.off()
    if (len(target.node) > 0):
        for o in [lastObj[1],lastObj[0]]:
            if o:
                ob = target.doc.getObject(o)
                if ob:
                    edges = ob.Shape.Edges
                    if len(edges)<10:
                        for e in edges:
                            if isinstance(e.Curve,Part.Line):
                                last = target.node[len(target.node)-1]
                                de = Part.Line(last,last.add(fcgeo.vec(e))).toShape()
                                np = getPerpendicular(e,point)
                                if (np.sub(point)).Length < radius:
                                    target.snap.coords.point.setValue((np.x,np.y,np.z))
                                    target.snap.setMarker("circle")
                                    target.snap.on()
                                    target.extsnap.p1(e.Vertexes[0].Point)
                                    target.extsnap.p2(np)
                                    target.extsnap.on()
                                    point = np
                                else:
                                    last = target.node[len(target.node)-1]
                                    de = Part.Line(last,last.add(fcgeo.vec(e))).toShape()  
                                    np = getPerpendicular(de,point)
                                    if (np.sub(point)).Length < radius:
                                        target.snap.coords.point.setValue((np.x,np.y,np.z))
                                        target.snap.setMarker("circle")
                                        target.snap.on()
                                        point = np

    # check if we snapped to something
    snapped=target.view.getObjectInfo((cursor[0],cursor[1]))

    if (snapped == None):
        # nothing has been snapped, check fro grid snap
        gpt = getGridSnap(target,point)
        if gpt:
            if radius != 0:
                dv = point.sub(gpt)
                if dv.Length <= radius:
                    target.snap.coords.point.setValue((gpt.x,gpt.y,gpt.z))
                    target.snap.setMarker("point")
                    target.snap.on()  
                    return gpt
        return point
    else:
        # we have something to snap
        obj = target.doc.getObject(snapped['Object'])
        if hasattr(obj.ViewObject,"Selectable"):
                        if not obj.ViewObject.Selectable:
                                return point
        if not ctrl:
                        # are we in passive snap?
                        snapArray = [getPassivePoint(snapped)]
        else:
            snapArray = []
            comp = snapped['Component']
            if obj.isDerivedFrom("Part::Feature"):
                if "Edge" in comp:
                    # get the stored objects to calculate intersections
                    intedges = []
                    if lastObj[0]:
                        lo = target.doc.getObject(lastObj[0])
                        if lo:
                            if lo.isDerivedFrom("Part::Feature"):
                                intedges = lo.Shape.Edges
                                                           
                    nr = int(comp[4:])-1
                    edge = obj.Shape.Edges[nr]
                    for v in edge.Vertexes:
                        snapArray.append([v.Point,0,v.Point])
                    if isinstance(edge.Curve,Part.Line):
                        # the edge is a line
                        midpoint = fcgeo.findMidpoint(edge)
                        snapArray.append([midpoint,1,midpoint])
                        if (len(target.node) > 0):
                            last = target.node[len(target.node)-1]
                            snapArray.extend(getConstrainedPoint(edge,last,target.constrain))
                            np = getPerpendicular(edge,last)
                            snapArray.append([np,1,np])

                    elif isinstance (edge.Curve,Part.Circle):
                        # the edge is an arc
                        rad = edge.Curve.Radius
                        pos = edge.Curve.Center
                        for i in [0,30,45,60,90,120,135,150,180,210,225,240,270,300,315,330]:
                            ang = math.radians(i)
                            cur = Vector(math.sin(ang)*rad+pos.x,math.cos(ang)*rad+pos.y,pos.z)
                            snapArray.append([cur,1,cur])
                        for i in [15,37.5,52.5,75,105,127.5,142.5,165,195,217.5,232.5,255,285,307.5,322.5,345]:
                            ang = math.radians(i)
                            cur = Vector(math.sin(ang)*rad+pos.x,math.cos(ang)*rad+pos.y,pos.z)
                            snapArray.append([cur,0,pos])

                    for e in intedges:
                        # get the intersection points
                        pt = fcgeo.findIntersection(e,edge)
                        if pt:
                            for p in pt:
                                snapArray.append([p,3,p])
                elif "Vertex" in comp:
                    # directly snapped to a vertex
                    p = Vector(snapped['x'],snapped['y'],snapped['z'])
                    snapArray.append([p,0,p])
                elif comp == '':
                    # workaround for the new view provider
                    p = Vector(snapped['x'],snapped['y'],snapped['z'])
                    snapArray.append([p,2,p])
                else:
                    snapArray = [getPassivePoint(snapped)]
            elif Draft.getType(obj) == "Dimension":
                for pt in [obj.Start,obj.End,obj.Dimline]:
                    snapArray.append([pt,0,pt])
            elif Draft.getType(obj) == "Mesh":
                for v in obj.Mesh.Points:
                    snapArray.append([v.Vector,0,v.Vector])
        if not lastObj[0]:
            lastObj[0] = obj.Name
            lastObj[1] = obj.Name
        if (lastObj[1] != obj.Name):
            lastObj[0] = lastObj[1]
            lastObj[1] = obj.Name

        # calculating shortest distance
        shortest = 1000000000000000000
        spt = Vector(snapped['x'],snapped['y'],snapped['z'])
        newpoint = [Vector(0,0,0),0,Vector(0,0,0)]
        for pt in snapArray:
            if pt[0] == None: print "snapPoint: debug 'i[0]' is 'None'"
            di = pt[0].sub(spt)
            if di.Length < shortest:
                shortest = di.Length
                newpoint = pt
        if radius != 0:
            dv = point.sub(newpoint[2])
            if (not extractrl) and (dv.Length > radius):
                newpoint = getPassivePoint(snapped)
        target.snap.coords.point.setValue((newpoint[2].x,newpoint[2].y,newpoint[2].z))
        if (newpoint[1] == 1):
            target.snap.setMarker("square")
        elif (newpoint[1] == 0):
            target.snap.setMarker("point")
        elif (newpoint[1] == 3):
            target.snap.setMarker("square")
        else:
            target.snap.setMarker("circle")
        target.snap.on()                                
        return newpoint[2]

def constrainPoint (target,pt,mobile=False,sym=False):
    '''
    Constrain function used by the Draft tools
    On commands that need to enter several points (currently only line/wire),
    you can constrain the next point to be picked to the last drawn point by
    pressing SHIFT. The vertical or horizontal constraining depends on the
    position of your mouse in relation to last point at the moment you press
    SHIFT. if mobile=True, mobile behaviour applies. If sym=True, x alway = y
    '''
    point = Vector(pt)
    if len(target.node) > 0:
        last = target.node[-1]
        dvec = point.sub(last)
        affinity = FreeCAD.DraftWorkingPlane.getClosestAxis(dvec)
        if ((target.constrain == None) or mobile):
            if affinity == "x":
                dv = fcvec.project(dvec,FreeCAD.DraftWorkingPlane.u)
                point = last.add(dv)
                if sym:
                    l = dv.Length
                    if dv.getAngle(FreeCAD.DraftWorkingPlane.u) > 1:
                        l = -l
                    point = last.add(FreeCAD.DraftWorkingPlane.getGlobalCoords(Vector(l,l,l)))
                target.constrain = 0 #x direction
                target.ui.xValue.setEnabled(True)
                target.ui.yValue.setEnabled(False)
                target.ui.zValue.setEnabled(False)
                target.ui.xValue.setFocus()
            elif affinity == "y":
                dv = fcvec.project(dvec,FreeCAD.DraftWorkingPlane.v)
                point = last.add(dv)
                if sym:
                    l = dv.Length
                    if dv.getAngle(FreeCAD.DraftWorkingPlane.v) > 1:
                        l = -l
                    point = last.add(FreeCAD.DraftWorkingPlane.getGlobalCoords(Vector(l,l,l)))
                target.constrain = 1 #y direction
                target.ui.xValue.setEnabled(False)
                target.ui.yValue.setEnabled(True)
                target.ui.zValue.setEnabled(False)
                target.ui.yValue.setFocus()
            elif affinity == "z":
                dv = fcvec.project(dvec,FreeCAD.DraftWorkingPlane.axis)
                point = last.add(dv)
                if sym:
                    l = dv.Length
                    if dv.getAngle(FreeCAD.DraftWorkingPlane.axis) > 1:
                        l = -l
                    point = last.add(FreeCAD.DraftWorkingPlane.getGlobalCoords(Vector(l,l,l)))
                target.constrain = 2 #z direction
                target.ui.xValue.setEnabled(False)
                target.ui.yValue.setEnabled(False)
                target.ui.zValue.setEnabled(True)
                target.ui.zValue.setFocus()
            else: target.constrain = 3
        elif (target.constrain == 0):
            dv = fcvec.project(dvec,FreeCAD.DraftWorkingPlane.u)
            point = last.add(dv)
            if sym:
                l = dv.Length
                if dv.getAngle(FreeCAD.DraftWorkingPlane.u) > 1:
                    l = -l
                point = last.add(FreeCAD.DraftWorkingPlane.getGlobalCoords(Vector(l,l,l)))
        elif (target.constrain == 1):
            dv = fcvec.project(dvec,FreeCAD.DraftWorkingPlane.v)
            point = last.add(dv)
            if sym:
                l = dv.Length
                if dv.getAngle(FreeCAD.DraftWorkingPlane.u) > 1:
                    l = -l
                point = last.add(FreeCAD.DraftWorkingPlane.getGlobalCoords(Vector(l,l,l)))
        elif (target.constrain == 2):
            dv = fcvec.project(dvec,FreeCAD.DraftWorkingPlane.axis)
            point = last.add(dv)
            if sym:
                l = dv.Length
                if dv.getAngle(FreeCAD.DraftWorkingPlane.u) > 1:
                    l = -l
                point = last.add(FreeCAD.DraftWorkingPlane.getGlobalCoords(Vector(l,l,l)))			
    return point

if not hasattr(FreeCADGui,"Snapper"):
    FreeCADGui.Snapper = Snapper()
