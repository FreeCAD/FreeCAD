from Base.Metadata import export, sequence_protocol
from Base.Persistence import Persistence

@export(
    Include="Mod/Spreadsheet/App/PropertySheet.h",
    Namespace="Spreadsheet",
    Constructor=True,
)
@sequence_protocol(
    mp_subscript="true",
)
class PropertySheet(Persistence):
    """
    Internal spreadsheet object
    """
