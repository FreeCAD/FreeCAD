# SPDX-License-Identifier: LGPL-2.1-or-later

"""Bridge the provider-neutral graph tools to the native CadX service.

The bridge is intentionally small: it owns no document pointers and never
serializes a live FreeCAD object.  In a FreeCAD process it delegates to the
``CadXApp`` extension.  A deterministic unavailable backend keeps the Python
contract tests usable outside FreeCAD.
"""

from __future__ import annotations

import json
from typing import Any

from CadXToolProtocol import (
    GRAPH_QUERY_RESULT_SCHEMA_VERSION,
    GRAPH_RESULT_SCHEMA_VERSION,
    ToolResult,
)


class GraphBackend:
    """Backend protocol used by the two public graph tool executors."""

    def snapshot(self, arguments: dict[str, Any]) -> ToolResult:
        raise NotImplementedError

    def query(self, arguments: dict[str, Any]) -> ToolResult:
        raise NotImplementedError


class UnavailableGraphBackend(GraphBackend):
    """Safe result when the native service or active Assembly is unavailable."""

    def snapshot(self, arguments: dict[str, Any]) -> ToolResult:
        del arguments
        return ToolResult.failure(
            "CADX_NO_ACTIVE_ASSEMBLY",
            "No exact Assembly is active in a FreeCAD 3D view.",
            retryable=True,
        )

    def query(self, arguments: dict[str, Any]) -> ToolResult:
        del arguments
        return ToolResult.failure(
            "CADX_GRAPH_NOT_FOUND",
            "Take an Assembly graph snapshot before querying it.",
            retryable=True,
        )


class NativeCadXBackend(GraphBackend):
    """Call the optional native extension without importing it in unit tests."""

    def __init__(self) -> None:
        self._native = None
        try:
            import CadXGuiApp as CadXApp  # type: ignore[import-not-found]

            self._native = CadXApp
        except ImportError:
            try:
                import CadXApp  # type: ignore[import-not-found]

                self._native = CadXApp
            except ImportError:
                pass

    def _execute(self, tool_name: str, arguments: dict[str, Any], version: str) -> ToolResult:
        if self._native is None or not hasattr(self._native, "execute_tool"):
            return UnavailableGraphBackend().snapshot(arguments) if tool_name.endswith("snapshot") else UnavailableGraphBackend().query(arguments)
        try:
            raw = self._native.execute_tool(
                tool_name,
                json.dumps(arguments, ensure_ascii=False, sort_keys=True, separators=(",", ":")),
            )
            payload = json.loads(str(raw))
            if not isinstance(payload, dict):
                raise ValueError("native tool returned a non-object")
            if payload.get("ok") is False:
                return ToolResult(False, {}, payload.get("error"))
            payload.setdefault("schema_version", version)
            return ToolResult.success(payload)
        except Exception as exc:
            return ToolResult.failure("CADX_INTERNAL_ERROR", str(exc), retryable=True)

    def snapshot(self, arguments: dict[str, Any]) -> ToolResult:
        return self._execute(
            "assembly.graph_snapshot", arguments, GRAPH_RESULT_SCHEMA_VERSION
        )

    def query(self, arguments: dict[str, Any]) -> ToolResult:
        return self._execute(
            "assembly.graph_query", arguments, GRAPH_QUERY_RESULT_SCHEMA_VERSION
        )


_DEFAULT_BACKEND: GraphBackend | None = None


def default_backend() -> GraphBackend:
    global _DEFAULT_BACKEND
    if _DEFAULT_BACKEND is None:
        _DEFAULT_BACKEND = NativeCadXBackend()
    return _DEFAULT_BACKEND


__all__ = [
    "GraphBackend",
    "NativeCadXBackend",
    "UnavailableGraphBackend",
    "default_backend",
]
