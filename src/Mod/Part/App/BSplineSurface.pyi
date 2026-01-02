# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Base.Metadata import export, constmethod
from GeometrySurface import GeometrySurface
from typing import Final, List, Any

@export(
    PythonName="Part.BSplineSurface",
    Twin="GeomBSplineSurface",
    TwinPointer="GeomBSplineSurface",
    Include="Mod/Part/App/Geometry.h",
    FatherInclude="Mod/Part/App/GeometrySurfacePy.h",
    Constructor=True,
)
class BSplineSurface(GeometrySurface):
    """
    Describes a B-Spline surface in 3D space

    Author: Werner Mayer (wmayer@users.sourceforge.net)
    Licence: LGPL
    DeveloperDocu: Describes a B-Spline surface in 3D space
    """

    UDegree: Final[int] = 0
    """Returns the degree of this B-Spline surface in the u parametric direction."""

    VDegree: Final[int] = 0
    """Returns the degree of this B-Spline surface in the v parametric direction."""

    MaxDegree: Final[int] = 25
    """
    Returns the value of the maximum polynomial degree of any
    B-Spline surface surface in either parametric directions.
    This value is 25.
    """

    NbUPoles: Final[int] = 0
    """Returns the number of poles of this B-Spline surface in the u parametric direction."""

    NbVPoles: Final[int] = 0
    """Returns the number of poles of this B-Spline surface in the v parametric direction."""

    NbUKnots: Final[int] = 0
    """Returns the number of knots of this B-Spline surface in the u parametric direction."""

    NbVKnots: Final[int] = 0
    """Returns the number of knots of this B-Spline surface in the v parametric direction."""

    FirstUKnotIndex: Final[Any] = None
    """
    Returns the index in the knot array associated with the u parametric direction,
    which corresponds to the first parameter of this B-Spline surface in the specified
    parametric direction.

    The isoparametric curves corresponding to these values are the boundary curves of
    this surface.

    Note: The index does not correspond to the first knot of the surface in the specified
    parametric direction unless the multiplicity of the first knot is equal to Degree + 1,
    where Degree is the degree of this surface in the corresponding parametric direction.
    """

    LastUKnotIndex: Final[Any] = None
    """
    Returns the index in the knot array associated with the u parametric direction,
    which corresponds to the last parameter of this B-Spline surface in the specified
    parametric direction.

    The isoparametric curves corresponding to these values are the boundary curves of
    this surface.

    Note: The index does not correspond to the first knot of the surface in the specified
    parametric direction unless the multiplicity of the last knot is equal to Degree + 1,
    where Degree is the degree of this surface in the corresponding parametric direction.
    """

    FirstVKnotIndex: Final[Any] = None
    """
    Returns the index in the knot array associated with the v parametric direction,
    which corresponds to the first parameter of this B-Spline surface in the specified
    parametric direction.

    The isoparametric curves corresponding to these values are the boundary curves of
    this surface.

    Note: The index does not correspond to the first knot of the surface in the specified
    parametric direction unless the multiplicity of the first knot is equal to Degree + 1,
    where Degree is the degree of this surface in the corresponding parametric direction.
    """

    LastVKnotIndex: Final[Any] = None
    """
    Returns the index in the knot array associated with the v parametric direction,
    which corresponds to the last parameter of this B-Spline surface in the specified
    parametric direction.

    The isoparametric curves corresponding to these values are the boundary curves of
    this surface.

    Note: The index does not correspond to the first knot of the surface in the specified
    parametric direction unless the multiplicity of the last knot is equal to Degree + 1,
    where Degree is the degree of this surface in the corresponding parametric direction.
    """

    UKnotSequence: Final[List[Any]] = []
    """
    Returns the knots sequence of this B-Spline surface in
    the u direction.
    """

    VKnotSequence: Final[List[Any]] = []
    """
    Returns the knots sequence of this B-Spline surface in
    the v direction.
    """

    @constmethod
    def bounds(self) -> Any:
        """
        Returns the parametric bounds (U1, U2, V1, V2) of this B-Spline surface.
        """
        ...

    @constmethod
    def isURational(self) -> Any:
        """
        Returns false if the equation of this B-Spline surface is polynomial
        (e.g. non-rational) in the u or v parametric direction.
        In other words, returns false if for each row of poles, the associated
        weights are identical
        """
        ...

    @constmethod
    def isVRational(self) -> Any:
        """
        Returns false if the equation of this B-Spline surface is polynomial
        (e.g. non-rational) in the u or v parametric direction.
        In other words, returns false if for each column of poles, the associated
        weights are identical
        """
        ...

    @constmethod
    def isUPeriodic(self) -> Any:
        """
        Returns true if this surface is periodic in the u parametric direction.
        """
        ...

    @constmethod
    def isVPeriodic(self) -> Any:
        """
        Returns true if this surface is periodic in the v parametric direction.
        """
        ...

    @constmethod
    def isUClosed(self) -> Any:
        """
        Checks if this surface is closed in the u parametric direction.
        Returns true if, in the table of poles the first row and the last
        row are identical.
        """
        ...

    @constmethod
    def isVClosed(self) -> Any:
        """
        Checks if this surface is closed in the v parametric direction.
        Returns true if, in the table of poles the first column and the
        last column are identical.
        """
        ...

    def increaseDegree(
        self,
        DegMin: int,
        DegMax: int,
        Continuity: int,
        Tolerance: float,
        X0: float = ...,
        dX: float = ...,
        Y0: float = ...,
        dY: float = ...,
        /,
    ) -> None:
        """
        increase(Int=UDegree, int=VDegree)
        Increases the degrees of this B-Spline surface to UDegree and VDegree
        in the u and v parametric directions respectively.
        As a result, the tables of poles, weights and multiplicities are modified.
        The tables of knots is not changed.

        Note: Nothing is done if the given degree is less than or equal to the
        current degree in the corresponding parametric direction.
        """
        ...

    def increaseUMultiplicity(self) -> None:
        """
        Increases the multiplicity in the u direction.
        """
        ...

    def increaseVMultiplicity(self) -> None:
        """
        Increases the multiplicity in the v direction.
        """
        ...

    def incrementUMultiplicity(self) -> None:
        """
        Increment the multiplicity in the u direction
        """
        ...

    def incrementVMultiplicity(self) -> None:
        """
        Increment the multiplicity in the v direction
        """
        ...

    def insertUKnot(self, U: float, Index: int, Tolerance: float, /) -> None:
        """
        insertUKnote(float U, int Index, float Tolerance) - Insert or override a knot
        """
        ...

    def insertUKnots(self, U: List[float], Mult: List[float], Tolerance: float, /) -> None:
        """
        insertUKnote(List of float U, List of float Mult, float Tolerance) - Inserts knots.
        """
        ...

    def insertVKnot(self, V: float, Index: int, Tolerance: float, /) -> None:
        """
        insertUKnote(float V, int Index, float Tolerance) - Insert or override a knot.
        """
        ...

    def insertVKnots(self, V: List[float], Mult: List[float], Tolerance: float, /) -> None:
        """
        insertUKnote(List of float V, List of float Mult, float Tolerance) - Inserts knots.
        """
        ...

    def removeUKnot(self, M: int, Index: int, Tolerance: float, /) -> bool:
        """
        Reduces to M the multiplicity of the knot of index Index in the given
        parametric direction. If M is 0, the knot is removed.
        With a modification of this type, the table of poles is also modified.
        Two different algorithms are used systematically to compute the new
        poles of the surface. For each pole, the distance between the pole
        calculated using the first algorithm and the same pole calculated using
        the second algorithm, is checked. If this distance is less than Tolerance
        it ensures that the surface is not modified by more than Tolerance.
        Under these conditions, the function returns true; otherwise, it returns
        false.

        A low tolerance prevents modification of the surface. A high tolerance
        'smoothes' the surface.
        """
        ...

    def removeVKnot(self, M: int, Index: int, Tolerance: float, /) -> bool:
        """
        Reduces to M the multiplicity of the knot of index Index in the given
        parametric direction. If M is 0, the knot is removed.
        With a modification of this type, the table of poles is also modified.
        Two different algorithms are used systematically to compute the new
        poles of the surface. For each pole, the distance between the pole
        calculated using the first algorithm and the same pole calculated using
        the second algorithm, is checked. If this distance is less than Tolerance
        it ensures that the surface is not modified by more than Tolerance.
        Under these conditions, the function returns true; otherwise, it returns
        false.

        A low tolerance prevents modification of the surface. A high tolerance
        'smoothes' the surface.
        """
        ...

    def segment(self, U1: float, U2: float, V1: float, V2: float, /) -> None:
        """
        Modifies this B-Spline surface by segmenting it between U1 and U2 in the
        u parametric direction and between V1 and V2 in the v parametric direction.
        Any of these values can be outside the bounds of this surface, but U2 must
        be greater than U1 and V2 must be greater than V1.

        All the data structure tables of this B-Spline surface are modified but the
        knots located between U1 and U2 in the u parametric direction, and between
        V1 and V2 in the v parametric direction are retained.
        The degree of the surface in each parametric direction is not modified.
        """
        ...

    def setUKnot(self, K: float, UIndex: int, M: int = ..., /) -> None:
        """
        Modifies this B-Spline surface by assigning the value K to the knot of index
        UIndex of the knots table corresponding to the u parametric direction.
        This modification remains relatively local, since K must lie between the values
        of the knots which frame the modified knot.

        You can also increase the multiplicity of the modified knot to M. Note however
        that it is not possible to decrease the multiplicity of a knot with this function.
        """
        ...

    def setVKnot(self, K: float, VIndex: int, M: int = ..., /) -> None:
        """
        Modifies this B-Spline surface by assigning the value K to the knot of index
        VIndex of the knots table corresponding to the v parametric direction.
        This modification remains relatively local, since K must lie between the values
        of the knots which frame the modified knot.

        You can also increase the multiplicity of the modified knot to M. Note however
        that it is not possible to decrease the multiplicity of a knot with this function.
        """
        ...

    @constmethod
    def getUKnot(self, UIndex: int, /) -> Any:
        """
        Returns, for this B-Spline surface, in the u parametric direction
        the knot of index UIndex of the knots table.
        """
        ...

    @constmethod
    def getVKnot(self, VIndex: int, /) -> Any:
        """
        Returns, for this B-Spline surface, in the v parametric direction
        the knot of index VIndex of the knots table.
        """
        ...

    def setUKnots(self, knots: List[Any], /) -> None:
        """
        Changes all knots of this B-Spline surface in the u parametric
        direction. The multiplicity of the knots is not modified.
        """
        ...

    def setVKnots(self, knots: List[Any], /) -> None:
        """
        Changes all knots of this B-Spline surface in the v parametric
        direction. The multiplicity of the knots is not modified.
        """
        ...

    @constmethod
    def getUKnots(self) -> List[Any]:
        """
        Returns, for this B-Spline surface, the knots table
        in the u parametric direction
        """
        ...

    @constmethod
    def getVKnots(self) -> List[Any]:
        """
        Returns, for this B-Spline surface, the knots table
        in the v parametric direction
        """
        ...

    def setPole(self, P: Any, UIndex: int, VIndex: int, Weight: float = ..., /) -> None:
        """
        Modifies this B-Spline surface by assigning P to the pole of
        index (UIndex, VIndex) in the poles table.
        The second syntax allows you also to change the weight of the
        modified pole. The weight is set to Weight. This syntax must
        only be used for rational surfaces.
        Modifies this B-Spline curve by assigning P to the pole of
        index Index in the poles table.
        """
        ...

    def setPoleCol(self, VIndex: int, values: List[Any], CPoleWeights: List[float], /) -> None:
        """
        Modifies this B-Spline surface by assigning values to all or part
        of the column of poles of index VIndex, of this B-Spline surface.
        You can also change the weights of the modified poles. The weights
        are set to the corresponding values of CPoleWeights.
        These syntaxes must only be used for rational surfaces.
        """
        ...

    def setPoleRow(self, UIndex: int, values: List[Any], CPoleWeights: List[float], /) -> None:
        """
        Modifies this B-Spline surface by assigning values to all or part
        of the row of poles of index UIndex, of this B-Spline surface.
        You can also change the weights of the modified poles. The weights
        are set to the corresponding values of CPoleWeights.
        These syntaxes must only be used for rational surfaces.
        """
        ...

    @constmethod
    def getPole(self, UIndex: int, VIndex: int, /) -> Any:
        """
        Returns the pole of index (UIndex,VIndex) of this B-Spline surface.
        """
        ...

    @constmethod
    def getPoles(self) -> List[Any]:
        """
        Returns the table of poles of this B-Spline surface.
        """
        ...

    def setWeight(self, Weight: float, UIndex: int, VIndex: int, /) -> None:
        """
        Modifies this B-Spline surface by assigning the value Weight to the weight
        of the pole of index (UIndex, VIndex) in the poles tables of this B-Spline
        surface.

        This function must only be used for rational surfaces.
        """
        ...

    def setWeightCol(self, VIndex: int, CPoleWeights: List[float], /) -> None:
        """
        Modifies this B-Spline surface by assigning values to all or part of the
        weights of the column of poles of index VIndex of this B-Spline surface.

        The modified part of the column of weights is defined by the bounds
        of the array CPoleWeights.

        This function must only be used for rational surfaces.
        """
        ...

    def setWeightRow(self, UIndex: int, CPoleWeights: List[float], /) -> None:
        """
        Modifies this B-Spline surface by assigning values to all or part of the
        weights of the row of poles of index UIndex of this B-Spline surface.

        The modified part of the row of weights is defined by the bounds of the
        array CPoleWeights.

        This function must only be used for rational surfaces.
        """
        ...

    @constmethod
    def getWeight(self, UIndex: int, VIndex: int, /) -> float:
        """
        Return the weight of the pole of index (UIndex,VIndex)
        in the poles table for this B-Spline surface.
        """
        ...

    @constmethod
    def getWeights(self) -> List[float]:
        """
        Returns the table of weights of the poles for this B-Spline surface.
        """
        ...

    @constmethod
    def getPolesAndWeights(self) -> List[Any]:
        """
        Returns the table of poles and weights in homogeneous coordinates.
        """
        ...

    @constmethod
    def getResolution(self, Tolerance3D: float, /) -> Any:
        """
        Computes two tolerance values for this B-Spline surface, based on the
        given tolerance in 3D space Tolerance3D. The tolerances computed are:
        -- UTolerance in the u parametric direction and
        -- VTolerance in the v parametric direction.

        If f(u,v) is the equation of this B-Spline surface, UTolerance and
        VTolerance guarantee that:
        |u1 - u0| < UTolerance
        |v1 - v0| < VTolerance
        ====> ||f(u1, v1) - f(u2, v2)|| < Tolerance3D
        """
        ...

    def movePoint(
        self,
        U: float,
        V: float,
        P: Any,
        UIndex1: int = ...,
        UIndex2: int = ...,
        VIndex1: int = ...,
        VIndex2: int = ...,
        /,
    ) -> Any:
        """
        Moves the point of parameters (U, V) of this B-Spline surface to P.
        UIndex1, UIndex2, VIndex1 and VIndex2 are the indexes in the poles
        table of this B-Spline surface, of the first and last poles which
        can be moved in each parametric direction.
        The returned indexes UFirstIndex, ULastIndex, VFirstIndex and
        VLastIndex are the indexes of the first and last poles effectively
        modified in each parametric direction.
        In the event of incompatibility between UIndex1, UIndex2, VIndex1,
        VIndex2 and the values U and V:
        -- no change is made to this B-Spline surface, and
        -- UFirstIndex, ULastIndex, VFirstIndex and VLastIndex are set to
           null.
        """
        ...

    def setUNotPeriodic(self) -> None:
        """
        Changes this B-Spline surface into a non-periodic one in the u parametric direction.
        If this B-Spline surface is already non-periodic in the given parametric direction,
        it is not modified.
        If this B-Spline surface is periodic in the given parametric direction, the boundaries
        of the surface are not given by the first and last rows (or columns) of poles (because
        the multiplicity of the first knot and of the last knot in the given parametric direction
        are not modified, nor are they equal to Degree+1, where Degree is the degree of this
        B-Spline surface in the given parametric direction). Only the function Segment ensures
        this property.

        Note: the poles and knots tables are modified.
        """
        ...

    def setVNotPeriodic(self) -> None:
        """
        Changes this B-Spline surface into a non-periodic one in the v parametric direction.
        If this B-Spline surface is already non-periodic in the given parametric direction,
        it is not modified.
        If this B-Spline surface is periodic in the given parametric direction, the boundaries
        of the surface are not given by the first and last rows (or columns) of poles (because
        the multiplicity of the first knot and of the last knot in the given parametric direction
        are not modified, nor are they equal to Degree+1, where Degree is the degree of this
        B-Spline surface in the given parametric direction). Only the function Segment ensures
        this property.

        Note: the poles and knots tables are modified.
        """
        ...

    def setUPeriodic(self, I1: int, I2: int, /) -> None:
        """
        Modifies this surface to be periodic in the u parametric direction.
        To become periodic in a given parametric direction a surface must
        be closed in that parametric direction, and the knot sequence relative
        to that direction must be periodic.
        To generate this periodic sequence of knots, the functions FirstUKnotIndex
        and LastUKnotIndex are used to compute I1 and I2. These are the indexes,
        in the knot array associated with the given parametric direction, of the
        knots that correspond to the first and last parameters of this B-Spline
        surface in the given parametric direction. Hence the period is:

        Knots(I1) - Knots(I2)

        As a result, the knots and poles tables are modified.
        """
        ...

    def setVPeriodic(self, I1: int, I2: int, /) -> None:
        """
        Modifies this surface to be periodic in the v parametric direction.
        To become periodic in a given parametric direction a surface must
        be closed in that parametric direction, and the knot sequence relative
        to that direction must be periodic.
        To generate this periodic sequence of knots, the functions FirstUKnotIndex
        and LastUKnotIndex are used to compute I1 and I2. These are the indexes,
        in the knot array associated with the given parametric direction, of the
        knots that correspond to the first and last parameters of this B-Spline
        surface in the given parametric direction. Hence the period is:

        Knots(I1) - Knots(I2)

        As a result, the knots and poles tables are modified.
        """
        ...

    def setUOrigin(self, Index: int, /) -> None:
        """
        Assigns the knot of index Index in the knots table
        in the u parametric direction to be the origin of
        this periodic B-Spline surface. As a consequence,
        the knots and poles tables are modified.
        """
        ...

    def setVOrigin(self, Index: int, /) -> None:
        """
        Assigns the knot of index Index in the knots table
        in the v parametric direction to be the origin of
        this periodic B-Spline surface. As a consequence,
        the knots and poles tables are modified.
        """
        ...

    @constmethod
    def getUMultiplicity(self, UIndex: int, /) -> Any:
        """
        Returns, for this B-Spline surface, the multiplicity of
        the knot of index UIndex in the u parametric direction.
        """
        ...

    @constmethod
    def getVMultiplicity(self, VIndex: int, /) -> Any:
        """
        Returns, for this B-Spline surface, the multiplicity of
        the knot of index VIndex in the v parametric direction.
        """
        ...

    @constmethod
    def getUMultiplicities(self) -> List[Any]:
        """
        Returns, for this B-Spline surface, the table of
        multiplicities in the u parametric direction
        """
        ...

    @constmethod
    def getVMultiplicities(self) -> List[Any]:
        """
        Returns, for this B-Spline surface, the table of
        multiplicities in the v parametric direction
        """
        ...

    def exchangeUV(self) -> None:
        """
        Exchanges the u and v parametric directions on this B-Spline surface.
        As a consequence:
        -- the poles and weights tables are transposed,
        -- the knots and multiplicities tables are exchanged,
        -- degrees of continuity and rational, periodic and uniform
           characteristics are exchanged and
        -- the orientation of the surface is reversed.
        """
        ...

    @constmethod
    def reparametrize(self) -> Any:
        """
        Returns a reparametrized copy of this surface
        """
        ...

    def approximate(
        self,
        *,
        Points: Any = ...,
        DegMin: int = ...,
        DegMax: int = ...,
        Continuity: int = ...,
        Tolerance: float = ...,
        X0: float = ...,
        dX: float = ...,
        Y0: float = ...,
        dY: float = ...,
        ParamType: str = ...,
        LengthWeight: float = ...,
        CurvatureWeight: float = ...,
        TorsionWeight: float = ...,
    ) -> None:
        """
        Replaces this B-Spline surface by approximating a set of points.
        This method uses keywords :
        - Points = 2Darray of points (or floats, in combination with X0, dX, Y0, dY)
        - DegMin (int), DegMax (int)
        - Continuity = 0,1 or 2 (for C0, C1, C2)
        - Tolerance (float)
        - X0, dX, Y0, dY (floats) with Points = 2Darray of floats
        - ParamType = 'Uniform','Centripetal' or 'ChordLength'
        - LengthWeight, CurvatureWeight, TorsionWeight (floats)
        (with this smoothing algorithm, continuity C1 requires DegMax >= 3 and C2, DegMax >=5)

        Possible combinations :
        - approximate(Points, DegMin, DegMax, Continuity, Tolerance)
        - approximate(Points, DegMin, DegMax, Continuity, Tolerance, X0, dX, Y0, dY)
        With explicit keywords :
        - approximate(Points, DegMin, DegMax, Continuity, Tolerance, ParamType)
        - approximate(Points, DegMax, Continuity, Tolerance, LengthWeight, CurvatureWeight, TorsionWeight)
        """
        ...

    def interpolate(
        self,
        points: Any = ...,
        zpoints: Any = ...,
        X0: float = ...,
        dX: float = ...,
        Y0: float = ...,
        dY: float = ...,
        /,
    ) -> None:
        """
        interpolate(points)
        interpolate(zpoints, X0, dX, Y0, dY)

        Replaces this B-Spline surface by interpolating a set of points.
        The resulting surface is of degree 3 and continuity C2.
        Arguments:
        a 2 dimensional array of vectors, that the surface passes through
        or
        a 2 dimensional array of floats with the z values,
        the x starting point X0 (float),
        the x increment dX (float),
        the y starting point Y0 and increment dY
        """
        ...

    def buildFromPolesMultsKnots(
        self,
        *,
        poles: List[List[Any]],
        umults: List[Any],
        vmults: List[Any],
        uknots: List[Any] = ...,
        vknots: List[Any] = ...,
        uperiodic: bool = ...,
        vperiodic: bool = ...,
        udegree: int = ...,
        vdegree: int = ...,
        weights: List[List[float]] = ...,
    ) -> None:
        """
        Builds a B-Spline by a lists of Poles, Mults and Knots
        arguments: poles (sequence of sequence of Base.Vector), umults, vmults, [uknots, vknots, uperiodic, vperiodic, udegree, vdegree, weights (sequence of sequence of float)]
        """
        ...

    def buildFromNSections(self, control_curves: Any, /) -> None:
        """
        Builds a B-Spline from a list of control curves
        """
        ...

    def scaleKnotsToBounds(self, u0: float, u1: float, v0: float, v1: float, /) -> None:
        """
        Scales the U and V knots lists to fit the specified bounds.
        The shape of the surface is not modified.
        bspline_surf.scaleKnotsToBounds(u0, u1, v0, v1)
        Default arguments are 0.0, 1.0, 0.0, 1.0
        """
        ...
