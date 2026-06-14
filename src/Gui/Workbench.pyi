# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from enum import IntEnum
from Base.Metadata import export
from Base.BaseClass import BaseClass

class ToolbarScope(IntEnum):
    Legacy = 0
    Shared = 1
    Workbench = 2
    Contextual = 3

class ToolbarTier(IntEnum):
    Recommended = 0
    Secondary = 1
    Advanced = 2
    Contextual = 3

class ToolbarVisibility(IntEnum):
    Visible = 0
    Hidden = 1
    Unavailable = 2

class ToolbarScopeId:
    scope: ToolbarScope
    workbench: str
    context: str

    def __init__(self, scope: ToolbarScope, workbench: str = "", context: str = "") -> None: ...
    @classmethod
    def legacy(cls) -> ToolbarScopeId: ...
    @classmethod
    def shared(cls) -> ToolbarScopeId: ...
    @classmethod
    def workbench(cls, workbench: str) -> ToolbarScopeId: ...
    @classmethod
    def contextual(cls, workbench: str, context: str) -> ToolbarScopeId: ...

class ToolbarOptions:
    id: str | None
    scope: ToolbarScopeId | None
    tier: ToolbarTier | None
    visibility: ToolbarVisibility | None

    def __init__(
        self,
        id: str | None = None,
        scope: ToolbarScopeId | None = None,
        tier: ToolbarTier | None = None,
        visibility: ToolbarVisibility | None = None,
    ) -> None: ...

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

    def listToolbars(self) -> list[str]:
        """
        Show a list of all toolbars
        """
        ...

    def getToolbarItems(self) -> dict[str, list[str]]:
        """
        Show a dict of all toolbars and their commands
        """
        ...

    def getToolbarIdentities(self) -> dict[str, str]:
        """
        Show a dict of all toolbars and their persistence identities
        """
        ...

    def listCommandbars(self) -> list[str]:
        """
        Show a list of all command bars
        """
        ...

    def listMenus(self) -> list[str]:
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
