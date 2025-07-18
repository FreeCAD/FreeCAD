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
    Name="DrawViewClipPy",
    Twin="DrawViewClip",
    TwinPointer="DrawViewClip",
    Include="Mod/TechDraw/App/DrawViewClip.h",
    Namespace="TechDraw",
    FatherInclude="Mod/TechDraw/App/DrawViewPy.h",
    FatherNamespace="TechDraw",
)
class DrawViewClipPy(object):
    """
    Feature for creating and manipulating Technical Drawing Clip Views
    """

    def addView(self) -> Any:
        """addView(DrawView) - Add a View to this ClipView"""
        ...

    def removeView(self) -> Any:
        """removeView(DrawView) - Remove specified View to this ClipView"""
        ...

    def getChildViewNames(self) -> Any:
        """getChildViewNames() - get a list of the DrawViews in this ClipView"""
        ...
