from typing import Any, Final

from Base.BaseClass import PyObjectBase
from Base.Metadata import constmethod, export

@export(
    Father="PyObjectBase",
    Name="BlendPointPy",
    Twin="BlendPoint",
    TwinPointer="BlendPoint",
    Include="Mod/Surface/App/Blending/BlendPoint.h",
    Namespace="Surface",
    FatherInclude="Base/BaseClassPy.h",
    FatherNamespace="Base",
    Constructor=True,
    Delete=True,
)
class BlendPointPy(PyObjectBase):
    """
    Create BlendPoint from a point and some derivatives.
    myBlendPoint = BlendPoint([Point, D1, D2, ..., DN])
    BlendPoint can also be constructed from an edge
    myBlendPoint = BlendPoint(Edge, parameter = float, continuity = int)
    """

    @constmethod
    def getSize(self) -> Any:
        """Return BlendPoint first derivative length."""
        ...

    def setSize(self) -> Any:
        """
        Resizes the BlendPoint vectors,
        by setting the length of the first derivative.
        theBlendPoint.setSize(new_size)
        """
        ...

    def setvectors(self) -> Any:
        """
        Set the vectors of BlendPoint.
        BlendPoint.setvectors([Point, D1, D2, ..., DN])
        """
        ...
    Vectors: Final[list]
    """The list of vectors of this BlendPoint."""
