# SPDX-License-Identifier: LGPL-2.1-or-later

"""Typed public signatures for the ``PathGui`` module-level helpers.

This source-adjacent stub file carries the GUI-assisted G-code import and
export helpers exposed directly by the CAM GUI module.
"""

from __future__ import annotations

from collections.abc import Sequence
from typing import overload

from FreeCAD import DocumentObject

def open(filename: str, /) -> None:
    """Open one G-code file through the GUI processor chooser."""
    ...

@overload
def insert(filename: str, /) -> None:
    """Insert one G-code file into the active document through the GUI processor chooser."""
    ...

@overload
def insert(filename: str, doc_name: str, /) -> None:
    """Insert one G-code file into the named document through the GUI processor chooser."""
    ...

def export(objects: Sequence[DocumentObject], filename: str, /) -> None:
    """Export selected path objects through the GUI post-processor chooser."""
    ...
