from Base.Metadata import export
from Base.Persistence import Persistence

@export(
    Include="Mod/Spreadsheet/App/PropertyColumnWidths.h",
    Namespace="Spreadsheet",
    Constructor=True,
)
class PropertyColumnWidths(Persistence):
    """
    Internal spreadsheet object
    """
