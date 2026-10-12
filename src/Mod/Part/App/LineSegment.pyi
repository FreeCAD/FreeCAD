# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Base.Metadata import export
from Base.Type import Type
from Base.Vector import Vector
from TrimmedCurve import TrimmedCurve
from Line import Line
from typing import overload

@export(
    PythonName="Part.LineSegment",
    Twin="GeomLineSegment",
    TwinPointer="GeomLineSegment",
    Include="Mod/Part/App/Geometry.h",
    FatherInclude="Mod/Part/App/TrimmedCurvePy.h",
    Constructor=True,
)
class LineSegment(TrimmedCurve):
    """
    Describes a line segment
    To create a line segment there are several ways:
    Part.LineSegment()
        Creates a default line segment

    Part.LineSegment(LineSegment)
        Creates a copy of the given line segment

    Part.LineSegment(Vector1,Vector2)
        Creates a line segment that goes through two given points
    """

    StartPoint: Type = ...
    """Returns the start point of this line."""

    EndPoint: Type = ...
    """Returns the end point point of this line."""

    @overload
    def __init__(self) -> None: ...
    @overload
    def __init__(self, line_segment: "LineSegment", /) -> None: ...
    @overload
    def __init__(self, line_segment: "LineSegment", first: float, last: float, /) -> None: ...
    @overload
    def __init__(self, line: Line, first: float, last: float, /) -> None: ...
    @overload
    def __init__(self, point1: Vector, point2: Vector, /) -> None: ...
    def setParameterRange(self, first: float, last: float, /) -> None:
        """
        Set the parameter range of the underlying line geometry
        """
        ...
