# pyright: strict

"""Structured source-input readers for the stub generation pipeline.

This module turns curated Python-side inputs into normalized internal data:
- binding class declarations from CMake-registered binding ``.pyi`` files
- source-adjacent ``*.module.pyi`` signatures for module APIs
- source-adjacent plain ``.pyi`` type stubs for PyCXX classes

In the overall pipeline this sits between raw discovery and final rendering.
``discovery`` tells us what runtime registrations exist; this module loads the
curated source-side typing information that refines and documents them.

Keep this file focused on parsing source-adjacent stub inputs and binding
declarations. Stub rendering and AST merge behavior belong in other modules.
"""

from __future__ import annotations

import ast
from pathlib import Path
import re

from .discovery import (
    collect_type_registrations,
    contextual_cpp_type_name,
    public_type_context_index,
)
from .model import (
    BindingMethod,
    BindingClass,
    MODULE_STUB_PYI_SUFFIX,
    StubSignature,
    StubSignatureGroup,
    StubSignatureOverrides,
)
from .naming import valid_identifier
from .parsing import (
    decorator_kwargs,
    decorator_name,
    extract_balanced,
    iter_binding_pyi_files,
    iter_module_stub_pyi_files,
    iter_source_files,
    iter_type_stub_pyi_files,
    parse_python_source,
)


def binding_export_name(class_name: str, export_kwargs: dict[str, object]) -> str:
    name = export_kwargs.get("Name")
    if isinstance(name, str) and name:
        return name
    if class_name == "PyObjectBase":
        return class_name
    return f"{class_name}Py"


def fallback_public_name(rel_path: str, class_name: str) -> str | None:
    parts = rel_path.split("/")
    if len(parts) < 3 or parts[0] != "src":
        return None
    if parts[1] == "Base":
        return f"FreeCAD.Base.{class_name}"
    if parts[1] == "App":
        return f"FreeCAD.{class_name}"
    if parts[1] == "Gui":
        return f"FreeCADGui.{class_name}"
    if parts[1] == "Mod" and len(parts) >= 3:
        module_name = parts[2]
        if "Gui" in parts[3:4]:
            return f"{module_name}Gui.{class_name}"
        return f"{module_name}.{class_name}"
    return None


def public_names_for_class(
    rel_path: str,
    class_name: str,
    export_name: str,
    python_name: str | None,
    export_kwargs: dict[str, object],
    type_registrations: dict[str, list[str]],
) -> list[str]:
    candidate_keys: list[str] = []
    namespace = export_kwargs.get("Namespace")
    if isinstance(namespace, str) and namespace:
        candidate_keys.append(f"{namespace}::{export_name}")
    contextual_name = contextual_cpp_type_name(rel_path, export_name)
    if contextual_name:
        candidate_keys.append(contextual_name)

    for key in dict.fromkeys(candidate_keys):
        names = list(dict.fromkeys(type_registrations.get(key, [])))
        if names:
            return names

    fallback_name = fallback_public_name(rel_path, class_name)
    unqualified_names = list(dict.fromkeys(type_registrations.get(export_name, [])))
    if unqualified_names and fallback_name in unqualified_names and len(unqualified_names) == 1:
        return unqualified_names

    names: list[str] = []
    if python_name:
        names.append(python_name)
    if fallback_name and not names:
        names.append(fallback_name)
    return names


def parse_binding_class_file(
    root: Path,
    path: Path,
    type_registrations: dict[str, list[str]],
) -> list[BindingClass]:
    rel = path.relative_to(root).as_posix()
    tree = parse_python_source(path)
    if not tree:
        return []

    classes: list[BindingClass] = []
    for node in tree.body:
        if not isinstance(node, ast.ClassDef):
            continue

        export_kwargs: dict[str, object] = {}
        explicit_export = False
        for decorator in node.decorator_list:
            if decorator_name(decorator) == "export":
                explicit_export = True
                export_kwargs = decorator_kwargs(decorator)
                break

        python_name = export_kwargs.get("PythonName")
        python_name = python_name if isinstance(python_name, str) and python_name else None
        export_name = binding_export_name(node.name, export_kwargs)
        base_class = None
        if node.bases:
            base_class = ast.unparse(node.bases[0]).split("[", 1)[0].split(".")[-1]

        classes.append(
            BindingClass(
                source=rel,
                line=node.lineno,
                class_name=node.name,
                export_name=export_name,
                python_name=python_name,
                public_names=public_names_for_class(
                    rel, node.name, export_name, python_name, export_kwargs, type_registrations
                ),
                base_class=base_class,
                explicit_export=explicit_export,
            )
        )

    return classes


def collect_binding_classes(
    root: Path,
    source_dir: Path,
    type_registrations: dict[str, list[str]] | None = None,
) -> list[BindingClass]:
    if type_registrations is None:
        source_files = list(iter_source_files(root, source_dir))
        type_registrations = collect_type_registrations(root, source_files)

    classes: list[BindingClass] = []
    for path in iter_binding_pyi_files(root, source_dir):
        classes.extend(parse_binding_class_file(root, path, type_registrations))

    return sorted(classes, key=lambda klass: (klass.source, klass.line, klass.class_name))


def extracted_function_signature_parts(
    path: Path,
    owner_name: str,
    source: str,
    node: ast.FunctionDef,
) -> tuple[str, str, str | None]:
    if node.returns is None:
        raise ValueError(f"{path}: {owner_name}.{node.name} is missing a return annotation")
    definition = ast.get_source_segment(source, node) or ""
    if definition:
        start = definition.find("(")
        if start == -1:
            raise ValueError(f"{path}: {owner_name}.{node.name} has no parameter list")
        parameters, end = extract_balanced(definition, start, "(", ")")
        return_match = re.match(r"\s*->\s*(?P<returns>.+?)\s*:", definition[end:], re.DOTALL)
        if not return_match:
            raise ValueError(f"{path}: {owner_name}.{node.name} is missing a return annotation")
        returns = " ".join(return_match.group("returns").split())
    else:
        parameters = ast.unparse(node.args)
        returns = ast.unparse(node.returns)
    return parameters.strip(), returns, ast.get_docstring(node, clean=True)


def stub_signature_from_function_node(
    path: Path,
    source: str,
    class_symbol: str,
    node: ast.FunctionDef,
) -> StubSignature:
    parameters, returns, doc = extracted_function_signature_parts(
        path,
        class_symbol,
        source,
        node,
    )
    if parameters == "self":
        parameters = ""
    elif parameters.startswith("self,"):
        parameters = parameters.removeprefix("self,").lstrip()
    else:
        raise ValueError(f"{path}: {class_symbol}.{node.name} must be an instance method")
    return StubSignature(parameters, returns, class_symbol, doc)


def module_stub_signature_from_function_node(
    path: Path,
    source: str,
    module_name: str,
    node: ast.FunctionDef,
) -> StubSignature:
    parameters, returns, doc = extracted_function_signature_parts(path, module_name, source, node)
    if parameters.startswith(("self", "cls")):
        raise ValueError(f"{path}: {module_name}.{node.name} must not declare self or cls")
    return StubSignature(parameters, returns, doc=doc)


def append_module_signature_group(
    signatures: dict[tuple[str, str], tuple[StubSignatureGroup, Path]],
    module_name: str,
    function_name: str,
    signature: StubSignature,
    path: Path,
) -> None:
    append_signature_group(
        signatures,
        (module_name, function_name),
        signature,
        path,
        f"{module_name}.{function_name}",
    )


def append_type_signature_group(
    signatures: dict[tuple[str, str, str], tuple[StubSignatureGroup, Path]],
    module_name: str,
    class_symbol: str,
    method_name: str,
    signature: StubSignature,
    path: Path,
) -> None:
    append_signature_group(
        signatures,
        (module_name, class_symbol, method_name),
        signature,
        path,
        f"{module_name}.{class_symbol}.{method_name}",
    )


def append_signature_group[K](
    signatures: dict[K, tuple[StubSignatureGroup, Path]],
    key: K,
    signature: StubSignature,
    path: Path,
    display_name: str,
) -> None:
    if key in signatures:
        earlier_group, earlier_path = signatures[key]
        if earlier_path != path:
            raise ValueError(
                f"{path}: duplicate signature for {display_name}; "
                f"already defined in {earlier_path}"
            )
        signatures[key] = (earlier_group + (signature,), path)
        return
    signatures[key] = ((signature,), path)


def parse_module_stub_signature_overrides(
    root: Path,
    source_dir: Path,
) -> dict[tuple[str, str], tuple[StubSignatureGroup, Path]]:
    signatures: dict[tuple[str, str], tuple[StubSignatureGroup, Path]] = {}
    for path in sorted(iter_module_stub_pyi_files(root, source_dir)):
        module_name = path.name.removesuffix(MODULE_STUB_PYI_SUFFIX)
        if not module_name or not all(valid_identifier(part) for part in module_name.split(".")):
            raise ValueError(f"{path}: invalid module stub filename")
        source = path.read_text(encoding="utf-8")
        try:
            tree = ast.parse(source, filename=str(path))
        except SyntaxError as exc:
            raise ValueError(f"{path}: invalid stub override syntax: {exc}") from exc

        for node in tree.body:
            if not isinstance(node, ast.FunctionDef):
                continue
            signature = module_stub_signature_from_function_node(path, source, module_name, node)
            append_module_signature_group(signatures, module_name, node.name, signature, path)

    return signatures


def parse_type_stub_target(path: Path) -> tuple[str, str]:
    target = path.stem
    if not target or "." not in target:
        raise ValueError(f"{path}: invalid type stub filename")
    module_name, class_symbol = target.rsplit(".", 1)
    if not module_name or not class_symbol:
        raise ValueError(f"{path}: invalid type stub filename")
    if not all(valid_identifier(part) for part in module_name.split(".")):
        raise ValueError(f"{path}: invalid module name in type stub filename")
    if not valid_identifier(class_symbol):
        raise ValueError(f"{path}: invalid class symbol in type stub filename")
    return module_name, class_symbol


def parse_source_type_stub_signature_overrides(
    root: Path,
    source_dir: Path,
) -> dict[tuple[str, str, str], tuple[StubSignatureGroup, Path]]:
    signatures: dict[tuple[str, str, str], tuple[StubSignatureGroup, Path]] = {}
    for path in sorted(iter_type_stub_pyi_files(root, source_dir)):
        module_name, class_symbol = parse_type_stub_target(path)
        source = path.read_text(encoding="utf-8")
        try:
            tree = ast.parse(source, filename=str(path))
        except SyntaxError as exc:
            raise ValueError(f"{path}: invalid type stub syntax: {exc}") from exc

        class_nodes = [
            node
            for node in tree.body
            if isinstance(node, ast.ClassDef) and node.name == class_symbol
        ]
        if not class_nodes:
            raise ValueError(f"{path}: missing class {class_symbol!r} for type stub file")
        if len(class_nodes) > 1:
            raise ValueError(f"{path}: duplicate class {class_symbol!r} in type stub file")

        for item in class_nodes[0].body:
            if not isinstance(item, ast.FunctionDef):
                continue
            signature = stub_signature_from_function_node(path, source, class_symbol, item)
            append_type_signature_group(
                signatures,
                module_name,
                class_symbol,
                item.name,
                signature,
                path,
            )

    return signatures


def load_stub_signature_overrides(
    root: Path,
    source_dir: Path,
    methods: list[BindingMethod],
    type_registrations: dict[str, list[str]],
) -> StubSignatureOverrides:
    public_signatures = parse_source_type_stub_signature_overrides(root, source_dir)
    public_module_signatures = parse_module_stub_signature_overrides(root, source_dir)
    if not public_signatures and not public_module_signatures:
        return {}

    context_index = public_type_context_index(methods, type_registrations)
    method_keys = {
        (method.source, method.context_name, method.python_name)
        for method in methods
        if method.context_kind == "python_type"
    }
    module_method_index: dict[tuple[str, str], list[tuple[str, str, str]]] = {}
    for method in methods:
        if not method.inferred_module:
            continue
        module_method_index.setdefault((method.inferred_module, method.python_name), []).append(
            (method.source, method.context_name, method.python_name)
        )
    overrides: StubSignatureOverrides = {}
    errors: list[str] = []

    for public_key, (signature_override, path) in sorted(public_signatures.items()):
        module_name, class_symbol, method_name = public_key
        context_keys = context_index.get((module_name, class_symbol), [])
        if not context_keys:
            errors.append(f"{path}: no mapped PyCXX type context for {module_name}.{class_symbol}")
            continue

        matched_keys = [
            (source, context_name, method_name)
            for source, context_name in context_keys
            if (source, context_name, method_name) in method_keys
        ]
        if not matched_keys:
            contexts = ", ".join(
                f"{source}:{context_name}" for source, context_name in context_keys
            )
            errors.append(
                f"{path}: {module_name}.{class_symbol}.{method_name} is not registered "
                f"in mapped contexts: {contexts}"
            )
            continue

        for override_key in matched_keys:
            existing = overrides.get(override_key)
            if existing and existing != signature_override:
                errors.append(f"{path}: conflicting override for {override_key}")
                continue
            overrides[override_key] = signature_override

    for public_key, (signature_override, path) in sorted(public_module_signatures.items()):
        matched_keys = module_method_index.get(public_key, [])
        if not matched_keys:
            continue

        for override_key in matched_keys:
            existing = overrides.get(override_key)
            if existing and existing != signature_override:
                errors.append(f"{path}: conflicting override for {override_key}")
                continue
            overrides[override_key] = signature_override

    if errors:
        raise ValueError("invalid stub signature overrides:\n  " + "\n  ".join(errors))
    return overrides
