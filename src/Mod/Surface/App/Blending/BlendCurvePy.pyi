from typing import Any

from Base.BaseClass import PyObjectBase
from Base.Metadata import export

@export(
    Father="PyObjectBase",
    Name="BlendCurvePy",
    Twin="BlendCurve",
    TwinPointer="BlendCurve",
    Include="Mod/Surface/App/Blending/BlendCurve.h",
    Namespace="Surface",
    FatherInclude="Base/BaseClassPy.h",
    FatherNamespace="Base",
    Constructor=True,
    Delete=True,
)
class BlendCurvePy(PyObjectBase):
    """
    Create a BlendCurve that interpolate 2 BlendPoints.
        curve = BlendCurve(BlendPoint1, BlendPoint2)
    """

    def compute(self) -> Any:
        """
        Return the BezierCurve that interpolate the input BlendPoints.
        """
        ...

    def setSize(self) -> Any:
        """
        Set the tangent size of the blendpoint at given index.
        If relative is true, the size is considered relative to the distance between the two blendpoints.
        myBlendCurve.setSize(idx, size, relative)
        """
        ...
