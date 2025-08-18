from Base.Metadata import export

from App.DocumentObjectGroup import DocumentObjectGroup

@export(
    Father="DocumentObjectGroupPy",
    Name="JointGroupPy",
    Twin="JointGroup",
    TwinPointer="JointGroup",
    Include="Mod/Assembly/App/JointGroup.h",
    Namespace="Assembly",
    FatherInclude="App/DocumentObjectGroupPy.h",
    FatherNamespace="App",
)
class JointGroupPy(DocumentObjectGroup):
    """
    This class is a group subclass for joints.
    """
