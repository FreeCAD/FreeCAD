# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Metadata import constmethod
from PyObjectBase import PyObjectBase
from typing import List, Final

class BaseClass(PyObjectBase):
    """
    This is the base class

    Author: Juergen Riegel (FreeCAD@juergen-riegel.net)
    Licence: LGPL
    """

    TypeId: Final[str] = ""
    """Is the type of the FreeCAD object with module domain"""

    Module: Final[str] = ""
    """Module in which this class is defined"""

    @constmethod
    def isDerivedFrom(self, typeName: str, /) -> bool:
        """
        Returns true if given type is a father
        """
        ...

    @constmethod
    def getAllDerivedFrom(self) -> List[object]:
        """
        Returns all descendants
        """
        ...
