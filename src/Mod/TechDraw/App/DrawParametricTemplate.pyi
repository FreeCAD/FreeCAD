# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from typing import Final

from Base.Metadata import export
from DrawTemplate import DrawTemplate


@export(
    Include="Mod/TechDraw/App/DrawParametricTemplate.h",
    Namespace="TechDraw",
    FatherInclude="DrawTemplatePy.h",
)
class DrawParametricTemplate(DrawTemplate):
    """
    Feature for creating and manipulating Technical Drawing Templates

    Author: Luke Parry (l.parry@warwick.ac.uk)
    License: LGPL-2.1-or-later
    """

    def drawLine(self, x1: float, y1: float, x2: float, y2: float, /) -> None:
        """Draw a line"""
        ...

    GeometryCount: Final[int]
    """Number of geometry in template"""
