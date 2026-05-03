# pyright: strict

"""Shared Python-signature parsing helpers for curated stub inputs.

This module extracts neutral callable-signature data from Python AST nodes.
It is intended to be shared by:

- the stub-generation pipeline
- future API documentation extraction
- any legacy binding-generator adapters that still need Python-side signatures

The goal is to keep Python signature semantics in one place instead of
duplicating argument classification, overload grouping, docstring capture, and
decorator-flag handling across multiple pipelines.
"""

from __future__ import annotations

import ast
import copy
from dataclasses import dataclass
from enum import Enum
import re

SELF_CLS_ARG = re.compile(r"\(\s*(self|cls)(\s*,\s*)?")


class ArgumentKind(str, Enum):
    """Structured Python parameter kinds in declaration order."""

    POSITION_ONLY = "position_only"
    POSITIONAL_OR_KEYWORD = "positional_or_keyword"
    VAR_POSITIONAL = "var_positional"
    KEYWORD_ONLY = "keyword_only"
    VAR_KEYWORD = "var_keyword"


@dataclass(frozen=True)
class CallableDecoratorFlags:
    """Decorator-derived signature flags that affect public callable behavior."""

    classmethod: bool = False
    staticmethod: bool = False
    overload: bool = False
    typing_only: bool = False


@dataclass(frozen=True)
class SignatureParameter:
    """One parsed callable parameter."""

    name: str
    annotation: str | None
    kind: ArgumentKind
    default: str | None = None


@dataclass(frozen=True)
class CallableSignature:
    """Neutral signature data for one function or method declaration."""

    name: str
    parameters: tuple[SignatureParameter, ...]
    return_annotation: str | None
    docstring: str | None
    flags: CallableDecoratorFlags
    display_signature: str
    runtime_signature: str
    has_keywords: bool


def annotation_text(node: ast.AST | None) -> str | None:
    if node is None:
        return None
    return ast.unparse(node)


def default_text(node: ast.AST | None) -> str | None:
    if node is None:
        return None
    return ast.unparse(node)


def callable_decorator_flags(
    node: ast.FunctionDef | ast.AsyncFunctionDef,
) -> CallableDecoratorFlags:
    classmethod = False
    staticmethod = False
    overload = False
    typing_only = False

    for decorator in node.decorator_list:
        name: str | None = None
        match decorator:
            case ast.Name(id=identifier):
                name = identifier
            case ast.Attribute(attr=attribute):
                name = attribute
            case _:
                continue

        if name == "classmethod":
            classmethod = True
        elif name == "staticmethod":
            staticmethod = True
        elif name == "overload":
            overload = True
        elif name == "typing_only":
            typing_only = True

    return CallableDecoratorFlags(
        classmethod=classmethod,
        staticmethod=staticmethod,
        overload=overload,
        typing_only=typing_only,
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


def has_keyword_arguments(args: ast.arguments) -> bool:
    if args.args:
        first_arg = args.args[0].arg
        if first_arg in {"self", "cls"}:
            instance_args = len(args.args) > 1
        else:
            instance_args = True
    else:
        instance_args = False
    return bool(instance_args or args.kwonlyargs or args.kwarg)


def display_signature(
    node: ast.FunctionDef | ast.AsyncFunctionDef,
    return_annotation: str | None,
) -> str:
    parameters = ast.unparse(copy.deepcopy(node.args))
    returns = return_annotation or "object"
    return SELF_CLS_ARG.sub("(", f"{node.name}({parameters}) -> {returns}", 1)


def runtime_signature(node: ast.FunctionDef | ast.AsyncFunctionDef) -> str:
    args_copy = copy.deepcopy(node.args)
    all_args = [
        *args_copy.posonlyargs,
        *args_copy.args,
        args_copy.vararg,
        *args_copy.kwonlyargs,
        args_copy.kwarg,
    ]
    for argument in all_args:
        if argument is not None:
            argument.annotation = None
    return SELF_CLS_ARG.sub(r"($\1\2", f"{node.name}({ast.unparse(args_copy)})", 1)


def parse_callable_signature(node: ast.FunctionDef | ast.AsyncFunctionDef) -> CallableSignature:
    """Parse one function-like AST node into neutral signature data."""

    return_annotation = annotation_text(node.returns)
    docstring = ast.get_docstring(node, clean=True)
    return CallableSignature(
        name=node.name,
        parameters=signature_parameters(node),
        return_annotation=return_annotation,
        docstring=docstring,
        flags=callable_decorator_flags(node),
        display_signature=display_signature(node, return_annotation),
        runtime_signature=runtime_signature(node),
        has_keywords=has_keyword_arguments(node.args),
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
