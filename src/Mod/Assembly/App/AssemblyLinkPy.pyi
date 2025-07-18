from typing import Final
from App import object
from Base.Metadata import export

@export(
    Father="PartPy",
    Name="AssemblyLinkPy",
    Twin="AssemblyLink",
    TwinPointer="AssemblyLink",
    Include="Mod/Assembly/App/AssemblyLink.h",
    Namespace="Assembly",
    FatherInclude="App/PartPy.h",
    FatherNamespace="App",
    ReadOnly=["Joints"],
)
class AssemblyLinkPy(object):
    """
    This class handles document objects in Assembly
    """

    Joints: Final[list]  # ReadOnly
    """A list of all joints this assembly link has."""
