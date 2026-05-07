# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 sliptonic <shopinthewoods@gmail.com>
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
Provider implementations for the FeedsSpeeds resolver chain.

Phase 1 PoC ships ``ToolPresetProvider`` and ``ToolDefaultsProvider``.
Additional providers (``RuleRegistryProvider``,
``MaterialMachinabilityProvider``, ``FormulaProvider``) will be added in
Phase 2.
"""

import math
from typing import Optional, Tuple

from .types import (
    MaterialContext,
    OpContext,
    PartialResult,
    ToolContext,
    normalize_material_name,
)


def _score_preset(
    preset: dict,
    material: MaterialContext,
    op: OpContext,
) -> Optional[float]:
    """
    Returns a confidence score in (0, 1] for a preset against the request,
    or None if the preset does not match.

    Scoring:
      - material UUID exact match contributes 0.55, name fallback 0.45,
        material_hint=None contributes 0.15 (generic).
      - op_type exact match contributes 0.4, op_type hint=None
        contributes 0.15 (generic).
      - Mismatch in any specified hint = no match.
    """
    score = 0.0
    hint = preset.get("material_hint")
    if hint is None:
        score += 0.15
    else:
        hint_uuid = hint.get("uuid") if isinstance(hint, dict) else None
        hint_name = hint.get("name") if isinstance(hint, dict) else None
        if hint_uuid and material.uuid and hint_uuid == material.uuid:
            score += 0.55
        elif (
            hint_name
            and material.name
            and normalize_material_name(hint_name) == normalize_material_name(material.name)
        ):
            score += 0.45
        else:
            return None

    op_hint = preset.get("op_type_hint")
    if op_hint is None:
        score += 0.15
    elif op.op_type and op_hint == op.op_type:
        score += 0.40
    else:
        return None

    return min(score, 1.0)


def _derive_from_engineering(
    surface_speed: Optional[float],
    chipload: Optional[float],
    vert_feed_ratio: float,
    diameter_mm: float,
    flutes: Optional[int],
) -> Tuple[Optional[float], Optional[float], Optional[float], Tuple[str, ...]]:
    """
    From surface speed (m/min), chipload (mm/tooth), and tool geometry,
    derive spindle speed (rpm), horiz feed (mm/min) and vert feed (mm/min).

    Returns (spindle_speed, horiz_feed, vert_feed, warnings).
    Any field will be None if its inputs are missing.
    """
    warnings = []
    spindle = None
    horiz = None
    vert = None

    if surface_speed is not None and diameter_mm > 0:
        # rpm = (surface_speed in m/min * 1000) / (pi * d_mm)
        spindle = (surface_speed * 1000.0) / (math.pi * diameter_mm)

    if spindle is not None and chipload is not None:
        if flutes is None or flutes <= 0:
            warnings.append("Flute count unknown; using 1 for feed derivation.")
            f = 1
        else:
            f = flutes
        horiz = spindle * f * chipload
        vert = horiz * vert_feed_ratio

    return spindle, horiz, vert, tuple(warnings)


class ToolPresetProvider:
    """
    Reads presets stored on the ToolBit and produces a PartialResult from
    the best-matching preset.
    """

    name = "tool_preset"

    def suggest(
        self,
        tool: ToolContext,
        material: MaterialContext,
        op: OpContext,
    ) -> Optional[PartialResult]:
        if not tool.presets:
            return None

        best: Optional[Tuple[float, dict]] = None
        for preset in tool.presets:
            score = _score_preset(preset, material, op)
            if score is None:
                continue
            if best is None or score > best[0]:
                best = (score, preset)

        if best is None:
            return None

        score, preset = best
        surface_speed = preset.get("surface_speed")
        chipload = preset.get("chipload")
        vert_ratio = preset.get("vert_feed_ratio", 0.33)

        warnings = []
        spindle = horiz = vert = None

        if surface_speed is not None or chipload is not None:
            spindle, horiz, vert, derive_warnings = _derive_from_engineering(
                surface_speed, chipload, vert_ratio, tool.diameter, tool.flutes
            )
            warnings.extend(derive_warnings)

        if surface_speed is not None and chipload is None:
            warnings.append(
                "Preset provides surface_speed but no chipload; " "feed rates could not be derived."
            )
        elif surface_speed is None and chipload is None:
            warnings.append("Preset has no usable surface_speed or chipload values.")

        # Build provenance source URI
        mh = preset.get("material_hint")
        m_part = (
            (mh.get("uuid") or normalize_material_name(mh.get("name")) or "any")
            if isinstance(mh, dict)
            else "any"
        )
        op_part = preset.get("op_type_hint") or "any"
        source = f"preset:tool/{m_part}/{op_part}"

        return PartialResult(
            horiz_feed=horiz,
            vert_feed=vert,
            spindle_speed=spindle,
            chipload=chipload,
            surface_speed=surface_speed,
            source=source,
            confidence=score,
            warnings=tuple(warnings),
        )


class ToolDefaultsProvider:
    """
    Reads the legacy single-value ``ToolBit.Chipload`` property as a
    low-confidence fallback. Contributes only to ``chipload``; surface
    speed and feed/spindle derivation come from higher-priority providers
    (or, in Phase 2, from ``MaterialMachinabilityProvider`` looking up the
    workpiece's SurfaceSpeed by tool-material enum).

    Confidence is fixed and low (0.10) so any matching preset always wins.
    Returns ``None`` if no chipload default is set on the tool.
    """

    name = "tool_defaults"

    def suggest(
        self,
        tool: ToolContext,
        material: MaterialContext,
        op: OpContext,
    ) -> Optional[PartialResult]:
        if tool.chipload_default is None or tool.chipload_default <= 0:
            return None
        return PartialResult(
            chipload=tool.chipload_default,
            source="tool_defaults:chipload",
            confidence=0.10,
            warnings=(
                "Using tool's default Chipload property; no per-material/op preset matched.",
            ),
        )
