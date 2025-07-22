from Base.Metadata import export, constmethod
from Base.Vector import Vector
from Conic import Conic
from typing import Final


@export(
    Twin="GeomParabola",
    TwinPointer="GeomParabola",
    PythonName="Part.Parabola",
    FatherInclude="Mod/Part/App/ConicPy.h",
    Include="Mod/Part/App/Geometry.h",
    Constructor=True,
)
class Parabola(Conic):
    """
    Describes a parabola in 3D space
    """

    Focal: float = ...
    """
    The focal distance is the distance between
    the apex and the focus of the parabola.
    """

    Focus: Final[Vector] = ...
    """
    The focus is on the positive side of the
    'X Axis' of the local coordinate system of the parabola.
    """

    Parameter: Final[float] = ...
    """
    Compute the parameter of this parabola
    which is the distance between its focus
    and its directrix. This distance is twice the focal length.
    """

    def compute(self, p1: Vector, p2: Vector, p3: Vector) -> None:
        """
        compute(p1,p2,p3) -> None

        The three points must lie on a plane parallel to xy plane and must not be collinear
        """
        ...
