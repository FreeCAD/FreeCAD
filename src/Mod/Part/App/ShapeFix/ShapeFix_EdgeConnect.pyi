# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Base.Metadata import export
from Base.PyObjectBase import PyObjectBase
from Part.App.TopoShapeEdge import TopoShapeEdge
from Part.App.TopoShape import TopoShape
from typing import overload

@export(
    PythonName="Part.ShapeFix.EdgeConnect",
    Include="ShapeFix_EdgeConnect.hxx",
    Constructor=True,
    Delete=True,
)
class ShapeFix_EdgeConnect(PyObjectBase):
    """
    Root class for fixing operations

    Author: Werner Mayer (wmayer@users.sourceforge.net)
    Licence: LGPL
    """

    @overload
    def add(self, edge1: TopoShapeEdge, edge2: TopoShapeEdge, /) -> None: ...
    @overload
    def add(self, shape: TopoShape, /) -> None: ...
    def add(self, *args, **kwargs) -> None:
        """
        add(edge, edge)
        Adds information on connectivity between start vertex
        of second edge and end vertex of first edge taking
        edges orientation into account

        add(shape)
        Adds connectivity information for the whole shape.
        """
        ...

    def build(self) -> None:
        """
        Builds shared vertices, updates their positions and tolerances
        """
        ...

    def clear(self) -> None:
        """
        Clears internal data structure
        """
        ...
