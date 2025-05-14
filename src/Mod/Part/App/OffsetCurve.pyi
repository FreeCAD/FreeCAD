from Base.Metadata import export
from Base.Vector import Vector
from GeometryCurve import GeometryCurve
from typing import Final


@export(
    PythonName="Part.OffsetCurve",
    Twin="GeomOffsetCurve",
    TwinPointer="GeomOffsetCurve",
    Include="Mod/Part/App/Geometry.h",
    FatherInclude="Mod/Part/App/GeometryCurvePy.h",
    Constructor=True,
)
class OffsetCurve(GeometryCurve):
    """
    Author: Werner Mayer (wmayer@users.sourceforge.net)
    Licence: LGPL
    """

    OffsetValue: float = ...
    """Sets or gets the offset value to offset the underlying curve."""

    OffsetDirection: Vector = ...
    """Sets or gets the offset direction to offset the underlying curve."""

    BasisCurve: GeometryCurve = ...
    """Sets or gets the basic curve."""
