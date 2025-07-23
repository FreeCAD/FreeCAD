from typing import Any, Final

from Base.Metadata import export
from Base.PyObjectBase import PyObjectBase

@export(
    Father="PyObjectBase",
    Name="MeshPointPy",
    Twin="MeshPoint",
    TwinPointer="MeshPoint",
    Include="Mod/Mesh/App/MeshPoint.h",
    Namespace="Mesh",
    FatherInclude="Base/PyObjectBase.h",
    FatherNamespace="Base",
    Constructor=True,
    Delete=True,
)
class MeshPointPy(PyObjectBase):
    """
    Point in mesh
    This is a point in a MeshObject. You can get it by e.g. iterating a
    mesh. The point has a connection to its mesh and allows therefore
    topological operations. It is also possible to create an unbounded mesh point e.g. to create
    a mesh. In this case the topological operations will fail. The same is
    when you cut the bound to the mesh by calling unbound().
    """

    def unbound(self) -> Any:
        """method unbound()
        Cut the connection to a MeshObject. The point becomes
        free and is more or less a simple vector/point.
        After calling unbound() no topological operation will
        work!"""
        ...
    Index: Final[int]
    """The index of this point in the MeshObject"""

    Bound: Final[bool]
    """Bound state of the point"""

    Normal: Final[Any]
    """Normal vector of the point computed by the surrounding mesh."""

    Vector: Final[Any]
    """Vector of the point."""

    x: Final[float]
    """The X component of the point."""

    y: Final[float]
    """The Y component of the point."""

    z: Final[float]
    """The Z component of the point."""
