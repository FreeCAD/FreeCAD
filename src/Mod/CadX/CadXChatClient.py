# SPDX-License-Identifier: LGPL-2.1-or-later

"""Chat transport and bounded tool-call loop for a local Ollama server.

One public entry point (:meth:`OllamaClient.stream_turn`) posts the
conversation to Ollama's OpenAI-compatible chat API, parses the
server-sent-event stream, and reports progress through a callback.
Ollama is a local service, so no credentials are required. Everything remote
is reachable only through injectable
``transport``/``get_json`` callables so tests run without sockets.
"""

from __future__ import annotations

from collections.abc import Callable, Iterable
from dataclasses import dataclass
from enum import Enum
import json
from typing import Any

from CadXConfig import (
    CHAT_PATH,
    DEFAULT_MODEL,
    MODEL_LIST_TIMEOUT_SECONDS,
    MODELS_PATH,
    OLLAMA_BASE_URL,
    REQUEST_TIMEOUT_SECONDS,
    SYSTEM_PROMPT,
    configured_model,
)
from CadXToolProtocol import ToolRegistry, ToolResult


class ChatClientError(RuntimeError):
    """Raised for transport and protocol failures."""


class ChatMessage:
    """One immutable transcript entry."""

    __slots__ = ("role", "text", "tool_call_id", "tool_calls")

    def __init__(
        self,
        role: str,
        text: str,
        *,
        tool_call_id: str = "",
        tool_calls: tuple["ToolCall", ...] = (),
    ) -> None:
        if role not in ("user", "assistant", "system", "tool"):
            raise ValueError(f"Unsupported chat role {role!r}.")
        self.role = role
        self.text = str(text)
        self.tool_call_id = str(tool_call_id)
        self.tool_calls = tuple(tool_calls)

    @classmethod
    def assistant_tool_call(cls, call: "ToolCall") -> "ChatMessage":
        return cls("assistant", "", tool_calls=(call,))

    @classmethod
    def tool_result(cls, result: "ToolExecutionResult") -> "ChatMessage":
        return cls("tool", result.content, tool_call_id=result.call_id)

    def to_payload(self) -> dict[str, Any]:
        if self.role == "tool":
            return {
                "role": "tool",
                "tool_call_id": self.tool_call_id,
                "content": self.text,
            }
        if self.tool_calls:
            return {
                "role": self.role,
                "content": self.text or None,
                "tool_calls": [call.to_payload() for call in self.tool_calls],
            }
        return {"role": self.role, "content": self.text}


@dataclass(frozen=True)
class ToolCall:
    call_id: str
    name: str
    arguments: str

    def to_payload(self) -> dict[str, Any]:
        return {
            "id": self.call_id,
            "type": "function",
            "function": {"name": self.name, "arguments": self.arguments},
        }


@dataclass(frozen=True)
class ToolExecutionResult:
    call_id: str
    name: str
    content: str
    result: ToolResult


class TurnKind(str, Enum):
    STARTED = "started"
    DELTA = "delta"
    COMPLETED = "completed"
    FAILED = "failed"
    CANCELLED = "cancelled"
    TOOL_CALL = "tool_call"
    TOOL_RESULT = "tool_result"


@dataclass(frozen=True)
class TurnEvent:
    kind: TurnKind
    text: str = ""
    message: str = ""
    tool_call: ToolCall | None = None
    tool_result: ToolExecutionResult | None = None


@dataclass(frozen=True)
class ChatRequestContext:
    """Values a turn needs beyond the transcript."""

    base_url: str = OLLAMA_BASE_URL
    chat_path: str = CHAT_PATH
    models_path: str = MODELS_PATH
    model: str = DEFAULT_MODEL
    system_prompt: str = SYSTEM_PROMPT
    timeout_seconds: float = REQUEST_TIMEOUT_SECONDS
    model_list_timeout_seconds: float = MODEL_LIST_TIMEOUT_SECONDS
    tool_registry: ToolRegistry | None = None
    max_tool_calls: int = 16
    max_snapshot_rebuilds: int = 4


Transport = Callable[[str, dict[str, str], bytes, float], Iterable[bytes]]
GetJson = Callable[[str, float], dict[str, Any]]


@dataclass
class _ToolCallAccumulator:
    call_id: str = ""
    name: str = ""
    arguments: str = ""

    def merge(self, fragment: Any) -> None:
        if not isinstance(fragment, dict):
            return
        self.call_id += str(fragment.get("id") or "")
        function = fragment.get("function")
        if not isinstance(function, dict):
            return
        self.name += str(function.get("name") or "")
        arguments = function.get("arguments")
        if arguments is not None:
            self.arguments += (
                arguments if isinstance(arguments, str) else json.dumps(arguments, separators=(",", ":"))
            )

    def build(self, index: int) -> ToolCall:
        return ToolCall(
            self.call_id or f"cadx-call-{index}",
            self.name,
            self.arguments or "{}",
        )


def default_transport(
    url: str, headers: dict[str, str], body: bytes, timeout_seconds: float
) -> Iterable[bytes]:
    """Post the request and yield raw response chunks as they arrive."""

    from urllib.error import HTTPError
    from urllib.request import Request, urlopen

    request = Request(url, data=body, headers=headers, method="POST")
    try:
        stream = urlopen(request, timeout=timeout_seconds)
    except HTTPError as exc:
        raise ChatClientError(_http_error_message(exc)) from exc
    with stream:
        while True:
            chunk = stream.read(4096)
            if not chunk:
                return
            yield chunk


def default_get_json(url: str, timeout_seconds: float) -> dict[str, Any]:
    """GET one JSON document."""

    from urllib.request import urlopen

    with urlopen(url, timeout=timeout_seconds) as response:
        payload = json.loads(response.read().decode("utf-8"))
    if not isinstance(payload, dict):
        raise ChatClientError("The server returned an unexpected payload.")
    return payload


def _http_error_message(exc: HTTPError) -> str:
    detail = ""
    try:
        detail = exc.read().decode("utf-8", errors="replace")[:500]
    except Exception:
        pass
    message = ""
    try:
        payload = json.loads(detail)
        error = payload.get("error")
        if isinstance(error, dict):
            message = str(error.get("message") or "")
        elif isinstance(error, str):
            message = error
    except ValueError:
        pass
    summary = f"Ollama rejected the request (HTTP {exc.code})."
    return f"{summary} {message}".strip() if message else (
        f"{summary} {detail}".strip()
    )


def parse_sse_events(chunks: Iterable[bytes]) -> Iterable[dict[str, Any]]:
    """Yield the JSON payload of every ``data:`` line in an SSE byte stream.

    Malformed fragments are skipped; the ``[DONE]`` sentinel ends the stream.
    """

    buffer = b""
    for chunk in chunks:
        buffer += chunk
        while b"\n" in buffer:
            line, buffer = buffer.split(b"\n", 1)
            stripped = line.strip()
            if not stripped.startswith(b"data:"):
                continue
            payload_text = stripped[len(b"data:") :].strip()
            if not payload_text:
                continue
            if payload_text == b"[DONE]":
                return
            try:
                payload = json.loads(payload_text.decode("utf-8"))
            except ValueError:
                continue
            if isinstance(payload, dict):
                yield payload


class OllamaClient:
    """Streaming chat client for one local Ollama server."""

    def __init__(
        self,
        *,
        context: ChatRequestContext | None = None,
        model: str | None = None,
        tool_registry: ToolRegistry | None = None,
        transport: Transport = default_transport,
        get_json: GetJson = default_get_json,
    ) -> None:
        base = context or ChatRequestContext()
        self._explicit_model = model is not None and bool(model.strip())
        explicit_model = model or configured_model() or base.model
        self._context = ChatRequestContext(
            base_url=base.base_url,
            chat_path=base.chat_path,
            models_path=base.models_path,
            model=explicit_model,
            system_prompt=base.system_prompt,
            timeout_seconds=base.timeout_seconds,
            model_list_timeout_seconds=base.model_list_timeout_seconds,
            tool_registry=tool_registry if tool_registry is not None else base.tool_registry,
            max_tool_calls=base.max_tool_calls,
            max_snapshot_rebuilds=base.max_snapshot_rebuilds,
        )
        self._transport = transport
        self._get_json = get_json
        self._resolved_model: str | None = None

    @property
    def model(self) -> str:
        """Best-known model name without touching the network."""

        if self._resolved_model:
            return self._resolved_model
        return self._context.model

    def list_models(self) -> tuple[str, ...]:
        """Return the locally available Ollama model names."""

        payload = self._get_json(
            f"{self._context.base_url}{self._context.models_path}",
            self._context.model_list_timeout_seconds,
        )
        names = [
            str(item.get("name") or item.get("model") or "")
            for item in payload.get("models") or []
            if isinstance(item, dict)
        ]
        return tuple(dict.fromkeys(name for name in names if name))

    def set_model(self, model: str) -> None:
        """Use one model selected by the user for subsequent turns."""

        selected = str(model or "").strip()
        if not selected:
            return
        self._context = ChatRequestContext(
            base_url=self._context.base_url,
            chat_path=self._context.chat_path,
            models_path=self._context.models_path,
            model=selected,
            system_prompt=self._context.system_prompt,
            timeout_seconds=self._context.timeout_seconds,
            model_list_timeout_seconds=self._context.model_list_timeout_seconds,
            tool_registry=self._context.tool_registry,
            max_tool_calls=self._context.max_tool_calls,
            max_snapshot_rebuilds=self._context.max_snapshot_rebuilds,
        )
        self._explicit_model = True
        self._resolved_model = selected

    def resolve_model(self) -> str:
        """Return a usable model, preferring the server's first installed one.

        Preference order: the explicit model, the Preferences selection, the
        first model reported by the server, then :data:`DEFAULT_MODEL`.  The
        result is cached; network failures fall back silently because the
        turn itself will surface a precise server error.
        """

        if self._resolved_model:
            return self._resolved_model
        if self._explicit_model:
            self._resolved_model = self._context.model
            return self._resolved_model
        preferred = configured_model()
        if preferred:
            self._resolved_model = preferred
            return self._resolved_model
        try:
            names = self.list_models()
        except Exception:
            return self._context.model
        if names:
            self._resolved_model = names[0]
        return self._resolved_model or self._context.model

    def stream_turn(
        self,
        history: list[ChatMessage],
        on_event: Callable[[TurnEvent], None],
        should_cancel: Callable[[], bool],
    ) -> str:
        """Run one assistant turn; return the completed assistant text.

        Emits :class:`TurnEvent` values through ``on_event``.  Raises only
        for programming errors; runtime failures arrive as FAILED events.
        """

        on_event(TurnEvent(TurnKind.STARTED))
        model = self.resolve_model()
        headers = {"Content-Type": "application/json"}
        url = f"{self._context.base_url}{self._context.chat_path}"
        request_history = list(history)
        call_count = 0
        snapshot_count = 0
        while True:
            if should_cancel():
                on_event(TurnEvent(TurnKind.CANCELLED))
                return ""
            collected: list[str] = []
            tool_accumulators: dict[int, _ToolCallAccumulator] = {}
            failure = ""
            body = json.dumps(self._request_body(request_history, model)).encode("utf-8")
            try:
                for payload in parse_sse_events(
                    self._transport(url, headers, body, self._context.timeout_seconds)
                ):
                    if should_cancel():
                        on_event(TurnEvent(TurnKind.CANCELLED))
                        return ""
                    delta = self._delta_text(payload)
                    if delta:
                        collected.append(delta)
                        on_event(TurnEvent(TurnKind.DELTA, text=delta))
                    self._merge_tool_calls(payload, tool_accumulators)
                    if payload.get("error"):
                        failure = str(payload["error"])
                        break
            except ChatClientError as exc:
                failure = str(exc)
            if failure:
                on_event(TurnEvent(TurnKind.FAILED, message=failure))
                return ""

            tool_calls = tuple(
                accumulator.build(index)
                for index, accumulator in sorted(tool_accumulators.items())
            )
            if tool_calls:
                if call_count + len(tool_calls) > self._context.max_tool_calls:
                    on_event(
                        TurnEvent(
                            TurnKind.FAILED,
                            message="The model exceeded the per-turn tool-call limit.",
                        )
                    )
                    return ""
                request_history.append(
                    ChatMessage(
                        "assistant",
                        "".join(collected),
                        tool_calls=tool_calls,
                    )
                )
                for call in tool_calls:
                    call_count += 1
                    if call.name == "assembly.graph_snapshot":
                        snapshot_count += 1
                    if snapshot_count > self._context.max_snapshot_rebuilds:
                        result = ToolResult.failure(
                            "CADX_GRAPH_LIMIT_EXCEEDED",
                            "The model exceeded the per-turn snapshot rebuild limit.",
                            retryable=True,
                        )
                    elif len(call.arguments.encode("utf-8")) > 64 * 1024:
                        result = ToolResult.failure(
                            "CADX_TOOL_ARGUMENTS_INVALID",
                            "Tool arguments exceed the byte limit.",
                        )
                    elif self._context.tool_registry is None:
                        result = ToolResult.failure(
                            "CADX_TOOL_ARGUMENTS_INVALID",
                            f"Unknown tool {call.name!r}.",
                        )
                    else:
                        result = self._context.tool_registry.execute(call.name, call.arguments)
                    on_event(TurnEvent(TurnKind.TOOL_CALL, tool_call=call))
                    execution = ToolExecutionResult(
                        call.call_id,
                        call.name,
                        json.dumps(result.to_payload(), ensure_ascii=False, sort_keys=True),
                        result,
                    )
                    request_history.append(ChatMessage.tool_result(execution))
                    on_event(TurnEvent(TurnKind.TOOL_RESULT, tool_result=execution))
                continue

            text = "".join(collected)
            if not text:
                on_event(
                    TurnEvent(TurnKind.FAILED, message="The model returned an empty response.")
                )
                return ""
            on_event(TurnEvent(TurnKind.COMPLETED, text=text))
            return text

    # -- internals -------------------------------------------------------------

    def _request_body(self, history: list[ChatMessage], model: str) -> dict[str, Any]:
        messages: list[dict[str, Any]] = []
        if self._context.system_prompt:
            messages.append(
                ChatMessage("system", self._context.system_prompt).to_payload()
            )
        messages.extend(message.to_payload() for message in history)
        body: dict[str, Any] = {"model": model, "messages": messages, "stream": True}
        if self._context.tool_registry is not None:
            body["tools"] = list(self._context.tool_registry.provider_definitions())
        return body

    @staticmethod
    def _merge_tool_calls(
        payload: dict[str, Any], accumulators: dict[int, _ToolCallAccumulator]
    ) -> None:
        choices = payload.get("choices")
        if not isinstance(choices, list) or not choices or not isinstance(choices[0], dict):
            return
        choice = choices[0]
        delta = choice.get("delta")
        if not isinstance(delta, dict):
            delta = choice.get("message")
        if not isinstance(delta, dict):
            return
        fragments = delta.get("tool_calls")
        if not isinstance(fragments, list):
            return
        for fallback_index, fragment in enumerate(fragments):
            if not isinstance(fragment, dict):
                continue
            raw_index = fragment.get("index", fallback_index)
            try:
                index = int(raw_index)
            except (TypeError, ValueError):
                index = fallback_index
            accumulators.setdefault(index, _ToolCallAccumulator()).merge(fragment)

    @staticmethod
    def _delta_text(payload: dict[str, Any]) -> str:
        choices = payload.get("choices")
        if not isinstance(choices, list) or not choices:
            return ""
        delta = choices[0].get("delta") if isinstance(choices[0], dict) else None
        if not isinstance(delta, dict):
            return ""
        return str(delta.get("content") or "")
