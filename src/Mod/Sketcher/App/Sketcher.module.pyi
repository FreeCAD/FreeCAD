# SPDX-License-Identifier: LGPL-2.1-or-later

"""Typed public signatures for the ``Sketcher`` module-level helpers.

This source-adjacent stub file carries the standalone sketch import helpers
exposed directly by the Sketcher application module.
"""

from __future__ import annotations

def open(name: str, /) -> None:
    """Open one sketch import file through the Sketcher module entry point."""
    ...

def insert(name: str, doc_name: str, /) -> None:
    """Insert one `.skf` sketch flat file into the named document."""
    ...
