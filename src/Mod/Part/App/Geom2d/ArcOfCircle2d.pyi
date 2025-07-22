from Metadata import export, constmethod
from typing import Final, overload
from Part.Geom2d import ArcOfConic2d

@export(
    PythonName="Part.Geom2d.ArcOfCircle2d",
    Twin="Geom2dArcOfCircle",
    TwinPointer="Geom2dArcOfCircle",
    Include="Mod/Part/App/Geometry2d.h",
    FatherInclude="Mod/Part/App/Geom2d/ArcOfConic2dPy.h",
    Constructor=True,
)
class ArcOfCircle2d(ArcOfConic2d):
    """
    Describes a portion of a circle

    Author: Werner Mayer (wmayer[at]users.sourceforge.net)
    Licence: LGPL
    """

    Radius: float = ...
    """The radius of the circle."""

    Circle: Final[object] = ...
    """The internal circle representation"""

    @overload
    def __init__(self, Radius: float, Circle: object) -> None: ...
    """
    ArcOfCircle2d(Radius, Circle) -> None

    Constructor for ArcOfCircle2d.

    Parameters:
        Radius : float
            The radius of the circle.
        Circle : object
            The internal circle representation.
    """
    ...