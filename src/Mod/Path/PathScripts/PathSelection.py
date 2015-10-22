# -*- coding: utf-8 -*-

#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2015 Dan Falck <ddfalck@gmail.com>                      *
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
'''Path selection function select a face or faces, two edges, etc to get a dictionary with what was selected in order '''

import FreeCAD,FreeCADGui
import Part
from FreeCAD import Vector

def equals(p1,p2):
    '''returns True if vertexes have same coordinates within precision amount of digits '''
    precision = 12 #hardcoded
    p=precision
    u = Vector(p1.X,p1.Y,p1.Z)
    v = Vector(p2.X,p2.Y,p2.Z)
    vector = (u.sub(v))
    isNull = (round(vector.x,p)==0 and round(vector.y,p)==0 and round(vector.z,p)==0)
    return isNull 



def Sort2Edges(edgelist):
    '''Sort2Edges(edgelist) simple function to reorder the start and end pts of two edges 
    based on their selection order. Returns the list, the start point, 
    and their common point, => edgelist, vertex, vertex'''
    if len(edgelist)>=2:
        vlist = []
        e0 = edgelist[0]
        e1=edgelist[1]
        a0 = e0.Vertexes[0]
        a1 = e0.Vertexes[1]
        b0 = e1.Vertexes[0]
        b1 = e1.Vertexes[1]      
        # comparison routine to order two edges:
        if equals(a1,b0):
            vlist.append((a0.Point.x,a0.Point.y))
            vlist.append((a1.Point.x,a1.Point.y))
            vlist.append((b1.Point.x,b1.Point.y))

        if equals(a0,b0):
            vlist.append((a1.Point.x,a1.Point.y))
            vlist.append((a0.Point.x,a0.Point.y))
            vlist.append((b1.Point.x,b1.Point.y))

        if equals(a0,b1):
            vlist.append((a1.Point.x,a1.Point.y))
            vlist.append((a0.Point.x,a0.Point.y))
            vlist.append((b0.Point.x,b0.Point.y))

        if equals(a1,b1):
            vlist.append((a0.Point.x,a0.Point.y))
            vlist.append((a1.Point.x,a1.Point.y))
            vlist.append((b0.Point.x,b0.Point.y))

        edgestart = Vector(vlist[0][0],vlist[0][1],e0.Vertexes[1].Z)
        edgecommon = Vector(vlist[1][0],vlist[1][1],e0.Vertexes[1].Z)

    return vlist,edgestart,edgecommon

def segments(poly):
    ''' A sequence of (x,y) numeric coordinates pairs '''
    return zip(poly, poly[1:] + [poly[0]])

def check_clockwise(poly):
    '''
     check_clockwise(poly) a function for returning a boolean if the selected wire is clockwise or counter clockwise
     based on point order. poly = [(x1,y1),(x2,y2),(x3,y3)]
    '''
    clockwise = False
    if (sum(x0*y1 - x1*y0 for ((x0, y0), (x1, y1)) in segments(poly))) < 0:
        clockwise = not clockwise
    return clockwise


def multiSelect():
    '''
    multiSelect() A function for selecting elements of an object for CNC path operations.
    Select just a face, an edge,or two edges to indicate direction, a vertex on the object, a point not on the object,
    or some combination. Returns a dictionary.
    '''
    sel = FreeCADGui.Selection.getSelectionEx()
    numobjs = len([selobj.Object for selobj in sel])
    if numobjs == 0:
        FreeCAD.Console.PrintError('Please select some objects and try again.\n')
        return
    goodselect = False
    for s in sel:
        for i in s.SubObjects:
            if i.ShapeType == 'Face':
                goodselect = True
            if i.ShapeType == 'Edge':
                goodselect = True
            if i.ShapeType == 'Vertex':
                goodselect = True
    if not goodselect:
        FreeCAD.Console.PrintError('Please select a face and/or edges along with points (optional) and try again.\n')
        return

    selItems = {}
    selItems['objname']=None #the parent object name - a 3D solid
    selItems['pointlist']=None #start and end points
    selItems['pointnames']=None #names of points for document object
    selItems['facenames']=None # the selected face name
    selItems['facelist']=None #list of faces selected
    selItems['edgelist']=None #some edges that could be selected along with points and faces
    selItems['edgenames']=None
    selItems['pathwire']=None #the whole wire around edges of the face
    selItems['clockwise']=None
    selItems['circles']=None
    facenames = []
    edgelist =[]
    edgenames=[]
    ptlist=[]
    ptnames=[]
    circlelist=[]
    face = False
    edges = False
    points = False
    wireobj = False
    circles = False
    facelist= []
    for s in sel:
        if s.Object.Shape.ShapeType in ['Solid','Compound','Wire','Vertex']:
            if not (s.Object.Shape.ShapeType =='Vertex'):
                objname = s.ObjectName 
                selItems['objname']   =objname
            if s.Object.Shape.ShapeType == 'Wire':
                wireobj = True
            if s.Object.Shape.ShapeType == 'Vertex':
                ptnames.append(s.ObjectName)
#                ptlist.append(s.Object)
                points = True
        for sub in s.SubObjects:
            if sub.ShapeType =='Face':
                facelist.append(sub)
                face = True
            if sub.ShapeType =='Edge':
                edge = sub
                edgelist.append(edge)
                edges = True
                if isinstance(sub.Curve,Part.Circle):
                    circlelist.append(edge)
                    circles = True
            if sub.ShapeType =='Vertex':
                ptlist.append(sub)
                points = True

        for sub in s.SubElementNames:
            if 'Face' in sub:
                facename = sub
                facenames.append(facename) 
            if 'Edge' in sub:
                edgenames.append(sub)
    # now indicate which wire is going to be processed, based on which edges are selected
    if facelist:
        selItems['facelist']=facelist

    if edges:
        if face:
            selItems['edgelist'] =edgelist
            for fw in facelist[0].Wires:
                for e in  fw.Edges:
                    if e.isSame(edge):
                        pathwire = fw
                        selItems['pathwire']  =pathwire
        elif wireobj:
            selItems['pathwire'] =s.Object.Shape
            selItems['edgelist'] =edgelist
        else:
            for w in s.Object.Shape.Wires:
                for e in  w.Edges:
                    if e.BoundBox.ZMax == e.BoundBox.ZMin: #if they are on same plane in Z as sel edge
                        if e.isSame(edge):
                            pathwire = w
                            selItems['pathwire']  =pathwire
            selItems['edgelist'] =edgelist

    if not edges:
        if face:
            selItems['pathwire']  =facelist[0].OuterWire

    if edges and (len(edgelist)>=2):
        vlist,edgestart,edgecommon=Sort2Edges(edgelist)
        edgepts ={}
        edgepts['vlist'] = vlist
        edgepts['edgestart']=edgestart # start point of edges selected
        edgepts['edgecommon']=edgecommon # point where two edges join- will be last point in in first gcode line
        selItems['edgepts']=edgepts

        if check_clockwise(vlist):
            selItems['clockwise']=True
        elif check_clockwise(vlist) == False:
            selItems['clockwise']=False

    if points:
        selItems['pointlist']  = ptlist
        selItems['pointnames'] = ptnames
    if edges:
        selItems['edgenames']=edgenames
    if face:
        selItems['facenames'] = facenames
    if circles:
        selItems['circles'] = circlelist

    return selItems

