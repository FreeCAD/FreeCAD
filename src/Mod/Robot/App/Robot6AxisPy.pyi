from typing import Any

from Base.Metadata import export
from Base.Persistence import Persistence

@export(
    Father="PersistencePy",
    Name="Robot6AxisPy",
    Twin="Robot6Axis",
    TwinPointer="Robot6Axis",
    Include="Mod/Robot/App/Robot6Axis.h",
    Namespace="Robot",
    FatherInclude="Base/PersistencePy.h",
    FatherNamespace="Base",
    Constructor=True,
    Delete=True,
)
class Robot6AxisPy(Persistence):
    """
    Robot6Axis class
    """

    def check(self) -> Any:
        """Checks the shape and report errors in the shape structure.
        This is a more detailed check as done in isValid()."""
        ...
    Axis1: float
    """Pose of Axis 1 in degrees"""

    Axis2: float
    """Pose of Axis 2 in degrees"""

    Axis3: float
    """Pose of Axis 3 in degrees"""

    Axis4: float
    """Pose of Axis 4 in degrees"""

    Axis5: float
    """Pose of Axis 5 in degrees"""

    Axis6: float
    """Pose of Axis 6 in degrees"""

    Tcp: Any
    """Tool center point frame. Where the tool of the robot is"""

    Base: Any
    """Actual Base system in respect to the robot world system"""
