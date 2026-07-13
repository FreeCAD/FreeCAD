# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 FreeCAD Project Association
# SPDX-FileNotice: Part of the FreeCAD project.
################################################################################
#                                                                              #
#   FreeCAD is free software: you can redistribute it and/or modify            #
#   it under the terms of the GNU Lesser General Public                       #
#   License as published by the Free Software Foundation, either version 2    #
#   of the License, or (at your option) any later version.                     #
#                                                                              #
################################################################################

"""Resolved wall baseline value objects.

The baseline contract is independent of wall proxies and FreeCAD wall
properties.  Callers provide global straight geometry together with explicit
semantic endpoints; later path operations build on this value.
"""

from dataclasses import dataclass

import FreeCAD
import Part


@dataclass(frozen=True)
class WallBaseline:
    """An oriented, resolved wall baseline in global coordinates.

    ``start_point`` and ``end_point`` are semantic wall endpoints.  They are
    stored explicitly so callers never need to infer Start/End from the
    topological ordering of a rebuilt shape.
    """

    edge: Part.Edge
    normal: FreeCAD.Vector
    start_point: FreeCAD.Vector
    end_point: FreeCAD.Vector

    def __post_init__(self):
        normal = FreeCAD.Vector(self.normal)
        normal.normalize()
        object.__setattr__(self, "normal", normal)
        object.__setattr__(self, "start_point", FreeCAD.Vector(self.start_point))
        object.__setattr__(self, "end_point", FreeCAD.Vector(self.end_point))
