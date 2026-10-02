# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Base.Metadata import export
from Base.PyObjectBase import PyObjectBase
from typing import overload
from Part.TopoShape import TopoShape
from Part.TopoShapeEdge import TopoShapeEdge
from Part.TopoShapeFace import TopoShapeFace
from Part.TopoShapeVertex import TopoShapeVertex

@export(
    PythonName="Part.ShapeFix.SplitTool",
    Include="ShapeFix_SplitTool.hxx",
    FatherInclude="Base/PyObjectBase.h",
    Constructor=True,
    Delete=True,
)
class ShapeFix_SplitTool(PyObjectBase):
    """
    Tool for splitting and cutting edges

    Author: Werner Mayer (wmayer@users.sourceforge.net)
    Licence: LGPL
    """

    @overload
    def splitEdge(
        self,
        edge: TopoShapeEdge,
        param1: float,
        vertex: TopoShapeVertex,
        face: TopoShapeFace,
        tol3d: float,
        tol2d: float,
        /,
    ) -> tuple[TopoShape, TopoShape]: ...
    @overload
    def splitEdge(
        self,
        edge: TopoShapeEdge,
        param1: float,
        param2: float,
        vertex: TopoShapeVertex,
        face: TopoShapeFace,
        tol3d: float,
        tol2d: float,
        /,
    ) -> tuple[TopoShape, TopoShape]: ...
    def splitEdge(self, *args) -> tuple[TopoShape, TopoShape]:
        """
        Split edge on two new edges using new vertex
        """
        ...

    def cutEdge(self, edge: TopoShapeEdge, pend: float, cut: float, face: TopoShapeFace, /) -> bool:
        """
        Cut edge by parameters pend and cut
        """
        ...
