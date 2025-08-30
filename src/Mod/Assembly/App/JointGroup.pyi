from Base.Metadata import export

from App.DocumentObjectGroup import DocumentObjectGroup

@export(Include="Mod/Assembly/App/JointGroup.h", Namespace="Assembly")
class JointGroup(DocumentObjectGroup):
    """
    This class is a group subclass for joints.
    """
