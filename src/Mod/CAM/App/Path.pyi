# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations
from typing import Any, Final, overload, Union

from Base.Metadata import constmethod, export
from Base.Persistence import Persistence

from .Command import Command

@export(
    Include="Mod/CAM/App/Path.h",
    Twin="Toolpath",
    TwinPointer="Toolpath",
    Namespace="Path",
    Delete=True,
    Constructor=True,
)
class Path(Persistence):
    """
    Path([commands]): Represents a basic Gcode path
    commands (optional) is a list of Path commands

    Author: Yorik van Havre (yorik@uncreated.net)
    License: LGPL-2.1-or-later
    """

    @overload
    def addCommands(self, command: Command, /) -> Path: ...
    @overload
    def addCommands(self, commands: list[Command], /) -> Path: ...
    def addCommands(self, arg: Union[Command, list[Command]], /) -> Path:
        """adds a command or a list of commands at the end of the path"""
        ...

    def insertCommand(self, command: Command, pos: int = -1, /) -> Path:
        """
        adds a command at the given position or at the end of the path
        """
        ...

    def deleteCommand(self, pos: int = -1, /) -> Path:
        """
        deletes the command found at the given position or from the end of the path
        """
        ...

    def setFromGCode(self, gcode: str, /) -> None:
        """sets the contents of the path from a gcode string"""
        ...

    @constmethod
    def toGCode(self) -> str:
        """returns a gcode string representing the path"""
        ...

    @constmethod
    def copy(self) -> Path:
        """returns a copy of this path"""
        ...

    @constmethod
    def getCycleTime(
        self, h_feed: float, v_feed: float, h_rapid: float, v_rapid: float, /
    ) -> float:
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
