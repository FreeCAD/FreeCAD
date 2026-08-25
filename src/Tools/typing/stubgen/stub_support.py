# pyright: strict

"""Output-only support fragments required by generated ``.pyi`` files.

Public declarations belong in ``PythonApiModel``. This module keeps the small
amount of syntax needed only to make generated stubs valid, such as private
protocols, type aliases, and ``TYPE_CHECKING`` declarations.
"""

from __future__ import annotations

import ast
from dataclasses import dataclass
from pathlib import Path

from .module_merge import (
    filtered_module_support_nodes,
    filtered_type_class_support_nodes,
    module_support_source,
    overlay_module_name,
    type_stub_support_sources,
    unparse_module_body,
)
from .model import BindingClass
from python_api_model.model import PythonApiModel
from .parsing import iter_module_stub_pyi_files, iter_type_stub_pyi_files
from .source_inputs import binding_class_source, parse_type_stub_target
from python_api_model.normalize import SOURCE_TYPE_ALIASES, normalize_source_type


def _merged_fragment_source(
    fragments: tuple[tuple[str, str], ...],
    key: str,
) -> str:
    """Parse, deduplicate, and unparse support fragments for one target."""

    nodes: list[ast.stmt] = []
    seen: set[str] = set()
    for name, source in fragments:
        if name != key or not source.strip():
            continue
        for node in ast.parse(source).body:
            node_key = ast.dump(node, include_attributes=False)
            if node_key in seen:
                continue
            seen.add(node_key)
            nodes.append(node)
    return unparse_module_body(nodes)


@dataclass(frozen=True)
class StubSupport:
    """Filtered support fragments indexed by their generated public module."""

    module_fragments: tuple[tuple[str, str], ...] = ()
    class_fragments: tuple[tuple[str, str], ...] = ()

    def module_source(self, module_name: str) -> str:
        return _merged_fragment_source(self.module_fragments, module_name)

    def class_source(self, module_name: str, class_name: str) -> str:
        qualified_name = f"{module_name}.{class_name}"
        return _merged_fragment_source(self.class_fragments, qualified_name)


def _module_public_symbols(model: PythonApiModel, module_name: str) -> set[str]:
    module = next((item for item in model.modules if item.name == module_name), None)
    if module is None:
        return set()
    return {
        symbol
        for symbol in (
            [function.name for function in module.functions]
            + [klass.name for klass in module.classes]
            + [attribute.name for attribute in module.attributes]
            + [alias.public_path.rsplit(".", 1)[-1] for alias in module.aliases]
        )
    }


def _class_public_symbols(
    model: PythonApiModel,
    module_name: str,
    class_name: str,
) -> set[str]:
    module = next((item for item in model.modules if item.name == module_name), None)
    if module is None:
        return set()
    klass = next((item for item in module.classes if item.name == class_name), None)
    if klass is None:
        return set()
    return {
        symbol
        for symbol in (
            [method.name for method in klass.methods]
            + [attribute.name for attribute in klass.attributes]
        )
    }


def _mapped_import_module(module: str | None, target_module: str) -> str:
    """Map source-stub imports to the module namespace emitted for ``target_module``.

    Source-adjacent stubs use the C++ binding layout, while generated stubs
    expose the public FreeCAD package layout. Unknown absolute imports are
    remapped into the generated target module so support declarations do not
    accidentally import a private source-side module.
    """

    if module is None:
        return target_module
    if module == "Base" or module.startswith("Base."):
        return "FreeCAD.Base"
    if module == "App" or module.startswith("App."):
        return "FreeCAD"
    if module == "Gui" or module.startswith("Gui."):
        return "FreeCADGui"
    if module.startswith("Part.App."):
        return "Part"
    if module.startswith("Part.Gui."):
        return "PartGui"
    if module.startswith(("PySide", "PySide2", "PySide6", "PyQt")):
        return module
    if module.startswith(("FreeCAD", "Qt", "numpy", "collections", "enum")):
        return module
    return target_module


def _normalized_module_support(
    source: str,
    module_name: str,
    existing_symbols: set[str],
) -> str:
    if not source.strip():
        return ""
    tree = ast.parse(source)
    nodes = filtered_module_support_nodes(tree.body, existing_symbols)
    normalized: list[ast.stmt] = []
    for node in nodes:
        if not isinstance(node, ast.ImportFrom):
            normalized.append(node)
            continue
        if node.module in {"typing", "typing_extensions", "collections.abc", "enum"}:
            normalized.append(node)
            continue
        if node.level:
            normalized.append(node)
            continue
        if node.module and node.module.endswith("Metadata"):
            continue
        mapped_module = _mapped_import_module(node.module, module_name)
        import_level = node.level
        if module_name == "FreeCAD":
            if mapped_module == "FreeCAD":
                mapped_module = None
                import_level = 1
            elif mapped_module.startswith("FreeCAD."):
                mapped_module = mapped_module.removeprefix("FreeCAD.")
                import_level = 1
        names = [
            alias
            for alias in node.names
            if alias.name != "PyObjectBase" and alias.asname not in SOURCE_TYPE_ALIASES
        ]
        if not names:
            continue
        normalized.append(ast.ImportFrom(module=mapped_module, names=names, level=import_level))
    return unparse_module_body(normalized)


def _normalize_support_annotations(source: str, module_name: str) -> str:
    if not source.strip():
        return ""
    tree = ast.parse(source)

    def annotation_node(text: str) -> ast.expr:
        parsed = ast.parse(f"_value: {text}").body[0]
        assert isinstance(parsed, ast.AnnAssign)
        return parsed.annotation

    for node in ast.walk(tree):
        if isinstance(node, ast.AnnAssign):
            node.annotation = annotation_node(
                normalize_source_type(ast.unparse(node.annotation), module_name)
                or ast.unparse(node.annotation)
            )
        elif isinstance(node, ast.arg) and node.annotation is not None:
            node.annotation = annotation_node(
                normalize_source_type(ast.unparse(node.annotation), module_name)
                or ast.unparse(node.annotation)
            )
        elif isinstance(node, (ast.FunctionDef, ast.AsyncFunctionDef)):
            if node.returns is not None:
                node.returns = annotation_node(
                    normalize_source_type(ast.unparse(node.returns), module_name)
                    or ast.unparse(node.returns)
                )
        elif isinstance(node, ast.ClassDef):
            node.bases = [
                annotation_node(
                    normalize_source_type(ast.unparse(base), module_name) or ast.unparse(base)
                )
                for base in node.bases
            ]
    return unparse_module_body(tree.body)


def collect_stub_support(
    root: Path,
    source_dir: Path,
    model: PythonApiModel,
    binding_classes: tuple[BindingClass, ...] = (),
    overlay_dir: Path | None = None,
) -> StubSupport:
    """Collect support syntax without adding it to the public API model."""

    module_fragments: list[tuple[str, str]] = []
    for path in sorted(iter_module_stub_pyi_files(root, source_dir)):
        module_name = path.name.removesuffix(".module.pyi")
        source = _normalized_module_support(
            module_support_source(path.read_text(encoding="utf-8")),
            module_name,
            _module_public_symbols(model, module_name),
        )
        if source.strip():
            module_fragments.append((module_name, source))

    class_fragments: list[tuple[str, str]] = []
    for path in sorted(iter_type_stub_pyi_files(root, source_dir)):
        target = parse_type_stub_target(path)
        module_name = target.module_name
        class_name = target.class_name
        module_source, source = type_stub_support_sources(
            path.read_text(encoding="utf-8"),
            class_name,
        )
        module_source = _normalized_module_support(
            module_source,
            module_name,
            _module_public_symbols(model, module_name),
        )
        if module_source.strip():
            module_fragments.append((module_name, module_source))
        support_tree = ast.parse(source) if source.strip() else ast.Module(body=[], type_ignores=[])
        source = unparse_module_body(
            filtered_type_class_support_nodes(
                support_tree.body,
                _class_public_symbols(model, module_name, class_name),
            )
        )
        source = _normalize_support_annotations(source, module_name)
        if source.strip():
            class_fragments.append((f"{module_name}.{class_name}", source))

    for binding_class in binding_classes:
        source = binding_class_source(root, binding_class)
        path = source.path
        tree = source.module
        source_class = source.class_node
        if tree is None:
            continue
        for public_name in binding_class.public_names:
            if "." not in public_name:
                continue
            module_name, class_name = public_name.rsplit(".", 1)
            module_source = _normalized_module_support(
                unparse_module_body([node for node in tree.body if node is not source_class]),
                module_name,
                _module_public_symbols(model, module_name) | {binding_class.class_name},
            )
            if module_source.strip():
                module_fragments.append((module_name, module_source))
            if source_class is None:
                continue
            class_source = unparse_module_body(
                filtered_type_class_support_nodes(
                    source_class.body,
                    _class_public_symbols(model, module_name, class_name),
                )
            )
            class_source = _normalize_support_annotations(class_source, module_name)
            if class_source.strip():
                class_fragments.append((f"{module_name}.{class_name}", class_source))

    if overlay_dir is not None and overlay_dir.exists():
        for path in sorted(overlay_dir.rglob("*.pyi")):
            module_name = overlay_module_name(path.relative_to(overlay_dir))
            if module_name is None:
                continue
            source = _normalized_module_support(
                module_support_source(path.read_text(encoding="utf-8")),
                module_name,
                _module_public_symbols(model, module_name),
            )
            if source.strip():
                module_fragments.append((module_name, source))

    return StubSupport(
        module_fragments=tuple(module_fragments),
        class_fragments=tuple(class_fragments),
    )
