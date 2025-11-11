# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Base.Metadata import export
from Base.Vector import Vector
from GeometryCurve import GeometryCurve
from typing import overload

@export(
    PythonName="Part.Line",
    Twin="GeomLine",
    TwinPointer="GeomLine",
    Include="Mod/Part/App/Geometry.h",
    FatherInclude="Mod/Part/App/GeometryCurvePy.h",
    Constructor=True,
)
class Line(GeometryCurve):
    """
    Describes an infinite line
    To create a line there are several ways:
    Part.Line()
        Creates a default line

    Part.Line(Line)
        Creates a copy of the given line

    Part.Line(Point1,Point2)
        Creates a line that goes through two given points

    Author: Werner Mayer (wmayer@users.sourceforge.net)
    Licence: LGPL
    """

    Location: Vector = ...
    """Returns the location of this line."""

    Direction: Vector = ...
    """Returns the direction of this line."""

    @overload
    def __init__(self) -> None: ...
    @overload
    def __init__(self, line: "Line") -> None: ...
    @overload
    def __init__(self, point1: Vector, point2: Vector) -> None: ...
