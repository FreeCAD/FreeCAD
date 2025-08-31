from Base.Metadata import export
from GeometryCurve import GeometryCurve
from typing import Any, Final


@export(
    Twin="GeomBoundedCurve",
    TwinPointer="GeomBoundedCurve",
    PythonName="Part.BoundedCurve",
    FatherInclude="Mod/Part/App/GeometryCurvePy.h",
    Include="Mod/Part/App/Geometry.h",
    Constructor=True,
)
class BoundedCurve(GeometryCurve):
    """
    The abstract class BoundedCurve is the root class of all bounded curve objects.

    Author: Abdullah Tahiri (abdullah.tahiri.yo@gmail.com)
    Licence: LGPL
    """

    StartPoint: Final[Any] = ...
    """Returns the starting point of the bounded curve."""

    EndPoint: Final[Any] = ...
    """Returns the end point of the bounded curve."""
