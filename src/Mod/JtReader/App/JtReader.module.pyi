# SPDX-License-Identifier: LGPL-2.1-or-later

"""Typed public signatures for the ``JtReader`` module-level helpers.

This source-adjacent stub file carries the JT file reader helpers exposed
directly by the JtReader application module.
"""

from __future__ import annotations

def read(name: str, /) -> None:
    """Read one JT file through the reader backend without returning public geometry."""
    ...

def open(name: str, /) -> None:
    """Open one JT file into a new document."""
    ...

def insert(name: str, doc_name: str, /) -> None:
    """Insert one JT file into the named document."""
    ...
