# pyright: strict

"""Shared structured Python-signature parsing helpers.

This module extracts neutral callable-signature data from Python AST nodes.
It is intended to be shared by:

- curated stub extraction
- binding normalization
- output-specific stub and documentation renderers

The goal is to keep Python signature semantics in one place instead of
duplicating argument classification, overload grouping, docstring capture, and
decorator-flag handling across multiple pipelines.
"""

from __future__ import annotations

import ast
from dataclasses import dataclass, replace
from enum import Enum


class ArgumentKind(str, Enum):
    """Structured Python parameter kinds in declaration order."""

    POSITION_ONLY = "position_only"
    POSITIONAL_OR_KEYWORD = "positional_or_keyword"
    VAR_POSITIONAL = "var_positional"
    KEYWORD_ONLY = "keyword_only"
    VAR_KEYWORD = "var_keyword"


@dataclass(frozen=True)
class CallableDecoratorFlags:
    """Decorator-derived semantic flags for one public callable."""

    classmethod: bool = False
    staticmethod: bool = False
    overload: bool = False
    property_getter: bool = False
    property_setter: bool = False


@dataclass(frozen=True)
class SignatureParameter:
    """One parsed callable parameter."""

    name: str
    annotation: str | None
    kind: ArgumentKind
    default: str | None = None


@dataclass(frozen=True)
class CallableSignature:
    """Structured semantic data for one function or method declaration."""

    name: str
    parameters: tuple[SignatureParameter, ...]
    return_annotation: str | None
    docstring: str | None
    flags: CallableDecoratorFlags
    is_async: bool = False
    deprecated_message: str | None = None
    decorators: tuple[str, ...] = ()


def annotation_text(node: ast.AST | None) -> str | None:
    if node is None:
        return None
    return ast.unparse(node)


def default_text(node: ast.AST | None) -> str | None:
    if node is None:
        return None
    return ast.unparse(node)


def decorator_name(node: ast.expr) -> str | None:
    """Return the leaf name of a decorator expression."""

    if isinstance(node, ast.Call):
        node = node.func
    if isinstance(node, ast.Name):
        return node.id
    if isinstance(node, ast.Attribute):
        return node.attr
    return None


def callable_decorator_flags(
    node: ast.FunctionDef | ast.AsyncFunctionDef,
) -> CallableDecoratorFlags:
    classmethod = False
    staticmethod = False
    overload = False
    property_getter = False
    property_setter = False

    for decorator in node.decorator_list:
        name = decorator_name(decorator)
        if name is None:
            continue
        if name == "setter":
            property_setter = True

        if name == "classmethod":
            classmethod = True
        elif name == "staticmethod":
            staticmethod = True
        elif name == "overload":
            overload = True
        elif name == "property":
            property_getter = True

    return CallableDecoratorFlags(
        classmethod=classmethod,
        staticmethod=staticmethod,
        overload=overload,
        property_getter=property_getter,
        property_setter=property_setter,
    )


def positional_parameter_defaults(
    args: ast.arguments,
) -> tuple[list[ast.arg], list[ast.AST | None]]:
    positional = [*args.posonlyargs, *args.args]
    defaults: list[ast.AST | None] = [None] * (len(positional) - len(args.defaults))
    defaults.extend(args.defaults)
    return positional, defaults


def signature_parameters(
    node: ast.FunctionDef | ast.AsyncFunctionDef,
) -> tuple[SignatureParameter, ...]:
    args = node.args
    parameters: list[SignatureParameter] = []

    positional, positional_defaults = positional_parameter_defaults(args)
    posonly_count = len(args.posonlyargs)
    for index, (argument, default_node) in enumerate(zip(positional, positional_defaults)):
        kind = (
            ArgumentKind.POSITION_ONLY
            if index < posonly_count
            else ArgumentKind.POSITIONAL_OR_KEYWORD
        )
        parameters.append(
            SignatureParameter(
                name=argument.arg,
                annotation=annotation_text(argument.annotation),
                kind=kind,
                default=default_text(default_node),
            )
        )

    if args.vararg is not None:
        parameters.append(
            SignatureParameter(
                name=args.vararg.arg,
                annotation=annotation_text(args.vararg.annotation),
                kind=ArgumentKind.VAR_POSITIONAL,
            )
        )

    for argument, default_node in zip(args.kwonlyargs, args.kw_defaults):
        parameters.append(
            SignatureParameter(
                name=argument.arg,
                annotation=annotation_text(argument.annotation),
                kind=ArgumentKind.KEYWORD_ONLY,
                default=default_text(default_node),
            )
        )

    if args.kwarg is not None:
        parameters.append(
            SignatureParameter(
                name=args.kwarg.arg,
                annotation=annotation_text(args.kwarg.annotation),
                kind=ArgumentKind.VAR_KEYWORD,
            )
        )

    return tuple(parameters)


def parse_callable_signature(node: ast.FunctionDef | ast.AsyncFunctionDef) -> CallableSignature:
    """Parse one function-like AST node into neutral signature data."""

    return_annotation = annotation_text(node.returns)
    flags = callable_decorator_flags(node)
    deprecated_message: str | None = None
    for decorator in node.decorator_list:
        if not isinstance(decorator, ast.Call) or decorator_name(decorator) != "deprecated":
            continue
        if not decorator.args:
            continue
        message = decorator.args[0]
        if isinstance(message, ast.Constant) and isinstance(message.value, str):
            deprecated_message = message.value
    docstring = ast.get_docstring(node, clean=True)
    return CallableSignature(
        name=node.name,
        parameters=signature_parameters(node),
        return_annotation=return_annotation,
        docstring=docstring,
        flags=flags,
        is_async=isinstance(node, ast.AsyncFunctionDef),
        deprecated_message=deprecated_message,
        decorators=tuple(ast.unparse(decorator) for decorator in node.decorator_list),
    )


def parse_signature_parts(
    name: str,
    parameters: str,
    return_annotation: str,
    *,
    docstring: str | None = None,
    overload: bool = False,
    deprecated_message: str | None = None,
) -> CallableSignature:
    """Parse a signature assembled from structured input fields.

    This is used for curated signature overrides. Discovered bindings use the
    direct fallback constructor in the binding adapter and never pass
    through rendered stub text.
    """

    source = f"def {name}({parameters}) -> {return_annotation}: ...\n"
    node = ast.parse(source).body[0]
    if not isinstance(node, ast.FunctionDef):
        raise TypeError("signature source did not produce a function")
    signature = parse_callable_signature(node)
    return replace(
        signature,
        docstring=docstring,
        deprecated_message=deprecated_message,
        flags=replace(signature.flags, overload=overload),
    )


def group_callable_definitions(
    body: list[ast.stmt],
) -> dict[str, list[ast.FunctionDef | ast.AsyncFunctionDef]]:
    """Collect top-level or class-body functions grouped by declared name."""

    groups: dict[str, list[ast.FunctionDef | ast.AsyncFunctionDef]] = {}
    for node in body:
        if not isinstance(node, (ast.FunctionDef, ast.AsyncFunctionDef)):
            continue
        groups.setdefault(node.name, []).append(node)
    return groups


def parse_callable_group(
    group: list[ast.FunctionDef | ast.AsyncFunctionDef],
) -> tuple[CallableSignature, ...]:
    """Parse one overload group in source order."""

    return tuple(parse_callable_signature(node) for node in group)
