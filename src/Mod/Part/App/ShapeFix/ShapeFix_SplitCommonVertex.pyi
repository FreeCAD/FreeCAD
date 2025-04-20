from Base.Metadata import export
from Part.App.ShapeFix.ShapeFix_Root import ShapeFix_Root
from typing import overload

@export(
    PythonName="Part.ShapeFix.SplitCommonVertex",
    Include="ShapeFix_SplitCommonVertex.hxx",
    FatherInclude="Mod/Part/App/ShapeFix/ShapeFix_RootPy.h",
    Constructor=True,
)
class ShapeFix_SplitCommonVertex(ShapeFix_Root):
    """
    Class for fixing operations on shapes

    Author: Werner Mayer (wmayer@users.sourceforge.net)
    Licence: LGPL
    """

    def init(self) -> None:
        """
        Initializes by shape
        """
        ...

    def perform(self) -> None:
        """
        Iterates on sub- shape and performs fixes
        """
        ...

    def shape(self) -> object:
        """
        Returns resulting shape
        """
        ...
