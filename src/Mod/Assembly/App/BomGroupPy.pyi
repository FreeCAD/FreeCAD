from Base.Metadata import export

from App.DocumentObjectGroup import DocumentObjectGroup

@export(
    Father="DocumentObjectGroupPy",
    Name="BomGroupPy",
    Twin="BomGroup",
    TwinPointer="BomGroup",
    Include="Mod/Assembly/App/BomGroup.h",
    Namespace="Assembly",
    FatherInclude="App/DocumentObjectGroupPy.h",
    FatherNamespace="App",
)
class BomGroupPy(DocumentObjectGroup):
    """
    This class is a group subclass for boms.
    """
