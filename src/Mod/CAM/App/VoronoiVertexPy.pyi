from typing import Final, Any
from Base import object
from Base.Metadata import export
from Base.Metadata import constmethod

@export(
    Father="BaseClassPy",
    Name="VoronoiVertexPy",
    PythonName="Path.Voronoi.Vertex",
    Twin="VoronoiVertex",
    TwinPointer="VoronoiVertex",
    Include="Mod/CAM/App/VoronoiVertex.h",
    Namespace="Path",
    FatherInclude="Base/BaseClassPy.h",
    FatherNamespace="Base",
    ReadOnly=["Index", "X", "Y", "IncidentEdge"],
    Constructor=True,
    RichCompare=True,
    Delete=True,
)
class VoronoiVertexPy(object):
    """
    Vertex of a Voronoi diagram
    """

    @constmethod
    def toPoint(self) -> Any:
        """Returns a Vector - or None if not possible"""
        ...
    Index: Final[int]
    """Internal id of the element."""

    Color: int
    """Assigned color of the receiver."""

    X: Final[float]
    """X position"""

    Y: Final[float]
    """Y position"""

    IncidentEdge: Final[Any]
    """Y position"""
