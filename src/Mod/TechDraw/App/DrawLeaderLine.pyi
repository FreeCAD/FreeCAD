from Base.Metadata import export
from DrawView import DrawView

@export(
    Include="Mod/TechDraw/App/DrawLeaderLine.h",
    Namespace="TechDraw",
    FatherInclude="Mod/TechDraw/App/DrawViewPy.h",
)
class DrawLeaderLine(DrawView):
    """
    Feature for adding leaders to Technical Drawings
    
    Author: WandererFan (wandererfan@gmail.com)
    License: LGPL-2.1-or-later
    """
