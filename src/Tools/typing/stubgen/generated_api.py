# pyright: strict

"""Project derived FreeCAD metadata into the normalized public API model."""

from __future__ import annotations

import ast
from collections import defaultdict
from dataclasses import replace
from pathlib import Path
import re

from python_api_model.model import (
    ApiCallableGroup,
    ApiClass,
    ApiModule,
    ApiOrigin,
    ApiSourceLocation,
    PythonApiModel,
)
from python_api_model.signatures import (
    ArgumentKind,
    CallableDecoratorFlags,
    CallableSignature,
    SignatureParameter,
)

from .api_extract import module_from_source
from .cpp_properties import TypedCppProperty
from .init_exports import ModuleExport, render_init_exports
from .model import PythonObjectType
from .module_merge import module_support_source
from .property_contracts import PropertyCatalog, render_property_aliases


def _parameter(
    name: str,
    annotation: str | None = None,
    default: str | None = None,
) -> SignatureParameter:
    return SignatureParameter(name, annotation, ArgumentKind.POSITIONAL_OR_KEYWORD, default)


def _module_from_fragment(root: Path, module_name: str, source: str, source_path: str) -> ApiModule:
    return module_from_source(
        root,
        root / source_path,
        source,
        module_name,
        origin=ApiOrigin.GENERATED,
        include_module_doc=False,
    )


def _import_fragment(source: str) -> str:
    imports: list[ast.stmt] = [
        node for node in ast.parse(source).body if isinstance(node, (ast.Import, ast.ImportFrom))
    ]
    if not imports:
        return ""
    return ast.unparse(ast.Module(body=imports, type_ignores=[])).rstrip() + "\n"


def generated_constant_model(
    root: Path,
    catalog: PropertyCatalog,
    exports: tuple[ModuleExport, ...],
) -> tuple[PythonApiModel, tuple[tuple[str, str], ...]]:
    """Return catalog aliases and bootstrap exports as public model declarations."""

    modules: list[ApiModule] = []
    support: list[tuple[str, str]] = []
    alias_source = render_property_aliases(catalog)
    modules.append(
        _module_from_fragment(root, "FreeCAD", alias_source, "src/App/PropertyPythonContracts.pyi")
    )
    support.append(("FreeCAD", module_support_source(alias_source)))

    by_module: dict[str, list[ModuleExport]] = defaultdict(list)
    for export in exports:
        by_module[export.module].append(export)
    for module_name, module_exports in sorted(by_module.items()):
        source = render_init_exports(tuple(module_exports))
        modules.append(_module_from_fragment(root, module_name, source, "src/App/FreeCADInit.py"))
        support.append((module_name, _import_fragment(source)))
    return PythonApiModel(tuple(modules)), tuple(support)


def _localize(expression: str, module_name: str) -> str:
    return re.sub(rf"\b{re.escape(module_name)}\.", "", expression)


def _property_group(property_: TypedCppProperty) -> ApiCallableGroup:
    module_name = property_.owner.module_name
    getter = _localize(property_.getter, module_name)
    setter = _localize(property_.setter, module_name) if property_.setter is not None else None
    location = ApiSourceLocation(property_.source, property_.line)
    signatures = [
        CallableSignature(
            property_.name,
            (_parameter("self"),),
            getter,
            property_.documentation,
            CallableDecoratorFlags(property_getter=True),
        )
    ]
    if setter is not None:
        signatures.append(
            CallableSignature(
                property_.name,
                (_parameter("self"), _parameter("value", setter)),
                "None",
                None,
                CallableDecoratorFlags(property_setter=True),
            )
        )
    return ApiCallableGroup(
        property_.name,
        tuple(signatures),
        doc=property_.documentation,
        origin=ApiOrigin.GENERATED,
        location=location,
    )


def add_cpp_properties_to_model(
    model: PythonApiModel,
    properties: tuple[TypedCppProperty, ...],
) -> tuple[PythonApiModel, tuple[tuple[str, str], ...]]:
    """Add dynamic C++ properties as semantic property accessors."""

    by_owner: dict[tuple[str, str], list[TypedCppProperty]] = defaultdict(list)
    for property_ in properties:
        by_owner[(property_.owner.module_name, property_.owner.python_name)].append(property_)

    modules: list[ApiModule] = []
    support: list[tuple[str, str]] = []
    for module in model.modules:
        classes: list[ApiClass] = []
        module_expressions: list[str] = []
        for klass in module.classes:
            additions = by_owner.pop((module.name, klass.name), [])
            if not additions:
                classes.append(klass)
                continue
            existing_methods = {method.name for method in klass.methods}
            attributes = {attribute.name: attribute for attribute in klass.attributes}
            placeholders = {
                attribute.name
                for attribute in attributes.values()
                if attribute.annotation in {"Any", "typing.Any"}
                and attribute.value in {None, "..."}
            }
            collisions = sorted(
                property_.name
                for property_ in additions
                if property_.name in existing_methods
                or property_.name in attributes
                and property_.name not in placeholders
            )
            if collisions:
                raise ValueError(
                    f"Generated {klass.qualified_name} already declares C++ properties: "
                    + ", ".join(collisions)
                )
            generated = tuple(
                _property_group(
                    replace(item, documentation=item.documentation or attributes[item.name].doc)
                    if item.name in placeholders
                    else item
                )
                for item in sorted(additions, key=lambda item: item.name)
            )
            generated_names = {item.name for item in additions}
            classes.append(
                replace(
                    klass,
                    attributes=tuple(
                        attribute
                        for attribute in klass.attributes
                        if attribute.name not in generated_names
                    ),
                    methods=tuple(
                        method for method in klass.methods if method.name not in generated_names
                    )
                    + generated,
                )
            )
            module_expressions.extend(
                expression for item in additions for expression in (item.getter, item.setter or "")
            )
        modules.append(replace(module, classes=tuple(classes)))

        imports: list[str] = []
        if any(re.search(r"\bSequence\b", expression) for expression in module_expressions):
            imports.append("from collections.abc import Sequence")
        dependencies = {
            match.group(1)
            for expression in module_expressions
            for match in re.finditer(r"\b([A-Za-z_]\w*)\.[A-Za-z_]\w*", expression)
            if match.group(1) != module.name
        }
        if "Base" in dependencies:
            dependencies.remove("Base")
            imports.append(
                "from . import Base as Base"
                if module.name == "FreeCAD"
                else "from FreeCAD import Base as Base"
            )
        imports.extend(
            f"import {dependency} as {dependency}" for dependency in sorted(dependencies)
        )
        if imports:
            support.append((module.name, "\n".join(imports) + "\n"))

    if by_owner:
        missing = ", ".join(f"{module}.{name}" for module, name in sorted(by_owner))
        raise ValueError(f"Generated model has no classes for C++ properties: {missing}")
    return PythonApiModel(tuple(modules)), tuple(support)


def _module_alias(module_name: str) -> str:
    return "_" + re.sub(r"[^A-Za-z0-9_]", "_", module_name)


def add_document_overloads_to_model(
    model: PythonApiModel,
    registrations: tuple[PythonObjectType, ...],
) -> tuple[PythonApiModel, tuple[tuple[str, str], ...]]:
    """Prepend TypeId-specific overloads to ``FreeCAD.Document.addObject``."""

    grouped: dict[tuple[str, str], list[str]] = defaultdict(list)
    for registration in registrations:
        grouped[(registration.python_module, registration.python_name)].append(registration.type_id)

    generated: list[CallableSignature] = []
    for (module_name, python_name), type_ids in sorted(grouped.items()):
        return_type = (
            python_name
            if module_name == "FreeCAD"
            else f"{_module_alias(module_name)}.{python_name}"
        )
        literals = ", ".join(repr(type_id) for type_id in sorted(type_ids))
        generated.append(
            CallableSignature(
                "addObject",
                (
                    _parameter("self"),
                    _parameter("type", f"Literal[{literals}]"),
                    _parameter("name", "str", "..."),
                    _parameter("objProxy", "object | None", "None"),
                    _parameter("viewProxy", "object | None", "None"),
                    _parameter("attach", "bool", "False"),
                    _parameter("viewType", "str", "..."),
                ),
                return_type,
                None,
                CallableDecoratorFlags(overload=True),
            )
        )

    found = False
    modules: list[ApiModule] = []
    for module in model.modules:
        if module.name != "FreeCAD":
            modules.append(module)
            continue
        classes: list[ApiClass] = []
        for klass in module.classes:
            if klass.name != "Document":
                classes.append(klass)
                continue
            methods: list[ApiCallableGroup] = []
            for method in klass.methods:
                if method.name != "addObject":
                    methods.append(method)
                    continue
                fallback = tuple(
                    replace(signature, flags=replace(signature.flags, overload=True))
                    for signature in method.signatures
                )
                methods.append(replace(method, signatures=tuple(generated) + fallback))
                found = True
            classes.append(replace(klass, methods=tuple(methods)))
        modules.append(replace(module, classes=tuple(classes)))
    if not found:
        raise ValueError("Generated FreeCAD.Document has no addObject method")

    imported_modules = sorted(
        {item.python_module for item in registrations if item.python_module != "FreeCAD"}
    )
    imports = ["from typing import Literal, overload"]
    imports.extend(f"import {name} as {_module_alias(name)}" for name in imported_modules)
    return PythonApiModel(tuple(modules)), (("FreeCAD", "\n".join(imports) + "\n"),)
