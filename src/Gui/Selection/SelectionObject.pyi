# SPDX-License-Identifier: LGPL-2.1-or-later

from App.Document import Document as AppDocument
from App.DocumentObject import DocumentObject
from Base.Metadata import export
from Base.BaseClass import BaseClass
from Base.Vector import Vector
from typing import Any, Final

@export(
    Include="Gui/Selection/SelectionObject.h",
    Delete=True,
)
class SelectionObject(BaseClass):
    """
    This class represents selections made by the user. It holds information about the object, document and sub-element of the selection.

    Author: Juergen Riegel (FreeCAD@juergen-riegel.net)
    Licence: LGPL
    """

    def remove(self) -> None:
        """
        Remove this selection item from the selection.

        --
        This object becomes invalid.
        """

    def isObjectTypeOf(self, type: str, /) -> bool:
        """
        Test for a certain father class.
        """
    ObjectName: Final[str]
    """Name of the selected object"""

    SubElementNames: Final[tuple[str, ...]]
    """Name of the selected sub-element if any"""

    FullName: Final[str]
    """Name of the selected object"""

    TypeName: Final[str]
    """Type name of the selected object"""

    DocumentName: Final[str]
    """Name of the document of the selected object"""

    Document: Final[AppDocument]
    """Document of the selected object"""

    Object: Final[DocumentObject]
    """Selected object"""

    SubObjects: Final[tuple[Any, ...]]
    """Selected sub-element, if any"""

    PickedPoints: Final[tuple[Vector, ...]]
    """Picked points for selection"""

    HasSubObjects: Final[bool]
    """Selected sub-element, if any"""
