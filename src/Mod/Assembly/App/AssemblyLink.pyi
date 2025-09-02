from typing import Final

from Base.Metadata import export

from App.Part import Part

@export(
    Include="Mod/Assembly/App/AssemblyLink.h",
    Namespace="Assembly",
)
class AssemblyLink(Part):
    """
    This class handles document objects in Assembly
    """

    Joints: Final[list]
    """A list of all joints this assembly link has."""
