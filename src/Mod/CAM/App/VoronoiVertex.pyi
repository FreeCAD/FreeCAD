from typing import Any, Final

from Base.BaseClass import BaseClass
from Base.Metadata import constmethod, export

@export(
    Include="Mod/CAM/App/VoronoiVertex.h",
    Namespace="Path",
    Constructor=True,
    RichCompare=True,
    Delete=True,
)
class VoronoiVertex(BaseClass):
    """
    Author: sliptonic (shopinthewoods@gmail.com)
    License: LGPL-2.1-or-later
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
