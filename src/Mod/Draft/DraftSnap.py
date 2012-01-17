#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2011                                                    *  
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

__title__="FreeCAD Draft Snap tools"
__author__ = "Yorik van Havre"
__url__ = "http://free-cad.sourceforge.net"


import FreeCAD, FreeCADGui, math, Draft, DraftGui, DraftTrackers
from DraftGui import todo
from draftlibs import fcvec
from FreeCAD import Vector
from pivy import coin
from PyQt4 import QtCore,QtGui

class Snapper:
    """The Snapper objects contains all the functionality used by draft
    and arch module to manage object snapping. It is responsible for
    finding snap points and displaying snap markers. Usually You
    only need to invoke it's snap() function, all the rest is taken
    care of.

    3 functions are useful for the scriptwriter: snap(), constrain()
    or getPoint() which is an all-in-one combo.

    The indivudual snapToXXX() functions return a snap definition in
    the form [real_point,marker_type,visual_point], and are not
    meant to be used directly, they are all called when necessary by
    the general snap() function.
    
    """

    def __init__(self):
        self.lastObj = [None,None]
        self.views = []
        self.maxEdges = 0
        self.radius = 0
        self.constraintAxis = None
        self.basepoint = None
        self.affinity = None
        self.cursorMode = None
        if Draft.getParam("maxSnap"):
            self.maxEdges = Draft.getParam("maxSnapEdges")

        # we still have no 3D view when the draft module initializes
        self.tracker = None
        self.extLine = None
        self.grid = None
        self.constrainLine = None
        self.trackLine = None
        
        # the snapmarker has "dot","circle" and "square" available styles
        self.mk = {'passive':'circle',
                   'extension':'circle',
                   'parallel':'circle',
                   'grid':'circle',
                   'endpoint':'dot',
                   'midpoint':'dot',
                   'perpendicular':'dot',
                   'angle':'dot',
                   'center':'dot',
                   'ortho':'dot',
                   'intersection':'dot'}
        self.cursors = {'passive':None,
                        'extension':':/icons/Constraint_Parallel.svg',
                        'parallel':':/icons/Constraint_Parallel.svg',
                        'grid':':/icons/Constraint_PointOnPoint.svg',
                        'endpoint':':/icons/Constraint_PointOnEnd.svg',
                        'midpoint':':/icons/Constraint_PointOnObject.svg',
                        'perpendicular':':/icons/Constraint_PointToObject.svg',
                        'angle':':/icons/Constraint_ExternalAngle.svg',
                        'center':':/icons/Constraint_Concentric.svg',
                        'ortho':':/icons/Constraint_Perpendicular.svg',
                        'intersection':':/icons/Constraint_Tangent.svg'}
        
    def snap(self,screenpos,lastpoint=None,active=True,constrain=False):
        """snap(screenpos,lastpoint=None,active=True,constrain=False): returns a snapped
        point from the given (x,y) screenpos (the position of the mouse cursor), active is to
        activate active point snapping or not (passive), lastpoint is an optional
        other point used to draw an imaginary segment and get additional snap locations. Constrain can
        be True to constrain the point against the closest working plane axis.
        Screenpos can be a list, a tuple or a coin.SbVec2s object."""

        global Part,fcgeo
        import Part, SketcherGui
        from draftlibs import fcgeo

        def cstr(point):
            "constrains if needed"
            if constrain:
                return self.constrain(point,lastpoint)
            else:
                self.unconstrain()
                return point

        snaps = []
        
        # type conversion if needed
        if isinstance(screenpos,list):
            screenpos = tuple(screenpos)
        elif isinstance(screenpos,coin.SbVec2s):
            screenpos = tuple(screenpos.getValue())
        elif  not isinstance(screenpos,tuple):
            print "snap needs valid screen position (list, tuple or sbvec2s)"
            return None

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
        if self.grid and Draft.getParam("grid"):
            self.grid.set()
        
        # checking if alwaySnap setting is on
        oldActive = False
        if Draft.getParam("alwaysSnap"):
            oldActive = active
            active = True

        self.setCursor('passive')
        if self.tracker:
            self.tracker.off()
        if self.extLine:
            self.extLine.off()

        point = self.getApparentPoint(screenpos[0],screenpos[1])
            
        # check if we snapped to something
        info = FreeCADGui.ActiveDocument.ActiveView.getObjectInfo((screenpos[0],screenpos[1]))

        # checking if parallel to one of the edges of the last objects
        point = self.snapToExtensions(point,lastpoint,constrain)
        
        if not info:
            
            # nothing has been snapped, check fro grid snap
            point = self.snapToGrid(point)
            return cstr(point)

        else:

            # we have an object to snap to

            obj = FreeCAD.ActiveDocument.getObject(info['Object'])
            if not obj:
                return cstr(point)
                
            if hasattr(obj.ViewObject,"Selectable"):
                if not obj.ViewObject.Selectable:
                    return cstr(point)
                
            if not active:
                
                # passive snapping
                snaps = [self.snapToVertex(info)]

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
            if not self.lastObj[1]:
                self.lastObj[1] = obj.Name
            elif self.lastObj[1] != obj.Name:
                self.lastObj[0] = self.lastObj[1]
                self.lastObj[1] = obj.Name

            if not snaps:
                return point

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

            # see if we are out of the max radius, if any
            if self.radius:
                dv = point.sub(winner[2])
                if (dv.Length > self.radius):
                    if not oldActive:
                        winner = self.snapToVertex(info)

            # setting the cursors
            if self.tracker:
                self.tracker.setCoords(winner[2])
                self.tracker.setMarker(self.mk[winner[1]])
                self.tracker.on()
            self.setCursor(winner[1])
            
            # return the final point
            return cstr(winner[2])

    def getApparentPoint(self,x,y):
        "returns a 3D point, projected on the current working plane"
        pt = FreeCADGui.ActiveDocument.ActiveView.getPoint(x,y)
        dv = FreeCADGui.ActiveDocument.ActiveView.getViewDirection()
        return FreeCAD.DraftWorkingPlane.projectPoint(pt,dv)
        
    def snapToExtensions(self,point,last,constrain):
        "returns a point snapped to extension or parallel line to last object, if any"

        tsnap = self.snapToExtOrtho(last,constrain)
        if tsnap:
            if (tsnap[0].sub(point)).Length < self.radius:
                if self.tracker:
                    self.tracker.setCoords(tsnap[2])
                    self.tracker.setMarker(self.mk[tsnap[1]])
                    self.tracker.on()
                if self.extLine:
                    self.extLine.p2(tsnap[2])
                    self.extLine.on()
                self.setCursor(tsnap[1])
                return tsnap[2]
                
        for o in [self.lastObj[1],self.lastObj[0]]:
            if o:
                ob = FreeCAD.ActiveDocument.getObject(o)
                if ob:
                    if ob.isDerivedFrom("Part::Feature"):
                        edges = ob.Shape.Edges
                        if (not self.maxEdges) or (len(edges) <= self.maxEdges):
                            for e in edges:
                                if isinstance(e.Curve,Part.Line):
                                    np = self.getPerpendicular(e,point)
                                    if not fcgeo.isPtOnEdge(np,e):
                                        if (np.sub(point)).Length < self.radius:
                                            if self.tracker:
                                                self.tracker.setCoords(np)
                                                self.tracker.setMarker(self.mk['extension'])
                                                self.tracker.on()
                                            if self.extLine:
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
                                                    if self.tracker:
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
                        if self.tracker:
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
                elif isinstance(shape.Curve,Part.BSplineCurve):
                    pr = shape.Curve.parameter(last)
                    np = shape.Curve.value(pr)
                else:
                    return snaps
                snaps.append([np,'perpendicular',np])
        return snaps

    def snapToOrtho(self,shape,last,constrain):
        "returns a list of ortho snap locations"
        snaps = []
        if constrain:
            if isinstance(shape,Part.Edge):
                if last:
                    if isinstance(shape.Curve,Part.Line):
                        if self.constraintAxis:
                            tmpEdge = Part.Line(last,last.add(self.constraintAxis)).toShape()
                            # get the intersection points
                            pt = fcgeo.findIntersection(tmpEdge,shape,True,True)
                            if pt:
                                for p in pt:
                                    snaps.append([p,'ortho',p])
        return snaps

    def snapToExtOrtho(self,last,constrain):
        "returns an ortho X extension snap location"
        if constrain and last and self.constraintAxis and self.extLine:
            tmpEdge1 = Part.Line(last,last.add(self.constraintAxis)).toShape()
            tmpEdge2 = Part.Line(self.extLine.p1(),self.extLine.p2()).toShape()
            # get the intersection points
            pt = fcgeo.findIntersection(tmpEdge1,tmpEdge2,True,True)
            if pt:
                    return [pt[0],'ortho',pt[0]]
        return None
    
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
        "setCursor(self,mode=None): sets or resets the cursor to the given mode or resets"
        
        if not mode:
            for v in self.views:
                v.unsetCursor()
            self.views = []
        else:
            if mode != self.cursorMode:
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
                self.cursorMode = mode
    
    def off(self):
        "finishes snapping"
        if self.tracker:
            self.tracker.off()
        if self.extLine:
            self.extLine.off()
        if self.grid:
            self.grid.off()
        if self.constrainLine:
            self.constrainLine.off()
        self.radius = 0
        self.setCursor()
        self.cursorMode = None

    def constrain(self,point,basepoint=None,axis=None):
        '''constrain(point,basepoint=None,axis=None: Returns a
        constrained point. Axis can be "x","y" or "z" or a custom vector. If None,
        the closest working plane axis will be picked.
        Basepoint is the base point used to figure out from where the point
        must be constrained. If no basepoint is given, the current point is
        used as basepoint.'''

        point = Vector(point)

        # setup trackers if needed
        if not self.constrainLine:
            self.constrainLine = DraftTrackers.lineTracker(dotted=True)

        # setting basepoint
        if not basepoint:
            if not self.basepoint:
                self.basepoint = point
        else:
            self.basepoint = basepoint
        delta = point.sub(self.basepoint)

        # setting constraint axis
        if not self.affinity:
            self.affinity = FreeCAD.DraftWorkingPlane.getClosestAxis(delta)
        if isinstance(axis,FreeCAD.Vector):
            self.constraintAxis = axis
        elif axis == "x":
            self.constraintAxis = FreeCAD.DraftWorkingPlane.u
        elif axis == "y":
            self.constraintAxis = FreeCAD.DraftWorkingPlane.v
        elif axis == "z":
            self.constraintAxis = FreeCAD.DraftWorkingPlane.axis
        else:
            if self.affinity == "x":
                self.constraintAxis = FreeCAD.DraftWorkingPlane.u
            elif self.affinity == "y":
                self.constraintAxis = FreeCAD.DraftWorkingPlane.v
            else:
                self.constraintAxis = FreeCAD.DraftWorkingPlane.axis
                
        # calculating constrained point
        cdelta = fcvec.project(delta,self.constraintAxis)
        npoint = self.basepoint.add(cdelta)

        # setting constrain line
        if self.constrainLine:
            if point != npoint:
                self.constrainLine.p1(point)
                self.constrainLine.p2(npoint)
                self.constrainLine.on()
            else:
                self.constrainLine.off()
		
        return npoint       

    def unconstrain(self):
        self.basepoint = None
        self.affinity = None
        if self.constrainLine:
            self.constrainLine.off()

    def getPoint(self,last=None,callback=None):
        """getPoint([last],[callback]) : gets a 3D point from the screen. You can provide an existing point,
        in that case additional snap options and a tracker are available. You can also passa function as
        callback, which will get called with the resulting point as argument, when a point is clicked:

        def cb(point):
            print "got a 3D point: ",point
        FreeCADGui.Snapper.getPoint(callback=cb)
        """
        self.pt = None
        self.ui = FreeCADGui.draftToolBar
        self.view = FreeCADGui.ActiveDocument.ActiveView

        # setting a track line if we got an existing point
        if last:
            if not self.trackLine:
                self.trackLine = DraftTrackers.lineTracker()
            self.trackLine.p1(last)
            self.trackLine.on()

        def move(event_cb):
            event = event_cb.getEvent()
            mousepos = event.getPosition()
            ctrl = event.wasCtrlDown()
            shift = event.wasShiftDown()
            self.pt = FreeCADGui.Snapper.snap(mousepos,lastpoint=last,active=ctrl,constrain=shift)
            self.ui.displayPoint(self.pt,last,plane=FreeCAD.DraftWorkingPlane,mask=FreeCADGui.Snapper.affinity)
            if self.trackLine:
                self.trackLine.p2(self.pt)
        
        def click(event_cb):
            event = event_cb.getEvent()
            if event.getState() == coin.SoMouseButtonEvent.DOWN:
                self.view.removeEventCallbackPivy(coin.SoMouseButtonEvent.getClassTypeId(),self.callbackClick)
                self.view.removeEventCallbackPivy(coin.SoLocation2Event.getClassTypeId(),self.callbackMove)
                FreeCADGui.Snapper.off()
                self.ui.offUi()
                if self.trackLine:
                    self.trackLine.off()
                if callback:
                    callback(self.pt)
                self.pt = None

        # adding 2 callback functions
        self.ui.pointUi()
        self.callbackClick = self.view.addEventCallbackPivy(coin.SoMouseButtonEvent.getClassTypeId(),click)
        self.callbackMove = self.view.addEventCallbackPivy(coin.SoLocation2Event.getClassTypeId(),move)            
            
if not hasattr(FreeCADGui,"Snapper"):
    FreeCADGui.Snapper = Snapper()
