# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Base.Metadata import export
from Base.Vector import Vector
from Base.Axis import Axis as AxisPy
from GeometrySurface import GeometrySurface
from typing import Final

@export(
    Twin="GeomSphere",
    TwinPointer="GeomSphere",
    PythonName="Part.Sphere",
    FatherInclude="Mod/Part/App/GeometrySurfacePy.h",
    Include="Mod/Part/App/Geometry.h",
    Constructor=True,
)
class Sphere(GeometrySurface):
    """
    Describes a sphere in 3D space

    Author: Werner Mayer (wmayer@users.sourceforge.net)
    Licence: LGPL
    """

    Radius: float = ...
    """The radius of the sphere."""

    Area: Final[float] = 0.0
    """Compute the area of the sphere."""

    Volume: Final[float] = 0.0
    """Compute the volume of the sphere."""

    Center: Vector = ...
    """Center of the sphere."""

    Axis: AxisPy = ...
    """The axis direction of the circle"""
