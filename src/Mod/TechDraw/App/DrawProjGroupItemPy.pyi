from typing import Any

from Base.Metadata import export
from TechDraw.DrawViewPart import DrawViewPart

@export(
    Father="DrawViewPartPy",
    Name="DrawProjGroupItemPy",
    Twin="DrawProjGroupItem",
    TwinPointer="DrawProjGroupItem",
    Include="Mod/TechDraw/App/DrawProjGroupItem.h",
    Namespace="TechDraw",
    FatherInclude="Mod/TechDraw/App/DrawViewPartPy.h",
    FatherNamespace="TechDraw",
)
class DrawProjGroupItemPy(DrawViewPart):
    """
    Feature for creating and manipulating component Views Technical Drawing Projection Groups
    """

    def autoPosition(self) -> Any:
        """autoPosition() - Move to AutoDistribute/Unlocked position on Page. Returns none."""
        ...
