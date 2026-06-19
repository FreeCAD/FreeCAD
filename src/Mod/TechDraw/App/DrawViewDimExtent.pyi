# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from typing import Any

from Base.Metadata import export
from TechDraw.DrawViewDimension import DrawViewDimension

@export()
class DrawViewDimExtent(DrawViewDimension):
    """
    Feature for creating and manipulating Technical Drawing DimExtents

    Author: WandererFan (wandererfan@gmail.com)
    License: LGPL-2.1-or-later
    """

    def tbd(self) -> Any:
        """tbd() - returns tbd."""
        ...
