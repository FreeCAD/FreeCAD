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

import FreeCAD, FreeCADGui, math, Draft, DraftTrackers, Part
from draftlibs import fcvec,fcgeo
from FreeCAD import Vector
from pivy import coin

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
