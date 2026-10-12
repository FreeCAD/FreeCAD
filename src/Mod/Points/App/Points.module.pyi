# SPDX-License-Identifier: LGPL-2.1-or-later

"""Typed public signatures for the ``Points`` module-level helpers.

This source-adjacent stub file carries the point-cloud file I/O and document
helpers exposed directly by the Points application module.
"""

from __future__ import annotations

from collections.abc import Sequence

from FreeCAD import DocumentObject

def open(name: str, /) -> None:
    """Open one ASC, E57, PLY, or PCD point-cloud file into a new document."""
    ...

def insert(name: str, doc_name: str, /) -> None:
    """Insert one ASC, E57, PLY, or PCD point-cloud file into the named document."""
    ...

def export(objectList: Sequence[DocumentObject], filename: str, /) -> None:
    """Export the first selected points feature to one ASC, PLY, or PCD file."""
    ...

def show(points: Points, name: str = "Points", /) -> DocumentObject:
    """Create one document object from one `Points` kernel in the active document."""
    ...
