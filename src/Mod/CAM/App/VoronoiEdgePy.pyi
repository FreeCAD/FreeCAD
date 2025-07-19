from typing import Final, Any
from Base import object
from Base.Metadata import export
from Base.Metadata import constmethod

@export(
    Father="BaseClassPy",
    Name="VoronoiEdgePy",
    Twin="VoronoiEdge",
    TwinPointer="VoronoiEdge",
    PythonName="Path.Voronoi.Edge",
    Include="Mod/CAM/App/VoronoiEdge.h",
    Namespace="Path",
    FatherInclude="Base/BaseClassPy.h",
    FatherNamespace="Base",
    ReadOnly=[
        "Index",
        "Cell",
        "Vertices",
        "Next",
        "Prev",
        "RotNext",
        "RotPrev",
        "Twin",
    ],
    RichCompare=True,
    Constructor=True,
    Delete=True,
)
class VoronoiEdgePy(object):
    """
    Edge of a Voronoi diagram
    """

    @constmethod
    def isFinite(self) -> Any:
        """Returns true if both vertices are finite"""
        ...

    @constmethod
    def isInfinite(self) -> Any:
        """Returns true if the end vertex is infinite"""
        ...

    @constmethod
    def isLinear(self) -> Any:
        """Returns true if edge is straight"""
        ...

    @constmethod
    def isCurved(self) -> Any:
        """Returns true if edge is curved"""
        ...

    @constmethod
    def isPrimary(self) -> Any:
        """Returns false if edge goes through endpoint of the segment site"""
        ...

    @constmethod
    def isSecondary(self) -> Any:
        """Returns true if edge goes through endpoint of the segment site"""
        ...

    @constmethod
    def isBorderline(self) -> Any:
        """Returns true if the point is on the segment"""
        ...

    @constmethod
    def toShape(self) -> Any:
        """Returns a shape for the edge"""
        ...

    @constmethod
    def getDistances(self) -> Any:
        """Returns the distance of the vertices to the input source"""
        ...

    @constmethod
    def getSegmentAngle(self) -> Any:
        """Returns the angle (in degree) of the segments if the edge was formed by two segments"""
        ...
    Index: Final[int]  # ReadOnly
    """Internal id of the element."""

    Color: int
    """Assigned color of the receiver."""

    Cell: Final[Any]  # ReadOnly
    """cell the edge belongs to"""

    Vertices: Final[list]  # ReadOnly
    """Begin and End voronoi vertex"""

    Next: Final[Any]  # ReadOnly
    """CCW next edge within voronoi cell"""

    Prev: Final[Any]  # ReadOnly
    """CCW previous edge within voronoi cell"""

    RotNext: Final[Any]  # ReadOnly
    """Rotated CCW next edge within voronoi cell"""

    RotPrev: Final[Any]  # ReadOnly
    """Rotated CCW previous edge within voronoi cell"""

    Twin: Final[Any]  # ReadOnly
    """Twin edge"""
