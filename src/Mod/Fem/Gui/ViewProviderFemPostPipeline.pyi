# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Base.Metadata import export

from Gui.ViewProviderDocumentObject import ViewProviderDocumentObject

@export(
    Include="Mod/Fem/Gui/ViewProviderFemPostPipeline.h",
    Namespace="FemGui",
)
class ViewProviderFemPostPipeline(ViewProviderDocumentObject):
    """
    ViewProviderFemPostPipeline class

    Author: Uwe Stöhr (uwestoehr@lyx.org)
    License: LGPL-2.1-or-later
    """

    def transformField(self, field_name: str, field_factor: float, /) -> None:
        """Scales values of given result mesh field by given factor"""
        ...

    def updateColorBars(self) -> None:
        """Update coloring of pipeline and its childs"""
        ...
