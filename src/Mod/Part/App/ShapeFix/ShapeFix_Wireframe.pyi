# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Base.Metadata import export
from Part.App.ShapeFix.ShapeFix_Root import ShapeFix_Root

@export(
    PythonName="Part.ShapeFix.Wireframe",
    Twin="ShapeFix_Wireframe",
    TwinPointer="ShapeFix_Wireframe",
    Include="ShapeFix_Wireframe.hxx",
    FatherInclude="Mod/Part/App/ShapeFix/ShapeFix_RootPy.h",
    Constructor=True,
)
class ShapeFix_Wireframe(ShapeFix_Root):
    """
    Provides methods for fixing wireframe of shape

    Author: Werner Mayer (wmayer@users.sourceforge.net)
    Licence: LGPL
    """

    ModeDropSmallEdges: bool = ...
    """Returns mode managing removing small edges"""

    LimitAngle: float = ...
    """Limit angle for merging edges"""

    def clearStatuses(self) -> None:
        """
        Clears all statuses
        """
        ...

    def load(self) -> None:
        """
        Loads a shape, resets statuses
        """
        ...

    def fixWireGaps(self) -> None:
        """
        Fixes gaps between ends of curves of adjacent edges
        """
        ...

    def fixSmallEdges(self) -> None:
        """
        Fixes small edges in shape by merging adjacent edges
        """
        ...

    def shape(self) -> None: ...
