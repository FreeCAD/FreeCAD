from Base.Metadata import export

from App.DocumentObjectGroup import DocumentObjectGroup

@export(Include="Mod/Assembly/App/SimulationGroup.h", Namespace="Assembly")
class SimulationGroup(DocumentObjectGroup):
    """
    This class is a group subclass for joints.
    """
