# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Base.Metadata import export, constmethod
from Base.BaseClass import BaseClass
from typing import Any, Final, overload, Dict


@export(
    Constructor=True,
    Reference=True,
)
class StringHasher(BaseClass):
    """
    This is the StringHasher class

    Author: Zheng, Lei (realthunder.dev@gmail.com)
    Licence: LGPL
    """

    @overload
    def getID(self, txt: str, base64: bool = False, /) -> Any:
        ...

    @overload
    def getID(self, id: int, base64: bool = False, /) -> Any:
        ...

    def getID(self, arg: Any, base64: bool = False, /) -> Any:
        """
        If the input is text, return a StringID object that is unique within this hasher. This
        StringID object is reference counted. The hasher may only save hash ID's that are used.

        If the input is an integer, then the hasher will try to find the StringID object stored
        with the same integer value.

        base64: indicate if the input 'txt' is base64 encoded binary data
        """
        ...

    @constmethod
    def isSame(self, other: "StringHasher", /) -> bool:
        """
        Check if two hasher are the same
        """
        ...

    Count: Final[int] = 0
    """Return count of used hashes"""

    Size: Final[int] = 0
    """Return the size of the hashes"""

    SaveAll: bool = False
    """Whether to save all string hashes regardless of its use count"""

    Threshold: int = 0
    """Data length exceed this threshold will be hashed before storing"""

    Table: Final[Dict[int, str]] = {}
    """Return the entire string table as Int->String dictionary"""
