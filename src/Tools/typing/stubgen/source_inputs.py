# pyright: strict

"""Structured source-input readers for the StubGen pipeline.

This module parses source-adjacent Python inputs and binding declarations used
by API extraction and binding discovery. It owns source-specific parsing and
metadata interpretation; public API resolution belongs to
``python_api_model.resolve`` and output formatting belongs to the renderer.
"""

from __future__ import annotations

import ast
from dataclasses import dataclass
from pathlib import Path
from typing import TypeVar

from python_api_model.signatures import decorator_name

from .deprecation import literal_keyword_values, structured_deprecation_message
from .discovery import (
    collect_type_registrations,
    cpp_namespace_for_source,
    contextual_cpp_type_name,
    normalize_cpp_qualified_name,
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
    extract_balanced,
    iter_binding_pyi_files,
    iter_module_stub_pyi_files,
    iter_type_stub_pyi_files,
    load_source_files,
    parse_python_source,
)

K = TypeVar("K")


def deprecated_message_from_decorator(decorator: ast.expr) -> str | None:
    if decorator_name(decorator) != "deprecated":
        return None

    if not isinstance(decorator, ast.Call):
        raise ValueError("deprecated must be called with structured lifecycle metadata")
    if decorator.args:
        raise ValueError("structured deprecated() metadata accepts only keyword arguments")

    kwargs = literal_keyword_values(decorator, "deprecated() metadata")
    message = structured_deprecation_message(kwargs)
    if message is None:
        raise ValueError("deprecated() requires structured lifecycle metadata")
    return message


def deprecated_message_from_function_node(node: ast.FunctionDef) -> str | None:
    for decorator in node.decorator_list:
        if message := deprecated_message_from_decorator(decorator):
            return message
        if message == "":
            return ""
    return None


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
) -> tuple[str, ...]:
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
            return tuple(names)

    fallback_name = fallback_public_name(rel_path, class_name)
    unqualified_names = list(dict.fromkeys(type_registrations.get(export_name, [])))
    if unqualified_names and fallback_name in unqualified_names and len(unqualified_names) == 1:
        return tuple(unqualified_names)

    names: list[str] = []
    if python_name:
        names.append(python_name)
    if fallback_name and not names:
        names.append(fallback_name)
    return tuple(names)


def cpp_type_names_for_class(
    rel_path: str,
    class_name: str,
    export_kwargs: dict[str, object],
) -> tuple[str, ...]:
    """Return the C++ TypeId represented by a binding input class."""

    twin = export_kwargs.get("Twin")
    raw_name = twin if isinstance(twin, str) and twin else class_name
    raw_name = normalize_cpp_qualified_name(raw_name)
    if "::" in raw_name:
        return (raw_name,)

    namespace = export_kwargs.get("Namespace")
    if not isinstance(namespace, str) or not namespace:
        namespace = cpp_namespace_for_source(rel_path)
    if not namespace:
        return (raw_name,)
    return (f"{namespace}::{raw_name}",)


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
                cpp_type_names=cpp_type_names_for_class(rel, node.name, export_kwargs),
            )
        )

    return classes


def collect_binding_classes(
    root: Path,
    source_dir: Path,
    type_registrations: dict[str, list[str]] | None = None,
) -> list[BindingClass]:
    if type_registrations is None:
        source_files = load_source_files(root, source_dir)
        type_registrations = collect_type_registrations(root, source_files)

    classes: list[BindingClass] = []
    for path in iter_binding_pyi_files(root, source_dir):
        classes.extend(parse_binding_class_file(root, path, type_registrations))

    return sorted(classes, key=lambda klass: (klass.source, klass.line, klass.class_name))


@dataclass(frozen=True)
class BindingClassSource:
    """Parsed source location and class node for one binding declaration."""

    path: Path
    module: ast.Module | None
    class_node: ast.ClassDef | None


def binding_class_source(
    root: Path,
    binding_class: BindingClass,
) -> BindingClassSource:
    """Load the binding class declaration identified by source and line."""

    path = root / binding_class.source
    tree = parse_python_source(path)
    source_class = next(
        (
            node
            for node in (tree.body if tree is not None else [])
            if isinstance(node, ast.ClassDef)
            and node.name == binding_class.class_name
            and node.lineno == binding_class.line
        ),
        None,
    )
    return BindingClassSource(path=path, module=tree, class_node=source_class)


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
        returns = ast.unparse(node.returns)
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
    return StubSignature(
        parameters,
        returns,
        class_symbol,
        doc,
        deprecated_message=deprecated_message_from_function_node(node),
    )


def module_stub_signature_from_function_node(
    path: Path,
    source: str,
    module_name: str,
    node: ast.FunctionDef,
) -> StubSignature:
    parameters, returns, doc = extracted_function_signature_parts(path, module_name, source, node)
    if parameters.startswith(("self", "cls")):
        raise ValueError(f"{path}: {module_name}.{node.name} must not declare self or cls")
    return StubSignature(
        parameters,
        returns,
        doc=doc,
        deprecated_message=deprecated_message_from_function_node(node),
    )


def is_overload_declaration(node: ast.FunctionDef) -> bool:
    return any(
        decorator_name(decorator).rsplit(".", 1)[-1] == "overload"
        for decorator in node.decorator_list
    )


def validate_stub_function_group(
    path: Path,
    owner_name: str,
    function_name: str,
    declarations: list[tuple[ast.FunctionDef, StubSignature]],
) -> None:
    overloads = [is_overload_declaration(node) for node, _ in declarations]
    if len(declarations) == 1:
        if overloads[0]:
            raise ValueError(
                f"{path}: {owner_name}.{function_name} has a single @overload "
                "declaration; overloaded .pyi declarations require at least two"
            )
        return

    lines = ", ".join(str(node.lineno) for node, _ in declarations)
    if not all(overloads):
        raise ValueError(
            f"{path}: {owner_name}.{function_name} has multiple declarations "
            f"at lines {lines}; overloaded .pyi declarations must all use @overload"
        )

    seen: dict[tuple[str, str], int] = {}
    for node, signature in declarations:
        key = (signature.parameters, signature.returns)
        earlier_line = seen.get(key)
        if earlier_line is not None:
            raise ValueError(
                f"{path}: {owner_name}.{function_name} has duplicate overload "
                f"signatures at lines {earlier_line} and {node.lineno}"
            )
        seen[key] = node.lineno


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


def append_signature_group(
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

        parsed_functions: list[tuple[ast.FunctionDef, StubSignature]] = []
        function_groups: dict[str, list[tuple[ast.FunctionDef, StubSignature]]] = {}
        for node in tree.body:
            if not isinstance(node, ast.FunctionDef):
                continue
            signature = module_stub_signature_from_function_node(path, source, module_name, node)
            declaration = (node, signature)
            parsed_functions.append(declaration)
            function_groups.setdefault(node.name, []).append(declaration)

        for function_name, declarations in function_groups.items():
            validate_stub_function_group(path, module_name, function_name, declarations)

        for node, signature in parsed_functions:
            append_module_signature_group(signatures, module_name, node.name, signature, path)

    return signatures


def supplement_module_methods_from_stub_signatures(
    root: Path,
    source_dir: Path,
    methods: list[BindingMethod],
) -> list[BindingMethod]:
    module_signatures = parse_module_stub_signature_overrides(root, source_dir)
    if not module_signatures:
        return methods

    supplemented = list(methods)
    existing = {
        (method.inferred_module, method.python_name)
        for method in methods
        if method.inferred_module is not None
    }
    source_line_numbers: dict[tuple[str, str], int] = {}
    for path in iter_module_stub_pyi_files(root, source_dir):
        module_name = path.name.removesuffix(MODULE_STUB_PYI_SUFFIX)
        try:
            tree = ast.parse(path.read_text(encoding="utf-8"), filename=str(path))
        except (OSError, SyntaxError):
            continue
        for node in tree.body:
            if not isinstance(node, ast.FunctionDef):
                continue
            source_line_numbers.setdefault((module_name, node.name), node.lineno)

    for (module_name, function_name), (signatures, path) in sorted(module_signatures.items()):
        key = (module_name, function_name)
        if key in existing:
            continue
        rel = path.relative_to(root).as_posix()
        supplemented.append(
            BindingMethod(
                family="module_stub",
                source=rel,
                line=source_line_numbers.get(key, 1),
                table=None,
                context_kind="unknown",
                context_name=module_name,
                inferred_module=module_name,
                method_kind="varargs",
                python_name=function_name,
                cxx_callable="",
                flags="",
                doc=next((signature.doc for signature in signatures if signature.doc), "") or "",
                generated_source=False,
            )
        )
        existing.add(key)

    return supplemented


@dataclass(frozen=True)
class TypeStubTarget:
    module_name: str
    class_name: str


def parse_type_stub_target(path: Path) -> TypeStubTarget:
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
    return TypeStubTarget(module_name=module_name, class_name=class_symbol)


def parse_source_type_stub_signature_overrides(
    root: Path,
    source_dir: Path,
) -> dict[tuple[str, str, str], tuple[StubSignatureGroup, Path]]:
    signatures: dict[tuple[str, str, str], tuple[StubSignatureGroup, Path]] = {}
    for path in sorted(iter_type_stub_pyi_files(root, source_dir)):
        target = parse_type_stub_target(path)
        module_name = target.module_name
        class_symbol = target.class_name
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

        parsed_functions: list[tuple[ast.FunctionDef, StubSignature]] = []
        function_groups: dict[str, list[tuple[ast.FunctionDef, StubSignature]]] = {}
        for item in class_nodes[0].body:
            if not isinstance(item, ast.FunctionDef):
                continue
            signature = stub_signature_from_function_node(path, source, class_symbol, item)
            declaration = (item, signature)
            parsed_functions.append(declaration)
            function_groups.setdefault(item.name, []).append(declaration)

        for function_name, declarations in function_groups.items():
            validate_stub_function_group(path, class_symbol, function_name, declarations)

        for item, signature in parsed_functions:
            append_type_signature_group(
                signatures, module_name, class_symbol, item.name, signature, path
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
