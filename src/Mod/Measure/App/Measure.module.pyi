# SPDX-License-Identifier: LGPL-2.1-or-later

"""Typed public signatures for the ``Measure`` module-level helpers.

This source-adjacent stub file carries the shape-resolution helper exposed
directly by the Measure application module.
"""

from __future__ import annotations

from FreeCAD import DocumentObject
from Part import Shape

def getLocatedTopoShape(root_object: DocumentObject, subname: str, /) -> Shape | None:
    """Resolve one document object subshape with its accumulated placement applied."""
    ...
