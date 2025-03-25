from Base.Metadata import export, constmethod
from Base.PyObjectBase import PyObjectBase

@export(
    PythonName="Part.ShapeFix.WireVertex",
    Include="ShapeFix_WireVertex.hxx",
    Constructor=True,
    Delete=True,
)
class ShapeFix_WireVertex(PyObjectBase):
    """
    Fixing disconnected edges in the wire

    Author: Werner Mayer (wmayer@users.sourceforge.net)
    Licence: LGPL
    """

    def init(self) -> None:
        """
        Loads the wire, ininializes internal analyzer with the given precision
        """
        ...

    def wire(self) -> object:
        """
        Returns resulting wire
        """
        ...

    def fixSame(self) -> int:
        """
        Returns the count of fixed vertices, 0 if none
        """
        ...

    def fix(self) -> int:
        """
        Fixes all statuses except Disjoined, i.e. the cases in which a
        common value has been set, with or without changing parameters
        Returns the count of fixed vertices, 0 if none
        """
        ...
