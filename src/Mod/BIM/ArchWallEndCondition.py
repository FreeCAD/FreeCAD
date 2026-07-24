# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 FreeCAD Project Association
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

"""End-condition records and provider selection for wall recompute.

Relation solvers publish :class:`WallTrimClaim` values.  The wall resolver
arbitrates those claims before converting the winning one into a
``WallEndCondition`` for wall construction.  Keeping the claim contract here
means two-wall and multi-wall relations expose exactly the same data without
requiring the resolver to know which solver produced it.
"""

from dataclasses import dataclass, field

import FreeCAD

END_CONDITION_SOURCES = ("Relation", "Manual")
DEFAULT_END_CONDITION_ORDER = ["Relation", "Manual"]


@dataclass(frozen=True)
class WallTrimClaim:
    """A typed request to trim one wall end at a global cutting plane."""

    wall: object
    end_name: str
    plane: FreeCAD.Placement
    extension: float = 0.0


@dataclass(frozen=True)
class WallEndCondition:
    source: str
    placement: FreeCAD.Placement = field(default_factory=FreeCAD.Placement)
    is_global: bool = False
    extension: float = 0.0


def select_end_condition(conditions, order):
    """Select the first active condition in the configured provider order."""
    for source in normalize_end_condition_order(order):
        for condition in conditions:
            if condition.source == source and not is_null_placement(condition.placement):
                return condition
    return None


def normalize_end_condition_order(order):
    """Normalize a persisted provider order to the known providers."""
    normalized = []
    for source in order or ():
        if source in END_CONDITION_SOURCES and source not in normalized:
            normalized.append(source)
    return normalized or list(DEFAULT_END_CONDITION_ORDER)


def is_null_placement(placement, tol=1e-9):
    if placement is None:
        return True
    return placement.Base.Length < tol and placement.Rotation.Angle < tol
