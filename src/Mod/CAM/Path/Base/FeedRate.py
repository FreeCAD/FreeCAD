# SPDX-License-Identifier: LGPL-2.1-or-later
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

import math
import FreeCAD
import Path
import Path.Base.MachineState as PathMachineState
import Part
from Path.Geom import CmdMoveDrill

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


def rampFeedRate(toolController, command, start):
    """Resolve an entry feed from its actual XY travel and Z displacement.

    Arc travel is measured in the XY plane, not along the endpoint chord.
    Automatic feeds limit both velocity components; custom feeds are unchanged.
    """
    if getattr(toolController, "RampFeedMode", "Custom") != "Automatic":
        return toolController.RampFeed.Value

    end = Path.Geom.commandEndPoint(command, start)
    vertical = abs(end.z - start.z)
    horizontal = math.hypot(end.x - start.x, end.y - start.y)
    if command.Name in Path.Geom.CmdMoveArc:
        params = command.Parameters
        # Entry generators emit XY-plane arcs with relative I/J centers.
        i, j = params.get("I", 0.0), params.get("J", 0.0)
        a = math.atan2(-j, -i)
        b = math.atan2(end.y - start.y - j, end.x - start.x - i)
        sweep = (a - b if command.Name in Path.Geom.CmdMoveCW else b - a) % math.tau
        if Path.Geom.pointsCoincide(Path.Geom.xy(start), Path.Geom.xy(end)):
            sweep = math.tau
        horizontal = math.hypot(i, j) * sweep

    length = math.hypot(horizontal, vertical)
    if not length:
        return toolController.HorizFeed.Value
    limits = []
    if horizontal:
        limits.append(toolController.HorizFeed.Value * length / horizontal)
    if vertical:
        limits.append(toolController.VertFeed.Value * length / vertical)
    return min(limits)


def setRampFeedRate(commands, toolController):
    """Apply entry feeds, tracking modal coordinates through positioning moves."""
    machine = PathMachineState.MachineState()
    for command in commands:
        if command.Name in Path.Geom.CmdMoveMill:
            params = command.Parameters
            params["F"] = rampFeedRate(toolController, command, machine.getPosition())
            command.Parameters = params
        machine.addCommand(command)
    return commands


def setFeedRate(commandlist, ToolController):
    """Set the appropriate feed rate for a list of Path commands using the information from a Tool Controller

    Every motion command in the list will have a feed rate parameter added or overwritten based
    on the information stored in the tool controller. If a motion is a plunge (vertical) motion, the
    VertFeed value will be used, otherwise the HorizFeed value will be used instead.

    Tapping cycles are left untouched, as their F word is the thread pitch."""

    def _isVertical(currentposition, command):
        x = command.Parameters["X"] if "X" in command.Parameters else currentposition.x
        y = command.Parameters["Y"] if "Y" in command.Parameters else currentposition.y
        z = command.Parameters["Z"] if "Z" in command.Parameters else currentposition.z
        endpoint = FreeCAD.Vector(x, y, z)
        if Path.Geom.pointsCoincide(currentposition, endpoint):
            return True
        return Path.Geom.isVertical(Part.makeLine(currentposition, endpoint))

    machine = PathMachineState.MachineState()

    for command in commandlist:
        if command.Name not in Path.Geom.CmdMoveAll:
            continue

        # On tapping cycles the F word is the thread pitch, not a feed rate
        if command.Name in Path.Geom.CmdMoveTap:
            continue

        # Canned drill cycles (G73, G81, G82, G83, G85) are vertical cutting operations
        # The F word in a drill cycle specifies the feed rate for the vertical cutting component
        # The positioning move to XY is done at rapid speed (not controlled by F word)
        if command.Name in Path.Geom.CmdMoveDrill:
            rate = ToolController.VertFeed.Value
        elif _isVertical(machine.getPosition(), command):
            rate = (
                ToolController.VertRapid.Value
                if command.Name in Path.Geom.CmdMoveRapid
                else ToolController.VertFeed.Value
            )
        else:
            rate = (
                ToolController.HorizRapid.Value
                if command.Name in Path.Geom.CmdMoveRapid
                else ToolController.HorizFeed.Value
            )

        params = command.Parameters
        params["F"] = rate
        command.Parameters = params

        machine.addCommand(command)

    return commandlist
