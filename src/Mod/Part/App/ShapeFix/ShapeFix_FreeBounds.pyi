# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Base.Metadata import export
from Base.PyObjectBase import PyObjectBase
from Part.App.TopoShapeCompound import TopoShapeCompound
from Part.App.TopoShape import TopoShape

@export(
    PythonName="Part.ShapeFix.FreeBounds",
    Include="ShapeFix_FreeBounds.hxx",
    Constructor=True,
    Delete=True,
)
class ShapeFix_FreeBounds(PyObjectBase):
    """
    This class is intended to output free bounds of the shape

    Author: Werner Mayer (wmayer@users.sourceforge.net)
    Licence: LGPL
    """

    def closedWires(self) -> TopoShapeCompound:
        """
        Returns compound of closed wires out of free edges
        """
        ...

    def openWires(self) -> TopoShapeCompound:
        """
        Returns compound of open wires out of free edges
        """
        ...

    def shape(self) -> TopoShape:
        """
        Returns modified source shape
        """
        ...
