# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Base.Metadata import constmethod
from Base.PyObjectBase import PyObjectBase
from typing import Any, Dict, List, Optional

class Command(PyObjectBase):
    """
    FreeCAD Python wrapper of Command functions

    Author: Werner Mayer (wmayer[at]users.sourceforge.net)
    Licence: LGPL
    """

    @staticmethod
    def get(name: str, /) -> Optional["Command"]:
        """
        Get a given command by name or None if it doesn't exist.

        name : str
            Command name.
        """
        ...

    @staticmethod
    def update() -> None:
        """
        Update active status of all commands.
        """
        ...

    @staticmethod
    def listAll() -> List[str]:
        """
        Returns the name of all commands.
        """
        ...

    @staticmethod
    def listByShortcut(string: str, useRegExp: bool = False, /) -> List[str]:
        """
        Returns a list of all commands, filtered by shortcut.
        Shortcuts are converted to uppercase and spaces removed
        prior to comparison.

        string :  str
            Shortcut to be searched.
        useRegExp : bool
            Filter using regular expression.
        """
        ...

    def run(self, item: int = 0, /) -> None:
        """
        Runs the given command.

        item : int
            Item to be run.
        """
        ...

    @constmethod
    def isActive(self) -> bool:
        """
        Returns True if the command is active, False otherwise.
        """
        ...

    def getShortcut(self) -> str:
        """
        Returns string representing shortcut key accelerator for command.
        """
        ...

    def setShortcut(self, string: str, /) -> bool:
        """
        Sets shortcut for given command, returns True for success.

        string : str
            Shortcut to be set.
        """
        ...

    def resetShortcut(self) -> bool:
        """
        Resets shortcut for given command back to the default, returns True for success.
        """
        ...

    def getInfo(self) -> Dict[Any, Any]:
        """
        Return information about this command.
        """
        ...

    def getAction(self) -> List[Any]:
        """
        Return the associated QAction object.
        """
        ...

    @staticmethod
    def createCustomCommand(
        *,
        macroFile: str,
        menuText: str,
        toolTip: str,
        whatsThis: str,
        statusTip: str,
        pixmap: str,
        shortcut: str,
    ) -> str:
        """
        Create a custom command for a macro. Returns name of the created command.

        macroFile : str
            Macro file.
        menuText : str
            Menu text. Optional.
        toolTip : str
            Tool tip text. Optional.
        whatsThis : str
            `What's this?` text. Optional.
        statusTip : str
            Status tip text. Optional.
        pixmap : str
            Pixmap name. Optional.
        shortcut : str
            Shortcut key sequence. Optional.
        """
        ...

    @staticmethod
    def removeCustomCommand(name: str, /) -> bool:
        """
        Remove the custom command if it exists.
        Given the name of a custom command, this removes that command.
        It is not an error to remove a non-existent command, the function
        simply does nothing in that case.
        Returns True if something was removed, or False if not.

        name : str
            Command name.
        """
        ...

    @staticmethod
    def findCustomCommand(name: str, /) -> Optional[str]:
        """
        Given the name of a macro, return the name of the custom command for that macro
        or None if there is no command matching that macro script name.

        name : str
            Macro name.
        """
        ...
