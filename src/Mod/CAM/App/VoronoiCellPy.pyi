from typing import Any, Final

from Base.BaseClass import BaseClass
from Base.Metadata import constmethod, export

@export(
    Father="BaseClassPy",
    Name="VoronoiCellPy",
    Twin="VoronoiCell",
    PythonName="Path.Voronoi.Cell",
    TwinPointer="VoronoiCell",
    Include="Mod/CAM/App/VoronoiCell.h",
    Namespace="Path",
    FatherInclude="Base/BaseClassPy.h",
    FatherNamespace="Base",
    Constructor=True,
    RichCompare=True,
    Delete=True,
)
class VoronoiCellPy(BaseClass):
    """
    Cell of a Voronoi diagram
    """

    @constmethod
    def containsPoint(self) -> Any:
        """Returns true if the cell contains a point site"""
        ...

    @constmethod
    def containsSegment(self) -> Any:
        """Returns true if the cell contains a segment site"""
        ...

    @constmethod
    def isDegenerate(self) -> Any:
        """Returns true if the cell doesn't have an incident edge"""
        ...

    @constmethod
    def getSource(self) -> Any:
        """Returns the Source for the cell"""
        ...
    Index: Final[int]
    """Internal id of the element."""

    Color: int
    """Assigned color of the receiver."""

    SourceIndex: Final[int]
    """Returns the index of the cell's source"""

    SourceCategory: Final[int]
    """Returns the cell's category as an integer"""

    SourceCategoryName: Final[str]
    """Returns the cell's category as a string"""

    IncidentEdge: Final[Any]
    """Incident edge of the cell - if exists"""
