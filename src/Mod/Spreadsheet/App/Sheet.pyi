# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from typing import Any

from Base.Metadata import export

from App.DocumentObject import DocumentObject

@export(
    Include="Mod/Spreadsheet/App/Sheet.h",
    Namespace="Spreadsheet",
    Constructor=True,
)
class Sheet(DocumentObject):
    """
    With this object you can manipulate spreadsheets

    Author: Eivind Kvedalen (eivind@kvedalen.name)
    License: LGPL-2.1-or-later
    """

    def set(self) -> Any:
        """Set data into a cell"""
        ...

    def get(self) -> Any:
        """Get evaluated cell contents"""
        ...

    def getContents(self) -> Any:
        """Get cell contents"""
        ...

    def clear(self) -> Any:
        """Clear a cell"""
        ...

    def clearAll(self) -> Any:
        """Clear all cells in the spreadsheet"""
        ...

    def importFile(self) -> Any:
        """Import file into spreadsheet"""
        ...

    def exportFile(self) -> Any:
        """Export file from spreadsheet"""
        ...

    def mergeCells(self) -> Any:
        """Merge given cell area into one cell"""
        ...

    def splitCell(self) -> Any:
        """Split a previously merged cell"""
        ...

    def insertColumns(self) -> Any:
        """Insert a given number of columns into the spreadsheet."""
        ...

    def removeColumns(self) -> Any:
        """Remove a given number of columns from the spreadsheet."""
        ...

    def insertRows(self) -> Any:
        """Insert a given number of rows into the spreadsheet."""
        ...

    def removeRows(self) -> Any:
        """Remove a given number of rows from the spreadsheet."""
        ...

    def setAlignment(self) -> Any:
        """Set alignment of the cell"""
        ...

    def getAlignment(self) -> Any:
        """Get alignment of the cell"""
        ...

    def setStyle(self) -> Any:
        """Set style of the cell"""
        ...

    def getStyle(self) -> Any:
        """Get style of the cell"""
        ...

    def setDisplayUnit(self) -> Any:
        """Set display unit for cell"""
        ...

    def setAlias(self) -> Any:
        """Set alias for cell address"""
        ...

    def getAlias(self) -> Any:
        """Get alias for cell address"""
        ...

    def getCellFromAlias(self) -> Any:
        """Get cell address given an alias"""
        ...

    def getDisplayUnit(self) -> Any:
        """Get display unit for cell"""
        ...

    def setForeground(self) -> Any:
        """Set foreground color of the cell"""
        ...

    def getForeground(self) -> Any:
        """Get foreground color of the cell"""
        ...

    def setBackground(self) -> Any:
        """Set background color of the cell"""
        ...

    def getBackground(self) -> Any:
        """Get background color of the cell"""
        ...

    def setColumnWidth(self) -> Any:
        """Set given spreadsheet column to given width"""
        ...

    def getColumnWidth(self) -> Any:
        """Get given spreadsheet column width"""
        ...

    def setRowHeight(self) -> Any:
        """Set given spreadsheet row to given height"""
        ...

    def getRowHeight(self) -> Any:
        """Get given spreadsheet row height"""
        ...

    def touchCells(self, address: str, address_to: str | None = None, /) -> None:
        """touch cells in the given range"""
        ...

    def recomputeCells(self, address: str, address_to: str | None = None, /) -> Any:
        """
        Manually recompute cells in the given range with the given order without
        following dependency order.
        """
        ...

    def getUsedCells(self) -> list[str]:
        """
        Get a list of the names of all cells that are marked as used. These cells may
        or may not have a non-empty string content.
        """
        ...

    def getNonEmptyCells(self) -> list[str]:
        """
        Get a list of the names of all cells with data in them.
        """
        ...

    def getUsedRange(self) -> tuple[str, str]:
        """
        Get a the total range of the used cells in a sheet, as a pair of strings
        representing the lowest row and column that are used, and the highest row and
        column that are used (inclusive). Note that the actual first and last cell
        of the block are not necessarily used.
        """
        ...

    def getNonEmptyRange(self) -> tuple[str, str]:
        """
        Get a the total range of the used cells in a sheet, as a pair of cell addresses
        representing the lowest row and column that contain data, and the highest row and
        column that contain data (inclusive). Note that the actual first and last cell
        of the block do not necessarily contain anything.
        """
        ...
