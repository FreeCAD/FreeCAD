# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Base.Metadata import export
from Base.PyObjectBase import PyObjectBase
from Part.TopoShapeFace import TopoShapeFace
from Part.TopoShapeShell import TopoShapeShell

@export(
    PythonName="Part.ShapeFix.FaceConnect",
    Include="ShapeFix_FaceConnect.hxx",
    Constructor=True,
    Delete=True,
)
class ShapeFix_FaceConnect(PyObjectBase):
    """
    Rebuilds connectivity between faces in shell

    Author: Werner Mayer (wmayer@users.sourceforge.net)
    Licence: LGPL
    """

    def add(self, face1: TopoShapeFace, face2: TopoShapeFace, /) -> None:
        """
        add(face1, face2)
        """
        ...

    def build(self, shell: TopoShapeShell, sewtolerance: float, fixtolerance: float, /) -> None:
        """
        build(shell, sewtolerance, fixtolerance)
        """
        ...

    def clear(self) -> None:
        """
        Clears internal data structure
        """
        ...
