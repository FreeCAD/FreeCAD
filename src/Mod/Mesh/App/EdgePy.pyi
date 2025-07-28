from typing import Final

from Base.Metadata import export
from Base.PyObjectBase import PyObjectBase

@export(
    Father="PyObjectBase",
    Name="EdgePy",
    Twin="Edge",
    TwinPointer="Edge",
    Include="Mod/Mesh/App/Edge.h",
    Namespace="Mesh",
    FatherInclude="Base/PyObjectBase.h",
    FatherNamespace="Base",
    Constructor=True,
    Delete=True,
)
class EdgePy(PyObjectBase):
    """
    Edge in mesh
    This is an edge of a facet in a MeshObject. You can get it by e.g. iterating over the facets of a
    mesh and calling getEdge(index).
    """

    def intersectWithEdge(self) -> Any:
        """intersectWithEdge(Edge) -> list
        Get a list of intersection points with another edge."""
        ...

    def isParallel(self) -> Any:
        """isParallel(Edge) -> bool
        Checks if the two edges are parallel."""
        ...

    def isCollinear(self) -> Any:
        """isCollinear(Edge) -> bool
        Checks if the two edges are collinear."""
        ...

    def unbound(self) -> Any:
        """method unbound()
        Cut the connection to a MeshObject. The edge becomes
        free and is more or less a simple edge.
        After calling unbound() no topological operation will
        work!"""
        ...
    Index: Final[int]
    """The index of this edge of the facet"""

    Points: Final[list]
    """A list of points of the edge"""

    PointIndices: Final[tuple]
    """The index tuple of point vertices of the mesh this edge is built of"""

    NeighbourIndices: Final[tuple]
    """The index tuple of neighbour facets of the mesh this edge is adjacent with"""

    Length: Final[float]
    """The length of the edge"""

    Bound: Final[bool]
    """Bound state of the edge"""
