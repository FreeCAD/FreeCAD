# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from typing import Any, Final

from Base.Metadata import export

from Gui.ViewProviderGeometryObject import ViewProviderGeometryObject

@export(
    Include="Mod/Fem/Gui/ViewProviderFemConstraint.h",
    Namespace="FemGui",
)
class ViewProviderFemConstraint(ViewProviderGeometryObject):
    """
    This is the ViewProviderFemConstraint class

    Author: Mario Passaglia (mpassaglia@cbc.uba.ar)
    License: LGPL-2.1-or-later
    """

    def loadSymbol(self, file_name: str, /) -> Any:
        """
        Load constraint symbol from Open Inventor file.
        The file structure should be as follows:
        A separator containing a separator with the symbol used in
        multiple copies at points on the surface and an optional
        separator with a symbol excluded from multiple copies.

        file_name : str
            Open Inventor file.
        """
        ...
    SymbolNode: Final[Any]
    """A pivy SoSeparator with the nodes of the constraint symbols"""

    ExtraSymbolNode: Final[Any]
    """A pivy SoSeparator with the nodes of the constraint extra symbols"""

    RotateSymbol: bool
    """Apply rotation on copies of the constraint symbol"""
