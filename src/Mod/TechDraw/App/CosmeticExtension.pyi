from Base.Metadata import export

from App.DocumentObjectExtension import DocumentObjectExtension

@export(
    Include="Mod/TechDraw/App/CosmeticExtension.h",
    Namespace="TechDraw",
)
class CosmeticExtension(DocumentObjectExtension):
    """
    Author: WandererFan (wandererfan@gmail.com)
    License: LGPL-2.1-or-later
    This object represents cosmetic features for a DrawViewPart.
    """
