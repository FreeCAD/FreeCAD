from Base.Metadata import export
from Base.Persistence import Persistence

@export(
    Father="PersistencePy",
    Name="PropertyColumnWidthsPy",
    Twin="PropertyColumnWidths",
    TwinPointer="PropertyColumnWidths",
    Include="Mod/Spreadsheet/App/PropertyColumnWidths.h",
    Namespace="Spreadsheet",
    FatherInclude="Base/PersistencePy.h",
    FatherNamespace="Base",
    Constructor=True,
)
class PropertyColumnWidthsPy(Persistence):
    """
    Internal spreadsheet object
    """
