from Base.Metadata import export

from App.DocumentObjectExtension import DocumentObjectExtension

@export(
    Include="Mod/TechDraw/App/CosmeticExtension.h",
    Namespace="TechDraw",
)
class CosmeticExtension(DocumentObjectExtension):
    """
    This object represents cosmetic features for a DrawViewPart.
    """
