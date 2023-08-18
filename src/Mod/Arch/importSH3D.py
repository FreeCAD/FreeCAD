#***************************************************************************
#*   Copyright (c) 2016 Yorik van Havre <yorik@uncreated.net>              *
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

__title__  = "FreeCAD SweetHome3D Importer"
__author__ = "Yorik van Havre"
__url__    = "https://www.freecad.org"

import math
import os
import tempfile
import xml.sax
import zipfile

import FreeCAD
import Arch
import Draft
import Mesh
import Part

## @package importSH3D
#  \ingroup ARCH
#  \brief SH3D (SweetHome3D) file format importer
#
#  This module provides tools to import SH3D files created from Sweet Home 3D.

DEBUG = True

if open.__module__ in ['__builtin__','io']:
    pyopen = open # because we'll redefine open below


def open(filename):
    "called when freecad wants to open a file"
    docname = os.path.splitext(os.path.basename(filename))[0]
    doc = FreeCAD.newDocument(docname)
    doc.Label = docname
    FreeCAD.ActiveDocument = doc
    read(filename)
    return doc


def insert(filename,docname):
    "called when freecad wants to import a file"
    try:
        doc = FreeCAD.getDocument(docname)
    except NameError:
        doc = FreeCAD.newDocument(docname)
    FreeCAD.ActiveDocument = doc
    read(filename)
    return doc


def read(filename):
    "reads the file and creates objects in the active document"

    z = zipfile.ZipFile(filename)
    homexml = z.read("Home.xml")
    handler = SH3DHandler(z)
    xml.sax.parseString(homexml,handler)
    FreeCAD.ActiveDocument.recompute()
    if not handler.makeIndividualWalls:
        delete = []
        walls = []
        for k,lines in handler.lines.items():
            sk = FreeCAD.ActiveDocument.addObject("Sketcher::SketchObject","Walls_trace")
            for l in lines:
                for edge in l.Shape.Edges:
                    sk.addGeometry(edge.Curve)
                delete.append(l.Name)
            FreeCAD.ActiveDocument.recompute()
            k = k.split(";")
            walls.append(Arch.makeWall(baseobj=sk,width=float(k[0]),height=float(k[1])))
        for d in delete:
            FreeCAD.ActiveDocument.removeObject(d)
        w = walls.pop()
        w.Additions = walls
        w.Subtractions = handler.windows
    g = FreeCAD.ActiveDocument.addObject("App::DocumentObjectGroup","Furniture")
    g.Group = handler.furniture
    FreeCAD.ActiveDocument.recompute()


class SH3DHandler(xml.sax.ContentHandler):

    def __init__(self,z):

        super().__init__()
        self.makeIndividualWalls = False
        self.z = z
        self.windows = []
        self.furniture = []
        self.lines = {}

    def startElement(self, tag, attributes):

        if tag == "wall":
            name = attributes["id"]
            p1 = FreeCAD.Vector(float(attributes["xStart"])*10,float(attributes["yStart"])*10,0)
            p2 = FreeCAD.Vector(float(attributes["xEnd"])*10,float(attributes["yEnd"])*10,0)
            height = float(attributes["height"])*10
            thickness = float(attributes["thickness"])*10
            if DEBUG: print("Creating wall: ",name)
            line = Draft.makeLine(p1,p2)
            if self.makeIndividualWalls:
                wall = Arch.makeWall(baseobj=line,width=thickness,height=height,name=name)
                wall.Label = name
            else:
                self.lines.setdefault(str(thickness)+";"+str(height),[]).append(line)

        elif tag == "pieceOfFurniture":
            name = attributes["name"]
            data = self.z.read(attributes["model"])
            th,tf = tempfile.mkstemp(suffix=".obj")
            f = pyopen(tf,"wb")
            f.write(data)
            f.close()
            os.close(th)
            m = Mesh.read(tf)
            fx = (float(attributes["width"])/100)/m.BoundBox.XLength
            fy = (float(attributes["height"])/100)/m.BoundBox.YLength
            fz = (float(attributes["depth"])/100)/m.BoundBox.ZLength
            mat = FreeCAD.Matrix()
            mat.scale(1000*fx,1000*fy,1000*fz)
            mat.rotateX(math.pi/2)
            mat.rotateZ(math.pi)
            if DEBUG: print("Creating furniture: ",name)
            if "angle" in attributes:
                mat.rotateZ(float(attributes["angle"]))
            m.transform(mat)
            os.remove(tf)
            p = m.BoundBox.Center.negative()
            p = p.add(FreeCAD.Vector(float(attributes["x"])*10,float(attributes["y"])*10,0))
            p = p.add(FreeCAD.Vector(0,0,m.BoundBox.Center.z-m.BoundBox.ZMin))
            m.Placement.Base = p
            obj = FreeCAD.ActiveDocument.addObject("Mesh::Feature",name)
            obj.Mesh = m
            self.furniture.append(obj)

        elif tag == "doorOrWindow":
            name = attributes["name"]
            data = self.z.read(attributes["model"])
            th,tf = tempfile.mkstemp(suffix=".obj")
            f = pyopen(tf,"wb")
            f.write(data)
            f.close()
            os.close(th)
            m = Mesh.read(tf)
            fx = (float(attributes["width"])/100)/m.BoundBox.XLength
            fy = (float(attributes["height"])/100)/m.BoundBox.YLength
            fz = (float(attributes["depth"])/100)/m.BoundBox.ZLength
            mat = FreeCAD.Matrix()
            mat.scale(1000*fx,1000*fy,1000*fz)
            mat.rotateX(math.pi/2)
            m.transform(mat)
            b = m.BoundBox
            v1 = FreeCAD.Vector(b.XMin,b.YMin-500,b.ZMin)
            v2 = FreeCAD.Vector(b.XMax,b.YMin-500,b.ZMin)
            v3 = FreeCAD.Vector(b.XMax,b.YMax+500,b.ZMin)
            v4 = FreeCAD.Vector(b.XMin,b.YMax+500,b.ZMin)
            sub = Part.makePolygon([v1,v2,v3,v4,v1])
            sub = Part.Face(sub)
            sub = sub.extrude(FreeCAD.Vector(0,0,b.ZLength))
            os.remove(tf)
            shape = Arch.getShapeFromMesh(m)
            if not shape:
                shape=Part.Shape()
                shape.makeShapeFromMesh(m.Topology,0.100000)
                shape = shape.removeSplitter()
            if shape:
                if DEBUG: print("Creating window: ",name)
                if "angle" in attributes:
                    shape.rotate(shape.BoundBox.Center,FreeCAD.Vector(0,0,1),math.degrees(float(attributes["angle"])))
                    sub.rotate(shape.BoundBox.Center,FreeCAD.Vector(0,0,1),math.degrees(float(attributes["angle"])))
                p = shape.BoundBox.Center.negative()
                p = p.add(FreeCAD.Vector(float(attributes["x"])*10,float(attributes["y"])*10,0))
                p = p.add(FreeCAD.Vector(0,0,shape.BoundBox.Center.z-shape.BoundBox.ZMin))
                if "elevation" in attributes:
                    p = p.add(FreeCAD.Vector(0,0,float(attributes["elevation"])*10))
                shape.translate(p)
                sub.translate(p)
                obj = FreeCAD.ActiveDocument.addObject("Part::Feature",name+"_body")
                obj.Shape = shape
                subobj = FreeCAD.ActiveDocument.addObject("Part::Feature",name+"_sub")
                subobj.Shape = sub
                if FreeCAD.GuiUp:
                    subobj.ViewObject.hide()
                win = Arch.makeWindow(baseobj=obj,name=name)
                win.Label = name
                win.Subvolume = subobj
                self.windows.append(win)
            else:
                print("importSH3D: Error creating shape for door/window "+name)
