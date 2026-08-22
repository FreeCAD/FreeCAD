# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2025 sliptonic sliptonic@freecad.org
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

"""
Directional (unidirectional) facing toolpath generator.

This module implements the unidirectional clearing pattern that cuts in the same
direction for every pass, providing consistent surface finish.

Feed moves (cutting) are aligned with the angle_degrees argument direction.

At the end of each cutting pass, the cutter retracts to safe height and moves laterally to
the start position of the next pass.

This strategy always maintains either climb or conventional milling direction.
"""

import FreeCAD
import Path
import math
from . import facing_common

if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


def directional(
    polygon,
    tool_diameter,
    stepover_percent,
    pass_extension=None,
    retract_height=None,
    milling_direction="climb",
    reverse=False,
    angle_degrees=None,
):

    if pass_extension is None:
        pass_extension = tool_diameter * 0.5

    theta = float(angle_degrees) if angle_degrees is not None else 0.0
    primary_vec, step_vec = facing_common.unit_vectors_from_angle(theta)
    primary_vec = FreeCAD.Vector(primary_vec).normalize()
    step_vec = FreeCAD.Vector(step_vec).normalize()

    origin = polygon.BoundBox.Center
    z = polygon.BoundBox.ZMin

    min_s, max_s = facing_common.project_bounds(polygon, primary_vec, origin)
    min_t, max_t = facing_common.project_bounds(polygon, step_vec, origin)

    if not all(math.isfinite(x) for x in [min_s, max_s, min_t, max_t]):
        Path.Log.error("Directional: non-finite projection bounds; aborting")
        return []

    step_positions = facing_common.generate_t_values(
        polygon, step_vec, tool_diameter, stepover_percent, origin
    )

    tool_radius = tool_diameter / 2.0

    # Reverse = mirror positions around center (exactly like bidirectional) to preserve engagement offset on the starting side
    if reverse:
        center = (min_t + max_t) / 2.0
        step_positions = [2 * center - t for t in step_positions]

    Path.Log.debug(f"Directional (fixed): {len(step_positions)} passes")

    # Use full-length passes exactly like bidirectional (no slice_wire_segments)
    total_extension = pass_extension + tool_radius
    start_s = min_s - total_extension
    end_s = max_s + total_extension

    s_mid = (min_s + max_s) / 2.0
    if start_s > s_mid or end_s < s_mid:
        step_positions = []

    commands = []
    kept_segments = 0

    for t in step_positions:
        # Cutting direction – reverse flips it to maintain climb/conventional preference
        if milling_direction == "climb":
            if reverse:
                p_start, p_end = start_s, end_s
            else:
                p_start, p_end = end_s, start_s
        else:  # conventional
            if reverse:
                p_start, p_end = end_s, start_s
            else:
                p_start, p_end = start_s, end_s

        start_point = origin + primary_vec * p_start + step_vec * t
        end_point = origin + primary_vec * p_end + step_vec * t
        start_point.z = z
        end_point.z = z

        if commands:  # not first pass
            if retract_height is not None:
                commands.append(Path.Command("G0", {"Z": retract_height}))
                commands.append(Path.Command("G0", {"X": start_point.x, "Y": start_point.y}))
                commands.append(Path.Command("G0", {"Z": z}))
            else:
                commands.append(
                    Path.Command("G0", {"X": start_point.x, "Y": start_point.y, "Z": z})
                )
        else:
            commands.append(Path.Command("G0", {"X": start_point.x, "Y": start_point.y, "Z": z}))

        commands.append(Path.Command("G1", {"X": end_point.x, "Y": end_point.y, "Z": z}))
        kept_segments += 1

    Path.Log.debug(f"Directional: generated {kept_segments} segments")

    return commands
