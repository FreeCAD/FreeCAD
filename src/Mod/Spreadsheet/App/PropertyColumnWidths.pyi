# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

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

    Author: Eivind Kvedalen (eivind@kvedalen.name)
    License: LGPL-2.1-or-later
    """
