# SPDX-License-Identifier: LGPL-2.1-or-later

"""Strict validator for the native CadX graph audit JSONL stream.

The native service writes operational events; the graph snapshot and its
revisions remain authoritative.  This module checks that the event stream
preserves the causal evidence chain and that every graph-bearing event carries
the hashes needed to compare it with an exported snapshot.
"""

from __future__ import annotations

import argparse
import json
import sys
from collections.abc import Iterable, Mapping
from pathlib import Path
from typing import Any


SCHEMA_VERSION = "cadx.graph-audit.v1"
_FIELDS = {
    "schema_version",
    "sequence",
    "timestamp_ms",
    "stage",
    "status",
    "operation",
    "graph_id",
    "graph_revision",
    "presentation_revision",
    "semantic_hash",
    "presentation_hash",
    "node_count",
    "edge_count",
    "error_code",
    "diagnostic",
}
_STAGES = {"build", "round_trip", "publish", "query"}
_STATUSES = {"started", "passed", "failed"}


class GraphAuditError(ValueError):
    """Raised when an audit stream cannot prove a valid graph lifecycle."""


def _require_string(event: Mapping[str, Any], key: str, line_number: int) -> str:
    value = event.get(key)
    if not isinstance(value, str):
        raise GraphAuditError(f"line {line_number}: {key} must be a string")
    return value


def _require_nonnegative_integer(event: Mapping[str, Any], key: str, line_number: int) -> int:
    value = event.get(key)
    if isinstance(value, bool) or not isinstance(value, int) or value < 0:
        raise GraphAuditError(f"line {line_number}: {key} must be a non-negative integer")
    return value


def validate_audit_records(
    records: Iterable[Mapping[str, Any]], *, strict: bool = True
) -> dict[str, Any]:
    """Validate records and return a compact evidence summary.

    In strict mode, a successful publish must be preceded in the same graph
    revision by a successful build and a successful evidence round-trip.
    """

    count = 0
    passed = 0
    failed = 0
    graph_ids: set[str] = set()
    graph_revisions: set[str] = set()
    prepared: set[tuple[str, str]] = set()
    round_tripped: set[tuple[str, str]] = set()

    for line_number, raw in enumerate(records, start=1):
        if not isinstance(raw, Mapping):
            raise GraphAuditError(f"line {line_number}: record must be an object")
        if strict and set(raw) != _FIELDS:
            missing = sorted(_FIELDS - set(raw))
            extra = sorted(set(raw) - _FIELDS)
            detail = []
            if missing:
                detail.append("missing=" + ",".join(missing))
            if extra:
                detail.append("extra=" + ",".join(extra))
            raise GraphAuditError(f"line {line_number}: unexpected fields ({'; '.join(detail)})")

        schema = _require_string(raw, "schema_version", line_number)
        if schema != SCHEMA_VERSION:
            raise GraphAuditError(f"line {line_number}: unsupported schema_version {schema!r}")
        sequence = _require_nonnegative_integer(raw, "sequence", line_number)
        expected_sequence = count + 1
        if sequence != expected_sequence:
            raise GraphAuditError(
                f"line {line_number}: sequence {sequence} does not follow {expected_sequence}"
            )
        timestamp = _require_nonnegative_integer(raw, "timestamp_ms", line_number)
        if timestamp == 0:
            raise GraphAuditError(f"line {line_number}: timestamp_ms must be positive")

        stage = _require_string(raw, "stage", line_number)
        status = _require_string(raw, "status", line_number)
        operation = _require_string(raw, "operation", line_number)
        if stage not in _STAGES or status not in _STATUSES:
            raise GraphAuditError(f"line {line_number}: unsupported stage/status")
        if stage in {"build", "round_trip", "publish"} and operation != "assembly.graph_snapshot":
            raise GraphAuditError(f"line {line_number}: snapshot stage has wrong operation")
        if stage == "query" and operation != "assembly.graph_query":
            raise GraphAuditError(f"line {line_number}: query stage has wrong operation")

        graph_id = _require_string(raw, "graph_id", line_number)
        graph_revision = _require_string(raw, "graph_revision", line_number)
        presentation_revision = _require_string(raw, "presentation_revision", line_number)
        semantic_hash = _require_string(raw, "semantic_hash", line_number)
        presentation_hash = _require_string(raw, "presentation_hash", line_number)
        error_code = _require_string(raw, "error_code", line_number)
        diagnostic = _require_string(raw, "diagnostic", line_number)
        node_count = _require_nonnegative_integer(raw, "node_count", line_number)
        edge_count = _require_nonnegative_integer(raw, "edge_count", line_number)

        has_graph = bool(graph_id or graph_revision or presentation_revision or semantic_hash or presentation_hash)
        if has_graph:
            if not all((graph_id, graph_revision, presentation_revision)):
                raise GraphAuditError(f"line {line_number}: incomplete graph identity")
            if not semantic_hash.startswith("sha256:") or not presentation_hash.startswith("sha256:"):
                raise GraphAuditError(f"line {line_number}: graph hashes must be sha256 revisions")
            graph_ids.add(graph_id)
            graph_revisions.add(graph_revision)
        elif any((node_count, edge_count)):
            raise GraphAuditError(f"line {line_number}: counts require a graph identity")

        if status == "failed":
            failed += 1
            if not error_code or not diagnostic:
                raise GraphAuditError(f"line {line_number}: failed event needs code and diagnostic")
        elif status == "passed":
            passed += 1
            if error_code or diagnostic:
                raise GraphAuditError(f"line {line_number}: passed event cannot carry an error")
        if status == "started" and error_code:
            raise GraphAuditError(f"line {line_number}: started event cannot carry an error")

        key = (graph_id, graph_revision)
        if strict and status == "passed" and stage == "build":
            if not has_graph:
                raise GraphAuditError(f"line {line_number}: passed build needs graph evidence")
            prepared.add(key)
        elif strict and status == "passed" and stage == "round_trip":
            if key not in prepared:
                raise GraphAuditError(f"line {line_number}: round-trip lacks a passed build")
            round_tripped.add(key)
        elif strict and status == "passed" and stage == "publish":
            if key not in round_tripped:
                raise GraphAuditError(f"line {line_number}: publish lacks a passed round-trip")

        count += 1

    if count == 0:
        raise GraphAuditError("audit stream is empty")
    return {
        "schema_version": SCHEMA_VERSION,
        "event_count": count,
        "passed_count": passed,
        "failed_count": failed,
        "graph_count": len(graph_ids),
        "graph_revision_count": len(graph_revisions),
        "last_sequence": count,
    }


def validate_audit_lines(lines: Iterable[str], *, strict: bool = True) -> dict[str, Any]:
    """Parse and validate JSONL lines, reporting the physical line on errors."""

    records: list[Mapping[str, Any]] = []
    for line_number, line in enumerate(lines, start=1):
        if not line.strip():
            continue
        try:
            value = json.loads(line)
        except json.JSONDecodeError as error:
            raise GraphAuditError(f"line {line_number}: invalid JSON: {error.msg}") from error
        if not isinstance(value, Mapping):
            raise GraphAuditError(f"line {line_number}: record must be an object")
        records.append(value)
    return validate_audit_records(records, strict=strict)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="Validate a CadX graph audit JSONL stream")
    parser.add_argument("path", type=Path)
    args = parser.parse_args(argv)
    try:
        with args.path.open(encoding="utf-8") as stream:
            summary = validate_audit_lines(stream)
    except (OSError, GraphAuditError) as error:
        print(f"cadx graph audit: invalid: {error}", file=sys.stderr)
        return 1
    print(json.dumps(summary, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
