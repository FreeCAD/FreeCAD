from Base.Metadata import export, constmethod
from Part.GeometrySurface import GeometrySurface
from Base.Vector import Vector
from typing import Final


@export(
    Name="ToroidPy",
    Namespace="Part",
    Twin="GeomToroid",
    TwinPointer="GeomToroid",
    PythonName="Part.Toroid",
    FatherInclude="Mod/Part/App/GeometrySurfacePy.h",
    Include="Mod/Part/App/Geometry.h",
    Father="GeometrySurfacePy",
    FatherNamespace="Part",
    Constructor=True,
)
class Toroid(GeometrySurface):
    """
    Describes a toroid in 3D space
    """

    MajorRadius: float = ...
    """The major radius of the toroid."""

    MinorRadius: float = ...
    """The minor radius of the toroid."""

    Center: Vector = ...
    """Center of the toroid."""

    Axis: Vector = ...
    """The axis direction of the toroid"""

    Area: Final[float] = 0.0
    """Compute the area of the toroid."""

    Volume: Final[float] = 0.0
    """Compute the volume of the toroid."""
