# SPDX-License-Identifier: LGPL-2.1-or-later

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
    Command([name],[parameters],[annotations]): Represents a basic Gcode command
    name (optional) is the name of the command, ex. G1
    parameters (optional) is a dictionary containing string:number
    pairs, or a placement, or a vector
    annotations (optional) is a dictionary containing string:string or string:number pairs
    """

    @constmethod
    def toGCode(self) -> str:
        """returns a GCode representation of the command"""
        ...

    def setFromGCode(self, gcode: str, /) -> None:
        """sets the path from the contents of the given GCode string"""
        ...

    def transform(self, placement: Placement, /) -> Command:
        """returns a copy of this command transformed by the given placement"""
        ...

    def addAnnotations(self, annotations, /) -> "Command":
        """addAnnotations(annotations): adds annotations from dictionary or string and returns self for chaining"""
        ...
    Name: str
    """The name of the command"""

    Parameters: dict[str, float]
    """The parameters of the command"""

    Annotations: dict[str, str]
    """The annotations of the command"""

    Placement: Placement
    """The coordinates of the endpoint of the command"""
