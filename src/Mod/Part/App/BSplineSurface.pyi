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

    def increaseDegree(self, u_degree: int, v_degree: int, /) -> None:
        """
        increaseDegree(u_degree, v_degree)
        Increases the degrees of this B-Spline surface to u_degree and v_degree
        in the u and v parametric directions respectively.
        As a result, the tables of poles, weights and multiplicities are modified.
        The tables of knots is not changed.

        Note: Nothing is done if the given degree is less than or equal to the
        current degree in the corresponding parametric direction.
        """
        ...

    def increaseUMultiplicity(self, start: int, end: int, mult: int = ..., /) -> None:
        """
        Increases the multiplicity in the u direction.
        """
        ...

    def increaseVMultiplicity(self, start: int, end: int, mult: int = ..., /) -> None:
        """
        Increases the multiplicity in the v direction.
        """
        ...

    def incrementUMultiplicity(self, start: int, end: int, mult: int, /) -> None:
        """
        Increment the multiplicity in the u direction
        """
        ...

    def incrementVMultiplicity(self, start: int, end: int, mult: int, /) -> None:
        """
        Increment the multiplicity in the v direction
        """
        ...

    def insertUKnot(self, u: float, mult: int, tol: float, add: bool = ..., /) -> None:
        """
        insertUKnot(u, mult, tol[, add]) - Insert or override a knot
        """
        ...

    def insertUKnots(
        self, u: List[float], mult: List[float], tol: float = 0.0, add: bool = True, /
    ) -> None:
        """
        insertUKnots(u, mult, tol[, add]) - Inserts knots.
        """
        ...

    def insertVKnot(self, v: float, mult: int, tol: float, add: bool = ..., /) -> None:
        """
        insertVKnot(v, mult, tol[, add]) - Insert or override a knot.
        """
        ...

    def insertVKnots(
        self, v: List[float], mult: List[float], tol: float = 0.0, add: bool = True, /
    ) -> None:
        """
        insertVKnots(v, mult, tol[, add]) - Inserts knots.
        """
        ...

    def removeUKnot(self, index: int, mult: int, tol: float, /) -> bool:
        """
        Reduces to mult the multiplicity of the knot of index index in the given
        parametric direction. If mult is 0, the knot is removed.
        With a modification of this type, the table of poles is also modified.
        Two different algorithms are used systematically to compute the new
        poles of the surface. For each pole, the distance between the pole
        calculated using the first algorithm and the same pole calculated using
        the second algorithm, is checked. If this distance is less than tol
        it ensures that the surface is not modified by more than tol.
        Under these conditions, the function returns true; otherwise, it returns
        false.

        A low tolerance prevents modification of the surface. A high tolerance
        'smoothes' the surface.
        """
        ...

    def removeVKnot(self, index: int, mult: int, tol: float, /) -> bool:
        """
        Reduces to mult the multiplicity of the knot of index index in the given
        parametric direction. If mult is 0, the knot is removed.
        With a modification of this type, the table of poles is also modified.
        Two different algorithms are used systematically to compute the new
        poles of the surface. For each pole, the distance between the pole
        calculated using the first algorithm and the same pole calculated using
        the second algorithm, is checked. If this distance is less than tol
        it ensures that the surface is not modified by more than tol.
        Under these conditions, the function returns true; otherwise, it returns
        false.

        A low tolerance prevents modification of the surface. A high tolerance
        'smoothes' the surface.
        """
        ...

    def segment(self, u1: float, u2: float, v1: float, v2: float, /) -> None:
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

    def setUKnot(self, index: int, knot: float, mult: int = ..., /) -> None:
        """
        Modifies this B-Spline surface by assigning the value knot to the knot of index
        index of the knots table corresponding to the u parametric direction.
        This modification remains relatively local, since knot must lie between the values
        of the knots which frame the modified knot.

        You can also increase the multiplicity of the modified knot to mult. Note however
        that it is not possible to decrease the multiplicity of a knot with this function.
        """
        ...

    def setVKnot(self, index: int, knot: float, mult: int = ..., /) -> None:
        """
        Modifies this B-Spline surface by assigning the value knot to the knot of index
        index of the knots table corresponding to the v parametric direction.
        This modification remains relatively local, since knot must lie between the values
        of the knots which frame the modified knot.

        You can also increase the multiplicity of the modified knot to mult. Note however
        that it is not possible to decrease the multiplicity of a knot with this function.
        """
        ...

    @constmethod
    def getUKnot(self, index: int, /) -> Any:
        """
        Returns, for this B-Spline surface, in the u parametric direction
        the knot of index index of the knots table.
        """
        ...

    @constmethod
    def getVKnot(self, index: int, /) -> Any:
        """
        Returns, for this B-Spline surface, in the v parametric direction
        the knot of index index of the knots table.
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

    def setPole(self, u_index: int, v_index: int, pole: Any, weight: float = ..., /) -> None:
        """
        Modifies this B-Spline surface by assigning pole to the pole of
        index (u_index, v_index) in the poles table.
        The second syntax allows you also to change the weight of the
        modified pole. The weight is set to weight. This syntax must
        only be used for rational surfaces.
        """
        ...

    def setPoleCol(
        self, v_index: int, values: List[Any], pole_weights: List[float] = ..., /
    ) -> None:
        """
        Modifies this B-Spline surface by assigning values to all or part
        of the column of poles of index v_index, of this B-Spline surface.
        You can also change the weights of the modified poles. The weights
        are set to the corresponding values of pole_weights.
        These syntaxes must only be used for rational surfaces.
        """
        ...

    def setPoleRow(
        self, u_index: int, values: List[Any], pole_weights: List[float] = ..., /
    ) -> None:
        """
        Modifies this B-Spline surface by assigning values to all or part
        of the row of poles of index u_index, of this B-Spline surface.
        You can also change the weights of the modified poles. The weights
        are set to the corresponding values of pole_weights.
        These syntaxes must only be used for rational surfaces.
        """
        ...

    @constmethod
    def getPole(self, u_index: int, v_index: int, /) -> Any:
        """
        Returns the pole of index (u_index, v_index) of this B-Spline surface.
        """
        ...

    @constmethod
    def getPoles(self) -> List[Any]:
        """
        Returns the table of poles of this B-Spline surface.
        """
        ...

    def setWeight(self, u_index: int, v_index: int, weight: float, /) -> None:
        """
        Modifies this B-Spline surface by assigning the value weight to the weight
        of the pole of index (u_index, v_index) in the poles tables of this B-Spline
        surface.

        This function must only be used for rational surfaces.
        """
        ...

    def setWeightCol(self, v_index: int, pole_weights: List[float], /) -> None:
        """
        Modifies this B-Spline surface by assigning values to all or part of the
        weights of the column of poles of index v_index of this B-Spline surface.

        The modified part of the column of weights is defined by the bounds
        of the array pole_weights.

        This function must only be used for rational surfaces.
        """
        ...

    def setWeightRow(self, u_index: int, pole_weights: List[float], /) -> None:
        """
        Modifies this B-Spline surface by assigning values to all or part of the
        weights of the row of poles of index u_index of this B-Spline surface.

        The modified part of the row of weights is defined by the bounds of the
        array pole_weights.

        This function must only be used for rational surfaces.
        """
        ...

    @constmethod
    def getWeight(self, u_index: int, v_index: int, /) -> float:
        """
        Return the weight of the pole of index (u_index, v_index)
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
    def getResolution(self, tolerance_3d: float, /) -> Any:
        """
        Computes two tolerance values for this B-Spline surface, based on the
        given tolerance in 3D space tolerance_3d. The tolerances computed are:
        -- UTolerance in the u parametric direction and
        -- VTolerance in the v parametric direction.

        If f(u,v) is the equation of this B-Spline surface, UTolerance and
        VTolerance guarantee that:
        |u1 - u0| < UTolerance
        |v1 - v0| < VTolerance
        ====> ||f(u1, v1) - f(u2, v2)|| < tolerance_3d
        """
        ...

    def movePoint(
        self,
        u: float,
        v: float,
        pole: Any,
        u_index1: int,
        u_index2: int,
        v_index1: int,
        v_index2: int,
        /,
    ) -> Any:
        """
        Moves the point of parameters (u, v) of this B-Spline surface to pole.
        u_index1, u_index2, v_index1 and v_index2 are the indexes in the poles
        table of this B-Spline surface, of the first and last poles which
        can be moved in each parametric direction.
        The returned indexes UFirstIndex, ULastIndex, VFirstIndex and
        VLastIndex are the indexes of the first and last poles effectively
        modified in each parametric direction.
        In the event of incompatibility between u_index1, u_index2, v_index1,
        v_index2 and the values u and v:
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

    def setUPeriodic(self) -> None:
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

    def setVPeriodic(self) -> None:
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

    def setUOrigin(self, index: int, /) -> None:
        """
        Assigns the knot at the given index in the knots table
        in the u parametric direction to be the origin of
        this periodic B-Spline surface. As a consequence,
        the knots and poles tables are modified.
        """
        ...

    def setVOrigin(self, index: int, /) -> None:
        """
        Assigns the knot at the given index in the knots table
        in the v parametric direction to be the origin of
        this periodic B-Spline surface. As a consequence,
        the knots and poles tables are modified.
        """
        ...

    @constmethod
    def getUMultiplicity(self, index: int, /) -> Any:
        """
        Returns, for this B-Spline surface, the multiplicity of
        the knot of index index in the u parametric direction.
        """
        ...

    @constmethod
    def getVMultiplicity(self, index: int, /) -> Any:
        """
        Returns, for this B-Spline surface, the multiplicity of
        the knot of index index in the v parametric direction.
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
    def reparametrize(self, u: int, v: int, tol: float = ..., /) -> Any:
        """
        Returns a reparametrized copy of this surface
        """
        ...

    def approximate(
        self,
        *,
        Points: Any,
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
        points: Any,
        x0: float = ...,
        dx: float = ...,
        y0: float = ...,
        dy: float = ...,
        /,
    ) -> None:
        """
        interpolate(points)
        interpolate(points, x0, dx, y0, dy)

        Replaces this B-Spline surface by interpolating a set of points.
        The resulting surface is of degree 3 and continuity C2.
        Arguments:
        a 2 dimensional array of vectors, that the surface passes through
        or
        a 2 dimensional array of floats with the z values,
        the x starting point x0 (float),
        the x increment dx (float),
        the y starting point y0 and increment dy
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

    def buildFromNSections(self, control_curves: Any, ref_surf: bool = ..., /) -> None:
        """
        Builds a B-Spline from a list of control curves
        """
        ...

    def scaleKnotsToBounds(
        self, u0: float = ..., u1: float = ..., v0: float = ..., v1: float = ..., /
    ) -> None:
        """
        Scales the U and V knots lists to fit the specified bounds.
        The shape of the surface is not modified.
        bspline_surf.scaleKnotsToBounds(u0, u1, v0, v1)
        Default arguments are 0.0, 1.0, 0.0, 1.0
        """
        ...
