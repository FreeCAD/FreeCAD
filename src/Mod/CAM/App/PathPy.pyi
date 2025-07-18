from typing import Final, List, Any
from Base import object
from Base.Metadata import export
from Base.Metadata import constmethod

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
    Delete=True,  # Allow deleting this object
    Constructor=True,  # Allow constructing this object
)
class PathPy(object):
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
    Length: Final[float]  # ReadOnly
    """the total length of this path in mm"""

    Size: Final[int]  # ReadOnly
    """the number of commands in this path"""

    Commands: List
    """the list of commands of this path"""

    Center: Any
    """the center position for all rotational parameters"""

    BoundBox: Final[Any]  # ReadOnly
    """the extent of this path"""
