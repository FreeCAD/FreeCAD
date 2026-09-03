# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Base.Metadata import deprecated, export
from Workbench import Workbench
from typing import Any, List

@export(
    Twin="PythonBaseWorkbench",
    TwinPointer="PythonBaseWorkbench",
    Include="Gui/Workbench.h",
)
class PythonWorkbench(Workbench):
    """
    This is the class for Python workbenches

    Author: Werner Mayer (wmayer@users.sourceforge.net)
    Licence: LGPL
    """

    def appendMenu(self) -> None:
        """
        Append a new menu
        """
        ...

    def removeMenu(self) -> None:
        """
        Remove a menu
        """
        ...

    def appendContextMenu(self) -> None:
        """
        Append a new context menu item
        """
        ...

    def removeContextMenu(self) -> None:
        """
        Remove a context menu item
        """
        ...

    def appendToolbar(self) -> None:
        """
        Append a new toolbar
        """
        ...

    def removeToolbar(self) -> None:
        """
        Remove a toolbar
        """
        ...

    def appendCommandbar(self) -> None:
        """
        Append a new command bar
        """
        ...

    def removeCommandbar(self) -> None:
        """
        Remove a command bar
        """
        ...

    @deprecated(deprecated_in="26.3", removed_in="27.2", replacement="appendMenu")
    def AppendMenu(self) -> None:
        """
        deprecated -- use appendMenu
        """
        ...

    @deprecated(deprecated_in="26.3", removed_in="27.2", replacement="removeMenu")
    def RemoveMenu(self) -> None:
        """
        deprecated -- use removeMenu
        """
        ...

    @deprecated(deprecated_in="26.3", removed_in="27.2", replacement="listMenus")
    def ListMenus(self) -> List[Any]:
        """
        deprecated -- use listMenus
        """
        ...

    @deprecated(deprecated_in="26.3", removed_in="27.2", replacement="appendContextMenu")
    def AppendContextMenu(self) -> None:
        """
        deprecated -- use appendContextMenu
        """
        ...

    @deprecated(deprecated_in="26.3", removed_in="27.2", replacement="removeContextMenu")
    def RemoveContextMenu(self) -> None:
        """
        deprecated -- use removeContextMenu
        """
        ...

    @deprecated(deprecated_in="26.3", removed_in="27.2", replacement="appendToolbar")
    def AppendToolbar(self) -> None:
        """
        deprecated -- use appendToolbar
        """
        ...

    @deprecated(deprecated_in="26.3", removed_in="27.2", replacement="removeToolbar")
    def RemoveToolbar(self) -> None:
        """
        deprecated -- use removeToolbar
        """
        ...

    @deprecated(deprecated_in="26.3", removed_in="27.2", replacement="listToolbars")
    def ListToolbars(self) -> List[Any]:
        """
        deprecated -- use listToolbars
        """
        ...

    @deprecated(deprecated_in="26.3", removed_in="27.2", replacement="appendCommandbar")
    def AppendCommandbar(self) -> None:
        """
        deprecated -- use appendCommandbar
        """
        ...

    @deprecated(deprecated_in="26.3", removed_in="27.2", replacement="removeCommandbar")
    def RemoveCommandbar(self) -> None:
        """
        deprecated -- use removeCommandbar
        """
        ...

    @deprecated(deprecated_in="26.3", removed_in="27.2", replacement="listCommandbars")
    def ListCommandbars(self) -> List[Any]:
        """
        deprecated -- use listCommandbars
        """
        ...
