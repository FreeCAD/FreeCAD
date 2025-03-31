from Base.Metadata import export, constmethod
from App.GeoFeature import GeoFeature
from App.DocumentObject import DocumentObject
from typing import List, Tuple, Union


@export(
    Twin="Feature",
    TwinPointer="Feature",
    Include="Mod/Part/App/PartFeature.h",
    FatherInclude="App/GeoFeaturePy.h",
)
class PartFeature(GeoFeature):
    """
    This is the father of all shape object classes

    Author: Juergen Riegel (FreeCAD@juergen-riegel.net)
    Licence: LGPL
    """

    @constmethod
    def getElementHistory(
        self,
        name: str,
        *,
        recursive: bool = True,
        sameType: bool = False,
        showName: bool = False,
    ) -> Union[
        Tuple[DocumentObject, str, List[str]],
        List[Tuple[DocumentObject, str, List[str]]],
    ]:
        """
        getElementHistory(name,recursive=True,sameType=False,showName=False) - returns the element mapped name history

        name: mapped element name belonging to this shape
        recursive: if True, then track back the history through other objects till the origin
        sameType: if True, then stop trace back when element type changes
        showName: if False, return the owner object, or else return a tuple of object name and label

        If not recursive, then return tuple(sourceObject, sourceElementName, [intermediateNames...]),
        otherwise return a list of tuple.
        """
        ...
