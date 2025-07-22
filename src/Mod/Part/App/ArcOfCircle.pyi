from Base.Metadata import export
from ArcOfConic import ArcOfConic
from typing import Final


@export(
    PythonName="Part.ArcOfCircle",
    Twin="GeomArcOfCircle",
    TwinPointer="GeomArcOfCircle",
    Include="Mod/Part/App/Geometry.h",
    FatherInclude="Mod/Part/App/ArcOfConicPy.h",
    Constructor=True,
)
class ArcOfCircle(ArcOfConic):
    """
    Author: Werner Mayer (wmayer[at]users.sourceforge.net)
    Licence: LGPL
    Describes a portion of a circle
    """

    Radius: float = ...
    """The radius of the circle."""

    Circle: Final[object] = ...
    """The internal circle representation"""
