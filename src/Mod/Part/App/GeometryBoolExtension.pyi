from Base.Metadata import export
from GeometryExtension import GeometryExtension


@export(
    PythonName="Part.GeometryBoolExtension",
    Include="Mod/Part/App/GeometryDefaultExtension.h",
    FatherInclude="Mod/Part/App/GeometryExtensionPy.h",
    Constructor=True,
)
class GeometryBoolExtension(GeometryExtension):
    """
    A GeometryExtension extending geometry objects with a boolean.
    """

    Value: bool = ...
    """Returns the value of the GeometryBoolExtension."""
