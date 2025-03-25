from Base.Metadata import export
from Base.BaseClass import BaseClass
from typing import Any, List, Dict


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
        ...

    def activate(self) -> None:
        """
        Activate this workbench
        """
        ...

    def listToolbars(self) -> List[Any]:
        """
        Show a list of all toolbars
        """
        ...

    def getToolbarItems(self) -> Dict[Any, Any]:
        """
        Show a dict of all toolbars and their commands
        """
        ...

    def listCommandbars(self) -> List[Any]:
        """
        Show a list of all command bars
        """
        ...

    def listMenus(self) -> List[Any]:
        """
        Show a list of all menus
        """
        ...

    @staticmethod
    def reloadActive() -> None:
        """
        Reload the active workbench after changing menus or toolbars
        """
        ...
