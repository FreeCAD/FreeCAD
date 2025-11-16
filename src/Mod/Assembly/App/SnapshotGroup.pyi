from Base.Metadata import export

from App.DocumentObjectGroup import DocumentObjectGroup

@export(Include="Mod/Assembly/App/Groups.h", Namespace="Assembly")
class SnapshotGroup(DocumentObjectGroup):
    """
    This class is a group subclass for snapshots.

    Author: Pierre-Louis Boyer (hello@astocad.com)
    License: LGPL-2.1-or-later
    """
