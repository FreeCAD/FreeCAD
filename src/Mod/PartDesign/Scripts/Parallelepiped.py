#! python
# -*- coding: utf-8 -*-
# (c) 2011 Werner Mayer LGPL

"""
An example for a high-level custom feature object to form a full-parametric parallelepiped.
"""

__author__ = "Werner Mayer <wmayer@users.sourceforge.net>"

import FreeCAD, Part, math
from FreeCAD import Base

class Parallelepiped:
    def __init__(self, obj):
        ''' Add the properties: Length, Edges, Radius, Height '''
        obj.addProperty("App::PropertyVector","A","Parallelepiped","Vector").A=Base.Vector(1,0,0)
        obj.addProperty("App::PropertyVector","B","Parallelepiped","Vector").B=Base.Vector(0,1,0)
        obj.addProperty("App::PropertyVector","C","Parallelepiped","Vector").C=Base.Vector(0,0,1)
        obj.Proxy = self

    def onChanged(self, fp, prop):
        if prop == "A" or prop == "B" or prop == "C":
            self.execute(fp)

    def execute(self, fp):
        a = fp.A
        b = fp.B
        c = fp.C

        m=Base.Matrix()
        m.A11=a.x
        m.A12=a.y
        m.A13=a.z
        m.A21=b.x
        m.A22=b.y
        m.A23=b.z
        m.A31=c.x
        m.A32=c.y
        m.A33=c.z
        box = Part.makeBox(1,1,1)
        fp.Shape = box.transformGeometry(m)

class BoxCylinder:
    def __init__(self, obj):
        obj.addProperty("App::PropertyFloat","Length","BoxCylinder","Length").Length=10.0
        obj.addProperty("App::PropertyFloat","Width","BoxCylinder","Width").Width=10.0
        obj.addProperty("App::PropertyLink","Source","BoxCylinder","Source").Source=None
        obj.Proxy = self

    def onChanged(self, fp, prop):
        if prop == "Length" or prop == "Width":
            self.execute(fp)

    def execute(self, fp):
        FreeCAD.Console.PrintMessage(str(fp.Source)+"\n")
        if fp.Source is None:
            return
        r = fp.Source.Radius
        l = fp.Length
        w = fp.Width
        h = 2*r+10
        fp.Shape = Part.makeBox(l,w,h)

def makeParallelepiped():
    doc = FreeCAD.activeDocument()
    if doc is None:
        doc = FreeCAD.newDocument()
    obj=doc.addObject("Part::FeaturePython","Parallelepiped")
    obj.Label = "Parallelepiped"
    Parallelepiped(obj)
    obj.ViewObject.Proxy=0


def makeBoxCylinder():
    doc = FreeCAD.activeDocument()
    if doc is None:
        doc = FreeCAD.newDocument()
    cyl=doc.addObject("Part::Cylinder","Cylinder")
    cyl.Radius=16.0
    cyl.Height=800.0
    obj=doc.addObject("Part::FeaturePython","Box")
    BoxCylinder(obj)
    obj.Source=cyl
    obj.Length=800.0
    obj.Width=600.0
    obj.ViewObject.Proxy=0
    doc.recompute()
