# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Metadata import export
from Base.PyObjectBase import PyObjectBase
from typing import overload
from Base.Vector import Vector
from Part.App.Plane import Plane
from Part.TopoShapeEdge import TopoShapeEdge
from Part.TopoShapeWire import TopoShapeWire

@export(
    Name="ChFi2d_FilletAlgoPy",
    PythonName="Part.ChFi2d.FilletAlgo",
    Twin="ChFi2d_FilletAlgo",
    TwinPointer="ChFi2d_FilletAlgo",
    Include="ChFi2d_FilletAlgo.hxx",
    Constructor=True,
    Delete=True,
)
class FilletAlgo(PyObjectBase):
    """
    Algorithm that creates fillet edge

    Author: Werner Mayer (wmayer[at]users.sourceforge.net)
    Licence: LGPL
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

    def numberOfResults(self, point: Vector, /) -> int:
        """
        Returns number of possible solutions
        """
        ...

    def result(self, point: Vector, solution: int = -1, /) -> tuple[object, object, object]:
        """
        result(point, solution=-1)

        Returns result (fillet edge, modified edge1, modified edge2)
        """
        ...
