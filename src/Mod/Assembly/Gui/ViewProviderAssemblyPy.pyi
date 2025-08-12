from typing import Any

from Base.Metadata import export

from Gui.ViewProvider import ViewProvider

@export(
    Father="ViewProviderPy",
    Name="ViewProviderAssemblyPy",
    Twin="ViewProviderAssembly",
    TwinPointer="ViewProviderAssembly",
    Include="Mod/Assembly/Gui/ViewProviderAssembly.h",
    Namespace="AssemblyGui",
    FatherInclude="Gui/ViewProviderPy.h",
    FatherNamespace="Gui",
)
class ViewProviderAssemblyPy(ViewProvider):
    """
    This is the ViewProviderAssembly class
    """

    def isInEditMode(self) -> Any:
        """
        Return true if the assembly object is currently in edit mode.

                      isInEditMode() -> bool"""
        ...

    def getDragger(self) -> Any:
        """
        Return the assembly dragger coin object.

                      getDragger() -> SoTransformDragger

                      Returns: dragger coin object of the assembly"""
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
