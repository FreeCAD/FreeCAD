from typing import Any

from Base.Metadata import export, sequence_protocol
from Base.Persistence import Persistence

@export(
    Father="PersistencePy",
    Name="PropertyRowHeightsPy",
    Twin="PropertyRowHeights",
    TwinPointer="PropertyRowHeights",
    Include="Mod/Spreadsheet/App/PropertyRowHeights.h",
    Namespace="Spreadsheet",
    FatherInclude="Base/PersistencePy.h",
    FatherNamespace="Base",
    Constructor=True,
)
class PropertyRowHeightsPy(Persistence):
    """
    Internal spreadsheet object
    """
