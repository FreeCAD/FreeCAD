# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Base.Metadata import export
from Base.PyObjectBase import PyObjectBase
from typing import Final

@export(
    PythonName="Part.GeomPlate.CurveConstraintPy",
    Twin="GeomPlate_CurveConstraint",
    TwinPointer="GeomPlate_CurveConstraint",
    Include="GeomPlate_CurveConstraint.hxx",
    Constructor=True,
    Delete=True,
)
class CurveConstraint(PyObjectBase):
    """
    Defines curves as constraints to be used to deform a surface

    Author: Werner Mayer (wmayer@users.sourceforge.net)
    Licence: LGPL
    """

    NbPoints: int = ...
    """
    The number of points on the curve used as a
    constraint. The default setting is 10. This parameter
    affects computation time, which increases by the cube of
    the number of points.
    """

    FirstParameter: Final[float] = ...
    """
    This function returns the first parameter of the curve.
    The first parameter is the lowest parametric value for the curve, which defines the starting point of the curve.
    """

    LastParameter: Final[float] = ...
    """
    This function returns the last parameter of the curve.
    The last parameter is the highest parametric value for the curve, which defines the ending point of the curve.
    """

    Length: Final[float] = ...
    """
    This function returns the length of the curve.
    The length of the curve is a geometric property that indicates how long the curve is in the space.
    """

    def setOrder(self) -> None:
        """
        Allows you to set the order of continuity required for the constraints: G0, G1, and G2, controlled
        respectively by G0Criterion G1Criterion and G2Criterion.
        """
        ...

    def order(self) -> None:
        """
        Returns the order of constraint, one of G0, G1 or G2
        """
        ...

    def G0Criterion(self) -> None:
        """
        Returns the G0 criterion at the parametric point U on the curve.
        This is the greatest distance allowed between the constraint and the target surface at U.
        """
        ...

    def G1Criterion(self) -> None:
        """
        Returns the G1 criterion at the parametric point U on the curve.
        This is the greatest angle allowed between the constraint and the target surface at U.
        Raises an exception if the curve is not on a surface.
        """
        ...

    def G2Criterion(self) -> None:
        """
        Returns the G2 criterion at the parametric point U on the curve.
        This is the greatest difference in curvature allowed between the constraint and the target surface at U.
        Raises an exception if the curve is not on a surface.
        """
        ...

    def setG0Criterion(self) -> None:
        """
        Allows you to set the G0 criterion. This is the law
        defining the greatest distance allowed between the
        constraint and the target surface for each point of the
        constraint. If this criterion is not set, TolDist, the
        distance tolerance from the constructor, is used.
        """
        ...

    def setG1Criterion(self) -> None:
        """
        Allows you to set the G1 criterion. This is the law
        defining the greatest angle allowed between the
        constraint and the target surface. If this criterion is not
        set, TolAng, the angular tolerance from the constructor, is used.
        Raises an exception if the curve is not on a surface.
        """
        ...

    def setG2Criterion(self) -> None:
        """
        Allows you to set the G2 criterion. This is the law
        defining the greatest difference in curvature allowed
        between the constraint and the target surface. If this
        criterion is not set, TolCurv, the curvature tolerance from
        the constructor, is used.
        Raises ConstructionError if the point is not on the surface.
        """
        ...

    def curve3d(self) -> None:
        """
        Returns a 3d curve associated the surface resulting of the constraints
        """
        ...

    def setCurve2dOnSurf(self) -> None:
        """
        Loads a 2d curve associated the surface resulting of the constraints
        """
        ...

    def curve2dOnSurf(self) -> None:
        """
        Returns a 2d curve associated the surface resulting of the constraints
        """
        ...

    def setProjectedCurve(self) -> None:
        """
        Loads a 2d curve  resulting from the normal projection of
        the curve on the initial surface
        """
        ...

    def projectedCurve(self) -> None:
        """
        Returns the projected curve resulting from the normal projection of the
        curve on the initial surface
        """
        ...
