# SPDX-License-Identifier: LGPL-2.1-or-later

"""Provider-neutral contracts for the native CadX tool surface."""

from __future__ import annotations

import json
import sys
from pathlib import Path
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

from CadXToolProtocol import (  # noqa: E402
    ASSEMBLY_CREATE_SCHEMA,
    ASSEMBLY_CONSTRAINT_RESULT_SCHEMA_VERSION,
    ASSEMBLY_MUTATION_RESULT_SCHEMA_VERSION,
    GRAPH_QUERY_RESULT_SCHEMA_VERSION,
    GRAPH_RESULT_SCHEMA_VERSION,
    ToolClassification,
    ThreadRequirement,
    ToolDefinition,
    ToolProtocolError,
    ToolRegistry,
    ToolResult,
    make_graph_registry,
)

SAFE_TOOLS = {
    "assembly.graph_snapshot", "assembly.graph_query", "assembly.create",
    "assembly.insert", "assembly.ground", "assembly.joint",
}


class NativeFixture:
    def __init__(self, names=None):
        self.names = list(SAFE_TOOLS if names is None else names)
        self.calls = []

    def tool_names(self):
        return self.names

    def snapshot(self, arguments):
        self.calls.append(("assembly.graph_snapshot", arguments))
        return ToolResult.success({"schema_version": GRAPH_RESULT_SCHEMA_VERSION, "graph_id": "g", "graph_revision": "r"})

    def query(self, arguments):
        self.calls.append(("assembly.graph_query", arguments))
        return ToolResult.success({"schema_version": GRAPH_QUERY_RESULT_SCHEMA_VERSION, "graph_id": "g", "graph_revision": "r"})

    def execute_tool(self, name, arguments_json):
        arguments = json.loads(arguments_json)
        self.calls.append((name, arguments))
        version = ASSEMBLY_CONSTRAINT_RESULT_SCHEMA_VERSION if name in {"assembly.ground", "assembly.joint"} else ASSEMBLY_MUTATION_RESULT_SCHEMA_VERSION
        return json.dumps({"schema_version": version, "operation_id": "op"})


class GraphFixture(NativeFixture):
    def __init__(self):
        super().__init__(["assembly.graph_snapshot", "assembly.graph_query"])


class ToolContractsTest(unittest.TestCase):
    def test_graph_registry_exposes_exact_provider_names(self):
        registry = make_graph_registry(GraphFixture())
        self.assertEqual(
            [definition.name for definition in registry.definitions()],
            ["assembly.graph_query", "assembly.graph_snapshot"],
        )
        self.assertEqual(
            [tool["function"]["name"] for tool in registry.provider_definitions()],
            ["assembly.graph_query", "assembly.graph_snapshot"],
        )

    def test_unknown_input_and_malformed_closed_schema_are_rejected(self):
        result = make_graph_registry(GraphFixture()).execute(
            "assembly.graph_snapshot", {"unexpected": True}
        )
        self.assertFalse(result.ok)
        self.assertEqual(result.error["code"], "CADX_TOOL_ARGUMENTS_INVALID")

        with self.assertRaises(ToolProtocolError):
            ToolRegistry().register(
                ToolDefinition(
                    name="cadx.bad",
                    description="closed test tool",
                    classification=ToolClassification.READ,
                    input_schema={"type": "object", "additionalProperties": True},
                    output_schema_version="cadx.bad.v1",
                    executor=lambda arguments: {"schema_version": "cadx.bad.v1"},
                    thread_requirement=ThreadRequirement.ANY,
                )
            )

    def test_query_validation_is_bounded_and_closed(self):
        registry = make_graph_registry(GraphFixture())
        result = registry.execute(
            "assembly.graph_query",
            {
                "graph_id": "assembly-graph:test",
                "graph_revision": "sha256:test",
                "operation": "subgraph",
                "start_node_ids": ["node:test"],
                "max_depth": 5,
                "edge_kinds": [],
            },
        )
        self.assertFalse(result.ok)
        self.assertEqual(result.error["code"], "CADX_TOOL_ARGUMENTS_INVALID")

        result = registry.execute(
            "assembly.graph_query",
            {
                "graph_id": "assembly-graph:test",
                "graph_revision": "sha256:test",
                "operation": "summary",
                "unknown": True,
            },
        )
        self.assertFalse(result.ok)
        self.assertEqual(result.error["code"], "CADX_TOOL_ARGUMENTS_INVALID")

    def test_create_schema_does_not_advertise_unimplemented_parent_assembly(self):
        self.assertNotIn("parent_assembly", ASSEMBLY_CREATE_SCHEMA["properties"])
        result = make_graph_registry(NativeFixture()).execute(
            "assembly.create",
            {
                "operation": "create_assembly",
                "operation_id": "nested-create",
                "expected_graph_revision": "",
                "label": "Assembly",
                "parent_assembly": {"object_name": "Top"},
            },
        )
        self.assertFalse(result.ok)
        self.assertEqual(result.error["code"], "CADX_TOOL_ARGUMENTS_INVALID")

    def test_registry_exposes_exact_native_safe_intersection(self):
        backend = NativeFixture([*SAFE_TOOLS, "model.primitive", "future.unsupported"])
        registry = make_graph_registry(backend)
        self.assertEqual({d.name for d in registry.definitions()}, SAFE_TOOLS)
        self.assertEqual(
            {d.name for d in registry.definitions() if d.classification == ToolClassification.MUTATION},
            {"assembly.create", "assembly.insert", "assembly.ground", "assembly.joint"},
        )

    def test_registry_fails_closed_without_queryable_native_availability(self):
        class NoAvailability:
            def snapshot(self, arguments):
                raise AssertionError("must not execute")

        self.assertEqual(make_graph_registry(NoAvailability()).definitions(), ())

        class BrokenAvailability:
            def tool_names(self):
                raise RuntimeError("native unavailable")

        self.assertEqual(make_graph_registry(BrokenAvailability()).definitions(), ())

    def test_registry_requires_a_callable_route_for_each_reported_tool(self):
        class NamesOnly:
            def tool_names(self):
                return list(SAFE_TOOLS)

        self.assertEqual(make_graph_registry(NamesOnly()).definitions(), ())

        class SnapshotOnly:
            def tool_names(self):
                return list(SAFE_TOOLS)

            def snapshot(self, arguments):
                del arguments
                return ToolResult.success(
                    {
                        "schema_version": GRAPH_RESULT_SCHEMA_VERSION,
                        "graph_id": "g",
                        "graph_revision": "r",
                    }
                )

        self.assertEqual(
            {definition.name for definition in make_graph_registry(SnapshotOnly()).definitions()},
            {"assembly.graph_snapshot"},
        )

    def test_mutation_definitions_execute_through_native_bridge(self):
        backend = NativeFixture()
        registry = make_graph_registry(backend)
        requests = {
            "assembly.create": {"operation": "create_assembly", "operation_id": "create-1", "expected_graph_revision": "", "label": "Assembly"},
            "assembly.insert": {"operation": "insert_component", "operation_id": "insert-1", "expected_graph_revision": "r", "assembly": {"object_name": "Assembly"}, "source": {"document_name": "Parts", "object_name": "Box"}},
            "assembly.ground": {"operation": "set_grounded", "operation_id": "ground-1", "expected_graph_revision": "r", "assembly": {"object_name": "Assembly"}, "components": ["Component"]},
            "assembly.joint": {"operation": "create", "operation_id": "joint-1", "expected_graph_revision": "r", "assembly": {"object_name": "Assembly"}, "first": {"component": "A", "connector_type": "element", "connector": "Face1"}, "second": {"component": "B", "connector_type": "interface", "connector": "Axis"}, "joint_type": "fixed"},
        }
        for name, arguments in requests.items():
            result = registry.execute(name, arguments)
            self.assertTrue(result.ok, (name, result.to_payload()))
        self.assertEqual({name for name, _ in backend.calls}, set(requests))
        self.assertNotIn("parent_assembly", backend.calls[0][1])

    def test_joint_offset_matches_native_object_contract(self):
        backend = NativeFixture()
        registry = make_graph_registry(backend)
        request = {"operation": "create", "operation_id": "joint-offset", "expected_graph_revision": "r", "assembly": {"object_name": "Assembly"}, "joint_type": "revolute", "first": {"component": "A", "connector_type": "element", "connector": "Face1", "offset": {"translation_mm": [1, 2, 3], "rotation_axis": [0, 0, 1], "rotation_degrees": 90}}, "second": {"component": "B", "connector_type": "element", "connector": "Face2"}, "limits": {"minimum_degrees": -10, "maximum_degrees": 20}}
        self.assertTrue(registry.execute("assembly.joint", request).ok)
        self.assertEqual(backend.calls[-1][1]["first"]["offset"]["translation_mm"], [1, 2, 3])
        invalid = dict(request, first=dict(request["first"], offset=[0, 0, 0, 0, 0, 0, 1]))
        result = registry.execute("assembly.joint", invalid)
        self.assertFalse(result.ok)
        self.assertEqual(result.error["code"], "CADX_TOOL_ARGUMENTS_INVALID")
        self.assertEqual(len(backend.calls), 1)

    def test_native_fixture_rejects_semantically_invalid_arguments(self):
        class StrictNative(NativeFixture):
            def execute_tool(self, name, arguments_json):
                arguments = json.loads(arguments_json)
                if name == "assembly.create" and not arguments["label"].strip():
                    return json.dumps(
                        {
                            "ok": False,
                            "error": {
                                "code": "CADX_TOOL_ARGUMENTS_INVALID",
                                "message": "label must not be blank",
                                "retryable": False,
                                "details": {},
                            },
                        }
                    )
                return super().execute_tool(name, arguments_json)

        registry = make_graph_registry(StrictNative())
        result = registry.execute(
            "assembly.create",
            {
                "operation": "create_assembly",
                "operation_id": "blank-label",
                "expected_graph_revision": "",
                "label": " ",
            },
        )
        self.assertFalse(result.ok)
        self.assertEqual(result.error["code"], "CADX_TOOL_ARGUMENTS_INVALID")

        invalid_offset = {
            "operation": "create",
            "operation_id": "bad-offset",
            "expected_graph_revision": "r",
            "assembly": {"object_name": "Assembly"},
            "first": {
                "component": "A",
                "connector_type": "element",
                "connector": "Face1",
                "offset": {
                    "translation_mm": [0, 0, 0],
                    "rotation_axis": [2, 0, 0],
                    "rotation_degrees": 0,
                },
            },
            "second": {
                "component": "B",
                "connector_type": "element",
                "connector": "Face2",
            },
            "joint_type": "fixed",
        }
        result = registry.execute("assembly.joint", invalid_offset)
        self.assertFalse(result.ok)
        self.assertEqual(result.error["code"], "CADX_TOOL_ARGUMENTS_INVALID")

    def test_native_error_envelope_is_preserved(self):
        class ErrorBackend(NativeFixture):
            def execute_tool(self, name, arguments_json):
                del name, arguments_json
                return json.dumps({"ok": False, "error": {"code": "CADX_PRECONDITION_FAILED", "message": "stale", "retryable": True, "details": {"expected": "r1"}}})

        result = make_graph_registry(ErrorBackend()).execute("assembly.create", {"operation": "create_assembly", "operation_id": "op", "expected_graph_revision": "", "label": "Assembly"})
        self.assertEqual(result.to_payload()["error"], {"code": "CADX_PRECONDITION_FAILED", "message": "stale", "retryable": True, "details": {"expected": "r1"}})


if __name__ == "__main__":
    unittest.main()
