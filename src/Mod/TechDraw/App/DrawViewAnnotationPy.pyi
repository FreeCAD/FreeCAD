from typing import ClassVar, Final, List, Dict, Tuple, TypeVar, Any, Optional, Union, overload
from TechDraw import object
from Base.Metadata import export
from Base.Metadata import constmethod

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
class DrawViewAnnotationPy(object):
    """
        Feature for creating and manipulating Technical Drawing Annotation Views
    """


