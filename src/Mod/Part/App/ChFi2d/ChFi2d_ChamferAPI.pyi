# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Base.Metadata import export
from typing import Tuple, overload

from Base.PyObjectBase import PyObjectBase

@export(
    PythonName="Part.ChFi2d.ChamferAPI",
    Twin="ChFi2d_ChamferAPI",
    TwinPointer="ChFi2d_ChamferAPI",
    Include="ChFi2d_ChamferAPI.hxx",
    Constructor=True,
    Delete=True,
)
class ChFi2d_ChamferAPI(PyObjectBase):
    """
    Algorithm that creates a chamfer between two linear edges

    Author: Werner Mayer (wmayer[at]users.sourceforge.net)
    Licence: LGPL
    """

    @overload
    def __init__(self, wire: object) -> None: ...
    @overload
    def __init__(self, edge1: object, edge2: object) -> None: ...
    @overload
    def init(self, wire: object, /) -> None: ...
    @overload
    def init(self, edge1: object, edge2: object, /) -> None: ...
    def init(self, *args) -> None:
        """
        Initializes a chamfer algorithm: accepts a wire consisting of two edges in a plane,
        or a pair of edges.
        """
        ...

    def perform(self) -> bool:
        """
        perform() -> bool

        Constructs a chamfer edge
        """
        ...

    def result(self, length1: float, length2: float, /) -> Tuple[object, object, object]:
        """
        result(length1, length2)

        Returns result (chamfer edge, modified edge1, modified edge2)
        """
        ...
