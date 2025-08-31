from typing import Any

from Base.Metadata import export
from TechDraw.DrawView import DrawView

@export(
    Father="DrawViewPy",
    Name="DrawViewCollectionPy",
    Twin="DrawViewCollection",
    TwinPointer="DrawViewCollection",
    Include="Mod/TechDraw/App/DrawViewCollection.h",
    Namespace="TechDraw",
    FatherInclude="Mod/TechDraw/App/DrawViewPy.h",
    FatherNamespace="TechDraw",
)
class DrawViewCollectionPy(DrawView):
    """
    Feature for creating and manipulating Technical Drawing View Collections
    """

    def addView(self) -> Any:
        """addView(DrawView object) - Add a new View to this Group. Returns count of views."""
        ...

    def removeView(self) -> Any:
        """removeView(DrawView object) - Remove specified Viewfrom this Group. Returns count of views in Group."""
        ...
