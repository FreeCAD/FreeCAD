from Base.Metadata import export, constmethod
from Base.Vector import Vector
from Geometry import Geometry
from typing import overload


@export(
    PythonName="Part.Point",
    Twin="GeomPoint",
    TwinPointer="GeomPoint",
    Include="Mod/Part/App/Geometry.h",
    FatherInclude="Mod/Part/App/GeometryPy.h",
    Constructor=True,
)
class Point(Geometry):
    """
    Describes a point
    To create a point there are several ways:
    Part.Point()
        Creates a default point

    Part.Point(Point)
        Creates a copy of the given point

    Part.Point(Vector)
        Creates a line for the given coordinates

    Author: Werner Mayer (wmayer@users.sourceforge.net)
    Licence: LGPL
    """

    @overload
    def __init__(self) -> None: ...

    @overload
    def __init__(self, other: "Point") -> None: ...

    @overload
    def __init__(self, coordinates: Vector) -> None: ...

    @constmethod
    def toShape(self) -> object:
        """
        Create a vertex from this point.
        """
        ...

    X: float = ...
    """X component of this point."""

    Y: float = ...
    """Y component of this point."""

    Z: float = ...
    """Z component of this point."""
