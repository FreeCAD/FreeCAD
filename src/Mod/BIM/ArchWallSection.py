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

"""Immutable cross-section data for wall relation geometry.

The wall proxy resolves FreeCAD wall semantics.  This module owns only the
typed section record and calculations over an already-resolved profile.
"""

from dataclasses import dataclass


@dataclass(frozen=True)
class WallSectionLayer:
    """One resolved material layer in local lateral coordinates.

    ``raw_thickness`` retains the source sign.  Positive layers are visible;
    negative layers are construction-only cursor steps.  ``y_min`` and
    ``y_max`` describe the layer's resolved lateral interval.
    """

    raw_thickness: float
    y_min: float
    y_max: float

    @property
    def visible(self):
        return self.raw_thickness > 0


@dataclass(frozen=True)
class WallSection:
    """Immutable resolved wall section consumed by relation geometry.

    The layer tuple is already ordered and positioned by ``ArchWall``.  This
    value object exposes only geometric queries and never resolves FreeCAD
    properties or wall proxies.
    """

    layers: tuple

    @property
    def visible_layers(self):
        return tuple(layer for layer in self.layers if layer.visible)

    @property
    def y_min(self):
        return min((layer.y_min for layer in self.visible_layers), default=0.0)

    @property
    def y_max(self):
        return max((layer.y_max for layer in self.visible_layers), default=0.0)

    def offset_towards(self, lateral_direction, world_direction):
        """Return the signed lateral offset to the requested visible face.

        A zero world direction has no preferred face, so it selects the
        ``y_min`` face deterministically.  Callers that require a directional
        result should resolve or reject that ambiguity before calling here.
        """
        if lateral_direction is None or world_direction is None:
            return None
        if lateral_direction.Length <= 1e-9:
            return None
        if not self.visible_layers:
            return None
        if world_direction.Length <= 1e-9:
            return -self.y_min
        if lateral_direction.dot(world_direction) >= 0:
            return -self.y_min
        return -self.y_max
