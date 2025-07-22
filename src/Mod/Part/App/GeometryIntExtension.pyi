from Base.Metadata import export
from Part.GeometryExtension import GeometryExtension
from typing import Final


@export(
    Father="GeometryExtensionPy",
    Name="GeometryIntExtensionPy",
    PythonName="Part.GeometryIntExtension",
    Twin="GeometryIntExtension",
    TwinPointer="GeometryIntExtension",
    Include="Mod/Part/App/GeometryDefaultExtension.h",
    Namespace="Part",
    FatherInclude="Mod/Part/App/GeometryExtensionPy.h",
    FatherNamespace="Part",
    Constructor=True,
)
class GeometryIntExtension(GeometryExtension):
    """
    A GeometryExtension extending geometry objects with an int.

    Author: Abdullah Tahiri (abdullah.tahiri.yo@gmail.com)
    Licence: LGPL
    """

    Value: int = ...
    """returns the value of the GeometryIntExtension."""
