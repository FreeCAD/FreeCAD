# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2015 Dan Falck <ddfalck@gmail.com>                      *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************
'''Selection gates and observers to control selectability while building Path operations '''

import FreeCAD
import FreeCADGui
from FreeCAD import Vector


def equals(p1, p2):
    '''returns True if vertexes have same coordinates within precision amount of digits '''
    precision = 12
    p = precision
    u = Vector(p1.X, p1.Y, p1.Z)
    v = Vector(p2.X, p2.Y, p2.Z)
    vector = (u.sub(v))
    isNull = (round(vector.x, p) == 0 and round(vector.y, p) == 0 and round(vector.z, p) == 0)
    return isNull


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


class FGate:
    def allow(self, doc, obj, sub):
        return (sub[0:4] == 'Face')


class VGate:
    def allow(self, doc, obj, sub):
        return (sub[0:6] == 'Vertex')


class EGate:
    def allow(self, doc, obj, sub):
        return (sub[0:4] == 'Edge')


class MESHGate:
    def allow(self, doc, obj, sub):
        return (obj.TypeId[0:4] == 'Mesh')


class ENGRAVEGate:
    def allow(self, doc, obj, sub):
        engraveable = False

        if hasattr(obj, "Shape"):
            if obj.Shape.BoundBox.ZLength == 0.0:
                try:
                    obj = obj.Shape
                except:
                    return False
                if len(obj.Wires) > 0:
                    engraveable = True

        return engraveable

class DRILLGate:
    def allow(self, doc, obj, sub):
        import Part
        drillable = False
        try:
            obj = obj.Shape
        except:
            return False
        if obj.ShapeType == 'Vertex':
                drillable = True
        elif obj.ShapeType in['Solid', 'Compound']:
            if sub[0:4] == 'Face':
                subobj = obj.getElement(sub)
                drillable = isinstance(subobj.Edges[0].Curve, Part.Circle)
                if str(subobj.Surface) == "<Cylinder object>":
                    drillable = True

            if sub[0:4] == 'Edge':
                o = obj.getElement(sub)
                drillable = isinstance(o.Curve, Part.Circle)

        return drillable


class PROFILEGate:
    def allow(self, doc, obj, sub):

        profileable = False
        try:
            obj = obj.Shape
        except:
            return False

        if obj.ShapeType == 'Edge':
            profileable = False

        elif obj.ShapeType == 'Compound':
            if sub[0:4] == 'Face':
                profileable = True

            if sub[0:4] == 'Edge':
                profileable = False

        elif obj.ShapeType == 'Face':
            profileable = False

        elif obj.ShapeType == 'Solid':
            if sub[0:4] == 'Face':
                profileable = True

            if sub[0:4] == 'Edge':
                profileable = False

        elif obj.ShapeType == 'Wire':
            profileable = False

        return profileable


class POCKETGate:
    def allow(self, doc, obj, sub):

        pocketable = False
        try:
            obj = obj.Shape
        except:
            return False

        if obj.ShapeType == 'Edge':
            pocketable = False

        elif obj.ShapeType == 'Face':
            pocketable = True

        elif obj.ShapeType == 'Solid':
            if sub[0:4] == 'Face':
                pocketable = True

        elif obj.ShapeType == 'Compound':
            if sub[0:4] == 'Face':
                pocketable = True

        return pocketable

class CONTOURGate:
    def allow(self, doc, obj, sub):
        pass

def contourselect():
    FreeCADGui.Selection.addSelectionGate(CONTOURGate())
    FreeCAD.Console.PrintWarning("Contour Select Mode\n")

def fselect():
    FreeCADGui.Selection.addSelectionGate(FGate())
    FreeCAD.Console.PrintWarning("Face Select Mode\n")


def vselect():
    FreeCADGui.Selection.addSelectionGate(VGate())
    FreeCAD.Console.PrintWarning("Vertex Select Mode\n")


def eselect():
    FreeCADGui.Selection.addSelectionGate(EGate())
    FreeCAD.Console.PrintWarning("Edge Select Mode\n")


def drillselect():
    FreeCADGui.Selection.addSelectionGate(DRILLGate())
    FreeCAD.Console.PrintWarning("Drilling Select Mode\n")


def engraveselect():
    FreeCADGui.Selection.addSelectionGate(ENGRAVEGate())
    FreeCAD.Console.PrintWarning("Engraving Select Mode\n")


def profileselect():
    FreeCADGui.Selection.addSelectionGate(PROFILEGate())
    FreeCAD.Console.PrintWarning("Profiling Select Mode\n")


def pocketselect():
    FreeCADGui.Selection.addSelectionGate(POCKETGate())
    FreeCAD.Console.PrintWarning("Pocketing Select Mode\n")


def surfaceselect():
    FreeCADGui.Selection.addSelectionGate(MESHGate())
    FreeCAD.Console.PrintWarning("Surfacing Select Mode\n")


def clear():
    FreeCADGui.Selection.removeSelectionGate()
    FreeCAD.Console.PrintWarning("Free Select\n")
