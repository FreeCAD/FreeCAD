# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Base.Metadata import export, constmethod
from Gui.ViewProviderExtension import ViewProviderExtension

@export(
    Twin="ViewProviderPreviewExtension",
    TwinPointer="ViewProviderPreviewExtension",
    Include="Mod/Part/Gui/ViewProviderPreviewExtension.h",
    FatherInclude="Gui/ViewProviderExtensionPy.h",
    Namespace="PartGui",
    FatherNamespace="Gui",
)
class ViewProviderPreviewExtension(ViewProviderExtension):
    """
    Provides a 3D preview of an upcoming geometry change.
    """

    def showPreview(self, enable: bool, /) -> None:
        """
        Show or hide the preview in the 3D view.
        """
        ...

    @constmethod
    def isPreviewEnabled(self) -> bool:
        """
        Returns if the preview is currently shown.
        """
        ...
