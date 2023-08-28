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


import Path
from enum import Enum

__title__ = "Toolchange Path Generator"
__author__ = "sliptonic (Brad Collette)"
__url__ = "https://www.freecad.org"
__doc__ = "Generates the rotation toolpath"


if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


class SpindleDirection(Enum):
    OFF = "OFF"
    CW = "M3"
    CCW = "M4"


def generate(
    toolnumber, toollabel, spindlespeed=0, spindledirection=SpindleDirection.OFF
):
    """
    Generates Gcode for a simple toolchange.

    """

    Path.Log.track(
        f"toolnumber:{toolnumber} toollabel: {toollabel} spindlespeed:{spindlespeed} spindledirection: {spindledirection}"
    )

    if spindledirection is not SpindleDirection.OFF and spindlespeed == 0:
        spindledirection = SpindleDirection.OFF
        # raise ValueError("Turning on spindle with zero speed is invalid")

    if spindlespeed < 0:
        raise ValueError("Spindle speed must be a positive value")

    commands = []

    commands.append(Path.Command(f"({toollabel})"))
    commands.append(Path.Command("M6", {"T": int(toolnumber)}))

    if spindledirection is SpindleDirection.OFF:
        return commands
    else:
        commands.append(Path.Command(spindledirection.value, {"S": spindlespeed}))

    Path.Log.track(commands)
    return commands


def generateSubstitute(newTC, oldTC=None):
    """
    The specific commands to emit, depend on the state of the machine.
    For example, the toolnumber may not change, only the spindle speed.
    This routine will generate a list of commands to substitute for a TC
    object to be handed to the postprocessor.
    It will contain only the commands needed
    """

    if oldTC is None:
        return newTC.Path.Commands

    if newTC.ToolNumber != oldTC.ToolNumber:  # Full toolchange
        return newTC.Path.Commands

    if (newTC.SpindleSpeed != oldTC.SpindleSpeed) or (
        newTC.SpindleDir != oldTC.SpindleDir
    ):
        if newTC.SpindleDir == "Forward":
            sd = SpindleDirection.CW
        elif newTC.SpindleDir == "Reverse":
            sd = SpindleDirection.CCW
        else:
            sd = SpindleDirection.OFF

        return [Path.Command(sd.value, {"S": newTC.SpindleSpeed})]

    return []
