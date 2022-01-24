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
import PathScripts.PathLog as PathLog
import PathScripts.PathPreferences as PathPreferences
import PathScripts.drillableLib as drillableLib
import math

PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())
# PathLog.trackModule(PathLog.thisModule())


class PathBaseGate(object):
    # pylint: disable=no-init
    pass


class EGate(PathBaseGate):
    def allow(self, doc, obj, sub):  # pylint: disable=unused-argument
        return sub and sub[0:4] == "Edge"


class MESHGate(PathBaseGate):
    def allow(self, doc, obj, sub):  # pylint: disable=unused-argument
        return obj.TypeId[0:4] == "Mesh"


class VCARVEGate:
    def allow(self, doc, obj, sub):
        try:
            shape = obj.Shape
        except Exception:  # pylint: disable=broad-except
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
    def allow(self, doc, obj, sub):  # pylint: disable=unused-argument
        try:
            shape = obj.Shape
        except Exception:  # pylint: disable=broad-except
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
    def allow(self, doc, obj, sub):  # pylint: disable=unused-argument
        try:
            shape = obj.Shape
        except Exception:  # pylint: disable=broad-except
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
    def allow(self, doc, obj, sub):  # pylint: disable=unused-argument
        PathLog.debug("obj: {} sub: {}".format(obj, sub))
        if not hasattr(obj, "Shape") and sub:
            return False
        shape = obj.Shape
        subobj = shape.getElement(sub)
        if subobj.ShapeType not in ["Edge", "Face"]:
            return False
        return drillableLib.isDrillable(shape, subobj, vector=None)


class FACEGate(
    PathBaseGate
):  # formerly PROFILEGate class using allow_ORIG method as allow()
    def allow(self, doc, obj, sub):  # pylint: disable=unused-argument
        profileable = False

        try:
            obj = obj.Shape
        except Exception:  # pylint: disable=broad-except
            return False

        if obj.ShapeType == "Compound":
            if sub and sub[0:4] == "Face":
                profileable = True

        elif obj.ShapeType == "Face":  # 3D Face, not flat, planar?
            profileable = True  # Was False

        elif obj.ShapeType == "Solid":
            if sub and sub[0:4] == "Face":
                profileable = True

        return profileable

    def allow_ORIG(self, doc, obj, sub):  # pylint: disable=unused-argument

        profileable = False
        try:
            obj = obj.Shape
        except Exception:  # pylint: disable=broad-except
            return False

        if obj.ShapeType == "Edge":
            profileable = False

        elif obj.ShapeType == "Compound":
            if sub and sub[0:4] == "Face":
                profileable = True

            if sub and sub[0:4] == "Edge":
                profileable = False

        elif obj.ShapeType == "Face":
            profileable = False

        elif obj.ShapeType == "Solid":
            if sub and sub[0:4] == "Face":
                profileable = True

            if sub and sub[0:4] == "Edge":
                profileable = False

        elif obj.ShapeType == "Wire":
            profileable = False

        return profileable


class PROFILEGate(PathBaseGate):
    def allow(self, doc, obj, sub):  # pylint: disable=unused-argument
        if sub and sub[0:4] == "Edge":
            return True

        try:
            obj = obj.Shape
        except Exception:  # pylint: disable=broad-except
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
    def allow(self, doc, obj, sub):  # pylint: disable=unused-argument

        pocketable = False
        try:
            obj = obj.Shape
        except Exception:  # pylint: disable=broad-except
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
    def allow(self, doc, obj, sub):  # pylint: disable=unused-argument

        adaptive = True
        try:
            obj = obj.Shape
        except Exception:  # pylint: disable=broad-except
            return False

        return adaptive


class CONTOURGate(PathBaseGate):
    def allow(self, doc, obj, sub):  # pylint: disable=unused-argument
        pass


class PROBEGate:
    def allow(self, doc, obj, sub):
        pass


class TURNGate(PathBaseGate):
    def allow(self, doc, obj, sub):  # pylint: disable=unused-argument
        PathLog.debug("obj: {} sub: {}".format(obj, sub))
        if hasattr(obj, "Shape") and sub:
            shape = obj.Shape
            subobj = shape.getElement(sub)
            return drillableLib.isDrillable(shape, subobj, vector=None)
        else:
            return False


class ALLGate(PathBaseGate):
    def allow(self, doc, obj, sub):  # pylint: disable=unused-argument
        if sub and sub[0:6] == "Vertex":
            return True
        if sub and sub[0:4] == "Edge":
            return True
        if sub and sub[0:4] == "Face":
            return True
        return False


def contourselect():
    FreeCADGui.Selection.addSelectionGate(CONTOURGate())
    if not PathPreferences.suppressSelectionModeWarning():
        FreeCAD.Console.PrintWarning("Contour Select Mode\n")


def eselect():
    FreeCADGui.Selection.addSelectionGate(EGate())
    if not PathPreferences.suppressSelectionModeWarning():
        FreeCAD.Console.PrintWarning("Edge Select Mode\n")


def drillselect():
    FreeCADGui.Selection.addSelectionGate(DRILLGate())
    if not PathPreferences.suppressSelectionModeWarning():
        FreeCAD.Console.PrintWarning("Drilling Select Mode\n")


def engraveselect():
    FreeCADGui.Selection.addSelectionGate(ENGRAVEGate())
    if not PathPreferences.suppressSelectionModeWarning():
        FreeCAD.Console.PrintWarning("Engraving Select Mode\n")


def fselect():
    FreeCADGui.Selection.addSelectionGate(FACEGate())  # Was PROFILEGate()
    if not PathPreferences.suppressSelectionModeWarning():
        FreeCAD.Console.PrintWarning("Profiling Select Mode\n")


def chamferselect():
    FreeCADGui.Selection.addSelectionGate(CHAMFERGate())
    if not PathPreferences.suppressSelectionModeWarning():
        FreeCAD.Console.PrintWarning("Deburr Select Mode\n")


def profileselect():
    FreeCADGui.Selection.addSelectionGate(PROFILEGate())
    if not PathPreferences.suppressSelectionModeWarning():
        FreeCAD.Console.PrintWarning("Profiling Select Mode\n")


def pocketselect():
    FreeCADGui.Selection.addSelectionGate(POCKETGate())
    if not PathPreferences.suppressSelectionModeWarning():
        FreeCAD.Console.PrintWarning("Pocketing Select Mode\n")


def adaptiveselect():
    FreeCADGui.Selection.addSelectionGate(ADAPTIVEGate())
    if not PathPreferences.suppressSelectionModeWarning():
        FreeCAD.Console.PrintWarning("Adaptive Select Mode\n")


def slotselect():
    FreeCADGui.Selection.addSelectionGate(ALLGate())
    if not PathPreferences.suppressSelectionModeWarning():
        FreeCAD.Console.PrintWarning("Slot Cutter Select Mode\n")


def surfaceselect():
    gate = False
    if MESHGate() or FACEGate():
        gate = True
    FreeCADGui.Selection.addSelectionGate(gate)
    if not PathPreferences.suppressSelectionModeWarning():
        FreeCAD.Console.PrintWarning("Surfacing Select Mode\n")


def vcarveselect():
    FreeCADGui.Selection.addSelectionGate(VCARVEGate())
    if not PathPreferences.suppressSelectionModeWarning():
        FreeCAD.Console.PrintWarning("Vcarve Select Mode\n")


def probeselect():
    FreeCADGui.Selection.addSelectionGate(PROBEGate())
    if not PathPreferences.suppressSelectionModeWarning():
        FreeCAD.Console.PrintWarning("Probe Select Mode\n")


def customselect():
    if not PathPreferences.suppressSelectionModeWarning():
        FreeCAD.Console.PrintWarning("Custom Select Mode\n")


def turnselect():
    FreeCADGui.Selection.addSelectionGate(TURNGate())
    if not PathPreferences.suppressSelectionModeWarning():
        FreeCAD.Console.PrintWarning("Turning Select Mode\n")


def select(op):
    opsel = {}
    opsel["Contour"] = contourselect  # (depreciated)
    opsel["Deburr"] = chamferselect
    opsel["Drilling"] = drillselect
    opsel["Engrave"] = engraveselect
    opsel["Helix"] = drillselect
    opsel["MillFace"] = pocketselect
    opsel["Pocket"] = pocketselect
    opsel["Pocket3D"] = pocketselect
    opsel["Pocket Shape"] = pocketselect
    opsel["Profile Edges"] = eselect  # (depreciated)
    opsel["Profile Faces"] = fselect  # (depreciated)
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
    if not PathPreferences.suppressSelectionModeWarning():
        FreeCAD.Console.PrintWarning("Free Select\n")
