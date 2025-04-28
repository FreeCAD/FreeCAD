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


def setFeedRate(commandlist_arg, ToolController):
    """Set the appropriate feed rate for a list of Path commands using the information from a Tool Controller

    Every motion command in the list will have a feed rate parameter added or overwritten based
    on the information stored in the tool controller. If a motion is a plunge (vertical) motion, the
    VertFeed value will be used, otherwise the HorizFeed value will be used instead."""

    def _isVertical(currentposition, command):
        x = command.Parameters["X"] if "X" in command.Parameters else currentposition.x
        y = command.Parameters["Y"] if "Y" in command.Parameters else currentposition.y
        return Path.Geom.isRoughly(x,currentposition.x) and Path.Geom.isRoughly(y,currentposition.y)

    def _isHorizontal(currentposition, command):
        z = command.Parameters["Z"] if "Z" in command.Parameters else currentposition.z
        return Path.Geom.isRoughly(z,currentposition.z) 


    machine = PathMachineState.MachineState()

    hFeed = ToolController.HorizFeed.Value
    vFeed = ToolController.VertFeed.Value
    #print("commandlist_arg[-1] = ", commandlist_arg[-1])
    #print("commandlist_arg[1].Parameters = ", commandlist_arg[1].Parameters.keys[1])
    #print("commandlist_arg[1].Parameters = ", commandlist_arg[1].Parameters.values[1])
    for i in range(len(commandlist_arg)):
        print("Name: ", commandlist_arg[i].Name)
        print("Parameters: ", commandlist_arg[i].Parameters)
        #print(" commandlist_arg[i]", commandlist_arg[i]) 
        #print(" commandlist_arg", commandlist_arg) 
        
    for command in commandlist_arg:
        if command.Name not in Path.Geom.CmdMoveAll:
            continue

        params = command.Parameters

        if _isVertical(machine.getPosition(), command): # also true for plunge helix
            rate = (
                ToolController.VertRapid.Value
                if command.Name in Path.Geom.CmdMoveRapid
                else vFeed
            )
        else:
            if "FR" in params: hFeed_adj = hFeed * params["FR"]
            else: hFeed_adj = hFeed

            if _isHorizontal(machine.getPosition(), command):
                rate = (
                    ToolController.HorizRapid.Value
                    if command.Name in Path.Geom.CmdMoveRapid
                    else hFeed_adj
                )
            elif (command.Name in ["G2", "G3"]):
                if ("DR" in params):
                    if params["DR"]==0:
                         rate = hFeed_adj
                    elif abs(params["DR"]) > 5: # arbitrary, why 5 ?
                         rate = hFeed_adj  # steep helix: use V feedrate
                    else:
                        descentAngle = math.atan(abs(params["DR"]))
                        rate = math.sqrt(vFeed * vFeed + hFeed_adj * hFeed_adj)  # vector sum
                        # ensure v,h feedrates not exceeded by "FR" rate
                        rate = min(rate, vFeed / math.cos(descentAngle))
                        rate = min(rate, hFeed_adj / math.sin(descentAngle))
    
                        # TODO modify pocket/contour arcs or ramps to use this code.
            else:
                rate = hFeed  # fallback  non v, non H, non arc == ramp??? ### should never

        params.pop("FR", None)
        params.pop("DR", None)

        params["F"] = rate
        command.Parameters = params

        machine.addCommand(command)

    return # commandlist_arg # return value unused !
