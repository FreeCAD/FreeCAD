from Base.Metadata import export, constmethod
from typing import Final, overload
from Part.Geom2d import Curve2d

@export(
    PythonName="Part.Geom2d.Line2dSegment",
    Twin="Geom2dLineSegment",
    TwinPointer="Geom2dLineSegment",
    Include="Mod/Part/App/Geometry2d.h",
    FatherInclude="Mod/Part/App/Geom2d/Curve2dPy.h",
    Constructor=True,
)
class Line2dSegment(Curve2d):
    """
    Describes a line segment in 2D space.

To create a line there are several ways:
Part.Geom2d.Line2dSegment()
    Creates a default line

Part.Geom2d.Line2dSegment(Line)
    Creates a copy of the given line

Part.Geom2d.Line2dSegment(Point1,Point2)
    Creates a line that goes through two given points.
    """

    StartPoint: object = ...
    """Returns the start point of this line segment."""
    
    EndPoint: object = ...
    """Returns the end point of this line segment."""

    @overload
    def __init__(self) -> None: ...
    
    @overload
    def __init__(self, Line: "Line2dSegment") -> None: ...
    
    @overload
    def __init__(self, Point1: object, Point2: object) -> None: ...

    def setParameterRange(self) -> None:
        """
        Set the parameter range of the underlying line segment geometry.
        """
        ...
