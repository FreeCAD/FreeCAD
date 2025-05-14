from Base.Metadata import export
from GeometryExtension import GeometryExtension


@export(
    PythonName="Part.GeometryStringExtension",
    Include="Mod/Part/App/GeometryDefaultExtension.h",
    FatherInclude="Mod/Part/App/GeometryExtensionPy.h",
    Constructor=True,
)
class GeometryStringExtension(GeometryExtension):
    """
    A GeometryExtension extending geometry objects with a string.

    Author: Abdullah Tahiri (abdullah.tahiri.yo@gmail.com)
    Licence: LGPL
    """

    Value: str = ...
    """returns the value of the GeometryStringExtension."""
