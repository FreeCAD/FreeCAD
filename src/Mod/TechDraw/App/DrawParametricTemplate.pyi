from typing import Any, Final

from Base.Metadata import export
from DrawTemplate import DrawTemplate

@export(
    Include="Mod/TechDraw/App/DrawParametricTemplate.h",
    Namespace="TechDraw",
    FatherInclude="DrawTemplatePy.h",
)
class DrawParametricTemplate(DrawTemplate):
    """
    Author: Luke Parry (l.parry@warwick.ac.uk)
    License: LGPL-2.1-or-later
    Feature for creating and manipulating Technical Drawing Templates
    """

    def drawLine(self) -> Any:
        """Draw a line"""
        ...
    GeometryCount: Final[int]
    """Number of geometry in template"""
