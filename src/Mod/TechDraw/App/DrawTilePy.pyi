from typing import ClassVar, Final, List, Dict, Tuple, TypeVar, Any, Optional, Union, overload
from App import object
from Base.Metadata import export
from Base.Metadata import constmethod

@export(
    Father="DocumentObjectPy",
    Name="DrawTilePy",
    Twin="DrawTile",
    TwinPointer="DrawTile",
    Include="Mod/TechDraw/App/DrawTile.h",
    Namespace="TechDraw",
    FatherInclude="App/DocumentObjectPy.h",
    FatherNamespace="App",

)
class DrawTilePy(object):
    """
        Feature for adding tiles to leader lines
    """


