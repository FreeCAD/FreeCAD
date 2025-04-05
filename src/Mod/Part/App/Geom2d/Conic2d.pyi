from Base.Metadata import export
from typing import Final
from Part.Geom2d import Curve2d

@export(
    PythonName="Part.Geom2d.Conic2d",
    Twin="Geom2dConic",
    TwinPointer="Geom2dConic",
    Include="Mod/Part/App/Geometry2d.h",
    FatherInclude="Mod/Part/App/Geom2d/Curve2dPy.h",
    Constructor=True,
)
class Conic2d(Curve2d):
    """
    Describes an abstract conic in 2d space

    Author: Werner Mayer (wmayer@users.sourceforge.net)
    Licence: LGPL
    """

    Location: object = ...
    """Location of the conic."""

    Eccentricity: Final[float] = ...
    """                
    returns the eccentricity value of the conic e.
        e = 0 for a circle
        0 < e < 1 for an ellipse  (e = 0 if MajorRadius = MinorRadius)
        e > 1 for a hyperbola
        e = 1 for a parabola
    """

    XAxis: object = ...
    """The X axis direction of the circle"""

    YAxis: object = ...
    """The Y axis direction of the circle"""
