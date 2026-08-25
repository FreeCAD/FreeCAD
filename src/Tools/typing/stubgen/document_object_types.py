# pyright: strict

"""Discover TypeId-based ``Document.addObject`` contracts.

The C++ TypeId registration graph is the source of truth for object
inheritance. Binding ``.pyi`` inputs provide the public Python name for the
registered wrapper at each useful point in that graph. This module joins the
two independent facts and augments only the generated public ``FreeCAD``
module; the source-adjacent ``App/Document.pyi`` remains Core-only.
"""

from __future__ import annotations

from typing import Mapping, Sequence

from .model import BindingClass, PublicPythonType, PythonObjectType
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
