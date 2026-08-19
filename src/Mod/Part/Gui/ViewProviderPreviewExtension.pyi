# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Base.Metadata import export, constmethod
from Gui.ViewProviderExtension import ViewProviderExtension
from typing import Any, Final

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

    PreviewRootNode: Final[Any] = ...
    """
    Root of the preview scene graph, as a pivy SoSeparator.

    Children may be added, removed, or replaced wholesale from an
    attachPreview or updatePreview hook.
    """

    PreviewShapeNode: Final[Any] = ...
    """
    The SoPreviewShape the default implementation feeds with the preview shape.
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

    def updatePreview(self) -> None:
        """
        Rebuild the preview scene graph.

        Use when the preview arrangement depends on data other than PreviewShape,
        which is the only property that triggers a rebuild automatically.

        Never call this from inside an updatePreview hook - it dispatches back
        into that hook and recurses.
        """
        ...

    def updatePreviewShape(self, shape: Any, node: Any, /) -> None:
        """
        Tessellate a Part.Shape into an SoPreviewShape node, honouring the view
        provider's Deviation and AngularDeflection.

        Raises TypeError if node is not an SoPreviewShape.
        """
        ...
