from Base.Metadata import export
from TechDraw.Drawview import DrawView

@export(
    Include="Mod/TechDraw/App/DrawViewAnnotation.h",
    Namespace="TechDraw",
    FatherInclude="Mod/TechDraw/App/DrawViewPy.h",
)
class DrawViewAnnotation(DrawView):
    """
    Author: WandererFan (wandererfan@gmail.com)
    License: LGPL-2.1-or-later
    Feature for creating and manipulating Technical Drawing Annotation Views
    """
