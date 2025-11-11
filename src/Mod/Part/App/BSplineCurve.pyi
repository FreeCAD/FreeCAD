# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Base.Metadata import export, constmethod
from Base.Vector import Vector
from BoundedCurve import BoundedCurve
from Part.App.Arc import Arc
from Part.App.BezierCurve import BezierCurve
from typing import Final, List, overload, Any

@export(
    PythonName="Part.BSplineCurve",
    Twin="GeomBSplineCurve",
    TwinPointer="GeomBSplineCurve",
    Include="Mod/Part/App/Geometry.h",
    FatherInclude="Mod/Part/App/BoundedCurvePy.h",
    Constructor=True,
)
class BSplineCurve(BoundedCurve):
    """
    Describes a B-Spline curve in 3D space

    Author: Werner Mayer (wmayer@users.sourceforge.net)
    Licence: LGPL
    """

    Degree: Final[int] = 0
    """Returns the polynomial degree of this B-Spline curve."""

    MaxDegree: Final[int] = 25
    """
    Returns the value of the maximum polynomial degree of any
    B-Spline curve curve. This value is 25.
    """

    NbPoles: Final[int] = 0
    """Returns the number of poles of this B-Spline curve."""

    NbKnots: Final[int] = 0
    """Returns the number of knots of this B-Spline curve."""

    StartPoint: Final[Vector] = Vector()
    """Returns the start point of this B-Spline curve."""

    EndPoint: Final[Vector] = Vector()
    """Returns the end point of this B-Spline curve."""

    FirstUKnotIndex: Final[int] = 0
    """
    Returns the index in the knot array of the knot
    corresponding to the first or last parameter
    of this B-Spline curve.
    """

    LastUKnotIndex: Final[int] = 0
    """
    Returns the index in the knot array of the knot
    corresponding to the first or last parameter
    of this B-Spline curve.
    """

    KnotSequence: Final[List[float]] = []
    """Returns the knots sequence of this B-Spline curve."""

    @constmethod
    def __reduce__(self) -> Any:
        """
        __reduce__()
        Serialization of Part.BSplineCurve objects
        """
        ...

    @constmethod
    def isRational(self) -> bool:
        """
        Returns true if this B-Spline curve is rational.
        A B-Spline curve is rational if, at the time of construction,
        the weight table has been initialized.
        """
        ...

    @constmethod
    def isPeriodic(self) -> bool:
        """
        Returns true if this BSpline curve is periodic.
        """
        ...

    @constmethod
    def isClosed(self) -> bool:
        """
        Returns true if the distance between the start point and end point of
        this B-Spline curve is less than or equal to gp::Resolution().
        """
        ...

    def increaseDegree(self, Degree: int = ..., /) -> None:
        """
        increase(Int=Degree)
        Increases the degree of this B-Spline curve to Degree.
        As a result, the poles, weights and multiplicities tables
        are modified; the knots table is not changed. Nothing is
        done if Degree is less than or equal to the current degree.
        """
        ...

    @overload
    def increaseMultiplicity(self, index: int, mult: int, /) -> None: ...
    @overload
    def increaseMultiplicity(self, start: int, end: int, mult: int, /) -> None: ...
    def increaseMultiplicity(self, *args, **kwargs) -> None:
        """
        increaseMultiplicity(int index, int mult)
        increaseMultiplicity(int start, int end, int mult)
        Increases multiplicity of knots up to mult.

        index: the index of a knot to modify (1-based)
        start, end: index range of knots to modify.
        If mult is lower or equal to the current multiplicity nothing is done.
        If mult is higher than the degree the degree is used.
        """
        ...

    def incrementMultiplicity(self, start: int, end: int, mult: int, /) -> None:
        """
        incrementMultiplicity(int start, int end, int mult)

        Raises multiplicity of knots by mult.

        start, end: index range of knots to modify.
        """
        ...

    def insertKnot(self, u: float, mult: int = 1, tol: float = 0.0, /) -> None:
        """
        insertKnot(u, mult = 1, tol = 0.0)

        Inserts a knot value in the sequence of knots. If u is an existing knot the
        multiplicity is increased by mult.
        """
        ...

    def insertKnots(
        self,
        list_of_floats: List[float],
        list_of_ints: List[int],
        tol: float = 0.0,
        bool_add: bool = True,
        /,
    ) -> None:
        """
        insertKnots(list_of_floats, list_of_ints, tol = 0.0, bool_add = True)

        Inserts a set of knots values in the sequence of knots.

        For each u = list_of_floats[i], mult = list_of_ints[i]

        If u is an existing knot the multiplicity is increased by mult if bool_add is
        True, otherwise increased to mult.

        If u is not on the parameter range nothing is done.

        If the multiplicity is negative or null nothing is done. The new multiplicity
        is limited to the degree.

        The tolerance criterion for knots equality is the max of Epsilon(U) and ParametricTolerance.
        """
        ...

    def removeKnot(self, Index: int, M: int, tol: float, /) -> bool:
        """
        removeKnot(Index, M, tol)

        Reduces the multiplicity of the knot of index Index to M.
        If M is equal to 0, the knot is removed.
        With a modification of this type, the array of poles is also modified.
        Two different algorithms are systematically used to compute the new
        poles of the curve. If, for each pole, the distance between the pole
        calculated using the first algorithm and the same pole calculated using
        the second algorithm, is less than Tolerance, this ensures that the curve
        is not modified by more than Tolerance. Under these conditions, true is
        returned; otherwise, false is returned.

        A low tolerance is used to prevent modification of the curve.
        A high tolerance is used to 'smooth' the curve.
        """
        ...

    def segment(self, u1: float, u2: float, /) -> None:
        """
        segment(u1,u2)

        Modifies this B-Spline curve by segmenting it.
        """
        ...

    def setKnot(self, knot: float, index: int, /) -> None:
        """
        Set a knot of the B-Spline curve.
        """
        ...

    @constmethod
    def getKnot(self, index: int, /) -> float:
        """
        Get a knot of the B-Spline curve.
        """
        ...

    def setKnots(self, knots: List[float], /) -> None:
        """
        Set knots of the B-Spline curve.
        """
        ...

    @constmethod
    def getKnots(self) -> List[float]:
        """
        Get all knots of the B-Spline curve.
        """
        ...

    def setPole(self, P: Vector, Index: int, /) -> None:
        """
        Modifies this B-Spline curve by assigning P
        to the pole of index Index in the poles table.
        """
        ...

    @constmethod
    def getPole(self, Index: int, /) -> Vector:
        """
        Get a pole of the B-Spline curve.
        """
        ...

    @constmethod
    def getPoles(self) -> List[Vector]:
        """
        Get all poles of the B-Spline curve.
        """
        ...

    def setWeight(self, weight: float, index: int, /) -> None:
        """
        Set a weight of the B-Spline curve.
        """
        ...

    @constmethod
    def getWeight(self, index: int, /) -> float:
        """
        Get a weight of the B-Spline curve.
        """
        ...

    @constmethod
    def getWeights(self) -> List[float]:
        """
        Get all weights of the B-Spline curve.
        """
        ...

    @constmethod
    def getPolesAndWeights(self) -> List[float]:
        """
        Returns the table of poles and weights in homogeneous coordinates.
        """
        ...

    @constmethod
    def getResolution(self, Tolerance3D: float, /) -> float:
        """
        Computes for this B-Spline curve the parametric tolerance (UTolerance)
        for a given 3D tolerance (Tolerance3D).
        If f(t) is the equation of this B-Spline curve, the parametric tolerance
        ensures that:
        |t1-t0| < UTolerance =\"\"==> |f(t1)-f(t0)| < Tolerance3D
        """
        ...

    def movePoint(self, U: float, P: Vector, Index1: int, Index2: int, /) -> tuple[int, int]:
        """
        movePoint(U, P, Index1, Index2)

        Moves the point of parameter U of this B-Spline curve to P.
        Index1 and Index2 are the indexes in the table of poles of this B-Spline curve
        of the first and last poles designated to be moved.

        Returns: (FirstModifiedPole, LastModifiedPole). They are the indexes of the
        first and last poles which are effectively modified.
        """
        ...

    def setNotPeriodic(self) -> None:
        """
        Changes this B-Spline curve into a non-periodic curve.
        If this curve is already non-periodic, it is not modified.
        """
        ...

    def setPeriodic(self) -> None:
        """
        Changes this B-Spline curve into a periodic curve.
        """
        ...

    def setOrigin(self, Index: int, /) -> None:
        """
        Assigns the knot of index Index in the knots table
        as the origin of this periodic B-Spline curve. As a consequence,
        the knots and poles tables are modified.
        """
        ...

    @constmethod
    def getMultiplicity(self, index: int, /) -> int:
        """
        Returns the multiplicity of the knot of index
        from the knots table of this B-Spline curve.
        """
        ...

    @constmethod
    def getMultiplicities(self) -> List[int]:
        """
        Returns the multiplicities table M of the knots of this B-Spline curve.
        """
        ...

    @overload
    def approximate(
        self,
        Points: List[Vector],
        DegMin: int = 3,
        DegMax: int = 8,
        Tolerance: float = 1e-3,
        Continuity: str = "C2",
        LengthWeight: float = 0.0,
        CurvatureWeight: float = 0.0,
        TorsionWeight: float = 0.0,
        Parameters: List[float] = None,
        ParamType: str = "Uniform",
    ) -> None: ...
    def approximate(self, **kwargs) -> None:
        """
        Replaces this B-Spline curve by approximating a set of points.

        The function accepts keywords as arguments.

        approximate(Points = list_of_points)

        Optional arguments :

        DegMin = integer (3) : Minimum degree of the curve.
        DegMax = integer (8) : Maximum degree of the curve.
        Tolerance = float (1e-3) : approximating tolerance.
        Continuity = string ('C2') : Desired continuity of the curve.
        Possible values : 'C0','G1','C1','G2','C2','C3','CN'

        LengthWeight = float, CurvatureWeight = float, TorsionWeight = float
        If one of these arguments is not null, the functions approximates the
        points using variational smoothing algorithm, which tries to minimize
        additional criterium:
        LengthWeight*CurveLength + CurvatureWeight*Curvature + TorsionWeight*Torsion
                Continuity must be C0, C1(with DegMax >= 3) or C2(with DegMax >= 5).

        Parameters = list of floats : knot sequence of the approximated points.
        This argument is only used if the weights above are all null.

        ParamType = string ('Uniform','Centripetal' or 'ChordLength')
        Parameterization type. Only used if weights and Parameters above aren't specified.

        Note : Continuity of the spline defaults to C2. However, it may not be applied if
        it conflicts with other parameters ( especially DegMax ).
        """
        ...

    @overload
    @constmethod
    def getCardinalSplineTangents(self, **kwargs) -> List[Vector]: ...
    @constmethod
    def getCardinalSplineTangents(self, **kwargs) -> List[Vector]:
        """
        Compute the tangents for a Cardinal spline
        """
        ...

    @overload
    def interpolate(
        self,
        Points: List[Vector],
        PeriodicFlag: bool = False,
        Tolerance: float = 1e-6,
        Parameters: List[float] = None,
        InitialTangent: Vector = None,
        FinalTangent: Vector = None,
        Tangents: List[Vector] = None,
        TangentFlags: List[bool] = None,
    ) -> None: ...
    def interpolate(self, **kwargs) -> None:
        """
        Replaces this B-Spline curve by interpolating a set of points.

        The function accepts keywords as arguments.

        interpolate(Points = list_of_points)

        Optional arguments :

        PeriodicFlag = bool (False) : Sets the curve closed or opened.
        Tolerance = float (1e-6) : interpolating tolerance

        Parameters : knot sequence of the interpolated points.
        If not supplied, the function defaults to chord-length parameterization.
        If PeriodicFlag == True, one extra parameter must be appended.

        EndPoint Tangent constraints :

        InitialTangent = vector, FinalTangent = vector
        specify tangent vectors for starting and ending points
        of the BSpline. Either none, or both must be specified.

        Full Tangent constraints :

        Tangents = list_of_vectors, TangentFlags = list_of_bools
        Both lists must have the same length as Points list.
        Tangents specifies the tangent vector of each point in Points list.
        TangentFlags (bool) activates or deactivates the corresponding tangent.
        These arguments will be ignored if EndPoint Tangents (above) are also defined.

        Note : Continuity of the spline defaults to C2. However, if periodic, or tangents
        are supplied, the continuity will drop to C1.
        """
        ...

    def buildFromPoles(
        self,
        poles: List[Vector],
        periodic: bool = False,
        degree: int = 3,
        interpolate: bool = False,
        /,
    ) -> None:
        """
        Builds a B-Spline by a list of poles.
        arguments: poles (sequence of Base.Vector), [periodic (default is False), degree (default is 3), interpolate (default is False)]

        Examples:
        from FreeCAD import Base
        import Part
        V = Base.Vector
        poles = [V(-2, 2, 0),V(0, 2, 1),V(2, 2, 0),V(2, -2, 0),V(0, -2, 1),V(-2, -2, 0)]

        # non-periodic spline
        n=Part.BSplineCurve()
        n.buildFromPoles(poles)
        Part.show(n.toShape())

        # periodic spline
        n=Part.BSplineCurve()
        n.buildFromPoles(poles, True)
        Part.show(n.toShape())
        """
        ...

    @overload
    def buildFromPolesMultsKnots(
        self,
        poles: List[Vector],
        mults: List[int],
        knots: List[float],
        periodic: bool,
        degree: int,
        weights: List[float] = None,
        CheckRational: bool = False,
    ) -> None: ...
    def buildFromPolesMultsKnots(self, **kwargs) -> None:
        """
        Builds a B-Spline by a lists of Poles, Mults, Knots.
        arguments: poles (sequence of Base.Vector), [mults , knots, periodic, degree, weights (sequence of float), CheckRational]

        Examples:
        from FreeCAD import Base
        import Part
        V=Base.Vector
        poles=[V(-10,-10),V(10,-10),V(10,10),V(-10,10)]

        # non-periodic spline
        n=Part.BSplineCurve()
        n.buildFromPolesMultsKnots(poles,(3,1,3),(0,0.5,1),False,2)
        Part.show(n.toShape())

        # periodic spline
        p=Part.BSplineCurve()
        p.buildFromPolesMultsKnots(poles,(1,1,1,1,1),(0,0.25,0.5,0.75,1),True,2)
        Part.show(p.toShape())

        # periodic and rational spline
        r=Part.BSplineCurve()
        r.buildFromPolesMultsKnots(poles,(1,1,1,1,1),(0,0.25,0.5,0.75,1),True,2,(1,0.8,0.7,0.2))
        Part.show(r.toShape())
        """
        ...

    @constmethod
    def toBezier(self) -> List[BezierCurve]:
        """
        Build a list of Bezier splines.
        """
        ...

    @constmethod
    def toBiArcs(self, tolerance: float, /) -> List[Arc]:
        """
        Build a list of arcs and lines to approximate the B-spline.
        toBiArcs(tolerance) -> list.
        """
        ...

    def join(self, other: "BSplineCurve", /) -> None:
        """
        Build a new spline by joining this and a second spline.
        """
        ...

    def makeC1Continuous(self, tol: float = 1e-6, ang_tol: float = 1e-7, /) -> "BSplineCurve":
        """
        makeC1Continuous(tol = 1e-6, ang_tol = 1e-7)
        Reduces as far as possible the multiplicities of the knots of this BSpline
        (keeping the geometry). It returns a new BSpline, which could still be C0.
        tol is a geometrical tolerance.
        The tol_ang is angular tolerance, in radians. It sets tolerable angle mismatch
        of the tangents on the left and on the right to decide if the curve is G1 or
        not at a given point.
        """
        ...

    def scaleKnotsToBounds(self, u0: float = 0.0, u1: float = 1.0, /) -> None:
        """
        Scales the knots list to fit the specified bounds.
        The shape of the curve is not modified.
        bspline_curve.scaleKnotsToBounds(u0, u1)
        Default arguments are (0.0, 1.0)
        """
        ...
