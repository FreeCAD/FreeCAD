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

def profileselect():
    FreeCADGui.Selection.addSelectionGate(PROFILEGate())
    FreeCAD.Console.PrintWarning("Profiling Select Mode\n")

def pocketselect():
    FreeCADGui.Selection.addSelectionGate(POCKETGate())
    FreeCAD.Console.PrintWarning("Pocketing Select Mode\n")

def surfaceselect():
    FreeCADGui.Selection.addSelectionGate(MESHGate())
    FreeCAD.Console.PrintWarning("Surfacing Select Mode\n")

def select(op):
    opsel = {}
    opsel['Contour'] = contourselect
    opsel['Drilling'] = drillselect
    opsel['Engrave'] = engraveselect
    opsel['Helix'] = drillselect
    opsel['MillFace'] = pocketselect
    opsel['Pocket'] = pocketselect
    opsel['Pocket 3D'] = pocketselect
    opsel['Pocket Shape'] = pocketselect
    opsel['Profile Edges'] = eselect
    opsel['Profile Faces'] = profileselect
    opsel['Surface'] = surfaceselect
    return opsel[op]

def clear():
    FreeCADGui.Selection.removeSelectionGate()
    FreeCAD.Console.PrintWarning("Free Select\n")
