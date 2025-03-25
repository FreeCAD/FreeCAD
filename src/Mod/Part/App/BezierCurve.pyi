from Base.Metadata import export, constmethod
from Base.Vector import Vector
from BoundedCurve import BoundedCurve
from typing import Final, List


@export(
    Twin="GeomBezierCurve",
    TwinPointer="GeomBezierCurve",
    PythonName="Part.BezierCurve",
    FatherInclude="Mod/Part/App/BoundedCurvePy.h",
    Include="Mod/Part/App/Geometry.h",
    Constructor=True,
)
class BezierCurve(BoundedCurve):
    """
    Describes a rational or non-rational Bezier curve:
    -- a non-rational Bezier curve is defined by a table of poles (also called control points)
    -- a rational Bezier curve is defined by a table of poles with varying weights

    Constructor takes no arguments.

    Example usage:
        p1 = Base.Vector(-1, 0, 0)
        p2 = Base.Vector(0, 1, 0.2)
        p3 = Base.Vector(1, 0, 0.4)
        p4 = Base.Vector(0, -1, 1)

        bc = BezierCurve()
        bc.setPoles([p1, p2, p3, p4])
        curveShape = bc.toShape()
    """

    Degree: Final[int] = 0
    """
    Returns the polynomial degree of this Bezier curve,
    which is equal to the number of poles minus 1.
    """

    MaxDegree: Final[int] = 25
    """
    Returns the value of the maximum polynomial degree of any
    Bezier curve curve. This value is 25.
    """

    NbPoles: Final[int] = 0
    """Returns the number of poles of this Bezier curve."""

    StartPoint: Final[Vector] = Vector()
    """Returns the start point of this Bezier curve."""

    EndPoint: Final[Vector] = Vector()
    """Returns the end point of this Bezier curve."""

    @constmethod
    def isRational(self) -> bool:
        """
        Returns false if the weights of all the poles of this Bezier curve are equal.
        """
        ...

    @constmethod
    def isPeriodic(self) -> bool:
        """
        Returns false.
        """
        ...

    @constmethod
    def isClosed(self) -> bool:
        """
        Returns true if the distance between the start point and end point of
        this Bezier curve is less than or equal to gp::Resolution().
        """
        ...

    def increase(self, Int: int = ...) -> None:
        """
        Increases the degree of this Bezier curve to Degree.
        As a result, the poles and weights tables are modified.
        """
        ...

    def insertPoleAfter(self, index: int) -> None:
        """
        Inserts after the pole of index.
        """
        ...

    def insertPoleBefore(self, index: int) -> None:
        """
        Inserts before the pole of index.
        """
        ...

    def removePole(self, Index: int) -> None:
        """
        Removes the pole of index Index from the table of poles of this Bezier curve.
        If this Bezier curve is rational, it can become non-rational.
        """
        ...

    def segment(self) -> None:
        """
        Modifies this Bezier curve by segmenting it.
        """
        ...

    def setPole(self, pole: Vector) -> None:
        """
        Set a pole of the Bezier curve.
        """
        ...

    @constmethod
    def getPole(self, index: int) -> Vector:
        """
        Get a pole of the Bezier curve.
        """
        ...

    @constmethod
    def getPoles(self) -> List[Vector]:
        """
        Get all poles of the Bezier curve.
        """
        ...

    def setPoles(self, poles: List[Vector]) -> None:
        """
        Set the poles of the Bezier curve.

        Takes a list of 3D Base.Vector objects.
        """
        ...

    def setWeight(self, id: int, weight: float) -> None:
        """
        (id, weight) Set a weight of the Bezier curve.
        """
        ...

    @constmethod
    def getWeight(self, id: int) -> float:
        """
        Get a weight of the Bezier curve.
        """
        ...

    @constmethod
    def getWeights(self) -> List[float]:
        """
        Get all weights of the Bezier curve.
        """
        ...

    @constmethod
    def getResolution(self, Tolerance3D: float) -> float:
        """
        Computes for this Bezier curve the parametric tolerance (UTolerance)
        for a given 3D tolerance (Tolerance3D).
        If f(t) is the equation of this Bezier curve, the parametric tolerance
        ensures that:
        |t1-t0| < UTolerance =\"\"==> |f(t1)-f(t0)| < Tolerance3D
        """
        ...

    def interpolate(
        self, constraints: List[List], parameters: List[float] = ...
    ) -> None:
        """
        Interpolates a list of constraints.
        Each constraint is a list of a point and some optional derivatives
        An optional list of parameters can be passed. It must be of same size as constraint list.
        Otherwise, a simple uniform parametrization is used.
        Example :
        bezier.interpolate([[pt1, deriv11, deriv12], [pt2,], [pt3, deriv31]], [0, 0.4, 1.0])
        """
        ...
