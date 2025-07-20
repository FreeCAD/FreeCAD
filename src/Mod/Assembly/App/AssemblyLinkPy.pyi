from typing import Final

from Base.Metadata import export

from App.Part import Part

@export(
    Father="PartPy",
    Name="AssemblyLinkPy",
    Twin="AssemblyLink",
    TwinPointer="AssemblyLink",
    Include="Mod/Assembly/App/AssemblyLink.h",
    Namespace="Assembly",
    FatherInclude="App/PartPy.h",
    FatherNamespace="App",
)
class AssemblyLinkPy(Part):
    """
    This class handles document objects in Assembly
    """

    Joints: Final[list]
    """A list of all joints this assembly link has."""
