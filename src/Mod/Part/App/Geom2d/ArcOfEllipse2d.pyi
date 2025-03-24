from Metadata import export, constmethod
from typing import Final, overload
from Part import ArcOfConic2d

@export(
    PythonName="Part.Geom2d.ArcOfEllipse2d",
    Twin="Geom2dArcOfEllipse",
    TwinPointer="Geom2dArcOfEllipse",
    Include="Mod/Part/App/Geometry2d.h",
    FatherInclude="Mod/Part/App/Geom2d/ArcOfConic2dPy.h",
    Constructor=True,
)
class ArcOfEllipse2d(ArcOfConic2d):
    """
    Describes a portion of an ellipse
    Author: Werner Mayer (wmayer[at]users.sourceforge.net)
    Licence: LGPL
    """

    MajorRadius: float = ...
    """The major radius of the ellipse."""

    MinorRadius: float = ...
    """The minor radius of the ellipse."""

    Ellipse: Final[object] = ...
    """The internal ellipse representation"""

    @overload
    def __init__(self) -> None:
        ...
