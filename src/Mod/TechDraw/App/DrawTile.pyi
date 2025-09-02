from Base.Metadata import export

from App.DocumentObject import DocumentObject

@export(
    Include="Mod/TechDraw/App/DrawTile.h",
    Namespace="TechDraw",
)
class DrawTile(DocumentObject):
    """
    Author: WandererFan (wandererfan@gmail.com)
    License: LGPL-2.1-or-later
    Feature for adding tiles to leader lines
    """
