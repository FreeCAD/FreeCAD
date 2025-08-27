from Base.Metadata import class_declarations, constmethod, export
from Base.Persistence import Persistence
from Base.Placement import Placement

@export(
    Include="Mod/CAM/App/Command.h",
    Namespace="Path",
    Delete=True,
    Constructor=True,
)
@class_declarations("mutable Py::Dict parameters_copy_dict;")
class Command(Persistence):
    """
    Command([name],[parameters]): Represents a basic Gcode command
    name (optional) is the name of the command, ex. G1
    parameters (optional) is a dictionary containing string:number
    pairs, or a placement, or a vector
    """

    @constmethod
    def toGCode(self) -> str:
        """toGCode(): returns a GCode representation of the command"""
        ...

    def setFromGCode(self, gcode: str) -> None:
        """setFromGCode(): sets the path from the contents of the given GCode string"""
        ...

    def transform(self, placement: Placement) -> "CommandPy":
        """transform(Placement): returns a copy of this command transformed by the given placement"""
        ...
    Name: str
    """The name of the command"""

    Parameters: dict[str, float]
    """The parameters of the command"""

    Placement: Placement
    """The coordinates of the endpoint of the command"""
