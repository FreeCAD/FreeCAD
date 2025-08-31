from Base.Metadata import export
from TechDraw.DrawView import DrawView

@export(
    Include="Mod/TechDraw/App/DrawWeldSymbol.h",
    Namespace="TechDraw",
    FatherInclude="Mod/TechDraw/App/DrawViewPy.h",
)
class DrawWeldSymbol(DrawView):
    """
    Feature for adding welding tiles to leader lines
    """
