from Base.Metadata import export

from App.DocumentObject import DocumentObject

@export(
    Include="Mod/TechDraw/App/DrawTemplate.h",
    Namespace="TechDraw",
)
class DrawTemplate(DocumentObject):
    """
    Feature for creating and manipulating Technical Drawing Templates
    """
