# SPDX-License-Identifier: LGPL-2.1-or-later

"""Typed public signatures for the ``ImportGui`` module helpers.

This source-adjacent stub file carries the GUI-assisted import/export helpers,
their option dictionaries, and the DXF inspection utilities.
"""

from __future__ import annotations

from collections.abc import Sequence
from typing import Literal, TypeAlias, TypedDict

from FreeCAD import DocumentObject

_DxfVersion: TypeAlias = Literal[12, 14]
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
ImportOptions = TypedDict(
    "ImportOptions",
    {
        "merge": bool,
        "useLinkGroup": bool,
        "useBaseName": bool,
        "importHidden": bool,
        "reduceObjects": bool,
        "showProgress": bool,
        "expandCompound": bool,
        "mode": int,
        "codePage": int,
    },
    total=False,
)
ExportOptions = TypedDict(
    "ExportOptions",
    {
        "exportHidden": bool,
        "keepPlacement": bool,
        "legacy": bool,
    },
    total=False,
)

def open(
    name: str,
    docName: str | None = None,
    options: ImportOptions | None = None,
    importHidden: bool | None = None,
    merge: bool | None = None,
    useLinkGroup: bool | None = None,
    mode: int = -1,
) -> DocumentObject | None:
    """Open one STEP, IGES, or glTF file with GUI import behavior."""
    ...

def insert(
    name: str,
    docName: str | None = None,
    options: ImportOptions | None = None,
    importHidden: bool | None = None,
    merge: bool | None = None,
    useLinkGroup: bool | None = None,
    mode: int = -1,
) -> DocumentObject | None:
    """Insert one STEP, IGES, or glTF file with GUI import behavior."""
    ...

def preScanDxf(filepath: str, /) -> dict[str, int]:
    """Return entity-count statistics gathered from one DXF file without importing it."""
    ...

def readDXF(
    filename: str,
    document: str | None = None,
    ignore_errors: bool = True,
    option_source: str | None = None,
    /,
) -> DxfImportStats:
    """Import one DXF file through the GUI path and return import statistics."""
    ...

def importOptions(name: str, /) -> ImportOptions:
    """Show the GUI import-options dialog for one file type and return the chosen options."""
    ...

def exportOptions(name: str, /) -> ExportOptions:
    """Show the GUI export-options dialog for one file type and return the chosen options."""
    ...

def export(
    obj: Sequence[DocumentObject],
    name: str,
    options: ExportOptions | None = None,
    exportHidden: bool | None = None,
    legacy: bool | None = None,
    keepPlacement: bool | None = None,
) -> None:
    """Export document objects through the GUI-aware OCAF exporter."""
    ...

def ocaf(name: str, /) -> None:
    """Open the OCAF browser for one STEP, IGES, or glTF file."""
    ...
