# pyright: strict

"""Deterministic resolution of Python API declarations from multiple inputs."""

from __future__ import annotations

from collections.abc import Mapping
from dataclasses import Field, dataclass, fields, is_dataclass, replace
from typing import Any, Generic, Literal, Protocol, TypeVar, cast

from .diagnostics import MergeDiagnostic, origin_precedence
from .model import ApiClass, ApiModule, ApiOrigin, ApiSourceLocation, PythonApiModel

SEMANTICALLY_IGNORED_FIELDS = frozenset({"origin", "location"})


class ApiDeclaration(Protocol):
    """Common provenance fields shared by every resolvable declaration."""

    @property
    def origin(self) -> ApiOrigin:
        raise NotImplementedError

    @property
    def location(self) -> ApiSourceLocation | None:
        raise NotImplementedError


T = TypeVar("T")
TDeclaration = TypeVar("TDeclaration", bound=ApiDeclaration)
DeclarationKind = Literal[
    "alias",
    "attribute",
    "class header",
    "function",
    "method",
]


@dataclass(frozen=True)
class ResolutionResult(Generic[T]):
    """Resolved declaration together with merge diagnostics."""

    value: T
    diagnostics: tuple[MergeDiagnostic, ...] = ()


def declaration_shape(value: object) -> object:
    """Return a declaration value without source-location-only differences."""

    if is_dataclass(value):
        # Reflection keeps new declaration fields in semantic equality while
        # provenance remains available only to resolution and diagnostics.
        declaration_fields: tuple[Field[Any], ...] = fields(value)
        return tuple(
            (item.name, declaration_shape(getattr(value, item.name)))
            for item in declaration_fields
            if item.name not in SEMANTICALLY_IGNORED_FIELDS
        )
    if isinstance(value, tuple):
        items = cast(tuple[object, ...], value)
        return tuple(declaration_shape(item) for item in items)
    return value


def _stable_key(value: ApiDeclaration) -> tuple[str, str, int, str]:
    origin = value.origin
    location = value.location
    return (
        origin.value,
        location.path if location is not None else "",
        location.line if location is not None and location.line is not None else 0,
        repr(declaration_shape(value)),
    )


def _preferred_declaration(
    existing: TDeclaration,
    incoming: TDeclaration,
) -> TDeclaration:
    """Choose provenance for semantically identical declarations."""

    highest_priority = max(
        origin_precedence(existing.origin),
        origin_precedence(incoming.origin),
    )
    return min(
        (
            value
            for value in (existing, incoming)
            if origin_precedence(value.origin) == highest_priority
        ),
        key=_stable_key,
    )


def _class_header(value: ApiClass) -> ApiClass:
    """Return the class fields that are exclusive rather than composable."""

    return replace(
        value,
        doc=None,
        methods=(),
        attributes=(),
        location=None,
    )


def _class_header_result(
    existing: ApiClass,
    incoming: ApiClass,
) -> tuple[ApiClass, tuple[MergeDiagnostic, ...]]:
    existing_header = _class_header(existing)
    incoming_header = _class_header(incoming)
    if declaration_shape(existing_header) == declaration_shape(incoming_header):
        winner = _preferred_declaration(existing, incoming)
        return winner, ()

    result = resolve_declaration(
        existing_header,
        incoming_header,
        kind="class header",
        symbol=existing.qualified_name,
    )
    winner = existing if result.value is existing_header else incoming
    return winner, result.diagnostics


def resolve_declaration(
    existing: TDeclaration,
    incoming: TDeclaration,
    *,
    kind: DeclarationKind,
    symbol: str,
) -> ResolutionResult[TDeclaration]:
    """Resolve two declarations according to origin and stable source order."""

    if declaration_shape(existing) == declaration_shape(incoming):
        return ResolutionResult(_preferred_declaration(existing, incoming))

    existing_origin = existing.origin
    incoming_origin = incoming.origin
    existing_priority = origin_precedence(existing_origin)
    incoming_priority = origin_precedence(incoming_origin)

    if incoming_priority > existing_priority:
        return ResolutionResult(incoming)
    if incoming_priority < existing_priority:
        diagnostic = MergeDiagnostic(
            code="lower-precedence-definition",
            severity="warning",
            symbol=symbol,
            message=(
                f"{symbol} ignored lower-precedence {kind} from "
                f"{incoming_origin.value}; keeping {existing_origin.value}"
            ),
            location=incoming.location,
        )
        return ResolutionResult(existing, (diagnostic,))

    winner = min((existing, incoming), key=_stable_key)
    loser = incoming if winner is existing else existing
    winner_location = winner.location
    loser_location = loser.location
    winner_source = (
        f"{winner_location.path}:{winner_location.line}"
        if winner_location is not None and winner_location.line is not None
        else winner_location.path if winner_location is not None else "unknown source"
    )
    loser_source = (
        f"{loser_location.path}:{loser_location.line}"
        if loser_location is not None and loser_location.line is not None
        else loser_location.path if loser_location is not None else "unknown source"
    )
    diagnostic = MergeDiagnostic(
        code="conflicting-definition",
        severity="error",
        symbol=symbol,
        message=(
            f"{symbol} has incompatible {kind} definitions at the same precedence; "
            f"keeping {winner.origin.value} from {winner_source} and ignoring "
            f"{loser.origin.value} from {loser_source}"
        ),
        location=loser.location,
    )
    return ResolutionResult(winner, (diagnostic,))


def resolve_named(
    existing: Mapping[str, TDeclaration],
    incoming: Mapping[str, TDeclaration],
    *,
    kind: DeclarationKind,
    symbol_prefix: str,
) -> tuple[dict[str, TDeclaration], tuple[MergeDiagnostic, ...]]:
    """Resolve a named declaration mapping in canonical key order."""

    resolved = dict(existing)
    diagnostics: list[MergeDiagnostic] = []
    for name in sorted(incoming):
        value = incoming[name]
        if name not in resolved:
            resolved[name] = value
            continue
        result = resolve_declaration(
            resolved[name],
            value,
            kind=kind,
            symbol=f"{symbol_prefix}.{name}",
        )
        resolved[name] = result.value
        diagnostics.extend(result.diagnostics)
    return resolved, tuple(diagnostics)


def merge_api_class(existing: ApiClass, incoming: ApiClass) -> ResolutionResult[ApiClass]:
    """Resolve the class header, then merge composable members independently."""

    winner, class_diagnostics = _class_header_result(existing, incoming)
    methods, method_diagnostics = resolve_named(
        {method.name: method for method in existing.methods},
        {method.name: method for method in incoming.methods},
        kind="method",
        symbol_prefix=existing.qualified_name,
    )
    attributes, attribute_diagnostics = resolve_named(
        {attribute.name: attribute for attribute in existing.attributes},
        {attribute.name: attribute for attribute in incoming.attributes},
        kind="attribute",
        symbol_prefix=existing.qualified_name,
    )
    merged = replace(
        winner,
        methods=tuple(methods[name] for name in sorted(methods)),
        attributes=tuple(attributes[name] for name in sorted(attributes)),
        doc=winner.doc or (incoming.doc if winner is existing else existing.doc),
        bases=winner.bases or (incoming.bases if winner is existing else existing.bases),
        decorators=winner.decorators
        or (incoming.decorators if winner is existing else existing.decorators),
    )
    return ResolutionResult(
        merged,
        class_diagnostics + method_diagnostics + attribute_diagnostics,
    )


def merge_api_module(existing: ApiModule, incoming: ApiModule) -> ResolutionResult[ApiModule]:
    """Merge module members using the central declaration policy."""

    functions, function_diagnostics = resolve_named(
        {function.name: function for function in existing.functions},
        {function.name: function for function in incoming.functions},
        kind="function",
        symbol_prefix=existing.name,
    )
    attributes, attribute_diagnostics = resolve_named(
        {attribute.name: attribute for attribute in existing.attributes},
        {attribute.name: attribute for attribute in incoming.attributes},
        kind="attribute",
        symbol_prefix=existing.name,
    )
    aliases, alias_diagnostics = resolve_named(
        {alias.public_path: alias for alias in existing.aliases},
        {alias.public_path: alias for alias in incoming.aliases},
        kind="alias",
        symbol_prefix=existing.name,
    )

    classes: dict[str, ApiClass] = {klass.name: klass for klass in existing.classes}
    class_diagnostics: list[MergeDiagnostic] = []
    for klass in sorted(incoming.classes, key=lambda item: item.name):
        current = classes.get(klass.name)
        if current is None:
            classes[klass.name] = klass
            continue
        result = merge_api_class(current, klass)
        classes[klass.name] = result.value
        class_diagnostics.extend(result.diagnostics)

    winner = (
        incoming
        if origin_precedence(incoming.origin) > origin_precedence(existing.origin)
        else existing
    )
    merged = replace(
        winner,
        name=existing.name,
        doc=winner.doc or (incoming.doc if winner is existing else existing.doc),
        functions=tuple(functions[name] for name in sorted(functions)),
        classes=tuple(classes[name] for name in sorted(classes)),
        attributes=tuple(attributes[name] for name in sorted(attributes)),
        aliases=tuple(aliases[name] for name in sorted(aliases)),
    )
    return ResolutionResult(
        merged,
        function_diagnostics + tuple(class_diagnostics) + attribute_diagnostics + alias_diagnostics,
    )


def merge_api_models(
    existing: PythonApiModel,
    incoming: PythonApiModel,
) -> ResolutionResult[PythonApiModel]:
    """Merge two API models through the module and declaration policies."""

    modules = {module.name: module for module in existing.modules}
    diagnostics: list[MergeDiagnostic] = []
    for incoming_module in sorted(incoming.modules, key=lambda module: module.name):
        current = modules.get(incoming_module.name)
        if current is None:
            modules[incoming_module.name] = incoming_module
            continue
        result = merge_api_module(current, incoming_module)
        modules[incoming_module.name] = result.value
        diagnostics.extend(result.diagnostics)

    return ResolutionResult(
        PythonApiModel(
            modules=tuple(modules[name] for name in sorted(modules)),
        ),
        tuple(diagnostics),
    )
