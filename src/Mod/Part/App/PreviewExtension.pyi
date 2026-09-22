# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Base.Metadata import export, constmethod
from App.DocumentObjectExtension import DocumentObjectExtension

@export(
    Twin="PreviewExtension",
    TwinPointer="PreviewExtension",
    Include="Mod/Part/App/PreviewExtension.h",
    FatherInclude="App/DocumentObjectExtensionPy.h",
)
class PreviewExtension(DocumentObjectExtension):
    """
    Computes the shape shown as a semi-transparent 3D preview of an upcoming
    geometry change.
    """

    def updatePreview(self) -> None:
        """
        Recompute the preview shape if it is stale.

        Does nothing while the preview is fresh. Pair with invalidatePreview()
        to force a recompute.
        """
        ...

    def invalidatePreview(self) -> None:
        """
        Mark the preview stale so the next updatePreview() recomputes it.
        """
        ...

    @constmethod
    def isPreviewFresh(self) -> bool:
        """
        Returns whether the preview shape is up to date with the object's inputs.
        """
        ...
