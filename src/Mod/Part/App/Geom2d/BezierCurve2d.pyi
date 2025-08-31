from Base.Metadata import export, constmethod
from Part.Curve2d import Curve2d
from typing import Final, List

@export(
    Twin="Geom2dBezierCurve",
    TwinPointer="Geom2dBezierCurve",
    PythonName="Part.Geom2d.BezierCurve2d",
    FatherInclude="Mod/Part/App/Geom2d/Curve2dPy.h",
    Include="Mod/Part/App/Geometry2d.h",
    Constructor=True,
)
class BezierCurve2d(Curve2d):
    """
    Describes a rational or non-rational Bezier curve in 2d space:
        -- a non-rational Bezier curve is defined by a table of poles (also called control points)
        -- a rational Bezier curve is defined by a table of poles with varying weights
    
    Author: Werner Mayer (wmayer@users.sourceforge.net)
    Licence: LGPL
    """

    Degree: Final[int] = ...
    """Returns the polynomial degree of this Bezier curve, which is equal to the number of poles minus 1."""

    MaxDegree: Final[int] = ...
    """Returns the value of the maximum polynomial degree of any Bezier curve curve. This value is 25."""

    NbPoles: Final[int] = ...
    """Returns the number of poles of this Bezier curve."""

    StartPoint: Final[object] = ...
    """Returns the start point of this Bezier curve."""

    EndPoint: Final[object] = ...
    """Returns the end point of this Bezier curve."""

    def isRational(self) -> bool:
        """
        Returns false if the weights of all the poles of this Bezier curve are equal.
        """
        ...

    def isPeriodic(self) -> bool:
        """
        Returns false.
        """
        ...

    def isClosed(self) -> bool:
        """
        Returns true if the distance between the start point and end point of this Bezier curve
        is less than or equal to gp::Resolution().
        """
        ...

    def increase(self, Degree: int) -> None:
        """
        increase(Int=Degree)
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

    def removePole(self, index: int) -> None:
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

    def setPole(self, index: int, pole: object) -> None:
        """
        Set a pole of the Bezier curve.
        """
        ...

    def getPole(self, index: int) -> object:
        """
        Get a pole of the Bezier curve.
        """
        ...

    def getPoles(self) -> List[object]:
        """
        Get all poles of the Bezier curve.
        """
        ...

    def setPoles(self, poles: List[object]) -> None:
        """
        Set the poles of the Bezier curve.
        """
        ...

    def setWeight(self, index: int, weight: float) -> None:
        """
        Set a weight of the Bezier curve.
        """
        ...

    def getWeight(self, index: int) -> float:
        """
        Get a weight of the Bezier curve.
        """
        ...

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
        If f(t) is the equation of this Bezier curve,
        the parametric tolerance ensures that:
        |t1-t0| < UTolerance =\"\"==> |f(t1)-f(t0)| < Tolerance3D
        """
        ...
