# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from typing import Any

from Base.Metadata import export
from Base.Persistence import Persistence

@export(
    Include="Mod/Robot/App/Waypoint.h",
    Namespace="Robot",
    Constructor=True,
    Delete=True,
)
class Waypoint(Persistence):
    """
    Waypoint class

    Author: Juergen Riegel (Juergen.Riegel@web.de)
    License: LGPL-2.1-or-later
    """

    Name: str
    """Name of the waypoint"""

    Type: str
    """Type of the waypoint[PTP|LIN|CIRC|WAIT]"""

    Pos: Any
    """End position (destination) of the waypoint"""

    Cont: bool
    """Control the continuity to the next waypoint in the trajectory"""

    Velocity: float
    """Control the velocity to the next waypoint in the trajectory
In Case of PTP 0-100% Axis speed
In Case of LIN m/s
In Case of WAIT s wait time"""

    Tool: int
    """Describe which tool frame to use for that point"""

    Base: int
    """Describe which Base frame to use for that point"""
