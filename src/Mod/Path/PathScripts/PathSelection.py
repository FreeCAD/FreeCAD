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
import PathScripts.PathLog as PathLog
import PathScripts.PathUtils as PathUtils
import math

if False:
    PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())
    PathLog.trackModule(PathLog.thisModule())


class EGate:
    def allow(self, doc, obj, sub):
        return sub and sub[0:4] == 'Edge'


class MESHGate:
    def allow(self, doc, obj, sub):
        return obj.TypeId[0:4] == 'Mesh'


class ENGRAVEGate:
    def allow(self, doc, obj, sub):
        try:
            shape = obj.Shape
        except:
            return False

        if math.fabs(shape.Volume) < 1e-9 and len(shape.Wires) > 0:
            return True

        if shape.ShapeType == 'Edge':
            return True

        if sub:
            subShape = shape.getElement(sub)
            if subShape.ShapeType == 'Edge':
                return True

        return False

class CHAMFERGate:
    def allow(self, doc, obj, sub):
        try:
            shape = obj.Shape
        except:
            return False

        if math.fabs(shape.Volume) < 1e-9 and len(shape.Wires) > 0:
            return True

        if 'Edge' == shape.ShapeType or 'Face' == shape.ShapeType:
            return True

        if sub:
            subShape = shape.getElement(sub)
            if 'Edge' == subShape.ShapeType or 'Face' == subShape.ShapeType:
                return True

        print(shape.ShapeType)
        return False


class DRILLGate:
    def allow(self, doc, obj, sub):
        PathLog.debug('obj: {} sub: {}'.format(obj, sub))
        if hasattr(obj, "Shape") and sub:
            shape = obj.Shape
            subobj = shape.getElement(sub)
            return PathUtils.isDrillable(shape, subobj, includePartials = True)
        else:
            return False


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
            if sub and sub[0:4] == 'Face':
                profileable = True

            if sub and sub[0:4] == 'Edge':
                profileable = False

        elif obj.ShapeType == 'Face':
            profileable = False

        elif obj.ShapeType == 'Solid':
            if sub and sub[0:4] == 'Face':
                profileable = True

            if sub and sub[0:4] == 'Edge':
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
            if sub and sub[0:4] == 'Face':
                pocketable = True

        elif obj.ShapeType == 'Compound':
            if sub and sub[0:4] == 'Face':
                pocketable = True

        return pocketable

class ADAPTIVEGate:
    def allow(self, doc, obj, sub):

        adaptive = True
        try:
            obj = obj.Shape
        except:
            return False
            
        return adaptive

class POCKETSHAPEGate:
    def allow(self, doc, obj, sub):

        try:
            obj = obj.Shape
        except:
            return False

        if obj.ShapeType == 'Edge':
            return True

        elif obj.ShapeType == 'Face':
            return True

        elif obj.ShapeType == 'Solid':
            if sub and (sub[0:4] == 'Face' or sub[0:4] == 'Edge'):
                return True

        elif obj.ShapeType == 'Compound':
            if sub and (sub[0:4] == 'Face' or sub[0:4] == 'Edge'):
                return True

        return False

class CONTOURGate:
    def allow(self, doc, obj, sub):
        pass

def contourselect():
    FreeCADGui.Selection.addSelectionGate(CONTOURGate())
    FreeCAD.Console.PrintWarning("Contour Select Mode\n")

def eselect():
    FreeCADGui.Selection.addSelectionGate(EGate())
    FreeCAD.Console.PrintWarning("Edge Select Mode\n")

def drillselect():
    FreeCADGui.Selection.addSelectionGate(DRILLGate())
    FreeCAD.Console.PrintWarning("Drilling Select Mode\n")

def engraveselect():
    FreeCADGui.Selection.addSelectionGate(ENGRAVEGate())
    FreeCAD.Console.PrintWarning("Engraving Select Mode\n")

def chamferselect():
    FreeCADGui.Selection.addSelectionGate(CHAMFERGate())
    FreeCAD.Console.PrintWarning("Deburr Select Mode\n")

def profileselect():
    FreeCADGui.Selection.addSelectionGate(PROFILEGate())
    FreeCAD.Console.PrintWarning("Profiling Select Mode\n")

def pocketselect():
    FreeCADGui.Selection.addSelectionGate(POCKETGate())
    FreeCAD.Console.PrintWarning("Pocketing Select Mode\n")

def adaptiveselect():
    FreeCADGui.Selection.addSelectionGate(ADAPTIVEGate())
    FreeCAD.Console.PrintWarning("Adaptive Select Mode\n")

def pocketshapeselect():
    FreeCADGui.Selection.addSelectionGate(POCKETSHAPEGate())
    FreeCAD.Console.PrintWarning("Pocketing Select Mode (shape)\n")

def surfaceselect():
    FreeCADGui.Selection.addSelectionGate(MESHGate())
    FreeCAD.Console.PrintWarning("Surfacing Select Mode\n")

def select(op):
    opsel = {}
    opsel['Contour'] = contourselect
    opsel['Deburr'] = chamferselect
    opsel['Drilling'] = drillselect
    opsel['Engrave'] = engraveselect
    opsel['Helix'] = drillselect
    opsel['MillFace'] = pocketselect
    opsel['Pocket'] = pocketselect
    opsel['Pocket 3D'] = pocketselect
    opsel['Pocket Shape'] = pocketshapeselect
    opsel['Profile Edges'] = eselect
    opsel['Profile Faces'] = profileselect
    opsel['Surface'] = surfaceselect
    opsel['Adaptive'] = adaptiveselect
    return opsel[op]

def clear():
    FreeCADGui.Selection.removeSelectionGate()
    FreeCAD.Console.PrintWarning("Free Select\n")
