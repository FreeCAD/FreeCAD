from typing import ClassVar, Final, List, Dict, Tuple, TypeVar, Any, Optional, Union, overload
from App import object
from Base.Metadata import export
from Base.Metadata import constmethod

@export(
    Father="DocumentObjectExtensionPy",
    Name="CosmeticExtensionPy",
    Twin="CosmeticExtension",
    TwinPointer="CosmeticExtension",
    Include="Mod/TechDraw/App/CosmeticExtension.h",
    Namespace="TechDraw",
    FatherInclude="App/DocumentObjectExtensionPy.h",
    FatherNamespace="App",

)
class CosmeticExtensionPy(object):
    """
        This object represents cosmetic features for a DrawViewPart.
    """


