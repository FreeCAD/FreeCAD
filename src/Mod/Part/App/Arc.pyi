from Base.Metadata import export
from TrimmedCurve import TrimmedCurve
from Geometry import Geom_Circle, Geom_Ellipse
from typing import overload


@export(
    Father="TrimmedCurvePy",
    PythonName="Part.Arc",
    Twin="GeomTrimmedCurve",
    TwinPointer="GeomTrimmedCurve",
    Include="Mod/Part/App/Geometry.h",
    FatherInclude="Mod/Part/App/TrimmedCurvePy.h",
    Constructor=True,
)
class Arc(TrimmedCurve):
    """
    Describes a portion of a curve

    Author: Werner Mayer (wmayer@users.sourceforge.net)
    Licence: LGPL
    """

    @overload
    def __init__(self, circ: Geom_Circle, T: type = ...) -> None: ...

    @overload
    def __init__(self, circ: Geom_Ellipse, T: type = ...) -> None: ...
