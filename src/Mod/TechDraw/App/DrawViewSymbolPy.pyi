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
    Father="DrawViewPy",
    Name="DrawViewSymbolPy",
    Twin="DrawViewSymbol",
    TwinPointer="DrawViewSymbol",
    Include="Mod/TechDraw/App/DrawViewSymbol.h",
    Namespace="TechDraw",
    FatherInclude="Mod/TechDraw/App/DrawViewPy.h",
    FatherNamespace="TechDraw",
)
class DrawViewSymbolPy(object):
    """
    Feature for creating and manipulating Drawing SVG Symbol Views
    """

    def dumpSymbol(self) -> Any:
        """dumpSymbol(fileSpec) - dump the contents of Symbol to a file"""
        ...
