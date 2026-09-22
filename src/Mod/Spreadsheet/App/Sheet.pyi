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

    def set(self, address: str, contents: str, /) -> None:
        """Set data into a cell"""
        ...

    def get(self, address: str, address_to: str | None = ..., /) -> Any:
        """Get evaluated cell contents"""
        ...

    def getContents(self, address: str, /) -> str:
        """Get cell contents"""
        ...

    def clear(self, address: str, all: bool = ..., /) -> None:
        """Clear a cell"""
        ...

    def clearAll(self) -> None:
        """Clear all cells in the spreadsheet"""
        ...

    def importFile(
        self,
        filename: str,
        delimiter: str = ...,
        quote_char: str = ...,
        escape_char: str = ...,
        /,
    ) -> bool:
        """Import file into spreadsheet"""
        ...

    def exportFile(
        self,
        filename: str,
        delimiter: str = ...,
        quote_char: str = ...,
        escape_char: str = ...,
        /,
    ) -> bool:
        """Export file from spreadsheet"""
        ...

    def mergeCells(self, range: str, /) -> None:
        """Merge given cell area into one cell"""
        ...

    def splitCell(self, address: str, /) -> None:
        """Split a previously merged cell"""
        ...

    def insertColumns(self, column: str, count: int, /) -> None:
        """Insert a given number of columns into the spreadsheet."""
        ...

    def removeColumns(self, column: str, count: int, /) -> None:
        """Remove a given number of columns from the spreadsheet."""
        ...

    def insertRows(self, row: str, count: int, /) -> None:
        """Insert a given number of rows into the spreadsheet."""
        ...

    def removeRows(self, row: str, count: int, /) -> None:
        """Remove a given number of rows from the spreadsheet."""
        ...

    def setAlignment(self, cell: str, value: str | set[str], options: str = ..., /) -> None:
        """Set alignment of the cell"""
        ...

    def getAlignment(self, address: str, /) -> set[str] | None:
        """Get alignment of the cell"""
        ...

    def setStyle(self, cell: str, value: str | set[str], options: str = ..., /) -> None:
        """Set style of the cell"""
        ...

    def getStyle(self, address: str, /) -> set[str] | None:
        """Get style of the cell"""
        ...

    def setDisplayUnit(self, cell: str, value: str, /) -> None:
        """Set display unit for cell"""
        ...

    def setAlias(self, address: str, value: str | None, /) -> None:
        """Set alias for cell address"""
        ...

    def getAlias(self, address: str, /) -> str | None:
        """Get alias for cell address"""
        ...

    def getCellFromAlias(self, alias: str, /) -> str | None:
        """Get cell address given an alias"""
        ...

    def getDisplayUnit(self, address: str, /) -> str | None:
        """Get display unit for cell"""
        ...

    def setForeground(self, address: str, value: tuple[float, ...], /) -> None:
        """Set foreground color of the cell"""
        ...

    def clearForeground(self, address: str, /) -> None:
        """Clears foreground color of the cell"""
        ...

    def getForeground(self, address: str, /) -> tuple[float, float, float, float] | None:
        """Get foreground color of the cell"""
        ...

    def setBackground(self, address: str, value: tuple[float, ...], /) -> None:
        """Set background color of the cell"""
        ...

    def clearBackground(self, address: str, /) -> None:
        """Clears background color of the cell"""
        ...

    def getBackground(self, address: str, /) -> tuple[float, float, float, float] | None:
        """Get background color of the cell"""
        ...

    def setColumnWidth(self, column: str, width: int, /) -> None:
        """Set given spreadsheet column to given width"""
        ...

    def getColumnWidth(self, column: str, /) -> int:
        """Get given spreadsheet column width"""
        ...

    def setRowHeight(self, row: str, height: int, /) -> None:
        """Set given spreadsheet row to given height"""
        ...

    def getRowHeight(self, row: str, /) -> int:
        """Get given spreadsheet row height"""
        ...

    def touchCells(self, address: str, address_to: str | None = None, /) -> None:
        """touch cells in the given range"""
        ...

    def recomputeCells(self, address: str, address_to: str | None = ..., /) -> None:
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

    def getUsedRange(self) -> tuple[str, str] | None:
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
