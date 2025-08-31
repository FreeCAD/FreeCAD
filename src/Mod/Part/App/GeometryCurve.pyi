from Base.Metadata import export, constmethod
from Base.Vector import Vector
from Base.Rotation import Rotation as RotationPy
from Geometry import Geometry
from typing import Final, overload, List, Union, Optional, Tuple


@export(
    Twin="GeomCurve",
    TwinPointer="GeomCurve",
    PythonName="Part.Curve",
    FatherInclude="Mod/Part/App/GeometryPy.h",
    Include="Mod/Part/App/Geometry.h",
    Constructor=True,
)
class GeometryCurve(Geometry):
    """
    The abstract class GeometryCurve is the root class of all curve objects.

    Author: Werner Mayer (wmayer@users.sourceforge.net)
    Licence: LGPL
    """

    Continuity: Final[str] = ""
    """Returns the global continuity of the curve."""

    FirstParameter: Final[float] = 0.0
    """Returns the value of the first parameter."""

    LastParameter: Final[float] = 0.0
    """Returns the value of the last parameter."""

    Rotation: Final[RotationPy] = ...
    """Returns a rotation object to describe the orientation for curve that supports it"""

    @constmethod
    def toShape(self) -> object:
        """
        Return the shape for the geometry.
        """
        ...

    @overload
    @constmethod
    def discretize(
        self,
        Number: int,
        *,
        First: Optional[float] = None,
        Last: Optional[float] = None
    ) -> List[Vector]:
        """
        Discretizes the curve and returns a list of points.
        """
        ...

    @overload
    @constmethod
    def discretize(
        self,
        QuasiNumber: int,
        *,
        First: Optional[float] = None,
        Last: Optional[float] = None
    ) -> List[Vector]:
        """
        Discretizes the curve and returns a list of quasi equidistant points.
        """
        ...

    @overload
    @constmethod
    def discretize(
        self,
        Distance: float,
        *,
        First: Optional[float] = None,
        Last: Optional[float] = None
    ) -> List[Vector]:
        """
        Discretizes the curve and returns a list of equidistant points with distance 'd'.
        """
        ...

    @overload
    @constmethod
    def discretize(
        self,
        Deflection: float,
        *,
        First: Optional[float] = None,
        Last: Optional[float] = None
    ) -> List[Vector]:
        """
        Discretizes the curve and returns a list of points with a maximum deflection 'd' to the curve.
        """
        ...

    @overload
    @constmethod
    def discretize(
        self,
        QuasiDeflection: float,
        *,
        First: Optional[float] = None,
        Last: Optional[float] = None
    ) -> List[Vector]:
        """
        Discretizes the curve and returns a list of points with a maximum deflection 'd' to the curve (faster).
        """
        ...

    @overload
    @constmethod
    def discretize(
        self,
        Angular: float,
        Curvature: float,
        Minimum: int = 2,
        *,
        First: Optional[float] = None,
        Last: Optional[float] = None
    ) -> List[Vector]:
        """
        Discretizes the curve and returns a list of points with an angular deflection of 'a' and a curvature deflection of 'c'.
        Optionally a minimum number of points can be set.
        """
        ...

    @constmethod
    def discretize(self, **kwargs) -> List[Vector]:
        """
        Discretizes the curve and returns a list of points.

        The function accepts keywords as argument:

        discretize(Number=n) => gives a list of 'n' equidistant points
        discretize(QuasiNumber=n) => gives a list of 'n' quasi equidistant points (is faster than the method above)
        discretize(Distance=d) => gives a list of equidistant points with distance 'd'
        discretize(Deflection=d) => gives a list of points with a maximum deflection 'd' to the curve
        discretize(QuasiDeflection=d) => gives a list of points with a maximum deflection 'd' to the curve (faster)
        discretize(Angular=a,Curvature=c,[Minimum=m]) => gives a list of points with an angular deflection of 'a'
                                            and a curvature deflection of 'c'. Optionally a minimum number of points
                                            can be set which by default is set to 2.

        Optionally you can set the keywords 'First' and 'Last' to define a sub-range of the parameter range
        of the curve.

        If no keyword is given then it depends on whether the argument is an int or float.
        If it's an int then the behaviour is as if using the keyword 'Number', if it's float
        then the behaviour is as if using the keyword 'Distance'.

        Example:

        import Part
        c=Part.Circle()
        c.Radius=5
        p=c.discretize(Number=50,First=3.14)
        s=Part.Compound([Part.Vertex(i) for i in p])
        Part.show(s)


        p=c.discretize(Angular=0.09,Curvature=0.01,Last=3.14,Minimum=100)
        s=Part.Compound([Part.Vertex(i) for i in p])
        Part.show(s)
        """
        ...

    @constmethod
    def getD0(self, parameter: float) -> Vector:
        """
        Returns the point of given parameter
        """
        ...

    @constmethod
    def getD1(self, parameter: float) -> Vector:
        """
        Returns the point and first derivative of given parameter
        """
        ...

    @constmethod
    def getD2(self, parameter: float) -> Vector:
        """
        Returns the point, first and second derivatives
        """
        ...

    @constmethod
    def getD3(self, parameter: float) -> Vector:
        """
        Returns the point, first, second and third derivatives
        """
        ...

    @constmethod
    def getDN(self, n: int, parameter: float) -> Vector:
        """
        Returns the n-th derivative
        """
        ...

    @constmethod
    def length(
        self,
        uMin: Optional[float] = None,
        uMax: Optional[float] = None,
        Tol: Optional[float] = None,
    ) -> float:
        """
        Computes the length of a curve
        length([uMin, uMax, Tol]) -> float
        """
        ...

    @constmethod
    def parameterAtDistance(
        self,
        abscissa: Optional[float] = None,
        startingParameter: Optional[float] = None,
    ) -> float:
        """
        Returns the parameter on the curve of a point at the given distance from a starting parameter.
        parameterAtDistance([abscissa, startingParameter]) -> float
        """
        ...

    @constmethod
    def value(self, u: float) -> Vector:
        """
        Computes the point of parameter u on this curve
        """
        ...

    @constmethod
    def tangent(self, u: float) -> Vector:
        """
        Computes the tangent of parameter u on this curve
        """
        ...

    @constmethod
    def makeRuledSurface(self, otherCurve: "GeometryCurve") -> object:
        """
        Make a ruled surface of this and the given curves
        """
        ...

    @constmethod
    def intersect2d(self, otherCurve: "GeometryCurve") -> List[Vector]:
        """
        Get intersection points with another curve lying on a plane.
        """
        ...

    @constmethod
    def continuityWith(self, otherCurve: "GeometryCurve") -> str:
        """
        Computes the continuity of two curves
        """
        ...

    @constmethod
    def parameter(self, point: Vector) -> float:
        """
        Returns the parameter on the curve of the nearest orthogonal projection of the point.
        """
        ...

    @constmethod
    def normal(self, pos: float) -> Vector:
        """
        Vector = normal(pos) - Get the normal vector at the given parameter [First|Last] if defined
        """
        ...

    @overload
    @constmethod
    def projectPoint(self, Point: Vector, Method: str = "NearestPoint") -> Vector:
        """
        projectPoint(Point=Vector, Method="NearestPoint") -> Vector
        """
        ...

    @overload
    @constmethod
    def projectPoint(self, Point: Vector, Method: str = "LowerDistance") -> float:
        """
        projectPoint(Vector, "LowerDistance") -> float.
        """
        ...

    @overload
    @constmethod
    def projectPoint(
        self, Point: Vector, Method: str = "LowerDistanceParameter"
    ) -> float:
        """
        projectPoint(Vector, "LowerDistanceParameter") -> float.
        """
        ...

    @overload
    @constmethod
    def projectPoint(self, Point: Vector, Method: str = "Distance") -> List[float]:
        """
        projectPoint(Vector, "Distance") -> list of floats.
        """
        ...

    @overload
    @constmethod
    def projectPoint(self, Point: Vector, Method: str = "Parameter") -> List[float]:
        """
        projectPoint(Vector, "Parameter") -> list of floats.
        """
        ...

    @overload
    @constmethod
    def projectPoint(self, Point: Vector, Method: str = "Point") -> List[Vector]:
        """
        projectPoint(Vector, "Point") -> list of points.
        """
        ...

    @constmethod
    def projectPoint(self, **kwargs) -> Union[Vector, float, List[float], List[Vector]]:
        """
        Computes the projection of a point on the curve

        projectPoint(Point=Vector,[Method="NearestPoint"])
        projectPoint(Vector,"NearestPoint") -> Vector
        projectPoint(Vector,"LowerDistance") -> float
        projectPoint(Vector,"LowerDistanceParameter") -> float
        projectPoint(Vector,"Distance") -> list of floats
        projectPoint(Vector,"Parameter") -> list of floats
        projectPoint(Vector,"Point") -> list of points
        """
        ...

    @constmethod
    def curvature(self, pos: float) -> float:
        """
        Float = curvature(pos) - Get the curvature at the given parameter [First|Last] if defined
        """
        ...

    @constmethod
    def centerOfCurvature(self, pos: float) -> Vector:
        """
        Vector = centerOfCurvature(float pos) - Get the center of curvature at the given parameter [First|Last] if defined
        """
        ...

    @constmethod
    def intersect(self, curve_or_surface: object, precision: float) -> object:
        """
        Returns all intersection points and curve segments between the curve and the curve/surface.

        arguments: curve/surface (for the intersection), precision (float)
        """
        ...

    @constmethod
    def intersectCS(self, surface: object) -> object:
        """
        Returns all intersection points and curve segments between the curve and the surface.
        """
        ...

    @constmethod
    def intersectCC(self, otherCurve: "GeometryCurve") -> List[Vector]:
        """
        Returns all intersection points between this curve and the given curve.
        """
        ...

    @constmethod
    def toBSpline(self, points: Tuple[float, float]) -> "BSplineCurve":
        """
        Converts a curve of any type (only part from First to Last) to BSpline curve.
        toBSpline((first: float, last: float)) -> BSplineCurve
        """
        ...

    @constmethod
    def toNurbs(self, points: Tuple[float, float]) -> "NurbsCurve":
        """
        Converts a curve of any type (only part from First to Last) to NURBS curve.
        toNurbs((first: float, last: float)) -> NurbsCurve
        """
        ...

    @constmethod
    def trim(self, points: Tuple[float, float]) -> "TrimmedCurve":
        """
        Returns a trimmed curve defined in the given parameter range.
        trim((first: float, last: float)) -> TrimmedCurve
        """
        ...

    @constmethod
    def approximateBSpline(
        self, Tolerance: float, MaxSegments: int, MaxDegree: int, Order: str = "C2"
    ) -> "BSplineCurve":
        """
        Approximates a curve of any type to a B-Spline curve.
        approximateBSpline(Tolerance, MaxSegments, MaxDegree, [Order='C2']) -> BSplineCurve
        """
        ...

    def reverse(self) -> None:
        """
        Changes the direction of parametrization of the curve.
        """
        ...

    @constmethod
    def reversedParameter(self, U: float) -> float:
        """
        Returns the parameter on the reversed curve for the point of parameter U on this curve.
        """
        ...

    @constmethod
    def isPeriodic(self) -> bool:
        """
        Returns true if this curve is periodic.
        """
        ...

    @constmethod
    def period(self) -> float:
        """
        Returns the period of this curve or raises an exception if it is not periodic.
        """
        ...

    @constmethod
    def isClosed(self) -> bool:
        """
        Returns true if the curve is closed.
        """
        ...
