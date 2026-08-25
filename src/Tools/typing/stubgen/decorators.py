# pyright: strict

"""Normalize binding-only decorators before building the public API model.

The binding source files use decorators as extraction metadata. They are not
all Python API decorators, so this policy belongs at the StubGen input
boundary rather than in a renderer.
"""

from __future__ import annotations

import ast
from dataclasses import replace

from python_api_model.signatures import CallableSignature, decorator_name

INTERNAL_DECORATOR_NAMES = frozenset(
    {
        "bootstrap_export",
        "callback",
        "class_declarations",
        "constmethod",
        "deprecated_attributes",
        "export",
        "forward_declarations",
        "no_args",
        "sequence_protocol",
        "typing_only",
    }
)


def raw_decorator_name(decorator: str) -> str:
    """Return the leaf name from a raw decorator expression."""

    expression = decorator.removeprefix("@").strip()
    try:
        node = ast.parse(expression, mode="eval").body
    except SyntaxError:
        return expression.split("(", 1)[0].strip()
    return decorator_name(node) or ast.unparse(node)


def public_decorators(decorators: tuple[str, ...]) -> tuple[str, ...]:
    """Remove binding metadata decorators from a public declaration."""

    return tuple(
        decorator
        for decorator in decorators
        if raw_decorator_name(decorator) not in INTERNAL_DECORATOR_NAMES
    )


def strip_binding_decorators(signature: CallableSignature) -> CallableSignature:
    """Remove binding metadata decorators from one public signature."""

    return replace(signature, decorators=public_decorators(signature.decorators))
