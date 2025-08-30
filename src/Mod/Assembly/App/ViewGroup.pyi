from Base.Metadata import export

from App.DocumentObjectGroup import DocumentObjectGroup

@export(Include="Mod/Assembly/App/ViewGroup.h", Namespace="Assembly")
class ViewGroup(DocumentObjectGroup):
    """
    This class is a group subclass for joints.
    """
