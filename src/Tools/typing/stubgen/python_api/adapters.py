# pyright: strict

"""Adapt legacy binding discovery records into the public API model.

The C++ registration scanner remains a discovery/input layer.  This module
converts its records into the neutral ``ApiModel`` before either stubs or
documentation are rendered, while keeping curated source-adjacent declarations
at higher precedence.
"""

from __future__ import annotations

import ast
from collections import defaultdict
from dataclasses import replace

from ..discovery import group_methods, group_type_methods_by_public_module
from ..model import BindingMethod, PublicTypeGroup, StubSignatureOverrides
from ..render import render_stub_lines
from ..signature_parser import CallableSignature, parse_callable_group
from .model import (
    ApiAttribute,
    ApiCallableGroup,
    ApiClass,
    ApiModel,
    ApiModule,
    ApiOrigin,
    ApiSourceLocation,
)


def _callable_group_from_methods(
    methods: list[BindingMethod],
    *,
    class_symbol: str | None,
    stub_signature_overrides: StubSignatureOverrides,
) -> ApiCallableGroup | None:
    """Convert one discovered registration group into callable signatures."""

    nodes: list[ast.FunctionDef | ast.AsyncFunctionDef] = []
    seen_declarations: set[tuple[object, ...]] = set()
    for method in methods:
        lines = render_stub_lines(
            method,
            class_method=class_symbol is not None,
            class_symbol=class_symbol,
            stub_signature_overrides=stub_signature_overrides,
        )
        if not any(line.lstrip().startswith(("def ", "async def ", "@")) for line in lines):
            continue
        try:
            tree = ast.parse("\n".join(lines) + "\n")
        except SyntaxError:
            continue
        for node in tree.body:
            if not isinstance(node, (ast.FunctionDef, ast.AsyncFunctionDef)):
                continue
            declaration = (
                node.name,
                ast.dump(node.args, include_attributes=False),
                ast.dump(node.returns, include_attributes=False) if node.returns else None,
                tuple(ast.dump(item, include_attributes=False) for item in node.decorator_list),
            )
            if declaration in seen_declarations:
                continue
            seen_declarations.add(declaration)
            nodes.append(node)

    if not nodes:
        return None

    signatures: tuple[CallableSignature, ...] = parse_callable_group(nodes)
    first = methods[0]
    doc = next((method.doc for method in methods if method.doc), None)
    return ApiCallableGroup(
        name=first.python_name,
        signatures=signatures,
        doc=doc,
        is_method=class_symbol is not None,
        origin=ApiOrigin.GENERATED,
        location=ApiSourceLocation(first.source, first.line),
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
        groups = [
            group
            for name in sorted(by_name)
            if (
                group := _callable_group_from_methods(
                    by_name[name],
                    class_symbol=None,
                    stub_signature_overrides=stub_signature_overrides,
                )
            )
            is not None
        ]
        if groups:
            grouped[module_name] = tuple(groups)
    return grouped


def _type_group_methods(
    type_group: PublicTypeGroup,
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
            stub_signature_overrides=stub_signature_overrides,
        )
        if group is not None:
            groups.append(group)
    return tuple(groups)


def _merge_generated_methods(
    existing: tuple[ApiCallableGroup, ...],
    generated: tuple[ApiCallableGroup, ...],
) -> tuple[ApiCallableGroup, ...]:
    """Add discovered methods without replacing curated declarations."""

    by_name = {group.name: group for group in existing}
    for group in generated:
        by_name.setdefault(group.name, group)
    return tuple(by_name[name] for name in sorted(by_name))


def merge_discovered_bindings(
    model: ApiModel,
    methods: list[BindingMethod],
    type_registrations: dict[str, list[str]],
    stub_signature_overrides: StubSignatureOverrides,
) -> ApiModel:
    """Add discovered module functions and type methods to ``model``."""

    module_functions = _module_function_groups(methods, stub_signature_overrides)
    type_groups = group_type_methods_by_public_module(methods, type_registrations)
    modules = {module.name: module for module in model.modules}

    for module_name, functions in module_functions.items():
        module = modules.get(module_name, ApiModule(name=module_name))
        modules[module_name] = replace(
            module,
            functions=_merge_generated_methods(module.functions, functions),
        )

    for module_name, public_type_groups in type_groups.items():
        module = modules.get(module_name, ApiModule(name=module_name))
        classes = {api_class.name: api_class for api_class in module.classes}
        attributes = {attribute.name: attribute for attribute in module.attributes}
        for type_group in public_type_groups:
            generated_methods = _type_group_methods(type_group, stub_signature_overrides)
            api_class = classes.get(
                type_group.class_symbol,
                ApiClass(
                    module_name=module_name,
                    name=type_group.class_symbol,
                    bases=type_group.base_symbols,
                    origin=ApiOrigin.GENERATED,
                    location=(generated_methods[0].location if generated_methods else None),
                ),
            )
            classes[type_group.class_symbol] = replace(
                api_class,
                bases=api_class.bases or type_group.base_symbols,
                methods=_merge_generated_methods(api_class.methods, generated_methods),
            )
            if type_group.variable_symbol:
                attributes.setdefault(
                    type_group.variable_symbol,
                    ApiAttribute(
                        name=type_group.variable_symbol,
                        annotation=type_group.class_symbol,
                        origin=ApiOrigin.GENERATED,
                        location=(generated_methods[0].location if generated_methods else None),
                    ),
                )
        modules[module_name] = replace(
            module,
            classes=tuple(classes[name] for name in sorted(classes)),
            attributes=tuple(attributes[name] for name in sorted(attributes)),
        )

    return ApiModel(modules=tuple(modules[name] for name in sorted(modules)))
