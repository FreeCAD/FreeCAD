# pyright: strict

"""Discover and render TypeId-based ``Document.addObject`` contracts.

The C++ TypeId registration graph is the source of truth for object
inheritance. Binding ``.pyi`` inputs provide the public Python name for the
registered wrapper at each useful point in that graph. This module joins the
two independent facts and augments only the generated public ``FreeCAD``
module; the source-adjacent ``App/Document.pyi`` remains Core-only.
"""

from __future__ import annotations

import ast
import re
from typing import Mapping, Sequence

from .model import BindingClass, PublicPythonType, PythonObjectType
from .module_merge import merge_module_support_nodes
from .type_hierarchy import TypeHierarchy


def _public_python_type(public_name: str) -> PublicPythonType | None:
    if "." not in public_name:
        return None
    module_name, python_name = public_name.rsplit(".", 1)
    if not module_name or not python_name or not python_name.isidentifier():
        return None
    return PublicPythonType(module_name, python_name)


def direct_python_types(
    classes: Sequence[BindingClass],
    hierarchy: TypeHierarchy | None = None,
    type_ids: set[str] | None = None,
) -> dict[str, PublicPythonType]:
    """Normalize existing binding-class public-name discovery for TypeIds."""

    result: dict[str, PublicPythonType] = {}
    for klass in classes:
        targets = [
            target
            for public_name in klass.public_names
            if (target := _public_python_type(public_name)) is not None
        ]
        for cpp_name in klass.cpp_type_names:
            if type_ids is not None and cpp_name not in type_ids:
                continue
            if hierarchy is not None and not hierarchy.is_derived_from(
                cpp_name,
                "App::DocumentObject",
            ):
                continue
            for target in targets:
                previous = result.get(cpp_name)
                if previous is not None and previous != target:
                    raise ValueError(
                        f"Conflicting public Python types for {cpp_name}: "
                        f"{previous.qualified_name} and {target.qualified_name}"
                    )
                result[cpp_name] = target
                break
    return result


def resolve_document_object_python_type(
    type_id: str,
    hierarchy: TypeHierarchy,
    direct_python_types: Mapping[str, PublicPythonType],
) -> PublicPythonType | None:
    """Resolve a TypeId through its ancestors to a public Python wrapper."""

    if not hierarchy.is_derived_from(type_id, "App::DocumentObject"):
        return None
    for candidate in hierarchy.chain(type_id):
        python_type = direct_python_types.get(candidate)
        if python_type is not None:
            return python_type
    return None


def document_object_python_types(
    hierarchy: TypeHierarchy,
    direct_python_types: Mapping[str, PublicPythonType],
) -> tuple[PythonObjectType, ...]:
    """Return resolved public Python types for known document-object TypeIds."""

    registrations: list[PythonObjectType] = []
    for type_id in sorted(hierarchy.nodes):
        if hierarchy.nodes[type_id].is_abstract:
            continue
        python_type = resolve_document_object_python_type(
            type_id,
            hierarchy,
            direct_python_types,
        )
        if python_type is None:
            continue
        registrations.append(
            PythonObjectType(type_id, python_type.module_name, python_type.python_name)
        )
    return tuple(registrations)


def _module_alias(module_name: str) -> str:
    return "_" + re.sub(r"[^A-Za-z0-9_]", "_", module_name)


def _type_expression(python_type: PublicPythonType) -> str:
    if python_type.module_name == "FreeCAD":
        return python_type.python_name
    return f"{_module_alias(python_type.module_name)}.{python_type.python_name}"


def _overload_source(
    python_type: PublicPythonType,
    type_ids: Sequence[str],
) -> str:
    literals = ", ".join(repr(type_id) for type_id in type_ids)
    return f"""@overload
def addObject(
    self,
    type: Literal[{literals}],
    name: str = ...,
    objProxy: object | None = None,
    viewProxy: object | None = None,
    attach: bool = False,
    viewType: str = ...,
) -> {_type_expression(python_type)}: ...
"""


def add_document_add_object_overloads(
    target_source: str,
    registrations: Sequence[PythonObjectType],
) -> str:
    """Insert grouped literal overloads before generic ``Document.addObject``."""

    if not registrations:
        return target_source
    tree = ast.parse(target_source)
    document = next(
        (node for node in tree.body if isinstance(node, ast.ClassDef) and node.name == "Document"),
        None,
    )
    if document is None:
        raise ValueError("Generated FreeCAD module has no Document class")
    add_object_nodes = [
        node
        for node in document.body
        if isinstance(node, (ast.FunctionDef, ast.AsyncFunctionDef)) and node.name == "addObject"
    ]
    if not add_object_nodes:
        raise ValueError("Generated FreeCAD.Document has no addObject method")

    # The C++ registration pass and the source-adjacent Document stub can both
    # contribute the generic declaration. Keep the last one, which carries the
    # curated documentation, so the generated overload set has one fallback.
    fallback = add_object_nodes[-1]
    document.body = [
        node
        for node in document.body
        if not (
            isinstance(node, (ast.FunctionDef, ast.AsyncFunctionDef))
            and node.name == "addObject"
            and node is not fallback
        )
    ]
    add_object_index = document.body.index(fallback)
    # In a ``.pyi`` an overload implementation is not used for overload
    # resolution. Mark the generic fallback as the final overload so dynamic
    # TypeIds continue to return ``DocumentObject``.
    if not any(
        isinstance(decorator, ast.Name) and decorator.id == "overload"
        for decorator in fallback.decorator_list
    ):
        fallback.decorator_list.append(ast.Name(id="overload", ctx=ast.Load()))

    grouped: dict[tuple[str, str], list[str]] = {}
    for registration in registrations:
        key = (registration.python_module, registration.python_name)
        grouped.setdefault(key, []).append(registration.type_id)

    overloads: list[ast.stmt] = []
    for (module_name, python_name), type_ids in sorted(grouped.items()):
        overloads.extend(
            ast.parse(
                _overload_source(
                    PublicPythonType(module_name, python_name),
                    tuple(sorted(type_ids)),
                )
            ).body
        )
    document.body[add_object_index:add_object_index] = overloads
    ast.fix_missing_locations(tree)
    merged = ast.unparse(tree).rstrip() + "\n"

    import_lines = ["from typing import Literal, overload"]
    modules = sorted(
        {
            registration.python_module
            for registration in registrations
            if registration.python_module != "FreeCAD"
        }
    )
    import_lines.extend(f"import {module} as {_module_alias(module)}" for module in modules)
    return merge_module_support_nodes(merged, "\n".join(import_lines) + "\n")
