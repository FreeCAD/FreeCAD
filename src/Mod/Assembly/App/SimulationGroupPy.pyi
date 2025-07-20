from Base.Metadata import export

from App.DocumentObjectGroup import DocumentObjectGroup

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
class SimulationGroupPy(DocumentObjectGroup):
    """
    This class is a group subclass for joints.
    """
