#! python
# -*- coding: utf-8 -*-
# (c) 2010 Werner Mayer LGPL

"""
An example for a high-level custom feature object to form a full-parametric distance bolt.
"""

__author__ = "Werner Mayer <wmayer@users.sourceforge.net>"

import FreeCAD, Part, math
from FreeCAD import Base

class DistanceBolt:
    def __init__(self, obj):
        ''' Add the properties: Length, Edges, Radius, Height '''
        obj.addProperty("App::PropertyInteger","Edges","Bolt","Number of edges of the outline").Edges=6
        obj.addProperty("App::PropertyLength","Length","Bolt","Length of the edges of the outline").Length=10.0
        obj.addProperty("App::PropertyLength","Radius","Bolt","Radius of the inner circle").Radius=4.0
        obj.addProperty("App::PropertyLength","Height","Bolt","Height of the extrusion").Height=20.0
        obj.Proxy = self

    def onChanged(self, fp, prop):
        if prop == "Edges" or prop == "Length" or prop == "Radius" or prop == "Height":
            self.execute(fp)

    def execute(self, fp):
        edges = fp.Edges
        if edges < 3:
            edges = 3
        length = fp.Length
        radius = fp.Radius
        height = fp.Height

        m=Base.Matrix()
        m.rotateZ(math.radians(360.0/edges))

        # create polygon
        polygon = []
        v=Base.Vector(length,0,0)
        for i in range(edges):
            polygon.append(v)
            v = m.multiply(v)
        polygon.append(v)
        wire = Part.makePolygon(polygon)

        # create circle
        circ=Part.makeCircle(radius)

        # Create the face with the polygon as outline and the circle as hole
        face=Part.Face([wire,Part.Wire(circ)])

        # Extrude in z to create the final solid
        extrude=face.extrude(Base.Vector(0,0,height))
        fp.Shape = extrude

def makeDistanceBolt():
    doc = FreeCAD.activeDocument()
    if doc is None:
        doc = FreeCAD.newDocument()
    bolt=doc.addObject("Part::FeaturePython","Distance_Bolt")
    bolt.Label = "Distance bolt"
    DistanceBolt(bolt)
    bolt.ViewObject.Proxy=0
