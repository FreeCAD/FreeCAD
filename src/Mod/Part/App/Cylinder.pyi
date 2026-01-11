# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Base.Metadata import export
from Base.Vector import Vector
from Circle import Circle
from GeometrySurface import GeometrySurface
from typing import overload

@export(
    PythonName="Part.Cylinder",
    Twin="GeomCylinder",
    TwinPointer="GeomCylinder",
    Include="Mod/Part/App/Geometry.h",
    FatherInclude="Mod/Part/App/GeometrySurfacePy.h",
    Constructor=True,
)
class Cylinder(GeometrySurface):
    """
    Describes a cylinder in 3D space

    To create a cylinder there are several ways:

    Part.Cylinder()
        Creates a default cylinder with center (0,0,0) and radius 1

    Part.Cylinder(Cylinder)
        Creates a copy of the given cylinder

    Part.Cylinder(Cylinder, Distance)
        Creates a cylinder parallel to given cylinder at a certain distance

    Part.Cylinder(Point1, Point2, Point2)
        Creates a cylinder defined by three non-linear points

    Part.Cylinder(Circle)
        Creates a cylinder by a circular base

    Author: Werner Mayer (wmayer@users.sourceforge.net)
    Licence: LGPL
    """

    Radius: float
    """The radius of the cylinder."""

    Center: Vector
    """Center of the cylinder."""

    Axis: Vector
    """The axis direction of the cylinder"""

    @overload
    def __init__(self) -> None: ...
    @overload
    def __init__(self, cylinder: "Cylinder") -> None: ...
    @overload
    def __init__(self, cylinder: "Cylinder", distance: float) -> None: ...
    @overload
    def __init__(self, point1: Vector, point2: Vector, point3: Vector) -> None: ...
    @overload
    def __init__(self, circle: Circle) -> None: ...
