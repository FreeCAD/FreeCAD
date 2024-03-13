# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2015 Dan Falck <ddfalck@gmail.com>                      *
# *   Copyright (c) 2021 Schildkroet                                        *
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

"""Selection gates and observers to control selectability while building Path operations """

import FreeCAD
import FreeCADGui
import Path
import Path.Base.Drillable as Drillable
import math

Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())
Path.Log.trackModule(Path.Log.thisModule())


class PathBaseGate(object):
    pass


class EGate(PathBaseGate):
    def allow(self, doc, obj, sub):
        return sub and sub[0:4] == "Edge"


class MESHGate(PathBaseGate):
    def allow(self, doc, obj, sub):
        return obj.TypeId[0:4] == "Mesh"


class VCARVEGate:
    def allow(self, doc, obj, sub):
        try:
            shape = obj.Shape
        except Exception:
            return False

        if math.fabs(shape.Volume) < 1e-9 and len(shape.Wires) > 0:
            return True

        if shape.ShapeType == "Face":
            return True

        elif shape.ShapeType == "Solid":
            if sub and sub[0:4] == "Face":
                return True

        elif shape.ShapeType == "Compound":
            if sub and sub[0:4] == "Face":
                return True

        if sub:
            subShape = shape.getElement(sub)
            if subShape.ShapeType == "Edge":
                return False

        return False


class ENGRAVEGate(PathBaseGate):
    def allow(self, doc, obj, sub):
        try:
            shape = obj.Shape
        except Exception:
            return False

        if math.fabs(shape.Volume) < 1e-9 and len(shape.Wires) > 0:
            return True

        if shape.ShapeType == "Edge":
            return True

        if sub:
            subShape = shape.getElement(sub)
            if subShape.ShapeType == "Edge":
                return True

        return False


class CHAMFERGate(PathBaseGate):
    def allow(self, doc, obj, sub):
        try:
            shape = obj.Shape
        except Exception:
            return False

        if math.fabs(shape.Volume) < 1e-9 and len(shape.Wires) > 0:
            return True

        if "Edge" == shape.ShapeType or "Face" == shape.ShapeType:
            return True

        if sub:
            subShape = shape.getElement(sub)
            if subShape.ShapeType == "Edge":
                return True
            elif subShape.ShapeType == "Face":
                return True

        return False


class DRILLGate(PathBaseGate):
    def allow(self, doc, obj, sub):
        Path.Log.debug("obj: {} sub: {}".format(obj, sub))
        if not hasattr(obj, "Shape"):
            return False
        shape = obj.Shape
        subobj = shape.getElement(sub)
        if subobj.ShapeType not in ["Edge", "Face"]:
            return False
        return Drillable.isDrillable(shape, subobj, vector=None, allowPartial=True)


class FACEGate(PathBaseGate):
    def allow(self, doc, obj, sub):
        isFace = False

        try:
            obj = obj.Shape
        except Exception:
            return False

        if obj.ShapeType == "Compound":
            if sub and sub[0:4] == "Face":
                isFace = True

        elif obj.ShapeType == "Face":  # 3D Face, not flat, planar?
            isFace = True

        elif obj.ShapeType == "Solid":
            if sub and sub[0:4] == "Face":
                isFace = True

        return isFace


class PROFILEGate(PathBaseGate):
    def allow(self, doc, obj, sub):
        if sub and sub[0:4] == "Edge":
            return True

        try:
            obj = obj.Shape
        except Exception:
            return False

        if obj.ShapeType == "Compound":
            if sub and sub[0:4] == "Face":
                return True

        elif obj.ShapeType == "Face":
            return True

        elif obj.ShapeType == "Solid":
            if sub and sub[0:4] == "Face":
                return True

        elif obj.ShapeType == "Wire":
            return True

        return False


class POCKETGate(PathBaseGate):
    def allow(self, doc, obj, sub):

        pocketable = False
        try:
            obj = obj.Shape
        except Exception:
            return False

        if obj.ShapeType == "Edge":
            pocketable = False

        elif obj.ShapeType == "Face":
            pocketable = True

        elif obj.ShapeType == "Solid":
            if sub and sub[0:4] == "Face":
                pocketable = True

        elif obj.ShapeType == "Compound":
            if sub and sub[0:4] == "Face":
                pocketable = True

        return pocketable


class ADAPTIVEGate(PathBaseGate):
    def allow(self, doc, obj, sub):

        adaptive = True
        try:
            obj = obj.Shape
        except Exception:
            return False

        return adaptive


class CONTOURGate(PathBaseGate):
    def allow(self, doc, obj, sub):
        pass


class PROBEGate:
    def allow(self, doc, obj, sub):
        pass


class TURNGate(PathBaseGate):
    def allow(self, doc, obj, sub):
        Path.Log.debug("obj: {} sub: {}".format(obj, sub))
        if hasattr(obj, "Shape") and sub:
            shape = obj.Shape
            subobj = shape.getElement(sub)
            return Drillable.isDrillable(shape, subobj, vector=None)
        else:
            return False


class ALLGate(PathBaseGate):
    def allow(self, doc, obj, sub):
        if sub and sub[0:6] == "Vertex":
            return True
        if sub and sub[0:4] == "Edge":
            return True
        if sub and sub[0:4] == "Face":
            return True
        return False


def contourselect():
    FreeCADGui.Selection.addSelectionGate(CONTOURGate())
    if not Path.Preferences.suppressSelectionModeWarning():
        FreeCAD.Console.PrintWarning("Contour Select Mode\n")


def eselect():
    FreeCADGui.Selection.addSelectionGate(EGate())
    if not Path.Preferences.suppressSelectionModeWarning():
        FreeCAD.Console.PrintWarning("Edge Select Mode\n")


def drillselect():
    FreeCADGui.Selection.addSelectionGate(DRILLGate())
    if not Path.Preferences.suppressSelectionModeWarning():
        FreeCAD.Console.PrintWarning("Drilling Select Mode\n")


def engraveselect():
    FreeCADGui.Selection.addSelectionGate(ENGRAVEGate())
    if not Path.Preferences.suppressSelectionModeWarning():
        FreeCAD.Console.PrintWarning("Engraving Select Mode\n")


def fselect():
    FreeCADGui.Selection.addSelectionGate(FACEGate())  # Was PROFILEGate()
    if not Path.Preferences.suppressSelectionModeWarning():
        FreeCAD.Console.PrintWarning("Profiling Select Mode\n")


def chamferselect():
    FreeCADGui.Selection.addSelectionGate(CHAMFERGate())
    if not Path.Preferences.suppressSelectionModeWarning():
        FreeCAD.Console.PrintWarning("Deburr Select Mode\n")


def profileselect():
    FreeCADGui.Selection.addSelectionGate(PROFILEGate())
    if not Path.Preferences.suppressSelectionModeWarning():
        FreeCAD.Console.PrintWarning("Profiling Select Mode\n")


def pocketselect():
    FreeCADGui.Selection.addSelectionGate(POCKETGate())
    if not Path.Preferences.suppressSelectionModeWarning():
        FreeCAD.Console.PrintWarning("Pocketing Select Mode\n")


def adaptiveselect():
    FreeCADGui.Selection.addSelectionGate(ADAPTIVEGate())
    if not Path.Preferences.suppressSelectionModeWarning():
        FreeCAD.Console.PrintWarning("Adaptive Select Mode\n")


def slotselect():
    FreeCADGui.Selection.addSelectionGate(ALLGate())
    if not Path.Preferences.suppressSelectionModeWarning():
        FreeCAD.Console.PrintWarning("Slot Cutter Select Mode\n")


def surfaceselect():
    gate = False
    if MESHGate() or FACEGate():
        gate = True
    FreeCADGui.Selection.addSelectionGate(gate)
    if not Path.Preferences.suppressSelectionModeWarning():
        FreeCAD.Console.PrintWarning("Surfacing Select Mode\n")


def vcarveselect():
    FreeCADGui.Selection.addSelectionGate(VCARVEGate())
    if not Path.Preferences.suppressSelectionModeWarning():
        FreeCAD.Console.PrintWarning("Vcarve Select Mode\n")


def probeselect():
    FreeCADGui.Selection.addSelectionGate(PROBEGate())
    if not Path.Preferences.suppressSelectionModeWarning():
        FreeCAD.Console.PrintWarning("Probe Select Mode\n")


def customselect():
    if not Path.Preferences.suppressSelectionModeWarning():
        FreeCAD.Console.PrintWarning("Custom Select Mode\n")


def turnselect():
    FreeCADGui.Selection.addSelectionGate(TURNGate())
    if not Path.Preferences.suppressSelectionModeWarning():
        FreeCAD.Console.PrintWarning("Turning Select Mode\n")


def select(op):
    opsel = {}
    opsel["Contour"] = contourselect  # deprecated
    opsel["Deburr"] = chamferselect
    opsel["Drilling"] = drillselect
    opsel["Engrave"] = engraveselect
    opsel["Helix"] = drillselect
    opsel["MillFace"] = pocketselect
    opsel["Pocket"] = pocketselect
    opsel["Pocket 3D"] = pocketselect
    opsel["Pocket3D"] = pocketselect  # deprecated
    opsel["Pocket Shape"] = pocketselect
    opsel["Profile Edges"] = eselect  # deprecated
    opsel["Profile Faces"] = fselect  # deprecated
    opsel["Profile"] = profileselect
    opsel["Slot"] = slotselect
    opsel["Surface"] = surfaceselect
    opsel["Waterline"] = surfaceselect
    opsel["Adaptive"] = adaptiveselect
    opsel["Vcarve"] = vcarveselect
    opsel["Probe"] = probeselect
    opsel["Custom"] = customselect
    opsel["ThreadMilling"] = drillselect
    opsel["TurnFace"] = turnselect
    opsel["TurnProfile"] = turnselect
    opsel["TurnPartoff"] = turnselect
    opsel["TurnRough"] = turnselect
    return opsel[op]


def clear():
    FreeCADGui.Selection.removeSelectionGate()
    if not Path.Preferences.suppressSelectionModeWarning():
        FreeCAD.Console.PrintWarning("Free Select\n")
