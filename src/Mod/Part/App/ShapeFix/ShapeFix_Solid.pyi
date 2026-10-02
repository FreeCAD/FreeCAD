# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Base.Metadata import export
from Part.App.ShapeFix.ShapeFix_Root import ShapeFix_Root
from Part.App.ShapeFix.ShapeFix_Shell import ShapeFix_Shell
from Part.TopoShape import TopoShape
from Part.TopoShapeShell import TopoShapeShell
from Part.TopoShapeSolid import TopoShapeSolid

@export(
    PythonName="Part.ShapeFix.Solid",
    Twin="ShapeFix_Solid",
    TwinPointer="ShapeFix_Solid",
    Include="ShapeFix_Solid.hxx",
    FatherInclude="Mod/Part/App/ShapeFix/ShapeFix_RootPy.h",
    Constructor=True,
)
class ShapeFix_Solid(ShapeFix_Root):
    """
    Root class for fixing operations

    Author: Werner Mayer (wmayer@users.sourceforge.net)
    Licence: LGPL
    """

    FixShellMode: bool = ...
    """Mode for applying fixes of ShapeFix_Shell"""

    FixShellOrientationMode: bool = ...
    """
    Mode for applying analysis and fixes of
    orientation of shells in the solid
    """

    CreateOpenSolidMode: bool = ...
    """Mode for creation of solids"""

    def init(self, solid: TopoShapeSolid, /) -> None:
        """
        Initializes by solid
        """
        ...

    def perform(self) -> bool:
        """
        Iterates on subshapes and performs fixes
        """
        ...

    def solidFromShell(self, shell: TopoShapeShell, /) -> TopoShape:
        """
        Calls MakeSolid and orients the solid to be not infinite
        """
        ...

    def solid(self) -> TopoShape:
        """
        Returns resulting solid
        """
        ...

    def shape(self) -> TopoShape:
        """
        In case of multiconnexity returns compound of fixed solids
        else returns one solid
        """
        ...

    def fixShellTool(self) -> ShapeFix_Shell:
        """
        Returns tool for fixing shells
        """
        ...
