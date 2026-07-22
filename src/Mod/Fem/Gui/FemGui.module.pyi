# SPDX-License-Identifier: LGPL-2.1-or-later

"""Typed public signatures for the ``FemGui`` module-level helpers.

This source-adjacent stub file carries the active-analysis selector and the
GUI editor helpers exposed directly by the Fem GUI module.
"""

from __future__ import annotations

from typing import overload

from FreeCAD import DocumentObject

@overload
def setActiveAnalysis() -> None:
    """Clear the currently active FEM analysis object."""
    ...

@overload
def setActiveAnalysis(obj: DocumentObject, /) -> None:
    """Set the active FEM analysis object."""
    ...

def getActiveAnalysis() -> DocumentObject | None:
    """Return the active FEM analysis object, if one is currently set."""
    ...

@overload
def open(name: str, /) -> None:
    """Open one Abaqus or Python input file in the FEM editor."""
    ...

@overload
def open(name: str, doc_name: str, /) -> None:
    """Open one Abaqus or Python input file in the FEM editor."""
    ...

@overload
def insert(name: str, /) -> None:
    """Open one Abaqus or Python input file in the FEM editor."""
    ...

@overload
def insert(name: str, doc_name: str, /) -> None:
    """Open one Abaqus or Python input file in the FEM editor."""
    ...
