from Base.Metadata import export, constmethod, class_declarations
from Base.BaseClass import BaseClass
from typing import Any, Final, List


@export(
    Include="App/StringHasher.h",
    Reference=True,
)
@class_declarations("""private:
    friend class StringID;
    int _index = 0;
        """
)
class StringID(BaseClass):
    """
    This is the StringID class

    Author: Zheng, Lei (realthunder.dev@gmail.com)
    Licence: LGPL
    """

    @constmethod
    def isSame(self, other: "StringID") -> bool:
        """
        Check if two StringIDs are the same
        """
        ...

    Value: Final[int] = 0
    """Return the integer value of this ID"""

    Related: Final[List[Any]] = []
    """Return the related string IDs"""

    Data: Final[str] = ""
    """Return the data associated with this ID"""

    IsBinary: Final[bool] = False
    """Check if the data is binary,"""

    IsHashed: Final[bool] = False
    """Check if the data is hash, if so 'Data' returns a base64 encoded string of the raw hash"""

    Index: int = 0
    """Geometry index. Only meaningful for geometry element name"""
