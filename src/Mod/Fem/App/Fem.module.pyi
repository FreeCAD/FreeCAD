# SPDX-License-Identifier: LGPL-2.1-or-later

"""Typed public signatures for the ``Fem`` module-level helpers.

This source-adjacent stub file carries the FEM mesh file I/O helpers and the
VTK-backed result utilities exposed directly by the Fem application module.
"""

from __future__ import annotations

from collections.abc import Sequence
from typing import overload

from FreeCAD import DocumentObject

# FEM mesh file and document I/O
def open(name: str, /) -> None:
    """Open one FEM mesh file into a new document."""
    ...

@overload
def insert(name: str, /) -> None:
    """Insert one FEM mesh or result file into the active document, creating one if needed."""
    ...

@overload
def insert(name: str, doc_name: str, /) -> None:
    """Insert one FEM mesh or result file into the named document, creating it if needed."""
    ...

def export(objects: Sequence[DocumentObject], name: str, /) -> None:
    """Export the first selected FEM mesh object to one mesh or solver file."""
    ...

def read(name: str, /) -> FemMesh:
    """Read one FEM mesh file and return the resulting `FemMesh` object."""
    ...

def show(mesh: FemMesh, name: str = "Mesh", /) -> None:
    """Create one FEM mesh object in the active document from a `FemMesh`."""
    ...

# VTK-backed result helpers
def frdToVTK(filename: str, binary: bool = True, /) -> None:
    """Convert one CalculiX `.frd` result file to VTK output."""
    ...

@overload
def readResult(file_name: str, /) -> None:
    """Read one CFD or mechanical result file into the active result target."""
    ...

@overload
def readResult(file_name: str, obj_name: str, /) -> None:
    """Read one CFD or mechanical result file into the named document object."""
    ...

@overload
def writeResult(file_name: str, /) -> None:
    """Write the active CFD or FEM result object to one file."""
    ...

@overload
def writeResult(file_name: str, obj: DocumentObject, /) -> None:
    """Write one specific CFD or FEM result object to one file."""
    ...

def getVtkVersion() -> str:
    """Return the VTK version string linked into this build."""
    ...

def getVtkVersionNumber() -> int:
    """Return the VTK version as one packed integer value."""
    ...

def vtkVersionCheck(major: int, minor: int, build: int = 0, /) -> int:
    """Pack one VTK major, minor, and build tuple into a version integer."""
    ...

def isVtkCompatible(vtkObject: object, /) -> bool:
    """Return whether one Python VTK object is compatible with FreeCAD's VTK ABI."""
    ...
