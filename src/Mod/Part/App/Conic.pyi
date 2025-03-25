from Base.Metadata import export
from GeometryCurve import GeometryCurve
from typing import Final


@export(
    PythonName="Part.Conic",
    Twin="GeomConic",
    TwinPointer="GeomConic",
    Include="Mod/Part/App/Geometry.h",
    FatherInclude="Mod/Part/App/GeometryCurvePy.h",
    Constructor=True,
)
class Conic(GeometryCurve):
    """
    Describes an abstract conic in 3d space

    Author: Werner Mayer (wmayer@users.sourceforge.net)
    Licence: LGPL
    """

    Location: object = ...
    """Location of the conic."""

    Center: object = ...
    """Deprecated -- use Location."""

    Eccentricity: Final[float] = ...
    """
    Returns the eccentricity value of the conic e.
    e = 0 for a circle
    0 < e < 1 for an ellipse  (e = 0 if MajorRadius = MinorRadius)
    e > 1 for a hyperbola
    e = 1 for a parabola
    """

    AngleXU: float = ...
    """The angle between the X axis and the major axis of the conic."""

    Axis: object = ...
    """The axis direction of the circle"""

    XAxis: object = ...
    """The X axis direction of the circle"""

    YAxis: object = ...
    """The Y axis direction of the circle"""
