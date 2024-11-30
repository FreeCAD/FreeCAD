#! python
# -*- coding: utf-8 -*-
# (c) 2011 Werner Mayer LGPL
#

from __future__ import division # allows floating point division from integers
import FreeCAD, Part, math
from FreeCAD import Base

class Epitrochoid:
    def __init__(self, obj):
        ''' Add the properties: Radius1, Radius2, Distance, Segments '''
        obj.addProperty("App::PropertyLength","Radius1","Epitrochoid","Interior radius").Radius1=60.0
        obj.addProperty("App::PropertyLength","Radius2","Epitrochoid","Exterior radius").Radius2=20.0
        obj.addProperty("App::PropertyLength","Distance","Epitrochoid","Distance").Distance=10.0
        obj.addProperty("App::PropertyInteger","Segments","Epitrochoid","Number of the line segments").Segments=72
        obj.Proxy = self

    def onChanged(self, fp, prop):
        if prop == "Radius1" or prop == "Radius2" or prop == "Distance" or prop == "Segments": #if one of these is changed
            self.execute(fp)

    def execute(self, fp): #main part of script
        steps=fp.Segments  #get value from property
        dang=math.radians(360/steps)
        r2=fp.Radius2
        r1=fp.Radius1
        f1 = r1 + r2
        f2 = f1 / r2
        d=fp.Distance
        ang=0
        z=0

        if r2 == 0:
            raise ValueError("Exterior radius must not be zero")

        for i in range(steps):
            if i==0:
                x1=f1*math.cos(ang)-d*math.cos(f2*ang) #coords for line startpoint
                y1=f1*math.sin(ang)-d*math.sin(f2*ang)
                ang=dang
                x2=f1*math.cos(ang)-d*math.cos(f2*ang) #coords for line endpoint
                y2=f1*math.sin(ang)-d*math.sin(f2*ang)
                seg=Part.makeLine((x1,y1,z),(x2,y2,z))
                wire=Part.Wire([seg])
                x1=x2
                y1=y2
            else:
                x2=f1*math.cos(ang)-d*math.cos(f2*ang)
                y2=f1*math.sin(ang)-d*math.sin(f2*ang)
                seg=Part.makeLine((x1,y1,z),(x2,y2,z))
                wire=Part.Wire([wire,seg])
                x1=x2
                y1=y2
            ang=ang+dang #increment angle
        fp.Shape = wire  #result shape

def makeEpitrochoid():
    doc = FreeCAD.activeDocument()
    if doc is None:
        doc = FreeCAD.newDocument()
    epitrochoid=doc.addObject("Part::FeaturePython","Epitrochoid") #add object to document
    epitrochoid.Label = "Epitrochoid"
    Epitrochoid(epitrochoid)
    epitrochoid.ViewObject.Proxy=0
    doc.recompute()

if __name__ == "__main__": #feature will be generated after macro execution
    makeEpitrochoid()
