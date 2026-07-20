# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from typing import Any

from Base.Metadata import constmethod, export, sequence_protocol
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

    Author: Eivind Kvedalen (eivind@kvedalen.name)
    License: LGPL-2.1-or-later
    """

    @constmethod
    def keys(self) -> Any:
        """Get all cell names"""
        ...
