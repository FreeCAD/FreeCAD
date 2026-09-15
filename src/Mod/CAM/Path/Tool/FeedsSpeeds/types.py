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
Pure data types for the FeedsSpeeds resolver.

This module is intentionally free of FreeCAD imports so the resolver and its
tests can be exercised outside a running FreeCAD process.
"""

from dataclasses import dataclass
from typing import Optional, Tuple

# Op-type vocabulary. Additive-only: adding values is free, renaming or
# removing values is a migration. ``OpContext.op_type=None`` (and a
# preset's ``op_type_hint=None``) means "generic, matches any op."
OP_TYPES = (
    "profile",
    "pocket",
    "slot",
    "drill",
    "adaptive",
    "surface_finish",
)


@dataclass(frozen=True)
class ToolContext:
    diameter: float
    flutes: Optional[int]
    presets: Tuple[dict, ...]
    shape_id: str
    # The ToolBit's ``Material`` enumeration value, typically "HSS" or
    # "Carbide". Used by ``MachinabilityProvider`` to pick the correct
    # surface-speed branch from the stock material's Machinability model.
    # None when the toolbit has no Material property set.
    tool_material: Optional[str] = None
    # The single "default chipload" value stored directly on the ToolBit
    # (App::PropertyLength "Chipload"). Read as a low-confidence fallback
    # by ToolDefaultsProvider when no preset matches. mm/tooth, or None
    # if the tool has no Chipload property or it is zero.
    chipload_default: Optional[float] = None


@dataclass(frozen=True)
class MaterialContext:
    uuid: Optional[str] = None
    name: Optional[str] = None
    # Machinability surface speeds (m/min) pre-extracted from the stock
    # material's PhysicalProperties by the adapter. Either may be None if
    # the material does not carry that branch of the Machinability model.
    surface_speed_hss: Optional[float] = None
    surface_speed_carbide: Optional[float] = None


@dataclass(frozen=True)
class OpContext:
    op_type: Optional[str] = None


@dataclass(frozen=True)
class MachineContext:
    """
    Machine limits used by the resolver's finalization to clamp the
    suggested spindle. ``0.0`` for either bound means "no limit known"
    (matches the Machine model's default) and disables that side of the
    clamp.
    """

    min_rpm: float = 0.0
    max_rpm: float = 0.0


@dataclass(frozen=True)
class FeedSpeedResult:
    horiz_feed: Optional[float] = None
    vert_feed: Optional[float] = None
    spindle_speed: Optional[float] = None
    chipload: Optional[float] = None
    surface_speed: Optional[float] = None  # m/min
    source: str = ""
    confidence: float = 0.0
    warnings: Tuple[str, ...] = ()


@dataclass(frozen=True)
class PartialResult:
    """A provider's contribution to a resolution. None fields are unfilled."""

    horiz_feed: Optional[float] = None
    vert_feed: Optional[float] = None
    spindle_speed: Optional[float] = None
    chipload: Optional[float] = None
    surface_speed: Optional[float] = None  # m/min
    source: str = ""
    confidence: float = 0.0
    warnings: Tuple[str, ...] = ()


def normalize_material_name(name: Optional[str]) -> Optional[str]:
    if name is None:
        return None
    return name.strip().casefold()
