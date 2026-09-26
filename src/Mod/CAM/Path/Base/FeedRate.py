# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2021 sliptonic <shopinthewoods@gmail.com>
# SPDX-FileNotice: Part of the FreeCAD project.

################################################################################
#                                                                              #
#   FreeCAD is free software: you can redistribute it and/or modify            #
#   it under the terms of the GNU Lesser General Public License as             #
#   published by the Free Software Foundation, either version 2.1              #
#   of the License, or (at your option) any later version.                     #
#                                                                              #
#   FreeCAD is distributed in the hope that it will be useful,                 #
#   but WITHOUT ANY WARRANTY; without even the implied warranty                #
#   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                    #
#   See the GNU Lesser General Public License for more details.                #
#                                                                              #
#   You should have received a copy of the GNU Lesser General Public           #
#   License along with FreeCAD. If not, see https://www.gnu.org/licenses       #
#                                                                              #
################################################################################

import FreeCAD
import Path
import Path.Base.MachineState as PathMachineState
import Part

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


def setFeedRate(commandlist, ToolController, horizFeed=0, vertFeed=0):
    """Set the appropriate feed rate for a list of Path commands using the information from a Tool Controller

    Every motion command in the list will have a feed rate parameter added or overwritten based
    on the information stored in the tool controller. If a motion is a plunge (vertical) motion, the
    VertFeed value will be used, otherwise the HorizFeed value will be used instead.

    Tapping cycles are left untouched, as their F word is the thread pitch."""

    HorizFeed = horizFeed.Value if horizFeed else ToolController.HorizFeed.Value
    VertFeed = vertFeed.Value if vertFeed else ToolController.VertFeed.Value
    HorizRapid = ToolController.HorizRapid.Value
    VertRapid = ToolController.VertRapid.Value

    def _isVertical(currentposition, command):
        x = command.Parameters.get("X", currentposition.x)
        y = command.Parameters.get("Y", currentposition.y)
        z = command.Parameters.get("Z", currentposition.z)
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
            rate = VertRapid if command.Name in Path.Geom.CmdMoveRapid else VertFeed
        else:
            rate = HorizRapid if command.Name in Path.Geom.CmdMoveRapid else HorizFeed

        params = command.Parameters
        params["F"] = rate
        command.Parameters = params

        machine.addCommand(command)

    return commandlist
