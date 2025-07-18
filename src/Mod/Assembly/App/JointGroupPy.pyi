from App import object
from Base.Metadata import export

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
class JointGroupPy(object):
    """
    This class is a group subclass for joints.
    """
