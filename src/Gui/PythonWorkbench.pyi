# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Base.Metadata import export
from Workbench import Workbench
from typing import Any, List
from typing_extensions import deprecated

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

    @deprecated("Use appendMenu instead.")
    def AppendMenu(self) -> None:
        """
        deprecated -- use appendMenu
        """
        ...

    @deprecated("Use removeMenu instead.")
    def RemoveMenu(self) -> None:
        """
        deprecated -- use removeMenu
        """
        ...

    @deprecated("Use listMenus instead.")
    def ListMenus(self) -> List[Any]:
        """
        deprecated -- use listMenus
        """
        ...

    @deprecated("Use appendContextMenu instead.")
    def AppendContextMenu(self) -> None:
        """
        deprecated -- use appendContextMenu
        """
        ...

    @deprecated("Use removeContextMenu instead.")
    def RemoveContextMenu(self) -> None:
        """
        deprecated -- use removeContextMenu
        """
        ...

    @deprecated("Use appendToolbar instead.")
    def AppendToolbar(self) -> None:
        """
        deprecated -- use appendToolbar
        """
        ...

    @deprecated("Use removeToolbar instead.")
    def RemoveToolbar(self) -> None:
        """
        deprecated -- use removeToolbar
        """
        ...

    @deprecated("Use listToolbars instead.")
    def ListToolbars(self) -> List[Any]:
        """
        deprecated -- use listToolbars
        """
        ...

    @deprecated("Use appendCommandbar instead.")
    def AppendCommandbar(self) -> None:
        """
        deprecated -- use appendCommandbar
        """
        ...

    @deprecated("Use removeCommandbar instead.")
    def RemoveCommandbar(self) -> None:
        """
        deprecated -- use removeCommandbar
        """
        ...

    @deprecated("Use listCommandbars instead.")
    def ListCommandbars(self) -> List[Any]:
        """
        deprecated -- use listCommandbars
        """
        ...
