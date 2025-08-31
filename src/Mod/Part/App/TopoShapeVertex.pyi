from Base.Metadata import export
from Base.Vector import Vector
from TopoShape import TopoShape
from typing import Final


@export(
    Twin="TopoShape",
    TwinPointer="TopoShape",
    FatherInclude="Mod/Part/App/TopoShapePy.h",
    Include="Mod/Part/App/TopoShape.h",
    Constructor=True,
)
class TopoShapeVertex(TopoShape):
    """
    TopoShapeVertex is the OpenCasCade topological vertex wrapper

    Author: Juergen Riegel (Juergen.Riegel@web.de)
    """

    X: Final[float] = ...
    """X component of this Vertex."""

    Y: Final[float] = ...
    """Y component of this Vertex."""

    Z: Final[float] = ...
    """Z component of this Vertex."""

    Point: Final[Vector] = ...
    """Position of this Vertex as a Vector"""

    Tolerance: float = ...
    """Set or get the tolerance of the vertex"""
