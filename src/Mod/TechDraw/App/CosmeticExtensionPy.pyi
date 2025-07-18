from Base.Metadata import export

from App import object

@export(
    Father="DocumentObjectExtensionPy",
    Name="CosmeticExtensionPy",
    Twin="CosmeticExtension",
    TwinPointer="CosmeticExtension",
    Include="Mod/TechDraw/App/CosmeticExtension.h",
    Namespace="TechDraw",
    FatherInclude="App/DocumentObjectExtensionPy.h",
    FatherNamespace="App",
)
class CosmeticExtensionPy(object):
    """
    This object represents cosmetic features for a DrawViewPart.
    """
