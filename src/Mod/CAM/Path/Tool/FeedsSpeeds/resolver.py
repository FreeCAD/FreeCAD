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

from .providers import ToolDefaultsProvider, ToolPresetProvider
from .types import (
    FeedSpeedResult,
    MaterialContext,
    OpContext,
    PartialResult,
    ToolContext,
)


def default_providers() -> Sequence:
    """
    Phase 1 PoC priority chain:
      1. ToolPresetProvider — best-match named preset (high confidence).
      2. ToolDefaultsProvider — single ``ToolBit.Chipload`` fallback
         (low confidence, contributes chipload only).

    Phase 2 will insert rule-registry, material-machinability, and
    formula providers above ToolDefaultsProvider.
    """
    return (ToolPresetProvider(), ToolDefaultsProvider())


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
    providers: Optional[Sequence] = None,
) -> FeedSpeedResult:
    """
    Walk the provider chain and produce a FeedSpeedResult.

    Returns a FeedSpeedResult with empty fields and source="" when no
    provider produced a result. Callers should treat that as "no
    suggestion available" and not write any TC values.
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

    return FeedSpeedResult(
        horiz_feed=accumulated.horiz_feed,
        vert_feed=accumulated.vert_feed,
        spindle_speed=accumulated.spindle_speed,
        chipload=accumulated.chipload,
        surface_speed=accumulated.surface_speed,
        source=accumulated.source,
        confidence=accumulated.confidence,
        warnings=tuple(accumulated.warnings),
    )
