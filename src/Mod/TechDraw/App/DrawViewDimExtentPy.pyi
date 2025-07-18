from typing import (
    ClassVar,
    Final,
    List,
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
    Father="DrawViewDimensionPy",
    Name="DrawViewDimExtentPy",
    Twin="DrawViewDimExtent",
    TwinPointer="DrawViewDimExtent",
    Include="Mod/TechDraw/App/DrawViewDimExtent.h",
    Namespace="TechDraw",
    FatherInclude="Mod/TechDraw/App/DrawViewDimensionPy.h",
    FatherNamespace="TechDraw",
)
class DrawViewDimExtentPy(object):
    """
    Feature for creating and manipulating Technical Drawing DimExtents
    """

    def tbd(self) -> Any:
        """tbd() - returns tbd."""
        ...
