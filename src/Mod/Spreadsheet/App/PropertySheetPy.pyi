from Base.Metadata import export, sequence_protocol
from Base.Persistence import Persistence

@export(
    Father="PersistencePy",
    Name="PropertySheetPy",
    Twin="PropertySheet",
    TwinPointer="PropertySheet",
    Include="Mod/Spreadsheet/App/PropertySheet.h",
    Namespace="Spreadsheet",
    FatherInclude="Base/PersistencePy.h",
    FatherNamespace="Base",
    Constructor=True,
)
@sequence_protocol(
    mp_subscript="true",
)
class PropertySheetPy(Persistence):
    """
    Internal spreadsheet object
    """
