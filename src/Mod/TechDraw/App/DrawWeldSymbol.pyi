from Base.Metadata import export
from TechDraw.DrawView import DrawView

@export(
    Include="Mod/TechDraw/App/DrawWeldSymbol.h",
    Namespace="TechDraw",
    FatherInclude="Mod/TechDraw/App/DrawViewPy.h",
)
class DrawWeldSymbol(DrawView):
    """
    Author: WandererFan (wandererfan@gmail.com)
    License: LGPL-2.1-or-later
    Feature for adding welding tiles to leader lines
    """
