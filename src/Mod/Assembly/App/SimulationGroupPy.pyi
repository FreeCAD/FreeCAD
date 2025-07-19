from App import object
from Base.Metadata import export

@export(
    Father="DocumentObjectGroupPy",
    Name="SimulationGroupPy",
    Twin="SimulationGroup",
    TwinPointer="SimulationGroup",
    Include="Mod/Assembly/App/SimulationGroup.h",
    Namespace="Assembly",
    FatherInclude="App/DocumentObjectGroupPy.h",
    FatherNamespace="App",
)
class SimulationGroupPy(object):
    """
    This class is a group subclass for joints.
    """
