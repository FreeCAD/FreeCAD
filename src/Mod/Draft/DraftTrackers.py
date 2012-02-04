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

__title__="FreeCAD Draft Trackers"
__author__ = "Yorik van Havre"
__url__ = "http://free-cad.sourceforge.net"

import FreeCAD,FreeCADGui,math,Draft
from FreeCAD import Vector
from draftlibs import fcvec
from pivy import coin
from DraftGui import todo

class Tracker:
    "A generic Draft Tracker, to be used by other specific trackers"
    def __init__(self,dotted=False,scolor=None,swidth=None,children=[],ontop=False):
        global Part, fcgeo
        import Part
        from draftlibs import fcgeo
        self.ontop = ontop
        color = coin.SoBaseColor()
        color.rgb = scolor or FreeCADGui.draftToolBar.getDefaultColor("ui")
        drawstyle = coin.SoDrawStyle()
        if swidth:
            drawstyle.lineWidth = swidth
        if dotted:
            drawstyle.style = coin.SoDrawStyle.LINES
            drawstyle.lineWeight = 3
            drawstyle.linePattern = 0x0f0f #0xaa
        node = coin.SoSeparator()
        for c in [drawstyle, color] + children:
            node.addChild(c)
        self.switch = coin.SoSwitch() # this is the on/off switch
        self.switch.addChild(node)
        self.switch.whichChild = -1
        self.Visible = False
        todo.delay(self._insertSwitch, self.switch)

    def finalize(self):
        todo.delay(self._removeSwitch, self.switch)
        self.switch = None

    def _insertSwitch(self, switch):
        '''insert self.switch into the scene graph.  Must not be called
        from an event handler (or other scene graph traversal).'''
        sg=FreeCADGui.ActiveDocument.ActiveView.getSceneGraph()
        if self.ontop:
            sg.insertChild(switch,0)
        else:
            sg.addChild(switch)

    def _removeSwitch(self, switch):
        '''remove self.switch from the scene graph.  As with _insertSwitch,
        must not be called during scene graph traversal).'''
        sg=FreeCADGui.ActiveDocument.ActiveView.getSceneGraph()
        sg.removeChild(switch)

    def on(self):
        self.switch.whichChild = 0
        self.Visible = True

    def off(self):
        self.switch.whichChild = -1
        self.Visible = False
				
class snapTracker(Tracker):
    "A Snap Mark tracker, used by tools that support snapping"
    def __init__(self):
        color = coin.SoBaseColor()
        color.rgb = FreeCADGui.draftToolBar.getDefaultColor("snap")
        self.marker = coin.SoMarkerSet() # this is the marker symbol
        self.marker.markerIndex = coin.SoMarkerSet.CIRCLE_FILLED_9_9
        self.coords = coin.SoCoordinate3() # this is the coordinate
        self.coords.point.setValue((0,0,0))
        node = coin.SoAnnotation()
        node.addChild(self.coords)
        node.addChild(color)
        node.addChild(self.marker)
        Tracker.__init__(self,children=[node])

    def setMarker(self,style):
        if (style == "point"):
            self.marker.markerIndex = coin.SoMarkerSet.CIRCLE_FILLED_9_9
        elif (style == "dot"):
            self.marker.markerIndex = coin.SoMarkerSet.CIRCLE_FILLED_9_9
        elif (style == "square"):
            self.marker.markerIndex = coin.SoMarkerSet.DIAMOND_FILLED_9_9
        elif (style == "circle"):
            self.marker.markerIndex = coin.SoMarkerSet.CIRCLE_LINE_9_9

    def setCoords(self,point):
        self.coords.point.setValue((point.x,point.y,point.z))

class lineTracker(Tracker):
    "A Line tracker, used by the tools that need to draw temporary lines"
    def __init__(self,dotted=False,scolor=None,swidth=None):
        line = coin.SoLineSet()
        line.numVertices.setValue(2)
        self.coords = coin.SoCoordinate3() # this is the coordinate
        self.coords.point.setValues(0,2,[[0,0,0],[1,0,0]])
        Tracker.__init__(self,dotted,scolor,swidth,[self.coords,line])

    def p1(self,point=None):
        "sets or gets the first point of the line"
        if point:
            self.coords.point.set1Value(0,point.x,point.y,point.z)
        else:
            return Vector(self.coords.point.getValues()[0].getValue())

    def p2(self,point=None):
        "sets or gets the second point of the line"
        if point:
            self.coords.point.set1Value(1,point.x,point.y,point.z)
        else:
            return Vector(self.coords.point.getValues()[-1].getValue())
                        
    def getLength(self):
        "returns the length of the line"
        p1 = Vector(self.coords.point.getValues()[0].getValue())
        p2 = Vector(self.coords.point.getValues()[-1].getValue())
        return (p2.sub(p1)).Length

class rectangleTracker(Tracker):
    "A Rectangle tracker, used by the rectangle tool"
    def __init__(self,dotted=False,scolor=None,swidth=None):
        self.origin = Vector(0,0,0)
        line = coin.SoLineSet()
        line.numVertices.setValue(5)
        self.coords = coin.SoCoordinate3() # this is the coordinate
        self.coords.point.setValues(0,50,[[0,0,0],[2,0,0],[2,2,0],[0,2,0],[0,0,0]])
        Tracker.__init__(self,dotted,scolor,swidth,[self.coords,line])
        self.u = FreeCAD.DraftWorkingPlane.u
        self.v = FreeCAD.DraftWorkingPlane.v

    def setorigin(self,point):
        "sets the base point of the rectangle"
        self.coords.point.set1Value(0,point.x,point.y,point.z)
        self.coords.point.set1Value(4,point.x,point.y,point.z)
        self.origin = point

    def update(self,point):
        "sets the opposite (diagonal) point of the rectangle"
        diagonal = point.sub(self.origin)
        inpoint1 = self.origin.add(fcvec.project(diagonal,self.v))
        inpoint2 = self.origin.add(fcvec.project(diagonal,self.u))
        self.coords.point.set1Value(1,inpoint1.x,inpoint1.y,inpoint1.z)
        self.coords.point.set1Value(2,point.x,point.y,point.z)
        self.coords.point.set1Value(3,inpoint2.x,inpoint2.y,inpoint2.z)

    def setPlane(self,u,v=None):
        '''sets given (u,v) vectors as working plane. You can give only u
        and v will be deduced automatically given current workplane'''
        self.u = u
        if v:
            self.v = v
        else:
            norm = FreeCAD.DraftWorkingPlane.u.cross(FreeCAD.DraftWorkingPlane.v)
            self.v = self.u.cross(norm)

    def p1(self,point=None):
        "sets or gets the base point of the rectangle"
        if point:
            self.setorigin(point)
        else:
            return Vector(self.coords.point.getValues()[0].getValue())

    def p2(self):
        "gets the second point (on u axis) of the rectangle"
        return Vector(self.coords.point.getValues()[3].getValue())

    def p3(self,point=None):
        "sets or gets the opposite (diagonal) point of the rectangle"
        if point:
            self.update(point)
        else:
            return Vector(self.coords.point.getValues()[2].getValue())

    def p4(self):
        "gets the fourth point (on v axis) of the rectangle"
        return Vector(self.coords.point.getValues()[1].getValue())
                
    def getSize(self):
        "returns (length,width) of the rectangle"
        p1 = Vector(self.coords.point.getValues()[0].getValue())
        p2 = Vector(self.coords.point.getValues()[2].getValue())
        diag = p2.sub(p1)
        return ((fcvec.project(diag,self.u)).Length,(fcvec.project(diag,self.v)).Length)

    def getNormal(self):
        "returns the normal of the rectangle"
        return (self.u.cross(self.v)).normalize()
                
class dimTracker(Tracker):
    "A Dimension tracker, used by the dimension tool"
    def __init__(self,dotted=False,scolor=None,swidth=None):
        line = coin.SoLineSet()
        line.numVertices.setValue(4)
        self.coords = coin.SoCoordinate3() # this is the coordinate
        self.coords.point.setValues(0,4,[[0,0,0],[0,0,0],[0,0,0],[0,0,0]])
        Tracker.__init__(self,dotted,scolor,swidth,[self.coords,line])
        self.p1 = self.p2 = self.p3 = None

    def update(self,pts):
        if len(pts) == 1:
            self.p3 = pts[0]
        else:
            self.p1 = pts[0]
            self.p2 = pts[1]
            if len(pts) > 2:
                self.p3 = pts[2]
        self.calc()
        
    def calc(self):
        import Part
        if (self.p1 != None) and (self.p2 != None):
            points = [fcvec.tup(self.p1,True),fcvec.tup(self.p2,True),\
                          fcvec.tup(self.p1,True),fcvec.tup(self.p2,True)]
            if self.p3 != None:
                p1 = self.p1
                p4 = self.p2
                if fcvec.equals(p1,p4):
                    proj = None
                else:
                    base = Part.Line(p1,p4).toShape()
                    proj = fcgeo.findDistance(self.p3,base)
                if not proj:
                    p2 = p1
                    p3 = p4
                else:
                    p2 = p1.add(fcvec.neg(proj))
                    p3 = p4.add(fcvec.neg(proj))
                points = [fcvec.tup(p1),fcvec.tup(p2),fcvec.tup(p3),fcvec.tup(p4)]
            self.coords.point.setValues(0,4,points)

class bsplineTracker(Tracker):
    "A bspline tracker"
    def __init__(self,dotted=False,scolor=None,swidth=None,points = []):
        self.bspline = None
        self.points = points
        self.trans = coin.SoTransform()
        self.sep = coin.SoSeparator()
        self.recompute()
        Tracker.__init__(self,dotted,scolor,swidth,[self.trans,self.sep])
        
    def update(self, points):
        self.points = points
        self.recompute()
            
    def recompute(self):
        if (len(self.points) >= 2):
            if self.bspline: self.sep.removeChild(self.bspline)
            self.bspline = None
            c =  Part.BSplineCurve()
            # DNC: allows to close the curve by placing ends close to each other
            if ( len(self.points) >= 3 ) and ( (self.points[0] - self.points[-1]).Length < Draft.tolerance() ):
                # YVH: Added a try to bypass some hazardous situations
                try:
                    c.interpolate(self.points[:-1], True)
                except:
                    pass
            elif self.points:
                try:
                    c.interpolate(self.points, False)
                except:
                    pass
            c = c.toShape()
            buf=c.writeInventor(2,0.01)
            #fp=open("spline.iv","w")
            #fp.write(buf)
            #fp.close()
            ivin = coin.SoInput()
            ivin.setBuffer(buf)
            ivob = coin.SoDB.readAll(ivin)
            # In case reading from buffer failed
            if ivob and ivob.getNumChildren() > 1:
                self.bspline = ivob.getChild(1).getChild(0)
                self.bspline.removeChild(self.bspline.getChild(0))
                self.bspline.removeChild(self.bspline.getChild(0))
                self.sep.addChild(self.bspline)
            else:
                FreeCAD.Console.PrintWarning("bsplineTracker.recompute() failed to read-in Inventor string\n")

class arcTracker(Tracker):
    "An arc tracker"
    def __init__(self,dotted=False,scolor=None,swidth=None,start=0,end=math.pi*2):
        self.circle = None
        self.startangle = math.degrees(start)
        self.endangle = math.degrees(end)
        self.trans = coin.SoTransform()
        self.trans.translation.setValue([0,0,0])
        self.sep = coin.SoSeparator()
        self.recompute()
        Tracker.__init__(self,dotted,scolor,swidth,[self.trans, self.sep])

    def setCenter(self,cen):
        "sets the center point"
        self.trans.translation.setValue([cen.x,cen.y,cen.z])

    def setRadius(self,rad):
        "sets the radius"
        self.trans.scaleFactor.setValue([rad,rad,rad])

    def getRadius(self):
        "returns the current radius"
        return self.trans.scaleFactor.getValue()[0]

    def setStartAngle(self,ang):
        "sets the start angle"
        self.startangle = math.degrees(ang)
        self.recompute()

    def setEndAngle(self,ang):
        "sets the end angle"
        self.endangle = math.degrees(ang)
        self.recompute()

    def getAngle(self,pt):
        "returns the angle of a given vector"
        c = self.trans.translation.getValue()
        center = Vector(c[0],c[1],c[2])
        base = FreeCAD.DraftWorkingPlane.u
        rad = pt.sub(center)
        return(fcvec.angle(rad,base,FreeCAD.DraftWorkingPlane.axis))

    def getAngles(self):
        "returns the start and end angles"
        return(self.startangle,self.endangle)
                
    def setStartPoint(self,pt):
        "sets the start angle from a point"
        self.setStartAngle(-self.getAngle(pt))

    def setEndPoint(self,pt):
        "sets the end angle from a point"
        self.setEndAngle(self.getAngle(pt))
                
    def setApertureAngle(self,ang):
        "sets the end angle by giving the aperture angle"
        ap = math.degrees(ang)
        self.endangle = self.startangle + ap
        self.recompute()

    def recompute(self):
        if self.circle: self.sep.removeChild(self.circle)
        self.circle = None
        if self.endangle < self.startangle:
            c = Part.makeCircle(1,Vector(0,0,0),FreeCAD.DraftWorkingPlane.axis,self.endangle,self.startangle)
        else:
            c = Part.makeCircle(1,Vector(0,0,0),FreeCAD.DraftWorkingPlane.axis,self.startangle,self.endangle)
        buf=c.writeInventor(2,0.01)
        ivin = coin.SoInput()
        ivin.setBuffer(buf)
        ivob = coin.SoDB.readAll(ivin)
        # In case reading from buffer failed
        if ivob and ivob.getNumChildren() > 1:
            self.circle = ivob.getChild(1).getChild(0)
            self.circle.removeChild(self.circle.getChild(0))
            self.circle.removeChild(self.circle.getChild(0))
            self.sep.addChild(self.circle)
        else:
            FreeCAD.Console.PrintWarning("arcTracker.recompute() failed to read-in Inventor string\n")

class ghostTracker(Tracker):
    '''A Ghost tracker, that allows to copy whole object representations.
    You can pass it an object or a list of objects, or a shape.'''
    def __init__(self,sel):
        self.trans = coin.SoTransform()
        self.trans.translation.setValue([0,0,0])
        self.children = [self.trans]
        self.ivsep = coin.SoSeparator()
        try:
            if isinstance(sel,Part.Shape):
                ivin = coin.SoInput()
                ivin.setBuffer(sel.writeInventor())
                ivob = coin.SoDB.readAll(ivin)
                self.ivsep.addChild(ivob.getChildren()[1])
            else:
                if not isinstance(sel,list):
                    sel = [sel]
                for obj in sel:
                    self.ivsep.addChild(obj.ViewObject.RootNode.copy())
        except:
            print "draft: Couldn't create ghost"
        self.children.append(self.ivsep)
        Tracker.__init__(self,children=self.children)

    def update(self,obj):
        obj.ViewObject.show()
        self.finalize()
        self.ivsep = coin.SoSeparator()
        self.ivsep.addChild(obj.ViewObject.RootNode.copy())
        Tracker.__init__(self,children=[self.ivsep])
        self.on()
        obj.ViewObject.hide()

class editTracker(Tracker):
    "A node edit tracker"
    def __init__(self,pos=Vector(0,0,0),name="None",idx=0,objcol=None):
        color = coin.SoBaseColor()
        if objcol:
            color.rgb = objcol[:3]
        else:
            color.rgb = FreeCADGui.draftToolBar.getDefaultColor("snap")
        self.marker = coin.SoMarkerSet() # this is the marker symbol
        self.marker.markerIndex = coin.SoMarkerSet.SQUARE_FILLED_9_9
        self.coords = coin.SoCoordinate3() # this is the coordinate
        self.coords.point.setValue((pos.x,pos.y,pos.z))
        selnode = coin.SoType.fromName("SoFCSelection").createInstance()
        selnode.documentName.setValue(FreeCAD.ActiveDocument.Name)
        selnode.objectName.setValue(name)
        selnode.subElementName.setValue("EditNode"+str(idx))
        node = coin.SoAnnotation()
        selnode.addChild(self.coords)
        selnode.addChild(color)
        selnode.addChild(self.marker)
        node.addChild(selnode)
        Tracker.__init__(self,children=[node],ontop=True)
        self.on()

    def set(self,pos):
        self.coords.point.setValue((pos.x,pos.y,pos.z))

    def get(self):
        p = self.coords.point.getValues()[0]
        return Vector(p[0],p[1],p[2])

    def move(self,delta):
        self.set(self.get().add(delta))

class PlaneTracker(Tracker):
    "A working plane tracker"
    def __init__(self):
        # getting screen distance
        p1 = FreeCADGui.ActiveDocument.ActiveView.getPoint((100,100))
        p2 = FreeCADGui.ActiveDocument.ActiveView.getPoint((110,100))
        bl = (p2.sub(p1)).Length * (Draft.getParam("snapRange")/2)
        self.trans = coin.SoTransform()
        self.trans.translation.setValue([0,0,0])
        m1 = coin.SoMaterial()
        m1.transparency.setValue(0.8)
        m1.diffuseColor.setValue([0.4,0.4,0.6])
        c1 = coin.SoCoordinate3()
        c1.point.setValues([[-bl,-bl,0],[bl,-bl,0],[bl,bl,0],[-bl,bl,0]])
        f = coin.SoIndexedFaceSet()
        f.coordIndex.setValues([0,1,2,3])
        m2 = coin.SoMaterial()
        m2.transparency.setValue(0.7)
        m2.diffuseColor.setValue([0.2,0.2,0.3])
        c2 = coin.SoCoordinate3()
        c2.point.setValues([[0,bl,0],[0,0,0],[bl,0,0],[-.05*bl,.95*bl,0],[0,bl,0],
                            [.05*bl,.95*bl,0],[.95*bl,.05*bl,0],[bl,0,0],[.95*bl,-.05*bl,0]])
        l = coin.SoLineSet()
        l.numVertices.setValues([3,3,3])
        s = coin.SoSeparator()
        s.addChild(self.trans)
        s.addChild(m1)
        s.addChild(c1)
        s.addChild(f)
        s.addChild(m2)
        s.addChild(c2)
        s.addChild(l)
        Tracker.__init__(self,children=[s])

    def set(self,pos=None):
        if pos:                        
            Q = FreeCAD.DraftWorkingPlane.getRotation().Rotation.Q
        else:
            plm = FreeCAD.DraftWorkingPlane.getPlacement()
            Q = plm.Rotation.Q
            pos = plm.Base
        self.trans.translation.setValue([pos.x,pos.y,pos.z])
        self.trans.rotation.setValue([Q[0],Q[1],Q[2],Q[3]])
        self.on()
                        
class wireTracker(Tracker):                
    "A wire tracker"
    def __init__(self,wire):
        self.line = coin.SoLineSet()
        self.closed = fcgeo.isReallyClosed(wire)
        if self.closed:
            self.line.numVertices.setValue(len(wire.Vertexes)+1)
        else:
            self.line.numVertices.setValue(len(wire.Vertexes))
        self.coords = coin.SoCoordinate3()
        self.update(wire)
        Tracker.__init__(self,children=[self.coords,self.line])

    def update(self,wire,forceclosed=False):
        if wire:
            if self.closed or forceclosed:
                self.line.numVertices.setValue(len(wire.Vertexes)+1)
            else:
                self.line.numVertices.setValue(len(wire.Vertexes))
            for i in range(len(wire.Vertexes)):
                p=wire.Vertexes[i].Point
                self.coords.point.set1Value(i,[p.x,p.y,p.z])
            if self.closed or forceclosed:
                t = len(wire.Vertexes)
                p = wire.Vertexes[0].Point
                self.coords.point.set1Value(t,[p.x,p.y,p.z])

class gridTracker(Tracker):
    "A grid tracker"
    def __init__(self):
        # self.space = 1
        self.space = Draft.getParam("gridSpacing")
        # self.mainlines = 10
        self.mainlines = Draft.getParam("gridEvery")
        self.numlines = 100
        col = [0.2,0.2,0.3]
        
        self.trans = coin.SoTransform()
        self.trans.translation.setValue([0,0,0])
                
        bound = (self.numlines/2)*self.space
        pts = []
        mpts = []
        for i in range(self.numlines+1):
            curr = -bound + i*self.space
            z = 0
            if i/float(self.mainlines) == i/self.mainlines:
                mpts.extend([[-bound,curr,z],[bound,curr,z]])
                mpts.extend([[curr,-bound,z],[curr,bound,z]])
            else:
                pts.extend([[-bound,curr,z],[bound,curr,z]])
                pts.extend([[curr,-bound,z],[curr,bound,z]])
        idx = []
        midx = []
        for p in range(0,len(pts),2):
            idx.append(2)
        for mp in range(0,len(mpts),2):
            midx.append(2)

        mat1 = coin.SoMaterial()
        mat1.transparency.setValue(0.7)
        mat1.diffuseColor.setValue(col)
        self.coords1 = coin.SoCoordinate3()
        self.coords1.point.setValues(pts)
        lines1 = coin.SoLineSet()
        lines1.numVertices.setValues(idx)
        mat2 = coin.SoMaterial()
        mat2.transparency.setValue(0.3)
        mat2.diffuseColor.setValue(col)
        self.coords2 = coin.SoCoordinate3()
        self.coords2.point.setValues(mpts)
        lines2 = coin.SoLineSet()
        lines2.numVertices.setValues(midx)
        s = coin.SoSeparator()
        s.addChild(self.trans)
        s.addChild(mat1)
        s.addChild(self.coords1)
        s.addChild(lines1)
        s.addChild(mat2)
        s.addChild(self.coords2)
        s.addChild(lines2)
        Tracker.__init__(self,children=[s])
        self.update()

    def update(self):
        bound = (self.numlines/2)*self.space
        pts = []
        mpts = []
        for i in range(self.numlines+1):
            curr = -bound + i*self.space
            if i/float(self.mainlines) == i/self.mainlines:
                mpts.extend([[-bound,curr,0],[bound,curr,0]])
                mpts.extend([[curr,-bound,0],[curr,bound,0]])
            else:
                pts.extend([[-bound,curr,0],[bound,curr,0]])
                pts.extend([[curr,-bound,0],[curr,bound,0]])
        self.coords1.point.setValues(pts)
        self.coords2.point.setValues(mpts)

    def setSpacing(self,space):
        self.space = space
        self.update()

    def setMainlines(self,ml):
        self.mainlines = ml
        self.update()

    def set(self):
        Q = FreeCAD.DraftWorkingPlane.getRotation().Rotation.Q
        self.trans.rotation.setValue([Q[0],Q[1],Q[2],Q[3]])
        self.on()

    def getClosestNode(self,point):
        "returns the closest node from the given point"
        # get the 2D coords.
        point = FreeCAD.DraftWorkingPlane.projectPoint(point)
        u = fcvec.project(point,FreeCAD.DraftWorkingPlane.u)
        lu = u.Length
        if u.getAngle(FreeCAD.DraftWorkingPlane.u) > 1.5:
            lu  = -lu
        v = fcvec.project(point,FreeCAD.DraftWorkingPlane.v)
        lv = v.Length
        if v.getAngle(FreeCAD.DraftWorkingPlane.v) > 1.5:
            lv = -lv
        # print "u = ",u," v = ",v
        # find nearest grid node
        pu = (round(lu/self.space,0))*self.space
        pv = (round(lv/self.space,0))*self.space
        rot = FreeCAD.Rotation()
        rot.Q = self.trans.rotation.getValue().getValue()
        return rot.multVec(Vector(pu,pv,0))

class boxTracker(Tracker):                
    "A box tracker, can be based on a line object"
    def __init__(self,line=None,width=0.1,height=1):
        self.trans = coin.SoTransform()
        m = coin.SoMaterial()
        m.transparency.setValue(0.8)
        m.diffuseColor.setValue([0.4,0.4,0.6])
        self.cube = coin.SoCube()
        self.cube.height.setValue(width)
        self.cube.depth.setValue(height)
        self.baseline = None
        if line:
            self.baseline = line
            self.update()
        Tracker.__init__(self,children=[self.trans,m,self.cube])

    def update(self,line=None,normal=None):
        import WorkingPlane
        from draftlibs import fcgeo
        if not normal:
            normal = FreeCAD.DraftWorkingPlane.axis
        if line:
            if isinstance(line,list):
                bp = line[0]
                lvec = line[1].sub(line[0])
            else:
                lvec = fcgeo.vec(line.Shape.Edges[0])
                bp = line.Shape.Edges[0].Vertexes[0].Point
        elif self.baseline:
            lvec = fcgeo.vec(self.baseline.Shape.Edges[0])
            bp = self.baseline.Shape.Edges[0].Vertexes[0].Point
        else:
            return
        right = lvec.cross(normal)
        self.cube.width.setValue(lvec.Length)
        p = WorkingPlane.getPlacementFromPoints([bp,bp.add(lvec),bp.add(right)])
        self.trans.rotation.setValue(p.Rotation.Q)
        bp = bp.add(fcvec.scale(lvec,0.5))
        bp = bp.add(fcvec.scaleTo(normal,self.cube.depth.getValue()/2))
        self.pos(bp)

    def pos(self,p):
        self.trans.translation.setValue(fcvec.tup(p))

    def width(self,w=None):
        if w:
            self.cube.height.setValue(w)
        else:
            return self.cube.height.getValue()

    def length(self,l=None):
        if l:
            self.cube.width.setValue(l)
        else:
            return self.cube.width.getValue()
        
    def height(self,h=None):
        if h:
            self.cube.depth.setValue(h)
            self.update()
        else:
            return self.cube.depth.getValue()
