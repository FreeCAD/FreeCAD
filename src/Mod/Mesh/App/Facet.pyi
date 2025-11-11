# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from typing import Any, Final

from Base.Metadata import export
from Base.PyObjectBase import PyObjectBase

@export(
    Include="Mod/Mesh/App/Facet.h",
    Namespace="Mesh",
    Constructor=True,
    Delete=True,
)
class Facet(PyObjectBase):
    """
    Facet in mesh
    This is a facet in a MeshObject. You can get it by e.g. iterating a
    mesh. The facet has a connection to its mesh and allows therefore
    topological operations. It is also possible to create an unbounded facet e.g. to create
    a mesh. In this case the topological operations will fail. The same is
    when you cut the bound to the mesh by calling unbound().

    Author: Juergen Riegel (Juergen.Riegel@web.de)
    License: LGPL-2.1-or-later
    """

    def unbound(self) -> Any:
        """method unbound()
        Cut the connection to a MeshObject. The facet becomes
        free and is more or less a simple facet.
        After calling unbound() no topological operation will
        work!"""
        ...

    def intersect(self) -> Any:
        """intersect(Facet) -> list
        Get a list of intersection points with another triangle."""
        ...

    def isDegenerated(self) -> Any:
        """isDegenerated([float]) -> boolean
        Returns true if the facet is degenerated, otherwise false."""
        ...

    def isDeformed(self) -> Any:
        """isDegenerated(MinAngle, MaxAngle) -> boolean
        Returns true if the facet is deformed, otherwise false.
        A triangle is considered deformed if an angle is less than MinAngle
        or higher than MaxAngle.
        The two angles are given in radian."""
        ...

    def getEdge(self) -> Any:
        """getEdge(int) -> Edge
        Returns the edge of the facet."""
        ...
    Index: Final[int]
    """The index of this facet in the MeshObject"""

    Bound: Final[bool]
    """Bound state of the facet"""

    Normal: Final[Any]
    """Normal vector of the facet."""

    Points: Final[list]
    """A list of points of the facet"""

    PointIndices: Final[tuple]
    """The index tuple of point vertices of the mesh this facet is built of"""

    NeighbourIndices: Final[tuple]
    """The index tuple of neighbour facets of the mesh this facet is adjacent with"""

    Area: Final[float]
    """The area of the facet"""

    AspectRatio: Final[float]
    """The aspect ratio of the facet computed by longest edge and its height"""

    AspectRatio2: Final[float]
    """The aspect ratio of the facet computed by radius of circum-circle and in-circle"""

    Roundness: Final[float]
    """The roundness of the facet"""

    CircumCircle: Final[tuple]
    """The center and radius of the circum-circle"""

    InCircle: Final[tuple]
    """The center and radius of the in-circle"""
