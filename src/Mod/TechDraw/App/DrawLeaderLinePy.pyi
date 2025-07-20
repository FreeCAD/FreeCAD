from Base.Metadata import export
from DrawView import DrawView

@export(
    Father="DrawViewPy",
    Name="DrawLeaderLinePy",
    Twin="DrawLeaderLine",
    TwinPointer="DrawLeaderLine",
    Include="Mod/TechDraw/App/DrawLeaderLine.h",
    Namespace="TechDraw",
    FatherInclude="Mod/TechDraw/App/DrawViewPy.h",
    FatherNamespace="TechDraw",
)
class DrawLeaderLinePy(DrawView):
    """
    Feature for adding leaders to Technical Drawings
    """
