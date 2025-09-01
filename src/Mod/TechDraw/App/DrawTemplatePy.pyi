from Base.Metadata import export

from App.DocumentObject import DocumentObject

@export(
    Father="DocumentObjectPy",
    Name="DrawTemplatePy",
    Twin="DrawTemplate",
    TwinPointer="DrawTemplate",
    Include="Mod/TechDraw/App/DrawTemplate.h",
    Namespace="TechDraw",
    FatherInclude="App/DocumentObjectPy.h",
    FatherNamespace="App",
)
class DrawTemplatePy(DocumentObject):
    """
    Feature for creating and manipulating Technical Drawing Templates
    """
