# SPDX-License-Identifier: LGPL-2.1-or-later

"""Fragmented Ollama tool calls and continuation requests."""

from __future__ import annotations

import json
import sys
from pathlib import Path
import unittest


sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

from CadXChatClient import (  # noqa: E402
    ChatMessage,
    OllamaClient,
    TurnKind,
)
from CadXToolProtocol import (  # noqa: E402
    GRAPH_RESULT_SCHEMA_VERSION,
    ToolResult,
    make_graph_registry,
)


def sse(*payloads):
    return [
        ("data: " + json.dumps(payload, separators=(",", ":")) + "\n\n").encode()
        for payload in payloads
    ] + [b"data: [DONE]\n\n"]


class Backend:
    def tool_names(self):
        return ["assembly.graph_snapshot", "assembly.graph_query"]

    def snapshot(self, arguments):
        return ToolResult.success(
            {
                "schema_version": GRAPH_RESULT_SCHEMA_VERSION,
                "graph_id": "assembly-graph:test",
                "graph_revision": "sha256:test",
            }
        )

    def query(self, arguments):
        raise AssertionError("the scripted turn only calls snapshot")


class ToolTurnTest(unittest.TestCase):
    def test_fragmented_tool_call_is_executed_then_turn_continues(self):
        requests = []
        responses = iter(
            [
                sse(
                    {
                        "choices": [
                            {
                                "delta": {
                                    "tool_calls": [
                                        {
                                            "index": 0,
                                            "id": "call_",
                                            "function": {"name": "assembly.graph_"},
                                        }
                                    ]
                                }
                            }
                        ]
                    },
                    {
                        "choices": [
                            {
                                "delta": {
                                    "tool_calls": [
                                        {
                                            "index": 0,
                                            "id": "1",
                                            "function": {
                                                "name": "snapshot",
                                                "arguments": "{}",
                                            },
                                        }
                                    ]
                                }
                            }
                        ]
                    },
                ),
                sse({"choices": [{"delta": {"content": "Assembly captured."}}]}),
            ]
        )

        def transport(url, headers, body, timeout):
            del url, headers, timeout
            requests.append(json.loads(body))
            return iter(next(responses))

        registry = make_graph_registry(Backend())
        events = []
        client = OllamaClient(
            model="fixture-model",
            tool_registry=registry,
            transport=transport,
            get_json=lambda url, timeout: {"models": []},
        )
        result = client.stream_turn(
            [ChatMessage("user", "inspect the assembly")],
            events.append,
            lambda: False,
        )

        self.assertEqual(result, "Assembly captured.")
        self.assertEqual(
            [event.kind for event in events],
            [
                TurnKind.STARTED,
                TurnKind.TOOL_CALL,
                TurnKind.TOOL_RESULT,
                TurnKind.DELTA,
                TurnKind.COMPLETED,
            ],
        )
        self.assertEqual(len(requests), 2)
        self.assertIn("tools", requests[0])
        self.assertEqual(requests[1]["messages"][-1]["role"], "tool")
        self.assertEqual(requests[1]["messages"][-1]["tool_call_id"], "call_1")

    def test_mutation_turn_executes_and_preserves_native_error(self):
        class MutationBackend:
            def __init__(self):
                self.calls = []

            def tool_names(self):
                return ["assembly.create"]

            def execute_tool(self, name, arguments_json):
                self.calls.append((name, json.loads(arguments_json)))
                return json.dumps(
                    {
                        "ok": False,
                        "error": {
                            "code": "CADX_PRECONDITION_FAILED",
                            "message": "graph revision is stale",
                            "retryable": True,
                            "details": {"expected": "r1"},
                        },
                    }
                )

        requests = []
        responses = iter(
            [
                sse(
                    {
                        "choices": [
                            {
                                "delta": {
                                    "tool_calls": [
                                        {
                                            "index": 0,
                                            "id": "create-1",
                                            "function": {
                                                "name": "assembly.create",
                                                "arguments": json.dumps(
                                                    {
                                                        "operation": "create_assembly",
                                                        "operation_id": "create-1",
                                                        "expected_graph_revision": "old",
                                                        "label": "Assembly",
                                                    },
                                                    separators=(",", ":"),
                                                ),
                                            },
                                        }
                                    ]
                                }
                            }
                        ]
                    }
                ),
                sse({"choices": [{"delta": {"content": "Creation was rejected as stale."}}]}),
            ]
        )

        def transport(url, headers, body, timeout):
            del url, headers, timeout
            requests.append(json.loads(body))
            return iter(next(responses))

        backend = MutationBackend()
        client = OllamaClient(
            model="fixture-model",
            tool_registry=make_graph_registry(backend),
            transport=transport,
            get_json=lambda url, timeout: {"models": []},
        )
        events = []
        result = client.stream_turn(
            [ChatMessage("user", "create an assembly")],
            events.append,
            lambda: False,
        )

        self.assertEqual(result, "Creation was rejected as stale.")
        self.assertEqual(backend.calls[0][0], "assembly.create")
        self.assertEqual(
            [event.kind for event in events],
            [
                TurnKind.STARTED,
                TurnKind.TOOL_CALL,
                TurnKind.TOOL_RESULT,
                TurnKind.DELTA,
                TurnKind.COMPLETED,
            ],
        )
        tool_result = next(event.tool_result for event in events if event.kind == TurnKind.TOOL_RESULT)
        self.assertFalse(tool_result.result.ok)
        self.assertEqual(tool_result.result.error["code"], "CADX_PRECONDITION_FAILED")
        self.assertEqual(requests[1]["messages"][-1]["role"], "tool")


if __name__ == "__main__":
    unittest.main()
