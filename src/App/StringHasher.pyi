# SPDX-License-Identifier: LGPL-2.1-or-later

from Base.Metadata import export, constmethod
from Base.BaseClass import BaseClass
from StringID import StringID
from typing import Final, overload


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
    def getID(self, txt: str, base64: bool = False, /) -> StringID: ...
    @overload
    def getID(self, id: int, index: int = 0, /) -> StringID | None: ...
    @overload
    def getID(self, arg: str | int, base64: bool = False, /) -> StringID | None:
        """
        If the input is text, return a StringID object that is unique within this hasher. This
        StringID object is reference counted. The hasher may only save hash ID's that are used.

        If the input is an integer, then the hasher will try to find the StringID object stored
        with the same integer value.

        base64: indicate if the input 'txt' is base64 encoded binary data
        """

    @constmethod
    def isSame(self, other: StringHasher, /) -> bool:
        """
        Check if two hasher are the same
        """

    Count: Final[int]
    """Return count of used hashes"""

    Size: Final[int]
    """Return the size of the hashes"""

    SaveAll: bool
    """Whether to save all string hashes regardless of its use count"""

    Threshold: int
    """Data length exceed this threshold will be hashed before storing"""

    Table: Final[dict[int, str]]
    """Return the entire string table as Int->String dictionary"""
