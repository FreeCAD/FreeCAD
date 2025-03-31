from Base.Metadata import (
    export,
    constmethod,
    forward_declarations,
    class_declarations,
    sequence_protocol,
)
from GeometrySurface import GeometrySurface
from typing import Final, Tuple


@export(
    Twin="GeomBezierSurface",
    TwinPointer="GeomBezierSurface",
    PythonName="Part.BezierSurface",
    FatherInclude="Mod/Part/App/GeometrySurfacePy.h",
    Include="Mod/Part/App/Geometry.h",
    Constructor=True,
)
class BezierSurface(GeometrySurface):
    """
    Describes a rational or non-rational Bezier surface
    -- A non-rational Bezier surface is defined by a table of poles (also known as control points).
    -- A rational Bezier surface is defined by a table of poles with varying associated weights.
    """

    UDegree: Final[int]
    """
    Returns the polynomial degree in u direction of this Bezier surface,
    which is equal to the number of poles minus 1.
    """

    VDegree: Final[int]
    """
    Returns the polynomial degree in v direction of this Bezier surface,
    which is equal to the number of poles minus 1.
    """
    
    MaxDegree: Final[int]
    """
    Returns the value of the maximum polynomial degree of any
    Bezier surface. This value is 25.
    """

    NbUPoles: Final[int]
    """Returns the number of poles in u direction of this Bezier surface."""

    NbVPoles: Final[int]
    """Returns the number of poles in v direction of this Bezier surface."""

    @constmethod
    def bounds(self) -> Tuple[float, float, float, float]:
        """
        Returns the parametric bounds (U1, U2, V1, V2) of this Bezier surface.
        """
        ...

    @constmethod
    def isURational(self) -> bool:
        """
        Returns false if the equation of this Bezier surface is polynomial
        (e.g. non-rational) in the u or v parametric direction.
        In other words, returns false if for each row of poles, the associated
        weights are identical.
        """
        ...

    @constmethod
    def isVRational(self) -> bool:
        """
        Returns false if the equation of this Bezier surface is polynomial
        (e.g. non-rational) in the u or v parametric direction.
        In other words, returns false if for each column of poles, the associated
        weights are identical.
        """
        ...

    @constmethod
    def isUPeriodic(self) -> bool:
        """
        Returns false.
        """
        ...

    @constmethod
    def isVPeriodic(self) -> bool:
        """
        Returns false.
        """
        ...

    @constmethod
    def isUClosed(self) -> bool:
        """
        Checks if this surface is closed in the u parametric direction.
        Returns true if, in the table of poles the first row and the last
        row are identical.
        """
        ...

    @constmethod
    def isVClosed(self) -> bool:
        """
        Checks if this surface is closed in the v parametric direction.
        Returns true if, in the table of poles the first column and the
        last column are identical.
        """
        ...

    def increase(self, DegreeU: int, DegreeV: int) -> None:
        """
        increase(DegreeU: int, DegreeV: int)
        Increases the degree of this Bezier surface in the two
        parametric directions.
        """
        ...

    def insertPoleColAfter(self, index: int) -> None:
        """
        Inserts into the table of poles of this surface, after the column
        of poles of index.
        If this Bezier surface is non-rational, it can become rational if
        the weights associated with the new poles are different from each
        other, or collectively different from the existing weights in the
        table.
        """
        ...

    def insertPoleRowAfter(self, index: int) -> None:
        """
        Inserts into the table of poles of this surface, after the row
        of poles of index.
        If this Bezier surface is non-rational, it can become rational if
        the weights associated with the new poles are different from each
        other, or collectively different from the existing weights in the
        table.
        """
        ...

    def insertPoleColBefore(self, index: int) -> None:
        """
        Inserts into the table of poles of this surface, before the column
        of poles of index.
        If this Bezier surface is non-rational, it can become rational if
        the weights associated with the new poles are different from each
        other, or collectively different from the existing weights in the
        table.
        """
        ...

    def insertPoleRowBefore(self, index: int) -> None:
        """
        Inserts into the table of poles of this surface, before the row
        of poles of index.
        If this Bezier surface is non-rational, it can become rational if
        the weights associated with the new poles are different from each
        other, or collectively different from the existing weights in the
        table.
        """
        ...

    def removePoleCol(self, VIndex: int) -> None:
        """
        removePoleRow(VIndex: int)
        Removes the column of poles of index VIndex from the table of
        poles of this Bezier surface.
        If this Bezier curve is rational, it can become non-rational.
        """
        ...

    def removePoleRow(self, UIndex: int) -> None:
        """
        removePoleRow(UIndex: int)
        Removes the row of poles of index UIndex from the table of
        poles of this Bezier surface.
        If this Bezier curve is rational, it can become non-rational.
        """
        ...

    def segment(self, U1: float, U2: float, V1: float, V2: float) -> None:
        """
        segment(U1: double, U2: double, V1: double, V2: double)
        Modifies this Bezier surface by segmenting it between U1 and U2
        in the u parametric direction, and between V1 and V2 in the v
        parametric direction.
        U1, U2, V1, and V2 can be outside the bounds of this surface.

        -- U1 and U2 isoparametric Bezier curves, segmented between
            V1 and V2, become the two bounds of the surface in the v
            parametric direction (0. and 1. u isoparametric curves).
        -- V1 and V2 isoparametric Bezier curves, segmented between
            U1 and U2, become the two bounds of the surface in the u
            parametric direction (0. and 1. v isoparametric curves).

        The poles and weights tables are modified, but the degree of
        this surface in the u and v parametric directions does not
        change.U1 can be greater than U2, and V1 can be greater than V2.
        In these cases, the corresponding parametric direction is inverted.
        The orientation of the surface is inverted if one (and only one)
        parametric direction is inverted.
        """
        ...

    def setPole(self, pole: Any) -> None:
        """
        Set a pole of the Bezier surface.
        """
        ...

    def setPoleCol(self, poles: Any) -> None:
        """
        Set the column of poles of the Bezier surface.
        """
        ...

    def setPoleRow(self, poles: Any) -> None:
        """
        Set the row of poles of the Bezier surface.
        """
        ...

    @constmethod
    def getPole(self, UIndex: int, VIndex: int) -> Any:
        """
        Get a pole of index (UIndex, VIndex) of the Bezier surface.
        """
        ...

    @constmethod
    def getPoles(self) -> Any:
        """
        Get all poles of the Bezier surface.
        """
        ...

    def setWeight(self, UIndex: int, VIndex: int, weight: float) -> None:
        """
        Set the weight of pole of the index (UIndex, VIndex)
        for the Bezier surface.
        """
        ...

    def setWeightCol(self, VIndex: int, weights: Any) -> None:
        """
        Set the weights of the poles in the column of poles
        of index VIndex of the Bezier surface.
        """
        ...

    def setWeightRow(self, UIndex: int, weights: Any) -> None:
        """
        Set the weights of the poles in the row of poles
        of index UIndex of the Bezier surface.
        """
        ...

    @constmethod
    def getWeight(self, UIndex: int, VIndex: int) -> float:
        """
        Get a weight of the pole of index (UIndex, VIndex)
        of the Bezier surface.
        """
        ...

    @constmethod
    def getWeights(self) -> Any:
        """
        Get all weights of the Bezier surface.
        """
        ...

    @constmethod
    def getResolution(self, Tolerance3D: float) -> Tuple[float, float]:
        """
        Computes two tolerance values for this Bezier surface, based on the
        given tolerance in 3D space Tolerance3D. The tolerances computed are:
        -- UTolerance in the u parametric direction and
        -- VTolerance in the v parametric direction.

        If f(u,v) is the equation of this Bezier surface, UTolerance and VTolerance
        guarantee that:
        |u1 - u0| < UTolerance
        |v1 - v0| < VTolerance
        ====> ||f(u1, v1) - f(u2, v2)|| < Tolerance3D
        """
        ...

    def exchangeUV(self) -> None:
        """
        Exchanges the u and v parametric directions on this Bezier surface.
        As a consequence:
        -- the poles and weights tables are transposed,
        -- degrees, rational characteristics and so on are exchanged between
           the two parametric directions, and
        -- the orientation of the surface is reversed.
        """
        ...
