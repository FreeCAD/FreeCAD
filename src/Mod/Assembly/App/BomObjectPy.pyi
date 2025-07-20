from Base.Metadata import export
from Spreadsheet.Sheet import Sheet

@export(
    Father="SheetPy",
    Name="BomObjectPy",
    Twin="BomObject",
    TwinPointer="BomObject",
    Include="Mod/Assembly/App/BomObject.h",
    Namespace="Assembly",
    FatherInclude="Mod/Spreadsheet/App/SheetPy.h",
    FatherNamespace="Spreadsheet",
)
class BomObjectPy(Sheet):
    """
    This class is the BOM object of assemblies, it derives from Spreadsheet::Sheet.
    """
