# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Base.Metadata import export, constmethod
from TopoShape import TopoShape

@export(
    Twin="TopoShape",
    TwinPointer="TopoShape",
    Include="Mod/Part/App/TopoShape.h",
    FatherInclude="Mod/Part/App/TopoShapePy.h",
    Constructor=True,
)
class TopoShapeCompound(TopoShape):
    """
    Create a compound out of a list of shapes

    Author: Juergen Riegel (Juergen.Riegel@web.de)
    Licence: LGPL
    """

    def add(self, shape: TopoShape, /) -> None:
        """
        Add a shape to the compound.
        add(shape)
        """
        ...

    @constmethod
    def connectEdgesToWires(
        self, Shared: bool = True, Tolerance: float = 1e-7, /
    ) -> "TopoShapeCompound":
        """
        Build a compound of wires out of the edges of this compound.
        connectEdgesToWires([Shared = True, Tolerance = 1e-7]) -> Compound
        --
        If Shared is True  connection is performed only when adjacent edges share the same vertex.
        If Shared is False connection is performed only when ends of adjacent edges are at distance less than Tolerance.
        """
        ...

    def setFaces(self) -> None:
        """
        A shape is created from points and triangles and set to this object
        """
        ...
