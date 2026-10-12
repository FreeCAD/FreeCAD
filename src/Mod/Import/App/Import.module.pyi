# SPDX-License-Identifier: LGPL-2.1-or-later

"""Typed public signatures for the ``Import`` application module.

This source-adjacent stub file carries the manual import/export helpers for
OCAF-backed CAD formats together with the DXF utility surface.
"""

from __future__ import annotations

from collections.abc import Sequence
from typing import Literal, TypeAlias, TypedDict, overload

from FreeCAD import DocumentObject
from Part import Feature, Shape

_ColorRGBA: TypeAlias = tuple[float, float, float, float]
_DxfVersion: TypeAlias = Literal[12, 14]
_ImportExportObject: TypeAlias = DocumentObject | tuple[DocumentObject, Sequence[_ColorRGBA]]
_ImportColorAssignments: TypeAlias = list[tuple[Feature, list[_ColorRGBA]]]
_UnsupportedFeatureOccurrences: TypeAlias = dict[str, list[tuple[int, str]]]
DxfImportStats = TypedDict(
    "DxfImportStats",
    {
        "dxfVersion": str,
        "dxfEncoding": str,
        "scalingSource": str,
        "fileUnits": str,
        "finalScalingFactor": float,
        "importTimeSeconds": float,
        "totalEntitiesCreated": int,
        "entityCounts": dict[str, int],
        "importSettings": dict[str, str],
        "unsupportedFeatures": _UnsupportedFeatureOccurrences,
        "systemBlockCounts": dict[str, int],
    },
)

# OCAF-backed file I/O
def open(
    name: str,
    docName: str | None = None,
    importHidden: bool | None = None,
    merge: bool | None = None,
    useLinkGroup: bool | None = None,
    mode: int = -1,
) -> _ImportColorAssignments | None:
    """Open one STEP, IGES, or glTF file and optionally return imported face colors."""
    ...

def insert(
    name: str,
    docName: str | None = None,
    importHidden: bool | None = None,
    merge: bool | None = None,
    useLinkGroup: bool | None = None,
    mode: int = -1,
) -> _ImportColorAssignments | None:
    """Insert one STEP, IGES, or glTF file into a document and optionally return face colors."""
    ...

def export(
    obj: Sequence[_ImportExportObject],
    name: str,
    exportHidden: bool | None = None,
    legacy: bool | None = None,
    keepPlacement: bool | None = None,
) -> None:
    """Export document objects to one STEP, IGES, or glTF file."""
    ...

# DXF helpers
def readDXF(
    filename: str,
    document: str | None = None,
    ignore_errors: bool = True,
    option_source: str | None = None,
    /,
) -> DxfImportStats:
    """Import one DXF file and return detailed import statistics."""
    ...

@overload
def writeDXFShape(
    shape: Shape,
    filename: str,
    version: _DxfVersion | int = -1,
    usePolyline: bool = False,
    optionSource: str | None = None,
    /,
) -> None:
    """Export one shape to a DXF file."""
    ...

@overload
def writeDXFShape(
    shape: Sequence[Shape],
    filename: str,
    version: _DxfVersion | int = -1,
    usePolyline: bool = False,
    optionSource: str | None = None,
    /,
) -> None:
    """Export a shape sequence to a DXF file."""
    ...

@overload
def writeDXFObject(
    obj: DocumentObject,
    filename: str,
    version: _DxfVersion | int = -1,
    usePolyline: bool = False,
    optionSource: str | None = None,
    /,
) -> None:
    """Export one document object to a DXF file."""
    ...

@overload
def writeDXFObject(
    obj: Sequence[DocumentObject],
    filename: str,
    version: _DxfVersion | int = -1,
    usePolyline: bool = False,
    optionSource: str | None = None,
    /,
) -> None:
    """Export a document-object sequence to a DXF file."""
    ...
