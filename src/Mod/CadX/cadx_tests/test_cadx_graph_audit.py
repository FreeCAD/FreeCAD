# SPDX-License-Identifier: LGPL-2.1-or-later

"""Strict lifecycle checks for native CadX graph audit events."""

from __future__ import annotations

import json
import sys
import unittest
from pathlib import Path


sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

from CadXGraphAudit import GraphAuditError, validate_audit_lines  # noqa: E402


def _event(sequence: int, stage: str, status: str, *, graph: bool = False) -> dict:
    return {
        "schema_version": "cadx.graph-audit.v1",
        "sequence": sequence,
        "timestamp_ms": 1_700_000_000_000 + sequence,
        "stage": stage,
        "status": status,
        "operation": "assembly.graph_snapshot"
        if stage != "query"
        else "assembly.graph_query",
        "graph_id": "assembly-graph:test" if graph else "",
        "graph_revision": "sha256:graph" if graph else "",
        "presentation_revision": "sha256:presentation" if graph else "",
        "semantic_hash": "sha256:semantic" if graph else "",
        "presentation_hash": "sha256:view" if graph else "",
        "node_count": 3 if graph else 0,
        "edge_count": 2 if graph else 0,
        "error_code": "",
        "diagnostic": "",
    }


class GraphAuditValidationTest(unittest.TestCase):
    def test_accepts_causal_build_round_trip_publish_chain(self):
        records = [
            _event(1, "build", "started"),
            _event(2, "build", "passed", graph=True),
            _event(3, "round_trip", "passed", graph=True),
            _event(4, "publish", "passed", graph=True),
        ]
        summary = validate_audit_lines(json.dumps(record) for record in records)
        self.assertEqual(summary["event_count"], 4)
        self.assertEqual(summary["graph_count"], 1)

    def test_rejects_publish_without_round_trip(self):
        records = [_event(1, "publish", "passed", graph=True)]
        with self.assertRaisesRegex(GraphAuditError, "publish lacks a passed round-trip"):
            validate_audit_lines(json.dumps(record) for record in records)

    def test_rejects_sequence_gap_and_bad_hash(self):
        records = [_event(1, "build", "started"), _event(3, "build", "passed", graph=True)]
        with self.assertRaisesRegex(GraphAuditError, "sequence 3"):
            validate_audit_lines(json.dumps(record) for record in records)

        records = [_event(1, "build", "passed", graph=True)]
        records[0]["semantic_hash"] = "not-a-hash"
        with self.assertRaisesRegex(GraphAuditError, "sha256 revisions"):
            validate_audit_lines(json.dumps(record) for record in records)


if __name__ == "__main__":
    unittest.main()
