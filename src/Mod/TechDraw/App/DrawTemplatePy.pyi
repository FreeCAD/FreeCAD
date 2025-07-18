from typing import (
    ClassVar,
    Final,
    List,
    Dict,
    Tuple,
    TypeVar,
    Any,
    Optional,
    Union,
    overload,
)
from App import object
from Base.Metadata import export
from Base.Metadata import constmethod

@export(
    Father="DocumentObjectPy",
    Name="DrawTemplatePy",
    Twin="DrawTemplate",
    TwinPointer="DrawTemplate",
    Include="Mod/TechDraw/App/DrawTemplate.h",
    Namespace="TechDraw",
    FatherInclude="App/DocumentObjectPy.h",
    FatherNamespace="App",
)
class DrawTemplatePy(object):
    """
    Feature for creating and manipulating Technical Drawing Templates
    """
