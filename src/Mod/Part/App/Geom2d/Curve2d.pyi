from Base.Metadata import export, constmethod
from Base.Vector import Vector
from Part.App.Geom2d.Geometry2d import Geometry2d
from Part.App.Geom2d.BSplineCurve import BSplineCurve
from typing import Final, overload, List

@export(
    Include="Mod/Part/App/Geometry2d.h",
    FatherInclude="Mod/Part/App/Geom2d/Geometry2dPy.h",
    Twin="Geom2dCurve",
    TwinPointer="Geom2dCurve",
    PythonName="Part.Geom2d.Curve2d",
    Constructor=True,
)
class Curve2d(Geometry2d):
    """
    The abstract class Geom2dCurve is the root class of all curve objects.
    Author: Werner Mayer (wmayer@users.sourceforge.net)
    Licence: LGPL
    """

    Continuity: Final[str] = ...
    """Returns the global continuity of the curve."""

    Closed: Final[bool] = ...
    """Returns true if the curve is closed."""

    Periodic: Final[bool] = ...
    """Returns true if the curve is periodic."""

    FirstParameter: Final[float] = ...
    """Returns the value of the first parameter."""

    LastParameter: Final[float] = ...
    """Returns the value of the last parameter."""

    def reverse(self) -> None:
        """
        Changes the direction of parametrization of the curve.
        """
        ...

    @constmethod
    def toShape(self) -> object:
        """
        Return the shape for the geometry.
        """
        ...

    @overload
    @constmethod
    def discretize(self, *, Number: int) -> List[Vector]:
        ...
    
    @overload
    @constmethod
    def discretize(self, *, QuasiNumber: int) -> List[Vector]:
        ...
    
    @overload
    @constmethod
    def discretize(self, *, Distance: float) -> List[Vector]:
        ...
    
    @overload
    @constmethod
    def discretize(self, *, Deflection: float) -> List[Vector]:
        ...
    
    @overload
    @constmethod
    def discretize(self, *, QuasiDeflection: float) -> List[Vector]:
        ...
    
    @overload
    @constmethod
    def discretize(self, *, Angular: float, Curvature: float, Minimum: int = 2) -> List[Vector]:
        ...
    
    @constmethod
    def discretize(self, **kwargs) -> List[Vector]:
        """
        Discretizes the curve and returns a list of points.
        The function accepts keywords as argument:
        discretize(Number=n) => gives a list of 'n' equidistant points.
        discretize(QuasiNumber=n) => gives a list of 'n' quasi-equidistant points (is faster than the method above).
        discretize(Distance=d) => gives a list of equidistant points with distance 'd'.
        discretize(Deflection=d) => gives a list of points with a maximum deflection 'd' to the curve.
        discretize(QuasiDeflection=d) => gives a list of points with a maximum deflection 'd' to the curve (faster).
        discretize(Angular=a,Curvature=c,[Minimum=m]) => gives a list of points with an angular deflection of 'a'
            and a curvature deflection of 'c'. Optionally a minimum number of points
            can be set, which by default is set to 2.

        Optionally you can set the keywords 'First' and 'Last' to define
            a sub-range of the parameter range of the curve.

        If no keyword is given, then it depends on whether the argument is an int or float.
        If it's an int then the behaviour is as if using the keyword 'Number',
        if it's a float then the behaviour is as if using the keyword 'Distance'.

        Example:

        import Part
        c=PartGeom2d.Circle2d()
        c.Radius=5
        p=c.discretize(Number=50,First=3.14)
        s=Part.Compound([Part.Vertex(i) for i in p])
        Part.show(s)


        p=c.discretize(Angular=0.09,Curvature=0.01,Last=3.14,Minimum=100)
        s=Part.Compound([Part.Vertex(i) for i in p])
        Part.show(s)
        """
        ...

    @overload
    def length(self) -> float:
        ...
    
    @overload
    def length(self, uMin: float) -> float:
        ...
    
    @overload
    def length(self, uMin: float, uMax: float) -> float:
        ...
    
    @overload
    def length(self, uMin: float, uMax: float, Tol: float) -> float:
        ...
    
    def length(self, *args: float) -> float:
        """
        Computes the length of a curve
        length([uMin,uMax,Tol]) -> Float
        """
        ...

    @overload
    def parameterAtDistance(self, abscissa: float) -> float:
        ...
    
    @overload
    def parameterAtDistance(self, abscissa: float, startingParameter: float) -> float:
        ...
    
    def parameterAtDistance(self, *args: float) -> float:
        """
        Returns the parameter on the curve of a point at
        the given distance from a starting parameter.
        parameterAtDistance([abscissa, startingParameter]) -> Float
        """
        ...

    def value(self, u: float) -> Vector:
        """
        Computes the point of parameter u on this curve
        """
        ...

    def tangent(self, u: float) -> Vector:
        """
        Computes the tangent of parameter u on this curve
        """
        ...

    def parameter(self, point: Vector) -> float:
        """
        Returns the parameter on the curve of the
        nearest orthogonal projection of the point.
        """
        ...

    @constmethod
    def normal(self, pos: float) -> Vector:
        """
        Vector = normal(pos) - Get the normal vector at the given parameter [First|Last] if defined.
        """
        ...

    @constmethod
    def curvature(self, pos: float) -> float:
        """
        Float = curvature(pos) - Get the curvature at the given parameter [First|Last] if defined.
        """
        ...

    @constmethod
    def centerOfCurvature(self, pos: float) -> Vector:
        """
        Vector = centerOfCurvature(float pos) - Get the center of curvature at the given parameter [First|Last] if defined.
        """
        ...

    @constmethod
    def intersectCC(self, other: "Curve2d") -> List[Vector]:
        """
        Returns all intersection points between this curve and the given curve.
        """
        ...

    @overload
    def toBSpline(self) -> BSplineCurve:
        ...
    
    @overload
    def toBSpline(self, First: float, Last: float) -> BSplineCurve:
        ...
    
    def toBSpline(self, *args: float) -> BSplineCurve:
        """
        Converts a curve of any type (only part from First to Last)
        toBSpline([Float=First, Float=Last]) -> B-Spline curve
        """
        ...

    def approximateBSpline(self, Tolerance: float, MaxSegments: int, MaxDegree: int, Order: str = "C2") -> BSplineCurve:
        """
        Approximates a curve of any type to a B-Spline curve
        approximateBSpline(Tolerance, MaxSegments, MaxDegree, [Order='C2']) -> B-Spline curve
        """
        ...