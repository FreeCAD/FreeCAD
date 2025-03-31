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
import Path
import Path.Base.MachineState as PathMachineState
import Part
import math

__title__ = "Feed Rate Helper Utility"
__author__ = "sliptonic (Brad Collette)"
__url__ = "https://www.freecad.org"
__doc__ = "Helper for adding Feed Rate to Path Commands"

"""
TODO:  This needs to be able to handle feedrates for axes other than X,Y,Z
"""

if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


def setFeedRate(commandlist, ToolController):
    """Set the appropriate feed rate for a list of Path commands using the information from a Tool Controller

    Every motion command in the list will have a feed rate parameter added or overwritten based
    on the information stored in the tool controller. If a motion is a plunge (vertical) motion, the
    VertFeed value will be used, otherwise the HorizFeed value will be used instead."""

    def _isVertical(currentposition, command):
        x = command.Parameters["X"] if "X" in command.Parameters else currentposition.x
        y = command.Parameters["Y"] if "Y" in command.Parameters else currentposition.y
        z = command.Parameters["Z"] if "Z" in command.Parameters else currentposition.z
        endpoint = FreeCAD.Vector(x, y, z)
        if Path.Geom.pointsCoincide(currentposition, endpoint):
            return True
        return Path.Geom.isVertical(Part.makeLine(currentposition, endpoint))

    def _isHorizontal(currentposition, command):
        x = command.Parameters["X"] if "X" in command.Parameters else currentposition.x
        y = command.Parameters["Y"] if "Y" in command.Parameters else currentposition.y
        z = command.Parameters["Z"] if "Z" in command.Parameters else currentposition.z
        endpoint = FreeCAD.Vector(x, y, z)
        if Path.Geom.pointsCoincide(currentposition, endpoint):
            return True
        return Path.Geom.isHorizontal(Part.makeLine(currentposition, endpoint))

    machine = PathMachineState.MachineState()


    hFeed = ToolController.HorizFeed.Value
    vFeed = ToolController.VertFeed.Value

    for command in commandlist:
        if command.Name not in Path.Geom.CmdMoveAll:
            continue

        params = command.Parameters
        if ("F" in params) and (command.Name in ["G2","G3"]): 
            # adj. spindle feedrate to ensure correct chip load on cutting edge, if "F" defined
            hFeed_adj = hFeed* params["F"]  
            rate = math.sqrt(vFeed * vFeed + hFeed_adj * hFeed_adj ) # vector sum
            # TODO combined V,H feedrates for 3D-ops, ramp, pocket/contour radius ?
        else:
            if _isVertical(machine.getPosition(), command):
                rate = (
                    ToolController.VertRapid.Value
                    if command.Name in Path.Geom.CmdMoveRapid
                    else vFeed
                )
            else:
                if _isHorizontal(machine.getPosition(), command) :
                    rate = (
                        ToolController.HorizRapid.Value
                        if command.Name in Path.Geom.CmdMoveRapid
                        else hFeed 
                    )
                else : 
                    rate = hFeed  # fallback

        params["F"] = rate
        command.Parameters = params

        machine.addCommand(command)

    return commandlist
