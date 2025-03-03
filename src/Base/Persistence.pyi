from Metadata import constmethod
from BaseClass import BaseClass
from typing import Final

class Persistence(BaseClass):
    """
    Base.Persistence class.

    Class to dump and restore the content of an object.

    Author: Juergen Riegel (FreeCAD@juergen-riegel.net)
    Licence: LGPL
    """

    Content: Final[str] = ""
    """Content of the object in XML representation."""

    MemSize: Final[int] = 0
    """Memory size of the object in bytes."""

    @constmethod
    def dumpContent(self, *, Compression: int = 3) -> bytearray:
        """
        dumpContent(Compression=3) -> bytearray

        Dumps the content of the object, both the XML representation and the additional
        data files required, into a byte representation.

        Compression : int
            Set the data compression level in the range [0,9]. Set to 0 for no compression.
        """
        ...

    def restoreContent(self, obj: object) -> None:
        # TODO: Starting with Python 3.12, collections.abc.Buffer can be used for type hinting
        """
        restoreContent(obj) -> None

        Restore the content of the object from a byte representation as stored by `dumpContent`.
        It could be restored from any Python object implementing the buffer protocol.

        obj : buffer
            Object with buffer protocol support.
        """
        ...
