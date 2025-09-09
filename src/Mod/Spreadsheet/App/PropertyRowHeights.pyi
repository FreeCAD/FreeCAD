from Base.Metadata import export
from Base.Persistence import Persistence

@export(
    Include="Mod/Spreadsheet/App/PropertyRowHeights.h",
    Namespace="Spreadsheet",
    Constructor=True,
)
class PropertyRowHeights(Persistence):
    """
    Internal spreadsheet object

    Author: Eivind Kvedalen (eivind@kvedalen.name)
    License: LGPL-2.1-or-later
    """
