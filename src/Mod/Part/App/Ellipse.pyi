from Base.Metadata import export
from Base.Vector import Vector
from Conic import Conic
from typing import Final


@export(
    Twin="GeomEllipse",
    TwinPointer="GeomEllipse",
    PythonName="Part.Ellipse",
    FatherInclude="Mod/Part/App/ConicPy.h",
    Include="Mod/Part/App/Geometry.h",
    Constructor=True,
)
class Ellipse(Conic):
    """
    Describes an ellipse in 3D space

    To create an ellipse there are several ways:

    Part.Ellipse()
        Creates an ellipse with major radius 2 and minor radius 1 with the
        center in (0,0,0)

    Part.Ellipse(Ellipse)
        Create a copy of the given ellipse

    Part.Ellipse(S1,S2,Center)
        Creates an ellipse centered on the point Center, where
        the plane of the ellipse is defined by Center, S1 and S2,
        its major axis is defined by Center and S1,
        its major radius is the distance between Center and S1, and
        its minor radius is the distance between S2 and the major axis.

    Part.Ellipse(Center,MajorRadius,MinorRadius)
        Creates an ellipse with major and minor radii MajorRadius and
        MinorRadius, and located in the plane defined by Center and
        the normal (0,0,1)
    """

    MajorRadius: float = 0.0
    """The major radius of the ellipse."""

    MinorRadius: float = 0.0
    """The minor radius of the ellipse."""

    Focal: Final[float] = 0.0
    """The focal distance of the ellipse."""

    Focus1: Final[Vector] = ...
    """The first focus is on the positive side of the major axis of the ellipse."""

    Focus2: Final[Vector] = ...
    """The second focus is on the negative side of the major axis of the ellipse."""
