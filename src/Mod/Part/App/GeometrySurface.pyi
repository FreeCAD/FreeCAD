from Base.Metadata import export, constmethod
from Base.Rotation import Rotation as RotationPy
from Base.Vector import Vector
from Geometry import Geometry
from GeometryCurve import GeometryCurve
from Line import Line
from typing import Final, overload, Any, Tuple, List, Literal, Union


@export(
    Twin="GeomSurface",
    TwinPointer="GeomSurface",
    PythonName="Part.GeometrySurface",
    FatherInclude="Mod/Part/App/GeometryPy.h",
    Include="Mod/Part/App/Geometry.h",
    Constructor=True,
)
class GeometrySurface(Geometry):
    """
    The abstract class GeometrySurface is the root class of all surface objects.

    Author: Werner Mayer (wmayer@users.sourceforge.net)
    Licence: LGPL
    """

    Continuity: Final[str] = ""
    """Returns the global continuity of the surface."""

    Rotation: Final[RotationPy] = ...
    """Returns a rotation object to describe the orientation for surface that supports it"""

    @constmethod
    def toShape(self) -> Any:
        """
        Return the shape for the geometry.
        """
        ...

    @constmethod
    def toShell(self, *, Bounds: object, Segment: object) -> Any:
        """
        Make a shell of the surface.
        """
        ...

    @constmethod
    def getD0(self, param: float) -> Vector:
        """
        Returns the point of given parameter
        """
        ...

    @constmethod
    def getDN(self, n: int) -> Any:
        """
        Returns the n-th derivative
        """
        ...

    @constmethod
    def value(self, u: float, v: float) -> Vector:
        """
        value(u,v) -> Point
        Computes the point of parameter (u,v) on this surface
        """
        ...

    @constmethod
    def tangent(self, u: float, v: float) -> Tuple[Vector, Vector]:
        """
        tangent(u,v) -> (Vector,Vector)
        Computes the tangent of parameter (u,v) on this geometry
        """
        ...

    @constmethod
    def normal(self, u: float, v: float) -> Vector:
        """
        normal(u,v) -> Vector
        Computes the normal of parameter (u,v) on this geometry
        """
        ...

    @overload
    def projectPoint(
        self, Point: Vector, Method: Literal["NearestPoint"] = "NearestPoint"
    ) -> Vector: ...

    @overload
    def projectPoint(
        self, Point: Vector, Method: Literal["LowerDistance"]
    ) -> float: ...

    @overload
    def projectPoint(
        self, Point: Vector, Method: Literal["LowerDistanceParameters"]
    ) -> Tuple[float, float]: ...

    @overload
    def projectPoint(
        self, Point: Vector, Method: Literal["Distance"]
    ) -> List[float]: ...

    @overload
    def projectPoint(
        self, Point: Vector, Method: Literal["Parameters"]
    ) -> List[Tuple[float, float]]: ...

    @overload
    def projectPoint(self, Point: Vector, Method: Literal["Point"]) -> List[Vector]: ...

    @constmethod
    def projectPoint(self, *, Point: Vector, Method: str = ...) -> Any:
        """
        Computes the projection of a point on the surface

        projectPoint(Point=Vector,[Method="NearestPoint"])
        projectPoint(Vector,"NearestPoint") -> Vector
        projectPoint(Vector,"LowerDistance") -> float
        projectPoint(Vector,"LowerDistanceParameters") -> tuple of floats (u,v)
        projectPoint(Vector,"Distance") -> list of floats
        projectPoint(Vector,"Parameters") -> list of tuples of floats
        projectPoint(Vector,"Point") -> list of points
        """
        ...

    @constmethod
    def isUmbillic(self, u: float, v: float) -> bool:
        """
        isUmbillic(u,v) -> bool
        Check if the geometry on parameter is an umbillic point,
        i.e. maximum and minimum curvature are equal.
        """
        ...

    @constmethod
    def curvature(self, u: float, v: float, type: str) -> float:
        """
        curvature(u,v,type) -> float
        The value of type must be one of this: Max, Min, Mean or Gauss
        Computes the curvature of parameter (u,v) on this geometry
        """
        ...

    @constmethod
    def curvatureDirections(self, u: float, v: float) -> Tuple[Vector, Vector]:
        """
        curvatureDirections(u,v) -> (Vector,Vector)
        Computes the directions of maximum and minimum curvature
        of parameter (u,v) on this geometry.
        The first vector corresponds to the maximum curvature,
        the second vector corresponds to the minimum curvature.
        """
        ...

    @constmethod
    def bounds(self) -> Tuple[float, float, float, float]:
        """
        Returns the parametric bounds (U1, U2, V1, V2) of this trimmed surface.
        """
        ...

    @constmethod
    def isPlanar(self, tolerance: float = 0.0) -> bool:
        """
        isPlanar([float]) -> Bool
        Checks if the surface is planar within a certain tolerance.
        """
        ...

    @constmethod
    def uIso(self, u: Tuple) -> Union[GeometryCurve, Line]:
        """
        Builds the U isoparametric curve
        """
        ...

    @constmethod
    def vIso(self, v: Tuple) -> Union[GeometryCurve, Line]:
        """
        Builds the V isoparametric curve
        """
        ...

    @constmethod
    def isUPeriodic(self) -> bool:
        """
        Returns true if this patch is periodic in the given parametric direction.
        """
        ...

    @constmethod
    def isVPeriodic(self) -> bool:
        """
        Returns true if this patch is periodic in the given parametric direction.
        """
        ...

    @constmethod
    def isUClosed(self) -> bool:
        """
        Checks if this surface is closed in the u parametric direction.
        """
        ...

    @constmethod
    def isVClosed(self) -> bool:
        """
        Checks if this surface is closed in the v parametric direction.
        """
        ...

    @constmethod
    def UPeriod(self) -> float:
        """
        Returns the period of this patch in the u parametric direction.
        """
        ...

    @constmethod
    def VPeriod(self) -> float:
        """
        Returns the period of this patch in the v parametric direction.
        """
        ...

    @constmethod
    def parameter(self) -> float:
        """
        Returns the parameter on the curve
        of the nearest orthogonal projection of the point.
        """
        ...

    @overload
    def toBSpline(
        self,
        tolerance: float = 1e-7,
        continuity_u: Literal["C0", "G0", "G1", "C1", "G2", "C3", "CN"] = "C1",
        continuity_v: Literal["C0", "G0", "G1", "C1", "G2", "C3", "CN"] = "C1",
        max_degree_u: int = 25,
        max_degree_v: int = 25,
        max_segments: int = 1000,
        precision_code: int = 0,
    ) -> Any: ...

    @constmethod
    def toBSpline(
        self,
        *,
        tolerance: float = 1e-7,
        continuity_u: str = "C1",
        continuity_v: str = "C1",
        max_degree_u: int = 25,
        max_degree_v: int = 25,
        max_segments: int = 1000,
        precision_code: int = 0
    ) -> Any:
        """
        Returns a B-Spline representation of this surface.
        The optional arguments are:
        * tolerance (default=1e-7)
        * continuity in u (as string e.g. C0, G0, G1, C1, G2, C3, CN) (default='C1')
        * continuity in v (as string e.g. C0, G0, G1, C1, G2, C3, CN) (default='C1')
        * maximum degree in u (default=25)
        * maximum degree in v (default=25)
        * maximum number of segments (default=1000)
        * precision code (default=0)
        Will raise an exception if surface is infinite in U or V (like planes, cones or cylinders)
        """
        ...

    @constmethod
    def intersect(self) -> Any:
        """
        Returns all intersection points/curves between the surface and the curve/surface.
        """
        ...

    @constmethod
    def intersectSS(self, SecondSurface: Any, precision_code: int = 0) -> Any:
        """
        Returns all intersection curves of this surface and the given surface.
        The required arguments are:
        * Second surface
        * precision code (optional, default=0)
        """
        ...
