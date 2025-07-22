from Base.Metadata import export
from Base.Type import Type
from TrimmedCurve import TrimmedCurve
from Point import Point
from typing import Final, overload


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

    Part.LineSegment(Point1,Point2)
        Creates a line segment that goes through two given points
    """

    StartPoint: Type = ...
    """Returns the start point of this line."""

    EndPoint: Type = ...
    """Returns the end point point of this line."""

    @overload
    def __init__(self) -> None: ...

    @overload
    def __init__(self, line_segment: "LineSegment") -> None: ...

    @overload
    def __init__(self, point1: Point, point2: Point) -> None: ...

    def setParameterRange(self) -> None:
        """
        Set the parameter range of the underlying line geometry
        """
        ...
