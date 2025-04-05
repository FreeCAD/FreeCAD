from Base.Metadata import export
from GeometrySurface import GeometrySurface
from GeometryCurve import GeometryCurve
from Base.Vector import Vector


@export(
    Twin="GeomSurfaceOfExtrusion",
    TwinPointer="GeomSurfaceOfExtrusion",
    PythonName="Part.SurfaceOfExtrusion",
    FatherInclude="Mod/Part/App/GeometrySurfacePy.h",
    Include="Mod/Part/App/Geometry.h",
    Constructor=True,
)
class SurfaceOfExtrusion(GeometrySurface):
    """
    Describes a surface of linear extrusion
    Author: Werner Mayer (<wmayer@users.sourceforge.net>)
    Licence: LGPL
    """

    Direction: Vector = ...
    """Sets or gets the direction of revolution."""

    BasisCurve: GeometryCurve = ...
    """Sets or gets the basic curve."""
