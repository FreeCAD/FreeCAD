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


import FreeCAD, FreeCADGui, math, Draft, DraftGui, DraftTrackers, DraftVecUtils
from DraftGui import todo,getMainWindow
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

    The Snapper lives inside FreeCADGui once the Draft module has been
    loaded.
    
    """

    def __init__(self):
        self.lastObj = [None,None]
        self.views = []
        self.maxEdges = 0
        self.radius = 0
        self.constraintAxis = None
        self.basepoint = None
        self.affinity = None
        self.mask = None
        self.cursorMode = None
        if Draft.getParam("maxSnap"):
            self.maxEdges = Draft.getParam("maxSnapEdges")

        # we still have no 3D view when the draft module initializes
        self.tracker = None
        self.extLine = None
        self.grid = None
        self.constrainLine = None
        self.trackLine = None
        self.radiusTracker = None
        self.snapInfo = None
        self.lastSnappedObject = None
        self.active = True
        self.forceGridOff = False
        self.trackers = [[],[],[],[],[]] # view, grid, snap, extline, radius

        self.polarAngles = [90,45]
        
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
        self.cursors = {'passive':':/icons/Snap_Near.svg',
                        'extension':':/icons/Snap_Extension.svg',
                        'parallel':':/icons/Snap_Parallel.svg',
                        'grid':':/icons/Snap_Grid.svg',
                        'endpoint':':/icons/Snap_Endpoint.svg',
                        'midpoint':':/icons/Snap_Midpoint.svg',
                        'perpendicular':':/icons/Snap_Perpendicular.svg',
                        'angle':':/icons/Snap_Angle.svg',
                        'center':':/icons/Snap_Center.svg',
                        'ortho':':/icons/Snap_Ortho.svg',
                        'intersection':':/icons/Snap_Intersection.svg'}
        
    def snap(self,screenpos,lastpoint=None,active=True,constrain=False):
        """snap(screenpos,lastpoint=None,active=True,constrain=False): returns a snapped
        point from the given (x,y) screenpos (the position of the mouse cursor), active is to
        activate active point snapping or not (passive), lastpoint is an optional
        other point used to draw an imaginary segment and get additional snap locations. Constrain can
        be True to constrain the point against the closest working plane axis.
        Screenpos can be a list, a tuple or a coin.SbVec2s object."""

        global Part, DraftGeomUtils
        import Part, DraftGeomUtils

        if not hasattr(self,"toolbar"):
            self.makeSnapToolBar()
        mw = getMainWindow()
        bt = mw.findChild(QtGui.QToolBar,"Draft Snap")
        if not bt:
            mw.addToolBar(self.toolbar)
        else:
            if Draft.getParam("showSnapBar"):
                bt.show()

        def cstr(point):
            "constrains if needed"
            if constrain or self.mask:
                fpt = self.constrain(point,lastpoint)
            else:
                self.unconstrain()
                fpt = point
            if self.radiusTracker:
                self.radiusTracker.update(fpt)
            return fpt

        snaps = []
        self.snapInfo = None
        
        # type conversion if needed
        if isinstance(screenpos,list):
            screenpos = tuple(screenpos)
        elif isinstance(screenpos,coin.SbVec2s):
            screenpos = tuple(screenpos.getValue())
        elif  not isinstance(screenpos,tuple):
            print "snap needs valid screen position (list, tuple or sbvec2s)"
            return None

        # setup trackers if needed
        self.setTrackers()

        # getting current snap Radius
        self.radius =  self.getScreenDist(Draft.getParam("snapRange"),screenpos)
        if self.radiusTracker:
            self.radiusTracker.update(self.radius)
            self.radiusTracker.off()

        # activate snap
        oldActive = False
        if Draft.getParam("alwaysSnap"):
            oldActive = active
            active = True
        if not self.active:
            active = False

        self.setCursor('passive')
        if self.tracker:
            self.tracker.off()
        if self.extLine:
            self.extLine.off()
        if self.trackLine:
            self.trackLine.off()

        point = self.getApparentPoint(screenpos[0],screenpos[1])

        # setup a track line if we got a last point
        if lastpoint:
            if not self.trackLine:
                self.trackLine = DraftTrackers.lineTracker()
            self.trackLine.p1(lastpoint)
            
        # check if we snapped to something
        self.snapInfo = Draft.get3DView().getObjectInfo((screenpos[0],screenpos[1]))

        # checking if parallel to one of the edges of the last objects or to a polar direction
        if active:
            eline = None
            point,eline = self.snapToPolar(point,lastpoint)
            point,eline = self.snapToExtensions(point,lastpoint,constrain,eline)
            
        if not self.snapInfo:
            
            # nothing has been snapped, check fro grid snap
            if active:
                point = self.snapToGrid(point)
            fp = cstr(point)
            if self.trackLine and lastpoint:
                self.trackLine.p2(fp)
                self.trackLine.on()
            return fp

        else:

            # we have an object to snap to

            obj = FreeCAD.ActiveDocument.getObject(self.snapInfo['Object'])
            if not obj:
                return cstr(point)

            self.lastSnappedObject = obj
                
            if hasattr(obj.ViewObject,"Selectable"):
                if not obj.ViewObject.Selectable:
                    return cstr(point)
                
            if not active:
                
                # passive snapping
                snaps = [self.snapToVertex(self.snapInfo)]

            else:

                # first stick to the snapped object
                s = self.snapToVertex(self.snapInfo)
                if s:
                    point = s[0]
                
                # active snapping
                comp = self.snapInfo['Component']

                if (Draft.getType(obj) == "Wall") and not oldActive:
                    edges = []
                    for o in [obj]+obj.Additions:
                        if Draft.getType(o) == "Wall":
                            if o.Base:
                                edges.extend(o.Base.Shape.Edges)
                    for edge in edges:
                        snaps.extend(self.snapToEndpoints(edge))
                        snaps.extend(self.snapToMidpoint(edge))
                        snaps.extend(self.snapToPerpendicular(edge,lastpoint))
                        snaps.extend(self.snapToIntersection(edge))
                        snaps.extend(self.snapToElines(edge,eline))
                            
                elif obj.isDerivedFrom("Part::Feature"):
                    if (not self.maxEdges) or (len(obj.Edges) <= self.maxEdges):
                        if "Edge" in comp:
                            # we are snapping to an edge
                            en = int(comp[4:])-1
                            if len(obj.Shape.Edges) > en:
                                edge = obj.Shape.Edges[en]
                                snaps.extend(self.snapToEndpoints(edge))
                                snaps.extend(self.snapToMidpoint(edge))
                                snaps.extend(self.snapToPerpendicular(edge,lastpoint))
                                #snaps.extend(self.snapToOrtho(edge,lastpoint,constrain)) # now part of snapToPolar
                                snaps.extend(self.snapToIntersection(edge))
                                snaps.extend(self.snapToElines(edge,eline))

                                if isinstance (edge.Curve,Part.Circle):
                                    # the edge is an arc, we have extra options
                                    snaps.extend(self.snapToAngles(edge))
                                    snaps.extend(self.snapToCenter(edge))

                        elif "Vertex" in comp:
                            # directly snapped to a vertex
                            snaps.append(self.snapToVertex(self.snapInfo,active=True))
                        elif comp == '':
                            # workaround for the new view provider
                            snaps.append(self.snapToVertex(self.snapInfo,active=True))
                        else:
                            # all other cases (face, etc...) default to passive snap
                            snapArray = [self.snapToVertex(self.snapInfo)]
                            
                elif Draft.getType(obj) == "Dimension":
                    # for dimensions we snap to their 3 points
                    for pt in [obj.Start,obj.End,obj.Dimline]:
                        snaps.append([pt,'endpoint',pt])
                        
                elif Draft.getType(obj) == "Mesh":
                    # for meshes we only snap to vertices
                    snaps.extend(self.snapToEndpoints(obj.Mesh))
                elif Draft.getType(obj) == "Points":
                    # for points we only snap to points
                    snaps.extend(self.snapToEndpoints(obj.Points))

            # updating last objects list
            if not self.lastObj[1]:
                self.lastObj[1] = obj.Name
            elif self.lastObj[1] != obj.Name:
                self.lastObj[0] = self.lastObj[1]
                self.lastObj[1] = obj.Name

            if not snaps:
                return cstr(point)

            # calculating the nearest snap point
            shortest = 1000000000000000000
            origin = Vector(self.snapInfo['x'],self.snapInfo['y'],self.snapInfo['z'])
            winner = [Vector(0,0,0),None,Vector(0,0,0)]
            for snap in snaps:
                if (not snap) or (snap[0] == None):
                    print "debug: Snapper: invalid snap point: ",snaps
                else:
                    delta = snap[0].sub(origin)
                    if delta.Length < shortest:
                        shortest = delta.Length
                        winner = snap

            # see if we are out of the max radius, if any
            if self.radius:
                dv = point.sub(winner[2])
                if (dv.Length > self.radius):
                    if (not oldActive) and self.isEnabled("passive"):
                        winner = self.snapToVertex(self.snapInfo)

            # setting the cursors
            if self.tracker:
                self.tracker.setCoords(winner[2])
                self.tracker.setMarker(self.mk[winner[1]])
                self.tracker.on()
            # setting the trackline
            fp = cstr(winner[2])
            if self.trackLine and lastpoint:
                self.trackLine.p2(fp)
                self.trackLine.on()
            # set the cursor
            self.setCursor(winner[1])

            # return the final point
            return fp

    def getApparentPoint(self,x,y):
        "returns a 3D point, projected on the current working plane"
        view = Draft.get3DView()
        pt = view.getPoint(x,y)
        if self.mask != "z":
            if hasattr(FreeCAD,"DraftWorkingPlane"):
                if view.getCameraType() == "Perspective":
                    camera = view.getCameraNode()
                    p = camera.getField("position").getValue()
                    dv = pt.sub(Vector(p[0],p[1],p[2]))
                else:
                    dv = view.getViewDirection()
                return FreeCAD.DraftWorkingPlane.projectPoint(pt,dv)
        return pt
        
    def snapToExtensions(self,point,last,constrain,eline):
        "returns a point snapped to extension or parallel line to last object, if any"

        if self.isEnabled("extension"):
            tsnap = self.snapToExtOrtho(last,constrain,eline)
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
                    return tsnap[2],eline
            else:
                tsnap = self.snapToExtPerpendicular(last)
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
                        return tsnap[2],eline
                
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
                                    if not DraftGeomUtils.isPtOnEdge(np,e):
                                        if (np.sub(point)).Length < self.radius:
                                            if self.isEnabled('extension'):
                                                if np != e.Vertexes[0].Point:
                                                    if self.tracker:
                                                        self.tracker.setCoords(np)
                                                        self.tracker.setMarker(self.mk['extension'])
                                                        self.tracker.on()
                                                    if self.extLine:
                                                        self.extLine.p1(e.Vertexes[0].Point)
                                                        self.extLine.p2(np)
                                                        self.extLine.on()
                                                    self.setCursor('extension')
                                                    return np,Part.Line(e.Vertexes[0].Point,np).toShape()
                                        else:
                                            if self.isEnabled('parallel'):
                                                if last:
                                                    ve = DraftGeomUtils.vec(e)
                                                    if not DraftVecUtils.isNull(ve):
                                                        de = Part.Line(last,last.add(ve)).toShape()  
                                                        np = self.getPerpendicular(de,point)
                                                        if (np.sub(point)).Length < self.radius:
                                                            if self.tracker:
                                                                self.tracker.setCoords(np)
                                                                self.tracker.setMarker(self.mk['parallel'])
                                                                self.tracker.on()
                                                            self.setCursor('parallel')
                                                            return np,de
        return point,eline

    def snapToPolar(self,point,last):
        "snaps to polar lines from the given point"
        if self.isEnabled('ortho') and (not self.mask): 
            if last:
                vecs = []
                if hasattr(FreeCAD,"DraftWorkingPlane"):
                    ax = [FreeCAD.DraftWorkingPlane.u,
                           FreeCAD.DraftWorkingPlane.v,
                           FreeCAD.DraftWorkingPlane.axis]
                else:
                    ax = [FreeCAD.Vector(1,0,0),
                          FreeCAD.Vector(0,1,0),
                          FreeCAD.Vector(0,0,1)]
                for a in self.polarAngles:
                        if a == 90:
                            vecs.extend([ax[0],DraftVecUtils.neg(ax[0])])
                            vecs.extend([ax[1],DraftVecUtils.neg(ax[1])])
                        else:
                            v = DraftVecUtils.rotate(ax[0],math.radians(a),ax[2])
                            vecs.extend([v,DraftVecUtils.neg(v)])
                            v = DraftVecUtils.rotate(ax[1],math.radians(a),ax[2])
                            vecs.extend([v,DraftVecUtils.neg(v)])
                for v in vecs:
                    de = Part.Line(last,last.add(v)).toShape()  
                    np = self.getPerpendicular(de,point)
                    if ((self.radius == 0) and (point.sub(last).getAngle(v) < 0.087)) \
                    or ((np.sub(point)).Length < self.radius):
                        if self.tracker:
                            self.tracker.setCoords(np)
                            self.tracker.setMarker(self.mk['parallel'])
                            self.tracker.on()
                            self.setCursor('ortho')
                        return np,de
        return point,None

    def snapToGrid(self,point):
        "returns a grid snap point if available"
        if self.grid:
            if self.grid.Visible:
                if self.isEnabled("grid"):
                    np = self.grid.getClosestNode(point)
                    if np:
                        dv = point.sub(np)
                        if (self.radius == 0) or (dv.Length <= self.radius):
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
        if self.isEnabled("endpoint"):
            if hasattr(shape,"Vertexes"):
                for v in shape.Vertexes:
                    snaps.append([v.Point,'endpoint',v.Point])
            elif hasattr(shape,"Point"):
                snaps.append([shape.Point,'endpoint',shape.Point])
            elif hasattr(shape,"Points"):
                if len(shape.Points) and hasattr(shape.Points[0],"Vector"):
                    for v in shape.Points:
                        snaps.append([v.Vector,'endpoint',v.Vector])
                else:
                    for v in shape.Points:
                        snaps.append([v,'endpoint',v])
        return snaps

    def snapToMidpoint(self,shape):
        "returns a list of midpoints snap locations"
        snaps = []
        if self.isEnabled("midpoint"):
            if isinstance(shape,Part.Edge):
                mp = DraftGeomUtils.findMidpoint(shape)
                if mp:
                    snaps.append([mp,'midpoint',mp])
        return snaps

    def snapToPerpendicular(self,shape,last):
        "returns a list of perpendicular snap locations"
        snaps = []
        if self.isEnabled("perpendicular"):
            if last:
                if isinstance(shape,Part.Edge):
                    if isinstance(shape.Curve,Part.Line):
                        np = self.getPerpendicular(shape,last)
                    elif isinstance(shape.Curve,Part.Circle):
                        dv = last.sub(shape.Curve.Center)
                        dv = DraftVecUtils.scaleTo(dv,shape.Curve.Radius)
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
        if self.isEnabled("ortho"):
            if constrain:
                if isinstance(shape,Part.Edge):
                    if last:
                        if isinstance(shape.Curve,Part.Line):
                            if self.constraintAxis:
                                tmpEdge = Part.Line(last,last.add(self.constraintAxis)).toShape()
                                # get the intersection points
                                pt = DraftGeomUtils.findIntersection(tmpEdge,shape,True,True)
                                if pt:
                                    for p in pt:
                                        snaps.append([p,'ortho',p])
        return snaps

    def snapToExtOrtho(self,last,constrain,eline):
        "returns an ortho X extension snap location"
        if self.isEnabled("extension") and self.isEnabled("ortho"):
            if constrain and last and self.constraintAxis and self.extLine:
                tmpEdge1 = Part.Line(last,last.add(self.constraintAxis)).toShape()
                tmpEdge2 = Part.Line(self.extLine.p1(),self.extLine.p2()).toShape()
                # get the intersection points
                pt = DraftGeomUtils.findIntersection(tmpEdge1,tmpEdge2,True,True)
                if pt:
                    return [pt[0],'ortho',pt[0]]
            if eline:
                try:
                    tmpEdge2 = Part.Line(self.extLine.p1(),self.extLine.p2()).toShape()
                    # get the intersection points
                    pt = DraftGeomUtils.findIntersection(eline,tmpEdge2,True,True)
                    if pt:
                        return [pt[0],'ortho',pt[0]]
                except:
                    return None
        return None

    def snapToExtPerpendicular(self,last):
        "returns a perpendicular X extension snap location"
        if self.isEnabled("extension") and self.isEnabled("perpendicular"):
            if last and self.extLine:
                if self.extLine.p1() != self.extLine.p2():
                    tmpEdge = Part.Line(self.extLine.p1(),self.extLine.p2()).toShape()
                    np = self.getPerpendicular(tmpEdge,last)
                    return [np,'perpendicular',np]
        return None

    def snapToElines(self,e1,e2):
        "returns a snap location at the infinite intersection of the given edges"
        snaps = []
        if self.isEnabled("intersection") and self.isEnabled("extension"):
            if e1 and e2:
                # get the intersection points
                pts = DraftGeomUtils.findIntersection(e1,e2,True,True)
                if pts:
                    for p in pts:
                        snaps.append([p,'intersection',p])
        return snaps
            
    
    def snapToAngles(self,shape):
        "returns a list of angle snap locations"
        snaps = []
        if self.isEnabled("angle"):
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
        if self.isEnabled("center"):
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
        if self.isEnabled("intersection"):
            # get the stored objects to calculate intersections
            if self.lastObj[0]:
                obj = FreeCAD.ActiveDocument.getObject(self.lastObj[0])
                if obj:
                    if obj.isDerivedFrom("Part::Feature"):
                        if (not self.maxEdges) or (len(obj.Shape.Edges) <= self.maxEdges):
                            for e in obj.Shape.Edges:
                                # get the intersection points
                                pt = DraftGeomUtils.findIntersection(e,shape)
                                if pt:
                                    for p in pt:
                                        snaps.append([p,'intersection',p])
        return snaps

    def snapToVertex(self,info,active=False):
        p = Vector(info['x'],info['y'],info['z'])
        if active:
            if self.isEnabled("endpoint"):
                return [p,'endpoint',p]
            else:
                return []
        elif self.isEnabled("passive"):
            return [p,'passive',p]
        else:
            return []
        
    def getScreenDist(self,dist,cursor):
        "returns a distance in 3D space from a screen pixels distance"
        view = Draft.get3DView()
        p1 = view.getPoint(cursor)
        p2 = view.getPoint((cursor[0]+dist,cursor[1]))
        return (p2.sub(p1)).Length

    def getPerpendicular(self,edge,pt):
        "returns a point on an edge, perpendicular to the given point"
        dv = pt.sub(edge.Vertexes[0].Point)
        nv = DraftVecUtils.project(dv,DraftGeomUtils.vec(edge))
        np = (edge.Vertexes[0].Point).add(nv)
        return np

    def setCursor(self,mode=None):
        "setCursor(self,mode=None): sets or resets the cursor to the given mode or resets"
        
        if not mode:
            for v in self.views:
                v.unsetCursor()
            self.views = []
            self.cursorMode = None
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

    def restack(self):
        if self.grid:
            self.grid.lowerTracker()

    def off(self):
        "finishes snapping"
        if self.tracker:
            self.tracker.off()
        if self.trackLine:
            self.trackLine.off()
        if self.extLine:
            self.extLine.off()
        if self.radiusTracker:
            self.radiusTracker.off()
        if self.grid:
            if not Draft.getParam("alwaysShowGrid"):
                self.grid.off()
        self.unconstrain()
        self.radius = 0
        self.setCursor()
        if Draft.getParam("hideSnapBar"):
            self.toolbar.hide()
        self.mask = None

    def constrain(self,point,basepoint=None,axis=None):
        '''constrain(point,basepoint=None,axis=None: Returns a
        constrained point. Axis can be "x","y" or "z" or a custom vector. If None,
        the closest working plane axis will be picked.
        Basepoint is the base point used to figure out from where the point
        must be constrained. If no basepoint is given, the current point is
        used as basepoint.'''

        # without the Draft module fully loaded, no axes system!"
        if not hasattr(FreeCAD,"DraftWorkingPlane"):
            return point

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
        if self.mask:
            self.affinity = self.mask
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
        cdelta = DraftVecUtils.project(delta,self.constraintAxis)
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

    def getPoint(self,last=None,callback=None,movecallback=None,extradlg=None):
        
        """
        getPoint([last],[callback],[movecallback],[extradlg]) : gets a 3D point
        from the screen. You can provide an existing point, in that case additional
        snap options and a tracker are available.
        You can also pass a function as callback, which will get called
        with the resulting point as argument, when a point is clicked, and optionally
        another callback which gets called when the mouse is moved.

        If the operation gets cancelled (the user pressed Escape), no point is returned.

        Example:

        def cb(point):
            if point:
                print "got a 3D point: ",point
                
        FreeCADGui.Snapper.getPoint(callback=cb)

        If the callback function accepts more than one argument, it will also receive
        the last snapped object. Finally, a pyqt dialog can be passed as extra taskbox.

        """

        import inspect
        
        self.pt = None
        self.lastSnappedObject = None
        self.ui = FreeCADGui.draftToolBar
        self.view = Draft.get3DView()

        def move(event_cb):
            event = event_cb.getEvent()
            mousepos = event.getPosition()
            ctrl = event.wasCtrlDown()
            shift = event.wasShiftDown()
            self.pt = FreeCADGui.Snapper.snap(mousepos,lastpoint=last,active=ctrl,constrain=shift)
            if hasattr(FreeCAD,"DraftWorkingPlane"):
                self.ui.displayPoint(self.pt,last,plane=FreeCAD.DraftWorkingPlane,mask=FreeCADGui.Snapper.affinity)
            if movecallback:
                movecallback(self.pt)
        
        def getcoords(point,relative=False):
            self.pt = point
            if relative and last and hasattr(FreeCAD,"DraftWorkingPlane"):
                v = FreeCAD.DraftWorkingPlane.getGlobalCoords(point)
                self.pt = last.add(v)
            accept()

        def click(event_cb):
            event = event_cb.getEvent()
            if event.getButton() == 1:
                if event.getState() == coin.SoMouseButtonEvent.DOWN:
                    accept()

        def accept():
            self.view.removeEventCallbackPivy(coin.SoMouseButtonEvent.getClassTypeId(),self.callbackClick)
            self.view.removeEventCallbackPivy(coin.SoLocation2Event.getClassTypeId(),self.callbackMove)
            obj = FreeCADGui.Snapper.lastSnappedObject
            FreeCADGui.Snapper.off()
            self.ui.offUi()
            if callback:
                if len(inspect.getargspec(callback).args) > 2:
                    callback(self.pt,obj)
                else:
                    callback(self.pt)
            self.pt = None

        def cancel():
            self.view.removeEventCallbackPivy(coin.SoMouseButtonEvent.getClassTypeId(),self.callbackClick)
            self.view.removeEventCallbackPivy(coin.SoLocation2Event.getClassTypeId(),self.callbackMove)
            FreeCADGui.Snapper.off()
            self.ui.offUi()
            if callback:
                callback(None)
            
        # adding callback functions
        self.ui.pointUi(cancel=cancel,getcoords=getcoords,extra=extradlg,rel=bool(last))
        self.callbackClick = self.view.addEventCallbackPivy(coin.SoMouseButtonEvent.getClassTypeId(),click)
        self.callbackMove = self.view.addEventCallbackPivy(coin.SoLocation2Event.getClassTypeId(),move)

    def makeSnapToolBar(self):
        "builds the Snap toolbar"
        self.toolbar = QtGui.QToolBar(None)
        self.toolbar.setObjectName("Draft Snap")
        self.toolbar.setWindowTitle("Draft Snap")
        self.toolbarButtons = []
        self.masterbutton = QtGui.QPushButton(None)
        self.masterbutton.setIcon(QtGui.QIcon(":/icons/Snap_Lock.svg"))
        self.masterbutton.setIconSize(QtCore.QSize(16, 16))
        self.masterbutton.setMaximumSize(QtCore.QSize(26,26))
        self.masterbutton.setToolTip("Snap On/Off")
        self.masterbutton.setObjectName("SnapButtonMain")
        self.masterbutton.setCheckable(True)
        self.masterbutton.setChecked(True)
        QtCore.QObject.connect(self.masterbutton,QtCore.SIGNAL("toggled(bool)"),self.toggle)
        self.toolbar.addWidget(self.masterbutton)
        for c,i in self.cursors.iteritems():
            if i:
                b = QtGui.QPushButton(None)
                b.setIcon(QtGui.QIcon(i))
                b.setIconSize(QtCore.QSize(16, 16))
                b.setMaximumSize(QtCore.QSize(26,26))
                b.setToolTip(c)
                b.setObjectName("SnapButton"+c)
                b.setCheckable(True)
                b.setChecked(True)
                self.toolbar.addWidget(b)
                self.toolbarButtons.append(b)
                QtCore.QObject.connect(b,QtCore.SIGNAL("toggled(bool)"),self.saveSnapModes)
        # restoring states 
        t = Draft.getParam("snapModes")
        if t:
            c = 0
            for b in [self.masterbutton]+self.toolbarButtons:
                if len(t) > c:
                    b.setChecked(bool(int(t[c])))
                    c += 1
        if not Draft.getParam("showSnapBar"):
            self.toolbar.hide()

    def saveSnapModes(self):
        "saves the snap modes for next sessions"
        t = ''
        for b in [self.masterbutton]+self.toolbarButtons:
            t += str(int(b.isChecked()))
        Draft.setParam("snapModes",t)

    def toggle(self,checked=None):
        "toggles the snap mode"
        if hasattr(self,"toolbarButtons"):
            if checked == None:
                self.masterbutton.toggle()
            elif checked:
                if hasattr(self,"savedButtonStates"):
                    for i in range(len(self.toolbarButtons)):
                        self.toolbarButtons[i].setEnabled(True)
                        self.toolbarButtons[i].setChecked(self.savedButtonStates[i])
            else:
                self.savedButtonStates = []
                for i in range(len(self.toolbarButtons)):
                    self.savedButtonStates.append(self.toolbarButtons[i].isChecked())
                    self.toolbarButtons[i].setEnabled(False)
        self.saveSnapModes()

    def showradius(self):
        "shows the snap radius indicator"
        self.radius =  self.getScreenDist(Draft.getParam("snapRange"),(400,300))
        if self.radiusTracker:
            self.radiusTracker.update(self.radius)
            self.radiusTracker.on()

    def isEnabled(self,but):
        "returns true if the given button is turned on"
        for b in self.toolbarButtons:
            if str(b.objectName()) == "SnapButton" + but:
                return (b.isEnabled() and b.isChecked())
        return False

    def show(self):
        "shows the toolbar and the grid"
        if not hasattr(self,"toolbar"):
            self.makeSnapToolBar()
        mw = getMainWindow()
        bt = mw.findChild(QtGui.QToolBar,"Draft Snap")
        if not bt:
            mw.addToolBar(self.toolbar)
        self.toolbar.show()
        if FreeCADGui.ActiveDocument:
            self.setTrackers()

    def setGrid(self):
        "sets the grid, if visible"
        if self.grid and (not self.forceGridOff):
            if self.grid.Visible:
                self.grid.set()
            self.setTrackers()

    def setTrackers(self):
        v = Draft.get3DView()
        if v in self.trackers[0]:
            i = self.trackers[0].index(v)
            self.grid = self.trackers[1][i]
            self.tracker = self.trackers[2][i]
            self.extLine = self.trackers[3][i]
            self.radiusTracker = self.trackers[4][i]
        else:
            if Draft.getParam("grid"):
                self.grid = DraftTrackers.gridTracker()
            else:
                self.grid = None
            self.tracker = DraftTrackers.snapTracker()
            self.extLine = DraftTrackers.lineTracker(dotted=True)
            self.radiusTracker = DraftTrackers.radiusTracker()
            self.trackers[0].append(v)
            self.trackers[1].append(self.grid)
            self.trackers[2].append(self.tracker)
            self.trackers[3].append(self.extLine)
            self.trackers[4].append(self.radiusTracker)
        if self.grid and (not self.forceGridOff):
            self.grid.set()
        
if not hasattr(FreeCADGui,"Snapper"):
    FreeCADGui.Snapper = Snapper()
if not hasattr(FreeCAD,"DraftWorkingPlane"):
    import WorkingPlane, Draft_rc
    FreeCAD.DraftWorkingPlane = WorkingPlane.plane()
    print FreeCAD.DraftWorkingPlane
    FreeCADGui.addIconPath(":/icons")
