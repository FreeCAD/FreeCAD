#! python
# -*- coding: utf-8 -*-
# (c) 2010 Werner Mayer LGPL

"""
An example for a high-level custom feature object to make a so called "radial copy".
"""
__author__ = "Werner Mayer <wmayer@users.sourceforge.net>"

import FreeCAD, FreeCADGui, Part, math
from PySide import QtGui
from FreeCAD import Base

def makeCopy(shape, radius, angle):
    mat = Base.Matrix()
    mat.rotateZ(math.radians(angle))
    step = int(360.0 / angle)
    shape = shape.copy()
    shape.translate((radius, 0, 0))
    comp = shape.copy()
    for i in range(step):
        shape.transformShape(mat)
        comp = comp.fuse(shape)
    return comp


class RadialCopy:
    def __init__(self, obj):
        obj.addProperty("App::PropertyLength","Radius","","Radius").Radius=10.0
        obj.addProperty("App::PropertyLength","Angle" ,"","Angle").Angle=20.0
        obj.addProperty("App::PropertyLink","Source" ,"","Source shape").Source=None
        obj.Proxy = self

#   def onChanged(self, fp, prop):
#       if prop == "Angle" or prop == "Radius":
#           self.execute(fp)

    def execute(self, fp):
        shape = fp.Source.Shape
        radius = fp.Radius
        angle = fp.Angle
        fp.Shape = makeCopy(shape, radius, angle)

def makeRadialCopy():
    sel = FreeCADGui.Selection.getSelection()
    try:
        sel = sel[0]
        shape = sel.Shape
        name = sel.Label
    except (IndexError, AttributeError):
        QtGui.QMessageBox.critical(None,"Wrong selection","Please select a shape object")
        #raise Exception("Nothing selected")
    else:
        doc = sel.Document
        rc = doc.addObject("Part::FeaturePython","RadialCopy")
        rc.Label = name+"(Radial Copy)"
        RadialCopy(rc)
        rc.Source = sel
        rc.ViewObject.Proxy=0

