from Base.Metadata import export, constmethod
from Base.Vector import Vector
from Conic import Conic
from typing import Final, overload


@export(
    Name="HyperbolaPy",
    Namespace="Part",
    Twin="GeomHyperbola",
    TwinPointer="GeomHyperbola",
    PythonName="Part.Hyperbola",
    FatherInclude="Mod/Part/App/ConicPy.h",
    Include="Mod/Part/App/Geometry.h",
    Father="ConicPy",
    FatherNamespace="Part",
    Constructor=True,
)
class Hyperbola(Conic):
    """
    Describes an hyperbola in 3D space

    To create a hyperbola there are several ways:

    Part.Hyperbola()
        Creates an hyperbola with major radius 2 and minor radius 1 with the
        center in (0,0,0)

    Part.Hyperbola(Hyperbola)
        Create a copy of the given hyperbola

    Part.Hyperbola(S1,S2,Center)
        Creates an hyperbola centered on the point Center, where
        the plane of the hyperbola is defined by Center, S1 and S2,
        its major axis is defined by Center and S1,
        its major radius is the distance between Center and S1, and
        its minor radius is the distance between S2 and the major axis.

    Part.Hyperbola(Center,MajorRadius,MinorRadius)
        Creates an hyperbola with major and minor radii MajorRadius and
        MinorRadius, and located in the plane defined by Center and
        the normal (0,0,1)
    """

    MajorRadius: float
    """The major radius of the hyperbola."""

    MinorRadius: float
    """The minor radius of the hyperbola."""

    Focal: Final[float]
    """The focal distance of the hyperbola."""

    Focus1: Final[Vector]
    """The first focus is on the positive side of the major axis of the hyperbola."""

    Focus2: Final[Vector]
    """The second focus is on the negative side of the major axis of the hyperbola."""

    @overload
    def __init__(self) -> None: ...

    @overload
    def __init__(self, hyperbola: "Hyperbola") -> None: ...

    @overload
    def __init__(self, S1: Vector, S2: Vector, Center: Vector) -> None: ...

    @overload
    def __init__(
        self, Center: Vector, MajorRadius: float, MinorRadius: float
    ) -> None: ...
