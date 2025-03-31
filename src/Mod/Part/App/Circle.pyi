from Base.Metadata import export
from Base.Vector import Vector
from Conic import Conic
from Point import Point
from typing import overload


@export(
    PythonName="Part.Circle",
    Twin="GeomCircle",
    TwinPointer="GeomCircle",
    Include="Mod/Part/App/Geometry.h",
    FatherInclude="Mod/Part/App/ConicPy.h",
    FatherNamespace="Part",
    Constructor=True,
)
class Circle(Conic):
    """
    Describes a circle in 3D space
    To create a circle there are several ways:
    Part.Circle()
        Creates a default circle with center (0,0,0) and radius 1

    Part.Circle(Circle)
        Creates a copy of the given circle

    Part.Circle(Circle, Distance)
        Creates a circle parallel to given circle at a certain distance

    Part.Circle(Center,Normal,Radius)
        Creates a circle defined by center, normal direction and radius

    Part.Circle(Point1,Point2,Point3)
        Creates a circle defined by three non-linear points
    """

    Radius: float = 0.0
    """The radius of the circle."""

    @overload
    def __init__(self) -> None: ...

    @overload
    def __init__(self, circle: "Circle") -> None: ...

    @overload
    def __init__(self, circle: "Circle", distance: float) -> None: ...

    @overload
    def __init__(self, center: Point, normal: Vector, radius: float) -> None: ...

    @overload
    def __init__(self, point1: Point, point2: Point, point3: Point) -> None: ...
