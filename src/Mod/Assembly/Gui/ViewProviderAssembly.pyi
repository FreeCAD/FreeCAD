# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from typing import Any, List, Tuple, TypeAlias

from Base.Metadata import export
from App.DocumentObject import DocumentObject
from Gui.ViewProvider import ViewProvider

SoTransformDragger: TypeAlias = Any

@export(Include="Mod/Assembly/Gui/ViewProviderAssembly.h", Namespace="AssemblyGui")
class ViewProviderAssembly(ViewProvider):
    """
    This is the ViewProviderAssembly class

    Author: Ondsel (development@ondsel.com)
    License: LGPL-2.1-or-later
    """

    def isInEditMode(self) -> Any:
        """
        Return true if the assembly object is currently in edit mode.
        """
        ...

    def getDragger(self) -> SoTransformDragger:
        """Return the assembly dragger coin object."""
        ...

    def isolateComponents(
        self, components: List[DocumentObject] | Tuple[DocumentObject, ...], mode: int, /
    ) -> None:
        """
        Temporarily isolates a given set of components in the 3D view.
        Other components are faded or hidden based on the specified mode.

        Args:
            components (List[DocumentObject] | Tuple[DocumentObject, ...]):
                A list or tuple of DocumentObjects to isolate.
            mode (int): An integer specifying the isolation mode:
                - 0: Transparent
                - 1: Wireframe
                - 2: Hidden
        """
        ...

    def clearIsolate(self) -> None:
        """
        Restores the visual state of all components, clearing any active isolation.
        """
        ...
    EnableMovement: bool
    """Enable moving the parts by clicking and dragging."""

    MoveOnlyPreselected: bool
    """If enabled, only the preselected object will move."""

    MoveInCommand: bool
    """If enabled, each move will be wrapped in a command."""

    DraggerVisibility: bool
    """Show or hide the assembly dragger."""

    DraggerPlacement: Any
    """Placement of the assembly dragger object."""
