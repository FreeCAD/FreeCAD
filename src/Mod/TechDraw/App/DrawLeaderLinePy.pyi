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
    Name="DrawLeaderLinePy",
    Twin="DrawLeaderLine",
    TwinPointer="DrawLeaderLine",
    Include="Mod/TechDraw/App/DrawLeaderLine.h",
    Namespace="TechDraw",
    FatherInclude="Mod/TechDraw/App/DrawViewPy.h",
    FatherNamespace="TechDraw",
)
class DrawLeaderLinePy(object):
    """
    Feature for adding leaders to Technical Drawings
    """
