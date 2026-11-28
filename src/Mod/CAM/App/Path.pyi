# SPDX-License-Identifier: LGPL-2.1-or-later

from typing import Any, Final, overload, Union

from Base.BoundBox import BoundBox
from Base.Metadata import constmethod, export
from Base.Persistence import Persistence

from .Area import Area
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

    def insertCommand(self, command: Command, pos: int = -1, /) -> Path:
        """
        adds a command at the given position or at the end of the path
        """

    def deleteCommand(self, pos: int = -1, /) -> Path:
        """
        deletes the command found at the given position or from the end of the path
        """

    def setFromGCode(self, gcode: str, /) -> None:
        """sets the contents of the path from a gcode string"""

    @constmethod
    def getClearedArea(self, diameter: float, zmax: float, bbox: BoundBox, /) -> Area:
        """Gets the area cleared when a tool of the specified diameter follows the gcode represented in the path, ignoring cleared space above zmax and path segments that don't affect space within the x/y space of bbox."""

    def addAnnotations(self, annotations: dict, /) -> Path:
        """
        addAnnotations(annotations): adds the items of a dictionary to the annotations
        of the path and returns the path
        """

    @constmethod
    def toGCode(self) -> str:
        """returns a gcode string representing the path"""

    @constmethod
    def copy(self) -> Path:
        """returns a copy of this path"""

    @constmethod
    def getCycleTime(self, h_feed: float, v_feed: float, rapid: float, /) -> float:
        """Return the cycle time estimation for this path in s. Parameter F from command overrides feeds from arguments."""
    Length: Final[float]
    """the total length of this path in mm"""

    Size: Final[int]
    """the number of commands in this path"""

    Commands: list
    """the list of commands of this path"""

    Annotations: dict
    """The annotations of the path as a whole (operation level).

    A dictionary of string keys with string or number values. Use it for
    information about the full path that a post-processor, a dressup or a
    report can read; use Command.Annotations for one command.

    The attribute returns a copy of the dictionary: a change to the returned
    dictionary does not change the path. To change one item, use
    addAnnotations(). An assignment replaces all items.

    The annotations are independent of the commands: a change to Commands keeps
    them. copy() copies them and the document stores them. Path.Path(commands)
    makes a path with no annotations. toGCode() does not write them.

    For a document object, obj.Path.addAnnotations({...}) and
    obj.Path.Annotations = {...} change the path of the object.
    """

    Center: Any
    """the center position for all rotational parameters"""

    BoundBox: Final[Any]
    """the extent of this path"""
