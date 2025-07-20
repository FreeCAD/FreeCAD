from Base.Metadata import export

from App.DocumentObjectGroup import DocumentObjectGroup

@export(
    Father="DocumentObjectGroupPy",
    Name="ViewGroupPy",
    Twin="ViewGroup",
    TwinPointer="ViewGroup",
    Include="Mod/Assembly/App/ViewGroup.h",
    Namespace="Assembly",
    FatherInclude="App/DocumentObjectGroupPy.h",
    FatherNamespace="App",
)
class ViewGroupPy(DocumentObjectGroup):
    """
    This class is a group subclass for joints.
    """
