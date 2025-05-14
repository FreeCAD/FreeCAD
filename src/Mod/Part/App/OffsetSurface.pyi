from Base.Metadata import export
from GeometrySurface import GeometrySurface


@export(
    Twin="GeomOffsetSurface",
    TwinPointer="GeomOffsetSurface",
    PythonName="Part.OffsetSurface",
    FatherInclude="Mod/Part/App/GeometrySurfacePy.h",
    Include="Mod/Part/App/Geometry.h",
    Constructor=True,
)
class OffsetSurface(GeometrySurface):
    """
    Author: Werner Mayer (wmayer@users.sourceforge.net)
    Licence: LGPL
    """

    OffsetValue: float = 0.0
    """Sets or gets the offset value to offset the underlying surface."""

    BasisSurface: object = ...
    """Sets or gets the basic surface."""
