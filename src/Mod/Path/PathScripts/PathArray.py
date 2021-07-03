# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2015 Yorik van Havre <yorik@uncreated.net>              *
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

import FreeCAD
import FreeCADGui
import Path
import PathScripts
from PathScripts import PathLog
from PySide import QtCore
import math
import random

__doc__ = """Path Array object and FreeCAD command"""

# Qt translation handling
def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)

class ObjectArray:

    def __init__(self, obj):
        obj.addProperty("App::PropertyLinkList", "Base",
                        "Path", QtCore.QT_TRANSLATE_NOOP("App::Property","The path(s) to array"))
        obj.addProperty("App::PropertyEnumeration", "Type",
                        "Path", QtCore.QT_TRANSLATE_NOOP("App::Property", "Pattern method"))
        obj.addProperty("App::PropertyVectorDistance", "Offset",
                        "Path", QtCore.QT_TRANSLATE_NOOP("App::Property","The spacing between the array copies in Linear pattern"))
        obj.addProperty("App::PropertyInteger", "CopiesX",
                        "Path", QtCore.QT_TRANSLATE_NOOP("App::Property","The number of copies in X direction in Linear pattern"))
        obj.addProperty("App::PropertyInteger", "CopiesY",
                        "Path", QtCore.QT_TRANSLATE_NOOP("App::Property","The number of copies in Y direction in Linear pattern"))
        obj.addProperty("App::PropertyAngle", "Angle",
                        "Path", QtCore.QT_TRANSLATE_NOOP("App::Property","Total angle in Polar pattern"))
        obj.addProperty("App::PropertyInteger", "Copies",
                        "Path", QtCore.QT_TRANSLATE_NOOP("App::Property","The number of copies in Linear 1D and Polar pattern"))
        obj.addProperty("App::PropertyVector", "Centre",
                        "Path", QtCore.QT_TRANSLATE_NOOP("App::Property","The centre of rotation in Polar pattern"))
        obj.addProperty("App::PropertyBool", "SwapDirection",
                        "Path", QtCore.QT_TRANSLATE_NOOP("App::Property","Make copies in X direction before Y in Linear 2D pattern"))
        obj.addProperty("App::PropertyInteger", "JitterPercent",
                        "Path", QtCore.QT_TRANSLATE_NOOP("App::Property","Percent of copies to randomly offset"))
        obj.addProperty("App::PropertyVectorDistance", "JitterMagnitude",
                        "Path", QtCore.QT_TRANSLATE_NOOP("App::Property","Maximum random offset of copies"))
        obj.addProperty("App::PropertyLink", "ToolController",
                        "Path", QtCore.QT_TRANSLATE_NOOP("App::Property", "The tool controller that will be used to calculate the path"))

        obj.Type = ['Linear1D', 'Linear2D', 'Polar']

        self.setEditorProperties(obj)
        obj.Proxy = self

    def __getstate__(self):
        return None

    def __setstate__(self, state):
        return None

    def setEditorProperties(self, obj):
        obj.setEditorMode('JitterPercent', 0)
        obj.setEditorMode('JitterMagnitude', 0)
        obj.setEditorMode('ToolController', 2)
        if obj.Type == 'Linear2D':
            obj.setEditorMode('Angle', 2)
            obj.setEditorMode('Copies', 2)
            obj.setEditorMode('Centre', 2)

            obj.setEditorMode('CopiesX', 0)
            obj.setEditorMode('CopiesY', 0)
            obj.setEditorMode('Offset', 0)
            obj.setEditorMode('SwapDirection', False)
        elif obj.Type == 'Polar':
            obj.setEditorMode('Angle', 0)
            obj.setEditorMode('Copies', 0)
            obj.setEditorMode('Centre', 0)

            obj.setEditorMode('CopiesX', 2)
            obj.setEditorMode('CopiesY', 2)
            obj.setEditorMode('Offset', 2)
        elif obj.Type == 'Linear1D':
            obj.setEditorMode('Angle', 2)
            obj.setEditorMode('Copies', 0)
            obj.setEditorMode('Centre', 2)

            obj.setEditorMode('CopiesX', 2)
            obj.setEditorMode('CopiesY', 2)
            obj.setEditorMode('Offset', 0)

    def onChanged(self, obj, prop):
        if prop == "Type":
            self.setEditorProperties(obj)

    def rotatePath(self, path, angle, centre):
        '''
            Rotates Path around given centre vector
            Only X and Y is considered
        '''
        CmdMoveRapid    = ['G0', 'G00']
        CmdMoveStraight = ['G1', 'G01']
        CmdMoveCW       = ['G2', 'G02']
        CmdMoveCCW      = ['G3', 'G03']
        CmdDrill        = ['G81', 'G82', 'G83']
        CmdMoveArc      = CmdMoveCW + CmdMoveCCW
        CmdMove         = CmdMoveStraight + CmdMoveArc

        commands = []
        ang = angle / 180 * math.pi
        currX = 0
        currY = 0
        for cmd in path.Commands:
            if (cmd.Name in CmdMoveRapid) or (cmd.Name in CmdMove) or (cmd.Name in CmdDrill):
                params = cmd.Parameters
                x = params.get("X")
                if x is None:
                    x = currX
                currX = x
                y = params.get("Y")
                if y is None:
                    y = currY
                currY = y

                # "move" the centre to origin
                x = x - centre.x
                y = y - centre.y

                # rotation around origin:
                nx = x * math.cos(ang) - y * math.sin(ang)
                ny = y * math.cos(ang) + x * math.sin(ang)

                # "move" the centre back and update
                params.update({'X': nx + centre.x, 'Y': ny + centre.y})

                # Arcs need to have the I and J params rotated as well
                if cmd.Name in CmdMoveArc:
                    i = params.get("I")
                    if i is None:
                        i = 0
                    j = params.get("J")
                    if j is None:
                        j = 0

                    ni = i * math.cos(ang) - j * math.sin(ang)
                    nj = j * math.cos(ang) + i * math.sin(ang)
                    params.update({'I': ni, 'J': nj})

                cmd.Parameters = params
            commands.append(cmd)
        newPath = Path.Path(commands)

        return newPath

    def calculateJitter(self, obj, pos):
        if random.randint(0,100) < obj.JitterPercent:
            pos.x = pos.x + random.uniform(-obj.JitterMagnitude.x, obj.JitterMagnitude.y)
            pos.y = pos.y + random.uniform(-obj.JitterMagnitude.y, obj.JitterMagnitude.y)
            pos.z = pos.z + random.uniform(-obj.JitterMagnitude.z, obj.JitterMagnitude.z)
        return pos


    def execute(self, obj):

        # backwards compatibility for PathArrays created before support for multiple bases
        if isinstance(obj.Base, list):
            base = obj.Base
        else:
            base = [obj.Base]

        if len(base)==0:
            return

        obj.ToolController = base[0].ToolController
        for b in base:
            if not b.isDerivedFrom("Path::Feature"):
                return
            if not b.Path:
                return
            if not b.ToolController:
                return
            if b.ToolController != obj.ToolController:
                # this may be important if Job output is split by tool controller
                PathLog.warning(QtCore.QT_TRANSLATE_NOOP("App::Property",'Arrays of paths having different tool controllers are handled according to the tool controller of the first path.'))

        # build copies
        output = ""
        random.seed(obj.Name)
        if obj.Type == 'Linear1D':
            for i in range(obj.Copies):
                pos = FreeCAD.Vector(obj.Offset.x * (i + 1), obj.Offset.y * (i + 1), 0)
                pos = self.calculateJitter(obj, pos)

                for b in base:
                    pl = FreeCAD.Placement()
                    pl.move(pos)
                    np = Path.Path([cm.transform(pl)
                                    for cm in b.Path.Commands])
                    output += np.toGCode()

        elif obj.Type == 'Linear2D':
            if obj.SwapDirection:
                for i in range(obj.CopiesY + 1):
                    for j in range(obj.CopiesX + 1):
                        if (i % 2) == 0:
                            pos = FreeCAD.Vector(obj.Offset.x * j, obj.Offset.y * i, 0)
                        else:
                            pos = FreeCAD.Vector(obj.Offset.x * (obj.CopiesX - j), obj.Offset.y * i, 0)
                        pos = self.calculateJitter(obj, pos)

                        for b in base:
                            pl = FreeCAD.Placement()
                            # do not process the index 0,0. It will be processed by the base Paths themselves
                            if not (i == 0 and j == 0):
                                pl.move(pos)
                                np = Path.Path([cm.transform(pl) for cm in b.Path.Commands])
                                output += np.toGCode()
            else:
                for i in range(obj.CopiesX + 1):
                    for j in range(obj.CopiesY + 1):
                        if (i % 2) == 0:
                            pos = FreeCAD.Vector(obj.Offset.x * i, obj.Offset.y * j, 0)
                        else:
                            pos = FreeCAD.Vector(obj.Offset.x * i, obj.Offset.y * (obj.CopiesY - j), 0)
                        pos = self.calculateJitter(obj, pos)

                        for b in base:
                            pl = FreeCAD.Placement()
                            # do not process the index 0,0. It will be processed by the base Paths themselves
                            if not (i == 0 and j == 0):
                                pl.move(pos)
                                np = Path.Path([cm.transform(pl) for cm in b.Path.Commands])
                                output += np.toGCode()


        else:
            for i in range(obj.Copies):
                for b in base:
                    ang = 360
                    if obj.Copies > 0:
                        ang = obj.Angle / obj.Copies * (1 + i)
                    np = self.rotatePath(b.Path.Commands, ang, obj.Centre)
                    output += np.toGCode()

        # print output
        path = Path.Path(output)
        obj.Path = path


class ViewProviderArray:

    def __init__(self, vobj):
        self.Object = vobj.Object
        vobj.Proxy = self

    def attach(self, vobj):
        self.Object = vobj.Object
        return

    def __getstate__(self):
        return None

    def __setstate__(self, state):
        return None

    def claimChildren(self):
        if hasattr(self, "Object"):
            if hasattr(self.Object, "Base"):
                if self.Object.Base:
                    return self.Object.Base
        return []


class CommandPathArray:

    def GetResources(self):
        return {'Pixmap': 'Path_Array',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Path_Array", "Array"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Path_Array", "Creates an array from selected path(s)")}

    def IsActive(self):
        if bool(FreeCADGui.Selection.getSelection()) is False:
            return False
        try:
            obj = FreeCADGui.Selection.getSelectionEx()[0].Object
            return isinstance(obj.Proxy, PathScripts.PathOp.ObjectOp)
        except(IndexError, AttributeError):
            return False

    def Activated(self):

        # check that the selection contains exactly what we want
        selection = FreeCADGui.Selection.getSelection()

        for sel in selection:
            if not(sel.isDerivedFrom("Path::Feature")):
                FreeCAD.Console.PrintError(
                    translate("Path_Array", "Arrays can be created only from Path operations.")+"\n")
                return

        # if everything is ok, execute and register the transaction in the
        # undo/redo stack
        FreeCAD.ActiveDocument.openTransaction("Create Array")
        FreeCADGui.addModule("PathScripts.PathArray")
        FreeCADGui.addModule("PathScripts.PathUtils")

        FreeCADGui.doCommand('obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython","Array")')

        FreeCADGui.doCommand('PathScripts.PathArray.ObjectArray(obj)')

        baseString = "[%s]" % ','.join(["FreeCAD.ActiveDocument.%s" % sel.Name for sel in selection])
        FreeCADGui.doCommand('obj.Base = %s' % baseString)

        FreeCADGui.doCommand('obj.ViewObject.Proxy = 0')
        FreeCADGui.doCommand('PathScripts.PathUtils.addToJob(obj)')
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()


if FreeCAD.GuiUp:
    # register the FreeCAD command
    FreeCADGui.addCommand('Path_Array', CommandPathArray())
