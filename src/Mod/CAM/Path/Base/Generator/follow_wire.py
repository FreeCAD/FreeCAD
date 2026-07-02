# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 Billy Huddleston <billy@ivdc.com>
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

__title__ = "CAM Wire Follow Generator"
__author__ = "Connor (Billy Huddleston <billy@ivdc.com>)"
__url__ = "https://www.freecad.org"
__doc__ = "Generate G-code for a single pass along any Part.Wire."

import Path
import Path.Geom as PathGeom


def generate(wire, retract_z, horiz_feed, vert_feed, arc_chord):
    """Generate G-code commands for a single pass along a wire.

    This generator is intentionally simple — it knows nothing about step-down
    passes, depth scaling, or operation strategy.  The caller is responsible
    for supplying a wire that already represents exactly one pass at the
    correct depth.

    Args:
        wire       - Part.Wire, the exact path to follow for this pass.
                     Point 0 should be the entry (shallowest) end.
        retract_z  - float, Z height for the rapid retract after the pass.
        horiz_feed - float, feed rate for XY+Z cutting moves.
        vert_feed  - float, feed rate for the initial Z plunge.
        arc_chord  - float, max chord length (mm) used to discretize curved
                     edges into G1 line segments.

    Returns a list of Path.Command objects:
        G0 X Y        — rapid to entry XY
        G1 Z F        — plunge to entry depth
        G1 X Y Z F    — cutting moves along the wire
        G0 Z          — retract
    """
    pts = PathGeom.edgesToPoints(wire.Edges, arc_chord)
    if not pts:
        return []

    commands = []
    first = pts[0]
    commands.append(Path.Command("G0", {"X": first.x, "Y": first.y}))
    commands.append(Path.Command("G1", {"Z": first.z, "F": vert_feed}))
    for wp in pts[1:]:
        commands.append(Path.Command("G1", {"X": wp.x, "Y": wp.y, "Z": wp.z, "F": horiz_feed}))
    commands.append(Path.Command("G0", {"Z": retract_z}))

    return commands
