# pyright: strict

"""Normalize source-side type spellings before they enter ``PythonApiModel``."""

from __future__ import annotations

import ast
from dataclasses import replace
from typing import cast

from .signatures import CallableSignature

SOURCE_TYPE_ALIASES = {
    "AxisPy": "FreeCAD.Base.Axis",
    "MatrixPy": "FreeCAD.Base.Matrix",
    "RotationPy": "FreeCAD.Base.Rotation",
    "UnitPy": "FreeCAD.Base.Unit",
}


def _dotted_name(node: ast.AST) -> str | None:
    if isinstance(node, ast.Name):
        return node.id
    if isinstance(node, ast.Attribute):
        parent = _dotted_name(node.value)
        return f"{parent}.{node.attr}" if parent else None
    return None


def _parse_expression(text: str) -> ast.expr | None:
    try:
        node = ast.parse(text, mode="eval").body
    except SyntaxError:
        return None
    return node


class _AnnotationNormalizer(ast.NodeTransformer):
    def __init__(self, module_name: str | None) -> None:
        self.module_name = module_name
        self._string_is_annotation = True

    def normalize(self, node: ast.expr) -> ast.expr:
        return cast(ast.expr, self._visit(node, string_is_annotation=True))

    def _visit(self, node: ast.AST, *, string_is_annotation: bool) -> ast.AST:
        previous = self._string_is_annotation
        self._string_is_annotation = string_is_annotation
        try:
            return cast(ast.AST, self.visit(node))
        finally:
            self._string_is_annotation = previous

    def visit_Constant(self, node: ast.Constant) -> ast.AST:
        if not self._string_is_annotation or not isinstance(node.value, str):
            return node
        parsed = _parse_expression(node.value)
        if parsed is None:
            return node
        normalized = self._visit(parsed, string_is_annotation=True)
        if isinstance(normalized, ast.Constant) and isinstance(normalized.value, str):
            value = normalized.value
        else:
            value = ast.unparse(normalized)
        return ast.copy_location(ast.Constant(value=value), node)

    def visit_Name(self, node: ast.Name) -> ast.AST:
        if not self._string_is_annotation:
            return node
        public_name = SOURCE_TYPE_ALIASES.get(node.id)
        if public_name is None:
            return node
        return ast.copy_location(ast.Constant(value=public_name), node)

    def visit_Attribute(self, node: ast.Attribute) -> ast.AST:
        dotted = _dotted_name(node)
        if (
            self._string_is_annotation
            and self.module_name
            and dotted
            and dotted.startswith(f"{self.module_name}.")
        ):
            remainder = dotted.removeprefix(f"{self.module_name}.")
            parsed = _parse_expression(remainder)
            if parsed is not None:
                return self._visit(parsed, string_is_annotation=self._string_is_annotation)
        return self.generic_visit(node)

    def visit_Subscript(self, node: ast.Subscript) -> ast.AST:
        node.value = cast(ast.expr, self._visit(node.value, string_is_annotation=False))
        base_name = _dotted_name(node.value)
        if base_name and base_name.rsplit(".", 1)[-1] == "Literal":
            node.slice = cast(ast.expr, self._visit(node.slice, string_is_annotation=False))
        elif base_name and base_name.rsplit(".", 1)[-1] == "Annotated":
            elements = node.slice.elts if isinstance(node.slice, ast.Tuple) else (node.slice,)
            normalized_elements = [
                self._visit(element, string_is_annotation=index == 0)
                for index, element in enumerate(elements)
            ]
            node.slice = (
                ast.Tuple(
                    elts=[cast(ast.expr, element) for element in normalized_elements],
                    ctx=ast.Load(),
                )
                if isinstance(node.slice, ast.Tuple)
                else cast(ast.expr, normalized_elements[0])
            )
        else:
            node.slice = cast(
                ast.expr,
                self._visit(node.slice, string_is_annotation=self._string_is_annotation),
            )
        return node


def normalize_source_type(text: str | None, module_name: str | None = None) -> str | None:
    """Map source-local aliases and self-qualified names to public spellings."""

    if text is None:
        return None
    expression = _parse_expression(text)
    if expression is None:
        return text
    normalized = _AnnotationNormalizer(module_name).normalize(expression)
    return ast.unparse(normalized)


def normalize_signature_types(
    signature: CallableSignature,
    module_name: str | None,
) -> CallableSignature:
    """Normalize annotations on one structured callable signature."""

    return replace(
        signature,
        parameters=tuple(
            replace(
                parameter,
                annotation=normalize_source_type(parameter.annotation, module_name),
            )
            for parameter in signature.parameters
        ),
        return_annotation=normalize_source_type(signature.return_annotation, module_name),
    )
