from Base.Metadata import export

from App import object

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
class DrawTemplatePy(object):
    """
    Feature for creating and manipulating Technical Drawing Templates
    """
