# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Metadata import export
from typing import overload, Tuple
from Part.Geom2d import Conic2d

@export(
    PythonName="Part.Geom2d.Circle2d",
    Twin="Geom2dCircle",
    TwinPointer="Geom2dCircle",
    Include="Mod/Part/App/Geometry2d.h",
    FatherInclude="Mod/Part/App/Geom2d/Conic2dPy.h",
    Constructor=True,
)
class Circle2d(Conic2d):
    """
    Describes a circle in 3D space
    To create a circle there are several ways:
    Part.Geom2d.Circle2d()
        Creates a default circle with center (0,0) and radius 1

    Part.Geom2d.Circle2d(circle)
        Creates a copy of the given circle

    Part.Geom2d.Circle2d(circle, Distance)
        Creates a circle parallel to given circle at a certain distance

    Part.Geom2d.Circle2d(Center,Radius)
        Creates a circle defined by center and radius

    Part.Geom2d.Circle2d(Point1,Point2,Point3)
        Creates a circle defined by three non-linear points

    Author: Werner Mayer (wmayer@users.sourceforge.net)
    Licence: LGPL
    """

    Radius: float = ...
    """The radius of the circle."""

    @overload
    def __init__(self) -> None: ...
    @overload
    def __init__(self, circle: "Circle2d") -> None: ...
    @overload
    def __init__(self, circle: "Circle2d", Distance: float) -> None: ...
    @overload
    def __init__(self, Center: Tuple[float, float], Radius: float) -> None: ...
    @overload
    def __init__(
        self, Point1: Tuple[float, float], Point2: Tuple[float, float], Point3: Tuple[float, float]
    ) -> None: ...
    @overload
    def __init__(self, *args, **kwargs) -> None:
        """
        Describes a circle in 3D space
        To create a circle there are several ways:
        Part.Geom2d.Circle2d()
            Creates a default circle with center (0,0) and radius 1

        Part.Geom2d.Circle2d(circle)
            Creates a copy of the given circle

        Part.Geom2d.Circle2d(circle, Distance)
            Creates a circle parallel to given circle at a certain distance

        Part.Geom2d.Circle2d(Center,Radius)
            Creates a circle defined by center and radius

        Part.Geom2d.Circle2d(Point1,Point2,Point3)
            Creates a circle defined by three non-linear points
        """
        ...

    @staticmethod
    def getCircleCenter() -> Tuple[float, float]:
        """
        Get the circle center defined by three points
        """
        ...
