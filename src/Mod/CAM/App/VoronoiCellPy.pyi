from typing import Final, Any
from Base import object
from Base.Metadata import export
from Base.Metadata import constmethod

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
    ReadOnly=[
        "Index",
        "SourceIndex",
        "SourceCategory",
        "SourceCategoryName",
        "IncidentEdge",
    ],
    Constructor=True,
    RichCompare=True,
    Delete=True,
)
class VoronoiCellPy(object):
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
    Index: Final[int]  # ReadOnly
    """Internal id of the element."""

    Color: int
    """Assigned color of the receiver."""

    SourceIndex: Final[int]  # ReadOnly
    """Returns the index of the cell's source"""

    SourceCategory: Final[int]  # ReadOnly
    """Returns the cell's category as an integer"""

    SourceCategoryName: Final[str]  # ReadOnly
    """Returns the cell's category as a string"""

    IncidentEdge: Final[Any]  # ReadOnly
    """Incident edge of the cell - if exists"""
