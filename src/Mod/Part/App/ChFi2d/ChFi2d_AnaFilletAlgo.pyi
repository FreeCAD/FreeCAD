# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Metadata import export
from Base.PyObjectBase import PyObjectBase
from Part.App.Plane import Plane
from Part.TopoShapeEdge import TopoShapeEdge
from Part.TopoShapeWire import TopoShapeWire
from typing import Tuple, overload

@export(
    Name="ChFi2d_AnaFilletAlgoPy",
    PythonName="Part.ChFi2d.AnaFilletAlgo",
    Twin="ChFi2d_AnaFilletAlgo",
    TwinPointer="ChFi2d_AnaFilletAlgo",
    Include="ChFi2d_AnaFilletAlgo.hxx",
    Constructor=True,
    Delete=True,
)
class AnaFilletAlgo(PyObjectBase):
    """
    An analytical algorithm for calculation of the fillets.
    It is implemented for segments and arcs of circle only.
    """

    @overload
    def init(self, wire: TopoShapeWire, plane: Plane, /) -> None: ...
    @overload
    def init(self, edge1: TopoShapeEdge, edge2: TopoShapeEdge, plane: Plane, /) -> None: ...
    def init(self, *args) -> None:
        """
        Initializes a fillet algorithm: accepts a wire consisting of two edges in a plane
        """
        ...

    def perform(self, radius: float, /) -> bool:
        """
        perform(radius) -> bool

        Constructs a fillet edge
        """
        ...

    def result(self) -> Tuple[PyObjectBase, PyObjectBase, PyObjectBase]:
        """
        result()

        Returns result (fillet edge, modified edge1, modified edge2)
        """
        ...
