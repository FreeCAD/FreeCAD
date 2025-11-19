from PyObjectBase import PyObjectBase

from Base.Metadata import export
from DocumentObject import DocumentObject
from typing import Final

@export(
    Father="PyObjectBase",
    FatherInclude="Base/PyObjectBase.h",
    FatherNamespace="Base",
    Delete=True,
)
class DepEdge(PyObjectBase):
    FromObj: Final[DocumentObject] = None
    FromProp: Final[str] = ""
    ToObj: Final[DocumentObject] = None
    ToProp: Final[str] = ""
