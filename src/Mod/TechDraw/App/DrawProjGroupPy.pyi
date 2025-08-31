from typing import Any

from Base.Metadata import export
from TechDraw.DrawViewCollection import DrawViewCollection

@export(
    Father="DrawViewCollectionPy",
    Name="DrawProjGroupPy",
    Twin="DrawProjGroup",
    TwinPointer="DrawProjGroup",
    Include="Mod/TechDraw/App/DrawProjGroup.h",
    Namespace="TechDraw",
    FatherInclude="Mod/TechDraw/App/DrawViewCollectionPy.h",
    FatherNamespace="TechDraw",
)
class DrawProjGroupPy(DrawViewCollection):
    """
    Feature for creating and manipulating Technical Drawing Projection Groups
    """

    def addProjection(self) -> Any:
        """addProjection(string projectionType) - Add a new Projection Item to this Group. Returns DocObj."""
        ...

    def removeProjection(self) -> Any:
        """removeProjection(string projectionType) - Remove specified Projection Item from this Group. Returns int number of views in Group."""
        ...

    def purgeProjections(self) -> Any:
        """purgeProjections() - Remove all Projection Items from this Group. Returns int number of views in Group (0)."""
        ...

    def getItemByLabel(self) -> Any:
        """getItemByLabel(string projectionType) - return specified Projection Item"""
        ...

    def getXYPosition(self) -> Any:
        """getXYPosition(string projectionType) - return the AutoDistribute position for specified Projection Item"""
        ...
