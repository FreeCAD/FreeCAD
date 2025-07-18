from App import object
from Base.Metadata import export

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
class ViewGroupPy(object):
    """
    This class is a group subclass for joints.
    """
