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
FeedSpeedResolver.

Pure entrypoint that walks a chain of providers and merges their PartialResult
contributions into a final FeedSpeedResult. The resolver is GUI-independent
and FreeCAD-Document-independent: callers build the input contexts from doc
objects via small adapter functions.
"""

from typing import List, Optional, Sequence

from .providers import (
    MachinabilityProvider,
    ToolDefaultsProvider,
    ToolPresetProvider,
    _derive_from_engineering,
)
from .types import (
    FeedSpeedResult,
    MachineContext,
    MaterialContext,
    OpContext,
    PartialResult,
    ToolContext,
)


def default_providers() -> Sequence:
    """
    Provider priority chain (first wins per field):
      1. ToolPresetProvider — best-match named preset (high confidence).
      2. MachinabilityProvider — stock-material Machinability model,
         surface_speed only (moderate confidence).
      3. ToolDefaultsProvider — single ``ToolBit.Chipload`` fallback
         (low confidence, contributes chipload only).
    """
    return (ToolPresetProvider(), MachinabilityProvider(), ToolDefaultsProvider())


def _merge(base: PartialResult, addition: PartialResult) -> PartialResult:
    """
    Fill any None field in ``base`` from ``addition``. Higher-priority
    providers come first; later providers only contribute fields that
    haven't been set yet.
    """
    return PartialResult(
        horiz_feed=base.horiz_feed if base.horiz_feed is not None else addition.horiz_feed,
        vert_feed=base.vert_feed if base.vert_feed is not None else addition.vert_feed,
        spindle_speed=(
            base.spindle_speed if base.spindle_speed is not None else addition.spindle_speed
        ),
        chipload=base.chipload if base.chipload is not None else addition.chipload,
        surface_speed=(
            base.surface_speed if base.surface_speed is not None else addition.surface_speed
        ),
        source=base.source if base.source is not None else addition.source,
        confidence=base.confidence if base.confidence is not None else addition.confidence,
        warnings=tuple(base.warnings) + tuple(addition.warnings),
    )


def resolve(
    tool: ToolContext,
    material: MaterialContext,
    op: OpContext,
    machine: Optional[MachineContext] = None,
    providers: Optional[Sequence] = None,
) -> FeedSpeedResult:
    """
    Walk the provider chain and produce a FeedSpeedResult.

    Returns a FeedSpeedResult with empty fields and source="" when no
    provider produced a result. Callers should treat that as "no
    suggestion available" and not write any TC values.

    When ``machine`` is provided and the derived spindle falls outside
    ``[min_rpm, max_rpm]``, the spindle is clamped, horiz/vert feeds are
    scaled by the same ratio (chipload preserved), and a warning is
    emitted. ``surface_speed`` is left at the recommended value so the
    user can see the discrepancy between intended cutting speed and the
    machine-limited reality.
    """
    chain = providers if providers is not None else default_providers()

    accumulated: Optional[PartialResult] = None
    for provider in chain:
        partial = provider.suggest(tool, material, op)
        if partial is None:
            continue
        accumulated = partial if accumulated is None else _merge(accumulated, partial)

    if accumulated is None:
        return FeedSpeedResult(source="", confidence=0.0, warnings=())

    # Finalization: when providers contributed engineering values
    # (surface_speed, chipload) but no raw rpm/feed — typically the
    # MachinabilityProvider + ToolDefaultsProvider combination — derive
    # them once from tool geometry. Uses a default vert_feed_ratio of
    # 0.33; preset-driven runs already filled these fields using the
    # preset's own ratio and won't be touched here.
    spindle = accumulated.spindle_speed
    horiz = accumulated.horiz_feed
    vert = accumulated.vert_feed
    warnings = list(accumulated.warnings)
    if (spindle is None or horiz is None) and (
        accumulated.surface_speed is not None or accumulated.chipload is not None
    ):
        d_spindle, d_horiz, d_vert, d_warnings = _derive_from_engineering(
            accumulated.surface_speed,
            accumulated.chipload,
            0.33,
            tool.diameter,
            tool.flutes,
        )
        if spindle is None:
            spindle = d_spindle
        if horiz is None:
            horiz = d_horiz
        if vert is None:
            vert = d_vert
        for w in d_warnings:
            if w not in warnings:
                warnings.append(w)

    # Machine clamping: spindle out of [min_rpm, max_rpm] gets pulled to
    # the limit; feeds scale by the same ratio so chipload per tooth stays
    # constant. surface_speed is left at the recommended value.
    if spindle is not None and spindle > 0 and machine is not None:
        clamped = None
        if machine.max_rpm > 0 and spindle > machine.max_rpm:
            clamped = machine.max_rpm
            warnings.append(
                f"Spindle clamped from {spindle:.0f} to machine max {machine.max_rpm:.0f} rpm."
            )
        elif machine.min_rpm > 0 and spindle < machine.min_rpm:
            clamped = machine.min_rpm
            warnings.append(
                f"Spindle clamped from {spindle:.0f} up to machine min {machine.min_rpm:.0f} rpm."
            )
        if clamped is not None:
            ratio = clamped / spindle
            spindle = clamped
            if horiz is not None:
                horiz *= ratio
            if vert is not None:
                vert *= ratio

    return FeedSpeedResult(
        horiz_feed=horiz,
        vert_feed=vert,
        spindle_speed=spindle,
        chipload=accumulated.chipload,
        surface_speed=accumulated.surface_speed,
        source=accumulated.source,
        confidence=accumulated.confidence,
        warnings=tuple(warnings),
    )
