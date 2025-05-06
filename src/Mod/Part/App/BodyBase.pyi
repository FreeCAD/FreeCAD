from Base.Metadata import export, constmethod
from PartFeature import PartFeature


@export(
    Twin="BodyBase",
    TwinPointer="BodyBase",
    Include="Mod/Part/App/BodyBase.h",
    Namespace="Part",
    FatherInclude="Mod/Part/App/PartFeaturePy.h",
    FatherNamespace="Part",
)
class BodyBase(PartFeature):
    """
    Base class of all Body objects

    Author: Juergen Riegel (FreeCAD@juergen-riegel.net)
    Licence: LGPL
    """

    ...
