# -*- coding: utf8 -*-

#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2009 Yorik van Havre <yorik@uncreated.net>              *
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

__title__="FreeCAD Draft Workbench - OCA importer/exporter"
__author__ = "Yorik van Havre <yorik@uncreated.net>"
__url__ = ["http://www.freecadweb.org"]

'''
This script imports OCA/gcad files into FreeCAD.
'''

import FreeCAD, os, Part, math, DraftVecUtils, DraftGeomUtils
from FreeCAD import Vector

try: import FreeCADGui
except ValueError: gui = False
else: gui = True

if open.__module__ == '__builtin__':
    pythonopen = open
    
params = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft")

def getpoint(data):
    "turns an OCA point definition into a FreeCAD Vector"
    print "found point ",data
    if (len(data) == 3):
        return Vector(float(data[0]),float(data[1]),float(data[2]))
    elif (data[0] == "P") and (len(data) == 4):
        return Vector(float(data[1]),float(data[2]),float(data[3]))
    elif (data[0][0] == "P") and (len(data[0]) > 1):
        if (len(data) == 1):
            return objects[data[0]]
        else:
            if (data[1][0] == "R"):
                return objects[data[0]].add(objects[data[1]])
            elif (data[1][0] == "C"):
                return DraftGeomUtils.findProjection(objects[data[0]],objects[data[1]])
    elif (data[0][0] == "C"):
        if objects[data[0]]:
            p1 = objects[data[0]].Curve.Position
            if (len(data) == 1):
                return p1
            else:
                if (data[1][0] == "L"):
                    l = objects[data[1]]
                    return p1.add(DraftGeomUtils.vec(l))
              
def getarea(data):
    "turns an OCA area definition into a FreeCAD Part Wire"
    print "found area ",data
    if (data[0] == "S"):
        if (data[1] == "POL"):
            pts = data[2:]
            verts = []
            for p in pts:
                if (p[0] == "P"):
                    verts.append(getpoint([p]))
            w = Part.makePolygon(verts)
            return w

def getarc(data):
    "turns an OCA arc definition into a FreeCAD Part Edge"
    print "found arc ", data
    c = None
    if (data[0] == "ARC"):
        # 3-points arc
        pts = data[1:]
        verts = []
        for p in range(len(pts)):
            if (pts[p] == "P"):
                verts.append(getpoint(pts[p:p+3]))
            elif (pts[p][0] == "P"):
                verts.append(getpoint([pts[p]]))
        if verts[0] and verts[1] and verts[2]:
            c = Part.Arc(verts[0],verts[1],verts[2])
    elif (data[0][0] == "P"):
        # 2-point circle
        verts = []
        rad = None
        for p in range(len(data)):
            if (data[p] == "P"):
                verts.append(getpoint(data[p:p+4]))
            elif (data[p][0] == "P"):
                verts.append(getpoint([data[p]]))
            elif (data[p] == "VAL"):
                rad = float(data[p+1])
            elif (data[p][0] == "L"):
                lines.append(objects[data[p]])
        c = Part.Circle()
        c.Center = verts[0]
        if rad: c.Radius = rad
        else: c.Radius = DraftVecUtils.new(verts[0],verts[1]).Length
    elif (data[0][0] == "L"):
        # 2-lines circle
        lines = []
        rad = None
        for p in range(len(data)):
            if (data[p] == "VAL"):
                rad = float(data[p+1])
            elif (data[p][0] == "L"):
                lines.append(objects[data[p]])
        circles = DraftGeomUtils.circleFrom2LinesRadius(lines[0],lines[1],rad)
        if circles: c = circles[0]
    if c: return c.toShape()

def getline(data):
    print "found line ", data
    "turns an OCA line definition into a FreeCAD Part Edge"
    verts = []
    for p in range(len(data)):
        if (data[p] == "P"):
            verts.append(getpoint(data[p:p+4]))
        elif (data[p][0] == "P"):
            verts.append(getpoint([data[p]]))
    l = Part.Line(verts[0],verts[1])
    return l.toShape()

def gettranslation(data):
    "retrieves a transformation vector"
    print "found translation ",data
    if (data[0] == "Z"):
        return Vector(0,0,float(data[1]))
    elif (data[0] == "Y"):
        return Vector(0,float(data[1]),0)
    elif (data[0] == "X"):
        return Vector(float(data[1]),0,0)    
    return Vector(0,0,0)

def writepoint(vector):
    "writes a FreeCAD vector in OCA format"
    return "P("+str(vector.x)+" "+str(vector.y)+" "+str(vector.z)+")"

def createobject(id,doc):
    "creates an object in the current document"
    if isinstance(objects[id],Part.Shape):
        ob = doc.addObject("Part::Feature",id)
        ob.Shape = objects[id]
        if gui: ob.ViewObject.ShapeColor = color

def parse(filename,doc):
    "inports an opened OCA file into the given doc"
    filebuffer = pythonopen(filename)
    global objects
    objects = {}
    global color
    color = (0,0,0)
    for l in filebuffer:
        readline = l.replace(","," ").upper()
        if ("=" in readline):
            # entity definitions
            pair = readline.split("=")
            id = pair[0]
            data = pair[1]
            data = data.replace(","," ")
            data = data.replace("("," ")
            data = data.replace(")"," ")
            data = data.split()
            if id[0] == "P":
                # point
                objects[id] = getpoint(data)
            elif ((id[0] == "A") and params.GetBool("ocaareas")):
                # area
                objects[id] = getarea(data)
                createobject(id,doc)

            elif id[0] == "C":
                # arc or circle
                objects[id] = getarc(data)
                createobject(id,doc)

            elif id[0] == "L":
                # line
                objects[id] = getline(data)
                createobject(id,doc)
                
            elif id[0] == "R":
                # translation
                objects[id] = gettranslation(data)

        elif (readline[0:6] == "DEFCOL"):
            # color
            c = readline.split()
            color = (float(c[1])/255,float(c[2])/255,float(c[3])/255)

    del color

def decodeName(name):
    "decodes encoded strings"
    try:
        decodedName = (name.decode("utf8"))
    except UnicodeDecodeError:
        try:
            decodedName = (name.decode("latin1"))
        except UnicodeDecodeError:
            print "oca: error: couldn't determine character encoding"
            decodedName = name
    return decodedName
    
def open(filename):
    docname=os.path.split(filename)[1]
    doc=FreeCAD.newDocument(docname)
    if (docname[-4:] == "gcad"): doc.Label = decodeName(docname[:-5])
    else: doc.Label = decodeName(docname[:-4])
    parse(filename,doc)
    doc.recompute()

def insert(filename,docname):
    try:
        doc=FreeCAD.getDocument(docname)
    except:
        doc=FreeCAD.newDocument(docname)
    FreeCAD.ActiveDocument = doc
    parse(filename,doc)
    doc.recompute()
            
def export(exportList,filename):
    "called when freecad exports a file"
    faces = []
    edges = []
    
    # getting faces and edges
    for ob in exportList:
        if ob.Shape.Faces:
            for f in ob.Shape.Faces:
                faces.append(f)
        else:
            for e in ob.Shape.Edges:
                edges.append(e)
    if not (edges or faces):
        print "oca: found no data to export"
        return
    
    # writing file
    oca = pythonopen(filename,'wb')
    oca.write("#oca file generated from FreeCAD\r\n")
    oca.write("# edges\r\n")
    count = 1
    for e in edges:
        if DraftGeomUtils.geomType(e) == "Line":
            oca.write("L"+str(count)+"=")
            oca.write(writepoint(e.Vertexes[0].Point))
            oca.write(" ")
            oca.write(writepoint(e.Vertexes[-1].Point))
            oca.write("\r\n")
        elif DraftGeomUtils.geomType(e) == "Circle":
            if (len(e.Vertexes) > 1):
                oca.write("C"+str(count)+"=ARC ")
                oca.write(writepoint(e.Vertexes[0].Point))
                oca.write(" ")
                oca.write(writepoint(DraftGeomUtils.findMidpoint(e)))
                oca.write(" ")
                oca.write(writepoint(e.Vertexes[-1].Point))
            else:
                oca.write("C"+str(count)+"= ")
                oca.write(writepoint(e.Curve.Center))
                oca.write(" ")
                oca.write(str(e.Curve.Radius))
            oca.write("\r\n")
        count += 1
    oca.write("# faces\r\n")
    for f in faces:
        oca.write("A"+str(count)+"=S(POL")
        for v in f.Vertexes:
            oca.write(" ")
            oca.write(writepoint(v.Point))
        oca.write(" ")
        oca.write(writepoint(f.Vertexes[0].Point))
        oca.write(")\r\n")
        count += 1

    # closing
    oca.close()
    FreeCAD.Console.PrintMessage("successfully exported "+filename)
    
        
            
            
        
    
            
                
                
            
        

        
    
