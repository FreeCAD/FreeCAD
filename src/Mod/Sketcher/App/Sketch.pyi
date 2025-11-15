# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Base.Metadata import export
from Base.Persistence import Persistence
from Base.Vector import Vector
from typing import Final, Tuple

@export(
    Include="Mod/Sketcher/App/Sketch.h",
    FatherInclude="Base/PersistencePy.h",
    Constructor=True,
)
class Sketch(Persistence):
    """
    With this objects you can handle constraint sketches

    Author: Juergen Riegel (FreeCAD@juergen-riegel.net)
    Licence: LGPL
    """

    Constraint: Final[int] = 0
    """0: exactly constraint, -1 under-constraint, 1 over-constraint"""

    Conflicts: Final[Tuple] = ()
    """Tuple of conflicting constraints"""

    Redundancies: Final[Tuple] = ()
    """Tuple of redundant constraints"""

    Geometries: Final[Tuple] = ()
    """Tuple of all geometric elements in this sketch"""

    Shape: Final[object] = None
    """Resulting shape from the sketch geometry"""

    def solve(self) -> None:
        """
        Solve the actual set of geometry and constraints
        """
        ...

    def addGeometry(self) -> None:
        """
        Add a geometric object to the sketch
        """
        ...

    def addConstraint(self) -> None:
        """
        Add an constraint object to the sketch
        """
        ...

    def clear(self) -> None:
        """
        Clear the sketch
        """
        ...

    def moveGeometry(
        self, GeoIndex: int, PointPos: Vector, Vector: Vector, relative: bool = False, /
    ) -> None:
        """
        Move a given point (or curve).
        to another location.
        It moves the specified point (or curve) to the given location by adding some
        temporary weak constraints and solve the sketch.
        This method is mostly used to allow the user to drag some portions of the sketch
        in real time by e.g. the mouse and it works only for underconstrained portions of
        the sketch.
        The argument 'relative', if present, states if the new location is given
        relatively to the current one.
        """
        ...
