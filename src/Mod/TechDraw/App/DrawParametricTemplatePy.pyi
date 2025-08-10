from typing import Any, Final

from Base.Metadata import export
from DrawTemplate import DrawTemplate

@export(
    Father="DrawTemplatePy",
    Name="DrawParametricTemplatePy",
    Twin="DrawParametricTemplate",
    TwinPointer="DrawParametricTemplate",
    Include="Mod/TechDraw/App/DrawParametricTemplate.h",
    Namespace="TechDraw",
    FatherInclude="DrawTemplatePy.h",
    FatherNamespace="TechDraw",
)
class DrawParametricTemplatePy(DrawTemplate):
    """
    Feature for creating and manipulating Technical Drawing Templates
    """

    def drawLine(self) -> Any:
        """Draw a line"""
        ...
    GeometryCount: Final[int]
    """Number of geometry in template"""
