from Base.Metadata import export
from Base.Placement import Placement
from Base.Vector import Vector
from GeometrySurface import GeometrySurface
from GeometryCurve import GeometryCurve
from typing import overload


@export(
    Twin="GeomSurfaceOfRevolution",
    TwinPointer="GeomSurfaceOfRevolution",
    PythonName="Part.SurfaceOfRevolution",
    FatherInclude="Mod/Part/App/GeometrySurfacePy.h",
    Include="Mod/Part/App/Geometry.h",
    Constructor=True,
)
class SurfaceOfRevolution(GeometrySurface):
    """
    Describes a surface of revolution

    Author: Werner Mayer (wmayer@users.sourceforge.net)
    Licence: LGPL
    """

    Location: Placement
    """Sets or gets the location of revolution."""

    Direction: Vector
    """Sets or gets the direction of revolution."""

    BasisCurve: GeometryCurve
    """Sets or gets the basic curve."""

    @overload
    def __init__(
        self, location: Placement, direction: Vector, basis_curve: GeometryCurve
    ) -> None: ...
