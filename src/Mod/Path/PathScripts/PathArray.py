# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
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
from PySide import QtCore
import math

"""Path Array object and FreeCAD command"""

# Qt translation handling
def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)

class ObjectArray:

    def __init__(self, obj):
        obj.addProperty("App::PropertyLink", "Base",
                        "Path", "The path to array")
        obj.addProperty("App::PropertyEnumeration", "Type",
                        "Path", QtCore.QT_TRANSLATE_NOOP("App::Property", "Pattern method"))
        obj.addProperty("App::PropertyVectorDistance", "Offset",
                        "Path", "The spacing between the array copies in Linear pattern")
        obj.addProperty("App::PropertyInteger", "CopiesX",
                        "Path", "The number of copies in X direction in Linear pattern")
        obj.addProperty("App::PropertyInteger", "CopiesY",
                        "Path", "The number of copies in Y direction in Linear pattern")
        obj.addProperty("App::PropertyAngle", "Angle",
                        "Path", "Total angle in Polar pattern")
        obj.addProperty("App::PropertyInteger", "Copies",
                        "Path", "The number of copies in Linear 1D and Polar pattern")
        obj.addProperty("App::PropertyVector", "Centre",
                        "Path", "The centre of rotation in Polar pattern")
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
        if obj.Type == 'Linear2D':
            obj.setEditorMode('Angle', 2)
            obj.setEditorMode('Copies', 2)
            obj.setEditorMode('Centre', 2)

            obj.setEditorMode('CopiesX', 0)
            obj.setEditorMode('CopiesY', 0)
            obj.setEditorMode('Offset', 0)
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

    def execute(self, obj):
        if obj.Base:
            if not obj.Base.isDerivedFrom("Path::Feature"):
                return
            if not obj.Base.Path:
                return
            if not obj.Base.ToolController:
                return

            obj.ToolController = obj.Base.ToolController

            # build copies
            basepath = obj.Base.Path
            output = ""
            if obj.Type == 'Linear1D':
                for i in range(obj.Copies):
                    pl = FreeCAD.Placement()
                    pos = FreeCAD.Vector(obj.Offset.x * (i + 1), obj.Offset.y * (i + 1), 0)
                    pl.move(pos)
                    np = Path.Path([cm.transform(pl)
                                    for cm in basepath.Commands])
                    output += np.toGCode()

            elif obj.Type == 'Linear2D':
                for i in range(obj.CopiesX + 1):
                    for j in range(obj.CopiesY + 1):
                        pl = FreeCAD.Placement()
                        # do not process the index 0,0. It will be processed at basepath
                        if not (i == 0 and j == 0):
                            if (i % 2) == 0:
                                pos = FreeCAD.Vector(obj.Offset.x * i, obj.Offset.y * j, 0)
                            else:
                                pos = FreeCAD.Vector(obj.Offset.x * i, obj.Offset.y * (obj.CopiesY - j), 0)

                            pl.move(pos)
                            np = Path.Path([cm.transform(pl)
                                            for cm in basepath.Commands])
                            output += np.toGCode()

            else:
                for i in range(obj.Copies):

                    ang = 360
                    if obj.Copies > 0:
                        ang = obj.Angle / obj.Copies * (1 + i)

                    np = self.rotatePath(basepath, ang, obj.Centre)
                    output += np.toGCode()
            # print output
            path = Path.Path(output)
            obj.Path = path


class ViewProviderArray:

    def __init__(self, vobj):
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
        return {'Pixmap': 'Path-Array',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Path_Array", "Array"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Path_Array", "Creates an array from a selected path")}

    def IsActive(self):
        if bool(FreeCADGui.Selection.getSelection()) is False:
            return False
        try:
            obj = FreeCADGui.Selection.getSelectionEx()[0].Object
            return isinstance(obj.Proxy, PathScripts.PathOp.ObjectOp)
        except:
            return False

    def Activated(self):

        # check that the selection contains exactly what we want
        selection = FreeCADGui.Selection.getSelection()
        if len(selection) != 1:
            FreeCAD.Console.PrintError(
                translate("Path_Array", "Please select exactly one path object")+"\n")
            return
        if not(selection[0].isDerivedFrom("Path::Feature")):
            FreeCAD.Console.PrintError(
                translate("Path_Array", "Please select exactly one path object")+"\n")
            return

        # if everything is ok, execute and register the transaction in the
        # undo/redo stack
        FreeCAD.ActiveDocument.openTransaction("Create Array")
        FreeCADGui.addModule("PathScripts.PathArray")
        FreeCADGui.addModule("PathScripts.PathUtils")
        FreeCADGui.doCommand(
            'obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython","Array")')
        FreeCADGui.doCommand('PathScripts.PathArray.ObjectArray(obj)')
        FreeCADGui.doCommand(
            'obj.Base = (FreeCAD.ActiveDocument.' + selection[0].Name + ')')
        # FreeCADGui.doCommand('PathScripts.PathArray.ViewProviderArray(obj.ViewObject)')
        FreeCADGui.doCommand('obj.ViewObject.Proxy = 0')
        FreeCADGui.doCommand('PathScripts.PathUtils.addToJob(obj)')
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()


if FreeCAD.GuiUp:
    # register the FreeCAD command
    FreeCADGui.addCommand('Path_Array', CommandPathArray())
