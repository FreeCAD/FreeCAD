# pyright: strict

"""Adapt legacy binding discovery records into the public API model.

The C++ registration scanner remains a discovery/input layer. This module
converts its records into a generated ``PythonApiModel``; model precedence and
conflict handling belong to ``python_api_model.resolve``.
"""

from __future__ import annotations

from collections.abc import Mapping, Sequence
from collections import defaultdict
from dataclasses import replace
import re

from python_api_model.model import (
    ApiAttribute,
    ApiCallableGroup,
    ApiClass,
    ApiModule,
    ApiOrigin,
    ApiSourceLocation,
    PythonApiModel,
)
from python_api_model.normalize import normalize_signature_types
from python_api_model.signatures import (
    ArgumentKind,
    CallableDecoratorFlags,
    CallableSignature,
    SignatureParameter,
    callable_shape,
    parse_signature_parts,
)

from .discovery import group_methods, group_type_methods_by_public_module
from .model import BindingMethod, PublicTypeGroup, StubSignatureOverrides
from .naming import valid_identifier
from .validation import validate_discovered_bindings


def _merge_signature_metadata(
    existing: CallableSignature,
    incoming: CallableSignature,
) -> CallableSignature:
    """Merge descriptive fields after two signatures share one call shape."""

    return replace(
        existing,
        docstring=existing.docstring or incoming.docstring,
        deprecated_message=existing.deprecated_message or incoming.deprecated_message,
        decorators=tuple(dict.fromkeys((*existing.decorators, *incoming.decorators))),
    )


def _unique_signatures(
    signatures: Sequence[CallableSignature],
) -> tuple[CallableSignature, ...]:
    """Deduplicate signatures by call shape while retaining useful metadata."""

    result: list[CallableSignature] = []
    indexes: dict[tuple[object, ...], int] = {}
    for signature in signatures:
        key = callable_shape(signature)
        index = indexes.get(key)
        if index is None:
            indexes[key] = len(result)
            result.append(signature)
        else:
            result[index] = _merge_signature_metadata(result[index], signature)
    return tuple(result)


def _callable_group_from_methods(
    methods: list[BindingMethod],
    *,
    class_symbol: str | None,
    module_name: str | None,
    stub_signature_overrides: StubSignatureOverrides,
) -> ApiCallableGroup | None:
    """Convert one discovered registration group into callable signatures."""

    signatures: list[CallableSignature] = []
    for method in methods:
        if not valid_identifier(method.python_name):
            raise ValueError(
                f"{method.source}:{method.line}: invalid discovered Python name "
                f"{method.python_name!r}"
            )
        method_overrides = stub_signature_overrides.get(
            (method.source, method.context_name, method.python_name)
        )
        if method_overrides:
            for override in method_overrides:
                parameters = resolve_signature_placeholders(
                    override.parameters,
                    class_symbol,
                    override.class_symbol,
                )
                if class_symbol is not None:
                    parameters = f"self, {parameters}" if parameters else "self"
                returns = resolve_signature_placeholders(
                    override.returns,
                    class_symbol,
                    override.class_symbol,
                )
                signature = parse_signature_parts(
                    method.python_name,
                    parameters,
                    returns,
                    docstring=override.doc or method.doc or None,
                    deprecated_message=override.deprecated_message,
                )
                signature = normalize_signature_types(
                    signature,
                    module_name or method.inferred_module,
                )
                signatures.append(signature)
        else:
            signature = binding_signature(method, class_symbol is not None)
            signature = normalize_signature_types(
                signature,
                module_name or method.inferred_module,
            )
            signatures.append(signature)

    signatures = list(_unique_signatures(signatures))
    if not signatures:
        return None

    first = methods[0]
    doc = next((method.doc for method in methods if method.doc), None)
    return ApiCallableGroup(
        name=first.python_name,
        signatures=tuple(signatures),
        doc=doc,
        origin=ApiOrigin.GENERATED,
        location=ApiSourceLocation(first.source, first.line),
    )


def resolve_signature_placeholders(
    text: str,
    class_symbol: str | None,
    source_class_symbol: str | None = None,
) -> str:
    if not class_symbol:
        return text
    text = text.replace("{class}", class_symbol)
    if source_class_symbol and source_class_symbol != class_symbol:
        return re.sub(rf"\b{re.escape(source_class_symbol)}\b", class_symbol, text)
    return text


def binding_signature(method: BindingMethod, class_method: bool) -> CallableSignature:
    """Normalize a discovered method without rendering and reparsing text."""

    parameters: list[SignatureParameter] = []
    if class_method:
        parameters.append(
            SignatureParameter(
                name="self",
                annotation=None,
                kind=ArgumentKind.POSITIONAL_OR_KEYWORD,
            )
        )
    if method.method_kind in {"keyword", "varargs"}:
        parameters.append(
            SignatureParameter(
                name="args",
                annotation="Any",
                kind=ArgumentKind.VAR_POSITIONAL,
            )
        )
    if method.method_kind == "keyword":
        parameters.append(
            SignatureParameter(
                name="kwargs",
                annotation="Any",
                kind=ArgumentKind.VAR_KEYWORD,
            )
        )
    return CallableSignature(
        name=method.python_name,
        parameters=tuple(parameters),
        return_annotation="Any",
        docstring=method.doc or None,
        flags=CallableDecoratorFlags(),
    )


def _module_function_groups(
    methods: list[BindingMethod],
    stub_signature_overrides: StubSignatureOverrides,
) -> dict[str, tuple[ApiCallableGroup, ...]]:
    module_methods, _, _ = group_methods(methods)
    grouped: dict[str, tuple[ApiCallableGroup, ...]] = {}
    for module_name, module_group in module_methods.items():
        by_name: dict[str, list[BindingMethod]] = defaultdict(list)
        for method in module_group:
            by_name[method.python_name].append(method)
        groups: list[ApiCallableGroup] = []
        for name in sorted(by_name):
            group = _callable_group_from_methods(
                by_name[name],
                class_symbol=None,
                module_name=module_name,
                stub_signature_overrides=stub_signature_overrides,
            )
            if group is not None:
                groups.append(group)
        if groups:
            grouped[module_name] = tuple(groups)
    return grouped


def _type_group_methods(
    type_group: PublicTypeGroup,
    module_name: str,
    stub_signature_overrides: StubSignatureOverrides,
) -> tuple[ApiCallableGroup, ...]:
    by_name: dict[str, list[BindingMethod]] = defaultdict(list)
    for method in type_group.methods:
        by_name[method.python_name].append(method)
    groups: list[ApiCallableGroup] = []
    for name in sorted(by_name):
        group = _callable_group_from_methods(
            by_name[name],
            class_symbol=type_group.class_symbol,
            module_name=module_name,
            stub_signature_overrides=stub_signature_overrides,
        )
        if group is not None:
            groups.append(group)
    return tuple(groups)


def _combine_generated_methods(
    groups: list[ApiCallableGroup],
) -> tuple[ApiCallableGroup, ...]:
    """Combine duplicate generated method groups from one discovery pass."""

    combined: dict[str, ApiCallableGroup] = {}
    for group in groups:
        current = combined.get(group.name)
        if current is None:
            combined[group.name] = group
            continue
        signatures = _unique_signatures((*current.signatures, *group.signatures))
        combined[group.name] = replace(
            current,
            signatures=signatures,
            doc=current.doc or group.doc,
        )
    return tuple(combined[name] for name in sorted(combined))


def adapt_discovered_bindings(
    methods: Sequence[BindingMethod],
    type_registrations: Mapping[str, Sequence[str]],
    stub_signature_overrides: StubSignatureOverrides,
) -> PythonApiModel:
    """Adapt discovered module functions and type methods into a model."""

    method_list = list(methods)
    registration_map = {key: list(names) for key, names in type_registrations.items()}
    validate_discovered_bindings(method_list, registration_map)
    module_functions = _module_function_groups(method_list, stub_signature_overrides)
    type_groups = group_type_methods_by_public_module(method_list, registration_map)
    module_names = set(module_functions) | set(type_groups)
    module_classes: dict[str, dict[str, list[ApiCallableGroup]]] = defaultdict(
        lambda: defaultdict(list)
    )
    class_bases: dict[tuple[str, str], list[str]] = defaultdict(list)
    class_locations: dict[tuple[str, str], ApiSourceLocation | None] = {}
    module_attributes: dict[str, dict[str, ApiAttribute]] = defaultdict(dict)

    for module_name, public_type_groups in type_groups.items():
        for type_group in public_type_groups:
            generated_methods = _type_group_methods(
                type_group,
                module_name,
                stub_signature_overrides,
            )
            class_key = (module_name, type_group.class_symbol)
            module_classes[module_name][type_group.class_symbol].extend(generated_methods)
            for base in type_group.base_symbols:
                if base not in class_bases[class_key]:
                    class_bases[class_key].append(base)
            if class_key not in class_locations:
                class_locations[class_key] = (
                    generated_methods[0].location if generated_methods else None
                )
            if type_group.variable_symbol:
                module_attributes[module_name].setdefault(
                    type_group.variable_symbol,
                    ApiAttribute(
                        name=type_group.variable_symbol,
                        annotation=type_group.class_symbol,
                        origin=ApiOrigin.GENERATED,
                        location=class_locations[class_key],
                    ),
                )

    modules: list[ApiModule] = []
    for module_name in sorted(module_names):
        classes = tuple(
            ApiClass(
                module_name=module_name,
                name=class_name,
                bases=tuple(class_bases[(module_name, class_name)]),
                methods=_combine_generated_methods(methods_for_class),
                origin=ApiOrigin.GENERATED,
                location=class_locations[(module_name, class_name)],
            )
            for class_name, methods_for_class in sorted(module_classes[module_name].items())
        )
        modules.append(
            ApiModule(
                name=module_name,
                functions=module_functions.get(module_name, ()),
                classes=classes,
                attributes=tuple(
                    module_attributes[module_name][name]
                    for name in sorted(module_attributes[module_name])
                ),
                origin=ApiOrigin.GENERATED,
            )
        )

    return PythonApiModel(modules=tuple(modules))
