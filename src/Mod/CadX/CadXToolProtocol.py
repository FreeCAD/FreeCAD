# SPDX-License-Identifier: LGPL-2.1-or-later

"""Provider-neutral tool contracts for cad-x.

The Ollama adapter consumes :class:`ToolDefinition` objects, but this module
does not import Ollama, Qt, or FreeCAD.  Keeping validation here makes the
tool boundary usable by another provider and keeps malformed model output
from reaching document-facing code.
"""

from __future__ import annotations

from collections.abc import Callable, Mapping
from dataclasses import dataclass
from enum import Enum
import json
import math
import re
import threading
from typing import Any


TOOL_RESULT_SCHEMA_VERSION = "cadx.tool-result.v1"
GRAPH_RESULT_SCHEMA_VERSION = "cadx.assembly-graph-result.v1"
GRAPH_QUERY_RESULT_SCHEMA_VERSION = "cadx.assembly-graph-query-result.v1"
MAX_TOOL_ARGUMENT_BYTES = 64 * 1024
MAX_TOOL_RESULT_BYTES = 128 * 1024

TOOL_ERROR_CODES = frozenset(
    {
        "CADX_NO_ACTIVE_DOCUMENT",
        "CADX_NO_ACTIVE_VIEW",
        "CADX_NO_ACTIVE_ASSEMBLY",
        "CADX_ACTIVE_ASSEMBLY_STALE",
        "CADX_VIEW_CHANGED",
        "CADX_DOCUMENT_BUSY",
        "CADX_CAPTURE_CHANGED",
        "CADX_UNSUPPORTED_OBJECT",
        "CADX_UNRESOLVED_SOURCE",
        "CADX_GRAPH_INVARIANT_FAILED",
        "CADX_GRAPH_LIMIT_EXCEEDED",
        "CADX_GRAPH_NOT_FOUND",
        "CADX_GRAPH_STALE",
        "CADX_GRAPH_REVISION_MISMATCH",
        "CADX_GRAPH_BUILD_FAILED",
        "CADX_GRAPH_CAPTURE_FAILED",
        "CADX_GRAPH_CONSISTENCY_FAILURE",
        "CADX_QUERY_INVALID",
        "CADX_QUERY_CURSOR_INVALID",
        "CADX_QUERY_RESULT_TOO_LARGE",
        "CADX_PRECONDITION_FAILED",
        "CADX_POSTCONDITION_FAILED",
        "CADX_BRIDGE_FAILURE",
        "CADX_DUPLICATE_JOINT",
        "CADX_INVALID_ASSEMBLY",
        "CADX_NO_GRAPH_SCOPE",
        "CADX_NATIVE_MUTATION_FAILED",
        "CADX_PRIMITIVE_INTEGRATION_REQUIRED",
        "CADX_UNSUPPORTED_OPERATION",
        "CADX_UNSUPPORTED_TOOL",
        "CADX_TOOL_ARGUMENTS_INVALID",
        "CADX_TOOL_CANCELLED",
        "CADX_INTERNAL_ERROR",
    }
)


class ToolProtocolError(ValueError):
    """Raised when a tool definition or schema is malformed."""


class ToolClassification(str, Enum):
    READ = "read"
    MUTATION = "mutation"
    PRESENTATION = "presentation"


class ThreadRequirement(str, Enum):
    ANY = "any"
    MAIN_THREAD = "main_thread"
    WORKER = "worker"


ToolExecutor = Callable[[dict[str, Any]], "ToolResult | Mapping[str, Any]"]


def _encoded_size(value: Any) -> int:
    return len(
        json.dumps(value, ensure_ascii=False, sort_keys=True, separators=(",", ":")).encode(
            "utf-8"
        )
    )


def _type_matches(value: Any, expected: str) -> bool:
    if expected == "object":
        return isinstance(value, dict)
    if expected == "array":
        return isinstance(value, list)
    if expected == "string":
        return isinstance(value, str)
    if expected == "boolean":
        return isinstance(value, bool)
    if expected == "integer":
        return isinstance(value, int) and not isinstance(value, bool)
    if expected == "number":
        return isinstance(value, (int, float)) and not isinstance(value, bool)
    return True


def _resolve_schema(schema: Mapping[str, Any], root_schema: Mapping[str, Any]) -> Mapping[str, Any]:
    reference = schema.get("$ref")
    if reference is None:
        return schema
    if not isinstance(reference, str) or not reference.startswith("#/$defs/"):
        raise ToolProtocolError("schema contains an unsupported reference.")
    definition_name = reference.removeprefix("#/$defs/")
    definitions = root_schema.get("$defs")
    if not isinstance(definitions, Mapping) or definition_name not in definitions:
        raise ToolProtocolError(f"schema reference {reference!r} is unresolved.")
    definition = definitions[definition_name]
    if not isinstance(definition, Mapping):
        raise ToolProtocolError(f"schema reference {reference!r} is not an object schema.")
    return definition


def _validate_value(
    value: Any,
    schema: Mapping[str, Any],
    path: str = "arguments",
    root_schema: Mapping[str, Any] | None = None,
) -> None:
    """Validate the deliberately small JSON-schema subset used by CadX.

    This avoids making ``jsonschema`` a FreeCAD module dependency.  Schemas
    are closed at every tool-operation boundary and only use the constructs
    implemented here.  The subset includes the keywords used by the native
    C++ mutation contracts, including ``const``, ``$ref``/``$defs``,
    ``exclusiveMinimum``, and ``uniqueItems``.
    """

    root_schema = schema if root_schema is None else root_schema
    schema = _resolve_schema(schema, root_schema)

    one_of = schema.get("oneOf")
    if one_of is not None:
        errors: list[str] = []
        for branch in one_of:
            try:
                _validate_value(value, branch, path, root_schema)
                return
            except ToolProtocolError as exc:
                errors.append(str(exc))
        raise ToolProtocolError(f"{path} does not match a supported operation.")

    expected = schema.get("type")
    if expected and not _type_matches(value, expected):
        raise ToolProtocolError(f"{path} must be a {expected}.")

    if "const" in schema and value != schema["const"]:
        raise ToolProtocolError(f"{path} has an unsupported value.")

    enum = schema.get("enum")
    if enum is not None and value not in enum:
        raise ToolProtocolError(f"{path} has an unsupported value.")

    if isinstance(value, Mapping):
        required = schema.get("required", ())
        missing = [name for name in required if name not in value]
        if missing:
            raise ToolProtocolError(f"{path} is missing {missing[0]!r}.")
        properties = schema.get("properties", {})
        if schema.get("additionalProperties") is False:
            unknown = sorted(set(value) - set(properties))
            if unknown:
                raise ToolProtocolError(f"{path} contains unknown field {unknown[0]!r}.")
        for key, child in properties.items():
            if key in value:
                _validate_value(value[key], child, f"{path}.{key}", root_schema)

    if isinstance(value, list):
        if "minItems" in schema and len(value) < schema["minItems"]:
            raise ToolProtocolError(f"{path} has too few items.")
        if "maxItems" in schema and len(value) > schema["maxItems"]:
            raise ToolProtocolError(f"{path} has too many items.")
        if schema.get("uniqueItems"):
            fingerprints = [
                json.dumps(item, ensure_ascii=False, sort_keys=True, separators=(",", ":"))
                for item in value
            ]
            if len(fingerprints) != len(set(fingerprints)):
                raise ToolProtocolError(f"{path} must contain unique items.")
    if isinstance(value, list) and isinstance(schema.get("items"), Mapping):
        for index, item in enumerate(value):
            _validate_value(item, schema["items"], f"{path}[{index}]", root_schema)

    if isinstance(value, str):
        if "minLength" in schema and len(value) < schema["minLength"]:
            raise ToolProtocolError(f"{path} is too short.")
        if "maxLength" in schema and len(value) > schema["maxLength"]:
            raise ToolProtocolError(f"{path} is too long.")
    if isinstance(value, (int, float)) and not isinstance(value, bool):
        if isinstance(value, float) and not math.isfinite(value):
            raise ToolProtocolError(f"{path} must be finite.")
        if "minimum" in schema and value < schema["minimum"]:
            raise ToolProtocolError(f"{path} is below its minimum.")
        if "maximum" in schema and value > schema["maximum"]:
            raise ToolProtocolError(f"{path} exceeds its maximum.")
        if "exclusiveMinimum" in schema:
            exclusive_minimum = schema["exclusiveMinimum"]
            if isinstance(exclusive_minimum, bool):
                if exclusive_minimum and "minimum" in schema and value <= schema["minimum"]:
                    raise ToolProtocolError(f"{path} is not above its minimum.")
            elif value <= exclusive_minimum:
                raise ToolProtocolError(f"{path} is not above its minimum.")
        if "exclusiveMaximum" in schema:
            exclusive_maximum = schema["exclusiveMaximum"]
            if isinstance(exclusive_maximum, bool):
                if exclusive_maximum and "maximum" in schema and value >= schema["maximum"]:
                    raise ToolProtocolError(f"{path} is not below its maximum.")
            elif value >= exclusive_maximum:
                raise ToolProtocolError(f"{path} is not below its maximum.")


def _assert_closed_schema(schema: Mapping[str, Any], path: str = "schema") -> None:
    if not isinstance(schema, Mapping):
        raise ToolProtocolError(f"{path} must be a JSON object schema.")
    if "$ref" in schema:
        if not isinstance(schema["$ref"], str):
            raise ToolProtocolError(f"{path} contains an invalid reference.")
        return
    if "oneOf" in schema:
        branches = schema["oneOf"]
        if not isinstance(branches, list) or not branches:
            raise ToolProtocolError(f"{path}.oneOf must contain schemas.")
        for index, branch in enumerate(branches):
            _assert_closed_schema(branch, f"{path}.oneOf[{index}]")
    elif schema.get("type") == "object":
        if schema.get("additionalProperties") is not False:
            raise ToolProtocolError(f"{path} must close additional properties.")
        properties = schema.get("properties", {})
        if not isinstance(properties, Mapping):
            raise ToolProtocolError(f"{path}.properties must be an object.")
        for name, child in properties.items():
            if isinstance(child, Mapping):
                _assert_closed_schema(child, f"{path}.properties.{name}")
    elif schema.get("type") == "array":
        items = schema.get("items")
        if isinstance(items, Mapping):
            _assert_closed_schema(items, f"{path}.items")
    elif (
        schema.get("type") in {"string", "number", "integer", "boolean"}
        or "const" in schema
        or "enum" in schema
    ):
        pass
    elif "$defs" not in schema:
        raise ToolProtocolError(f"{path} must contain a supported schema type.")

    definitions = schema.get("$defs")
    if definitions is not None:
        if not isinstance(definitions, Mapping):
            raise ToolProtocolError(f"{path}.$defs must be an object.")
        for name, definition in definitions.items():
            _assert_closed_schema(definition, f"{path}.$defs.{name}")


@dataclass(frozen=True)
class ToolResult:
    """Bounded provider-neutral tool result envelope."""

    ok: bool
    payload: Mapping[str, Any]
    error: Mapping[str, Any] | None = None

    @classmethod
    def success(cls, payload: Mapping[str, Any]) -> "ToolResult":
        return cls(True, dict(payload), None)

    @classmethod
    def failure(
        cls,
        code: str,
        message: str,
        *,
        retryable: bool = False,
        details: Mapping[str, Any] | None = None,
    ) -> "ToolResult":
        if code not in TOOL_ERROR_CODES:
            code = "CADX_INTERNAL_ERROR"
        return cls(
            False,
            {},
            {
                "code": code,
                "message": str(message),
                "retryable": bool(retryable),
                "details": dict(details or {}),
            },
        )

    def to_payload(self) -> dict[str, Any]:
        result: dict[str, Any] = {
            "schema_version": TOOL_RESULT_SCHEMA_VERSION,
            "ok": self.ok,
        }
        if self.ok:
            result.update(dict(self.payload))
        else:
            result["error"] = dict(self.error or {})
        return result

    def encoded_size(self) -> int:
        return _encoded_size(self.to_payload())


@dataclass(frozen=True)
class ToolDefinition:
    name: str
    description: str
    classification: ToolClassification
    input_schema: Mapping[str, Any]
    output_schema_version: str
    executor: ToolExecutor
    thread_requirement: ThreadRequirement = ThreadRequirement.ANY
    result_size_limit: int = MAX_TOOL_RESULT_BYTES

    def __post_init__(self) -> None:
        if not re.fullmatch(r"[a-z][a-z0-9_.-]{2,127}", self.name):
            raise ToolProtocolError(f"Invalid tool name {self.name!r}.")
        if not self.description.strip():
            raise ToolProtocolError(f"Tool {self.name!r} needs a description.")
        _assert_closed_schema(self.input_schema)
        if not re.fullmatch(r"[a-z0-9_.-]+\.v[0-9]+", self.output_schema_version):
            raise ToolProtocolError(f"Invalid output schema for {self.name!r}.")
        if not callable(self.executor):
            raise ToolProtocolError(f"Tool {self.name!r} needs an executor.")
        if self.result_size_limit <= 0:
            raise ToolProtocolError("Tool result size must be positive.")

    def provider_definition(self) -> dict[str, Any]:
        """Return the OpenAI-compatible function shape at the adapter edge."""

        return {
            "type": "function",
            "function": {
                "name": self.name,
                "description": self.description,
                "parameters": dict(self.input_schema),
            },
        }


class ToolRegistry:
    """Thread-safe registry with validation and bounded execution."""

    def __init__(self, *, default_result_size_limit: int = MAX_TOOL_RESULT_BYTES) -> None:
        self._definitions: dict[str, ToolDefinition] = {}
        self._lock = threading.RLock()
        self._default_result_size_limit = default_result_size_limit

    def register(self, definition: ToolDefinition) -> None:
        with self._lock:
            if definition.name in self._definitions:
                raise ToolProtocolError(f"Duplicate tool name {definition.name!r}.")
            self._definitions[definition.name] = definition

    def get(self, name: str) -> ToolDefinition | None:
        with self._lock:
            return self._definitions.get(name)

    def definitions(self) -> tuple[ToolDefinition, ...]:
        with self._lock:
            return tuple(self._definitions[name] for name in sorted(self._definitions))

    def provider_definitions(self) -> tuple[dict[str, Any], ...]:
        return tuple(definition.provider_definition() for definition in self.definitions())

    def execute(self, name: str, arguments: Mapping[str, Any] | str) -> ToolResult:
        definition = self.get(name)
        if definition is None:
            return ToolResult.failure("CADX_TOOL_ARGUMENTS_INVALID", f"Unknown tool {name!r}.")

        try:
            if isinstance(arguments, str):
                if len(arguments.encode("utf-8")) > MAX_TOOL_ARGUMENT_BYTES:
                    raise ToolProtocolError("Tool arguments exceed the byte limit.")
                parsed = json.loads(arguments)
            else:
                parsed = dict(arguments)
            if not isinstance(parsed, dict):
                raise ToolProtocolError("Tool arguments must be an object.")
            if _encoded_size(parsed) > MAX_TOOL_ARGUMENT_BYTES:
                raise ToolProtocolError("Tool arguments exceed the byte limit.")
            _validate_value(parsed, definition.input_schema)
            raw = definition.executor(parsed)
            result = raw if isinstance(raw, ToolResult) else ToolResult.success(raw)
            if result.ok and "schema_version" not in result.payload:
                result = ToolResult.failure(
                    "CADX_INTERNAL_ERROR",
                    f"Tool {name!r} omitted its output schema version.",
                )
            elif result.ok and result.payload.get("schema_version") != definition.output_schema_version:
                result = ToolResult.failure(
                    "CADX_INTERNAL_ERROR",
                    f"Tool {name!r} returned the wrong output schema version.",
                )
            if result.encoded_size() > min(
                self._default_result_size_limit, definition.result_size_limit
            ):
                return ToolResult.failure(
                    "CADX_QUERY_RESULT_TOO_LARGE",
                    f"Tool {name!r} returned too much data.",
                    retryable=True,
                )
            return result
        except ToolProtocolError as exc:
            return ToolResult.failure("CADX_TOOL_ARGUMENTS_INVALID", str(exc))
        except Exception as exc:  # tool failures must not escape into the model loop
            return ToolResult.failure("CADX_INTERNAL_ERROR", str(exc), retryable=True)


def _common_query_properties() -> dict[str, Any]:
    return {
        "graph_id": {"type": "string", "maxLength": 256},
        "graph_revision": {"type": "string", "maxLength": 128},
        "operation": {"type": "string"},
        "limit": {"type": "integer", "minimum": 1, "maximum": 100},
        "cursor": {"type": "string", "maxLength": 1024},
    }


def _query_branch(operation: str, properties: Mapping[str, Any], required: list[str]) -> dict[str, Any]:
    merged = _common_query_properties()
    merged["operation"] = {"type": "string", "enum": [operation]}
    merged.update(properties)
    return {
        "type": "object",
        "properties": merged,
        "required": ["graph_id", "graph_revision", "operation", *required],
        "additionalProperties": False,
    }


GRAPH_SNAPSHOT_SCHEMA: dict[str, Any] = {
    "type": "object",
    "properties": {
        "geometry_detail": {
            "type": "string",
            "enum": ["none", "summary"],
            "default": "summary",
        },
        "include_view_state": {"type": "boolean", "default": True},
        "refresh": {
            "type": "string",
            "enum": ["if_stale", "always"],
            "default": "if_stale",
        },
    },
    "additionalProperties": False,
}

GRAPH_QUERY_SCHEMA: dict[str, Any] = {
    "oneOf": [
        _query_branch("summary", {}, []),
        _query_branch(
            "find_nodes",
            {
                "node_kinds": {"type": "array", "maxItems": 16, "items": {"type": "string"}},
                "native_type": {"type": "string", "maxLength": 256},
                "label": {"type": "string", "maxLength": 256},
                "label_match": {"type": "string", "enum": ["exact", "contains"]},
                "semantic_part_kind": {"type": "string", "maxLength": 64},
                "visible": {"type": "boolean"},
                "source_document_uid": {"type": "string", "maxLength": 256},
            },
            [],
        ),
        _query_branch(
            "neighbors",
            {
                "start_node_ids": {
                    "type": "array",
                    "minItems": 1,
                    "maxItems": 16,
                    "items": {"type": "string", "maxLength": 256},
                },
                "direction": {"type": "string", "enum": ["incoming", "outgoing", "both"]},
                "edge_kinds": {"type": "array", "maxItems": 16, "items": {"type": "string"}},
            },
            ["start_node_ids"],
        ),
        _query_branch(
            "subgraph",
            {
                "start_node_ids": {
                    "type": "array",
                    "minItems": 1,
                    "maxItems": 16,
                    "items": {"type": "string", "maxLength": 256},
                },
                "max_depth": {"type": "integer", "minimum": 0, "maximum": 4},
                "edge_kinds": {"type": "array", "maxItems": 16, "items": {"type": "string"}},
            },
            ["start_node_ids", "max_depth", "edge_kinds"],
        ),
        _query_branch(
            "shortest_path",
            {
                "start_node_id": {"type": "string", "maxLength": 256},
                "target_node_id": {"type": "string", "maxLength": 256},
                "max_depth": {"type": "integer", "minimum": 0, "maximum": 4},
                "edge_kinds": {"type": "array", "maxItems": 16, "items": {"type": "string"}},
            },
            ["start_node_id", "target_node_id", "max_depth", "edge_kinds"],
        ),
    ]
}


ASSEMBLY_MUTATION_RESULT_SCHEMA_VERSION = "cadx.assembly-mutation-result.v1"
ASSEMBLY_CONSTRAINT_RESULT_SCHEMA_VERSION = "cadx.assembly-constraint-result.v1"


ASSEMBLY_CREATE_SCHEMA: dict[str, Any] = {
    "type": "object",
    "properties": {
        "operation": {"const": "create_assembly"},
        "operation_id": {"type": "string", "minLength": 1, "maxLength": 128},
        "expected_graph_revision": {"type": "string", "maxLength": 128},
        "label": {"type": "string", "minLength": 1, "maxLength": 160},
    },
    "required": ["operation", "operation_id", "expected_graph_revision", "label"],
    "additionalProperties": False,
}


ASSEMBLY_INSERT_SCHEMA: dict[str, Any] = {
    "type": "object",
    "properties": {
        "operation": {"const": "insert_component"},
        "operation_id": {"type": "string", "minLength": 1, "maxLength": 128},
        "expected_graph_revision": {"type": "string", "maxLength": 128},
        "assembly": {
            "type": "object",
            "properties": {
                "object_name": {"type": "string", "minLength": 1, "maxLength": 256}
            },
            "required": ["object_name"],
            "additionalProperties": False,
        },
        "source": {
            "type": "object",
            "properties": {
                "document_name": {"type": "string", "minLength": 1, "maxLength": 256},
                "object_name": {"type": "string", "minLength": 1, "maxLength": 256},
            },
            "required": ["document_name", "object_name"],
            "additionalProperties": False,
        },
        "label": {"type": "string", "minLength": 1, "maxLength": 160},
        "placement": {
            "type": "array",
            "minItems": 7,
            "maxItems": 7,
            "items": {"type": "number"},
        },
    },
    "required": ["operation", "operation_id", "expected_graph_revision", "assembly", "source"],
    "additionalProperties": False,
}


def _assembly_grounding_branch(operation: str) -> dict[str, Any]:
    return {
        "type": "object",
        "properties": {
            "operation": {"const": operation},
            "operation_id": {"type": "string", "minLength": 1, "maxLength": 128},
            "expected_graph_revision": {"type": "string", "maxLength": 128},
            "assembly": {
                "type": "object",
                "properties": {
                    "object_name": {"type": "string", "minLength": 1, "maxLength": 256}
                },
                "required": ["object_name"],
                "additionalProperties": False,
            },
            "components": {
                "type": "array",
                "minItems": 1,
                "maxItems": 16,
                "uniqueItems": True,
                "items": {"type": "string", "minLength": 1, "maxLength": 128},
            },
            "expected_component_count": {
                "type": "integer",
                "minimum": 0,
                "maximum": 1000000,
            },
            "expected_grounded_count": {
                "type": "integer",
                "minimum": 0,
                "maximum": 1000000,
            },
        },
        "required": ["operation", "operation_id", "expected_graph_revision", "assembly", "components"],
        "additionalProperties": False,
    }


ASSEMBLY_GROUND_SCHEMA: dict[str, Any] = {
    "oneOf": [
        _assembly_grounding_branch("set_grounded"),
        _assembly_grounding_branch("set_movable"),
    ]
}


ASSEMBLY_JOINT_SCHEMA: dict[str, Any] = {
    "type": "object",
    "properties": {
        "operation": {"const": "create"},
        "operation_id": {"type": "string", "minLength": 1, "maxLength": 128},
        "expected_graph_revision": {"type": "string", "maxLength": 128},
        "assembly": {
            "type": "object",
            "properties": {
                "object_name": {"type": "string", "minLength": 1, "maxLength": 256}
            },
            "required": ["object_name"],
            "additionalProperties": False,
        },
        "first": {"$ref": "#/$defs/connector"},
        "second": {"$ref": "#/$defs/connector"},
        "joint_type": {"enum": ["fixed", "revolute"]},
        "label": {"type": "string", "minLength": 1, "maxLength": 160},
        "reverse": {"type": "boolean"},
        "limits": {
            "type": "object",
            "properties": {
                "minimum_degrees": {"type": "number", "minimum": -180, "maximum": 180},
                "maximum_degrees": {"type": "number", "minimum": -180, "maximum": 180},
            },
            "required": ["minimum_degrees", "maximum_degrees"],
            "additionalProperties": False,
        },
    },
    "required": [
        "operation",
        "operation_id",
        "expected_graph_revision",
        "assembly",
        "first",
        "second",
        "joint_type",
    ],
    "additionalProperties": False,
    "$defs": {
        "connector": {
            "type": "object",
            "properties": {
                "component": {"type": "string", "minLength": 1, "maxLength": 128},
                "connector_type": {"enum": ["element", "interface"]},
                "connector": {"type": "string", "minLength": 1, "maxLength": 512},
                "offset": {
                    "type": "object",
                    "properties": {
                        "translation_mm": {
                            "type": "array",
                            "minItems": 3,
                            "maxItems": 3,
                            "items": {
                                "type": "number",
                                "minimum": -1000000,
                                "maximum": 1000000,
                            },
                        },
                        "rotation_axis": {
                            "type": "array",
                            "minItems": 3,
                            "maxItems": 3,
                            "items": {"type": "number", "minimum": -1, "maximum": 1},
                        },
                        "rotation_degrees": {
                            "type": "number",
                            "minimum": -360,
                            "maximum": 360,
                        },
                    },
                    "required": ["translation_mm", "rotation_axis", "rotation_degrees"],
                    "additionalProperties": False,
                },
            },
            "required": ["component", "connector_type", "connector"],
            "additionalProperties": False,
        }
    },
}


_PUBLIC_NATIVE_TOOLS = frozenset(
    {
        "assembly.graph_snapshot",
        "assembly.graph_query",
        "assembly.create",
        "assembly.insert",
        "assembly.ground",
        "assembly.joint",
    }
)


def _normalize_reported_tool_names(value: Any) -> frozenset[str]:
    if isinstance(value, str):
        values = (value,)
    elif isinstance(value, Mapping):
        values = value.keys()
    else:
        try:
            values = iter(value)
        except TypeError:
            return frozenset()
    return frozenset(name for name in values if isinstance(name, str))


def _reported_native_tool_names(backend: Any) -> frozenset[str]:
    """Read availability from the native service and fail closed.

    ``CadXToolBridge.NativeCadXBackend`` intentionally keeps the imported
    extension private.  The extension itself exposes ``tool_names()``, so the
    registry checks that capability on the bridge's native module as well as
    on small provider-neutral test/integration bridges.
    """

    for owner in (backend, getattr(backend, "_native", None)):
        if owner is None:
            continue
        for attribute in ("tool_names",):
            reporter = getattr(owner, attribute, None)
            if reporter is None:
                continue
            try:
                value = reporter() if callable(reporter) else reporter
            except Exception:
                return frozenset()
            return _normalize_reported_tool_names(value)
    return frozenset()


def _has_native_execution_route(backend: Any, tool_name: str) -> bool:
    """Return whether *tool_name* has a callable route on this backend.

    Native availability is deliberately a two-part contract: the extension
    must report the name and the bridge must have a route that can dispatch
    that family.  This keeps a names-only capability mock (or a partially
    initialized bridge) from advertising tools that fail on first use.
    """

    direct_method_names = {
        "assembly.graph_snapshot": "snapshot",
        "assembly.graph_query": "query",
    }
    direct_method = direct_method_names.get(tool_name)
    if direct_method and callable(getattr(backend, direct_method, None)):
        return True

    for owner in (backend, getattr(backend, "_native", None)):
        if owner is None:
            continue
        if callable(getattr(owner, "execute_tool", None)):
            return True
        if callable(getattr(owner, "execute", None)):
            return True

    # NativeCadXBackend exposes the provider-neutral route as _execute and
    # keeps the extension object private.
    return callable(getattr(backend, "_execute", None))


def _decode_native_result(raw: Any) -> ToolResult:
    """Convert an existing bridge result without rewriting native errors."""

    if isinstance(raw, ToolResult):
        return raw
    if isinstance(raw, (str, bytes, bytearray)):
        try:
            raw = json.loads(raw)
        except (TypeError, ValueError) as exc:
            raise ValueError("native tool returned invalid JSON") from exc
    if not isinstance(raw, Mapping):
        raise ValueError("native tool returned a non-object")
    payload = dict(raw)
    if payload.get("ok") is False:
        error = payload.get("error")
        if not isinstance(error, Mapping):
            raise ValueError("native tool returned a malformed error envelope")
        # Preserve the native code, message, retryability, and any details.
        return ToolResult(False, {}, dict(error))
    return ToolResult.success(payload)


def _native_tool_executor(backend: Any, tool_name: str) -> ToolExecutor:
    """Route one public tool through the existing native service bridge."""

    direct_method_names = {
        "assembly.graph_snapshot": "snapshot",
        "assembly.graph_query": "query",
    }

    def execute(arguments: dict[str, Any]) -> ToolResult:
        direct_method = getattr(backend, direct_method_names.get(tool_name, ""), None)
        if callable(direct_method):
            return _decode_native_result(direct_method(arguments))

        # NativeCadXBackend already owns the JSON conversion and error
        # envelope handling in this private bridge method.  Reuse it for the
        # mutation names without duplicating CAD or graph behavior here.
        bridge_execute = getattr(backend, "_execute", None)
        if callable(bridge_execute):
            return _decode_native_result(
                bridge_execute(tool_name, arguments, _output_schema_for(tool_name))
            )

        # Also accept the extension-shaped bridge directly.  Its C++ binding
        # takes the exact JSON string expected by CadXService::executeTool().
        native_execute = getattr(backend, "execute_tool", None)
        if callable(native_execute):
            encoded = json.dumps(arguments, ensure_ascii=False, sort_keys=True, separators=(",", ":"))
            return _decode_native_result(native_execute(tool_name, encoded))

        generic_execute = getattr(backend, "execute", None)
        if callable(generic_execute):
            return _decode_native_result(generic_execute(tool_name, arguments))

        raise RuntimeError(f"native bridge cannot execute {tool_name!r}")

    return execute


def _output_schema_for(tool_name: str) -> str:
    if tool_name in {"assembly.ground", "assembly.joint"}:
        return ASSEMBLY_CONSTRAINT_RESULT_SCHEMA_VERSION
    if tool_name in {"assembly.create", "assembly.insert"}:
        return ASSEMBLY_MUTATION_RESULT_SCHEMA_VERSION
    if tool_name == "assembly.graph_snapshot":
        return GRAPH_RESULT_SCHEMA_VERSION
    if tool_name == "assembly.graph_query":
        return GRAPH_QUERY_RESULT_SCHEMA_VERSION
    return TOOL_RESULT_SCHEMA_VERSION


def make_graph_registry(backend: Any = None) -> ToolRegistry:
    """Create the provider-neutral registry from native service capabilities."""

    if backend is None:
        from CadXToolBridge import default_backend

        backend = default_backend()
    reported = _reported_native_tool_names(backend) & _PUBLIC_NATIVE_TOOLS
    available = frozenset(
        name for name in reported if _has_native_execution_route(backend, name)
    )
    registry = ToolRegistry()
    definitions = {
        "assembly.graph_snapshot": ToolDefinition(
            name="assembly.graph_snapshot",
            description=(
                "Capture the active Assembly view as a revisioned semantic graph "
                "and retain it in memory for bounded queries."
            ),
            classification=ToolClassification.READ,
            input_schema=GRAPH_SNAPSHOT_SCHEMA,
            output_schema_version=GRAPH_RESULT_SCHEMA_VERSION,
            thread_requirement=ThreadRequirement.MAIN_THREAD,
            executor=_native_tool_executor(backend, "assembly.graph_snapshot"),
        ),
        "assembly.graph_query": ToolDefinition(
            name="assembly.graph_query",
            description="Query nodes and relationships from one exact stored Assembly graph revision.",
            classification=ToolClassification.READ,
            input_schema=GRAPH_QUERY_SCHEMA,
            output_schema_version=GRAPH_QUERY_RESULT_SCHEMA_VERSION,
            thread_requirement=ThreadRequirement.WORKER,
            executor=_native_tool_executor(backend, "assembly.graph_query"),
        ),
        "assembly.create": ToolDefinition(
            name="assembly.create",
            description="Create a FreeCAD Assembly and publish its verified graph revision.",
            classification=ToolClassification.MUTATION,
            input_schema=ASSEMBLY_CREATE_SCHEMA,
            output_schema_version=ASSEMBLY_MUTATION_RESULT_SCHEMA_VERSION,
            thread_requirement=ThreadRequirement.MAIN_THREAD,
            result_size_limit=MAX_TOOL_RESULT_BYTES,
            executor=_native_tool_executor(backend, "assembly.create"),
        ),
        "assembly.insert": ToolDefinition(
            name="assembly.insert",
            description="Insert a resolved source object as an Assembly link and publish its verified graph revision.",
            classification=ToolClassification.MUTATION,
            input_schema=ASSEMBLY_INSERT_SCHEMA,
            output_schema_version=ASSEMBLY_MUTATION_RESULT_SCHEMA_VERSION,
            thread_requirement=ThreadRequirement.MAIN_THREAD,
            result_size_limit=MAX_TOOL_RESULT_BYTES,
            executor=_native_tool_executor(backend, "assembly.insert"),
        ),
        "assembly.ground": ToolDefinition(
            name="assembly.ground",
            description="Ground or release Assembly components and publish their verified graph revision.",
            classification=ToolClassification.MUTATION,
            input_schema=ASSEMBLY_GROUND_SCHEMA,
            output_schema_version=ASSEMBLY_CONSTRAINT_RESULT_SCHEMA_VERSION,
            thread_requirement=ThreadRequirement.MAIN_THREAD,
            result_size_limit=MAX_TOOL_RESULT_BYTES,
            executor=_native_tool_executor(backend, "assembly.ground"),
        ),
        "assembly.joint": ToolDefinition(
            name="assembly.joint",
            description="Create a fixed or revolute Assembly joint and publish its verified graph revision.",
            classification=ToolClassification.MUTATION,
            input_schema=ASSEMBLY_JOINT_SCHEMA,
            output_schema_version=ASSEMBLY_CONSTRAINT_RESULT_SCHEMA_VERSION,
            thread_requirement=ThreadRequirement.MAIN_THREAD,
            result_size_limit=MAX_TOOL_RESULT_BYTES,
            executor=_native_tool_executor(backend, "assembly.joint"),
        )
    }
    for name in sorted(available):
        registry.register(definitions[name])
    return registry


__all__ = [
    "ASSEMBLY_CONSTRAINT_RESULT_SCHEMA_VERSION",
    "ASSEMBLY_CREATE_SCHEMA",
    "ASSEMBLY_GROUND_SCHEMA",
    "ASSEMBLY_INSERT_SCHEMA",
    "ASSEMBLY_JOINT_SCHEMA",
    "ASSEMBLY_MUTATION_RESULT_SCHEMA_VERSION",
    "GRAPH_QUERY_RESULT_SCHEMA_VERSION",
    "GRAPH_QUERY_SCHEMA",
    "GRAPH_RESULT_SCHEMA_VERSION",
    "GRAPH_SNAPSHOT_SCHEMA",
    "ThreadRequirement",
    "ToolClassification",
    "ToolDefinition",
    "ToolProtocolError",
    "ToolRegistry",
    "ToolResult",
    "make_graph_registry",
]
