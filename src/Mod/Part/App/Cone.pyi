# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Base.Metadata import export
from Base.Vector import Vector
from Base.Axis import Axis as AxisPy
from GeometrySurface import GeometrySurface
from typing import Final

@export(
    PythonName="Part.Cone",
    Twin="GeomCone",
    TwinPointer="GeomCone",
    Include="Mod/Part/App/Geometry.h",
    FatherInclude="Mod/Part/App/GeometrySurfacePy.h",
    Constructor=True,
)
class Cone(GeometrySurface):
    """
    Describes a cone in 3D space

    To create a cone there are several ways:

    Part.Cone()
        Creates a default cone with radius 1

    Part.Cone(Cone)
        Creates a copy of the given cone

    Part.Cone(Cone, Distance)
        Creates a cone parallel to given cone at a certain distance

    Part.Cone(Point1,Point2,Radius1,Radius2)
        Creates a cone defined by two points and two radii
        The axis of the cone is the line passing through
        Point1 and Poin2.
        Radius1 is the radius of the section passing through
        Point1 and Radius2 the radius of the section passing
        through Point2.

    Part.Cone(Point1,Point2,Point3,Point4)
        Creates a cone passing through three points Point1,
        Point2 and Point3.
        Its axis is defined by Point1 and Point2 and the radius of
        its base is the distance between Point3 and its axis.
        The distance between Point and the axis is the radius of
        the section passing through Point4.

    Author: Werner Mayer (wmayer@users.sourceforge.net)
    Licence: LGPL
    """

    Apex: Final[Vector] = Vector()
    """Compute the apex of the cone."""

    Radius: float = 0.0
    """The radius of the cone."""

    SemiAngle: float = 0.0
    """The semi-angle of the cone."""

    Center: Vector = Vector()
    """Center of the cone."""

    Axis: AxisPy = AxisPy()
    """The axis direction of the cone"""
