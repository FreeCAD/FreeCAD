# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Base.Metadata import export, constmethod
from App.GeoFeature import GeoFeature
from App.DocumentObject import DocumentObject
from typing import TYPE_CHECKING, List, Tuple, Union

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

    if TYPE_CHECKING:
        Shape: "Part.Shape" = ...

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

    @staticmethod
    def doNamesMatch(name1: str, name2: str, /) -> bool:
        """
        Returns true when `name1` and `name2` match according to FreeCAD's design intent standard.

        `name1` and `name2` are `MappedName`s converted to strings.

        Example:
            `name1` = `g1125;_;9;SKT;0;E;0;SRC;_|_;_;23;CUT;0;E;0;MOD;_`
            `name2` = `g1125;_;9;SKT;0;E;0;SRC;_`
            According to FreeCAD's design intent standard, these two names would match, therefore
            this method would return `True`.
        """
        ...
