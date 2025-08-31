from Base.Metadata import export

from App.DocumentObject import DocumentObject

@export(
    Include="Mod/TechDraw/App/DrawTile.h",
    Namespace="TechDraw",
)
class DrawTile(DocumentObject):
    """
    Feature for adding tiles to leader lines
    """
