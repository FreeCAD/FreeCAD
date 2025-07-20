from Base.Metadata import export
from TechDraw.Drawview import DrawView

@export(
    Father="DrawViewPy",
    Name="DrawViewAnnotationPy",
    Twin="DrawViewAnnotation",
    TwinPointer="DrawViewAnnotation",
    Include="Mod/TechDraw/App/DrawViewAnnotation.h",
    Namespace="TechDraw",
    FatherInclude="Mod/TechDraw/App/DrawViewPy.h",
    FatherNamespace="TechDraw",
)
class DrawViewAnnotationPy(DrawView):
    """
    Feature for creating and manipulating Technical Drawing Annotation Views
    """
