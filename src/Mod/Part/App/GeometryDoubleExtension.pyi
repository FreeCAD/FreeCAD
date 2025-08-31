from Base.Metadata import export
from GeometryExtension import GeometryExtension


@export(
    PythonName="Part.GeometryDoubleExtension",
    Include="Mod/Part/App/GeometryDefaultExtension.h",
    FatherInclude="Mod/Part/App/GeometryExtensionPy.h",
    Constructor=True,
)
class GeometryDoubleExtension(GeometryExtension):
    """
    A GeometryExtension extending geometry objects with a double.

    Author: Abdullah Tahiri (abdullah.tahiri.yo@gmail.com)
    Licence: LGPL
    """

    Value: float = ...
    """Returns the value of the GeometryDoubleExtension."""
