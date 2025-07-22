from Base.Metadata import export, constmethod
from Part.App.GeometryExtension import GeometryExtension

@export(
    Name="SketchGeometryExtensionPy",
    PythonName="Sketcher.SketchGeometryExtension",
    Include="Mod/Sketcher/App/SketchGeometryExtension.h",
    FatherInclude="Mod/Part/App/GeometryExtensionPy.h",
    Constructor=True,
)
class SketchGeometryExtension(GeometryExtension):
    """
    Describes a SketchGeometryExtension

    Author: Abdullah Tahiri (abdullah.tahiri.yo@gmail.com)
    Licence: LGPL
    """

    Id: int = 0
    """Returns the Id of the SketchGeometryExtension."""

    InternalType: str = ""
    """Returns the Id of the SketchGeometryExtension."""

    Blocked: bool = False
    """Sets/returns whether the geometry is blocked or not."""

    Construction: bool = False
    """Sets/returns this geometry as a construction one, which will not be part of a later built shape."""

    GeometryLayerId: int = 0
    """Returns the Id of the geometry Layer in which the geometry is located."""

    @constmethod
    def testGeometryMode(self) -> bool:
        """
        Returns a boolean indicating whether the given bit is set.
        """
        ...

    def setGeometryMode(self) -> None:
        """
        Sets the given bit to true/false.
        """
        ...
