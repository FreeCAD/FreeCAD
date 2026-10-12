# SPDX-License-Identifier: LGPL-2.1-or-later

"""Data model for the Python API deprecation inventory."""

from __future__ import annotations

from dataclasses import dataclass


@dataclass(frozen=True, order=True)
class DeprecationRecord:
    symbol: str
    kind: str
    deprecated_in: str
    removed_in: str
    replacement: str | None
    details: str | None
    source: str
    line: int


@dataclass(frozen=True, order=True)
class Diagnostic:
    source: str
    line: int
    severity: str
    message: str


@dataclass(frozen=True)
class ScanResult:
    records: tuple[DeprecationRecord, ...]
    diagnostics: tuple[Diagnostic, ...]
