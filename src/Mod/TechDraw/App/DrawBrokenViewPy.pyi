from typing import Any

from Base.Metadata import export
from TechDraw.DrawViewPart import DrawViewPart

@export(
    Father="DrawViewPartPy",
    Name="DrawBrokenViewPy",
    Twin="DrawBrokenView",
    TwinPointer="DrawBrokenView",
    Include="Mod/TechDraw/App/DrawBrokenView.h",
    Namespace="TechDraw",
    FatherInclude="Mod/TechDraw/App/DrawViewPartPy.h",
    FatherNamespace="TechDraw",
)
class DrawBrokenViewPy(DrawViewPart):
    """
    Feature for creating and manipulating Technical Drawing broken views
    """

    def mapPoint3dToView(self) -> Any:
        """point2d = mapPoint3dToView(point3d) - returns the position of the 3d point within the broken view."""
        ...

    def mapPoint2dFromView(self) -> Any:
        """point2d = mapPoint2dFromView(point3d) - returns the position of the 2d point within an unbroken view."""
        ...

    def getCompressedCenter(self) -> Any:
        """point3d = getCompressedCenter() - returns the geometric center of the source shapes after break cuts and gap compression."""
        ...
