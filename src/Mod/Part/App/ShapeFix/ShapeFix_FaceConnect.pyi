from Base.Metadata import export, constmethod
from Base.PyObjectBase import PyObjectBase


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

    def add(self, face) -> None:
        """
        add(face, face)
        """
        ...

    def build(self, shell, sewtolerance, fixtolerance) -> None:
        """
        build(shell, sewtolerance, fixtolerance)
        """
        ...

    def clear(self) -> None:
        """
        Clears internal data structure
        """
        ...
