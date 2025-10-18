# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from typing import Any

from Base.Metadata import export

from Gui.ViewProviderDocumentObject import ViewProviderDocumentObject

@export(
    Twin="ViewProviderSheet",
    TwinPointer="ViewProviderSheet",
    Include="Mod/Spreadsheet/Gui/ViewProviderSpreadsheet.h",
    Namespace="SpreadsheetGui",
)
class ViewProviderSpreadsheet(ViewProviderDocumentObject):
    """
    ViewProviderSheet class

    Author: Jose Luis Cercos Pita (jlcercos@gmail.com)
    License: LGPL-2.1-or-later
    """

    def getView(self) -> Any:
        """Get access to the sheet view"""
        ...

    def showSheetMdi(self) -> None:
        """Create (if necessary) and switch to the Spreadsheet MDI."""
        ...

    def exportAsFile(self) -> None:
        """Export the sheet as a file."""
        ...
