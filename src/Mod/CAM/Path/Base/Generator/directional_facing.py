# -*- coding: utf-8 -*-
# SPDX-License-Identifier: LGPL-2.1-or-later
# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2025 sliptonic sliptonic@freecad.org                    *
# *                                                                         *
# *   This file is part of FreeCAD.                                         *
# *                                                                         *
# *   FreeCAD is free software: you can redistribute it and/or modify it    *
# *   under the terms of the GNU Lesser General Public License as           *
# *   published by the Free Software Foundation, either version 2.1 of the  *
# *   License, or (at your option) any later version.                       *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful, but        *
# *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
# *   Lesser General Public License for more details.                       *
# *                                                                         *
# *   You should have received a copy of the GNU Lesser General Public      *
# *   License along with FreeCAD. If not, see                               *
# *   <https://www.gnu.org/licenses/>.                                      *
# *                                                                         *
# ***************************************************************************
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

    import math
    import Path
    import FreeCAD
    from . import facing_common

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
    stepover_distance = tool_diameter * (stepover_percent / 100.0)

    if stepover_percent >= 99.9 and step_positions:
        min_covered = min(step_positions) - tool_radius
        max_covered = max(step_positions) + tool_radius

        added = False
        if max_covered < max_t - 1e-4:
            step_positions.append(step_positions[-1] + stepover_distance)
            added = True
        if min_covered > min_t + 1e-4:
            step_positions.insert(0, step_positions[0] - stepover_distance)
            added = True
        if added:
            Path.Log.info("Directional: Added extra pass(es) for full coverage at high stepover")

    # Reverse = mirror positions around center (exactly like bidirectional) to preserve engagement offset on the starting side
    if reverse:
        center = (min_t + max_t) / 2.0
        step_positions = [2 * center - t for t in step_positions]

    Path.Log.debug(f"Directional (fixed): {len(step_positions)} passes")

    # Use full-length passes exactly like bidirectional (no slice_wire_segments)
    total_extension = (
        pass_extension
        + tool_radius
        + facing_common.calculate_engagement_offset(tool_diameter, stepover_percent)
    )

    start_s = min_s - total_extension
    end_s = max_s + total_extension

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
    # Fallback: if nothing kept due to numeric guards, emit a single mid-line pass across bbox
    if kept_segments == 0:
        t_candidates = []
        # mid, min, max t positions
        t_candidates.append(0.5 * (min_t + max_t))
        t_candidates.append(min_t)
        t_candidates.append(max_t)
        for t in t_candidates:
            intervals = facing_common.slice_wire_segments(polygon, primary_vec, step_vec, t, origin)
            if not intervals:
                continue
            s0, s1 = intervals[0]
            start_s = max(s0 - pass_extension, min_s - s_margin)
            end_s = min(s1 + pass_extension, max_s + s_margin)
            if end_s <= start_s:
                continue
            if milling_direction == "climb":
                p_start, p_end = start_s, end_s
            else:
                p_start, p_end = end_s, start_s
            if reverse:
                p_start, p_end = p_end, p_start
            sp = (
                FreeCAD.Vector(origin)
                .add(FreeCAD.Vector(primary_vec).multiply(p_start))
                .add(FreeCAD.Vector(step_vec).multiply(t))
            )
            ep = (
                FreeCAD.Vector(origin)
                .add(FreeCAD.Vector(primary_vec).multiply(p_end))
                .add(FreeCAD.Vector(step_vec).multiply(t))
            )
            sp.z = z
            ep.z = z
            # Minimal preamble
            if retract_height is not None:
                commands.append(Path.Command("G0", {"Z": retract_height}))
                commands.append(Path.Command("G0", {"X": sp.x, "Y": sp.y}))
                commands.append(Path.Command("G0", {"Z": z}))
            else:
                commands.append(Path.Command("G1", {"X": sp.x, "Y": sp.y, "Z": z}))
            commands.append(Path.Command("G1", {"X": ep.x, "Y": ep.y, "Z": z}))
            break
    return commands
