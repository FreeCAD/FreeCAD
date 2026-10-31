# SPDX-License-Identifier: LGPL-2.1-or-later

from Base.Metadata import export
from Base.BaseClass import BaseClass

@export(
    Include="Gui/Workbench.h",
)
class Workbench(BaseClass):
    """
    This is the base class for workbenches

    Author: Werner Mayer (wmayer@users.sourceforge.net)
    Licence: LGPL
    """

    def name(self) -> str:
        """
        Return the workbench name
        """

    def activate(self) -> None:
        """
        Activate this workbench
        """

    def listToolbars(self) -> list[str]:
        """
        Show a list of all toolbars
        """

    def getToolbarItems(self) -> dict[str, list[str]]:
        """
        Show a dict of all toolbars and their commands
        """

    def listCommandbars(self) -> list[str]:
        """
        Show a list of all command bars
        """

    def listMenus(self) -> list[str]:
        """
        Show a list of all menus
        """

    @staticmethod
    def reloadActive() -> None:
        """
        Reload the active workbench after changing menus or toolbars
        """
