# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from enum import IntEnum
from Base.Metadata import export
from Base.BaseClass import BaseClass

class ToolbarScope(IntEnum):
    """The layout scope represented by a toolbar persistence identity.

    Shared toolbars retain one identity across workbenches. Workbench toolbars
    belong to one workbench, while contextual toolbars belong to a named state
    within a workbench. Legacy scope is reserved for existing unscoped toolbar
    settings.
    """

    Legacy = 0
    Shared = 1
    Workbench = 2
    Contextual = 3

class ToolbarTier(IntEnum):
    """A toolbar's presentation category and recommended-layout role."""

    Recommended = 0
    Secondary = 1
    Advanced = 2
    Contextual = 3

class ToolbarVisibility(IntEnum):
    """A toolbar's default visibility in its scope.

    Hidden toolbars remain available in the toolbar menu. Unavailable toolbars
    are omitted from the normal visibility UI until their context makes them
    available.
    """

    Visible = 0
    Hidden = 1
    Unavailable = 2

class ToolbarScopeId:
    """The scope portion of a toolbar's stable persistence identity."""

    scope: ToolbarScope
    workbench: str
    context: str

    def __init__(self, scope: ToolbarScope, workbench: str = "", context: str = "") -> None: ...
    @classmethod
    def legacy(cls) -> ToolbarScopeId:
        """Use existing unscoped toolbar settings."""
        ...

    @classmethod
    def shared(cls) -> ToolbarScopeId:
        """Identify a toolbar shared across workbenches."""
        ...

    @classmethod
    def workbench(cls, workbench: str) -> ToolbarScopeId:
        """Identify a toolbar owned by ``workbench``."""
        ...

    @classmethod
    def contextual(cls, workbench: str, context: str) -> ToolbarScopeId:
        """Identify a toolbar in a named context within ``workbench``."""
        ...

class ToolbarOptions:
    """Metadata used to give a toolbar a stable identity and layout behavior.

    ``id`` must be a stable, untranslated identifier. The user-facing, translated
    label is the ``name`` passed to ``appendToolbar()``. Changing ``id`` or
    ``scope`` creates a different persistence identity, so layout state saved for
    the previous identity will no longer apply.

    ``tier`` controls how the toolbar is presented and used by recommended-layout
    actions. ``visibility`` supplies its default visibility within the scope.
    """

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
    ) -> None:
        """Create optional identity, scope, tier, and visibility metadata."""
        ...

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
