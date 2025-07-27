from typing import Any, Final

from Base.Metadata import export
from Base.Persistence import Persistence

@export(
    Father="PersistencePy",
    Name="TrajectoryPy",
    Twin="Trajectory",
    TwinPointer="Trajectory",
    Include="Mod/Robot/App/Trajectory.h",
    Namespace="Robot",
    FatherInclude="Base/PersistencePy.h",
    FatherNamespace="Base",
    Constructor=True,
    Delete=True,
)
class TrajectoryPy(Persistence):
    """
    Trajectory class
    """

    def insertWaypoints(self) -> Any:
        """adds one or a list of waypoint to the end of the trajectory"""
        ...

    def position(self) -> Any:
        """returns a Frame to a given time in the trajectory"""
        ...

    def velocity(self) -> Any:
        """returns the velocity to a given time in the trajectory"""
        ...

    def deleteLast(self) -> Any:
        """
        deleteLast(n) - delete n waypoints at the end
        deleteLast()  - delete the last waypoint
        """
        ...
    Duration: Final[float]
    """duration of the trajectory"""

    Length: Final[float]
    """length of the trajectory"""

    Waypoints: list
    """waypoints of this trajectory"""
