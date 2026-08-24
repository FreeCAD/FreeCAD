# SPDX-License-Identifier: LGPL-2.1-or-later

"""Typed public signatures for the ``SpreadsheetGui`` module-level helpers.

This source-adjacent stub file carries the spreadsheet file import helpers
exposed directly by the Spreadsheet GUI module.
"""

from __future__ import annotations

from typing import overload

@overload
def open(name: str, /) -> None:
    """Open one tab-delimited text file into a new spreadsheet document."""
    ...

@overload
def open(name: str, doc_name: str, /) -> None:
    """Open one tab-delimited text file into a newly created named document."""
    ...

@overload
def insert(name: str, /) -> None:
    """Insert one tab-delimited text file into the active document, creating one if needed."""
    ...

@overload
def insert(name: str, doc_name: str, /) -> None:
    """Insert one tab-delimited text file into the named document, creating it if needed."""
    ...
