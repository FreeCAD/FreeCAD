from typing import ClassVar, Final, List, Dict, Tuple, TypeVar, Any, Optional, Union, overload
from TechDraw import object
from Base.Metadata import export
from Base.Metadata import constmethod

@export(
    Father="DrawTilePy",
    Name="DrawTileWeldPy",
    Twin="DrawTileWeld",
    TwinPointer="DrawTileWeld",
    Include="Mod/TechDraw/App/DrawTileWeld.h",
    Namespace="TechDraw",
    FatherInclude="Mod/TechDraw/App/DrawTilePy.h",
    FatherNamespace="TechDraw",

)
class DrawTileWeldPy(object):
    """
        Feature for adding welding tiles to leader lines
    """


