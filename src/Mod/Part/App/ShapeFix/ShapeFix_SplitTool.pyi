from Base.Metadata import export, constmethod
from Base.PyObjectBase import PyObjectBase
from typing import overload


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

    def splitEdge(self) -> None:
        """
        Split edge on two new edges using new vertex
        """
        ...

    def cutEdge(self) -> None:
        """
        Cut edge by parameters pend and cut
        """
        ...