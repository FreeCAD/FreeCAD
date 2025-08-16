from typing import Any

from Base.Metadata import export
from TechDraw.DrawView import DrawView

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
class DrawViewSymbolPy(DrawView):
    """
    Feature for creating and manipulating Drawing SVG Symbol Views
    """

    def dumpSymbol(self) -> Any:
        """dumpSymbol(fileSpec) - dump the contents of Symbol to a file"""
        ...
