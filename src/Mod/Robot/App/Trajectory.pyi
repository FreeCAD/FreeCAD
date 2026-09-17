# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from typing import Any, Final

from Base.Metadata import export
from Base.Persistence import Persistence
from Base.Placement import Placement
from Waypoint import Waypoint

@export(
    Include="Mod/Robot/App/Trajectory.h",
    Namespace="Robot",
    Constructor=True,
    Delete=True,
)
class Trajectory(Persistence):
    """
    Trajectory class

    Author: Juergen Riegel (Juergen.Riegel@web.de)
    License: LGPL-2.1-or-later
    """

    def insertWaypoints(self, waypoint: Placement | Waypoint | list[Waypoint], /) -> Trajectory:
        """adds one or a list of waypoint to the end of the trajectory"""
        ...

    def position(self, pos: float, /) -> Placement:
        """returns a Frame to a given time in the trajectory"""
        ...

    def velocity(self, pos: float, /) -> float:
        """returns the velocity to a given time in the trajectory"""
        ...

    def deleteLast(self, n: int = ..., /) -> Trajectory:
        """
        deleteLast(n) - delete n waypoints at the end
        deleteLast()  - delete the last waypoint
        """
        ...
    Duration: Final[float]
    """duration of the trajectory"""

    Length: Final[float]
    """length of the trajectory"""

    Waypoints: list[Waypoint]
    """waypoints of this trajectory"""
