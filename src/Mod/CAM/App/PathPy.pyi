from typing import Any, Final

from Base.Metadata import constmethod, export
from Base.Persistence import Persistence

@export(
    Father="PersistencePy",
    Name="PathPy",
    Twin="Toolpath",
    TwinPointer="Toolpath",
    Include="Mod/CAM/App/Path.h",
    Namespace="Path",
    FatherInclude="Base/PersistencePy.h",
    FatherNamespace="Base",
    ReadOnly=["Length", "Size", "BoundBox"],
    Delete=True,
    Constructor=True,
)
class PathPy(Persistence):
    """
    Path([commands]): Represents a basic Gcode path
    commands (optional) is a list of Path commands
    """

    def addCommands(self) -> Any:
        """adds a command or a list of commands at the end of the path"""
        ...

    def insertCommand(self) -> Any:
        """insertCommand(Command,[int]):
        adds a command at the given position or at the end of the path"""
        ...

    def deleteCommand(self) -> Any:
        """deleteCommand([int]):
        deletes the command found at the given position or from the end of the path"""
        ...

    def setFromGCode(self) -> Any:
        """sets the contents of the path from a gcode string"""
        ...

    @constmethod
    def toGCode(self) -> Any:
        """returns a gcode string representing the path"""
        ...

    @constmethod
    def copy(self) -> Any:
        """returns a copy of this path"""
        ...

    @constmethod
    def getCycleTime(self) -> Any:
        """return the cycle time estimation for this path in s"""
        ...
    Length: Final[float]
    """the total length of this path in mm"""

    Size: Final[int]
    """the number of commands in this path"""

    Commands: list
    """the list of commands of this path"""

    Center: Any
    """the center position for all rotational parameters"""

    BoundBox: Final[Any]
    """the extent of this path"""
