# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Base.Metadata import export
from Base.PyObjectBase import PyObjectBase
from Base.Vector import Vector
from typing import Tuple, overload

@export(
    PythonName="Part.GeomPlate.PointConstraintPy",
    Twin="GeomPlate_PointConstraint",
    TwinPointer="GeomPlate_PointConstraint",
    Include="GeomPlate_PointConstraint.hxx",
    Constructor=True,
    Delete=True,
)
class PointConstraint(PyObjectBase):
    """
    Defines points as constraints to be used to deform a surface

    Author: Werner Mayer (wmayer@users.sourceforge.net)
    Licence: LGPL
    """

    @overload
    def __init__(self, Point: Vector, Order: int = ..., TolDist: float = ...) -> None: ...
    def setOrder(self, order: int, /) -> None:
        """
        Allows you to set the order of continuity required for
        the constraints: G0, G1, and G2 (as 0, 1 and 2), controlled
        respectively by G0Criterion G1Criterion and G2Criterion.
        """
        ...

    def order(self) -> int:
        """
        Returns the order of constraint (0, 1 or 2 for G0, G1 or G2)
        """
        ...

    def G0Criterion(self) -> float:
        """
        Returns the G0 criterion.
        This is the greatest distance allowed between
        the constraint and the target surface.
        """
        ...

    def G1Criterion(self) -> float:
        """
        Returns the G1 criterion.
        This is the greatest angle allowed between
        the constraint and the target surface.
        Raises an exception if  the  curve  is  not  on  a  surface.
        """
        ...

    def G2Criterion(self) -> float:
        """
        Returns the G2 criterion.
        This is the greatest difference in curvature
        allowed between the constraint and the target surface.
        Raises an exception if  the  curve  is  not  on  a  surface.
        """
        ...

    def setG0Criterion(self, value: float, /) -> None:
        """
        Allows you to set the G0 criterion. This is the law
        defining the greatest distance allowed between the
        constraint and the target surface for each point of the
        constraint. If this criterion is not set, TolDist, the
        distance tolerance from the constructor, is used.
        """
        ...

    def setG1Criterion(self, value: float, /) -> None:
        """
        Allows you to set the G1 criterion. This is the law
        defining the greatest angle allowed between the
        constraint and the target surface. If this criterion is not
        set, TolAng, the angular tolerance from the constructor, is used.
        Raises an exception if  the  curve  is  not  on  a  surface
        """
        ...

    def setG2Criterion(self, value: float, /) -> None:
        """
        Allows you to set the G2 criterion. This is the law
        defining the greatest difference in curvature  allowed between the
        constraint and the target surface. If this criterion is not
        set, TolCurv, the curvature tolerance from the constructor, is used.
        Raises  ConstructionError if  the  curve  is  not  on  a  surface
        """
        ...

    def hasPnt2dOnSurf(self) -> bool:
        """
        Checks if there is a 2D point associated with the surface. It returns a boolean indicating whether such a point exists.
        """
        ...

    def setPnt2dOnSurf(self, x: float, y: float, /) -> None:
        """
        Allows you to set a 2D point on the surface. It takes a gp_Pnt2d as an argument, representing the 2D point to be associated with the surface.
        """
        ...

    def pnt2dOnSurf(self) -> Tuple[float, float]:
        """
        Returns the 2D point on the surface. It returns a gp_Pnt2d representing the associated 2D point.
        """
        ...
