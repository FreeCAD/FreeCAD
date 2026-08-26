# SPDX-License-Identifier: LGPL-2.1-or-later

"""Tests for CadXChatClient: Ollama SSE parsing, turns, and model resolution."""

from __future__ import annotations

import json
import sys
import unittest
from pathlib import Path


sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

from CadXChatClient import (  # noqa: E402
    ChatClientError,
    ChatMessage,
    ChatRequestContext,
    OllamaClient,
    TurnKind,
    parse_sse_events,
)


def _sse(*payloads: dict, done: bool = True) -> list[bytes]:
    """Encode payloads as an SSE byte stream split into awkward chunks."""

    lines = [f"data: {json.dumps(payload)}\n\n" for payload in payloads]
    if done:
        lines.append("data: [DONE]\n\n")
    raw = "".join(lines).encode("utf-8")
    # Split mid-line so parser buffering is exercised.
    return [raw[i : i + 7] for i in range(0, len(raw), 7)]


def _chunk(delta_content: str | None, finish: str | None = None) -> dict:
    return {
        "id": "1",
        "object": "chat.completion.chunk",
        "model": "test-model",
        "choices": [
            {"index": 0, "delta": {"content": delta_content}, "finish_reason": finish}
        ],
    }


class FakeAuthServicelessWorld:
    """Placeholder name kept minimal; Ollama needs no auth."""


class RecordingSink:
    def __init__(self):
        self.events: list[tuple[TurnKind, str, str]] = []

    def __call__(self, event):
        self.events.append((event.kind, event.text, event.message))

    def kinds(self):
        return [kind for kind, _text, _message in self.events]


class ParseSseEventsTest(unittest.TestCase):
    def test_parses_data_lines_across_chunk_boundaries(self):
        events = list(parse_sse_events(_sse({"a": 1}, {"b": 2})))
        self.assertEqual([event["a" if "a" in event else "b"] for event in events], [1, 2])

    def test_stops_at_done_sentinel_and_skips_malformed(self):
        raw = (
            b": keep-alive\n\n"
            b"data: not-json\n\n"
            b'data: {"type": "ok"}\n\n'
            b"data: [DONE]\n\n"
            b'data: {"type": "after-done"}\n\n'
        )
        events = list(parse_sse_events([raw]))
        self.assertEqual([event.get("type") for event in events], ["ok"])

    def test_ignores_non_dict_payloads(self):
        raw = b"data: [1, 2]\n\ndata: {\"type\": \"ok\"}\n\n"
        events = list(parse_sse_events([raw]))
        self.assertEqual([event.get("type") for event in events], ["ok"])


class ModelResolutionTest(unittest.TestCase):
    def make_client(self, *, model=None, tags=None, tags_error=None):
        calls: list[str] = []

        def get_json(url, timeout_seconds):
            calls.append(url)
            if tags_error is not None:
                raise tags_error
            return {"models": [{"name": name} for name in tags or []]}

        client = OllamaClient(
            model=model,
            get_json=get_json,
            transport=lambda *args: iter([]),
        )
        client.tag_calls = calls
        return client

    def test_explicit_model_wins_without_network(self):
        client = self.make_client(model="m-explicit", tags=["m-installed"])
        self.assertEqual(client.model, "m-explicit")
        self.assertEqual(client.resolve_model(), "m-explicit")
        self.assertEqual(client.tag_calls, [])

    def test_first_installed_model_used_when_no_preference(self):
        client = self.make_client(tags=["first-local-model", "second-local-model"])
        self.assertEqual(client.model, "")  # no model is selected before discovery
        self.assertEqual(client.resolve_model(), "first-local-model")
        self.assertEqual(client.model, "first-local-model")

    def test_lists_unique_local_models(self):
        client = self.make_client(tags=["one", "one", "two"])

        self.assertEqual(client.list_models(), ("one", "two"))

    def test_selected_model_is_used_without_network(self):
        client = self.make_client(tags=["one", "two"])
        client.set_model("two")

        self.assertEqual(client.model, "two")
        self.assertEqual(client.resolve_model(), "two")
        self.assertEqual(client.tag_calls, [])

    def test_server_failure_falls_back_to_context_default(self):
        client = self.make_client(tags_error=ChatClientError("down"))
        self.assertEqual(client.resolve_model(), "")

    def test_resolution_is_cached(self):
        client = self.make_client(tags=["one", "two"])
        self.assertEqual(client.resolve_model(), "one")
        self.assertEqual(client.resolve_model(), "one")
        self.assertEqual(len(client.tag_calls), 1)


class StreamTurnTest(unittest.TestCase):
    def make_client(self, transport, model=None, base_url="http://ollama.local:11434"):
        context = ChatRequestContext(
            base_url=base_url,
            model="context-model",
            system_prompt="Be brief.",
        )
        return OllamaClient(
            context=context,
            model=model,
            transport=transport,
            get_json=lambda url, timeout: {"models": []},
        )

    @staticmethod
    def capturing_transport(chunks):
        calls: list[tuple[str, dict, bytes]] = []

        def transport(url, headers, body, timeout_seconds):
            calls.append((url, headers, body))
            return iter(chunks)

        transport.calls = calls
        return transport

    def test_happy_path_streams_deltas_and_completes(self):
        transport = self.capturing_transport(
            _sse(
                _chunk("Hello"),
                _chunk(", world"),
                _chunk(None, finish="stop"),
            )
        )
        sink = RecordingSink()
        client = self.make_client(transport)
        text = client.stream_turn(
            [ChatMessage("user", "hi")], on_event=sink, should_cancel=lambda: False
        )
        self.assertEqual(text, "Hello, world")
        self.assertEqual(
            sink.kinds(),
            [TurnKind.STARTED, TurnKind.DELTA, TurnKind.DELTA, TurnKind.COMPLETED],
        )
        url, headers, body = transport.calls[0]
        self.assertEqual(url, "http://ollama.local:11434/v1/chat/completions")
        decoded = json.loads(body)
        self.assertTrue(decoded["stream"])
        self.assertEqual(decoded["model"], "context-model")
        roles = [message["role"] for message in decoded["messages"]]
        self.assertEqual(roles, ["system", "user"])
        self.assertEqual(decoded["messages"][0]["content"], "Be brief.")
        self.assertEqual(decoded["messages"][1]["content"], "hi")

    def test_history_roles_use_plain_content(self):
        captured = {}

        def transport(url, headers, body, timeout_seconds):
            captured["body"] = json.loads(body)
            return iter(_sse(_chunk("ok")))

        sink = RecordingSink()
        client = self.make_client(transport)
        client.stream_turn(
            [
                ChatMessage("assistant", "earlier"),
                ChatMessage("user", "again"),
            ],
            on_event=sink,
            should_cancel=lambda: False,
        )
        roles = [(m["role"], m["content"]) for m in captured["body"]["messages"]]
        self.assertEqual(
            roles,
            [
                ("system", "Be brief."),
                ("assistant", "earlier"),
                ("user", "again"),
            ],
        )

    def test_http_error_surfaces_as_failure(self):
        def transport(url, headers, body, timeout_seconds):
            raise ChatClientError(
                'Ollama rejected the request (HTTP 404). model "nope" not found'
            )

        sink = RecordingSink()
        client = self.make_client(transport)
        text = client.stream_turn([], on_event=sink, should_cancel=lambda: False)
        self.assertEqual(text, "")
        failed = sink.events[-1]
        self.assertEqual(failed[0], TurnKind.FAILED)
        self.assertIn("not found", failed[2])

    def test_error_payload_in_stream_maps_to_failure(self):
        transport = self.capturing_transport(
            _sse({"error": "server overheated"}, done=True)
        )
        sink = RecordingSink()
        client = self.make_client(transport)
        text = client.stream_turn([], on_event=sink, should_cancel=lambda: False)
        self.assertEqual(text, "")
        failed = sink.events[-1]
        self.assertEqual(failed[0], TurnKind.FAILED)
        self.assertEqual(failed[2], "server overheated")

    def test_empty_stream_reports_empty_response_failure(self):
        transport = self.capturing_transport([])
        sink = RecordingSink()
        client = self.make_client(transport)
        text = client.stream_turn([], on_event=sink, should_cancel=lambda: False)
        self.assertEqual(text, "")
        failed = sink.events[-1]
        self.assertEqual(failed[0], TurnKind.FAILED)
        self.assertIn("empty", failed[2])

    def test_cancellation_stops_the_stream(self):
        state = {"seen_delta": False}

        def transport(url, headers, body, timeout_seconds):
            yield from _sse(_chunk("partial"), done=False)
            state["seen_delta"] = True
            yield from _sse(_chunk(" more"), _chunk(None, finish="stop"))

        sink = RecordingSink()
        client = self.make_client(transport)

        def should_cancel():
            return state["seen_delta"]

        text = client.stream_turn([], on_event=sink, should_cancel=should_cancel)
        self.assertEqual(text, "")
        self.assertIn(TurnKind.CANCELLED, sink.kinds())
        self.assertNotIn(TurnKind.COMPLETED, sink.kinds())


if __name__ == "__main__":
    unittest.main()
