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

    def showSheetMdi(self) -> Any:
        """
        Create (if necessary) and switch to the Spreadsheet MDI.

        showSheetMdi()

        Returns: None
        """
        ...

    def exportAsFile(self) -> Any:
        """
        Export the sheet as a file.

        exportAsFile()

        Returns: None
        """
        ...
