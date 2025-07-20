from typing import Any

from Base.Metadata import export
from TechDraw.DrawView import DrawView

@export(
    Father="DrawViewPy",
    Name="DrawViewDimensionPy",
    Twin="DrawViewDimension",
    TwinPointer="DrawViewDimension",
    Include="Mod/TechDraw/App/DrawViewDimension.h",
    Namespace="TechDraw",
    FatherInclude="Mod/TechDraw/App/DrawViewPy.h",
    FatherNamespace="TechDraw",
)
class DrawViewDimensionPy(DrawView):
    """
    Feature for creating and manipulating Technical Drawing Dimensions
    """

    def getRawValue(self) -> Any:
        """getRawValue() - returns Dimension value in mm."""
        ...

    def getText(self) -> Any:
        """getText() - returns Dimension text."""
        ...

    def getLinearPoints(self) -> Any:
        """getLinearPoints() - returns list of points for linear Dimension"""
        ...

    def getArcPoints(self) -> Any:
        """getArcPoints() - returns list of points for circle/arc Dimension"""
        ...

    def getAnglePoints(self) -> Any:
        """getAnglePoints() - returns list of points for angle Dimension"""
        ...

    def getAreaPoints(self) -> Any:
        """getAreaPoints() - returns list of values (center, filled area, actual area) for area Dimension."""
        ...

    def getArrowPositions(self) -> Any:
        """getArrowPositions() - returns list of locations or Dimension Arrowheads. Locations are in unscaled coordinates of parent View"""
        ...
