# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Base.Metadata import export
from Part.App.ShapeFix.ShapeFix_Root import ShapeFix_Root
from Part.TopoShape import TopoShape

@export(
    PythonName="Part.ShapeFix.Shape",
    Include="ShapeFix_Shape.hxx",
    FatherInclude="Mod/Part/App/ShapeFix/ShapeFix_RootPy.h",
    Constructor=True,
)
class ShapeFix_Shape(ShapeFix_Root):
    """
    Class for fixing operations on shapes

    Author: Werner Mayer (wmayer@users.sourceforge.net)
    Licence: LGPL
    """

    FixSolidMode: bool = ...
    """Mode for applying fixes of ShapeFix_Solid"""

    FixFreeShellMode: bool = ...
    """Mode for applying fixes of ShapeFix_Shell"""

    FixFreeFaceMode: bool = ...
    """Mode for applying fixes of ShapeFix_Face"""

    FixFreeWireMode: bool = ...
    """Mode for applying fixes of ShapeFix_Wire"""

    FixSameParameterMode: bool = ...
    """Mode for applying ShapeFix::SameParameter after all fixes"""

    FixVertexPositionMode: bool = ...
    """Mode for applying ShapeFix::FixVertexPosition before all fixes"""

    FixVertexTolMode: bool = ...
    """Mode for fixing tolerances of vertices on whole shape"""

    def init(self) -> None:
        """
        Initializes by shape
        """
        ...

    def perform(self) -> None:
        """
        Iterates on sub- shape and performs fixes
        """
        ...

    def shape(self) -> TopoShape:
        """
        Returns resulting shape
        """
        ...

    def fixSolidTool(self) -> object:
        """
        Returns tool for fixing solids
        """
        ...

    def fixShellTool(self) -> object:
        """
        Returns tool for fixing shells
        """
        ...

    def fixFaceTool(self) -> object:
        """
        Returns tool for fixing faces
        """
        ...

    def fixWireTool(self) -> object:
        """
        Returns tool for fixing wires
        """
        ...

    def fixEdgeTool(self) -> object:
        """
        Returns tool for fixing edges
        """
        ...
