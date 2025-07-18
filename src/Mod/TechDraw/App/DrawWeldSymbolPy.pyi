from typing import ClassVar, Final, List, Dict, Tuple, TypeVar, Any, Optional, Union, overload
from TechDraw import object
from Base.Metadata import export
from Base.Metadata import constmethod

@export(
    Father="DrawViewPy",
    Name="DrawWeldSymbolPy",
    Twin="DrawWeldSymbol",
    TwinPointer="DrawWeldSymbol",
    Include="Mod/TechDraw/App/DrawWeldSymbol.h",
    Namespace="TechDraw",
    FatherInclude="Mod/TechDraw/App/DrawViewPy.h",
    FatherNamespace="TechDraw",

)
class DrawWeldSymbolPy(object):
    """
        Feature for adding welding tiles to leader lines
    """


