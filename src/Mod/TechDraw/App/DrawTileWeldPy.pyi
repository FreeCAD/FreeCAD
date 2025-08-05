from Base.Metadata import export
from TechDraw.DrawTile import DrawTile

@export(
    Father="DrawTilePy",
    Name="DrawTileWeldPy",
    Twin="DrawTileWeld",
    TwinPointer="DrawTileWeld",
    Include="Mod/TechDraw/App/DrawTileWeld.h",
    Namespace="TechDraw",
    FatherInclude="Mod/TechDraw/App/DrawTilePy.h",
    FatherNamespace="TechDraw",
)
class DrawTileWeldPy(DrawTile):
    """
    Feature for adding welding tiles to leader lines
    """
