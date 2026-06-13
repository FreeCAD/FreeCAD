# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Base.Metadata import export, constmethod
from Base.PyObjectBase import PyObjectBase


@export(
    Constructor=False,
    Delete=True,
)
class DocumentSettings(PyObjectBase):
    """
    Namespaced document settings backed by the owning document's Meta map.
    """

    @constmethod
    def getString(self, key: str, default: str = "", /) -> str:
        """
        Return a string setting, or default when the key is missing.
        """
        ...

    def setString(self, key: str, value: str, /) -> None:
        """
        Store a string setting.
        """
        ...

    @constmethod
    def getInt(self, key: str, default: int = 0, /) -> int:
        """
        Return an integer setting, or default when the key is missing or invalid.
        """
        ...

    def setInt(self, key: str, value: int, /) -> None:
        """
        Store an integer setting.
        """
        ...

    @constmethod
    def getFloat(self, key: str, default: float = 0.0, /) -> float:
        """
        Return a float setting, or default when the key is missing or invalid.
        """
        ...

    def setFloat(self, key: str, value: float, /) -> None:
        """
        Store a float setting.
        """
        ...

    @constmethod
    def getBool(self, key: str, default: bool = False, /) -> bool:
        """
        Return a bool setting, or default when the key is missing or invalid.
        """
        ...

    def setBool(self, key: str, value: bool, /) -> None:
        """
        Store a bool setting.
        """
        ...

    def remove(self, key: str, /) -> None:
        """
        Remove one setting from this namespace.
        """
        ...

    @constmethod
    def keys(self) -> list[str]:
        """
        Return setting keys present in this namespace.
        """
        ...
