from Base.Metadata import export

from App import object

@export(
    Father="DocumentObjectPy",
    Name="DrawTilePy",
    Twin="DrawTile",
    TwinPointer="DrawTile",
    Include="Mod/TechDraw/App/DrawTile.h",
    Namespace="TechDraw",
    FatherInclude="App/DocumentObjectPy.h",
    FatherNamespace="App",
)
class DrawTilePy(object):
    """
    Feature for adding tiles to leader lines
    """
