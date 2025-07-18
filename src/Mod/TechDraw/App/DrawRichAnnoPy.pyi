from typing import ClassVar, Final, List, Dict, Tuple, TypeVar, Any, Optional, Union, overload
from TechDraw import object
from Base.Metadata import export
from Base.Metadata import constmethod

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
class DrawRichAnnoPy(object):
    """
        Feature for adding rich annotation blocks to Technical Drawings
    """


