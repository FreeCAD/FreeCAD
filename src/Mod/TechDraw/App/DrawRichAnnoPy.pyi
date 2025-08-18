from Base.Metadata import export
from TechDraw.DrawView import DrawView

@export(
    Father="DrawViewPy",
    Name="DrawRichAnnoPy",
    Twin="DrawRichAnno",
    TwinPointer="DrawRichAnno",
    Include="Mod/TechDraw/App/DrawRichAnno.h",
    Namespace="TechDraw",
    FatherInclude="Mod/TechDraw/App/DrawViewPy.h",
    FatherNamespace="TechDraw",
)
class DrawRichAnnoPy(DrawView):
    """
    Feature for adding rich annotation blocks to Technical Drawings
    """
