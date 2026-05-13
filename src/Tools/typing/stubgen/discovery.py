# pyright: strict

"""Binding discovery helpers for the stub generation pipeline.

This module is responsible for turning repository source files into a raw
binding inventory. It scans C++ registrations, PyCXX method tables, and module
creation code, then maps those findings onto the public FreeCAD Python module
tree as far as that can be done mechanically.

In the overall pipeline this is the first semantic stage:
- ``parsing`` provides syntax-oriented helpers for C++/CMake/Python source
- ``discovery`` extracts binding methods, type registrations, and context maps
- later modules turn that inventory into curated public stubs

Keep this file focused on discovery and low-level name inference. Rendering,
AST merging, and output layout belong elsewhere.
"""

from __future__ import annotations

from collections import defaultdict
from dataclasses import dataclass
from pathlib import Path
import re
from typing import Iterable

from .model import (
    ADD_METHOD_RE,
    BEHAVIOR_NAME_RE,
    BindingMethod,
    CPP_TYPE_NAME_RE,
    ContextEntry,
    ContextKind,
    EXTENSION_MODULE_RE,
    GETATTR_MODULE_RE,
    INIT_MODULE_RE,
    MethodKind,
    ModuleDef,
    PYMETHODDEF_RE,
    PYMETHOD_ALIAS_RE,
    PYIMPORT_ADD_MODULE_RE,
    PYMODULEDEF_RE,
    PYMODULE_ADD_FUNCTIONS_RE,
    PYMODULE_ADD_OBJECT_RE,
    PYMODULE_CREATE_RE,
    PYMODULE_IMPORT_RE,
    PY_OBJECT_WRAPPER_RE,
    PublicTypeGroup,
    PublicTypeTarget,
)
from .naming import valid_identifier
from .parsing import (
    add_type_calls,
    extract_balanced,
    first_string_literal,
    generated_source,
    iter_source_files,
    line_number,
    normalize_doc,
    normalize_expr,
    split_top_level,
    strip_comments,
)
from .type_context_rules import type_context_internal_reason, type_context_public_targets

KNOWN_PYMETHODDEF_MODULE_HINTS: dict[tuple[str, str], str] = {
    ("src/App/ApplicationPy.cpp", "ApplicationPy::Methods"): "FreeCAD",
    ("src/Base/Console.cpp", "ConsoleSingleton::Methods"): "FreeCAD.Console",
    ("src/Base/UnitsApiPy.cpp", "UnitsApi::Methods"): "FreeCAD.Units",
    ("src/Gui/ApplicationPy.cpp", "ApplicationPy::Methods"): "FreeCADGui",
    ("src/Gui/Selection/Selection.cpp", "SelectionSingleton::Methods"): "FreeCADGui.Selection",
    ("src/Main/FreeCADGuiPy.cpp", "FreeCADGui_methods"): "FreeCADGui",
    ("src/Gui/Application.cpp", "FreeCADGui_methods"): "FreeCADGui",
    (
        "src/Mod/Part/Gui/AttacherTexts.cpp",
        "AttacherGuiPy::Methods",
    ): "PartGui.AttachEngineResources",
}


KNOWN_PYCXX_MODULE_HINTS: dict[tuple[str, str], str] = {
    ("src/Base/Translate.cpp", "__Translate__"): "FreeCAD.Qt",
    ("src/Gui/UiLoader.cpp", "PySideUic"): "FreeCADGui.PySideUic",
    ("src/Mod/Part/App/AppPartPy.cpp", "ShapeFix"): "Part.ShapeFix",
}


@dataclass
class ModuleState:
    variables: dict[str, str]


def discover_contexts(source: str) -> list[ContextEntry]:
    contexts: list[ContextEntry] = []
    for match in BEHAVIOR_NAME_RE.finditer(source):
        contexts.append((match.start(), "python_type", match.group(1)))
    for match in EXTENSION_MODULE_RE.finditer(source):
        contexts.append((match.start(), "pycxx_module", match.group("python_name")))
    contexts.sort(key=lambda item: item[0])
    return contexts


def nearest_context(contexts: list[ContextEntry], position: int) -> tuple[ContextKind, str]:
    selected: tuple[ContextKind, str] = ("unknown", "unknown")
    for context_position, kind, name in contexts:
        if context_position > position:
            break
        selected = (kind, name)
    return selected


def normalize_table_reference(table: str | None, aliases: dict[str, str]) -> str | None:
    if table is None:
        return None
    normalized = normalize_expr(table)
    seen: set[str] = set()
    while normalized in aliases and normalized not in seen:
        seen.add(normalized)
        normalized = aliases[normalized]
    return normalized


def module_state_for_source(
    source: str,
    initial_variables: dict[str, str] | None = None,
) -> ModuleState:
    module_vars = dict(initial_variables or {})
    for match in PYMODULE_IMPORT_RE.finditer(source):
        module_vars[match.group("variable")] = match.group("module")
    for match in PYIMPORT_ADD_MODULE_RE.finditer(source):
        module_vars[match.group("variable")] = match.group("module")
    for match in INIT_MODULE_RE.finditer(source):
        module_vars[match.group("variable")] = match.group("namespace")
    wrappers: dict[str, str] = {}
    for match in PY_OBJECT_WRAPPER_RE.finditer(source):
        source_module = module_vars.get(match.group("source"))
        if source_module:
            wrappers[match.group("variable")] = source_module
    for match in GETATTR_MODULE_RE.finditer(source):
        owner_module = wrappers.get(match.group("owner")) or module_vars.get(match.group("owner"))
        if owner_module:
            module_vars[match.group("variable")] = f"{owner_module}.{match.group('name')}"
    for match in PYMODULE_ADD_OBJECT_RE.finditer(source):
        parent_module = module_vars.get(match.group("parent"))
        child_module = module_vars.get(match.group("child"))
        if parent_module and child_module:
            module_vars[match.group("child")] = f"{parent_module}.{match.group('name')}"
    return ModuleState(variables=module_vars)


def collect_module_definitions(
    root: Path, files: Iterable[Path]
) -> tuple[dict[tuple[str, str], ModuleDef], dict[str, str], dict[tuple[str, str], str]]:
    module_defs: dict[tuple[str, str], ModuleDef] = {}
    aliases: dict[str, str] = {}
    table_modules: dict[tuple[str, str], str] = {}

    file_data: dict[Path, str] = {}
    for path in files:
        try:
            file_data[path] = strip_comments(path.read_text(encoding="utf-8", errors="replace"))
        except OSError:
            continue

    for path, source in file_data.items():
        rel = path.relative_to(root).as_posix()
        for match in PYMETHOD_ALIAS_RE.finditer(source):
            table = normalize_expr(match.group("table"))
            if "::Methods" in table or table.endswith("_methods"):
                aliases[match.group("alias")] = table

        for match in PYMODULEDEF_RE.finditer(source):
            try:
                body, _ = extract_balanced(source, match.end() - 1, "{", "}")
            except ValueError:
                continue
            fields = split_top_level(body)
            if len(fields) < 5:
                continue
            name = first_string_literal(fields[1])
            if not name:
                continue
            table = normalize_table_reference(fields[4], aliases)
            table = None if table in {None, "nullptr", "NULL", "0"} else table
            definition = match.group("definition")
            module_def = ModuleDef(source=rel, name=name, table=table)
            module_defs[(rel, definition)] = module_def
            if table:
                table_modules[(rel, table)] = name

    for path, source in file_data.items():
        rel = path.relative_to(root).as_posix()
        variable_modules = module_state_for_source(source).variables
        variable_tables: dict[str, str] = {}

        for match in PYMODULE_CREATE_RE.finditer(source):
            module_def = module_defs.get((rel, match.group("definition")))
            if not module_def:
                continue
            variable = match.group("variable")
            variable_modules[variable] = module_def.name
            if module_def.table:
                variable_tables[variable] = module_def.table

        variable_modules = module_state_for_source(source, variable_modules).variables

        for match in PYMODULE_ADD_OBJECT_RE.finditer(source):
            public_name = variable_modules.get(match.group("child"))
            child_table = variable_tables.get(match.group("child"))
            if not public_name or not child_table:
                continue
            table_modules[(rel, child_table)] = public_name

        for match in PYMODULE_ADD_FUNCTIONS_RE.finditer(source):
            module_name = variable_modules.get(match.group("module"))
            table = normalize_table_reference(match.group("table"), aliases)
            if module_name and table:
                table_modules[(rel, table)] = module_name

    return module_defs, aliases, table_modules


def normalize_cpp_qualified_name(name: str) -> str:
    return "::".join(part.strip() for part in name.split("::"))


def cpp_namespace_for_source(rel_path: str) -> str | None:
    parts = rel_path.split("/")
    if len(parts) < 3 or parts[0] != "src":
        return None
    if parts[1] in {"App", "Base", "Gui"}:
        return parts[1]
    if parts[1] == "Mod" and len(parts) >= 4:
        if parts[3] == "Gui":
            return f"{parts[2]}Gui"
        return parts[2]
    return None


def contextual_cpp_type_name(rel_path: str, type_name: str) -> str | None:
    if "::" in type_name:
        return type_name
    namespace = cpp_namespace_for_source(rel_path)
    if not namespace:
        return None
    return f"{namespace}::{type_name}"


def cpp_type_name(expression: str) -> str | None:
    match = CPP_TYPE_NAME_RE.search(expression)
    if match:
        return normalize_cpp_qualified_name(match.group(1))
    match = re.search(
        r"((?:[A-Za-z_]\w*\s*::\s*)*[A-Za-z_]\w*)\s*::\s*type_object\s*\(\s*\)",
        expression,
    )
    if match:
        return normalize_cpp_qualified_name(match.group(1))
    return None


def collect_type_registrations(root: Path, files: Iterable[Path]) -> dict[str, list[str]]:
    registrations: dict[str, list[str]] = defaultdict(list)

    for path in files:
        rel = path.relative_to(root).as_posix()
        try:
            source = strip_comments(path.read_text(encoding="utf-8", errors="replace"))
        except OSError:
            continue

        module_vars = module_state_for_source(source).variables

        for _, type_expr, module_var, export_name in add_type_calls(source):
            module_name = module_vars.get(normalize_expr(module_var))
            type_name = cpp_type_name(type_expr)
            if not module_name or not type_name:
                continue
            public_name = f"{module_name}.{export_name}"
            keys = [type_name]
            context_name = contextual_cpp_type_name(rel, type_name)
            if context_name:
                keys.append(context_name)
            if "::" in type_name:
                keys.append(type_name.rsplit("::", 1)[-1])
            for key in dict.fromkeys(keys):
                if public_name not in registrations[key]:
                    registrations[key].append(public_name)

    return dict(registrations)


def public_type_target(public_name: str) -> PublicTypeTarget | None:
    if "." not in public_name:
        return None
    module_name, class_symbol = public_name.rsplit(".", 1)
    if not valid_identifier(class_symbol):
        return None
    return PublicTypeTarget(module_name=module_name, class_symbol=class_symbol)


def public_type_targets_for_context(
    context_name: str,
    methods: list[BindingMethod],
    type_registrations: dict[str, list[str]],
) -> list[PublicTypeTarget]:
    targets: list[PublicTypeTarget] = []
    candidate_keys = [context_name, f"{context_name}Py"]

    for method in methods:
        targets.extend(type_context_public_targets(method.source, context_name))
        owner = cxx_owner_hint(method.cxx_callable)
        if owner:
            candidate_keys.append(owner)

    for key in dict.fromkeys(candidate_keys):
        for public_name in type_registrations.get(key, []):
            target = public_type_target(public_name)
            if target:
                targets.append(target)

    return list(dict.fromkeys(targets))


def known_module_hint(rel_path: str, table: str) -> str | None:
    return KNOWN_PYMETHODDEF_MODULE_HINTS.get((rel_path, table))


def known_pycxx_module_hint(rel_path: str, module_name: str) -> str | None:
    return KNOWN_PYCXX_MODULE_HINTS.get((rel_path, module_name))


def cxx_owner_hint(cxx_callable: str) -> str | None:
    match = re.search(r"&\s*(?P<owner>[A-Za-z_]\w*(?:::[A-Za-z_]\w*)*)(?:<[^>]+>)?::", cxx_callable)
    if not match:
        return None
    return match.group("owner").rsplit("::", 1)[-1]


def inferred_pymethoddef_module(
    rel_path: str,
    table: str,
    table_modules: dict[tuple[str, str], str],
) -> str | None:
    hint = known_module_hint(rel_path, table)
    if hint:
        return hint

    candidates = {
        module_name
        for (source_path, table_name), module_name in table_modules.items()
        if source_path == rel_path and table_name == table
    }
    if len(candidates) == 1:
        return next(iter(candidates))

    candidates = {
        module_name for (_, table_name), module_name in table_modules.items() if table_name == table
    }
    if len(candidates) == 1:
        return next(iter(candidates))

    return None


def extract_pycxx_methods(root: Path, path: Path, source: str) -> list[BindingMethod]:
    rel = path.relative_to(root).as_posix()
    contexts = discover_contexts(source)
    methods: list[BindingMethod] = []

    for match in ADD_METHOD_RE.finditer(source):
        try:
            args, _ = extract_balanced(source, match.end() - 1, "(", ")")
        except ValueError:
            continue
        fields = split_top_level(args)
        if len(fields) < 2:
            continue
        python_name = first_string_literal(fields[0])
        if not python_name:
            continue
        kind, context = nearest_context(contexts, match.start())
        match match.group("kind"):
            case "add_keyword_method":
                method_kind: MethodKind = "keyword"
            case "add_noargs_method":
                method_kind = "noargs"
            case "add_varargs_method":
                method_kind = "varargs"
            case _:
                raise AssertionError("unexpected PyCXX method registration kind")
        cxx_callable = normalize_expr(fields[1])
        doc = normalize_doc(fields[2]) if len(fields) >= 3 else ""
        if kind == "unknown":
            owner = cxx_owner_hint(cxx_callable)
            if owner:
                kind = "python_type"
                context = owner
        inferred_module = None
        if kind == "pycxx_module":
            inferred_module = known_pycxx_module_hint(rel, context) or context

        methods.append(
            BindingMethod(
                family="pycxx_add_method",
                source=rel,
                line=line_number(source, match.start()),
                table=None,
                context_kind=kind,
                context_name=context,
                inferred_module=inferred_module,
                method_kind=method_kind,
                python_name=python_name,
                cxx_callable=cxx_callable,
                flags="",
                doc=doc,
                generated_source=generated_source(rel),
            )
        )

    return methods


def extract_pymethoddef_methods(
    root: Path,
    path: Path,
    source: str,
    table_modules: dict[tuple[str, str], str],
) -> list[BindingMethod]:
    rel = path.relative_to(root).as_posix()
    methods: list[BindingMethod] = []

    for match in PYMETHODDEF_RE.finditer(source):
        table = normalize_expr(match.group("table"))
        try:
            body, _ = extract_balanced(source, match.end() - 1, "{", "}")
        except ValueError:
            continue
        for entry in split_top_level(body):
            if not entry.startswith("{"):
                continue
            try:
                entry_body, _ = extract_balanced(entry, 0, "{", "}")
            except ValueError:
                continue
            fields = split_top_level(entry_body)
            if len(fields) < 3:
                continue
            python_name = first_string_literal(fields[0])
            if not python_name:
                continue
            cxx_callable = normalize_expr(fields[1])
            if cxx_callable in {"nullptr", "NULL", "0"}:
                continue
            flags = normalize_expr(fields[2])
            doc = normalize_doc(fields[3]) if len(fields) >= 4 else ""
            module_name = inferred_pymethoddef_module(rel, table, table_modules)

            methods.append(
                BindingMethod(
                    family="pymethoddef",
                    source=rel,
                    line=line_number(source, match.start() + body.find(entry)),
                    table=table,
                    context_kind="pymethoddef_table",
                    context_name=table,
                    inferred_module=module_name,
                    method_kind=flags_to_method_kind(flags),
                    python_name=python_name,
                    cxx_callable=cxx_callable,
                    flags=flags,
                    doc=doc,
                    generated_source=generated_source(rel),
                )
            )

    return methods


def flags_to_method_kind(flags: str) -> MethodKind:
    match flags:
        case value if "METH_KEYWORDS" in value:
            return "keyword"
        case value if "METH_NOARGS" in value:
            return "noargs"
        case _:
            return "varargs"


def collect_methods(root: Path, source_dir: Path) -> list[BindingMethod]:
    files = list(iter_source_files(root, source_dir))
    _, _, table_modules = collect_module_definitions(root, files)

    methods: list[BindingMethod] = []
    for path in files:
        try:
            source = strip_comments(path.read_text(encoding="utf-8", errors="replace"))
        except OSError:
            continue
        methods.extend(extract_pycxx_methods(root, path, source))
        methods.extend(extract_pymethoddef_methods(root, path, source, table_modules))

    return sorted(methods, key=lambda method: (method.source, method.line, method.python_name))


def group_methods(methods: list[BindingMethod]) -> tuple[
    dict[str, list[BindingMethod]],
    dict[str, list[BindingMethod]],
    dict[str, list[BindingMethod]],
]:
    module_methods: dict[str, list[BindingMethod]] = defaultdict(list)
    type_methods: dict[str, list[BindingMethod]] = defaultdict(list)
    unknown_methods: dict[str, list[BindingMethod]] = defaultdict(list)

    for method in methods:
        if method.inferred_module:
            module_methods[method.inferred_module].append(method)
        elif method.context_kind == "python_type":
            type_methods[method.context_name].append(method)
        else:
            key = f"{method.source}:{method.context_name}"
            unknown_methods[key].append(method)

    return module_methods, type_methods, unknown_methods


def public_type_context_index(
    methods: list[BindingMethod],
    type_registrations: dict[str, list[str]],
) -> dict[tuple[str, str], list[tuple[str, str]]]:
    _, type_methods, _ = group_methods(methods)
    index: dict[tuple[str, str], list[tuple[str, str]]] = defaultdict(list)

    for context_name, group in sorted(type_methods.items()):
        context_sources = sorted({method.source for method in group})
        for target in public_type_targets_for_context(context_name, group, type_registrations):
            key = (target.module_name, target.class_symbol)
            for source in context_sources:
                context_key = (source, context_name)
                if context_key not in index[key]:
                    index[key].append(context_key)

    return dict(index)


def module_names_from_methods(methods: list[BindingMethod]) -> set[str]:
    return {method.inferred_module for method in methods if method.inferred_module}


def group_type_methods_by_public_module(
    methods: list[BindingMethod],
    type_registrations: dict[str, list[str]],
) -> dict[str, list[PublicTypeGroup]]:
    _, type_methods, _ = group_methods(methods)
    grouped: dict[str, list[PublicTypeGroup]] = defaultdict(list)
    seen: set[tuple[str, str, str | None, tuple[str, ...]]] = set()

    for context_name, group in sorted(type_methods.items()):
        for target in public_type_targets_for_context(context_name, group, type_registrations):
            if not valid_identifier(target.class_symbol):
                continue
            if target.variable_symbol and not valid_identifier(target.variable_symbol):
                continue
            if any(not valid_identifier(base_symbol) for base_symbol in target.base_symbols):
                continue
            key = (
                target.module_name,
                target.class_symbol,
                target.variable_symbol,
                target.base_symbols,
            )
            if key in seen:
                continue
            seen.add(key)
            grouped[target.module_name].append(
                PublicTypeGroup(
                    class_symbol=target.class_symbol,
                    variable_symbol=target.variable_symbol,
                    base_symbols=target.base_symbols,
                    methods=group,
                )
            )

    return grouped


def module_names_from_type_methods(
    methods: list[BindingMethod],
    type_registrations: dict[str, list[str]],
) -> set[str]:
    return set(group_type_methods_by_public_module(methods, type_registrations))


def internal_type_context_reason(context_name: str, methods: list[BindingMethod]) -> str | None:
    reasons = {
        reason
        for method in methods
        if (reason := type_context_internal_reason(method.source, context_name)) is not None
    }
    if not reasons:
        return None
    if any(type_context_internal_reason(method.source, context_name) is None for method in methods):
        return None
    return "; ".join(sorted(reasons))


def unmapped_type_contexts(
    methods: list[BindingMethod],
    type_registrations: dict[str, list[str]],
) -> list[str]:
    _, type_methods, _ = group_methods(methods)
    return [
        context_name
        for context_name, group in sorted(type_methods.items())
        if not public_type_targets_for_context(context_name, group, type_registrations)
        and not internal_type_context_reason(context_name, group)
    ]
