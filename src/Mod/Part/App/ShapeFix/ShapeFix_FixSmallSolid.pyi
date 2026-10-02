# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Base.Metadata import export
from Part.App.ShapeFix.ShapeFix_Root import ShapeFix_Root
from Part.TopoShape import TopoShape

@export(
    PythonName="Part.ShapeFix.FixSmallSolid",
    Include="ShapeFix_FixSmallSolid.hxx",
    FatherInclude="Mod/Part/App/ShapeFix/ShapeFix_RootPy.h",
    Constructor=True,
)
class ShapeFix_FixSmallSolid(ShapeFix_Root):
    """
    Fixing solids with small size

    Author: Werner Mayer (wmayer@users.sourceforge.net)
    Licence: LGPL
    """

    def setFixMode(self, theMode: int, /) -> None:
        """
        Set working mode for operator:
        - theMode = 0 use both WidthFactorThreshold and VolumeThreshold parameters
        - theMode = 1 use only WidthFactorThreshold parameter
        - theMode = 2 use only VolumeThreshold parameter
        """
        ...

    def setVolumeThreshold(self, value: float = -1.0, /) -> None:
        """
        Set or clear volume threshold for small solids
        """
        ...

    def setWidthFactorThreshold(self, value: float = -1.0, /) -> None:
        """
        Set or clear width factor threshold for small solids
        """
        ...

    def remove(self, shape: TopoShape, /) -> TopoShape:
        """
        Remove small solids from the given shape
        """
        ...

    def merge(self, shape: TopoShape, /) -> TopoShape:
        """
        Merge small solids in the given shape to adjacent non-small ones
        """
        ...
