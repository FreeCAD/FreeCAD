# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2021 sliptonic <shopinthewoods@gmail.com>               *
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
import PathScripts.PathLog as PathLog
import PathMachineState
import PathScripts.PathGeom as PathGeom
import Part

__title__ = "Feed Rate Helper Utility"
__author__ = "sliptonic (Brad Collette)"
__url__ = "https://www.freecadweb.org"
__doc__ = "Helper for adding Feed Rate to Path Commands"

"""
TODO:  This needs to be able to handle feedrates for axes other than X,Y,Z
"""

if True:
    PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())
    PathLog.trackModule(PathLog.thisModule())
else:
    PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())


def setFeedRate(commandlist, ToolController):
    def _isVertical(currentposition, command):
        x = (
            command.Parameters["X"]
            if "X" in command.Parameters.keys()
            else currentposition.x
        )
        y = (
            command.Parameters["Y"]
            if "Y" in command.Parameters.keys()
            else currentposition.y
        )
        z = (
            command.Parameters["Z"]
            if "Z" in command.Parameters.keys()
            else currentposition.z
        )
        endpoint = FreeCAD.Vector(x, y, z)
        if currentposition == endpoint:
            return True
        return PathGeom.isVertical(Part.makeLine(currentposition, endpoint))

    feedcommands = ["G01", "G1", "G2", "G3", "G02", "G03", "G81", "G82", "G83"]
    rapidcommands = ["G0", "G00"]

    machine = PathMachineState.MachineState()

    for command in commandlist:
        if command.Name not in feedcommands + rapidcommands:
            continue

        if _isVertical(FreeCAD.Vector(machine.X, machine.Y, machine.Z), command):
            rate = (
                ToolController.VertRapid.Value
                if command.Name in rapidcommands
                else ToolController.VertFeed.Value
            )
        else:
            rate = (
                ToolController.HorizRapid.Value
                if command.Name in rapidcommands
                else ToolController.HorizFeed.Value
            )

        params = command.Parameters
        params["F"] = rate
        command.Parameters = params

        machine.addCommand(command)

    return commandlist
