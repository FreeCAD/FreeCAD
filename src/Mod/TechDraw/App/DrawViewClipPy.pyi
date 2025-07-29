from typing import Any

from Base.Metadata import export
from TechDraw.DrawView import DrawView

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
class DrawViewClipPy(DrawView):
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
