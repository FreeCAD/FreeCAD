# SPDX-License-Identifier: LGPL-2.1-or-later

"""Extract structured exports from the Python-defined FreeCAD bootstrap API."""

from __future__ import annotations

import ast
import copy
from dataclasses import dataclass
from pathlib import Path
import re

from .class_merge import keep_public_stub_decorator

INIT_BOOTSTRAP_PATH = Path("src/App/FreeCADInit.py")
INIT_LOGGING_STUB_PATH = Path("src/App/FreeCADInit.pyi")

_ASSIGNMENT_EXPORT_SOURCES = (
    (INIT_BOOTSTRAP_PATH, "App", "FreeCAD"),
    (INIT_BOOTSTRAP_PATH, "units", "FreeCAD.Units"),
)


@dataclass(frozen=True)
class ModuleExport:
    """One public name installed by a Python bootstrap helper."""

    module: str
    name: str
    type_expression: str
    doc: str = ""
    class_definition: str = ""


def _constant_string(node: ast.expr, *, path: Path, line: int) -> str:
    if isinstance(node, ast.Constant) and isinstance(node.value, str):
        return node.value
    raise ValueError(f"{path}:{line}: expected a string export name")


def _humanize(name: str) -> str:
    name = re.sub(r"(?<=[a-z0-9])(?=[A-Z])", " ", name.replace("_", " "))
    name = re.sub(r"(?<=[A-Z])(?=[A-Z][a-z])", " ", name)
    return name.lower()


def _default_declaration_doc(
    name: str,
    constructor_name: str,
    declaration: ast.Call,
) -> str:
    if constructor_name == "QuantityConstant":
        expression = declaration.args[0] if declaration.args else None
        if isinstance(expression, ast.Constant) and isinstance(expression.value, str):
            display = {
                "<angular-minute>": "1 angular minute",
                "<angular-second>": "1 angular second",
            }.get(expression.value, f"1 {expression.value}")
            return f"Predefined quantity representing {display}."
        return f"Predefined quantity constant {name}."

    return f"Predefined dimensional unit for {_humanize(name)}."


def _declaration_entries(
    tree: ast.Module,
    table_name: str,
    constructor_name: str,
    type_expression: str,
    path: Path,
) -> tuple[ModuleExport, ...]:
    table: ast.expr | None = None
    for node in tree.body:
        if isinstance(node, ast.AnnAssign) and isinstance(node.target, ast.Name):
            if node.target.id == table_name:
                table = node.value
                break
    if not isinstance(table, (ast.Tuple, ast.List)):
        raise ValueError(f"{path}: {table_name} must be a literal tuple or list")

    exports: list[ModuleExport] = []
    for entry in table.elts:
        if not isinstance(entry, ast.Tuple) or len(entry.elts) != 2:
            raise ValueError(f"{path}: {table_name} entries must be (name, declaration) pairs")
        name = _constant_string(entry.elts[0], path=path, line=entry.lineno)
        declaration = entry.elts[1]
        if not isinstance(declaration, ast.Call) or not isinstance(declaration.func, ast.Name):
            raise ValueError(f"{path}:{entry.lineno}: invalid {table_name} declaration")
        if declaration.func.id != constructor_name:
            raise ValueError(
                f"{path}:{entry.lineno}: {table_name} uses {declaration.func.id!r}; "
                f"expected {constructor_name!r}"
            )
        doc = ""
        for keyword in declaration.keywords:
            if keyword.arg == "doc":
                doc = _constant_string(keyword.value, path=path, line=entry.lineno)
        if not doc:
            doc = _default_declaration_doc(name, constructor_name, declaration)
        exports.append(ModuleExport("FreeCAD.Units", name, type_expression, doc))
    return tuple(exports)


def _class_definitions(tree: ast.Module) -> dict[str, ast.ClassDef]:
    return {node.name: node for node in tree.body if isinstance(node, ast.ClassDef)}


def _assignment_targets(node: ast.stmt) -> tuple[ast.expr, ...]:
    if isinstance(node, ast.Assign):
        return tuple(node.targets)
    if isinstance(node, ast.AnnAssign):
        return (node.target,)
    return ()


def _is_int_enum(node: ast.ClassDef) -> bool:
    return any(ast.unparse(base).rsplit(".", 1)[-1] == "IntEnum" for base in node.bases)


def _render_enum_class(node: ast.ClassDef) -> str:
    bases = ", ".join(ast.unparse(base) for base in node.bases)
    lines = [f"class {node.name}({bases}):"]
    body = list(node.body)
    if body and isinstance(body[0], ast.Expr) and isinstance(body[0].value, ast.Constant):
        if isinstance(body[0].value.value, str):
            lines.append(f"    {ast.unparse(body[0].value)}")
        body = body[1:]

    for member in body:
        if isinstance(member, ast.Assign) and len(member.targets) == 1:
            target = member.targets[0]
            if isinstance(target, ast.Name):
                lines.append(f"    {target.id}: Final[int] = {ast.unparse(member.value)}")
        elif isinstance(member, ast.AnnAssign) and isinstance(member.target, ast.Name):
            lines.append(f"    {ast.unparse(member)}")

    if len(lines) == 1:
        lines.append("    pass")
    return "\n".join(lines)


def _render_function_stub(node: ast.FunctionDef | ast.AsyncFunctionDef) -> str:
    stub = copy.deepcopy(node)
    stub.decorator_list = [
        decorator for decorator in stub.decorator_list if keep_public_stub_decorator(decorator)
    ]
    docstring = None
    if (
        stub.body
        and isinstance(stub.body[0], ast.Expr)
        and isinstance(stub.body[0].value, ast.Constant)
        and isinstance(stub.body[0].value.value, str)
    ):
        docstring = stub.body[0]
    stub.body = ([docstring] if docstring is not None else []) + [
        ast.Expr(value=ast.Constant(value=Ellipsis))
    ]
    return ast.unparse(stub)


def _is_public_runtime_method(node: ast.FunctionDef | ast.AsyncFunctionDef) -> bool:
    """Return whether a runtime method belongs in the public class stub."""
    return node.name == "__init__" or not node.name.startswith("_")


def _render_runtime_class(
    node: ast.ClassDef,
    supplemental: ast.ClassDef | None = None,
) -> str:
    bases = f"({', '.join(ast.unparse(base) for base in node.bases)})" if node.bases else ""
    lines = [f"class {node.name}{bases}:"]
    body = list(node.body)
    if body and isinstance(body[0], ast.Expr) and isinstance(body[0].value, ast.Constant):
        if isinstance(body[0].value.value, str):
            lines.append(f"    {ast.unparse(body[0].value)}")
        body = body[1:]

    emitted_methods: set[str] = set()
    for member in body:
        if isinstance(member, ast.AnnAssign):
            lines.append(f"    {ast.unparse(member)}")
        elif isinstance(
            member, (ast.FunctionDef, ast.AsyncFunctionDef)
        ) and _is_public_runtime_method(member):
            lines.append("")
            lines.extend(f"    {line}" for line in _render_function_stub(member).splitlines())
            emitted_methods.add(member.name)

    if supplemental is not None:
        for member in supplemental.body:
            if (
                isinstance(member, (ast.FunctionDef, ast.AsyncFunctionDef))
                and member.name not in emitted_methods
            ):
                lines.append("")
                lines.extend(f"    {line}" for line in _render_function_stub(member).splitlines())
                emitted_methods.add(member.name)

    if len(lines) == 1:
        lines.append("    pass")
    return "\n".join(lines)


def _render_class(node: ast.ClassDef, supplemental: ast.ClassDef | None = None) -> str:
    if _is_int_enum(node):
        return _render_enum_class(node)
    return _render_runtime_class(node, supplemental)


def _assigned_class_exports(
    tree: ast.Module,
    path: Path,
    receiver_name: str,
    module: str,
    supplemental_tree: ast.Module | None = None,
) -> tuple[ModuleExport, ...]:
    """Discover ``app.Name = ClassName`` style bootstrap exports."""

    classes = _class_definitions(tree)
    supplemental_classes = (
        _class_definitions(supplemental_tree) if supplemental_tree is not None else {}
    )
    exports: list[ModuleExport] = []
    seen: set[str] = set()
    for node in ast.walk(tree):
        if not isinstance(node, (ast.Assign, ast.AnnAssign)):
            continue
        value = node.value
        if not isinstance(value, ast.Name):
            continue
        for target in _assignment_targets(node):
            if not (
                isinstance(target, ast.Attribute)
                and isinstance(target.value, ast.Name)
                and target.value.id == receiver_name
            ):
                continue
            if target.attr.startswith("_"):
                continue
            if value.id not in classes:
                raise ValueError(
                    f"{path}:{node.lineno}: bootstrap export {receiver_name}.{target.attr} "
                    f"references missing class {value.id!r}"
                )
            if target.attr in seen:
                continue
            seen.add(target.attr)
            class_definition = _render_class(
                classes[value.id],
                supplemental_classes.get(value.id),
            )
            exports.append(
                ModuleExport(
                    module,
                    target.attr,
                    f"type[{value.id}]",
                    (
                        "Tagged logger class installed as FreeCAD.Logger."
                        if value.id == "FCADLogger"
                        else ast.get_docstring(classes[value.id])
                        or f"Python-defined {value.id} enum exported as {module}.{target.attr}."
                    ),
                    class_definition,
                )
            )
    return tuple(exports)


def _load_assignment_exports(
    root: Path,
    relative_path: Path,
    receiver_name: str,
    module: str,
    supplemental_relative_path: Path | None = None,
) -> tuple[ModuleExport, ...]:
    path = root / relative_path
    tree = ast.parse(path.read_text(encoding="utf-8"), filename=str(path))
    supplemental_tree = None
    if supplemental_relative_path is not None:
        supplemental_path = root / supplemental_relative_path
        supplemental_tree = ast.parse(
            supplemental_path.read_text(encoding="utf-8"),
            filename=str(supplemental_path),
        )
    return _assigned_class_exports(tree, path, receiver_name, module, supplemental_tree)


def _validate_exports(exports: tuple[ModuleExport, ...]) -> tuple[ModuleExport, ...]:
    seen: dict[tuple[str, str], ModuleExport] = {}
    for export in exports:
        key = (export.module, export.name)
        previous = seen.get(key)
        if previous is not None:
            if previous.type_expression != export.type_expression:
                raise ValueError(
                    f"Conflicting bootstrap export types for {export.module}.{export.name}: "
                    f"{previous.type_expression!r} and {export.type_expression!r}"
                )
            raise ValueError(f"Duplicate bootstrap export: {export.module}.{export.name}")
        seen[key] = export
    return exports


def load_init_exports(root: Path) -> tuple[ModuleExport, ...]:
    """Read structured exports from the monolithic bootstrap source."""
    path = root / INIT_BOOTSTRAP_PATH
    tree = ast.parse(path.read_text(encoding="utf-8"), filename=str(path))
    exports = _declaration_entries(
        tree,
        "QUANTITY_CONSTANTS",
        "QuantityConstant",
        "Quantity",
        path,
    ) + _declaration_entries(
        tree,
        "UNIT_CONSTANTS",
        "UnitConstant",
        "Unit",
        path,
    )
    for relative_path, receiver_name, module in _ASSIGNMENT_EXPORT_SOURCES:
        supplemental_path = INIT_LOGGING_STUB_PATH if module == "FreeCAD" else None
        exports += _load_assignment_exports(
            root,
            relative_path,
            receiver_name,
            module,
            supplemental_path,
        )
    return _validate_exports(exports)


def render_init_exports(exports: tuple[ModuleExport, ...]) -> str:
    """Render export records as a mergeable support fragment."""
    lines: list[str] = []
    if any(
        export.module == "FreeCAD.Units" and export.type_expression in {"Quantity", "Unit"}
        for export in exports
    ):
        lines.append("from FreeCAD.Base import Quantity, Unit")
        lines.append("")
    definitions: dict[str, str] = {}
    for export in exports:
        if not export.class_definition:
            continue
        previous = definitions.get(export.name)
        if previous is not None and previous != export.class_definition:
            raise ValueError(f"Conflicting bootstrap class definitions for {export.name}")
        definitions[export.name] = export.class_definition

    if any("IntEnum" in definition for definition in definitions.values()):
        lines.append("from enum import IntEnum")
    if any("Final[" in definition for definition in definitions.values()):
        lines.append("from typing import Final")
    if any("datetime" in definition for definition in definitions.values()):
        lines.append("from datetime import datetime")
    if any("Callable" in definition for definition in definitions.values()):
        lines.append("from collections.abc import Callable")
    if len(lines) > 0 and lines[-1] != "":
        lines.append("")
    for definition in definitions.values():
        lines.extend(definition.splitlines())
        lines.append("")
    if lines and lines[-1] == "":
        lines.pop()
    if definitions:
        lines.append("")
    for export in exports:
        lines.append(f"{export.name}: {export.type_expression}")
        if export.doc:
            lines.append(f'"""{export.doc}"""')
    return "\n".join(lines) + "\n"
