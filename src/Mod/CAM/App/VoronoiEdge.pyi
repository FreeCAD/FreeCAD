# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileNotice: Part of the FreeCAD project.

from typing import Any, Final

from Base.BaseClass import BaseClass
from Base.Metadata import constmethod, export

@export(
    Include="Mod/CAM/App/VoronoiEdge.h",
    Namespace="Path",
    RichCompare=True,
    Constructor=True,
    Delete=True,
)
class VoronoiEdge(BaseClass):
    """
    Edge of a Voronoi diagram

    Author: sliptonic (shopinthewoods@gmail.com)
    License: LGPL-2.1-or-later
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
    Index: Final[int]
    """Internal id of the element."""

    Color: int
    """Assigned color of the receiver."""

    Cell: Final[Any]
    """cell the edge belongs to"""

    Vertices: Final[list]
    """Begin and End voronoi vertex"""

    Next: Final[Any]
    """CCW next edge within voronoi cell"""

    Prev: Final[Any]
    """CCW previous edge within voronoi cell"""

    RotNext: Final[Any]
    """Rotated CCW next edge within voronoi cell"""

    RotPrev: Final[Any]
    """Rotated CCW previous edge within voronoi cell"""

    Twin: Final[Any]
    """Twin edge"""
