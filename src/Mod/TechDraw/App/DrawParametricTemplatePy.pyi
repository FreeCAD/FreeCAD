from typing import (
    ClassVar,
    Final,
    List,
    Long,
    Dict,
    Tuple,
    TypeVar,
    Any,
    Optional,
    Union,
    overload,
)
from TechDraw import object
from Base.Metadata import export
from Base.Metadata import constmethod

@export(
    Father="DrawTemplatePy",
    Name="DrawParametricTemplatePy",
    Twin="DrawParametricTemplate",
    TwinPointer="DrawParametricTemplate",
    Include="Mod/TechDraw/App/DrawParametricTemplate.h",
    Namespace="TechDraw",
    FatherInclude="DrawTemplatePy.h",
    FatherNamespace="TechDraw",
    ReadOnly=["GeometryCount"],
)
class DrawParametricTemplatePy(object):
    """
    Feature for creating and manipulating Technical Drawing Templates
    """

    def drawLine(self) -> Any:
        """Draw a line"""
        ...
    GeometryCount: Final[int]  # ReadOnly
    """Number of geometry in template"""
