# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 FreeCAD contributors
# SPDX-FileNotice: Part of the FreeCAD project.


"""IFC file content summary utility.

Extracts basic metadata and entity counts from IFC files
without loading geometry or traversing object relationships.
Intended for quick overview of IFC file content in UI such as import/export dialogs.
"""

from __future__ import annotations

import os
import time  # TODO: remove before merge

from collections import OrderedDict
from dataclasses import dataclass
from typing import Any, Callable


@dataclass(frozen=True)
class _SummaryField:
    """IFC summary field definition."""

    name: str
    entity: str | None = None


_SUMMARY_FIELDS: tuple[_SummaryField, ...] = (
    _SummaryField("schema"),
    _SummaryField("projects", "IfcProject"),
    _SummaryField("sites", "IfcSite"),
    _SummaryField("buildings", "IfcBuilding"),
    _SummaryField("storeys", "IfcBuildingStorey"),
    _SummaryField("products", "IfcProduct"),
    _SummaryField("types", "IfcTypeObject"),
    _SummaryField("property_sets", "IfcPropertySet"),
    _SummaryField("materials", "IfcMaterial"),
    _SummaryField("layers", "IfcPresentationLayerAssignment"),
)

# Keep the process-wide cache size small.
_MAX_CACHE_SIZE = 16

# Summaries cache keyed by IFC file absolute path, file size, and modification time.
# Useful when same unchanged IFC file is summarized again, e.g. inexpensive import dialog reopen.
_SUMMARY_CACHE: OrderedDict[tuple[str, int, int], IfcSummary] = OrderedDict()


@dataclass(frozen=True)
class IfcSummary:
    """Summary of IFC file content.

    `created` is creation timestamp stored in IFC FILE_NAME header.
    `modified` is filesystem modification timestamp of IFC file.
    `schema` is IFC schema identifier, e.g. `IFC2X3` or `IFC4`.
    Counts are IFC entities and do not necessarily correspond to number of FreeCAD objects.
    """

    filename: str
    file_size: int
    created: str
    modified: float

    schema: str
    projects: int
    sites: int
    buildings: int
    storeys: int
    products: int
    types: int
    property_sets: int
    materials: int
    layers: int


def _get_created(ifc_file: Any) -> str:
    """IFC file creation timestamp from header."""

    file_name = ifc_file.header.file_name
    timestamp = getattr(file_name, "time_stamp", None)

    return timestamp or ""


def _count_type_entities(
    ifc_file: Any,
    entity_name: str,
    cancelled: Callable[[], bool] | None = None,
) -> int:
    """Number of IFC entities of given type."""

    if cancelled is not None and cancelled():
        raise InterruptedError("IFC summary generation cancelled")

    return len(ifc_file.by_type(entity_name))


def _cache_key(
    filename: str | os.PathLike[str],
) -> tuple[tuple[str, int, int], os.stat_result]:
    """Summary cache key from IFC file absolute path, size, and modification time."""

    path: str = os.path.abspath(os.fspath(filename))
    stat: os.stat_result = os.stat(path)
    return (path, stat.st_size, stat.st_mtime_ns), stat


def _emit_cached_summary(
    summary: IfcSummary,
    metadata: Callable[[str, int, str, float], None] | None,
    update: Callable[[str, str | int], None] | None,
) -> None:
    """Cached summary sent to callback."""

    if metadata is not None:
        metadata(
            summary.filename,
            summary.file_size,
            summary.created,
            summary.modified,
        )

    if update is not None:
        for field in _SUMMARY_FIELDS:
            update(field.name, getattr(summary, field.name))


def _get_cached_summary(
    cache_key: tuple[str, int, int],
) -> IfcSummary | None:
    """Cached summary marked as used."""

    summary: IfcSummary | None = _SUMMARY_CACHE.get(cache_key)

    if summary is not None:
        _SUMMARY_CACHE.move_to_end(cache_key)

    return summary


def _cache_summary(
    cache_key: tuple[str, int, int],
    summary: IfcSummary,
) -> None:
    """Store summary in cache."""

    _SUMMARY_CACHE[cache_key] = summary
    _SUMMARY_CACHE.move_to_end(cache_key)

    if len(_SUMMARY_CACHE) > _MAX_CACHE_SIZE:
        _SUMMARY_CACHE.popitem(last=False)


def get_summary(
    source: str | os.PathLike[str] | Any,
    metadata: Callable[[str, int, str, float], None] | None = None,
    update: Callable[[str, str | int], None] | None = None,
    cancelled: Callable[[], bool] | None = None,
) -> IfcSummary:
    """Summary of IFC file with basic metadata and IFC entities counts.

    `source` is either filename/path or already-open IFC object.
    Open file objects are not cached as no reliable filesystem stats can be used.

    Summaries are cached using absolute path, file size, and filesystem modification time.
    Repeated requests for same unchanged file directly return cached summary.
    Cancellation is checked before each entity count.
    But it cannot interrupt individual IfcOpenShell operation already in progress.
    """

    time_0 = time.perf_counter()  # TODO: remove before merge

    filename: str = ""
    file_size: int = 0
    modified: float = 0.0
    cache_key: tuple[str, int, int] | None = None

    if hasattr(source, "by_type"):
        ifc_file = source  # Already open IFC file object.

    else:
        cache_key, stat = _cache_key(source)
        cached_summary: IfcSummary | None = _get_cached_summary(cache_key)

        if cached_summary is not None:
            _emit_cached_summary(cached_summary, metadata, update)
            return cached_summary

        if cancelled is not None and cancelled():
            raise InterruptedError("IFC summary generation cancelled")

        abspath: str = cache_key[0]
        filename = os.path.basename(abspath)
        file_size = cache_key[1]
        modified = stat.st_mtime

        # Update IFC file metadata immediately before expensive IFC parsing completes.
        if metadata is not None:
            metadata(filename, file_size, "", modified)

        import ifcopenshell

        ifc_file = ifcopenshell.open(abspath)

    time_1 = time.perf_counter()  # TODO: remove before merge

    # Info from already open IFC file object.
    if not filename:
        path: str = getattr(ifc_file, "filename", "")
        if path:
            filename = os.path.basename(path)
            if os.path.isfile(path):
                stat = os.stat(path)
                file_size = stat.st_size
                modified = stat.st_mtime

    created: str = _get_created(ifc_file)
    schema: str = str(getattr(ifc_file, "schema", "") or "Unknown")

    if metadata is not None:
        metadata(filename, file_size, created, modified)

    if update is not None:
        update("schema", schema)

    values: dict[str, int] = {}

    for field in _SUMMARY_FIELDS:
        if field.entity is None:
            continue

        count = _count_type_entities(ifc_file, field.entity, cancelled)
        values[field.name] = count

        if update is not None:
            update(field.name, count)

    time_2 = time.perf_counter()  # TODO: remove before merge

    summary = IfcSummary(
        filename=filename,
        file_size=file_size,
        created=created,
        modified=modified,
        schema=schema,
        **values,
    )

    if cache_key is not None:
        _cache_summary(cache_key, summary)

    time_3 = time.perf_counter()  # TODO: remove before merge
    print(
        f"IFC Summary timing for {filename}: open={time_1-time_0:.3f}s count={time_2-time_1:.3f}s store={time_3-time_2:.3f}s total={time_3-time_0:.3f}s"
    )

    return summary
