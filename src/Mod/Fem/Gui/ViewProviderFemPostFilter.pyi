# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from typing import Any

from Base.Metadata import export

from Gui.ViewProviderDocumentObject import ViewProviderDocumentObject

@export(
    Twin="ViewProviderFemPostObject",
    TwinPointer="ViewProviderFemPostObject",
    Include="Mod/Fem/Gui/ViewProviderFemPostObject.h",
    Namespace="FemGui",
    FatherInclude="Gui/ViewProviderDocumentObjectPy.h",
)
class ViewProviderFemPostFilter(ViewProviderDocumentObject):
    """
    ViewProviderFemPostPipeline class

    Author: Stefan Tröger (stefantroeger@gmx.net)
    License: LGPL-2.1-or-later
    """

    def createDisplayTaskWidget(self) -> Any:
        """Returns the display option task panel for a post processing edit task dialog."""
        ...

    def createExtractionTaskWidget(self) -> Any:
        """Returns the data extraction task panel for a post processing edit task dialog."""
        ...
