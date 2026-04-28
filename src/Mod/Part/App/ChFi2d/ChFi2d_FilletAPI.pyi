# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Base.Metadata import export
from Base.PyObjectBase import PyObjectBase
from Part.TopoShapeEdgePy import TopoShapeEdge
from Part.PointPy import Point

@export(
    PythonName="Part.ChFi2d.FilletAPI",
    Twin="ChFi2d_FilletAPI",
    TwinPointer="ChFi2d_FilletAPI",
    Include="ChFi2d_FilletAPI.hxx",
    Constructor=True,
    Delete=True,
)
class ChFi2d_FilletAPI(PyObjectBase):
    """
    Algorithm that creates fillet edge

    Author: Werner Mayer (wmayer[at]users.sourceforge.net)
    Licence: LGPL
    """

    def init(self) -> None:
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

    def numberOfResults(self) -> int:
        """
        Returns number of possible solutions
        """
        ...

    def result(
        self, point: Point, solution: int = -1, /
    ) -> tuple[TopoShapeEdge, TopoShapeEdge, TopoShapeEdge]:
        """
        result(point, solution=-1)

        Returns result (fillet edge, modified edge1, modified edge2)
        """
        ...
