from App import object
from Base.Metadata import export

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
class BomGroupPy(object):
    """
    This class is a group subclass for boms.
    """
