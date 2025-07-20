from typing import Any, Final

from Base.Metadata import export

from Gui.ViewProviderGeometryObject import ViewProviderGeometryObject

@export(
    Father="ViewProviderGeometryObjectPy",
    Name="ViewProviderFemConstraintPy",
    Twin="ViewProviderFemConstraint",
    TwinPointer="ViewProviderFemConstraint",
    Include="Mod/Fem/Gui/ViewProviderFemConstraint.h",
    Namespace="FemGui",
    FatherInclude="Gui/ViewProviderGeometryObjectPy.h",
    FatherNamespace="Gui",
)
class ViewProviderFemConstraintPy(ViewProviderGeometryObject):
    """
    This is the ViewProviderFemConstraint class
    """

    def loadSymbol(self) -> Any:
        """loadSymbol(filename) -> None

        Load constraint symbol from Open Inventor file.
        The file structure should be as follows:
        A separator containing a separator with the symbol used in
        multiple copies at points on the surface and an optional
        separator with a symbol excluded from multiple copies.

        filename : str
            Open Inventor file."""
        ...
    SymbolNode: Final[Any]
    """A pivy SoSeparator with the nodes of the constraint symbols"""

    ExtraSymbolNode: Final[Any]
    """A pivy SoSeparator with the nodes of the constraint extra symbols"""

    RotateSymbol: bool
    """Apply rotation on copies of the constraint symbol"""
