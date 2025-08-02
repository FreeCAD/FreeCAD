from typing import Any

from Base.Metadata import export
from Base.PyObjectBase import PyObjectBase

@export(
    Father="PyObjectBase",
    Name="StepShapePy",
    Twin="StepShape",
    TwinPointer="StepShape",
    Include="Mod/Import/App/StepShape.h",
    Namespace="Import",
    FatherInclude="Base/PyObjectBase.h",
    FatherNamespace="Base",
    Constructor=True,
    Delete=True,
)
class StepShapePy(PyObjectBase):
    """
    StepShape in Import
    This class gives a interface to retrieve TopoShapes out of an loaded STEP file of any kind.
    """

    def read(self) -> Any:
        """method read()
        Read a STEP file into memory and make it accessible"""
        ...
