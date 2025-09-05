from Base.Metadata import export
from TechDraw.DrawView import DrawView

@export(
    Father="DrawViewPy",
    Name="DrawWeldSymbolPy",
    Twin="DrawWeldSymbol",
    TwinPointer="DrawWeldSymbol",
    Include="Mod/TechDraw/App/DrawWeldSymbol.h",
    Namespace="TechDraw",
    FatherInclude="Mod/TechDraw/App/DrawViewPy.h",
    FatherNamespace="TechDraw",
)
class DrawWeldSymbolPy(DrawView):
    """
    Feature for adding welding tiles to leader lines
    """
