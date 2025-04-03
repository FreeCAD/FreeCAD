from Metadata import export, constmethod
from Base.PyObjectBase import PyObjectBase
from typing import Tuple

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

    def init(self) -> None:
        """
        Initializes a fillet algorithm: accepts a wire consisting of two edges in a plane
        """
        ...

    def perform(self, radius: float) -> bool:
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
