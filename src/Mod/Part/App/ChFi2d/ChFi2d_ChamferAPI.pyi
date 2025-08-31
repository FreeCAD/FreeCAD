from Base.Metadata import export, constmethod
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

    def init(self) -> None:
        """
        Initializes a chamfer algorithm: accepts a wire consisting of two edges in a plane
        """
        ...

    def perform(self, radius: float) -> bool:
        """
        perform(radius) -> bool

        Constructs a chamfer edge
        """
        ...

    def result(self, point: object, solution: int = -1) -> Tuple[object, object, object]:
        """
        result(point, solution=-1)

        Returns result (chamfer edge, modified edge1, modified edge2)
        """
        ...
