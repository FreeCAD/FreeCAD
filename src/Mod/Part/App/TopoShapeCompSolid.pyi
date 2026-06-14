# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Base.Metadata import export
from TopoShape import TopoShape

@export(
    Father="TopoShapePy",
    Name="TopoShapeCompSolidPy",
    Twin="TopoShape",
    TwinPointer="TopoShape",
    Include="Mod/Part/App/TopoShape.h",
    Namespace="Part",
    FatherInclude="Mod/Part/App/TopoShapePy.h",
    FatherNamespace="Part",
    Constructor=True,
)
class TopoShapeCompSolid(TopoShape):
    """
    TopoShapeCompSolid is the OpenCasCade topological compound solid wrapper
    """

    def add(self, solid: TopoShape, /) -> None:
        """
        Add a solid to the compound.
        add(solid)
        """
        ...
